#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Handle
 * \{
 *
 * \brief   Helper functions for the Handle Active Object
 *
 * \details The Handle Active object invokes states that are defined outside of the
 *          platform repository. This module contains helper functions used by the
 *          states in the Handle AO. This separate file is provided to ease maintenance
 *          and readability.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    HandleUtil.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Signia.h"
#include "HandleUtil.h"
#include "Signia_ChargerManager.h"
#include "FaultHandler.h"

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
//#define LOG_GROUP_IDENTIFIER    LOG_GROUP_AO

#define DATE_STR_LENGTH 23          ///< size of string to hold date and time   /// \todo 08/01/2022 DAZ - This is also defined in logger.h. Should be centrally defined
#define COUNTDOWN_SCREENS    (3u)   ///< Rotation Count down screens

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

static void ProcessClamshell(Handle * const pMe);
static void ProcessHandleFaults(Handle * const pMe);
static bool DisplayReqRstScreens(Handle *const pMe, void (*pScreenHandle[])(void));


/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Process Clamshell connection event
 *
 * \details This function processes clamshellconnection events, Reads tje Clamshell
 *          memory to validate clamshell status and verifies if it could be used
 *
 * \param   pMe  - Pointer to local data store
 *
 * \return  None
 *
 * ========================================================================== */
static void ProcessClamshell(Handle * const pMe)
{
    DEVICE_UNIQUE_ID StoredHandleAddress;      /* Handle Address Stored in Clamshell */
    DEVICE_UNIQUE_ID StoredClamshellAddress;   /* Clamshell Address Stored in Handle */
    DEVICE_UNIQUE_ID ClamshellAddr;            /* Clamshell Address */
    DEVICE_UNIQUE_ID HandleAddr;               /* Handle Address */
    DEVICE_UNIQUE_ID DeviceOWID;               /* OW Device Address Received */
    uint8_t ClamshellStatusFlags;              /* Clamshell Status Read from Clamshell */

    /* Read Handle and Clamshell memory to get the stored address */
    DeviceMem_READ(pMe->Clamshell, FiringHandleAddress, StoredHandleAddress);
    DeviceMem_READ(pMe->Handle, LastClamshellAddress, StoredClamshellAddress);

    /* Read Clamshell Status flags */
    DeviceMem_READ(pMe->Clamshell, StatusFlags, ClamshellStatusFlags);

    do
    {
        ProcessHandleFaults(pMe);

        /* Format the Clamshell Address for inline with legacy */
        DeviceOWID = pMe->Clamshell.DevAddr;
        FormatDeviceAddr((uint8_t *)&DeviceOWID,(uint8_t *)&ClamshellAddr);

        /* Format the Handle Address for inline with legacy */
        DeviceOWID = pMe->Handle.DevAddr;
        FormatDeviceAddr((uint8_t *)&DeviceOWID,(uint8_t *)&HandleAddr);

        /* Test the used status of the detected CLAMSHELL */
        /* Note : Legacy code not maintaining used Status only comparing stored CLAMSHELL ID matches a detected CLAMSHELL ID */
        if ( ClamshellStatusFlags & CLAMSHELL_STATUS_FLAG_USED )
        {
            /* Check if the HANDLE software stored CLAMSHELL ID matches a detected used CLAMSHELL ID.*/
            if ( StoredClamshellAddress != ClamshellAddr )
            {
                Log(DEV, "Used Clamshell : Id %llx", ClamshellAddr);
                //pMe->Clamshell.Status = AM_DEVICE_ACCESS_FAIL; // \\todo 03-16-2022 JA : Check, this fails clamshell connection. Disabling for temporarily
                pMe->Clamshell.Status = AM_DEVICE_CONNECTED;
                pMe->Clamshell.ClamshellEOL = true;
                // Set Used Clamshell Error
                FaultHandlerSetFault(ERRUSED_CLAMSHELLID_DOESNTMATCH,SET_ERROR);
                break;
            }
            Log(DEV, "Same clamshell as the previous one : Id %llx", ClamshellAddr);
        }
        else   /* Not Used Clamshell */
        {
            Log(DEV, "New clamshell ");
            /* Update the used state to the CLAMSHELL 1_WIRE EEPROM after a detected CLAMSHELL is verified as authentic and unused. */
            ClamshellStatusFlags |= CLAMSHELL_STATUS_FLAG_USED;
            DeviceMem_WRITE(pMe->Clamshell, StatusFlags, ClamshellStatusFlags);
            /* Store  CLAMSHELL ID in HANDLE NVM memory */
            DeviceMem_WRITE(pMe->Handle, LastClamshellAddress, ClamshellAddr);
            /* Store  Handle Address in Clamshell NVM memory */
            DeviceMem_WRITE(pMe->Clamshell, FiringHandleAddress, HandleAddr);
        }
    } while ( false );

    /// \todo 11/18/2021 CPK remove after testing - retained for debug purposes
    Log(DEV, "Handle :%llx        Clamshell : %llx",pMe->Handle.DevAddr, pMe->Clamshell.DevAddr);
    Log(DEV, "FiringHandleAddress :%llx LastClamshellAddress: %llx",HandleAddr,ClamshellAddr);
}

/* ========================================================================== */
/**
 * \brief   Process Handle faults identified during startup
 *
 * \details This function plays appropriate tones based on the
 *          Handle faults identified during startup
 *
 * \param   pMe  - Pointer to local data store
 *
 * \return  None
 *
 * ========================================================================== */
static void ProcessHandleFaults(Handle * const pMe)
{
    bool ErrorStatus;

    do
    {
        /* Play the FAULT_TONE if the HANDLE is in an ERROR state upon CLAMSHELL detection. */
        /* Is ACCEL ERR is active or ERR FILE is active */
        ErrorStatus = pMe->ActiveFaultsInfo.IsFileSysErr || pMe->ActiveFaultsInfo.IsAccelErr;
        if ( ErrorStatus )
        {
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            break;
        }
    } while ( false );
}

/* ========================================================================== */
/**
 * \brief   Display the Request Reset screen1, screen2
 *
 * \details This function displays the Request Reset screen1, screen2 of screen Sequence B, C & D as given in below table
 *                    SCREEN               DELAY    SEQUENCE
 *
 *                    SCREEN1              500ms     B or C or D
 *                    SCREEN2              500ms     B or C or D
 *                    SCREEN1              500ms     B or C or D
 *                    SCREEN2              500ms     B or C or D
 *                    SCREEN1              500ms     B or C or D
 *                    SCREEN2              500ms     B or C or D
 *
 * \note    Seq B,C,D will have different SCREEN1, SCREEN2
 *
 * \param   pMe  - Pointer to local data store
 * \param   pScreenHandle - Pointer to Screens
 *
 * \return  bool - progress
 * \retval  true  - Display of screens as given above(for any sequence B,C or D) is completed
 * \retval  false - Display screen as given is not completed (for any sequence B,C or D)
 *
 * ========================================================================== */
bool DisplayReqRstScreens(Handle *const pMe, void (*pScreenHandle[])(void))
{
    bool Status;

    Status = false;
    switch (pMe->ReqRstScreenInfo.ReqRstScreen )
    {
        case REQRST_SCREEN_ONE:
             /* Display Request Reset Screen1 */
             pScreenHandle[REQRST_SCREEN_ONE]();
             /*Change to Screen2, on next Timeout Screen2 is display*/
             pMe->ReqRstScreenInfo.ReqRstScreen = REQRST_SCREEN_TWO;
           break;
        case REQRST_SCREEN_TWO:
             /* Count number of times Scree1, Screen2 displayed*/
             pMe->ReqRstScreenInfo.ScreenDispCount += 1;
             /* Is SCREEN1, SCREEN2 displayed each 3 times */
             Status = (pMe->ReqRstScreenInfo.ScreenDispCount == MAX_REQRST_SCREENSDISP ) ? true : false;
             /* Reset count to 0 on Disp Count reaching 3 */
             pMe->ReqRstScreenInfo.ScreenDispCount  = ( pMe->ReqRstScreenInfo.ScreenDispCount == MAX_REQRST_SCREENSDISP ) ? 0 : pMe->ReqRstScreenInfo.ScreenDispCount;
             /* Move to Screen1, on next call Screen1 is displayed */
             pMe->ReqRstScreenInfo.ReqRstScreen     = REQRST_SCREEN_ONE;
             /* Display Screen2 */
             pScreenHandle[REQRST_SCREEN_TWO]();
           break;
        default:
           /*Do nothing*/
           break;
    }
    /* Start Timer */
    AO_TimerArm(&pMe->FaultTimer, REQRESET_SEQSCREEN_TIMEOUT,0);
    return Status;
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Accelerometer event handler
 *
 * \details Handles accelerometer events generated periodically or whenever
 *          move/fall is detected.
 *
 * \param   ACCELINFO *pAccelInfo - Accelerometer Data
 *
 * \return  None
 *
 * ========================================================================== */
void HNutil_ProcessAccelEvents(ACCELINFO *pAccelInfo)
{
    QEVENT_ACCEL    *pAccelEvent;
    static uint32_t Timer;       /* By default intialized with zero. Holds the time instance after publishing the Signal */


    do
    {
        /* Movement of handle is publishing the MOVEMENT sig continuosly, Queue is getting Filled up and QonAssert is raised.
           To avoid this 100ms time gap is maintained between publishing of each MOVEMENT sig */
        /* Check if 100ms expired from last published Signal */
        if ( ( SigTime() - Timer ) < MSEC_100 )
        {
           /* 100ms not expired, break */
            break;
        }
        /* Publish MOVEMENT Sig 100ms expired from last published Signal*/
        pAccelEvent = AO_EvtNew(P_MOVEMENT_SIG, sizeof(QEVENT_ACCEL));

        if(pAccelEvent)
        {
            memcpy(&pAccelEvent->Info, pAccelInfo, sizeof(ACCELINFO));
            AO_Publish(pAccelEvent, NULL);

            /* Get time instance */
            Timer = SigTime();
        }
    } while ( false );
}




/* ========================================================================== */
/**
 * \brief   Charger Event Handler
 *
 * \details Converts charger manager events to QPC events and publish
 *
 * \param   pChargerInfo - Charger state and infoormation.
 *
 * \return  None
 *
 * ========================================================================== */
void HNutil_ProcessChargerEvents(CHARGER_INFO *pChargerInfo)
{
    QEVENT_CHARGER *pInfo;
    SIGNAL Signal[CHARGER_EVENT_LAST] =
    {
       P_OFF_CHARGER_SIG,
       P_ON_CHARGER_SIG,
       P_CHARGER_FAULT_SIG,
       P_BATTERY_INFO_SIG,
       P_ONCHARGER_WAKEFROMSLEEP_SIG
    };

    if (pChargerInfo->Event < CHARGER_EVENT_LAST)
    {
        pInfo = AO_EvtNew(Signal[pChargerInfo->Event], sizeof(QEVENT_CHARGER));
        if(pInfo)
        {
            memcpy(&pInfo->Info, pChargerInfo, sizeof(CHARGER_INFO));
            AO_Post(AO_Handle, &pInfo->Event, NULL);
        }
    }
}
/* ========================================================================== */
/**
 * \brief   Update the Battery Error status based on Battery conditions
 *
 * \details Checks for the following Error conditions
 * 
 *   LowBatTriggered        : Flag sets to True If LowBattery Detected once ( stops to log cause continously ) 
 *   InsufBatTriggered      : Flag sets to True If InsuffBattery Detected once ( stops to log cause continously ) 
 *   ShudnBatTriggered      : Flag sets to True If ShutdwnBattery Detected once ( stops to log cause continously ) 
 *   Battery Low            : Battery RSOC Level <= 25%
 *   Battery Insufficient   : Battery RSOC Level < 9%
 *   Battery Shutdown fault : Battery Voltage < 6626mV 
 *
 * \param   pInfo           - Pointer to the Battery info
 * \param   pMe             - Pointer to local data store
 * \param   ReloadConnected - true  - Reload is connected
 *                            false - Reload is not connected
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_BatteryUpdateErrors(CHARGER_INFO *pInfo, Handle *const pMe, bool ReloadConnected)
{
    do
    {
      if ((pInfo->BatteryLevel >= BATTERY_LIMIT_LOW_MIN  && pInfo->BatteryLevel <= BATTERY_LIMIT_LOW) && !pMe->LowBatteryTriggered)
      {
        FaultHandlerSetFault(BATTERY_ISLOW, SET_ERROR);
        pMe->LowBatteryTriggered = true;
      }
      else if ( (pInfo->BatteryLevel <= BATTERY_LIMIT_INSUFFICIENT) && !pMe->InsufficientBatteryTriggered)
      {
        FaultHandlerSetFault(BATTERY_ISINSUFFICIENT, SET_ERROR);
        pMe->InsufficientBatteryTriggered = true;
      }
      else if ((pInfo->BatteryLevel <= BATT_RSOCSHUTDOWN) && !ReloadConnected && !pMe->ShutdownBatteryTriggered)
      {
        FaultHandlerSetFault(BATTSHUTDN_VOLTAGE_TOOLOW, SET_ERROR);
        pMe->ShutdownBatteryTriggered = true;
      }
      else
      {
        //Do nothing
      }
        
    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Initialize System Time
 *
 * \details Reads clock reference from RTC chip and Initializes system clock
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void HNutil_SystemClockUpdate(void)
{
    RTC_SECONDS     RtcTime;
    CLK_DATE_TIME   DateTime;
    char_t          DateTimeStr[DATE_STR_LENGTH];
    CLK_TZ_SEC      TzSec;

    TzSec = 0;  // UTC

    if (BATT_RTC_STATUS_OK == L3_BatteryRtcRead(&RtcTime))
    {
        L2_OnchipRtcWrite(RtcTime);                  /* set handle RTC to 1-Wire RTC time */

        /// \todo 12/07/2022 CPK - Reference Clock logic to be updated.
        /*
        /// \todo 12/07/2022 CPK Notes from Review.(ReviewId:37771)
        The value that comes back from BatteryRtcRead is unix time (Time in SECONDS from 1/1/70). It is being assigned to the OSTimer, which is incremented every MILLISECOND.
        As a result, the value is worthless as a date/time stamp. Would need a 42 bit (at least) number (a 64 bit value would be easier to handle in C) for
        the system timer for this to work properly -system time would be set to unix time * 1000.
        Then would have to divide it by 1000 again to get time/date stamp in unix format, w/ the % 1000 value being milliseconds..
        */
/// \\todo 02-13-2022 KA : do not set here. All the work is already done in L4/L3/L2.
//        SigTimeSet(ReferenceClock);
//        Log(REQ, "Handle clock updated from reference clock [%u]", ReferenceClock);
//        SecurityLog("Handle clock updated from reference clock [%u]", ReferenceClock);
    }
    else
    {
        /* Do nothing */
    }

    RtcTime = L2_OnchipRtcRead();
    Clk_TS_UnixToDateTime (RtcTime, TzSec, &DateTime);
    Clk_DateTimeToStr (&DateTime, CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS, DateTimeStr, sizeof(DateTimeStr)-1);
    Log (REQ, "UTC Date (Y-M-D) Time (H:M:S) is %s", DateTimeStr);

    return;
}

