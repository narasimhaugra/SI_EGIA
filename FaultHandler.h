#ifndef FAULTHANDLER_H
#define FAULTHANDLER_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup FaultHandler
 * \{
 *
 * \brief   Header file for Fault Handler
 *
 * \details Global defines and prototypes defined here.
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    FaultHandler.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Common.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define SET_ERROR   (true)           ///< Set Error
#define CLEAR_ERROR (false)          ///< Clear Error

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/*! \enum ERROR_CAUSE
*   Error Cause
*/
// this MUST align with strings ErrorCauseStrings[]
typedef enum                    /* Enum which holds Hardware Error Cause*/
{
    NO_ERROR_CAUSE,                      ///< Represents no error cause
    REQRST_FPGA_SELFTEST,                ///< FPGA SELF TEST FAIL
    REQRST_MOTOR_TEST,                   ///< MOTOR TEST FAIL
    REQRST_BATTONEWIRE_READERROR,        ///< BATTERY ONEWIRE READ ERROR
    REQRST_BATTONEWIRE_WRITEERROR,       ///< BATTERY ONEWIRE WRITE ERROR
    REQRST_I2CBUSLOCKUP,                 ///< Request Reset due to I2C Bus LockUp
    PERMFAIL_OLEDSELFTEST,               ///< OLED SELF TEST
    PERMFAIL_ONEWIREMASTER_COMMFAIL,     ///< ONE WIRE MASTER CHIP COMMUNICATION FAIL
    PERMFAIL_ONEWIRE_AUTHFAIL,           ///< ONEWIRE AUTHENTICATION FAIL
    PERMFAIL_ONEWIRE_WRITEFAIL,          ///< ONEWIRE WRITE FAIL
    PERMFAIL_ONEWIRE_READFAIL,           ///< ONEWIRE READ FAIL
    ERR_PERMANENT_FAIL_ONEWIRE_SHORT,    ///< ONEWIRE SHORT
    PERMFAIL_BATTERYONEWIRE_SELFTESTFAIL,///< BATTERY ONEWIRE SELF TEST FAIL
    HANDLE_EOL_ZEROBATTCHARGECYCLE,      ///< HANDLE EOL ZERO BATTERY CHARGE CYCLE
    ACCEL_SELFTEST_FAIL,                 ///< ACCELEROMETER SELF TEST FAIL
    REQRST_MICROHARD_FAULT,             ///< Request Reset due to MicroController Hard Faults
    REQRST_RAM_INTEGRITYFAIL,           ///< Request Reset due to RAM Integrity Test Fail
    REQRST_PROGRAMCODE_INTEGRITYFAIL,   ///< Request Reset due to Program Code Integrity Fail
    REQRST_MEMORYFENCE_ERROR,           ///< Request Reset due to Memory Fence Test Error
    REQRST_FPGA_READFAIL,               ///< Request Reset deu to FPGA READ FAIL
    REQRST_MOTORSTALLS_NOTCOMMANDED,    ///< Request Reset due to Motor STALLS
    REQRST_GPIOEXP_COMMFAIL,            ///< Request Reset due to GPIO Expander Comm fail
    REQRST_WATCHDOGINIT_FAIL,           ///< Request Reset due to WatchDog Init Fail
    REQRST_TASKMONITOR_FAIL,            ///< Request Reset due to Taskmonitor failures
    REQRST_MOO_SYSTEM_FAULT,            ///< System Fault Errors due to OS Errors Eg. OS Task, Mutex, Queue create failure
    REQRST_BATTONEWIRE_WRITEFAIL,       ///< Request Reset due to Battery One wire Write Fail
    REQRST_BATTONEWIRE_READFAIL,        ///< Request Reset due to Battery One Wire Read Fail
    BATTERY_COMMFAIL,                   ///< Battery Communication Fail
    BATTERY_TEMP_OUTOFRANGE,            ///< Battery Temperature Out of Range
    BATTSHUTDN_VOLTAGE_TOOLOW,          ///< Battery Shutdown
    BATTWARN_CHARGECYCLE_INCREMENT,     ///< Battery Warning due to CHARGE CYCLE INCREMENT BY 300
    BATT_CHARGECYCLE_EOL,               ///< Battery EOL due to CHARGE CYCLE 300
    SDCARD_NOTPRESENT,                  ///< SD CARD MISSING
    PERMFAIL_BATT_ONEWIRE_SHORT,        ///< Permanent Failure due to Battery One Wire Short
    PERMFAIL_BATT_ONEWIRE_AUTHFAIL,     ///< Permanent Failure due to Battery One Wire Authentication Fail
    HANDLE_MEMORY_ERROR,                ///< Handle Memory Error
    PIEZO_GPIO_FAIL,                    ///< PIEZO GPIO ERROR
    FILESYS_INTEGRITYFAIL,              ///< FILE SYS INTEGRITY FAIL
    BATTERY_ISLOW,                      ///< Battery is Low
    BATTERY_ISINSUFFICIENT,             ///< Battery is Insufficient
    USB_COMMUNICATION_FAIL,             ///< USB Communication fail
    RTC_ONEWIRE_COMMFAIL,               ///< RTC OneWire Communiction Fail
    ACCELEROMETER_COMMFAIL,             ///< Accelerometer Comm Fail
    HEARTBEAT_GPIO_FAIL,                ///< HeartBeat GPIO FAIL
    GREENKEY_GPIO_FAIL,                 ///< SAFETY key Failure
    ERRSHELL_UNSUPPORTED_CLAMSHELL,     ///< ERROR SHELL due to Unsupported Clampshell
    ERRSHELL_CLAMSHELL_AUTHFAIL,        ///< ERROR SHELL due to Clamshell Auth fail
    ERR_CLAMSHELL_ONEWIRE_SHORT,        ///< ERROR SHELL due to ONEWIRE Short
    ERRUSED_CLAMSHELLID_DOESNTMATCH,    ///< USED CLAMSHELL, CLAMSHELL ID DOESN'T MATCH
    UNSUPPORTED_ADAPTER_DETECTED,       ///< Unsupported Adapter
    ADAPTER_ERR_START = UNSUPPORTED_ADAPTER_DETECTED, ///< Set the Adapter Error Start for future use  /// \todo 08/05/2022 DAZ - Violation of coding standard. Trouble for following enums?
    UNKNOWN_ADAPTER_DETECTED,           ///< Unknown Adapter Detected
    ADAPTER_AUTH_FAIL,                  ///< Adapter Authentication fail
    ADAPTER_CRC_FAIL,                   ///< Adapter CRC fail
    ADAPTER_SGCOEFF_ZERO,               ///< Adapter SG Values zero
    ADAPTER_ONEWIRE_SHORT,              ///< Adapter OneWire Short
    HANDLE_EOL_ZEROPROCEDURECOUNT,      ///< Handle procedure count 0
    HANDLE_EOL_ZEROFIRINGCOUNTER,       ///< Handle fire count 0
    ONEWIRE_NVM_TESTFAIL,               ///< NVM test fail
    ONEWIRE_SHORT_NO_DEVICE,            ///< 1W Bus short without device connected       
    HANDLE_PROCEDURE_FIRE_COUNT_TEST_FAIL,///< Handle Procedure Count or Fire Count Test Failed    
    LAST_ERROR_CAUSE
} ERROR_CAUSE;

/*! \struct FAULTINFO_STARTUP
 *   Fault Information during Startup
 */
typedef struct
{
    uint64_t ErrorStatus;            ///< Cause for the Error
    bool FaultHandlerAppInit;        ///< True - App is ready Publish  Error signals , False - App not yet initiliazed, collect the Errors
} FAULTINFO_STARTUP;

typedef struct
{
    char_t *    ErrorCauseStrings;
    SIGNAL      Sig;
} CAUSETOSIG;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
void FaultHandlerInit(void);
uint32_t GetHeartBeatLedPeriod(void);
void SetHeartBeatPeriod(uint32_t HbPeriod);
bool FaultHandlerSetFault(ERROR_CAUSE Error, bool ErrorStatus);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* FAULTHANDLER_H */

