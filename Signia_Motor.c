#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_Motor
 * \{
 *
 * \brief   Motor API Functions
 *
 * \details This module defines the API functions for controlling the motors. High
 *          level functions are provided for initiating and halting motor moves.
 *          Functions are also provided to allow the user to specify current based,
 *          or force based criteria for terminating the move or limiting its speed
 *          to prevent excessive current or force.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include <Common.h>
#include <math.h>
#include "L2_Adc.h"
#include "L3_Fpga.h"
#include "L3_GpioCtrl.h"
#include "L3_Motor.h"
#include "Signia_Motor.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER            (LOG_GROUP_MOTOR)    ///< Log Group Identifier

/**
 * \brief   TICKS_PER_AMP derivation
 *
 * \details The derivation of ticks / amp is based on the V 4.2.1 Schematic. The derivation is as follows:
 *              - The motor current shunt is 0.0143 ohms (0.02 in parallel w/ 0.05). This results in a voltage of 14.3mV @ 1A
 *              - This voltage is fed into the Allegro sense amp circuit, which has gain of 17.55 (Per Tony C)
 *              - 250.9mV (14.3mV * 17.55) is fed into a buffer amplifer/filter which divides the voltage in half &
 *                feeds it to the CPU A/D converter. (125.5mV)
 *              - The A/D converter is a 16 bit converter, converting against a reference of 2.5v. Thus:
 *              - (0.12555 / 2.5) * 65535 = 3290
 */
#define TICKS_PER_AMP                   (3290u)         ///< ADC ticks per Amp of motor current

/// \todo 11/16/2021 DAZ - This was passed by the caller in legacy, but it is not available in motor move API.
/// \todo 11/16/2021 DAZ - Create separate API call to set this? Set to fixed value for now.
/// \todo 11/16/2021 DAZ - Logging period was also specified by caller but not available in motor move API.
/// \todo 11/16/2021 DAZ - Currently hard coded at 10mS. May need API to set this.


// Default stream flags
#define STREAMFLAGS (MOT_STREAM_TIME_BIT | MOT_STREAM_SPEED_SETPOINT_BIT | \
                     MOT_STREAM_AVG_SPEED_BIT | MOT_STREAM_INST_SPEED_BIT | \
                     MOT_STREAM_POSITION_BIT | /*MOT_STREAM_INST_CURRENT_BIT  | */ \
                     MOT_STREAM_FILTER_CURRENT_BIT | MOT_STREAM_AVG_CURRENT_BIT | \
                     MOT_STREAM_PID_ERROR_BIT | MOT_STREAM_PID_ERRORSUM_BIT | \
                     MOT_STREAM_PID_OUTPUT_BIT | MOT_STREAM_PWM_OUTPUT_BIT | \
                     MOT_STREAM_RAW_SG_BIT | MOT_STREAM_SCALED_SG_BIT)


/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/

/******************************************************************************/
/*                                 Global Functions                           */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Set the current trip profile for the desired motor.
 *
 * \details The pointer to the desired current profile is stored in the motor
 *          parameter block of the desired motor. A null pointer may be given
 *          to disable current profile processing and use a fixed current trip
 *          value. This must be specified BEFORE each move, as the pointer is
 *          reset at the end of the move.
 *
 *          The current trip profile is defined in the structure MOT_CURTRIP_PROFILE.
 *          The structure contains the number of entries (not to exceed MOT_MAX_CURLIMIT)
 *          and 5 arrays. The arrays and their purposes are as follows:
 *              - TurnsPosition - This array contains the last position (in turns from 0)
 *                                that this zone (entry) is effective for.
 *              - CurrentTrip - This array contains the current trip value for the zone. This
 *                              may be expressed as an absolute value or a delta from some calculated
 *                              value (usually LongTermCurrentPeak) depending on the method below. The
 *                              value is specified in ADC counts.
 *              - Method - This array contains the processing method for the current trip value. Presently,
 *                         absolute and delta values are defined.
 *              - ZoneID - This array identifies the zone type, which affects current processing.
 *              - Kcoeff - This array contains a scale factor to be used with the delta method. Delta
 *                         current identified with the processing method is multiplied by this value
 *                         before calculating a final current trip value.
 *
 * \note    When a current trip profile is set, its values override the current
 *          trip value set by Signia_MotorStart().
 *
 * \note    This function relies upon the MOTOR_ID enum to insure that the motor
 *          argument is valid.
 *
 * \note    Since this function is called by a lower priority task than the motor servo
 *          loop, the pointer CANNOT be changed in the middle of motor processing. Thus,
 *          no synchronization is required.
 *
 * \param   MotorId  - Motor to set current profile for
 * \param   pProfile - Pointer to profile to set
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_MotorSetCurrentTripProfile(MOTOR_ID MotorId, MOT_CURTRIP_PROFILE *pProfile )
{
    // Set the Current Trip Profile pointer for the specified motor
    if (MotorId < MOTOR_COUNT)
    {
        L3_MotorGetPointer(MotorId)->pCurTripProfile = pProfile;
    }
}