/* ========================================================================== */
/**
 * \brief   Process Adapter and Clamshell events
 *
 * \details This function processes clamshell/adapter connection events, updating
 *          local data and displays as appropriate.
 *
 * \note    Auto variables are permitted, but static variables are not. If a variable
 *          whose value must survive from one function call to the next, it must be
 *          defined as a member of the Handle structure and accessed via the pMe pointer
 *
 * \param   pMe  - Pointer to local data store
 * \param   pSig - Pointer to signal to process
 *
 * \return  None
 *
 * ========================================================================== */
void HNutil_ProcessDeviceConnEvents(Handle * const pMe, QEvt const * const pSig)
{
    AM_STATUS   Status;
    QEVENT_ADAPTER_MANAGER  *pEvent;
    uint16_t    FireCount;
    uint16_t    FireLimit;
    uint16_t    ProcedureCount;
    uint16_t    ProcedureLimit;

    pEvent = (QEVENT_ADAPTER_MANAGER *)pSig;

    /**
     * \todo 01/20/2022 DAZ - This is a hack to insure that handle information is set
     *                        before processing the clamshell, as it requires handle
     *                        information to process correctly. This depends on AdapterManager
     *                        establishing the handle information BEFORE issuing a Clamshell
     *                        connection event.
     *                        As a brute force solution, we are checking handle information
     *                        just before handling any possible connection event. This can
     *                        still fail if a connection event occurs before AdapterManager
     *                        has gathered handle information from OneWire.
     */
    if (AM_DEVICE_DISCONNECTED == pMe->Handle.Status)
    {
        pMe->Handle.Status = Signia_GetHandleStatus();                  // Set the handle status
        pMe->Handle.pHandle = Signia_GetHandleIF();                     // Set the pointer to handle methods & data
        pMe->Handle.DevAddr = Signia_GetHandleAddr();                   // Set the Unique ID (OneWire address)
        DeviceMem_READ(pMe->Handle, DeviceType, pMe->Handle.DevID);     // Set the device ID (Family/variant)
    }

    switch (pEvent->Event.sig)
    {
        case P_ONEWIRE_DEVICE_CHECK_SIG:
            Log(DBG, "Handle - New Onewire Device Found Device Authentication Started");
            break;
        case P_ONEWIRE_ADAPTER_CHECK_SIG:
            Log(DBG, "Handle - New Adapter Found Authentication Started");
            // ADDED FROM EGIA:
            Gui_AdapterCheck_Screen();
            break;
        case P_CLAMSHELL_CONNECTED_SIG:
            Log(DBG, "Handle - P_CLAMSHELL_CONNECTED_SIG received");
            pMe->Clamshell.Status = AM_DEVICE_CONNECTED;
            pMe->Clamshell.pHandle = (AM_CLAMSHELL_IF *)pEvent->pDeviceHandle;
            pMe->Clamshell.DevAddr =  pEvent->DevAddr;
            DeviceMem_READ(pMe->Clamshell, DeviceType, pMe->Clamshell.DevID);
            // Enable Accelerometer
            Signia_AccelEnable(true, 0, HNutil_ProcessAccelEvents);
            ProcessClamshell(pMe);
            break;

        case P_ADAPTER_CONNECTED_SIG:
            Log(DBG, "Handle - P_ADAPTER_CONNECTED_SIG received");
            pMe->Adapter.Authenticated = pEvent->Authentic;
            pMe->Adapter.Status = AM_DEVICE_CONNECTED;
            pMe->Adapter.pHandle = pEvent->pDeviceHandle;
            pMe->Adapter.DevID = DEVICE_ID_UNKNOWN;
            pMe->Adapter.AdapterUnsupported = Signia_GetAdapterStatus();
            if(pMe->Adapter.Authenticated)
            {
                DeviceMem_READ(pMe->Adapter, DeviceType, pMe->Adapter.DevID);
            }
            Gui_AdapterCheck_Screen();
            if (!(pMe->Adapter.AdapterUnsupported))
            {
                Status = HNutil_InitAdapterComm(pMe);
            }
            /* Read the Adapter FireCount, Fire Limit */
            DeviceMem_READ(pMe->Adapter, FireCount, FireCount);
            DeviceMem_READ(pMe->Adapter, FireLimit, FireLimit);
            /* Read the Adapter ProcedureCount, Procedure Limit */
            DeviceMem_READ(pMe->Adapter, ProcedureLimit, ProcedureLimit);
            DeviceMem_READ(pMe->Adapter, ProcedureCount, ProcedureCount);
            /* Calculate Adapter EOL */
            pMe->Adapter.AdapterEOL = ( FireCount >= FireLimit) || (ProcedureCount >= ProcedureLimit);
            break;

        case P_RELOAD_CONNECTED_SIG:
            Log(DBG, "Handle - P_RELOAD_CONNECTED_SIG received");
            pMe->Reload.Status = AM_DEVICE_CONNECTED;
            pMe->Reload.pHandle = pEvent->pDeviceHandle;
            DeviceMem_READ(pMe->Reload, DeviceType, pMe->Reload.DevID);
            break;

        case P_CARTRIDGE_CONNECTED_SIG:
            Log(DBG, "Handle - P_CARTRIDGE_CONNECTED_SIG received");
            pMe->Cartridge.Status= AM_DEVICE_CONNECTED;
            pMe->Cartridge.pHandle = pEvent->pDeviceHandle;
            DeviceMem_READ(pMe->Cartridge, DeviceType, pMe->Cartridge.DevID);
            break;

        case P_CLAMSHELL_REMOVED_SIG:
            Log(DBG, "Handle - P_CLAMSHELL_REMOVED_SIG received");
            pMe->Clamshell.Status = AM_DEVICE_DISCONNECTED;
            pMe->Clamshell.ClamshellEOL = false;
            // Disable Accelerometer
            Signia_AccelEnable(false, 0, NULL);
            break;

        case P_ADAPTER_REMOVED_SIG:
            Log(DBG, "Handle - P_ADAPTER_REMOVED_SIG received");
            pMe->Adapter.Status= AM_DEVICE_DISCONNECTED;
            pMe->Adapter.DevID = DEVICE_ID_UNKNOWN;
            pMe->Adapter.AdapterEOL = false;
            break;

        case P_RELOAD_REMOVED_SIG:
            Log(DBG, "Handle - P_RELOAD_REMOVED_SIG received");
            pMe->Reload.Status = AM_DEVICE_DISCONNECTED;

            /* Clear Dev Id */
            pMe->Reload.DevID   = DEVICE_ID_UNKNOWN;
            pMe->Reload.pHandle->Status = AM_STATUS_DISCONNECTED;
            break;

        case P_CARTRIDGE_REMOVED_SIG:
            Log(DBG, "Handle - P_CARTRIDGE_REMOVED_SIG received");
            pMe->Cartridge.Status= AM_DEVICE_DISCONNECTED;
            break;

        default:
            break;

    }
}

