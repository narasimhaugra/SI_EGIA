#ifndef EGIAUTIL_H
#define EGIAUTIL_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \brief   Header file for EGIA utilies
 *
 * \details This file defines utility prototypes and necessary types and
 *          constants required by EGIA in using these utilities.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    EGIAutil.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define NO_OF_RELOAD_TYPES          (0x3u)     ///< Number of Reload Types

#define EGIA_MAX_RELOAD_TYPES       (7u)       ///< Maximum Reload types
#define EGIA_MAX_ARTIC_ENTRIES      (7u)       ///< Maximum Articulation entries
#define EGIA_MAX_TOS_ENTRIES        (3u)       ///< Maximum SG to Speed entires
#define EGIA_MAX_FIRE_SPEEDS        (3u)       ///< Maximum Fire speeds

#define LEFT_ROTATION_CONFIG_KEYSEQ  (1<<LATERAL_LEFT_UP | 1<< LATERAL_LEFT_DOWN)
#define RIGHT_ROTATION_CONFIG_KEYSEQ  (1<<LATERAL_RIGHT_UP | 1<< LATERAL_RIGHT_DOWN)

#define ROTATE_CCW_KEYMASK       (1<<LATERAL_LEFT_DOWN | 1<<LATERAL_RIGHT_UP)
#define ROTATE_CW_KEYMASK       (1<<LATERAL_LEFT_UP | 1<<LATERAL_RIGHT_DOWN)

#define ROTATION_CONFIG_TIMEOUT                 (3000u) // Rotation screen timeout time
#define ROTATION_CONFIG_2SEC_TIMEOUT            (2000u) // Rotation screen activation/deactivation time of 2sec
#define ROTATION_CONFIG_SCREEN_COUNTDOWNTIME    (1000u) // Rotation config screen countdown time of 1sec
#define ROTATION_DEBOUNCE_TIME                  (100u)  // 100ms Debounce time



/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/// \todo 08/09/2021 DAZ - Is this structure used? No use for such a signal?

typedef struct
{

    QEvt     Event;                     ///< Event Structure to hold event details
    bool     Success;                   ///< Indication for calibration status
    bool     AdapterConnected;          ///< Flag to indicate whether adapter is connected
    bool     Reload_connected;          ///< Flag to indicate whether reload is connected
    uint32_t AdapterType;               ///< Adapter type (EGIA/EEA)
    int32_t   Ticks;                    ///< Total number of ticks to rotate
    int32_t   TicksToHardstop;          ///< used to store ticks to hardstop
} QEVENT_ADAPTER_CAL_INFO;

typedef struct
{
    QSignal Sig;
    KEY_ID  Key_Id;
    KEY_STATE State;
} KeyToSignal;

///Structure to hold Mulu and Cartridge compatibility
typedef struct
{
    uint16_t MuluId;                 ///< MULU Reload ID
    uint16_t CompatibleCartridgeId;  ///< Compatible Cartridge ID
} MuluCartridgeCompatibleList;

/// \todo 07/26/2022 DAZ - This structure is not referenced anywhere
typedef struct
{
    QEvt       Event;          ///< QPC event header
    ASA_INFO   *pAsaInfo;      ///< SG Force
} QEVENT_UPDATE_SPEED;


typedef struct                  // Tissue Optimized Speed (TOS) per SG Force Table
{
    uint8_t     num_entries;
    uint16_t    force[EGIA_MAX_TOS_ENTRIES];      // SG Force (Lbs)
    uint16_t    speed[EGIA_MAX_TOS_ENTRIES];      // Speed (Shaft RPM)
} EgiaTOSTable;

typedef struct                // EGIA Current Limit Profile per Artic angle Table
{
    uint8_t                       num_entries;
    float32_t                     artic_position[EGIA_MAX_ARTIC_ENTRIES]; // Artic Turns from Hardstop
    const MOT_CURTRIP_PROFILE     *clprof_tables[EGIA_MAX_ARTIC_ENTRIES]; // Current Limit Table corresponding to Artic Angle
} EgiaCLProfArticTable;

