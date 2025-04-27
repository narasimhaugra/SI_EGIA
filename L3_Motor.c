#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_Motor
 * \{
 *
 * \brief   This module contains functions for Motor control.
 *
 * \details The motor servo loop is implemented here. This module contains all the
 *          supporting routines required for the motor servo control loop.
 *          Routines are provided to start/stop move, read/set position, set speed.
 *          Outputs from the servo are made available via the Active Object
 *          publish/subscribe mechanism.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_Motor.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include <Common.h>
#include <math.h>
#include "L2_Adc.h"
#include "L3_Fpga.h"
#include "L3_FpgaMgr.h"
#include "L3_Motor.h"
#include "L3_Tone.h"
#include "L3_GpioCtrl.h"
#include "FaultHandler.h"
#include "TestManager.h"
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/
// Table of stop reasons, ordered by bit number.  These must be in the same order as MOT_STOP_STATUS_xxx defines. /// \todo 03/24/2022 DAZ - SHOULD THESE BE ENUM?
const char *StopStr[MOT_NUM_RDF_STOPINFO_MSGS] =
{
    "InPos",
    "Timeout",
    "Request",
    "EndStop",
    "ZeroRPM",
    "StrainGauge",
    "Obstruction",
    "CurTgtLoad",
    "Fatal",
    "NoStaples",
    "Overrun",
    "DirErr",           // Motor direction error
    "FpgaSpiErr"        // FPGA SPI error - force refresh
};

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_MOTOR)       ///< Log Group Identifier

#define MOTOR_REG_SYNC_PERIOD   (FPGA_SYNC_PERIOD + 2u) ///< Initial motor register sync wait
#define ALLEGRO_STABILIZE_TIME  (50u)                   ///< Time to allow allegro chips to stabilize after enabling.
#define MOTOR_CUR_PWM_MAX       (0x1FFu)                ///< Full scale for motor current limit PWM

#define MOTOR_BLOCK_BASE        (4u)                    ///< Motor register bank base
#define MOTOR_REG(Motor, Reg)   ((FPGA_REG)(MOTOR_BLOCK_BASE + ((uint8_t)(Motor) * MOTOR_REG_COUNT) + (uint8_t)(Reg))) ///< Motor register by motor ID

/// \todo 05/16/2022 DAZ - Define register bits as enum?
#define FPGA_CFG_MOTOR_EN_BIT           (0x0u)          ///< Motor enable bit in FPGA control register
#define FPGA_CFG_ADC_SRC_BIT            (0x1u)          ///< ADC trigger select bit in FPGA control register
#define FPGA_CFG_MOTOR_TICK_HIGH_BIT    (0x2u)          ///< 12 tick/turn select bit in FPGA control register (0 = 6 ticks/turn)

#define MOTOR_RESIDUAL_MOVE_MAX         (30u)           ///< Maximum allowed residual move after motor is stopped as per ID 294008.
#define MOTOR_STALL_LIMIT               (4u)            ///< Motor stall limit
#define UINT32_SIGN_MASK                (0x80000000u)   ///< Mask for uint32_t sign bit
#define DIRECTION_ERROR_DIST            (30u)           ///< Wrong way distance to trigger move error
#define MAX_TICKS_PER_MS                (10u)           ///< Maximum reasonable movement per mS. (~50000 RPM)
#define MOT_CURSETTLE_TIME              (6u)            ///< Motor Current Settling Time (loop ticks)
#define MOT_MAX_PWM                     (0x1FFu)        ///< Maximum motor drive (100% PWM)
#define MOT_STALL_RPM                   (10u)           ///< Motor stall speed (at armature/rotor)
#define MOT_STALL_PWM                   (450u)          ///< Motor stall PWM
#define MOT_STALL_TIME                  (200u)          ///< Motor stall time (in mS)

#define FPGA_REFRESH_TIME               (20u)           ///< Time in mS to allow Allegro chips to stabilize before allowing motor move (on FPGA refresh)

// Utility macros to access bits in byte/word/long memory
#define MODIFY_BIT(Src, Bit, Value)     ((Value) ? (Src) | (0x1 << (Bit)) : (Src) & ~(0x1 << (Bit))) ///< Set specified bit to specified value
#define GET_BIT(Value, Bit)             ((Value) & (0x1 << (Bit)) ? true : false)                    ///< Get specified bit value

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/// Shadow copies of FPGA control register and motor status / control registers.
typedef struct
{
    uint8_t Config;                     ///< Overall FPGA Control register
    uint8_t ControlReg[MOTOR_COUNT];    ///< Motor control register value
    uint8_t StatusReg[MOTOR_COUNT];     ///< Motor status register value
} MOTOR_CB;

/// These are offsets into a table that contains the FPGA addresses. These are used with the MOTOR_REG macro. See its usage with L3_FpgaReadReg / L3_FpgaWriteReg.
typedef enum
{
    MOTOR_REG_CONTROL,              ///< Offset ID - Motor control
    MOTOR_REG_CURR_PWM,             ///< Offset ID - Motor current limit PWM
    MOTOR_REG_VEL_PWM,              ///< Offset ID - Motor drive PWM
    MOTOR_REG_STATUS,               ///< Offset ID - Motor status
    MOTOR_REG_POSITION,             ///< Offset ID - Motor position
    MOTOR_REG_PERIOD,               ///< Offset ID - Motor period (19.5nS counts between position ticks - speed)
    MOTOR_REG_DELTA_COUNT,          ///< Offset ID - Motor ticks from last read
    MOTOR_REG_COUNT                 ///< Number of motor registers in FPGA
} MOTOR_REG;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
static const ADC_INSTANCE AdcSel[MOTOR_COUNT] = { ADC0, ADC3, ADC2 };   // Motor ID - ADC channel translation /// \todo 10/08/2021 DAZ - This should be moved to a central place. It is duplicated in multiple places.

// PID Table for 12VDC Motor Voltage
static const PID_INTERP_TABLE  MotorPidInterpTable12V =
{
    {    25,     49,     50,     99,    100,    149,    150,    500,   1000,   1600 },   ///< RPM
    { 0.080,  0.100,  0.100,  0.110,  0.110,  0.110,  0.055,  0.150,  0.270,  0.285 },   ///< Proportional gain
    { 0.0011, 0.0016, 0.0016, 0.0021, 0.0021, 0.0030, 0.0035, 0.0083, 0.0155, 0.0255},   ///< Integral gain
    { 0.000,  0.000,  0.000,  0.000,  0.000,  0.000,  0.000,  0.000,  0.000,  0.000 },   ///< Differential gain
    {   128,    128,     96,     96,     64,     64,     32,     32,     32,     32 },   ///< Filter taps
    {    78,     78,     78,     78,     78,     78,    156,    156,    156,    156 },   ///< RPM error threshold
};

// PID Table for 15VDC Motor Voltage
static const PID_INTERP_TABLE  MotorPidInterpTable15V =
{
    {    25,     49,     50,     99,    100,    149,    150,    500,   1000,   1600 },   ///< RPM
    { 0.090,  0.100,  0.100,  0.110,  0.110,  0.110,  0.055,  0.150,  0.270,  0.285 },   ///< Proportional gain
    { 0.0011, 0.0014, 0.0014, 0.0020, 0.0020, 0.0029, 0.0040, 0.0090, 0.0163, 0.0268},   ///< Integral gain
    { 0.000,  0.000,  0.000,  0.000,  0.000,  0.000,  0.000,  0.000,  0.000,  0.000 },   ///< Differential gain
    {   128,    128,     96,     96,     64,     64,     32,     32,     32,     32 },   ///< Filter taps
    {    78,     78,     78,     78,     78,     78,    156,    156,    156,    156 },   ///< RPM error threshold
};

// Since these signals do not contain variable data, they can be declared as const in flash.
// This avoids the overhead and error checking connected with AO_EvtNew().
static const QEvt MotorsIdle   = { P_MOTOR_IDLE_SIG,   0, 0 };          // All motors stopped signal
static const QEvt MotorsMoving = { P_MOTOR_MOVING_SIG, 0, 0 };          // Any motor moving signal

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static MOTOR_CB         MotorControlBlock;              // Shadow FPGA control and Motor status & control registers. /// \todo 03/24/2022 DAZ - Better name for this? Not a control block.
MOTOR_CTRL_PARAM        MotorCtrlParam[MOTOR_COUNT];    // Motor control data for each motor.
static uint16_t         UniqueNumber;                   // Unique # for naming RDF files.
static uint16_t         RawStrain;                      // UnScaled strain gauge value (ADC counts)
static uint16_t         ScaledStrain;                   // Scaled, unfiltered strain gauge value (LBS)
static uint8_t          RefreshTimer;                   // FPGA refresh timer
static bool             AnyMotorsOn;                    // True if any motors on this tick
static bool             LastAnyMotorsOn = false;        // Last value of AnyMotorsOn. True if any motor not in idle state.
static bool             MotorInitDone  = false;         // Motor initalization flag.
static bool             Motor12vTrig = false;           // True if 12v motor supply has been triggered this cycle

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void StartMotor(MOTOR_CTRL_PARAM *pMotor);
static void MotorStateMachine(MOTOR_ID MotorId);
static void ProcessIdleState(MOTOR_CTRL_PARAM *pMotor);
static void ProcessRunningState(MOTOR_CTRL_PARAM *pMotor);
static void ProcessStoppingState(MOTOR_CTRL_PARAM *pMotor);
static void MotorUpdatePosition(MOTOR_CTRL_PARAM *pMotor);
static void MotorUpdateSpeed(MOTOR_CTRL_PARAM *pMotor);
static void MotorUpdatePwm(MOTOR_CTRL_PARAM *pMotor);
static void MotorUpdateCurrent(MOTOR_CTRL_PARAM *pMotor);
static void MotorUpdateLTPV(MOTOR_CTRL_PARAM *pMotor);