/* ========================================================================== */
/**
 * \brief   Update HeartBeat On Off Time
 *
 * \details This function updates HeartBeat On Off Time for different Error Signals
 *
 * \param   pSig - Pointer to signal to process
 *
 * \return  None
 *
 * ========================================================================== */
void HNutil_UpdateHeartBeatPeriod(QEvt const * const pSig)
{
    QEVENT_FAULT *pEvent;
    uint32 HeartBeatPeriod;

    pEvent = (QEVENT_FAULT *)pSig;

    switch (pEvent->Event.sig)
    {
        case  P_PERM_FAIL_SIG:
        case  P_PERM_FAIL_WOP_SIG:
            HeartBeatPeriod = SEC_2;
            break;

        case P_REQ_RST_SIG:
            HeartBeatPeriod = SEC_3;
            break;

        case P_HANDLE_EOL_SIG:
            HeartBeatPeriod = SEC_4;
            break;

        case P_BATT_COMM_SIG:
            HeartBeatPeriod = SEC_5;
            break;

        default:
            HeartBeatPeriod = SEC_1;
            break;
    }
    SetHeartBeatPeriod(HeartBeatPeriod);
}

/* ======================================================7==================== */
/**
 * \brief   Display the Request Reset screens in sequence
 *
 * \details This function displays the Request Reset screens in Sequence A, B, C & D
 *
 * \param   pMe  - Pointer to local data store
 *
 * \return  None
 *
 * ========================================================================== */
