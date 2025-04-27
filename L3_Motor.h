#ifndef MOTOR_H
#define MOTOR_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \brief   Public interface for Motor control.
 *
 * \details This file contains the enumerations as well as the function prototypes
 *          for the Motor control module
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_Motor.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "fs_type.h"
#include "FilterAverage.h"
#include "PidController.h"
#include "L3_MotorCommon.h"
#include "L2_Adc.h"
#include "Rdf.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

// Algorithm constants

#define MOT_POSITION_TOLERANCE         (5u)                                         ///< Ticks from Target Position to initate motor stop
#define MOT_RPM_PER_TICK_PER_MSEC      (MSEC_PER_MIN / HANDLE_PARAM_TICKS_PER_REV)  ///< Tick/msec to RPM Factor: (mSec * Revs)/(minute * ticks)

// Stop status bit values:

/// \todo 08/05/2021 DAZ - This list needs some cleanup. (ie. SPM_ES is unused, NO_STAPLES may not be used) Implement as bitmap ENUM?

/// NOTE: This list must be synchronized with the constant array StopStr.
#define MOT_STOP_STATUS_IN_POS          (0x0001u)   ///< Specified position reached
#define MOT_STOP_STATUS_TIMEOUT         (0x0002u)   ///< Excessive move time
#define MOT_STOP_STATUS_REQUEST         (0x0004u)   ///< User requested stop
#define MOT_STOP_STATUS_CURRENT_ES      (0x0008u)   ///< Excessive current (End Stop)
#define MOT_STOP_STATUS_ZERO_RPM        (0x0010u)   ///< Could not detect movement
#define MOT_STOP_STATUS_STRAINGAGE      (0x0020u)   ///< Stop due to excessive force
#define MOT_STOP_STATUS_OBSTRUCTION     (0x0040u)   ///< Obstruction found
#define MOT_STOP_STATUS_CURRENT_TL      (0x0080u)   ///< Target load achieved
#define MOT_STOP_STATUS_FATAL_ERROR     (0x0100u)   ///< Fatal Error
#define MOT_STOP_STATUS_NO_STAPLES      (0x0200u)   ///< No staples detected
#define MOT_STOP_STATUS_OVERRUN         (0x0400u)   ///< Motor has overrun specified destination position
#define MOT_STOP_STATUS_DIR_ERR         (0x0800u)   ///< Motor is running in the wrong direction
#define MOT_STOP_STATUS_FPGA_SPI        (0x1000u)   ///< FPGA SPI communication error

#define MOT_MAX_CURLIMIT_ENTRIES        (21u)       ///< Max # entries in current limit profile
#define MOT_MAX_ARA_ENTRIES             (7u)        ///< Max # entries in Adaptive Retraction Algorithm table

#define MOT_LTA_FILTER                  (100u)      ///< Current LongTerm Average Filter size

#define HANDLE_PARAM_TICKS_PER_REV      (12u)       ///< Ticks per rotor revolution (Portescap motor only)
#define HANDLE_PARAM_GEAR_RATIO         (25u)       ///< Handle gear ratio (Portescap motor only)

#define MOT_FP_ROUNDING                 (0.5f)      ///< Floating Point Rounding to integer
#define MSEC_PER_MIN                    (60000.0f)  ///< Milliseconds per Minute /// \todo 03/24/2022 DAZ - Really belongs in common

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

/// Motor function status   /// \todo 03/24/2022 DAZ - Really belongs in Signia_Motor.
typedef enum
{
    MM_STATUS_OK,                       ///< No error
    MM_STATUS_ERROR,                    ///< Motor error
    MM_STATUS_INVALID_PARAM,            ///< Invalid parameter
    MM_STATUS_COMPLETE,                 ///< Complete
    MM_STATUS_FAULT                     ///< Motor Fault
} MM_STATUS;

/// Motor requests
typedef enum
{
    MM_REQ_NONE,                       ///< No request
    MM_REQ_MOVE,                       ///< Motor move request
    MM_REQ_STOP                        ///< Motor stop request
} MM_REQ_TYPE;

/// Velocity filter structure
typedef struct
{
    FILTER_AVERAGE FilterAvg;                                 ///< Average Filter
    uint8_t        FilterSize;                                ///< Size of filter (taps)
    uint8_t        Size;                                      ///< Size of Velocity Filter as motor ramps-up (0 to FilterSize)
    float32_t      Conversion;                                ///< Convert velocity to Rpm    /// \todo 10/11/2021 DAZ - This is set to a system constant. Don't need this member
    float32_t      Rpm;                                       ///< Calculated RPM
} FILTER_VELOCITY;