/* ========================================================================== */
/**
 * \brief   Enable motor
 *
 * \details Enables specified motor by asserting the brake and deasserting the coast
 *          signals for the Allegro chip of the selected motor.
 *
 * \note    This function deasserts the Allegro reset signal for ALL motors,
 *          regardless of the one specified. All motors must have the reset
 *          deasserted so that the ADCs will work correctly. (Having any Allegro
 *          chip in reset loads the ADC reference voltage, resulting in invalid
 *          ADC readings)
 *
 * \note    This function does NOT enable power to the motor subsystem.
 *
 * \todo 04/07/2022 DAZ - These motor enable/disable functions are probably NOT
 *                        what is wanted for power control. Further investigation
 *                        should be performed to see if these functions should be
 *                        exposed to the application.
 *
 * \sa  L3_MotorEnable() for further details on enable operation.
 *
 * \param   MotorId - Motor ID
 *
 * \return  MM_STATUS - Status
 *
 * ========================================================================== */
MM_STATUS Signia_MotorEnable(MOTOR_ID MotorId)
{
    MM_STATUS Status;

    Status = MM_STATUS_INVALID_PARAM;
    if (MOTOR_STATUS_OK == L3_MotorEnable(MotorId))
    {
        Status = MM_STATUS_OK;      /// \todo 04/26/2022 DAZ - Just translating status. Use common status to reduce code?
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Disable motor
 *
 * \details Disables specified motor by deasserting the brake signal, and asserting
 *          coast and reset.
 *
 * \warning When disabling motors, all 3 should be disabled. Disabling any motor
 *          will cause all motor ADC readings to be invalid. Thus, disable should
 *          not be called when any motor is running. All motors are automatically
 *          disabled when all motor movement stops.
 *
 * \note    This function does NOT disable power to the motor subsystem.
 *
 * \todo 04/07/2022 DAZ - This function should probably unconditionally disable
 *                        all 3 motors. Parameter really isn't required.
 *
 * \param   MotorId - Motor ID to Disable
 *
 * \return  MM_STATUS - Status
 *
 * ========================================================================== */
MM_STATUS Signia_MotorDisable(MOTOR_ID MotorId)
{
    MM_STATUS Status;

    Status = MM_STATUS_INVALID_PARAM;
    if (MOTOR_STATUS_OK == L3_MotorDisable(MotorId))
    {
        Status = MM_STATUS_OK;
    }
    return Status;
}


/* ========================================================================== */
/**
 * \brief   Start the motor
 *
 * \details Start the motor with specified parameters. The speed is specified
 *          in RPM, at the ADAPTER. (Trilobe speed).
 * \n \n
 *          If no motor is presently running, and no sound is presently in progress,
 *          the FPGA will be reset prior to the start of the move. This will occur
 *          even if the FPGA has been refreshed as a result of communication error
 *          during a previous move.
 * \n \n
 *          If an FPGA error occurs DURING a move, all motors will be stopped with
 *          an appropriate stop status. (and P_MOTOR_STOP_INFO signals will be issued)
 *          When all motors have stopped, the FPGA will be REFRESHED via the I2C bus.
 *          (See L3_FpgaMgrRefresh)
 *
 * \note    Presently, MotorStart returns error if motor is invalid, distance to move is zero
 *          (or < MOT_POSITION_TOLERANCE), or motor is already running.
 * \n \n
 *          The start of move is logged when REQUESTED, not when actually processed.
 *          This is done so that the overhead of constructing the log entry does not
 *          impact the timing of the servo processing. As a result, the timestamp of
 *          the start record may be slightly earlier than the actual motor start.
 *
 * \warning As implemented, the destination position must be < 0x7FFFFFFFF ticks
 *          from the current position. For example, cannot move from Max position
 *          to Min position in a single move. The calculated displacement will overflow.
 *          Presently, this condition is not tested for, as this distance corresponds
 *          to 7,158,278 turns, FAR in excess of any move likely to be encountered.
 *
 * \param   MotorId         - Motor ID
 * \param   Position        - Destination position in ticks
 * \param   Speed           - Trilobe speed (RPM)
 * \param   TimeDelay       - Time delay before processing current trip (end stop detection)
 * \param   Timeout         - Max time to run motor (msec)
 * \param   CurrentTrip     - Current value to stop move when exceeded (In ADC counts)
 * \param   CurrentLimit    - Limit motor current (PWM counts)
 * \param   InitCurrent     - Initialize long term current filter
 * \param   MotorVoltage    - Motor voltage (12/15V)
 * \param   ReportInterval  - Periodically publish motor status(if > 0)
 *
 * \todo 05/06/2022 DAZ - ReportInterval not implemented. Should be removed.
 *
 * \return  MM_STATUS - Motor Status
 * \retval  MM_STATUS_ERROR - On Error
 * \retval  MM_STATUS_OK - On success
 *
 * ========================================================================== */
MM_STATUS Signia_MotorStart(MOTOR_ID MotorId, int32_t Position, uint16_t Speed, uint16_t TimeDelay, uint16_t Timeout, uint16_t CurrentTrip, uint16_t CurrentLimit, bool InitCurrent, MOTOR_SUPPLY MotorVoltage, uint32_t ReportInterval)
{
    MM_STATUS Status;                 // contains status
    MOTOR_CTRL_PARAM *pMotor;         // Pointer to Motor Info
    float32_t   Kp;                   // Temporary servo Kp
    float32_t   Ki;                   // Temporary servo Ki
    float32_t   Kd;                   // Temporary servo Kd
    float32_t   TempFloat;            // Temporary velocity calculation

    Status = MM_STATUS_ERROR;

    do
    {
        BREAK_IF(MotorId >= MOTOR_COUNT);                                           // Break if bad motor ID
        pMotor = L3_MotorGetPointer(MotorId);                                       // Get pointer to motor data
        BREAK_IF(pMotor->State != MOTOR_STATE_IDLE);                                // Break if motor already running
        BREAK_IF(labs(pMotor->MotorPosition - Position) < MOT_POSITION_TOLERANCE);  // No move required

        // If FPGA is refreshing, block here until refresh complete.
        while (L3_FpgaIsRefreshPending())
        {
            OSTimeDly(1);
        }

        // If an FPGA refresh just completed (due to a failure during move, for example)
        // a (redundant) reset will be performed upon the start of the 1st motor.

        pMotor->StreamFlags |= STREAMFLAGS;                         // Or in basic streaming flags
        pMotor->TargetSpeed = Speed * HANDLE_PARAM_GEAR_RATIO;      // Calculate rotor (winding) speed
        pMotor->TargetMoveDist = Position - pMotor->MotorPosition;  // Calculate displacement from current position & destiniation position
        pMotor->MotorCurrentLimit = CurrentLimit;
        pMotor->Timeout = Timeout;
        pMotor->TimeDelay = TimeDelay;
        pMotor->InitCurrent = InitCurrent;
        pMotor->MotorVoltage = MotorVoltage;
        pMotor->MotorCurrentTrip = CurrentTrip;
        pMotor->TargetShaftRpm = Speed;                 // Save shaft (trilobe) target speed
        pMotor->ZoneID = MOT_CTPROF_ZONE_NOTUSED;       // Initialize zone id
        pMotor->DataLogPeriod = 10;                     /// \todo 11/16/2021 DAZ - DataLogPeriod should be passed as parameter? Hard coded to 10mS. (1mS for debug?)

        // Initialize PID & speed filter
        pMotor->TableData.TableId = MotorVoltage;               // Specify the motor supply voltage
        pMotor->TableData.DataInput = pMotor->TargetShaftRpm;   // Specify the desired trilobe RPM to select PID gains

        PidInterpolation(&pMotor->TableData, &Kp, &Ki, &Kd);
        PidInit(&pMotor->Pid, Kp, Ki, Kd);
        PidSetTapsRpmThreshold(&pMotor->TableData, &pMotor->VelocityFilter.FilterSize, &pMotor->RpmThresh);

        /// \todo 09/24/2021 DAZ - Why not point to the velocity filter directly? (&pMotor->VelocityFilter) Function shouldn't know member names.
        // Initialize the velocity Filter data
        L3_MotorVelocityFilterClear(pMotor);

        // Initialize motor current
        FilterAverageInit(&pMotor->CurrentFilter, pMotor->CurrentFilterData, CURRENT_FILTER_SIZE);
        pMotor->MotorCurrent = 0;       // Clear motor current

        if (InitCurrent)
        {
            pMotor->CurrentLongTermAvg = 0;         // Initialize long term current average values
            pMotor->CurrentLongTermValley = 0;
            pMotor->CurrentLongTermPeak = 0;

            // If current trip profile in effect, set threshold positions in ticks
            if (pMotor->pCurTripProfile != NULL)
            {
                uint8_t Idx;
                for (Idx = 0; Idx < pMotor->pCurTripProfile->NumEntries; Idx++)
                {
                    pMotor->TicksPosition[Idx] = pMotor->pCurTripProfile->TurnsPosition[Idx] *
                        HANDLE_PARAM_TICKS_PER_REV * HANDLE_PARAM_GEAR_RATIO;
                }
            }
        }

        /* Force rotor target speed to tick boundary value.
           The number of rpms represented by a tick (resolution) is dependent on the filter size.
           The given speed is calculated as ticks based on the speed filter size (sampling window)
           This value is rounded to the nearest integer, and RPM is then calculated from it. This is the
           speed closest to the requested speed that can be represented by the present speed filter size.
        */

        // Calculate speed in ticks
        TempFloat = ((float32_t)pMotor->TargetSpeed * (float32_t)pMotor->VelocityFilter.FilterAvg.Length) / MOT_RPM_PER_TICK_PER_MSEC;

        // Round to nearest tick
        pMotor->TargetSpeed = (uint32_t)(TempFloat + MOT_FP_ROUNDING);

        // Calculate rotor speed in RPM
        pMotor->TargetSpeed = (pMotor->TargetSpeed * (uint32_t)MOT_RPM_PER_TICK_PER_MSEC) / pMotor->VelocityFilter.FilterAvg.Length;

        L3_MotorSetupStreamingVars(pMotor, pMotor->StreamFlags);        // Set up specified parameters for logging

        pMotor->ErrorDirTicks = 0;      // Reset motor direction error accumulator
        pMotor->LastEndStop = false;    // Reset endStop zone found flag

        // Log start of move
        Log(REQ, "Start Motor %d, Spd=%d, Time=%d, Pos=%d, Ticks=%d",   pMotor->MotorId,
                                                                        pMotor->TargetSpeed,
                                                                        pMotor->Timeout,
                                                                        pMotor->MotorPosition,
                                                                        pMotor->TargetMoveDist);

        pMotor->Request = MM_REQ_MOVE;  // Request move start
        Status = MM_STATUS_OK;
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Update the motor speed while it is running
 *
 * \details In addition to updating the motor speed, the PID gains are updated
 *          to values appropriate for the new speed.
 *
 * \note    This function is only useful while the motor is running, as MotorStart
 *          will overwrite any changes this function makes. Race conditions are avoided
 *          due to the fact that this function is not invoked by any task having a
 *          higher priority than the one invoking the motor servo (FPGA).
 *
 * \warning This function is not thread safe. (reentrant) It is expected that it
 *          will be invoked by no task other than the application. If this is not
 *          the case, steps must be taken to provide mutual exclusion.
 *
 * \warning This function does not change the speed averaging interval, so if
 *          speed is changed from a high speed to very low speed, speed regulation
 *          (as well as resolution) may be affected.
 *
 * \param   MotorId         - Motor ID
 * \param   Speed           - Trilobe RPM
 * \param   MotorVoltage    - Motor voltage (12/15V)
 *
 * \return  MM_STATUS - Motor Status
 * \retval  MM_STATUS_ERROR - On Error
 * \retval  MM_STATUS_OK - On success
 *
 * ========================================================================== */
/// \todo 03/24/2022 DAZ - This should probably post a request to servo to update the values. (Active Object style)
MM_STATUS Signia_MotorUpdateSpeed(MOTOR_ID MotorId, uint16_t Speed, MOTOR_SUPPLY MotorVoltage)
{
    MM_STATUS        Status;    // Return status
    MOTOR_CTRL_PARAM *pMotor;   // Pointer to Motor Info
    float32_t        TempFloat; // Temporary for forcing speed to tick boundary value

    Status = MM_STATUS_ERROR;               // Default Status

    do
    {
        BREAK_IF(MotorId >= MOTOR_COUNT);   // Quit if invalid motor

        pMotor = L3_MotorGetPointer(MotorId);                   // Point to motor data
        pMotor->TargetSpeed = Speed * HANDLE_PARAM_GEAR_RATIO;  // Update target armature (rotor) speed
        pMotor->MotorVoltage = MotorVoltage;
        pMotor->TableData.TableId = MotorVoltage;
        pMotor->TargetShaftRpm = Speed;
        pMotor->TableData.DataInput = pMotor->TargetShaftRpm;   // Update target trilobe speed
        PidInterpolation(&pMotor->TableData, &pMotor->Pid.Kp, &pMotor->Pid.Ki, &pMotor->Pid.Kd);

        // Update Integrator clamp values in accordance w/ new Ki
        pMotor->Pid.IntegratorHighClamp = PID_INTEGRATOR_HIGH / pMotor->Pid.Ki;
        pMotor->Pid.IntegratorLowClamp = PID_INTEGRATOR_LOW / pMotor->Pid.Ki;

        /* Force rotor target speed to tick boundary value.
           The number of rpms represented by a tick (resolution) is dependent on the filter size.
           The given speed is calculated as ticks based on the speed filter size (sampling window)
           This value is rounded to the nearest integer, and RPM is then calculated from it. This is the
           speed closest to the requested speed that can be represented by the present speed filter size.*/

        // Calculate speed in ticks
        TempFloat = ((float32_t)pMotor->TargetSpeed * (float32_t)pMotor->VelocityFilter.FilterAvg.Length) / MOT_RPM_PER_TICK_PER_MSEC;

        // Round to nearest tick
        pMotor->TargetSpeed = (uint32_t)(TempFloat + MOT_FP_ROUNDING);

        // Calculate rotor speed in RPM
        pMotor->TargetSpeed = (pMotor->TargetSpeed * (uint32_t)MOT_RPM_PER_TICK_PER_MSEC) / pMotor->VelocityFilter.FilterAvg.Length;

        Status = MM_STATUS_OK;

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Stop the motor
 *
 * \details This function requests a motor stop. The stop will be processed on
 *          the next motor servo tick. (1mS)
 *
 * \param   MotorId -  Motor ID to stop
 *
 * \return  MM_STATUS - Motor Status
 * \retval  MM_STATUS_ERROR - On Error
 * \retval  MM_STATUS_OK - On success
 *
 * ========================================================================== */
MM_STATUS Signia_MotorStop(MOTOR_ID MotorId)
{
    MM_STATUS        Status;      // Contains the status
    MOTOR_CTRL_PARAM *pMotor;     // Pointer to Motor Info

    Status = MM_STATUS_ERROR;     // Default Status

    do
    {
        BREAK_IF(MotorId >= MOTOR_COUNT);       // Quit if bad motor specified

        pMotor = L3_MotorGetPointer(MotorId);
        pMotor->Request = MM_REQ_STOP;          // Set stop request

        Status = MM_STATUS_OK;

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Set motor position
 *
 * \details If the specified motor is not running, this function sets the current
 *          position of the motor to the given value.
 *
 * \param   MotorId -  Motor ID
 * \param   Ticks   - Set position in units of ticks
 *
 * \return  MM_STATUS - Motor Status
 * \retval  MM_STATUS_ERROR - On Error
 * \retval  MM_STATUS_OK - On success
 *
 * ========================================================================== */
MM_STATUS Signia_MotorSetPos(MOTOR_ID MotorId, int32_t Ticks)
{
    MM_STATUS Status;             // Contains the status

    Status = MM_STATUS_ERROR;         // Default Status

    do
    {
        BREAK_IF(MotorId >= MOTOR_COUNT);                                   // Quit if invalid motor specified
        // NOTE: Do NOT use L3_MotorIsStopped() here as it references the FPGA status, which may be out of date.
        //       Instead, check the status of the motor state machine to ensure it is in the idle state.
        BREAK_IF(L3_MotorGetPointer(MotorId)->State != MOTOR_STATE_IDLE);   // Return if motor is running
        BREAK_IF(L3_MotorSetPos(MotorId, Ticks) != MOTOR_STATUS_OK);        // Return if error
        Status = MM_STATUS_OK;
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Get motor position
 *
 * \details Reads present motor position from the appropriate motor control block.
 *          This function is reentrant. There is no mutex required, as motor position
 *          is an atomic access.
 *
 * \param   MotorId   -  Motor ID
 * \param   pPosition -  Pointer to motor position returned in ticks
 *
 * \return  MM_STATUS - Motor Status
 * \retval  MM_STATUS_ERROR - On Error
 * \retval  MM_STATUS_OK - On success
 *
 * ========================================================================== */
MM_STATUS Signia_MotorGetPos(MOTOR_ID MotorId, int32_t *pPosition)
{
    MM_STATUS Status;               // Contains the status
    MOTOR_CTRL_PARAM *pMotor;       // Pointer to Motor data

    Status = MM_STATUS_ERROR;       // Default Status

    do
    {
        BREAK_IF(MotorId >= MOTOR_COUNT);   // Quit if bad motor

        pMotor = L3_MotorGetPointer(MotorId);
        *pPosition = pMotor->MotorPosition;

        Status = MM_STATUS_OK;

    } while (false);

    return Status;
}

/// \todo 12/13/2021 DAZ - Need APIs to set RDF sample rate, variables to log.
/* ========================================================================== */
/**
 * \brief   Determine if motor is stopped
 *
 * \details L3_MotorIsStopped() is called to determine if the specified
 *          motor is stopped.
 *
 * \param   MotorId -      Motor ID to check
 * \param   pHasStopped -  Pointer to stopped status. (True if stopped)
 *
 * \return  MM_STATUS - Motor Status
 * \retval  MM_STATUS_ERROR - On Error
 * \retval  MM_STATUS_OK - On success
 *
 * ========================================================================== */
/// \todo 04/04/2022 DAZ - Change to return stopped status rather than error, etc? Makes function more convenient to use.
MM_STATUS Signia_MotorIsStopped(MOTOR_ID MotorId, bool *pHasStopped)
{
    MOTOR_STATUS Status;                   // contains status
    MM_STATUS RetStatus;

    Status = MOTOR_STATUS_ERROR;
    RetStatus = MM_STATUS_ERROR;

    // Get the Motor Stopped Status
    Status = L3_MotorIsStopped(MotorId, pHasStopped);

    if (MOTOR_STATUS_OK == Status)
    {
        RetStatus = MM_STATUS_OK;
    }

    return RetStatus;
}

/* ========================================================================== */
/**
 * \brief   Check to see if any motor is running.
 *
 * \details This function calls L3_AnyMotorRunning() which returns true if all
 *          motors are in the idle state.
 *
 * \param   < None >
 *
 * \return  true if all motors in the idle state, false otherwise
 *
 * ========================================================================== */
bool Signia_AnyMotorRunning(void)
{
    return L3_AnyMotorRunning();
}

/* ========================================================================== */
/**
 * \brief   Set external motor processing
 *
 * \details This function will set a pointer to an external processing routine
 *          that will be called each millisecond the specified motor is running
 *          or stopping. The function's single argument is a pointer to the MOTOR_CTRL_PARAM
 *          structure for the specified motor. The external processing can be disabled by
 *          calling this function with a null parameter.
 *
 * \note    Motors can have different external functions, or may share the same function.
 *
 * \code
 *          // Usage:
 *
 *          void ExtProc(MOTOR_CTRL_PARAM *pMotor)
 *          {
 *              // ....
 *          }
 *
 *          Signia_MotorSetExternalProcess(MOTOR_ID0, &ExtProc);
 *
 * \endcode
 *
 * \param   MotorId - ID of motor to set processing routine for.
 * \param   pFunction - Pointer to function to execute.
 *
 * \return  Function status
 * \retval  MM_STATUS_OK - No error
 * \retval  MM_STATUS_ERROR - Invalid motor ID
 *
 * ========================================================================== */
MM_STATUS Signia_MotorSetExternalProcess(MOTOR_ID MotorId, MOTOR_PROCESS_FUNCTION pFunction)
{
    MM_STATUS Status;

    Status = MM_STATUS_ERROR;
    if (MotorId < MOTOR_COUNT)
    {
        Status = MM_STATUS_OK;
        L3_MotorGetPointer(MotorId)->pExternalProcess = pFunction;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Set motor streaming flags
 *
 * \details Sets the flags to log the specified parameters to the RDF file. These
 *          are added to the base flags logged for every move.
 *
 * \warning This function MUST be called BEFORE Signia_MotorStart() for each move
 *          that requires additional streaming, as SigniaMotorStart() also sets
 *          up the streaming for the move. The flags are reset at the end of
 *          each move.
 *
 * \note    Presently, the only streaming bit that is not part of the base list
 *          is MOT_STREAM_INST_CURRENT_BIT, which is not presently implemented.
 *          The base values that are streamed are:
 *              - MOT_STREAM_TIME_BIT
 *              - MOT_STREAM_SPEED_SETPOINT_BIT
 *              - MOT_STREAM_SPEED_BIT
 *              - MOT_STREAM_INST_SPEED_BIT
 *              - MOT_STREAM_POSITION_BIT
 *              - MOT_STREAM_FILT_CURRENT_BIT
 *              - MOT_STREAM_AVG_CURRENT_BIT
 *              - MOT_STREAM_PID_ERROR_BIT
 *              - MOT_STREAM_PID_ERRORSUM_BIT
 *              - MOT_STREAM_PID_OUTPUT_BIT
 *              - MOT_STREAM_PWM_OUTPUT_BIT
 *              - MOT_STREAM_RAW_SG_BIT
 *              - MOT_STREAM_SCALED_SG_BIT
 *
 * \param   MotorId - ID of motor to add parameters to.
 * \param   StreamFlags - Additional parameters to stream.
 *
 * \return  MM_STATUS_INVALID_PARAM if invalid Motor ID.
 *
 * ========================================================================== */
MM_STATUS Signia_SetRdfLog(MOTOR_ID MotorId, uint32_t StreamFlags)
{
    MM_STATUS Status;

    Status = MM_STATUS_INVALID_PARAM;

    do
    {
        BREAK_IF(MotorId >= MOTOR_COUNT);                           // Quit if invalid motor ID
        L3_MotorGetPointer(MotorId)->StreamFlags |= StreamFlags;    // Add specified stream flags for motor.
        Status = MM_STATUS_OK;
    } while (false);

    return Status;
}

/**
*\}
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