void HNutil_DisplayRequestScreenSeq(Handle *const pMe)
{
    bool Status;
    void (*ScreenHandler[2])(void) = {NULL,NULL};

    switch (pMe->ReqRstScreenInfo.ReqRstSeq)
    {
        case REQRSTSCREEN_SEQA:
             //Start 5 sec timer
             AO_TimerArm(&pMe->FaultTimer, REQRESET_SEQA_TIMEOUT,0);
             pMe->ReqRstScreenInfo.ReqRstSeq = REQRSTSCREEN_SEQB;
             //Display Req Reset Screen A
             Show_ResetErrScreen();
             break;

        case REQRSTSCREEN_SEQB:
             //Get SEQB screens
             ScreenHandler[0] = &Gui_ReqReset1_Screen;
             ScreenHandler[1] = &Gui_ReqReset2_Screen;
             // Display Request Reset SeqB
             Status = DisplayReqRstScreens(pMe,ScreenHandler);
             //Is SEQB screen display completed
             if ( Status )
             {
                 //Change to SEQC. In next timeout SEQC screen is displayed
                 pMe->ReqRstScreenInfo.ReqRstSeq = REQRSTSCREEN_SEQC;
             }
             break;

        case REQRSTSCREEN_SEQC:
             //Get SEQC screens
             ScreenHandler[0] = &Gui_ReqReset3_Screen;
             ScreenHandler[1] = &Gui_ReqReset4_Screen;
             //Display SEQC screens
             Status = DisplayReqRstScreens(pMe, ScreenHandler);
             if ( Status )
             {
                 //Change to SEQD. In next timeout SEQD screen is displayed
                 pMe->ReqRstScreenInfo.ReqRstSeq = REQRSTSCREEN_SEQD;
             }
             break;

        case REQRSTSCREEN_SEQD:
             //Get SEQD screen1, screen2
             ScreenHandler[0] = &Gui_ReqReset5_Screen;
             ScreenHandler[1] = &Gui_ReqReset6_Screen;
             // Display Request Reset SeqD
             Status = DisplayReqRstScreens(pMe, ScreenHandler);
             if ( Status )
             {
                 //Change to SEQA. In next timeout SEQA screen is displayed
                 pMe->ReqRstScreenInfo.ReqRstSeq = REQRSTSCREEN_SEQA;
             }
             break;

        default:
            /* Do Nothing */
            break;
    }
}

