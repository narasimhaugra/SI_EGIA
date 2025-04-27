#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signals
 * \{
 *
 * \brief   This module is part of the active object system. It provides all the
 *          defined signals of the Signia system. A compile time check is included
 *          to verify that SigTable is the same size as the SIGNAL enumeration.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signals.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Common.h"
#include "Signals.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/
//*******************************************************************************
//***  NOTE:  THE FOLLOWING SigNameTable IS LINKED TO THE SIGNAL ENUMS in      **
//***  Signals.h! The ENUM is used as an index into the SigNameTable.          **
//***  When adding Table Entries a corresponding ENUM must also be added.      **
//***  These entries MUST be in the same order as the enum in Signals.h.       **
//*******************************************************************************
const char *const SigNameTable[] =
{
    // =================================
    // Reserved signals   (Q_ENTRY_SIG, etc)
    // =================================
    "R_EMPTY_SIG",
    "R_ENTRY_SIG",
    "R_EXIT_SIG",
    "R_INIT_SIG",
    "R_USER_SIG",

    // =================================
    // Published signals
    // =================================
     /*Physical  Key Signals */

    "P_TOGGLE_DOWN_RELEASE_SIG",
    "P_TOGGLE_DOWN_PRESS_SIG",
    "P_TOGGLE_UP_RELEASE_SIG",
    "P_TOGGLE_UP_PRESS_SIG",
    "P_TOGGLE_LEFT_RELEASE_SIG",
    "P_TOGGLE_LEFT_PRESS_SIG",
    "P_TOGGLE_RIGHT_RELEASE_SIG",
    "P_TOGGLE_RIGHT_PRESS_SIG",
    "P_LATERAL_LEFT_UP_RELEASE_SIG",
    "P_LATERAL_LEFT_UP_PRESS_SIG",
    "P_LATERAL_RIGHT_UP_RELEASE_SIG",
    "P_LATERAL_RIGHT_UP_PRESS_SIG",
    "P_LATERAL_LEFT_DOWN_RELEASE_SIG",
    "P_LATERAL_LEFT_DOWN_PRESS_SIG",
    "P_LATERAL_RIGHT_DOWN_RELEASE_SIG",
    "P_LATERAL_RIGHT_DOWN_PRESS_SIG",

    /*Virtual Key Signals */
    "P_LATERAL_UP_RELEASE_SIG",
    "P_LATERAL_UP_PRESS_SIG",
    "P_LATERAL_DOWN_RELEASE_SIG",
    "P_LATERAL_DOWN_PRESS_SIG",
    "P_SAFETY_RELEASE_SIG",
    "P_SAFETY_PRESS_SIG",

    "P_ACCELEROMETER_MOVEMENT_SIG",
    "P_BATTERY_LEVEL_INSUFF_SIG",
    "P_POWERING_DOWN_SIG",
    "P_SHIPMODE_REQ_SIG",

    "P_SD_CARD_INSERTED_SIG",
    "P_SD_CARD_REMOVED_SIG",

    "P_MOTOR_0_STOP_INFO_SIG",
    "P_MOTOR_1_STOP_INFO_SIG",
    "P_MOTOR_2_STOP_INFO_SIG",
    "P_MOTOR_EXCESSIVE_LOAD_SIG",
    "P_MOTOR_TARGET_LOAD_SIG",
    "P_MOTOR_FPGA_ERROR_SIG",
    "P_MOTOR_STOPPED_SIG",

    "P_CLAMSHELL_CONNECTED_SIG",
    "P_CLAMSHELL_REMOVED_SIG",
    "P_ADAPTER_CONNECTED_SIG",
    "P_ADAPTER_REMOVED_SIG",
    "P_EGIA_RELOAD_CONNECTED_SIG",
    "P_EGIA_RELOAD_REMOVED_SIG",
    "P_RELOAD_CONNECTED_SIG",
    "P_RELOAD_REMOVED_SIG",
    "P_CARTRIDGE_CONNECTED_SIG",
    "P_CARTRIDGE_REMOVED_SIG",
    "P_ON_CHARGER_SIG",
    "P_OFF_CHARGER_SIG",
    "P_CHARGER_FAULT_SIG",
    "P_BATTERY_LOW_SIG",
    "P_BATTERY_INFO_SIG",
    "P_RELOAD_SWITCH_EVENT_SIG",
    "P_CHARGER_CONNECTED_SIG",
    "P_CHARGER_REMOVED_SIG",
    "P_IDLE_TIMEOUT_SIG",
    "P_KEYPRESS_SIG",
    "P_MOVEMENT_SIG",
    "P_ROTATION_CONFIG_PRESS_SIG",
    "P_ROTATION_CONFIG_RELEASE_SIG",
    "P_ROTATION_CONFIG_COMPLETED_SIG",
    "P_ROTATION_DEACTIVATED_SIG",
    "P_MOTOR_IDLE_SIG",
    "P_MOTOR_MOVING_SIG",

    "P_CLAMPCYCLE_DONE_SIG",
    "P_EEA_GRAPHIC_SIG",
    "P_EEA_CTC_PERCENT_SIG",
    "P_CLAMPING_SIG",
    "P_UNCLAMPING_SIG",
    "P_FIRE_ZONE_SIG",
    "P_CTC_TIMEOUT_SIG",
    "P_SHIPCAP_EXTENDED_SIG",
    "P_SHIPCAP_COMPLETE_SIG",

    "P_EEA_SG_TARE_SIG",
    "P_EEA_SG_TARE_COMPLETE_SIG",
    "P_EEA_ROTATION_ERR_SIG",

    "P_RELOAD_FULLY_CLAMPED_SIG",
    "P_RELOAD_FULLY_OPEN_SIG",
    "P_RELOAD_TILTED_SIG",
    "P_RELOAD_HOMING_ERR_SIG",
    "P_ADAPTER_MISSING_COEFFICIENTS_SIG",
    "P_ADAPTER_FULLY_CALIBRATED_SIG",
    "P_ADAPTER_SG_CLAMP_TEST_ERROR_SIG",
    "P_ADAPTER_SG_TARE_ERROR_SIG",
    "P_ADAPTER_FORCE_EOL_SIG",
    "P_ADAPTER_ENTER_LOW_POWER_MODE_SIG",
    "P_ADAPTER_EXIT_LOW_POWER_MODE_SIG",
    "P_FIRE_MODE_SIG",
    "P_HIGH_FIRING_FORCE_SIG",
    "P_RETRACTING_SIG",
    "P_ARTIC_CENTER_SIG",
    "P_FIRING_COMPLETE_SIG",
    "P_EGIA_RETRACT_STOP_SIG",

    "P_STRAIN_GAUGE_DATA_SIG",
    "P_STRAIN_GAUGE_INVALID_SIG",
    "P_LINEAR_SENSOR_DATA_SIG",
    "P_EXTERN_GPIO_FAILURE_SIG",

    "P_MOTOR_SET_HOME_SIG",
    "P_MOTOR_GET_POSITION_SIG",
    "P_MOTOR_UPDATE_SPEED_SIG",

    "P_SYSTEM_FAULT_SIG",
    "P_USED_SHELL_SIG",
    "P_ERR_SHELL_SIG",
    "P_ERR_BATT_LOW_SIG",

    "P_BATT_COMM_SIG",
    "P_BATT_TEMP_SIG",
    "P_BATT_DISABLED_SIG",
    "P_BATT_SHUTDN_SIG",
    "P_BATT_WARN_SIG",
    "P_BATT_EOL_SIG",
    "P_REQ_RST_SIG",
    "P_HANDLE_EOL_SIG",
    "P_HANDLE_MEM_SIG",
    "P_PERM_FAIL_SIG",
    "P_PERM_FAIL_WOP_SIG",
    "P_PIEZO_ERROR_SIG",
    "P_ERRFILE_SYS_SIG",
    "P_SDCARD_ERROR_SIG",
    "P_FILESYS_INTEGRITY_SIG",
    "P_ACCELERR_SIG",
    "P_BATT_LOW_SIG",
    "P_BATT_INSUFF_SIG",
    "P_ERRBAT_DISABLED_SIG",
    "P_USB_ERROR_SIG",
    "P_USB_CONNECTED_SIG",
    "P_USB_REMOVED_SIG",
    "P_RTC_ERROR_SIG",
    "P_HBEAT_GPIOFAIL_SIG",
    "P_GNKEY_LED_SIG",
    "P_UNSUPPORTED_ADAPTER_SIG",
    "P_ADAPTER_ERROR_SIG",
    "P_WIFI_ERROR_SIG",
    "P_SDCARD_EM_SIG",
    "P_SDFS_EM_SIG",
    "P_ONEWIRE_ADAPTER_CHECK_SIG",
    "P_ONEWIRE_DEVICE_CHECK_SIG",
    "P_ONCHARGER_WAKEFROMSLEEP_SIG",
    "P_ASA_FORCE_DIAL_UPDATE_SIG",
    "P_ADAP_COM_RESP_RECEIVED_SIG",
    "P_ADAP_COM_RETRY_FAIL_SIG",
    "P_ERROR_OWSHORT_NO_DEVICE_SIG",
    "P_HANDLE_FIRE_PROCEDURE_COUNT_TEST_SIG",
    "P_LAST_SIG",

    // =================================
    // directly posted signals - Cannot be published
    // =================================
    "DYNO_START_RAMP_SIG",

    "RDF_OPEN_SIG",
    "RDF_CLOSE_SIG",
    "RDF_DATA_SIG",
    "PRINTF_SIG",
    "SECURITY_LOG_SIG",

    "PLAY_TONE_SIG",
    "LED_BLINK_SIG",

    "REQ_TEST_SIG",
    // Timer signals
    "ANIMATION_TIMEOUT_SIG",
    "IDLE_TIMEOUT_SIG",
    "SLEEP_TIMEOUT_SIG",
    "RETRY_TIMEOUT_SIG",
    "CHARGER_IDLEMODE_TIMEOUT_SIG",
    "TIMEOUT_SIG",
    "FAULT_TIMEOUT_SIG",
    "BATTERY_LOW_TIMEOUT_SIG",
    "MOTOR_DISABLE_SIG",
    "MOTOR_IDLE_TIMEOUT_SIG",
    "COUNTDOWN_SCREEN_TIMEOUT_SIG",
    "ROTATION_CONFIG_SCREEN_TIMEOUT_SIG",
    "ROTATION_COMMAND_TIMEOUT_SIG",
    "RELOAD_SWITCH_TIMEOUT_SIG",
    "RETRY_FIRE_COUNT_UPDATE_TIMEOUT_SIG",
    "TM_TIMEOUT_SIG",
    "TM_KEYTIMEOUT_SIG",
    "FIREMODE_TIMEOUT_SIG",
    "SD_CARD_PRESENCE_TIMEOUT_SIG",
    "CLAMSHELL_1WSHORT_TIMEOUT_SIG",
    "WAIT_FOR_BH_TIMEOUT_SIG",
    "ADAPTER_TYPE_CHECK_TIMEOUT_SIG"
};

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/

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
/*                             Local Function(s)                              */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
// A compile-time check to ensure that SignalName table is the same size as SIGNAL enumerations.

static_assert((sizeof(SigNameTable) / sizeof(SigNameTable[0])) == (int)LAST_SIG,
              "SigNameTable is wrong size. Should have LAST_SIG entries.");

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