/// Motor control state
typedef enum
{
    MOTOR_STATE_IDLE,                   ///< Motor Idle state
    MOTOR_STATE_STARTUP,                ///< Motor Startup state
    MOTOR_STATE_RUNNING,                ///< Motor Running state
    MOTOR_STATE_STOPPING,               ///< Motor Stopping state
    MOTOR_STATE_COUNT
} MOTOR_STATE;

/// Ticks per motor revolution
typedef enum {

    MOTOR_REV_TICK_6,       ///< 6 tick/revolution    /// \todo 07/06/2021 DAZ - NOTE: Order of these enums is critical!
    MOTOR_REV_TICK_12,      ///< 12 tick/revolution   /// \todo 07/06/2021 DAZ - Comment was wrong
    MOTOR_REV_TICK_COUNT    ///< Option count
} MOTOR_REV_TICK;

/// Motor supply voltage
typedef enum
{
    MOTOR_VOLT_12,          ///< 12V option
    MOTOR_VOLT_15,          ///< 15V option
    MOTOR_VOLT_COUNT        ///< Option count
} MOTOR_SUPPLY;

/// ADC triggger source
typedef enum
{
    MOTOR_ADC_TRIG_PWM,     ///< ADC trigger source PWM    /// \todo 07/06/2021 DAZ - NOTE: Order of these enums is critical!
    MOTOR_ADC_TRIG_FGL,     ///< ADC trigger source FGL
    MOTOR_ADC_TRIG_COUNT    ///< ADC trigger source count
} MOTOR_ADC_TRIG;


/// Motor Current Trip Profile calculation methods
typedef enum
{
    MOT_CTPROF_METH_ABSOLUTE,   ///< Current Limit Profile Method - Absolute
    MOT_CTPROF_METH_DELTA,      ///< Current Limit Profile Method - Delta
    MOT_CTPROF_METH_SLOPE       ///< Current Limit Profile Method - Slope (Not implemented)
}MOT_CURTRIP_METHOD;

/// Motor Current Trip Profile operating zones
typedef enum
{
    MOT_CTPROF_ZONE_LEARNING,   ///< Current Limit Profile Zone - Learning
    MOT_CTPROF_ZONE_ENDSTOP,    ///< Current Limit Profile Zone - Endstop
    MOT_CTPROF_ZONE_INTERLOCK,  ///< Current Limit Profile Zone - InterLock
    MOT_CTPROF_ZONE_NORMAL,     ///< Current Limit Profile Zone - Normal
    MOT_CTPROF_ZONE_NOTUSED     ///< Current Limit Profile Zone - Not Used
}MOT_CURTRIP_ZONE;

/// Motor Current trip profile table structure
typedef struct
{
    uint8_t               NumEntries;                                 ///< Number of entries in table
                                                                      /// Turns from 0 are Positive & Ordered smallest to largest with movement direction
    int32_t               TurnsPosition[MOT_MAX_CURLIMIT_ENTRIES];    ///< End of range in Turns from Origin. Entry applies to positions < this value
    uint16_t              CurrentTrip[MOT_MAX_CURLIMIT_ENTRIES];      ///< ADC Count - Threshold or delta depending on method
    MOT_CURTRIP_METHOD    Method[MOT_MAX_CURLIMIT_ENTRIES];           ///< Absolute, Delta, or Slope (Slope not implemented)
    MOT_CURTRIP_ZONE      ZoneID[MOT_MAX_CURLIMIT_ENTRIES];           ///< Zone ID
    float32_t             Kcoeff[MOT_MAX_CURLIMIT_ENTRIES];           ///< Delta method K (Coefficient)
} MOT_CURTRIP_PROFILE;


/// \todo 04/12/2022 DAZ - Maybe this should contain ADC channel so we don't need constant array?
/// Motor control parameters