static MOTOR_STATUS MotorWriteSignal(MOTOR_ID MotorId, MOTOR_SIGNAL Signal, bool Value);
static MOTOR_STATUS MotorReadSignal(MOTOR_ID MotorId, MOTOR_SIGNAL Signal, bool *pValue);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
//
// Motor streaming functions
//
/* ========================================================================== */
/**
 * \brief   Create an RDF record write signal
 *
 * \details This function gathers all the writeable data to the RDF file
 *          and sends a signal to write it to the log file.
 *
 * \param   pMotor - Pointer to the motor control parameters
 *
 * \return  None
 *
 * ========================================================================== */
static void RdfMotorDataWrite(MOTOR_CTRL_PARAM *pMotor)
{
    uint32_t Now;

    do
    {
        BREAK_IF(NULL == pMotor->Rdf);      // No RDF object available - quit now

        Now = SigTime();
        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_TIME,             &Now);                       // Present time
        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_SPEED_SETPOINT,   &pMotor->TargetSpeed);       // Speed setpoint
        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_AVG_SPEED,        &pMotor->MotorAvgSpeed);     // Armature speed (unsigned)
        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_INST_SPEED,       &pMotor->MotorInstSpeed);    // Instantaneous Armature speed (FPGA)
        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_POSITION,         &pMotor->MotorPosition);     // Position

        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_FILTER_CURRENT,   &pMotor->MotorCurrentRaw);   // Raw (filtered) motor current
        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_AVG_CURRENT,      &pMotor->MotorCurrent);      // Averaged motor current

        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_PID_ERROR,        &pMotor->Pid.Error);
        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_PID_ERRORSUM,     &pMotor->Pid.ErrorSum);
        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_PID_OUTPUT,       &pMotor->Pid.Output);
        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_PWM_OUTPUT,       &pMotor->Pwm);

        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_RAW_SG,           &RawStrain);                 // Only one strain gauge is available. May be logged with any motor move.
        RdfVariableWrite(pMotor->Rdf, MOT_STREAM_SCALED_SG,        &ScaledStrain);              // Only one strain gauge is available. May be logged with any motor move.

// NOTE: This is not currently implemented.It is included here for completeness
//        RdfVariableWrite(&pMotor->Rdf,     MOT_STREAM_INST_CURRENT,     &pMotor->?);            // Instantaneous (unfiltered) current
        RdfWriteData(pMotor->Rdf);

    } while (false);
}

//
// Move control functions
//

/* ========================================================================== */
/**
 * \brief   Process the specified current limit profile (if active) and set
 *          the motor current trip appropriately
 *
 * \details This internal function determines whether a Current Limit Profile is
 *          active, and if so, gets the appropriate Current Trip and zone values
 *          from the specified table based on the present motor position. For details
 *          on the current trip table, see the definition of the type MOT_CURTRIP_PROFILE.
 *
 * \param   pMotor - Motor control variables.
 *
 * \return  None
 *
 * ========================================================================== */
static void MotorSetCurrentTrip(MOTOR_CTRL_PARAM *pMotor)
{
    uint8_t             Index;            // Index for stepping through table
    uint8_t             NumEntries;       // Table size in entries
    MOT_CURTRIP_METHOD  *pMethod;         // Method of determining current trip value (0-Abs, 1-Delta, 2-Slope)
    MOT_CURTRIP_ZONE    *pZoneID;         // Processing zone (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused)
    float32_t           *pKcoeff;         // Pointer to Current delta coefficient (0.0 - 2.0)
    uint16_t            *pCurLimit;       // Pointer to Current Trip value (absolute/delta depending on method)
    uint16_t            DeltaCur;         // Calculated delta current
    int32_t             *pTicksPos;       // Pointer to zone start position in ticks
    int32_t             CurTicksPos;      // Current position in ticks

    if (NULL != pMotor->pCurTripProfile)
    {
        NumEntries = pMotor->pCurTripProfile->NumEntries;           // Set number of table entries to search
        pCurLimit = &pMotor->pCurTripProfile->CurrentTrip[0];       // Point to initial current trip
        pMethod = &pMotor->pCurTripProfile->Method[0];              // ... method of determining trip
        pZoneID = &pMotor->pCurTripProfile->ZoneID[0];              // ... zone ID
        pKcoeff = &pMotor->pCurTripProfile->Kcoeff[0];              // ... trip calculation coefficient
        pTicksPos = &pMotor->TicksPosition[0];                      // ... position threshold for determining processing
        CurTicksPos = labs(pMotor->MotorPosition);                  // Get present displacement from 0.

        // Locate table entry for present position
        for (Index = 0; Index < NumEntries; Index++)
        {
            //**  If the present tick position <= tick position in table
            if (CurTicksPos <= *pTicksPos++)
            {
                pMotor->MotorCurrentTrip = *pCurLimit;      // Set Current trip from Table
                pMotor->ZoneID = *pZoneID;                  // Get Zone ID
                pMotor->Kcoeff = *pKcoeff;                  //** Get K Coefficient

                if (*pMethod == MOT_CTPROF_METH_DELTA)
                { // Value in Table is a Delta Current Limit above the Long Term Peak
                    DeltaCur = *pCurLimit;
                    if (pMotor->ZoneID == MOT_CTPROF_ZONE_ENDSTOP)    // In an end stop zone
                    {
                        // In endstop zone. Calculate delta from lowest to highest current in learning
                        // zone. Multiply by specified coefficient.
                        DeltaCur = (uint16_t)((pMotor->CurrentLongTermPeak - pMotor->CurrentLongTermValley) * pMotor->Kcoeff);
                        if (DeltaCur < *pCurLimit)
                        {
                            DeltaCur = *pCurLimit;  // Calculated delta < table value. Use table value.
                        }
                    }
                    pMotor->MotorCurrentTrip = pMotor->CurrentLongTermPeak + DeltaCur;  // Set current trip value
                    pMotor->UsingDelta = true;
                }
                else
                {
                    pMotor->UsingDelta = false;
                }
                break;
            }
            pCurLimit++;
            pMethod++;
            pZoneID++;
            pKcoeff++;
        }

        // If present tick position > max table entry use max table entry
        if (Index == NumEntries)
        {
            pCurLimit--;
            pMethod--;
            pZoneID--;
            pKcoeff--;

            pMotor->ZoneID = *pZoneID;              //  Get Zone ID
            pMotor->MotorCurrentTrip = *pCurLimit;  //  Set current limit from Table
            pMotor->Kcoeff = *pKcoeff;              //  Set K Coeff from Table
            if (*pMethod == MOT_CTPROF_METH_DELTA)
            { // Value in Table is a Delta Current Limit above the Long Term Avg
                DeltaCur = *pCurLimit;
                if (pMotor->ZoneID == MOT_CTPROF_ZONE_ENDSTOP)
                {
                    DeltaCur = (INT16U)((pMotor->CurrentLongTermPeak - pMotor->CurrentLongTermValley) * pMotor->Kcoeff);
                    if (DeltaCur < *pCurLimit)
                    {
                        DeltaCur = *pCurLimit;
                    }
                }
                pMotor->MotorCurrentTrip = pMotor->CurrentLongTermPeak + DeltaCur;
                pMotor->UsingDelta = true;
            }
            else
            {
                pMotor->UsingDelta = false;
            }

            if (pMotor->ZoneID == MOT_CTPROF_ZONE_ENDSTOP)
            {
                pMotor->LastEndStop = true;          // End stop is the last entry in the table
            }
        }

        // Reset the long term peak & valley when entering a learning zone:
        if ((pMotor->ZoneID == MOT_CTPROF_ZONE_LEARNING) &&
            (pMotor->PrevZoneID != MOT_CTPROF_ZONE_LEARNING))
        {
            pMotor->CurrentLongTermPeak = 0;
            pMotor->CurrentLongTermValley = 0;
        }

        pMotor->PrevZoneID = pMotor->ZoneID;
    }
}

/* ========================================================================== */
/**
 * \brief   Set motor pwm
 *
 * \details Schedules the writing of the PWM value to the FPGA.
 *
 * \note    This function is used to refresh the FPGA to prevent motor stop
 *          due to FPGA motor watchdog expiry.
 *
 * \param   MotorId - Motor ID
 * \param   Pwm - Motor pwm
 *
 * \return  MOTOR_STATUS - Status
 *
 * ========================================================================== */