typedef struct                 // EGIA Max Fire Turns per Reload type Table
{
    float32_t   maxturns[EGIA_MAX_RELOAD_TYPES];
} EgiaMaxFireTurnsTable;

typedef struct
{
    const EgiaCLProfArticTable  *clprof_artic;
} EgiaReloadTable;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/
#define ARTIC_MOTOR      MOTOR_ID0
#define FIRE_MOTOR       MOTOR_ID1
#define ROTATE_MOTOR     MOTOR_ID2

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void EGutil_ReloadRecognitionStatus(EGIA * const me);
extern bool EGutil_CalibrationNextStatePreCond(EGIA * const me, QEvt const * const e);
extern bool EGutil_ReloadClampTestNextStatePreCond(EGIA * const me, QEvt const * const e);
extern void EGutil_GetReloadDeviceTypeLength(Handle * const pMe);
extern void EGutil_ReloadMULUFireCountTest(EGIA * const me);
extern void EGutil_UsedCartridgeTest(EGIA * const me);
extern void EGutil_ProcessReloadEOL(EGIA * const me);
extern void EGutil_ProcessDeviceConnEvents(Handle * const pMe, QEvt const * const pSig);
extern void EGutil_UpdateRotation(MOTOR_COMMAND Command, int32_t Position, uint16_t Speed);
extern void EGutil_UpdateArticulation(MOTOR_COMMAND Command, int32_t Position, uint16_t Speed);
extern void EGutil_CheckForReloadErrors(Handle * const pMe);
extern void EGutil_FireModeHandling(Handle * const pMe);
extern void EGutil_FireModeOpenPress(Handle * const pMe);
extern void EGutil_AdapterFiringAutoclaveCounterTest(Handle * const pMe);
extern void EGutil_AsaUpdateForceToSpeedTable(Handle * const pMe);
extern uint16_t EGutil_GetFireSpeedFromClampForce(Handle * const pMe);
extern bool EGutil_GetBottomKeyPressedTwiceInHalfSecond(EGIA * const me, KEY_ID Key);
extern void EGutil_UpdateSpeedBasedOnKeyPress(void);
extern void EGutil_ProcessEgiaStrainGaugeRawData(void *pTemp);
extern bool EGutil_SetCalibrationTareLbs( uint16_t Tare );
extern bool EGutil_ValidateCalibCoefficients(float32_t *pRoot, uint16_t Tare);
extern bool EGutil_IsOKToFire( EGIA * const me );
extern void EGutil_LoadDefaultCalibParams(void);
extern void EGutil_RepublishDeferredSig(Handle * const pMe);
extern void EGutil_ProcessAdapterEOL(Handle * const pMe);
extern bool EGutil_CheckUsedCartridge(Handle * const pMe);
extern void EGutil_ASAUpdateCallBack(MOTOR_CTRL_PARAM *pMotor);
extern void EGutil_UpdateMaxClampForceCallBack(MOTOR_CTRL_PARAM *pMotor);
extern bool EGUtil_StopRotArtOnMultiKey( KEY_ID KeyId, uint16_t KeyState );
extern void EGUtil_StartArticulation( KEY_ID KeyId, uint16_t KeyState );
extern bool EGutil_IsFPGAReset(QEvt const * const e);
extern void EGutil_ProcessEgiaReloadSwitchData(void *pTemp);
extern bool EGutil_GetCurrentLimitProfile(Handle *const pMe, MOT_CURTRIP_PROFILE *MotorITripProf);
extern int32_t EGutil_GetMaxFireTurns(Handle *const pMe);
extern void EGUtil_ProcessRotationRequest( QEvt const * const e, Handle *const pMe );
void EGUtil_InitRotationConfig(Handle *const pMe);


#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif  /* EGIAUTIL_H */