typedef struct MOTOR_CTRL_PARAM
{
    MOTOR_ID    MotorId;                                    ///< Motor Number (Id)
    MOTOR_STATE State;                                      ///< Motor state machine state
    MM_REQ_TYPE Request;                                    ///< State machine request

    // User move parameters:

    void (*pExternalProcess)(struct MOTOR_CTRL_PARAM *pMotor); ///< External motor processing - called every tick during run/stop
    uint32_t                StreamFlags;                    ///< Bitmap of parameters to be written to RDF file.
    uint16_t                MotorCurrentLimit;              ///< Motor current limit (PWM value)
    uint16_t                MotorCurrentTrip;               ///< Motor current trip threshold
    uint32_t                DataLogPeriod;                  ///< Streaming period (mSec)
    int32_t                 TargetMoveDist;                 ///< Distance target for this move (In ticks)
    uint32_t                TargetSpeed;                    ///< Motor target speed (Armature / rotor RPM)
    uint16_t                TargetShaftRpm;                 ///< Trilobe speed (RPM)
    uint16_t                Timeout;                        ///< Maximum time to run motor (mSec)
    uint16_t                TimeDelay;                      ///< Ignore initial current values (mSec)
    bool                    InitCurrent;                    ///< Initialize Long Term Average current filter to 0.0 if true

    // Real time data:

    int32_t     MotorPosition;                              ///< Motor position (Total accumulated ticks from last position set)
    uint32_t    MotorAvgSpeed;                              ///< Present average motor speed (rotor/armature)
    uint32_t    MotorInstSpeed;                             ///< Instantaneous motor speed (from FPGA)
    uint16_t    MotorCurrent;                               ///< Present motor current (avg)
    uint16_t    MotorCurrentRaw;                            ///< Present motor current (raw)
    uint16_t    StopStatus;                                 ///< Reason for move stop (bitmap)
    uint32_t    StartTime;                                  ///< Time move started (mSec)
    uint32_t    ElapsedTime;                                ///< Move elapsed time

    uint8_t     ErrorDirTicks;                              ///< Distance moved in the wrong direction (In ticks)
    int32_t     TicksThisMs;                                ///< Distance moved this mS
    int32_t     TicksMoved;                                 ///< Distance covered this move
    int32_t     StopDistance;                               ///< Distance traveled after stop issued.

    // Velocity control:

    uint32_t        RpmThresh;                              ///< Minimum servo error for correction.
    FILTER_VELOCITY VelocityFilter;                         ///< Buffer for speed calculation
    int16_t         VelocityFilterData[FILTER_SIZE_MAX];    ///< Filter data
    PID             Pid;                                    ///< Servo control variables
    int16_t         Pwm;                                    ///< Calculated PWM for motor voltage
    MOTOR_SUPPLY    MotorVoltage;                           ///< Motor voltage ID
    PID_TABLE_DATA  TableData;                              ///< PID Interpolation table

    // Current Control:
    bool                UsingDelta;                         ///< Delta current in use for current trip calculation
    bool                LastEndStop;                        ///< True if last current profile entry is EndStop
    MOT_CURTRIP_ZONE    ZoneID;                             ///< Present profile zone ID
    MOT_CURTRIP_ZONE    PrevZoneID;                         ///< Last profile zone ID
    float32_t           Kcoeff;                             ///< Current trip value K coefficient
    uint16_t            CurrentLongTermAvg;                 ///< Long term average current (ADC counts)
    uint16_t            CurrentLongTermPeak;                ///< Long term peak current (ADC counts)
    uint16_t            CurrentLongTermValley;              ///< Long term valley current (ADC counts)

    /// \todo 03/30/2022 DAZ - OY. Another bad name. Should this be unsigned?
    int32_t                 TicksPosition[MOT_MAX_CURLIMIT_ENTRIES];    ///< Current profile position thresholds in ticks
    MOT_CURTRIP_PROFILE     *pCurTripProfile;                           ///< Current trip profile pointer

    FILTER_AVERAGE          CurrentFilter;                          ///< Current Filter
    int16_t                 CurrentFilterData[CURRENT_FILTER_SIZE]; ///< Filter data

    // Streaming support
    RDF_OBJECT              *Rdf;                           ///< Pointer to RDF object
    char                    RdfName[MOT_RDF_NAMESIZE_GEN];  ///< Name of Rdf file
    RDF_VAR                 RdfVars[MOT_STREAM_MAX_VARS];   ///< RDF variable objects
    int8_t                  MemoryFence[MEMORY_FENCE_SIZE_BYTES];
} MOTOR_CTRL_PARAM;

typedef void (MOTOR_PROCESS_FUNCTION)(MOTOR_CTRL_PARAM *pMotor);    ///< Motor processing callback

