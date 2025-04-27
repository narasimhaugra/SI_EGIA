#ifndef SIGNALS_H__
#define SIGNALS_H__

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup  Signals
 * \{
 *
 * \brief   This module is part of the active object system. It provides all the
 *          defined signals of the Signia system.
 *
 * \todo  These are signals are derived from legacy. List needs to be cleaned.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signals.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
//*******************************************************************************
//***  NOTE:  THE FOLLOWING SIGNAL ENUMS ARE LINKED TO THE SigNameTable in     **
//***  Signals.c. The ENUM is used as an index into the SigNameTable.          **
//***  When adding ENUMs a corresponding entry must also be added in the Table **
//*******************************************************************************

/*! \todo 02/09/2021 DAZ -  In the enum below, obsolete/unused signals should be removed.
                            Also, the list (and SigNameTable) ought to be reordered so
                            that related signals are grouped together for easier reference.
*/

typedef enum
{
  // =================================
  // Reserved signals   (Q_ENTRY_SIG, etc)
  // =================================
  R_EMPTY_SIG,
  R_ENTRY_SIG,                  /*!< signal for coding entry actions */
  R_EXIT_SIG,                   /*!< signal for coding exit actions */
  R_INIT_SIG,                   /*!< signal for coding initial transitions */
  R_USER_SIG,                   /*!< first signal that can be used for user signals */

  // =================================
  // Published signals
  // =================================
  /* Physical Key Signals */

  P_TOGGLE_DOWN_RELEASE_SIG,
  P_TOGGLE_DOWN_PRESS_SIG,
  P_TOGGLE_UP_RELEASE_SIG,
  P_TOGGLE_UP_PRESS_SIG,
  P_TOGGLE_LEFT_RELEASE_SIG,
  P_TOGGLE_LEFT_PRESS_SIG,
  P_TOGGLE_RIGHT_RELEASE_SIG,
  P_TOGGLE_RIGHT_PRESS_SIG,
  P_LATERAL_LEFT_UP_RELEASE_SIG,
  P_LATERAL_LEFT_UP_PRESS_SIG,
  P_LATERAL_RIGHT_UP_RELEASE_SIG,
  P_LATERAL_RIGHT_UP_PRESS_SIG,
  P_LATERAL_LEFT_DOWN_RELEASE_SIG,
  P_LATERAL_LEFT_DOWN_PRESS_SIG,
  P_LATERAL_RIGHT_DOWN_RELEASE_SIG,
  P_LATERAL_RIGHT_DOWN_PRESS_SIG,

  /*Virtual Key Signals */
  P_LATERAL_UP_RELEASE_SIG,
  P_LATERAL_UP_PRESS_SIG,
  P_LATERAL_DOWN_RELEASE_SIG,
  P_LATERAL_DOWN_PRESS_SIG,
  P_SAFETY_RELEASE_SIG,
  P_SAFETY_PRESS_SIG,

  P_ACCELEROMETER_MOVEMENT_SIG,
  P_BATTERY_LEVEL_INSUFF_SIG,
  P_POWERING_DOWN_SIG,
  P_SHIPMODE_REQ_SIG,

  P_SD_CARD_INSERTED_SIG,
  P_SD_CARD_REMOVED_SIG,

  P_MOTOR_0_STOP_INFO_SIG,
  P_MOTOR_1_STOP_INFO_SIG,
  P_MOTOR_2_STOP_INFO_SIG,
  P_MOTOR_EXCESSIVE_LOAD_SIG,
  P_MOTOR_TARGET_LOAD_SIG,
  P_MOTOR_FPGA_ERROR_SIG,
  P_MOTOR_STOPPED_SIG,

  P_CLAMSHELL_CONNECTED_SIG,
  P_CLAMSHELL_REMOVED_SIG,
  P_ADAPTER_CONNECTED_SIG,
  P_ADAPTER_REMOVED_SIG,
  P_EGIA_RELOAD_CONNECTED_SIG,
  P_EGIA_RELOAD_REMOVED_SIG,
  P_RELOAD_CONNECTED_SIG,
  P_RELOAD_REMOVED_SIG,
  P_CARTRIDGE_CONNECTED_SIG,
  P_CARTRIDGE_REMOVED_SIG,
  P_ON_CHARGER_SIG,
  P_OFF_CHARGER_SIG,
  P_CHARGER_FAULT_SIG,
  P_BATTERY_LOW_SIG,
  P_BATTERY_INFO_SIG,
  P_RELOAD_SWITCH_EVENT_SIG,
  P_CHARGER_CONNECTED_SIG,
  P_CHARGER_REMOVED_SIG,
  P_IDLE_TIMEOUT_SIG,
  P_KEYPRESS_SIG,
  P_MOVEMENT_SIG,
  P_ROTATION_CONFIG_PRESS_SIG,
  P_ROTATION_CONFIG_RELEASE_SIG,
  P_ROTATION_CONFIG_COMPLETED_SIG,
  P_ROTATION_DEACTIVATED_SIG,
  P_MOTOR_IDLE_SIG,
  P_MOTOR_MOVING_SIG,

  /// \todo 02/09/2021 DAZ - These signals are not used YET, but may be needed for applications:
  P_CLAMPCYCLE_DONE_SIG,
  P_EEA_GRAPHIC_SIG,
  P_EEA_CTC_PERCENT_SIG,
  P_CLAMPING_SIG,
  P_UNCLAMPING_SIG,
  P_FIRE_ZONE_SIG,
  P_CTC_TIMEOUT_SIG,
  P_SHIPCAP_EXTENDED_SIG,
  P_SHIPCAP_COMPLETE_SIG,

  P_EEA_SG_TARE_SIG,
  P_EEA_SG_TARE_COMPLETE_SIG,
  P_EEA_ROTATION_ERR_SIG,

  /// \todo 05/24/2021 DAZ - Unused signals from legacy. May be implemented in the future

  P_RELOAD_FULLY_CLAMPED_SIG,
  P_RELOAD_FULLY_OPEN_SIG,
  P_RELOAD_TILTED_SIG,
  P_RELOAD_HOMING_ERR_SIG,
  P_ADAPTER_MISSING_COEFFICIENTS_SIG,
  P_ADAPTER_FULLY_CALIBRATED_SIG,
  P_ADAPTER_SG_CLAMP_TEST_ERROR_SIG,
  P_ADAPTER_SG_TARE_ERROR_SIG,
  P_ADAPTER_FORCE_EOL_SIG,
  P_ADAPTER_ENTER_LOW_POWER_MODE_SIG,
  P_ADAPTER_EXIT_LOW_POWER_MODE_SIG,
  P_FIRE_MODE_SIG,
  P_HIGH_FIRING_FORCE_SIG,
  P_RETRACTING_SIG,
  P_ARTIC_CENTER_SIG,
  P_FIRING_COMPLETE_SIG,
  P_EGIA_RETRACT_STOP_SIG,

  P_STRAIN_GAUGE_DATA_SIG,
  P_STRAIN_GAUGE_INVALID_SIG,
  P_LINEAR_SENSOR_DATA_SIG,
  P_EXTERN_GPIO_FAILURE_SIG,

  P_MOTOR_SET_HOME_SIG,               /// \todo 02/10/2022 DAZ - These 3 signals are obsolete and should be removed
  P_MOTOR_GET_POSITION_SIG,
  P_MOTOR_UPDATE_SPEED_SIG,

  P_SYSTEM_FAULT_SIG,
  P_USED_SHELL_SIG,
  P_ERR_SHELL_SIG,
  P_ERR_BATT_LOW_SIG,

  P_BATT_COMM_SIG,
  P_BATT_TEMP_SIG,
  P_BATT_DISABLED_SIG,
  P_BATT_SHUTDN_SIG,
  P_BATT_WARN_SIG,
  P_BATT_EOL_SIG,
  P_REQ_RST_SIG,
  P_HANDLE_EOL_SIG,
  P_HANDLE_MEM_SIG,
  P_PERM_FAIL_SIG,
  P_PERM_FAIL_WOP_SIG,
  P_PIEZO_ERROR_SIG,
  P_ERRFILE_SYS_SIG,
  P_SDCARD_ERROR_SIG,
  P_FILESYS_INTEGRITY_SIG,
  P_ACCELERR_SIG,
  P_BATT_LOW_SIG,
  P_BATT_INSUFF_SIG,
  P_ERRBAT_DISABLED_SIG,
  P_USB_ERROR_SIG,
  P_USB_CONNECTED_SIG,
  P_USB_REMOVED_SIG,
  P_RTC_ERROR_SIG,
  P_HBEAT_GPIOFAIL_SIG,
  P_GNKEY_LED_SIG,
  P_UNSUPPORTED_ADAPTER_SIG,
  P_ADAPTER_ERROR_SIG,
  P_WIFI_ERROR_SIG,
  P_SDCARD_EM_SIG,
  P_SDFS_EM_SIG,
  P_ONEWIRE_ADAPTER_CHECK_SIG,
  P_ONEWIRE_DEVICE_CHECK_SIG,           ///< Signal to facilatate display of check one wire device for authentication
  P_ONCHARGER_WAKEFROMSLEEP_SIG,
  P_ASA_FORCE_DIAL_UPDATE_SIG,
  P_ADAPTER_COM_RESP_RECEIVED_SIG,
  P_ADAPTER_COM_RETRY_FAIL_SIG,                ///< Signal to indicate all the Adapter Uart com retries failed.
  P_ERROR_OWSHORT_NO_DEVICE_SIG,
  P_HANDLE_FIRE_PROCEDURE_COUNT_TEST_SIG,
  P_LAST_SIG,                           ///< End of publishable signals

  // =================================
  // directly posted signals - Cannot be published
  // =================================
  DYNO_START_RAMP_SIG,        /// \todo 05/24/2021 DAZ - Is this needed?

  RDF_OPEN_SIG,               ///< Open motor RDF file request
  RDF_CLOSE_SIG,              ///< Close motor RDF file request
  RDF_DATA_SIG,               ///< Write RDF data request
  PRINTF_SIG,                 ///< Standard log file write request \todo 05/24/2021 DAZ - For logging. Rename?
  SECURITY_LOG_SIG,           ///< Security log file write request

  PLAY_TONE_SIG,
  LED_BLINK_SIG,

  REQ_TEST_SIG,              ///< Test mode request
  // Timer signals
  ANIMATION_TIMEOUT_SIG,
  IDLE_TIMEOUT_SIG,
  SLEEP_TIMEOUT_SIG,
  RETRY_TIMEOUT_SIG,
  CHARGER_IDLEMODE_TIMEOUT_SIG,
  TIMEOUT_SIG,
  FAULT_TIMEOUT_SIG,
  BATTERY_LOW_TIMEOUT_SIG,
  MOTOR_DISABLE_SIG,
  MOTOR_IDLE_TIMEOUT_SIG,
  COUNTDOWN_SCREEN_TIMEOUT_SIG,
  ROTATION_CONFIG_SCREEN_TIMEOUT_SIG,
  ROTATION_COMMAND_TIMEOUT_SIG,
  SWITCH_STATE_TIMEOUT_SIG,
  RETRY_FIRE_COUNT_UPDATE_TIMEOUT_SIG,
  TM_TIMEOUT_SIG,            ///< TestMode Timer Timeout signal
  TM_KEYTIMEOUT_SIG,         ///< TestManager Timer for Key Delay
  FIREMODE_TIMEOUT_SIG,
  SD_CARD_PRESENCE_TIMEOUT_SIG,
  CLAMSHELL_1WSHORT_TIMEOUT_SIG,
  WAIT_FOR_BH_TIMEOUT_SIG,
  ADAPTER_TYPE_CHECK_TIMEOUT_SIG,
  LAST_SIG  // reserved
} SIGNAL;

// Common signal formats:
// These formats are declared in a central location since their members do not use
// any module specific structures. Signals that do, have their formates defined in
// the header file of the module in which the specific structures are defined.

/// Signal containing a boolean
typedef struct
{
  QEvt    Event;
  bool    Flag;
} QEVENT_BOOLEAN;

/// Signal containing a uint16_t
typedef struct
{
  QEvt      Event;
  uint16_t  Value;
} QEVENT_INT16;

/// Signal containing a uint32_t
typedef struct
{
  QEvt      Event;
  uint32_t  Value;
} QEVENT_INT32;

/// Signal containing a uint32_t - Sent on high firing force, & motor speed change.
typedef struct
{
  QEvt      Event;
  uint32_t  ShaftRPM;       ///< New shaft RPM
} QEVENT_HIGH_FIRING_FORCE;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/
/// \todo FUTURE_ENHANCEMENT 02/15/2021 DAZ - Table to decide which signals should be logged upon posting. For now, we only have a table of signal names above.
// In order to avoid having the caller explicitly call log after each post/publish, this can be handled in the
// post / publish wrappers. This table is intended to designate which signals are logged & which aren't. Also,
// a callback is provided for any special processing that may be required before logging. A member might be added
// to the structure to denote the class of logging desired. (DBG, REQ)

#ifdef UNUSED_CODE       // { 02/15/2021 DAZ -

typedef struct
{
  SIGNAL sig;                                                             /*<! Name of signal */
  BOOL   log;                                                             /*<! True if signal is to be logged */
  char  *pString;                                                         /*<! Signal description string */
  void (*pFunc)( char *buf, INT8U bufSize, void const * const pEvent );   /// \todo 02/05/2021 DAZ - /*<! Pointer to function to execute when signal posted */

} SIG_TABLE;

//*******************************************************************************
//***  NOTE:  THE FOLLOWING SIG_TABLE IS LINKED TO THE SIGNAL ENUMS ABOVE      **
//***  The ENUM is used as an index into the SIGNAL Table                      **
//***  When adding Table Entries a corresponding ENUM must also be added       **
//***  These entries must be in the same order as the enum above               **
//*******************************************************************************

/// \todo 05/24/2021 DAZ - NOTE: Only sample entries are shown...

const SIG_TABLE sigTable[] =
{
//    sig                                log    *pString                        *pFunc
    { R_EMPTY_SIG,                       FALSE, "R_SIG_EMPTY",                   0 },    // QEP_EMPTY_SIG_
    { R_ENTRY_SIG,                       FALSE, "R_SIG_ENTRY",                   0 },    // Q_ENTRY_SIG
    { R_EXIT_SIG,                        FALSE, "R_SIG_EXIT",                    0 },    // Q_EXIT_SIG
    { R_INIT_SIG,                        FALSE, "R_SIG_INIT",                    0 },    // Q_INIT_SIG
    { R_USER_SIG,                        FALSE, "R_SIG_USER",                    0 },    // Q_USER_SIG
    { P_CLOSE_KEY_PRESS_SIG,             TRUE,  "Close Key Pressed",             0 },
    { P_CLOSE_KEY_RELEASE_SIG,           TRUE,  "Close Key Released",            0 },
    { P_OPEN_KEY_PRESS_SIG,              TRUE,  "Open Key Pressed",              0 },
    { P_OPEN_KEY_RELEASE_SIG,            TRUE,  "Open Key Released",             0 },
    { P_LEFT_ARTIC_KEY_PRESS_SIG,        TRUE,  "Left Articulation Pressed",     0 },
    { P_LEFT_ARTIC_KEY_RELEASE_SIG,      TRUE,  "Left Articulation Released",    0 },
    { P_RIGHT_ARTIC_KEY_PRESS_SIG,       TRUE,  "Right Articulation Pressed",    0 },
    { P_RIGHT_ARTIC_KEY_RELEASE_SIG,     TRUE,  "Right Articulation Released",   0 },
    { P_CW_ROTATE_KEY_PRESS_SIG,         TRUE,  "CW Rotate Key Pressed",         0 },
    { P_CW_ROTATE_KEY_RELEASE_SIG,       TRUE,  "CW Rotate Key Released",        0 },
    { P_CCW_ROTATE_KEY_PRESS_SIG,        TRUE,  "CCW Rotate Key Pressed",        0 },
    { P_CCW_ROTATE_KEY_RELEASE_SIG,      TRUE,  "CCW Rotate Key Released",       0 },
    { P_GREEN_KEY_PRESS_SIG,             TRUE,  "Green Key Pressed",             0 },
    { P_GREEN_KEY_RELEASE_SIG,           TRUE,  "Green Key Released",            0 },
    { P_SHIPMODE_REQ_SIG,                TRUE,  "Shipmode Request",              0 }
};

#endif                   // UNUSED_CODE

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern const char *const SigNameTable[];    // Signal to name translation table

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SIGNALS_H */