/* ========================================================================== */
/**
 * \brief   Process Display Rotation Count Down Screens
 *
 * \details This function processes Rotation Count Down Screens for activation/Deactivation
 *
 * \note    Auto variables are permitted, but static variables are not. If a variable
 *          whose value must survive from one function call to the next, it must be
 *          defined as a member of the Handle structure and accessed via the pMe pointer
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void HNutil_RotationConfigDisplayCountDownScreens(Handle *const pMe)
{
    static uint8_t ScreenCount = COUNTDOWN_SCREENS;

    switch (ScreenCount--)
    {
        case 3:
            /* Display Rotation Count Down 3 Screens based on the Activate / Deactivation */
            if(!pMe->IsKeySideRotationDisabled)
            {
                /* Display Rotation Deactivation Count Down 3 Screens based on the Side */
                if(pMe->KeySide == KEY_SIDE_LEFT)
                {
                    Gui_RotateDeactLeftCount3_ScreenSet();
                }
                else
                {
                    Gui_RotateDeactRightCount3_ScreenSet();
                }
            }
            else
            {
                /* Display Rotation Activation Count Down 3 Screens based on the Side */
                if(pMe->KeySide == KEY_SIDE_LEFT)
                {
                    Gui_RotateActivateLeftCount3_ScreenSet();
                }
                else
                {
                    Gui_RotateActivateRightCount3_ScreenSet();
                }
            }
            break;

        case 2:
            /* Display Rotation Count Down 2 Screens based on the Activate / Deactivation */
            if(!pMe->IsKeySideRotationDisabled)
            {
                /* Display Rotation Deactivation Count Down 2 Screens based on the Side */
                if(pMe->KeySide == KEY_SIDE_LEFT)
                {
                    Gui_RotateDeactLeftCount2_ScreenSet();
                }
                else
                {
                    Gui_RotateDeactRightCount2_ScreenSet();
                }
            }
            else
            {
                /* Display Rotation Activation Count Down 2 Screens based on the Side */
                if(pMe->KeySide == KEY_SIDE_LEFT)
                {
                    Gui_RotateActivateLeftCount2_ScreenSet();
                }
                else
                {
                    Gui_RotateActivateRightCount2_ScreenSet();
                }
            }
            break;

        case 1:
            /* Display Rotation Count Down 1 Screens based on the Activate / Deactivation */
            if(!pMe->IsKeySideRotationDisabled)
            {
                /* Display Rotation Deactivation Count Down 1 Screens based on the Side */
                if(pMe->KeySide == KEY_SIDE_LEFT)
                {
                    Gui_RotateDeactLeftCount1_ScreenSet();
                }
                else
                {
                    Gui_RotateDeactRightCount1_ScreenSet();
                }
            }
            else
            {
                /* Display Rotation Activation Count Down 1 Screens based on the Side */
                if(pMe->KeySide == KEY_SIDE_LEFT)
                {
                    Gui_RotateActivateLeftCount1_ScreenSet();
                }
                else
                {
                    Gui_RotateActivateRightCount1_ScreenSet();
                }
            }

        default:
            ScreenCount = COUNTDOWN_SCREENS;
            break;
    }
}