/// Motor control/status signal bits. Enum entry MUST match the bit position in actual register
typedef enum
{
    /// Control bits, affects register @ offset 0
    MOTOR_SIGNAL_CTL_RESET_N,       ///< Allegro reset - active low
    MOTOR_SIGNAL_CTL_DIR,           ///< Motor direction - 0 = CW, positive direction
    MOTOR_SIGNAL_CTL_BRAKE_N,       ///< Motor brake - active low
    MOTOR_SIGNAL_CTL_MODE,          ///< Allegro mode (See A3931 Datasheet)
    MOTOR_SIGNAL_CTL_COAST_N,       ///< Coast mode - active low
    MOTOR_SIGNAL_CTL_ESF,           ///< ESF - Stop on Allegro fault
    MOTOR_SIGNAL_CTL_CAL_N,         ///< Motor current cal enable - active low

    /// Status bits - Read from status register @ offset 03
    MOTOR_SIGNAL_STS_5V_OK,        ///< Motor 5V OK.
    MOTOR_SIGNAL_STS_FF1,          ///< Allegro fault line 1 (See A3931 datasheet)
    MOTOR_SIGNAL_STS_FF2,          ///< Allegro fault line 2
    MOTOR_SIGNAL_STS_HE0_ERR,      ///< Hall Effect 0 error (See FPGA code
    MOTOR_SIGNAL_STS_HE1_ERR,      ///< Hall Effect 1 error
    MOTOR_SIGNAL_STS_HE2_ERR,      ///< Hall Effect 2 error
    MOTOR_SIGNAL_STS_STOPPED,      ///< Set when motor stopped
    MOTOR_SIGNAL_STS_DOG_TRIP,     ///< FPGA watchdog tripped
    MOTOR_SIGNAL_COUNT             ///< Number of motor signal entries (control/status)
} MOTOR_SIGNAL;

/// Motor stop signal
typedef struct
{
    QEvt      Event;                          ///< QPC event header
    MOTOR_ID  MotorNum;                       ///< Motor number
    int32_t   Position;                       ///< Stop position
    uint16_t  StopStatus;                     ///< Reason for move stop bitmap
    uint32_t  TargetShaftRpm;                 ///< Requested speed at output shaft
    uint32_t  MaxTime;                        ///< Max time to perform move.
    uint32_t  EndTime;                        ///< Time of move completion    (valid only after move complete)
    uint32_t  ElapsedTime;                    ///< Move elapsed time

    uint16_t  CurrentLongTermAvg;             ///< Long term average motor current
    uint16_t  CurrentLongTermPeak;            ///< Long term average motor current peak
    uint16_t  CurrentLongTermValley;          ///< Long term average motor current valley
    uint8_t   ZoneID;                         ///< Zone current trip occurred in (if Current Trip profile in effect)

    MOT_CURTRIP_PROFILE    *pCurTripProfile;  ///< If non-null, Current Trip profile was in effect
    MOTOR_PROCESS_FUNCTION *pExternalProcess; ///< If non-null, External processing was in effect

    uint8_t   RdfName[MOT_RDF_NAMESIZE_GEN];  ///< RDF filename for this move
} QEVENT_MOTOR_STOP_INFO;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/
extern const char *StopStr[MOT_NUM_RDF_STOPINFO_MSGS];

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/
/* None */
/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
MOTOR_STATUS L3_MotorInit(void);
MOTOR_STATUS L3_MotorInterfaceStart(MOTOR_ID MotorId, int32_t Tick, uint16_t CurrentLimitPwm);
MOTOR_STATUS L3_MotorStop(MOTOR_ID MotorId);
MOTOR_STATUS L3_MotorEnable(MOTOR_ID MotorId);
MOTOR_STATUS L3_MotorDisable(MOTOR_ID MotorId);
MOTOR_STATUS L3_MotorSetPos(MOTOR_ID MotorId, int32_t Pos);
MOTOR_STATUS L3_MotorGetPos(MOTOR_ID MotorId, int32_t *pPos);
MOTOR_STATUS L3_MotorIsStopped(MOTOR_ID MotorId, bool *pHasStopped);

bool L3_AnyMotorRunning(void);

void L3_MotorServo(void);
void L3_MotorSetupStreamingVars(MOTOR_CTRL_PARAM *pMotor, uint32_t StreamFlags);
void L3_MotorVelocityFilterClear(MOTOR_CTRL_PARAM *pMotorInfo);
void L3_MotorSetStrain(float32_t ScaleValue, uint16_t RawValue);

MOTOR_CTRL_PARAM* L3_MotorGetPointer(MOTOR_ID MotorId);

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif  /* MOTOR_H */
