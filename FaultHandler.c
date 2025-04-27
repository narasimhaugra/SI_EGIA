#ifdef __cplusplus  /* Header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup FaultHandler
 * \{
 *
 * \brief   Accumulate system faults, publish the respective Fault signal.
 *
 * \details This module handles faults in two ways
 *          i.  Faults that occur before APP Startup.
 *          ii. Fault that occur after after APP Startup.
 *
 * \note    Fault respective Signal can be published only whe App is initialized
 *          Faults before App Initialization are collected and are published once App is ready
 *          at initial transitioning to App Start
 *          Faults after App Initialization are published immediately
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    FaultHandler.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "FaultHandler.h"
#include "Signia_FaultEvents.h"
#include "Signals.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */

/******************************************************************************/
#define CLEAR_ERRORCAUSE                         (0u)
#define LOG_GROUP_IDENTIFIER              (LOG_GROUP_FH)        ///< Log Group Identifier

/******************************************************************************/
/*                    Local Type Definition(s)  / Function Prototypes         */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
// this MUST align with enum ERROR_CAUSE
const CAUSETOSIG CauseToSig_Table[LAST_ERROR_CAUSE] =
{
  { "NO ERROR CAUSE"                             ,LAST_SIG        },               // No Error Cause     
  { "ERR_REQ_RST, FPGA SELF TEST FAIL"           ,P_REQ_RST_SIG   },               // FPGA SELF TEST FAIL     
  { "ERR_REQ_RST, MOTOR TEST FAIL"               ,P_REQ_RST_SIG   },               // MOTOR TEST FAIL
  { "ERR_REQ_RST, BATT ONEWIRE READ ERROR"       ,P_REQ_RST_SIG   },               // BATTERY ONEWIRE READ ERROR
  { "ERR_REQ_RST, BATT ONEWIRE WRITEERROR"       ,P_REQ_RST_SIG   },               // BATTERY ONEWIRE WRITE ERROR
  { "ERR_REQ_RST, I2C BUS LOCKUP"                ,P_REQ_RST_SIG   },               // I2C Bus Lockup
  { "PERMFAIL, OLEDSELFTEST"                     ,P_PERM_FAIL_SIG },               // OLED SELF TEST1 
  { "PERMFAIL, ONEWIREMASTER COMMFAIL"           ,P_PERM_FAIL_SIG },               // ONE WIRE MASTER CHIP COMMUNICATION FAIL
  { "PERMFAIL, ONEWIRE AUTHENTICATE FAIL"        ,P_PERM_FAIL_SIG },               // PERMFAIL_ONEWIRE_AUTHFAIL
  { "PERMFAIL, ONEWIRE WRITE FAIL"               ,P_PERM_FAIL_SIG },               // PERMFAIL_ONEWIRE_WRITEFAIL
  { "PERMFAIL, ONEWIRE READ FAIL"                ,P_PERM_FAIL_SIG },               // PERMFAIL_ONEWIRE_READFAIL
  { "PERMFAIL, ONEWIRE SHORT"                    ,P_PERM_FAIL_SIG },               // PERMFAIL_ONEWIRE_SHORT
  { "PERMFAIL, BATTERY ONEWIRE SELFTEST FAIL"    ,P_PERM_FAIL_SIG },               // BATTERY ONEWIRE SELF TEST FAIL
  { "HANDLE_EOL ZERO BATT CHARGECYCLE"           ,P_HANDLE_EOL_SIG},               // HANDLE EOL ZERO BATTERY CHARGE CYCLE
  { "ACCEL SELFTEST FAIL"                        ,P_ACCELERR_SIG  },               // ACCELEROMETER SELF TEST FAIL
  { "ERR_REQ_RST, MCU HARD FAULTS"               ,P_REQ_RST_SIG   },               // REQRST_MICROHARD_FAULT
  { "ERR_REQ_RST, RAM INTEGRITY TEST FAIL"       ,P_REQ_RST_SIG   },               // REQRST_RAM_INTEGRITYFAIL
  { "ERR_REQ_RST, PROGRAM FLASH INTEGRITY FAIL"  ,P_REQ_RST_SIG   },               // REQRST_PROGRAMCODE_INTEGRITYFAIL
  { "ERR_REQ_RST, MEMORY FENCE ERROR"            ,P_REQ_RST_SIG   },               // REQRST_MEMORYFENCE_ERROR
  { "ERR_REQ_RST, FPGA READ FAIL"                ,P_REQ_RST_SIG   },               // REQRST_FPGA_READFAIL
  {  "ERR_REQ_RST, MOTOR STALL NOT COMMANDED"    ,P_REQ_RST_SIG   },               // REQRST_MOTORSTALLS_NOTCOMMANDED
  {  "ERR_REQ_RST, GPIO EXP COMM FAIL"           ,P_REQ_RST_SIG   },               // REQRST_GPIOEXP_COMMFAIL
  {  "ERR_REQ_RST, WATCHDOG INIT"                ,P_REQ_RST_SIG   },               // REQRST_WATCHDOGINIT_FAIL
  {  "ERR_REQ_RST_TASKMONITOR FAIL"              ,P_REQ_RST_SIG   },               // REQRST_TASKMONITOR_FAIL
  {  "REQRST_MOO_SYSTEM_FAULT, System Fault"     ,P_SYSTEM_FAULT_SIG },            // REQRST_MOO_SYSTEM_FAULT
  {  "ERR_REQ_RST, BATT ONEWIRE WRITE FAIL"      ,P_REQ_RST_SIG   },               // REQRST_BATTONEWIRE_WRITEFAIL
  {  "ERR_REQ_RST, BATT ONEWIRE READ FAIL"       ,P_REQ_RST_SIG   },               // REQRST_BATTONEWIRE_READFAIL
  {  "BATT COMM FAIL"                            ,P_BATT_COMM_SIG },               // BATTERY_COMMFAIL
  {  "BATT TEMP OUT OF RANGE"                    ,P_BATT_TEMP_SIG },               // BATTERY_TEMP_OUTOFRANGE
  {  "BATT SHUTDOWN, VOLTAGE INSUFFICIENT"       ,P_BATT_SHUTDN_SIG},              // BATTSHUTDN_VOLTAGE_TOOLOW
  {  "BATT WARNING, CHARGECYCLE MAXIMUM"         ,P_BATT_WARN_SIG },               // BATTWARN_CHARGECYCLE_INCREMENT
  {  "BATTERY EOL, CHARGECYCLES EXCEEDED"        ,P_BATT_EOL_SIG  },               // BATT_CHARGECYCLE_EOL
  {  "SD CARD NOT PRESENT"                       ,P_SDCARD_ERROR_SIG },            // SD CARD ERROR: SD CARD MISSING
  {  "PERMFAIL, BATT ONEWIRE SHORT"              ,P_PERM_FAIL_SIG },               // PERMFAIL_BATT_ONEWIRE_SHORT
  {  "PERMFAIL, BATT ONEWIRE AUTHENTICATE FAIL"  ,P_PERM_FAIL_SIG },               // PERMFAIL_BATT_ONEWIRE_AUTHFAIL
  {  "HANDLE MEMORY ERROR"                       ,P_HANDLE_MEM_SIG},               // HANDLE_MEMORY_ERROR
  {  "PIEZO GPIO FAIL"                           ,P_PIEZO_ERROR_SIG},              // PIEZO_GPIO_FAIL
  {  "FILE SYS INTEGRITY"                        ,P_FILESYS_INTEGRITY_SIG},        // FILESYS_INTEGRITYFAIL
  {  "BATT LOW, 9%< BATT CAPACITY <= 25%"        ,P_BATTERY_LOW_SIG },             // BATTERY_ISLOW
  {  "BATT INSUFF, BATT CAPACITY <=9%"           ,P_BATTERY_LEVEL_INSUFF_SIG},     // BATTERY_ISINSUFFICIENT
  {  "USB COMM FAIL"                             ,P_USB_ERROR_SIG},                // USB_COMMUNICATION_FAIL
  {  "RTC ONEWIRE COMM FAIL"                     ,P_RTC_ERROR_SIG},                // RTC_ONEWIRE_COMMFAIL
  {  "ACCEL COMM FAIL"                           ,P_ACCELERR_SIG },                // ACCELEROMETER_COMMFAIL
  {  "HEARTBEAT GPIO FAIL"                       ,P_HBEAT_GPIOFAIL_SIG},           // HEARTBEAT_GPIO_FAIL
  {  "GREENKEY GPIO FAIL"                        ,P_GNKEY_LED_SIG},                // GREENKEY_GPIO_FAIL
  {  "UNSUPPORTED CLAMSHELL"                     ,P_ERR_SHELL_SIG},                // ERRSHELL_UNSUPPORTED_CLAMSHELL
  {  "CLAMSHELL AUTHENTICATE FAIL"               ,P_ERR_SHELL_SIG},                // ERRSHELL_CLAMSHELL_AUTHFAIL
  {  "CLAMSHELL ONEWIRE SHORT"                   ,P_ERR_SHELL_SIG},                // ERRSHELL_CLAMSHELL_ONEWIRE_SHORT
  {  "USED CLAMSHELL, ID DOESN'T MATCH"          ,P_USED_SHELL_SIG},               // ERRUSED_CLAMSHELLID_DOESNTMATCH
  {  "UNSUPPORTED ADAPTER DETECTED"              ,P_UNSUPPORTED_ADAPTER_SIG},      // UNSUPPORTED_ADAPTER_DETECTED
  {  "UNKNOWN ADAPTER DETECTED"                  ,P_ADAPTER_ERROR_SIG},            // UNKNOWN_ADAPTER_DETECTED
  {  "ADAPTER AUTHENTICATE FAIL"                 ,P_ADAPTER_ERROR_SIG},            // ADAPTER_AUTH_FAIL
  {  "ADAPTER CRC FAIL"                         ,P_ADAPTER_ERROR_SIG},
  {  "STRAIN GAUGE COEFF ZERO"                   ,P_ADAPTER_ERROR_SIG},            // ADAPTER_SGCOEFF_ZERO
  {  "ADAPTER ONEWIRE SHORT"                     ,P_ADAPTER_ERROR_SIG},            // ADAPTER_ONEWIRE_SHORT
  {  "HANDLE EOL, ZERO PROCEDURE COUNT"          ,P_HANDLE_EOL_SIG   },            // HANDLE_EOL_ZEROPROCEDURECOUNT
  {  "HANDLE EOL, ZERO FIRE COUNT"               ,P_HANDLE_EOL_SIG   },            // HANDLE_EOL_ZEROFIRINGCOUNTERON
  {  "ERR_PERM_FAIL_WOP, ONEWIRE DEVICE NVM TEST FAIL"                     ,P_PERM_FAIL_WOP_SIG },            // OneWire NVM test fail with PermFailWOP
  {  "ONEWIRE SHORT NO DEVICE"                   ,P_ERROR_OWSHORT_NO_DEVICE_SIG }, // OneWire Bus Short without device
  {  "HANDLE PROCEDURE FIRE COUNT TEST FAILED"   ,P_HANDLE_FIRE_PROCEDURE_COUNT_TEST_SIG} // Handle Procedure Count or Fire Count Test Failed
};

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
FAULTINFO_STARTUP  FaultInfoFromStartup;        ///< Holds Fault Info before App Startup  /// \todo 05082022 KA: local or global?
static uint32_t HeartBeatLedPeriod;             ///< HeartBeat Led Period
static OS_EVENT * mhFHMutex;                    ///< Fault Handler Mutex

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
static bool FaultHandlerAfterAppInit(ERROR_CAUSE Cause, bool ErrorStatus);
static void FaultHandlerBeforeAppInit(ERROR_CAUSE ErrorCause, bool ErrorStatus);

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   FaultHandler Initialization
 *
 * \details This function clears Errors, ErrorCause which are handled before App Startup
 *
 * \note  This API has to be called only from main
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void FaultHandlerInit(void)
{
    uint8_t u8OSError;

    /* Clear Status, Error Causes */
    FaultInfoFromStartup.ErrorStatus &= CLEAR_ERRORCAUSE;
    SetHeartBeatPeriod(SEC_1);
    FaultInfoFromStartup.FaultHandlerAppInit = false;

    /* Create a Mutex to protect parallel GPIO calls. */
    mhFHMutex = SigMutexCreate("FAULT HANDLER MUTEX", &u8OSError);

    /* Are we able to create the Mutex? */
    if ( NULL == mhFHMutex )
    {
        Log(ERR, "Fault Handler: OSMutexCreate Error!");
    }
}

/* ========================================================================== */
/**
 * \brief   Fault Manager Set fault during Handle startup
 *
 * \details This function sets the respective fault, its cause during startup.
 *
 * \param   ErrorCause  - Holds the ErrorCause bit set
 * \param   ErrorStatus - Set or Clear Error
 *
 * \return  None
 *
 * ========================================================================== */
static void FaultHandlerBeforeAppInit(ERROR_CAUSE ErrorCause, bool ErrorStatus)
{
   uint8_t u8OSError;

   do
   {
       if ( LAST_ERROR_CAUSE <= ErrorCause )
       {
           break;
       }

       /* Fault Handler, Set a Mutex entry. */
       OSMutexPend(mhFHMutex, OS_WAIT_FOREVER, &u8OSError);
       if (OS_ERR_NONE != u8OSError)
       {
           /* Error, Exit */
           break;
       }

       /* This variable will get update from mulitple thread, until App Startup is called*/
       /* Faults before App Startup are considered startup and Later as runtime */
       /* Set the ErrorStatus, Error Cause for each Error */
       if ( ErrorStatus )
       {
           FaultInfoFromStartup.ErrorStatus |= (1 << ErrorCause );
       }
       else
       {
            /*Clear the Error*/
            FaultInfoFromStartup.ErrorStatus &= ~(1 << ErrorCause );
       }

       /* Done with the Error Status update, Post the Fault Handler Mutex */
       OSMutexPost(mhFHMutex);

   } while ( false );
}

/* ========================================================================== */
/**
 * \brief   Publish Handle Error Signals
 *
 * \details This function publishes the Handle Error Signals after APP Initilization
 *
 * \param   Cause       - Cause for the Error
 * \param   ErrorStatus - Set or Clear Error
 *
 * \return  bool
 * \retval  False - Error in Publishing Signal
 * \retval  True - Successfully Published Signal
 *
 * ========================================================================== */
static bool FaultHandlerAfterAppInit(ERROR_CAUSE Cause, bool ErrorStatus)
{
    bool Status;

    do
    {
        if ( LAST_ERROR_CAUSE <= Cause )
        {
            Status = false;
            Log(DBG, "Undefined ERROR Cause: %d ", Cause);
            break;
        }
        /* Set Error */
        if ( ErrorStatus )
        {
            /* log all Permanent Failures with a log level "FLT  warnings as "WNG" and other Errors as "ERR" */
            if ( P_PERM_FAIL_SIG == CauseToSig_Table[Cause].Sig )
            {
                Log(FLT, "Fault: %s",CauseToSig_Table[Cause].ErrorCauseStrings);
            }
            else if ( P_BATT_WARN_SIG == CauseToSig_Table[Cause].Sig )
            {
                Log(WNG, "Warning: %s",CauseToSig_Table[Cause].ErrorCauseStrings);
            }
            else
            {
                Log(ERR, "Error: %s",CauseToSig_Table[Cause].ErrorCauseStrings);
            }
        }
        else
        {
            /* Clear Error */
            Log(ERR, "Clear Error: %s",CauseToSig_Table[Cause].ErrorCauseStrings);
        }
        Status = Signia_ErrorEventPublish( Cause, ErrorStatus );
    } while (false);
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Update Heart Beat Led On, Off Time
 *
 * \details This function is used to set the Heart Beat Led On, Off time
 *
 * \param   HbPeriod - HearBeat Led On, Off time Period
 *
 * \return  None
 *
 * ========================================================================== */
void SetHeartBeatPeriod(uint32_t HbPeriod)
{
    HeartBeatLedPeriod = HbPeriod;
}

/* ========================================================================== */
/**
 * \brief   Get Heart Beat Led On, Off Time
 *
 * \details This function is used to get the Heart Beat Led On, Off time
 *
 * \param   < None >
 *
 * \return  uint32_t - HeartBeat (Led On, Off Time Period)
 *
 * ========================================================================== */
uint32_t GetHeartBeatLedPeriod(void)
{
    return HeartBeatLedPeriod;
}

/* ========================================================================== */
/**
 * \brief   Publish Error signal at Startup or during Runtime
 *
 * \details This API is used to log startup error or publish the error signal
 * \details This API can be used to set or clear an Error. The same Error signal is published to set or clear the Error with different States of Error i.e., Set (true) and Clear (false)
 *
 * \param   Error       - Error Cause
 * \param   ErrorStatus - Set or Clear Error
 *
 * \return  bool - status
 *
 * ========================================================================== */
bool FaultHandlerSetFault(ERROR_CAUSE Error, bool ErrorStatus)
{
    bool Status;

    Status = true;

    /* Is App Initiliazed */
    if ( !FaultInfoFromStartup.FaultHandlerAppInit )
    {
        /* Log Startup Error */
        FaultHandlerBeforeAppInit(Error, ErrorStatus);
    }
    else
    {
        /* Log Runtime Error */
        Status = FaultHandlerAfterAppInit(Error, ErrorStatus);
    }
    return Status;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* Header compatible with C++ project */
}
#endif