/* ========================================================================== */
/**
 * \brief   Start motor test
 *
 * \details Starts all 3 motors moving distance & speed specified either by default
 *          or by kvf file. The initial positions of all the motors are set to 0
 *          prior to starting them.
 *
 * \param   pMe  - Pointer to local data store
 * \param   pSig - Pointer to signal to process
 *
 * \return  None
 *
 * ========================================================================== */
void HNutil_StartMotorTest(Handle *const pMe, QEvt const *const pSig)
{
    Signia_MotorSetPos(MOTOR_ID0, 0); // Set starting point for motor tests
    Signia_MotorSetPos(MOTOR_ID1, 0);
    Signia_MotorSetPos(MOTOR_ID2, 0);

    Signia_MotorStart(MOTOR_ID0, MOTEST_POS, MOTEST_SPD, MOTEST_DELAY, MOTEST_TIMEOUT, MOTEST_CUR_TRIP, MOTEST_CUR_LIM, true, MOTOR_VOLT_15, 0);
    Signia_MotorStart(MOTOR_ID1, MOTEST_POS, MOTEST_SPD, MOTEST_DELAY, MOTEST_TIMEOUT, MOTEST_CUR_TRIP, MOTEST_CUR_LIM, true, MOTOR_VOLT_15, 0);
    Signia_MotorStart(MOTOR_ID2, MOTEST_POS, MOTEST_SPD, MOTEST_DELAY, MOTEST_TIMEOUT, MOTEST_CUR_TRIP, MOTEST_CUR_LIM, true, MOTOR_VOLT_15, 0);
}