static MOTOR_STATUS MotorSetPwm(MOTOR_ID MotorId, uint16_t Pwm)
{
    MOTOR_STATUS Status;        // Return status variable
    bool FpgaStatus;            // FPGA access result

    Status = MOTOR_STATUS_INVALID_PARAM;

    // Validate motor ID
    if (MotorId < MOTOR_COUNT)
    {
        FpgaStatus = L3_FpgaWriteReg(MOTOR_REG(MotorId, MOTOR_REG_VEL_PWM), Pwm);
        Status = (FpgaStatus) ? MOTOR_STATUS_ERROR : MOTOR_STATUS_OK;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Perform time delayed motor processing
 *
 * \details This function mantains the LongTermCurrent parameters (average, peak,
 *          and valley), and performs current trip checking, setting the appropriate
 *          stop status as required. This function is called after an initial startup
 *          delay in the move to allow for current to settle.
 *
 * \param   pMotor - Pointer to motor control parameters (task extended data)
 *
 * \return  None
 *
 * ========================================================================== */
static void MotorTimeDelayedProcessing(MOTOR_CTRL_PARAM *pMotor)
{
    if (pMotor->CurrentLongTermAvg == 0)
    {
        pMotor->CurrentLongTermAvg = pMotor->MotorCurrent;          // Start average
    }
    else
    {
        // Average in new data
        pMotor->CurrentLongTermAvg = (((uint32_t)pMotor->CurrentLongTermAvg * (MOT_LTA_FILTER - 1)) +
                                      pMotor->MotorCurrent) / MOT_LTA_FILTER;
    }

    // In learning zone
    if ((MOT_CTPROF_ZONE_LEARNING == pMotor->ZoneID) ||
        (MOT_CTPROF_ZONE_INTERLOCK == pMotor->ZoneID))
    {
        MotorUpdateLTPV(pMotor);    // Update Long Term current Peak/Valley
    }

    // check for endstop
    /// \todo 04/01/2022 DAZ - NOTE: Legacy disabled current trip if the value was 0. I'm leaving it in. Disabling current trip
    /// \todo 04/01/2022 DAZ -       entirely seems like a *BAD* idea. Set it to a high value if you need to.
    if (pMotor->MotorCurrent > pMotor->MotorCurrentTrip)
    {
        pMotor->StopStatus |= MOT_STOP_STATUS_CURRENT_ES;
    }
}

/* ========================================================================== */
/**
 * \brief   Function to Publish the Stop signal
 *
 * \details Publish the signal P_MOTOR_0_STOP_INFO_SIG, P_MOTOR_1_STOP_INFO_SIG,
 *          P_MOTOR_2_STOP_INFO_SIG
 *
 * \param   pMotor      - Pointer to Motor control information
 *
 * \return  None
 *
 * ========================================================================== */
static void MotorPublishStopSig(MOTOR_CTRL_PARAM *pMotor)
{
    static const SIGNAL StopInfoSig[MOTOR_COUNT] = { P_MOTOR_0_STOP_INFO_SIG, P_MOTOR_1_STOP_INFO_SIG, P_MOTOR_2_STOP_INFO_SIG };   /* Motor to Signal array */

    char StopCause[64];                 // String to store stop reason(s)
    uint16_t Mask;                      // Mask to isolate stop status bits
    uint8_t Idx;                        // Index to step through stop status bits

    QEVENT_MOTOR_STOP_INFO *pStopSig;   // Motor Stop signal pointer

    pStopSig = AO_EvtNew(StopInfoSig[pMotor->MotorId], sizeof(QEVENT_MOTOR_STOP_INFO));

    if (NULL != pStopSig)
    {
        /* Update the signal and publish */
        pStopSig->MotorNum = pMotor->MotorId;                               // Return motor number
        pStopSig->Position = pMotor->MotorPosition;                         // Return absolute position
        pStopSig->TargetShaftRpm = pMotor->TargetShaftRpm;                  // Return target shaft speed (in case it has changed during move)
        pStopSig->StopStatus = pMotor->StopStatus;                          // Return stop status
        pStopSig->ElapsedTime = pMotor->ElapsedTime;                        // Return  move elapsed time
        pStopSig->CurrentLongTermAvg = pMotor->CurrentLongTermAvg;          // Return long term current average
        pStopSig->CurrentLongTermPeak = pMotor->CurrentLongTermPeak;        // Return long term current average peak
        pStopSig->CurrentLongTermValley = pMotor->CurrentLongTermValley;    // Return long term current average valley
        pStopSig->ZoneID = pMotor->ZoneID;                                  // Return zone stop occurred in (if Current Trip Profile active)

        pStopSig->pCurTripProfile = pMotor->pCurTripProfile;                // Return ptr to optional Current Trip Profile
        pStopSig->pExternalProcess = pMotor->pExternalProcess;              // Return ptr to optional external process function

        memcpy(pStopSig->RdfName, pMotor->RdfName, MOT_RDF_NAMESIZE_GEN);   // Return RDF file name
        AO_Publish(pStopSig, NULL);

        // Reset current profile table pointer & Motor external processing pointer

        pMotor->pCurTripProfile = NULL;         // Reset ptr to optional Current Trip Profile
        pMotor->pExternalProcess = NULL;        // Reset ptr to external motor processing

        // Translate stop status to text:

        StopCause[0] = 0;
        Mask = 1;
        for (Idx = 0; Idx < MOT_NUM_RDF_STOPINFO_MSGS; Idx++)
        {
            if (Mask & pMotor->StopStatus)
            {
                Str_Cat_N(StopCause, StopStr[Idx], sizeof(StopCause));
                Str_Cat_N(StopCause,  ", ", sizeof(StopCause));
            }
            Mask <<= 1;
        }

        // Log motor stop information
        Log(REQ, "StopInfo Motor %d, CurTrip=%d, Pos=%d, Time=%d, Reason=%s%s",
            pMotor->MotorId,
            pMotor->MotorCurrentTrip,
            pMotor->MotorPosition,
            pMotor->ElapsedTime,
            StopCause,
            pMotor->RdfName);
    }
}

//
// Motor servo functions
//

/* ========================================================================== */
/**
 * \brief   Update motor speed calculation
 *
 * \details This function calculates speed by accumulating the distance traveled
 *          over a period of time via FilterAverage. The result is then used to
 *          calculate the average, yielding speed in units of ticks / mS, which
 *          is then converted to RPM.
 *
 * \note    At the start of the move, the number of entries used to calculate
 *          the average increases as the buffer fills. This is done to provide
 *          a more accurate speed calculation at the start of the move. Once the
 *          whole buffer contains valid data, the full buffer size is used for
 *          calculating the average.
 *
 * \note    The servo only controls speed, not velocity, thus the absolute
 *          value of the displacement is taken before averaging.
 *
 * \param   pMotor - Pointer to motor control variables
 *
 * \return  None
 *
 * /// \todo 10/04/2021 DAZ - Returning speed instead of velocity. (Unsigned)
 *
 * ========================================================================== */
static void MotorUpdateSpeed(MOTOR_CTRL_PARAM *pMotor)
{
    uint32_t    MotorPeriod;    // Period from FPGA
    float32_t   TempSpeed;      // Calculated speed from period

    if (labs(pMotor->TicksThisMs) > MAX_TICKS_PER_MS)
    {
        Log(ERR, "Motor %d excessive speed: %d ticks/mS ~ %d RPM", pMotor->MotorId, pMotor->TicksThisMs, pMotor->TicksThisMs * MOT_RPM_PER_TICK_PER_MSEC);
    }

    // Read / calculate instantaneous speed from FPGA:

    if (L3_FpgaReadReg(MOTOR_REG(pMotor->MotorId, MOTOR_REG_PERIOD), &MotorPeriod))
    {
        TempSpeed = 1;          // Report 1 RPM if FPGA read register error.
    }
    else if (0 == MotorPeriod)
    {
        TempSpeed = 0xFFFFFFFF; // Invalid FPGA register value - return maximum speed
    }
    else
    {
        // Calculate motor speed in RPM from FPGA period for the last revolution.
        // Period reported by the FPGA is 19.56E-9 seconds / count. (See FPGA_PERIOD_TIME)
        // Denominator is time / revolution in seconds based on latest period.
        // Reciprocal is taken to get revolutions / sec.
        // Multiply by 60 to get RPM.

        TempSpeed = (1.0 / ((float32_t)MotorPeriod * FPGA_PERIOD_TIME)) * (float32_t)SEC_PER_MIN;
    }
    pMotor->MotorInstSpeed = (uint32_t)TempSpeed;

    // Average calculation
    FilterAverage(&pMotor->VelocityFilter.FilterAvg, labs(pMotor->TicksThisMs));    // Calculate speed (via labs).

    // Ramp the size up at the start of Motor Movement
    // This provides more accurate speed calculation during startup
    if (pMotor->VelocityFilter.Size < pMotor->VelocityFilter.FilterAvg.Length)
    {
        pMotor->VelocityFilter.Size++;
    }

    // Convert Speed to RPM (from ticks / mS)
    pMotor->VelocityFilter.Rpm = (pMotor->VelocityFilter.Conversion * (float32_t)pMotor->VelocityFilter.FilterAvg.SumData) / pMotor->VelocityFilter.Size;
    pMotor->MotorAvgSpeed = (uint32_t)pMotor->VelocityFilter.Rpm;
}

/* ========================================================================== */
/**
 * \brief   Update PWM via PID calculation
 *
 * \details This function calculates the speed error which is used to update
 *          the PID controller, which calculates a new PWM drive to be written
 *          out to the FPGA by the caller.
 *
 * \param   pMotor - Pointer to motor control variables
 *
 * \return  None
 *
 * ========================================================================== */
static void MotorUpdatePwm(MOTOR_CTRL_PARAM *pMotor)
{
    float32_t   Error;      // Velocity error
    float32_t   MaxPwm;     // PWM limit

    if (pMotor->TargetSpeed > 0)
    {
        // Calculate %error
        Error = ((float32_t)pMotor->TargetSpeed - (float32_t)pMotor->VelocityFilter.Rpm) / (float32_t)pMotor->TargetSpeed;
        if (fabs((float32_t)pMotor->TargetSpeed - (float32_t)pMotor->VelocityFilter.Rpm) < pMotor->RpmThresh)
        {
            Error = 0;      // Error too small to worry about. Force to 0.
        }

        PidController(&pMotor->Pid, Error);     // Update PID
    }
    else
    {
        PidReset(&pMotor->Pid, PID_OUTPUT_MAX, PID_OUTPUT_MIN, PID_INTEGRATOR_HIGH, PID_INTEGRATOR_LOW);
    }

    // Calculate new pwm value
    MaxPwm = MOT_MAX_PWM;
    pMotor->Pwm = (int16_t)(pMotor->Pid.Output * MaxPwm);   // Pid.Output is limited between 0 & 1 by PidController.
}

/* ========================================================================== */
/**
 * \brief   Read most recent A/D sample & update motor current.
 *
 * \details The most recent A/D sample is read and a new conversion is initiated.
 *          The sample is added to the average, and this result is used as the
 *          smoothed motor current. Averaging starts only after MOT_CURSETTLE_TIME
 *          has passed since the start of the move in order to let motor current settle.
 *
 * \param   pMotor - Pointer to motor control variables
 *
 * \return  None
 *
 * ========================================================================== */
static void MotorUpdateCurrent(MOTOR_CTRL_PARAM *pMotor)
{
    uint16_t TempCurrent;   // ADC current

    // Get most recent current sample
    L2_AdcRead(AdcSel[pMotor->MotorId], &pMotor->MotorCurrentRaw);

    // Start next sample w/ HW triggering
    L2_AdcStart(AdcSel[pMotor->MotorId], true);

    // Motor current settle time
    if (pMotor->ElapsedTime <= MOT_CURSETTLE_TIME)      /// \todo 03/22/2022 DAZ - What about the startup time supplied by the caller?
    {
        TempCurrent = 0;        // Ignore current until settle time expired
    }
    else
    {
        TempCurrent = pMotor->MotorCurrentRaw;
    }

    FilterAverage(&pMotor->CurrentFilter, TempCurrent);
    pMotor->MotorCurrent = pMotor->CurrentFilter.Output;            // Update averaged motor current
    TM_Hook(HOOK_MTR_CRNTRIP_SIMULATE, &pMotor->MotorCurrent);
}


/* ========================================================================== */
/**
 * \brief   Update motor Long Term Peak and Valley
 *
 * \details This function updates the long term peak and valley values used by
 *          current profile processing. The long term peak and valley values are
 *          initialized by setting them both to zero.
 *
 * \note    The CurrentLongTermPeak and CurrentLongTermValley values are the
 *          peak (and valley, respectively) of the filtered current, NOT of
 *          the CurrentLongTermAvg value. Thus, the peak and valley will be
 *          higher (and lower) than the highest/lowest values of the CurrentLongTermAvg
 *          value.
 *
 * \param   pMotor - Pointer to motor control variables
 *
 * \return  None
 *
 * ========================================================================== */
static void MotorUpdateLTPV(MOTOR_CTRL_PARAM *pMotor)
{
    if (pMotor->MotorCurrent > pMotor->CurrentLongTermPeak)
    {
        pMotor->CurrentLongTermPeak = pMotor->MotorCurrent;         // Keep track of peak
    }
    if (pMotor->CurrentLongTermValley == 0)
    {
        pMotor->CurrentLongTermValley = pMotor->MotorCurrent;       // Initialize valley value
    }
    else
    {
        if (pMotor->MotorCurrent < pMotor->CurrentLongTermValley)
        {
            pMotor->CurrentLongTermValley = pMotor->MotorCurrent;
        }
    }
}

// Motor servo processing states:

/* ========================================================================== */
/**
 * \brief   Motor Idle state
 *
 * \details This state is active when a motor is not running/stopping. Upon
 *          receiving a move request, it enables the allegro chips, and sets
 *          the direction of the selected motor, and transitions to the startup
 *          state to allow the allegro chips to stabilize before actually running
 *          the motor.
 *
 * \param   pMotor -  Pointer to motor control variables
 *
 * \return  None
 *
 * ========================================================================== */
static void ProcessIdleState(MOTOR_CTRL_PARAM *pMotor)
{
    if (MM_REQ_MOVE == pMotor->Request)
    {
        // If no motors are moving and no sound is happening, reset the FPGA
        // before starting the motor.

        if (!(AnyMotorsOn || LastAnyMotorsOn || L3_IsToneActive()))
        {
            Log(DBG, "Start FPGA reset");
            L3_FpgaMgrReset();      // Reset FPGA via ProgramN pin
            OSTimeDly(MSEC_10);     // Allow 10mSec for FPGA to reload
            L3_FpgaReload();        // Reset complete - mark selected registers for reloading
            Log(DBG, "End FPGA reset");
        }

        // Prepare the motor for move and enable all Allegro chips to allow proper motor ADC operation
        StartMotor(pMotor);

        pMotor->StartTime = SigTime();              // Set start time for startup
        pMotor->Request =  MM_REQ_NONE;             // Request processed - now remove.
        pMotor->State = MOTOR_STATE_STARTUP;        // Switch to startup state
    }
}

/* ========================================================================== */
/**
 * \brief   Motor Startup State
 *
 * \details This state waits a specified period for the allegro chips to stabilize
 *          before actually moving the motors. This is done to insure accurate
 *          current measurements at motor startup.
 *
 * \param   pMotor - Pointer to motor control variables
 *
 * \return  None
 *
 * ========================================================================== */
static void ProcessStartupState(MOTOR_CTRL_PARAM *pMotor)
{
    if ((SigTime() - pMotor->StartTime) >= ALLEGRO_STABILIZE_TIME)
    {
        // Ensure that delta count & status registers are up to date for running state
        L3_FpgaReadReg(MOTOR_REG(pMotor->MotorId, MOTOR_REG_DELTA_COUNT), NULL);    // Schedule delta counts to be read next tick. Not interested in current value
        L3_FpgaReadReg(MOTOR_REG(pMotor->MotorId, MOTOR_REG_STATUS), NULL);         // Schedule status to be read next tick. Not interested in current value
        L3_FpgaReadReg(MOTOR_REG(pMotor->MotorId, MOTOR_REG_PERIOD), NULL);         // Schedule motor period to be read next tick. Not interested in current value
        pMotor->StartTime = SigTime();                                              // Reset start time of move
        pMotor->ElapsedTime = 0;                                                    // Reset elapsed time
        RdfMotorDataWrite(pMotor);                                                  // Write initial log record
        pMotor->State = MOTOR_STATE_RUNNING;                                        // Transition to running state
    }
}

/* ========================================================================== */
/**
 * \brief   Motor Running state
 *
 * \details Update motor position, speed, current and PWM. Also, check for any
 *          stop conditions and transfer to stopping state as appropriate.
 *
 * \note    Moves less than MOT_POSITION_TOLERANCE result in no motion. \n \n
 *
 *          If changing speed, pMotor->TimeDelay may be changed to pMotor->ElapsedTime + n to
 *          suppress current processing till current spike from motor speed change stabilizes.
 *
 * \param   pMotor - Pointer to motor control variables
 *
 * \return  None
 *
 * ========================================================================== */
static void ProcessRunningState(MOTOR_CTRL_PARAM *pMotor)
{

    pMotor->ElapsedTime = SigTime() - pMotor->StartTime;    // Update elapsed run time
    MotorUpdatePosition(pMotor);                            // Update motor position

    // If multiple motors are running concurrently, ensure that the 12v select pulse is only
    // generated once / millisecond.

    // If 12V mode is enabled, pulse enable line low for ~10uSec to select 12V.
    if (MOTOR_VOLT_12 == pMotor->MotorVoltage)
    {
        if (!Motor12vTrig)                              // 12v select pulse has not been generated for this cycle.
        {
            L3_GpioCtrlClearSignal(GPIO_EN_BATT_15V);
        }
    }

    // Update current and speed:

    MotorSetCurrentTrip(pMotor);                        // Update current trip according to profile /// \todo 04/25/2022 DAZ - Shouldn't this happen AFTER current update?
    MotorUpdateSpeed(pMotor);                           // Update motor speed
    MotorUpdateCurrent(pMotor);                         // Update motor current

    TM_Hook(HOOK_MTRSPEED, (void *)(pMotor));
    // 05/16/2022 DAZ - Block execution time above is ~9uSec.

    // Complete 12V select pulse if enabled.
    if (MOTOR_VOLT_12 == pMotor->MotorVoltage)
    {
        if (!Motor12vTrig)                              // 12v select pulse has not been generated for this cycle.
        {
            L3_GpioCtrlSetSignal(GPIO_EN_BATT_15V);     // Complete 12V select pulse
        }
        Motor12vTrig = true;                            // 12V select pulse has been generated for this cycle
    }

    // Perform external processing if defined:
    if (pMotor->pExternalProcess != NULL)
    {
        pMotor->pExternalProcess(pMotor);
    }

    // All processing affecting speed, current trip, etc. has been completed.
    // Perform velocity processing below. Update the PWM value via PID loop,
    // and send the updated value to the FPGA.

    MotorUpdatePwm(pMotor);                                 // Maintain constant speed. Update the PWM

    // Set PWM. If error returned, stop motor, request refresh
    if (MOTOR_STATUS_ERROR == MotorSetPwm(pMotor->MotorId, pMotor->Pwm))
    {
        L3_FpgaRequestRefresh(true);
        pMotor->StopStatus |= MOT_STOP_STATUS_FPGA_SPI;     // Request stop
    }

    // Log data periodically
    if (0 == (pMotor->ElapsedTime % pMotor->DataLogPeriod))
    {
        RdfMotorDataWrite(pMotor);      // Write data out to log
    }

    // Time delayed processing. Long Term Average update and current trip test are performed here.
    if (pMotor->ElapsedTime > pMotor->TimeDelay)
    {
        MotorTimeDelayedProcessing(pMotor);
    }

    // Is there a reason to stop the motor?

    // Check for wrong direction
    // If Distance traveled this tick is not zero, and it does not have the same
    // sign as target move distance, we accumulate the ticks traveled. If they meet
    // or exceed the threshold, we have a direction error.
    if (0 != pMotor->TicksThisMs)
    {
        if (((pMotor->TicksThisMs ^ pMotor->TargetMoveDist) & UINT32_SIGN_MASK) != 0)   // Check if signs are different
        {
            pMotor->ErrorDirTicks += labs(pMotor->TicksThisMs);                         // Accumulate error movement
            if (pMotor->ErrorDirTicks >= DIRECTION_ERROR_DIST)
            {
                pMotor->StopStatus |= MOT_STOP_STATUS_DIR_ERR;
                FaultHandlerSetFault(REQRST_MOTOR_TEST, SET_ERROR);
            }
        }
    }

    // Check for move timeout
    if ((pMotor->ElapsedTime > pMotor->Timeout) && (pMotor->Timeout > 0))
    {
        pMotor->StopStatus |= MOT_STOP_STATUS_TIMEOUT;          // Move timeout
    }

    // Check for stop request
    if (MM_REQ_STOP == pMotor->Request)
    {
        pMotor->Request =  MM_REQ_NONE;
        pMotor->StopStatus |= MOT_STOP_STATUS_REQUEST;          // Stop requested
    }

    // Check for position reached. Stop when within MOT_POSITION_TOLERANCE ticks of target.
    /// \todo 12/10/2021 DAZ - NOTE: MOT_POSITION_TOLERANCE may eventually change with move speed.
    /// \todo 03/21/2022 DAZ - NOTE: TargetMoveDist < MOT_POSITION_TOLERANCE results in no motion.
    if ((labs(pMotor->TicksMoved) + MOT_POSITION_TOLERANCE) >= labs(pMotor->TargetMoveDist))
    {
        pMotor->State = MOTOR_STATE_STOPPING;
        pMotor->StopStatus |= MOT_STOP_STATUS_IN_POS;           // Position reached
    }

    // Check for stall - Speed must be low, time must be greater than startup, and PWM must be high
    // (Speed is low and we're trying hard). Note that speed is winding (rotor) speed.
    if ((pMotor->VelocityFilter.Rpm < MOT_STALL_RPM) && (pMotor->ElapsedTime > MOT_STALL_TIME) &&
        (pMotor->Pwm > MOT_STALL_PWM))
    {
        pMotor->StopStatus |= MOT_STOP_STATUS_ZERO_RPM;         // Stall
    }

    // Stop if any status pending
    if (pMotor->StopStatus)
    {
        L3_MotorStop(pMotor->MotorId);
        pMotor->StopDistance = 0;                               // Reset stopping distance
        pMotor->State = MOTOR_STATE_STOPPING;
    }
}

/* ========================================================================== */
/**
 * \brief   Motor Stopping state
 *
 * \details Update position and speed. Check for overrun. When motor has stopped,
 *          the appropriate motor stop info event is published.
 *
 * \param   pMotor -  Pointer to motor control variables
 *
 * \return  None
 *
 * ========================================================================== */
static void ProcessStoppingState(MOTOR_CTRL_PARAM *pMotor)
{
    bool        HasStopped;         // Flag to check if motor has stopped
    uint32_t    TimeNow;            // Current time

    // Get the current time

    TimeNow = OSTimeGet();
    pMotor->ElapsedTime = TimeNow - pMotor->StartTime;              // Update elapsed time

    // Motor may still be moving, keep updating position
    MotorUpdatePosition(pMotor);
    pMotor->StopDistance += pMotor->TicksThisMs;                    // Distance moved after motor shutoff /// \todo 10/08/2021 DAZ - I think we can use updateposition here.
    MotorUpdateSpeed(pMotor);                                       // Update motor speed

    pMotor->Pwm = 0;
    pMotor->MotorCurrent = 0;                                       // PWM off - force current to 0.
    pMotor->MotorCurrentRaw = 0;
    MotorSetPwm(pMotor->MotorId, pMotor->Pwm);                      // Force motor off
    FilterAverage(&pMotor->CurrentFilter, pMotor->MotorCurrent);    // Update current average (should be averaging in 0's)

    // Log streaming data

    if (0 == (pMotor->ElapsedTime % pMotor->DataLogPeriod))
    {
        RdfMotorDataWrite(pMotor);      // Write data out to log
    }

    // Perform external processing if defined:
    if (pMotor->pExternalProcess != NULL)
    {
        pMotor->pExternalProcess(pMotor);
    }

    /// \todo 12/10/2021 DAZ - Need to check stop conditiions. 1.3 uses 5msec or speed filter wind down.

    // Check if the motor is stopped - timeout?   // \todo 06/08/2021 KA - verify this status is reliable (legacy usage?) & additional error handling needed? (e.g. fpga lockup)
    (void)L3_MotorIsStopped(pMotor->MotorId, &HasStopped);  // \todo 06/08/2021 KA - process the return status?

    // Check for over-run
    if (labs(pMotor->StopDistance) >= MOTOR_RESIDUAL_MOVE_MAX)
    {
        // Critical Error: Overshoot
        Log(FLT, "Motor %d overshoot error. Extra %d ticks", pMotor->MotorId, pMotor->StopDistance);  /// \todo 10/15/2021 DAZ - Additional actions to stop motor?
        pMotor->StopStatus |= MOT_STOP_STATUS_OVERRUN;
        pMotor->StopStatus &= ~MOT_STOP_STATUS_IN_POS;      // If we overran, reset in position bit
        HasStopped = true;                                  // Force stop processing below
    }

    if (HasStopped)
    {
        pMotor->State = MOTOR_STATE_IDLE;

        // Publish the stop signal
        MotorPublishStopSig(pMotor);
        RdfClose(pMotor->Rdf);                              // Close the RDF file
        pMotor->StreamFlags = 0;                            // Reset motor RDF log parameters
    }
}

/* ========================================================================== */
/**
 * \brief   Reads ticks moved from last read.
 *
 * \details Get ticks traveled since last read
 *
 * \sa      L3_FpgaReadReg() for details on reading of FPGA
 *
 * \param   MotorId - Motor ID
 * \param   pTicks  - Pointer to distance moved since last read (in ticks)
 *
 * \return  MOTOR_STATUS - Status
 *
 * ========================================================================== */
static MOTOR_STATUS MotorGetDeltaCount(MOTOR_ID MotorId, int32_t *pTicks)
{
    MOTOR_STATUS Status;        // Return status variable
    bool FpgaStatus;            // FPGA transfer call

    FpgaStatus = L3_FpgaReadReg(MOTOR_REG(MotorId, MOTOR_REG_DELTA_COUNT), (uint32_t *)pTicks);

    Status = (FpgaStatus) ? MOTOR_STATUS_ERROR : MOTOR_STATUS_OK;

    return Status;
}


/* ========================================================================== */
/**
 * \brief   Update the motor position
 *
 * \details Retrieve the ticks moved from the FPGA delta ticks register
 *
 * \param   pMotor -  Pointer to motor information
 *
 * \return  None
 *
 * ========================================================================== */
static void MotorUpdatePosition(MOTOR_CTRL_PARAM *pMotor)
{
    bool Dummy; // Dummy argument for MotorIsRunning

    /// \todo 03/01/2022 DAZ - Refresh could be requested directly by FPGA itself, as it reports
    ///                        CRC errors on read which result in MOTOR_STATUS_ERRROR.... The trick
    ///                        is then requesting stop for all relevant motors. Cross reference between
    ///                        FGPA & motor...

    // If fpga error requesting delta count, refresh FPGA
    if (MOTOR_STATUS_ERROR == MotorGetDeltaCount(pMotor->MotorId, &pMotor->TicksThisMs))
    {
        L3_FpgaRequestRefresh(true);
        pMotor->StopStatus |= MOT_STOP_STATUS_FPGA_SPI;     // Request stop
    }

    pMotor->MotorPosition += pMotor->TicksThisMs;
    pMotor->TicksMoved += pMotor->TicksThisMs;

    // Invoke MotorIsRunning here to insure that the latest status is always in RAM to be available
    // when transitioning to the stopping state. MotorIsRunning invokes ReadMotorSignal, which operates
    // on the RAM value of the FPGA register, and schedules the FPGA to be read on the NEXT tick.
    if (MOTOR_STATUS_ERROR == L3_MotorIsStopped(pMotor->MotorId, &Dummy))
    {
        L3_FpgaRequestRefresh(true);
        pMotor->StopStatus |= MOT_STOP_STATUS_FPGA_SPI;     // Request stop
    }
}

/* ========================================================================== */
/**
 * \brief   Start the Motor
 *
 * \details Perpares the motor for a move by setting the proper motor direction,
 *          current limit, Allegro control lines, and the motor status & delta ticks
 *          registers in the FPGA.
 *
 * \param   pMotor -  Pointer to motor information
 *
 * \return  None
 *
 * ========================================================================== */
static void StartMotor(MOTOR_CTRL_PARAM *pMotor)
{
    bool Direction;                     // Temporary direction variable

    pMotor->TicksMoved = 0;             // Reset distance moved
    pMotor->StopStatus = 0;             // Reset stop status.
    
 
    // Set direction & current limit
    Direction = (pMotor->TargetMoveDist >= 0) ? true : false;   // Set direction based on sign of displacement parameter: negative/false - CW direction looking into adapter
    TM_Hook(HOOK_SIMULATEMOTOREVERSAL, &Direction);

    // Write direction, Current limit pwm
    MotorWriteSignal(pMotor->MotorId, MOTOR_SIGNAL_CTL_DIR, Direction);
    L3_FpgaWriteReg(MOTOR_REG(pMotor->MotorId, MOTOR_REG_CURR_PWM), pMotor->MotorCurrentLimit);

    // Schedule setting of Allegro control signals, reading of delta counts, motor status - Error check not required until read attempted
    // Note: Register read/writes do not take effect until next FPGA I/O. (Up to 1msec from now)
    MotorWriteSignal(pMotor->MotorId, MOTOR_SIGNAL_CTL_BRAKE_N, true);          // Release brake
    MotorWriteSignal(pMotor->MotorId, MOTOR_SIGNAL_CTL_COAST_N, true);          // Disable coast

    // Release reset line on ALL Allegro chips to insure proper motor ADC operation.
    MotorWriteSignal(MOTOR_ID0, MOTOR_SIGNAL_CTL_RESET_N, true);
    MotorWriteSignal(MOTOR_ID1, MOTOR_SIGNAL_CTL_RESET_N, true);
    MotorWriteSignal(MOTOR_ID2, MOTOR_SIGNAL_CTL_RESET_N, true);

    L3_FpgaReadReg(MOTOR_REG(pMotor->MotorId, MOTOR_REG_DELTA_COUNT), NULL);    // Schedule delta counts to be read next tick. Not interested in current value
    L3_FpgaReadReg(MOTOR_REG(pMotor->MotorId, MOTOR_REG_STATUS), NULL);         // Schedule status to be read next tick. Not interested in current value
}

/* ========================================================================== */
/**
 * \brief   Motor state machine
 *
 * \details This state machine is run every mSec for each motor, and invokes the
 *          appropriate state processing for each.
 *
 * \param   MotorId - Motor ID
 *
 * \return  void
 *
 * ========================================================================== */
static void MotorStateMachine(MOTOR_ID MotorId)
{
    MOTOR_CTRL_PARAM *pMotor;

    do
    {
        // \todo 06/08/2021 KA - revisit when API added. Might be convenient to use QP/C states.
        // \todo 06/28/2021 KA - benchmark with motor simulator - must perform at least as well as legacy.

        pMotor = &MotorCtrlParam[MotorId];          // Calculate pointer to motor control block

        // See state processing function headers for details on each state

        switch (pMotor->State)
        {
            case MOTOR_STATE_IDLE:
                ProcessIdleState(pMotor);
                break;

            case MOTOR_STATE_STARTUP:
                ProcessStartupState(pMotor);
                break;

            case MOTOR_STATE_RUNNING:
                ProcessRunningState(pMotor);
                break;

            case MOTOR_STATE_STOPPING:
                ProcessStoppingState(pMotor);
                break;

            default:
                break;
        }

    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Enable/Disable Motor power.
 *
 * \details Enabling motor turns on the motor voltage, both via FPGA, and GPIO
 *          expander.
 *
 * \param   Enable - Motor enable control
 *
 * \return  MOTOR_STATUS - Status
 *
 * ========================================================================== */
/// \todo 03/24/2022 DAZ - The GPIO signal EN_BATT_15V appears to go to the battery subsystem & do something?
/// \todo 03/24/2022 DAZ - EN_VMOT on the FPGA, gates the V_MOTOR supply.
static MOTOR_STATUS MotorPowerEnable(bool Enable)
{
    MOTOR_STATUS Status;        // Return status
    bool FpgaStatus;            // Holds return status from FPGA function calls
    uint8_t TempMCB;            // Helper variable to access 'MotorControlBlock.Config'

    TempMCB = MotorControlBlock.Config; /// \todo 03/24/2022 DAZ - Simpler way do to this?

    MotorControlBlock.Config  = MODIFY_BIT(MotorControlBlock.Config, FPGA_CFG_MOTOR_EN_BIT, !Enable);
    FpgaStatus = L3_FpgaWriteReg(FPGA_REG_CONTROL, MotorControlBlock.Config);   // Gate Battery 15V to motor subsystem

    // Enable 15V if write was successful
    if (!FpgaStatus)
    {
        Status = MOTOR_STATUS_OK;
        if (Enable)
        {
            L3_GpioCtrlSetSignal(GPIO_EN_BATT_15V);     // Enable Battery Voltage
            L3_GpioCtrlClearSignal(GPIO_EN_2P5V);       // Enable ADC reference
        }
        else
        {
            L3_GpioCtrlClearSignal(GPIO_EN_BATT_15V);   // Disable Battery Voltage
            L3_GpioCtrlSetSignal(GPIO_EN_2P5V);         // Disable ADC reference
        }
    }
    else
    {
        // Something went wrong, restore original value
        MotorControlBlock.Config = TempMCB;
        Status = MOTOR_STATUS_ERROR;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Set motor Configuration
 *
 * \details This function sets the the appropriate bits in the FPGA control register
 *          to configure the number of ticks / motor turn (6 or 12), and ADC sample
 *          trigger type (PWM or FGL), and writes the register to the FPGA.
 *
 * \param   TickRate - Ticks per revolution
 * \param   AdcTrigger - ADC trigger type
 *
 * \return  MOTOR_STATUS - Status
 *
 * ========================================================================== */
/// \todo 07/06/2021 DAZ - This routine is only called once & with fixed arguments. Could be hard coded in Init.
static MOTOR_STATUS MotorConfig(MOTOR_REV_TICK TickRate, MOTOR_ADC_TRIG AdcTrigger)
{
    MOTOR_STATUS Status;        // Return status variable
    bool FpgaStatus;            // Holds return status from FPGA function calls
    uint8_t Temp;               // Helper variable to access 'MotorControlBlock.Config'

    Temp = MotorControlBlock.Config;

    Status = MOTOR_STATUS_INVALID_PARAM;

    if ((TickRate < MOTOR_REV_TICK_COUNT) && (AdcTrigger < MOTOR_ADC_TRIG_COUNT))
    {
        // Write Tick/Rev and ADC trigger to FPGA
        Temp = MODIFY_BIT(Temp, FPGA_CFG_MOTOR_TICK_HIGH_BIT, TickRate);
        Temp = MODIFY_BIT(Temp, FPGA_CFG_ADC_SRC_BIT, AdcTrigger);

        FpgaStatus = L3_FpgaWriteReg(FPGA_REG_CONTROL, Temp);

        if (!FpgaStatus)
        {
            // Write was successful, update local config structure
            MotorControlBlock.Config = Temp;
            Status = MOTOR_STATUS_OK;
        }
        else
        {
            Status = MOTOR_STATUS_ERROR;
        }
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Update motor control signal
 *
 * \details Allows caller to update specified signal bit in motor control register
 *
 * \note    MOTOR_SIGNAL enum is designed to match bit position in the register
 *
 * \param   Motor ID - for which motor the signal to be set
 * \param   Signal - Motor control signal to write
 * \param   Value - Value to write
 *
 * \return  MOTOR_STATUS - Status
 *
 * ========================================================================== */
static MOTOR_STATUS MotorWriteSignal(MOTOR_ID MotorId, MOTOR_SIGNAL Signal, bool Value)
{
    MOTOR_STATUS Status;        // Return status variable
    bool FpgaStatus;            // Holds return status from FPGA function calls

    Status =  MOTOR_STATUS_INVALID_PARAM;

    // Validate motor ID. Check if the signal is writable.
    if ((MotorId < MOTOR_COUNT) && (Signal <= MOTOR_SIGNAL_CTL_CAL_N))
    {
        MotorControlBlock.ControlReg[MotorId] = MODIFY_BIT(MotorControlBlock.ControlReg[MotorId], Signal, Value);

        FpgaStatus = L3_FpgaWriteReg(MOTOR_REG(MotorId, MOTOR_REG_CONTROL),  MotorControlBlock.ControlReg[MotorId]);

        Status = (FpgaStatus) ? MOTOR_STATUS_ERROR : MOTOR_STATUS_OK;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Read motor control signal
 *
 * \details Allows caller to read specified signal bit in motor control register
 *
 * \note    MOTOR_SIGNAL enum is designed to match bit position in the register
 *
 * \param   MotorId - for which motor the signal to be set
 * \param   Signal - Motor control signal to write
 * \param   pValue - pointer to Value to write
 *
 * \return  MOTOR_STATUS - Status
 *
 * ========================================================================== */
static MOTOR_STATUS MotorReadSignal(MOTOR_ID MotorId, MOTOR_SIGNAL Signal, bool *pValue)
{
    MOTOR_STATUS Status;    // Return status variable
    bool FpgaStatus;        // Holds return status from FPGA function calls
    uint8_t *pTemp;         // Helper variable to access 'MotorControlBlock.Config'
    uint32_t TempValue;     // Helper variable to hold control/status register value
    FPGA_REG RegToRead;      // Helper variable holds register address to access

    // Default to error status
    Status =  MOTOR_STATUS_INVALID_PARAM;
    FpgaStatus = true;

    // Validate input parameters.
    if ((MotorId < MOTOR_COUNT) && (Signal < MOTOR_SIGNAL_COUNT) && (NULL != pValue))
    {
        // Check signal ID to find out register to read
        if (Signal <= MOTOR_SIGNAL_CTL_CAL_N)
        {
            // its a control signal
            RegToRead = MOTOR_REG(MotorId, MOTOR_REG_CONTROL);  // Get register address
            pTemp = &MotorControlBlock.ControlReg[MotorId];
        }
        else
        {
            // its a status signal
            Signal = (MOTOR_SIGNAL)(Signal - MOTOR_SIGNAL_STS_5V_OK);   /// \todo 10/11/2021 DAZ - Was MOTOR_SIGNAL_CTL_CAL_N       // Get relative status signal position
            RegToRead = MOTOR_REG(MotorId, MOTOR_REG_STATUS);   // Get register address
            pTemp = &MotorControlBlock.StatusReg[MotorId];
        }

        FpgaStatus = L3_FpgaReadReg(RegToRead, &TempValue);
        *pTemp = (uint8_t)TempValue;

        *pValue = GET_BIT(*pTemp, Signal);
        Status = (FpgaStatus) ? MOTOR_STATUS_ERROR : MOTOR_STATUS_OK;

    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Set DC offset of specified ADC.
 *
 * \details All Allegro chips must be on (with zero drive) & reference voltage
 *          enabled before calling this function. It reads the specified ADC and
 *          sets the offset to this value.
 *
 * \param   Num - ADC instance to set offset for
 *
 * \return  None
 *
 * ========================================================================== */
static void MotorAdcSetOffset(ADC_INSTANCE Num)
{
    uint16_t AdcOfst;                                       // Adc temporary offset

    L2_AdcSetOffset(Num, 0);                                // Clear offset in ADC structure
    L2_AdcStart(Num, false);                                // Start ADC immediately
/// \todo 10/22/2021 DAZ - Provide timeout to avoid hanging in loop below. Check L2_AdcRead exception handling.
    while (L2_AdcGetStatus(Num) != ADC_STATUS_DATA_NEW);
    L2_AdcRead(Num, &AdcOfst);                              // Read offset
    L2_AdcSetOffset(Num, AdcOfst);                          // Set offset
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Initialize Motor Interface module
 *
 * \details Initializes motor controller hardware and internal control structures.
 *
 * \param   < None >
 *
 * \return  MOTOR_STATUS - Status
 *
 * ========================================================================== */
MOTOR_STATUS L3_MotorInit(void)
{
    MOTOR_CTRL_PARAM    *pMotor;    // Pointer to motor data
    MOTOR_STATUS        Status;     // Return status variable
    MOTOR_ID            MotorId;    // Temporary Motor Id used as index in loops
    bool                FpgaStatus; // Holds return status from FPGA function calls
    uint32_t            TempValue;

    FpgaStatus = false;             // Default to no FPGA error
    UniqueNumber = 0;               // Initialize Unique # for naming RDF files.
    RefreshTimer = 0;               // Initialize FPGA refresh timer

    // Default configuration: 12 tick/rev, motor disabled, ADC trigger set to PWM
    /// \todo 03/24/2022 DAZ - This is currently hard coded. Should come from handle / kvf?
    MotorConfig(MOTOR_REV_TICK_12, MOTOR_ADC_TRIG_PWM);
    MotorPowerEnable(true);         // Must enable reference voltage & motor voltage to set ADC offset
    L3_MotorEnable(MOTOR_ID0);      // Ensure all allegro chips are enabled before measuring ADC offset.
    L3_MotorEnable(MOTOR_ID1);
    L3_MotorEnable(MOTOR_ID2);
    OSTimeDly(ALLEGRO_STABILIZE_TIME);  // Allow FPGA to update control/status registers & Allegro chips to stabilize

    // Set motor ADC offsets & schedule FPGA control/status register reads
    for (MotorId = MOTOR_ID0; MotorId < MOTOR_COUNT; MotorId++)
    {
        MotorAdcSetOffset(AdcSel[MotorId]);     // Now that all motors are enabled, set the motor ADC offset for each.

        // FpgaReadReg is called here to schedule the FPGA registers to be read on the next tick.
        // We don't care about the current value in RAM, which is why the result pointer is null.
        L3_FpgaReadReg(MOTOR_REG(MotorId, MOTOR_REG_CONTROL), NULL);
        L3_FpgaReadReg(MOTOR_REG(MotorId, MOTOR_REG_STATUS), NULL);
    }

    OSTimeDly(MOTOR_REG_SYNC_PERIOD);   // Allow reg value synch up, read again for latest value

    // Assuming sync is done. Read control and status registers
    for (MotorId = MOTOR_ID0; MotorId < MOTOR_COUNT; MotorId++)
    {
        // Sync local register values cache
        FpgaStatus |= L3_FpgaReadReg(MOTOR_REG(MotorId, MOTOR_REG_CONTROL), &TempValue);
        MotorControlBlock.ControlReg[MotorId] = TempValue;

        FpgaStatus |= L3_FpgaReadReg(MOTOR_REG(MotorId, MOTOR_REG_STATUS), &TempValue);
        MotorControlBlock.StatusReg[MotorId] = TempValue;
    }

    // Now that local register cache is updated, good to start with actual signal read/write
    // Set signals to default values
    for (MotorId = MOTOR_ID0; MotorId < MOTOR_COUNT; MotorId++)
    {
        FpgaStatus |= MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_COAST_N, true);    // Coast off
        FpgaStatus |= MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_MODE, true);       // Set slow decay mode
        FpgaStatus |= MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_BRAKE_N, false);   // Brake on
        FpgaStatus |= MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_RESET_N, true);    // Motor power on
        FpgaStatus |= MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_CAL_N, true);      // Calibration off
        FpgaStatus |= MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_ESF, true);        // Enable stop on fault
    }

    // Initialize motor state machine for all motors, and set interpolation tables for PID calculations.
    for (MotorId = MOTOR_ID0; MotorId < MOTOR_COUNT; MotorId++)
    {
        pMotor = &MotorCtrlParam[MotorId];
        pMotor->MotorId = MotorId;                  // Motor ID for functions that require it
        pMotor->State = MOTOR_STATE_IDLE;
        pMotor->Pid.IntegratorHighClamp = 1.0;
        pMotor->Pid.IntegratorLowClamp =  0.0;

        // Initialize motor structures:

        /// \todo 10/15/2021 DAZ - The pointers below imply that multiple motors can be run concurrently
        ///                        at different voltages. They can't. There is only one supply for all
        ///                        3 motors. If any motor is set to run @ 12 volts, any other motor that
        ///                        is running concurrently will run at 12 volts, whether or not its MotorVoltage
        ///                        flag is set for 12 volts.
        pMotor->TableData.PidInterpTables.PidDataTable[MOTOR_VOLT_12] = (PID_INTERP_TABLE *)(&MotorPidInterpTable12V);
        pMotor->TableData.PidInterpTables.PidDataTable[MOTOR_VOLT_15] = (PID_INTERP_TABLE *)(&MotorPidInterpTable15V);

        pMotor->pCurTripProfile = NULL;                 // Reset current profile table pointer
        pMotor->pExternalProcess = NULL;                // Reset external motor processing pointer
        pMotor->PrevZoneID = MOT_CTPROF_ZONE_NOTUSED;   // No zone ID encountered yet (for current profile processing)

        // Reset speed filter
        L3_MotorVelocityFilterClear(pMotor);
    }

    if (FpgaStatus)
    {
        // Consolidated error status
        Log(DBG, "L3_MotorInit: Motor initialized with errors");
        Status = MOTOR_STATUS_ERROR;
    }
    else
    {
        Log(DBG, "L3_MotorInit: Motor initialized.");
        Status = MOTOR_STATUS_OK;

        // NOTE: The MotorInitDone flag is required by L3_MotorServo() to insure that no servo
        //       processing is performed before motor initialization is complete.
        MotorInitDone = true;
    }

    return Status;
}


/* ========================================================================== */
/**
 * \brief   Motor servo
 *
 * \details Call from FPGA immediately after registers read & before registers
 *          written.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L3_MotorServo(void)
{
    MOTOR_ID MotorIndex;                    // Motor Index

    if (MotorInitDone)                      // Do servo processing only if motor initialization has completed
    {
        AnyMotorsOn = false;

        for (MotorIndex = MOTOR_ID0; MotorIndex < MOTOR_COUNT; MotorIndex++)
        {
            MotorStateMachine(MotorIndex);

            AnyMotorsOn |= (MOTOR_STATE_IDLE != MotorCtrlParam[MotorIndex].State) ? true : false;
        }

        Motor12vTrig = false;                   // Reset 12V motor voltage flag

        if (LastAnyMotorsOn && !AnyMotorsOn)    // Disable only when transitioning from any motors on to all motors off.
        {
            /// \todo 10/22/2021 DAZ - Update L3_MotorDisable/Enable to do all motors with single call.
            L3_MotorDisable(MOTOR_ID0);
            L3_MotorDisable(MOTOR_ID1);
            L3_MotorDisable(MOTOR_ID2);
            AO_Publish((void *)(&MotorsIdle), NULL);    // Publish all motors idle signal
        }

        if (!LastAnyMotorsOn && AnyMotorsOn)            // Detect all motors off to any motor on
        {
            AO_Publish((void *)(&MotorsMoving), NULL);  // Publish any motor moving signal
        }

        // If the refresh timer is active (!= 0), decrement until it reaches 0. At that time,
        // the request refresh flag is reset, allowing motor move access.

        if (RefreshTimer != 0)
        {
            RefreshTimer--;                     // Count down refresh timer (decrement every mSec)
            if (0 == RefreshTimer)
            {
                L3_FpgaRequestRefresh(false);   // Refresh timeout - OK to allow motor access again
                Log(DBG, "FPGA error - Refresh Complete");
            }
        }

        // If all motors have stopped, and there is a FPGA refresh request pending,
        // refresh the FPGA before allowing any new motor moves.

        if (!AnyMotorsOn && L3_FpgaIsRefreshPending() && (0 == RefreshTimer))
        {
            Log(DBG, "FPGA error - Refresh Start");
            if (!L3_FpgaMgrRefresh())                   // Refresh the FPGA
            {
                L3_FpgaReload();                        // Refresh succeeded - mark selected registers for reloading
            }
            RefreshTimer = FPGA_REFRESH_TIME;           // Start timer
        }

        LastAnyMotorsOn = AnyMotorsOn;          // Save previous value of AnyMotorsOn
    }
}


/* ========================================================================== */
/**
 * \brief   Get position for specified motor
 *
 * \details Retrieves the position value from the MotorCtrlParam block for the
 *          specified motor.
 *
 * \param   MotorId - Motor ID to retrieve position for
 * \param   pPos - Pointer to location to store position
 *
 * \return  MOTOR_STATUS - INVALID_PARAM if bad motor ID or null pointer.
 *
 * ========================================================================== */
MOTOR_STATUS L3_MotorGetPos(MOTOR_ID MotorId, int32_t *pPos)
{
    MOTOR_STATUS Status;

    Status = MOTOR_STATUS_INVALID_PARAM;
    if ((MotorId < MOTOR_COUNT) && (pPos != NULL))
    {
        *pPos = MotorCtrlParam[MotorId].MotorPosition;   // Retrieve current position
        Status = MOTOR_STATUS_OK;
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Set position for specified motor
 *
 * \details Sets the position value in the MotorCtrlParam block for the
 *          specified motor.
 *
 * \param   MotorId - Motor ID to set position for
 * \param   Pos - Position value to set
 *
 * \warning This function allows the motor position to be set while the motor
 *          is running. It is the caller's responsibility to insure that the
 *          motor is stopped before calling this function.
 *
 * \return  MOTOR_STATUS - INVALID_PARAM if bad motor ID.
 *
 * ========================================================================== */
MOTOR_STATUS L3_MotorSetPos(MOTOR_ID MotorId, int32_t Pos)
{
    MOTOR_STATUS Status;

    Status = MOTOR_STATUS_INVALID_PARAM;
    if (MotorId < MOTOR_COUNT)
    {
        MotorCtrlParam[MotorId].MotorPosition = Pos;     // Set current position
        Status = MOTOR_STATUS_OK;
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Stop motor
 *
 * \details Stops motor by enabling the brake and setting its PWM to 0.
 *
 * \param   MotorId - Motor ID to stop
 *
 * \return  MOTOR_STATUS - Status
 *
 * ========================================================================== */
MOTOR_STATUS L3_MotorStop(MOTOR_ID MotorId)
{
    MOTOR_STATUS Status;        // Return status variable
    bool FpgaStatus;            // FPGA transfer call status

    Status = MOTOR_STATUS_OK;
    FpgaStatus = false;

    do
    {
        BREAK_IF(MotorId >= MOTOR_COUNT);       // Validate motor ID

        Status |= MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_BRAKE_N, false);       // Brake on
        FpgaStatus |= L3_FpgaWriteReg(MOTOR_REG(MotorId, MOTOR_REG_VEL_PWM), 0);    // Motor drive to 0

        if (FpgaStatus  || (MOTOR_STATUS_OK != Status))
        {
            Status = MOTOR_STATUS_ERROR;
            break;
        }

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Enable motor
 *
 * \details Enables specified motor by asserting the brake and deasserting the coast
 *          signals for the Allegro chip of the selected motor. The motor is held in
 *          a dynamic braking condition. While the motor is in this condition, it will
 *          ignore any PWM signal being applied.
 *
 * \warning If this function is called while the motor is running, it will act as
 *          an emergency stop, overriding the motor servo. The motor servo will
 *          most likely return a stop status of ZERO_RPM (stall) as the motor
 *          will stop despite the best efforts of the servo.
 *
 * \note    The Brake signal MUST be released prior to moving a motor. Presently,
 *          this is done when starting a move.
 *
 * \sa      StartMotor() in L3_Motor.c
 *
 * \note    This function deasserts the Allegro reset signal for ALL motors,
 *          regardless of the one specified. All motors must have the reset
 *          deasserted so that the ADCs will work correctly. (Having any Allegro
 *          chip in reset loads the ADC reference voltage, resulting in invalid
 *          ADC readings)
 *
 * \param   MotorId - Motor ID
 *
 * \return  MOTOR_STATUS - Status
 *
 * ========================================================================== */
MOTOR_STATUS L3_MotorEnable(MOTOR_ID MotorId)
{
    MOTOR_STATUS Status;        // Return status variable
    bool FpgaStatus;

    Status = MOTOR_STATUS_INVALID_PARAM;

    // Validate motor ID
    if (MotorId < MOTOR_COUNT)
    {
        Status = MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_BRAKE_N, false);        // Enable brake mode (low true)
        Status |= MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_COAST_N, true);        // Disable coast mode (low true)
        Status |= MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_MODE, true);           // Ensure mode signal properly set
        Status |= MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_CAL_N, true);          // Ensure calibration is disabled

        // Ensure all allegros are out of reset so ADCs will work correctly
        Status |= MotorWriteSignal(MOTOR_ID0, MOTOR_SIGNAL_CTL_RESET_N, true);      // Note that reset is active low so "true" is disabled.
        Status |= MotorWriteSignal(MOTOR_ID1, MOTOR_SIGNAL_CTL_RESET_N, true);
        Status |= MotorWriteSignal(MOTOR_ID2, MOTOR_SIGNAL_CTL_RESET_N, true);

        FpgaStatus = L3_FpgaWriteReg(MOTOR_REG(MotorId, MOTOR_REG_VEL_PWM), 0);     // Ensure PWM register is 0.
        FpgaStatus |= L3_FpgaWriteReg(MOTOR_REG(MotorId, MOTOR_REG_CURR_PWM), MOT_MAX_PWM);

        Status = ((MOTOR_STATUS_OK == Status) && !FpgaStatus) ?  Status : MOTOR_STATUS_ERROR;
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
 * \todo 04/07/2022 DAZ - This function should probably unconditionally disable
 *                        all 3 motors. Parameter really isn't required.
 *
 * \param   MotorId - Motor ID to Disable
 *
 * \return  MOTOR_STATUS - Status
 *
 * ========================================================================== */
MOTOR_STATUS L3_MotorDisable(MOTOR_ID MotorId)
{
    MOTOR_STATUS Status;        // Return status variable
    bool FpgaStatus;

    Status = MOTOR_STATUS_INVALID_PARAM;

    // Validate motor ID
    if (MotorId < MOTOR_COUNT)
    {
        Status  = MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_BRAKE_N, true);  // De-energize  // \todo 06/08/2021 KA - compare to legacy usage
        Status |= MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_COAST_N, false);
        Status |= MotorWriteSignal(MotorId, MOTOR_SIGNAL_CTL_RESET_N, false);
        FpgaStatus = L3_FpgaWriteReg(MOTOR_REG(MotorId, MOTOR_REG_VEL_PWM), 0);

        Status = ((MOTOR_STATUS_OK == Status) && !FpgaStatus) ?  Status : MOTOR_STATUS_ERROR;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Check if motor is stopped
 *
 * \details Reads the last motor stopped status read from the FPGA.
 *
 * \note    Since this reads the last known value of the stopped status from the
 *          FPGA, it is possible for this function to return an erroneous stopped
 *          status if it is called immediately after a motor start and before the
 *          FPGA has been read.
 *
 * \todo 06/07/2021 KA - check if this is a reliable indicator (legacy usage?)
 * \todo 04/19/2022 DAZ - NOT used by legacy. Legacy uses calculated velocity.
 * \todo 04/19/2022 DAZ - This function detects stop ~250mSec after 0 velocity.
 *
 * \param   MotorId - Motor ID
 * \param   pHasStopped - Pointer to Motor status. ‘true’ if stopped
 *
 * \return  MOTOR_STATUS - Status
 *
 * ========================================================================== */
MOTOR_STATUS L3_MotorIsStopped(MOTOR_ID MotorId, bool *pHasStopped)
{
    MOTOR_STATUS Status;        // Return status variable

    Status = MOTOR_STATUS_INVALID_PARAM;

    do
    {
        // Validate motor ID and destination pointer
        if (MotorId >= MOTOR_COUNT)
        {
            break;
        }

        // Check if the motor has stopped
        Status = MotorReadSignal(MotorId, MOTOR_SIGNAL_STS_STOPPED, pHasStopped);

        if (MOTOR_STATUS_OK != Status)
        {
            break;
        }

        Status = MOTOR_STATUS_OK;

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Check for any motor running.
 *
 * \details This function determines if any motor is running by checking the
 *          static LastAnyMotorsOn, which is true if any motor is not in the
 *          idle state. This variable is updated every millisecond by L3_MotorServo().
 *
 * \param   < None >
 *
 * \return  True if any motor is running
 *
 * ========================================================================== */
bool L3_AnyMotorRunning(void)
{
    return LastAnyMotorsOn;
}
/* ========================================================================== */
/**
 * \brief   Set up streaming data logging for each motor movement
 *
 * \details Based on the bitmap supplied, this function creates an RDF object, which
 *          is a list of variables to be logged to the RDF file. It also creates a new
 *          rdf file for logging.
 *
 * \param   pMotor      - Pointer to motor control variables
 * \param   StreamFlags - Streaming flag bitmap
 *
 * \return  None
 *
 * ========================================================================== */
void L3_MotorSetupStreamingVars(MOTOR_CTRL_PARAM *pMotor, uint32_t StreamFlags)
{
    do
    {
        if (0 == StreamFlags)    // Quit if there's nothing to stream
        {
            // Don't open the RDF file if there's nothing to stream.
            pMotor->Rdf = NULL;
            Str_Copy_N(pMotor->RdfName, "None", sizeof(pMotor->RdfName));
            pMotor->RdfName[sizeof(pMotor->RdfName) - 1] = '\0';                // Ensure string is null terminated
            break;
        }

        snprintf(pMotor->RdfName, sizeof(pMotor->RdfName), "%05d.rdf", UniqueNumber++);
        pMotor->Rdf = RdfCreate(pMotor->RdfName, pMotor->MotorId, pMotor->DataLogPeriod, StreamFlags);  // Set up RDF object

        RdfOpen(pMotor->Rdf);
    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Clear the velocity filter data.
 *
 * \details Calls FilterAverageInit to initialize the velocity filter.
 *
 * \param   pMotor - Pointer to motor control variables
 *
 * \return  None
 *
 * ========================================================================== */
void L3_MotorVelocityFilterClear(MOTOR_CTRL_PARAM *pMotor)
{
    pMotor->VelocityFilter.Size = 0;
    pMotor->VelocityFilter.Conversion = MOT_RPM_PER_TICK_PER_MSEC;
    pMotor->VelocityFilter.Rpm = 0.0f;
    FilterAverageInit(&pMotor->VelocityFilter.FilterAvg, pMotor->VelocityFilterData, pMotor->VelocityFilter.FilterSize);
}


/* ========================================================================== */
/**
 * \brief   Get pointer to motor control data.
 *
 * \details This function returns a pointer to the motor specific data used by
 *          the motor control state machine invoked by L3_Servo().
 *
 * \warning The user must take care in the dereferencing of this pointer, as it
 *          is possible to change data while in the middle of a move. It is the
 *          caller's responsibility to ensure that any data dereferenced by this
 *          pointer is updated safely.
 *
 * \param   MotorId - Motor to retrieve motor control data pointer for.
 *
 * \return  Pointer to motor control data.
 *
 * ========================================================================== */
MOTOR_CTRL_PARAM* L3_MotorGetPointer(MOTOR_ID MotorId)
{
    return &MotorCtrlParam[MotorId];
}


/* ========================================================================== */
/**
 * \brief   Set scaled strain gauge value for motor logging
 *
 * \details This function sets the local variables ScaledStrain and RawStrain to
 *          the supplied values. They are both used for strain gauge logging by
 *          RdfMotorDataWrite.
 *              - This function is expected to be called every millisecond (by adapter)
 *                when a new strain gauge value is received.
 *              - The ScaleValue is expected to be scaled to pounds, compensated for tare, etc.
 *              - The RawValue is expected to be unscaled, uncompensated, in ADC counts.
 *              - It is NOT expected to be filtered.
 *
 * \note    This code is in a critical section to insure that the variable update
 *          process is not interrupted.
 *
 * \param   ScaleValue -  Scaled Value to set ScaledStrain local variable to.
 * \param   RawValue   -  Raw Value to set RawStrain local variable to.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_MotorSetStrain(float32_t ScaleValue, uint16_t RawValue)
{
    OS_CPU_SR cpu_sr;   // Status register save for enter/exit critical section

    OS_ENTER_CRITICAL();                    // Insure both values are updated without interruption
    ScaledStrain = (uint16_t)ScaleValue;    // Set local scaled & raw strain gauge values
    RawStrain = RawValue;
    OS_EXIT_CRITICAL();
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