/* ========================================================================== */
/**
 * \brief   Function initializing the Adapter Communication.
 *
 * \details This function is required to communicate with the adapter MCU to initialize the
 *          command interface. calls include initializing Adapter Boot, Main and to initialize the Adapter onewire
 *          Initializing Adapter one-wire is required to communicate to the Reload connected over the one wire
 *
 * \param   < None >
 *
 * \return  SIGNIA_AM_EVENT_STATUS  - Adapter Manager Status
 * \retval  SIGNIA_AM_STATUS_OK     - No error in initilalizing
 * \retval  SIGNIA_AM_STATUS_ERROR  - Error in initializing
 *
 * ========================================================================== */
AM_STATUS HNutil_InitAdapterComm(Handle * const pMe)
{
    AM_STATUS Status;
    AM_ADAPTER_IF* pAdapterHandle;

    pAdapterHandle = NULL;
    Status = AM_STATUS_ERROR;

    do
    {
        pAdapterHandle = (AM_ADAPTER_IF*)Signia_AdapterManagerDeviceHandle(AM_DEVICE_ADAPTER);
        if (NULL == pAdapterHandle)
        {
            break;
        }

        pAdapterHandle->pSupplyON();

        // Initialize Adapter Communication and command to Enter Boot, Main
        // Delay to Adapter power-up and stabilization
        OSTimeDly(MSEC_50);

        if( AM_STATUS_OK != Signia_AdapterRequestCmd(ADAPTER_ENTERBOOT,0) )
        {
            Log(DEV, "AdapterEvents: Adapter BootEnter Request Failed");
            break;
        }
        Log(DEV, "AdapterEvents: Adapter BootEnter Request Successful");


        if( AM_STATUS_OK != Signia_AdapterRequestCmd(ADAPTER_GET_VERSION,0) )
        {
            Log(DEV, "AdapterEvents: Adapter Version Request Failed");
            break;
        }
        Log(DEV, "AdapterEvents: Adapter Version Request Successful");

        if( AM_STATUS_OK != Signia_AdapterRequestCmd(ADAPTER_UPDATE_MAIN,0) )
        {
            Log(DEV, "AdapterEvents: Adapter UpdateMain Request Failed");
            break;
        }
        Log(DEV, "AdapterEvents: Adapter UpdateMain Request Successful");

        //Enter main command with 500ms delay after succesful response to allow adapter to go to main
        if( AM_STATUS_OK != Signia_AdapterRequestCmd(ADAPTER_ENTERMAIN,500) )
        {
            Log(DEV, "AdapterEvents: Adapter EnterMain Request Failed");
            break;
        }
        Log(DEV, "AdapterEvents: Adapter EnterMain Request Successful");

        if( AM_STATUS_OK != Signia_AdapterRequestCmd(ADAPTER_GET_TYPE,0) )
        {
            Log(DEV, "AdapterEvents: Adapter Type Request Failed");
            break;
        }
        Log(DEV, "AdapterEvents: Adapter Type Request Successful");

        if( AM_STATUS_OK != Signia_AdapterRequestCmd(ADAPTER_ENABLE_ONEWIRE,0) )
        {
            Log(DEV, "AdapterEvents: Adapter OneWireEnable Request Failed");
            break;
        }
        Log(DEV, "AdapterEvents: Adapter OneWireEnable Request Successful");


        if( AM_STATUS_OK != Signia_AdapterRequestCmd(ADAPTER_GET_HWVERSION,0) )
        {
            Log(DEV, "AdapterEvents: Adapter HardwareVersion Request Failed");
            break;
        }
        Log(DEV, "AdapterEvents: Adapter HardwareVersion Request Successful");

        Log(DBG, "AdapterEvents: Triggered Adapter Init Sequence");
        Status = AM_STATUS_OK;
    } while (false);

    return Status;
}



/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
