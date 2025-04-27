#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{
 *
 * \brief   Adapter Manager functions
 *
 * \details The Adapter Manager is responsible for handling all interaction
            between the Signia Handle and the Adapter. The Adapter Manager also
            covers communications with additional devices connected to the
            Signia Handler via Adapter such as Reload, Cartridge and Clamshell.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_AdapterManager.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Signia_AdapterManager.h"
#include "L2_Uart.h"
#include "L3_OneWireEeprom.h"
#include "L3_OneWireRtc.h"
#include "FaultHandler.h"
#include "TestManager.h"
#include "L4_DetachableCommon.h"
#include "Signia_AdapterEvents.h"
#include "Signia_ChargerManager.h"
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER             (LOG_GROUP_ADAPTER)   /*! Log Group Identifier */
#define ADAPTER_MNGR_TASK_STACK_SIZE     (512u)                /*! Adapter Manager Task Stack Size */
#define MAX_AM_REQUESTS                  (10u)                 /*! Maximum request size */
#define MSG_Q_TIMEOUT_TICKS              (10u)                 /*! Message Q Timeout ticks */

#define HANDLE_DEV_ID                    (0x401)
#define BATTERY_DEV_ID                   (0x402)
#define CLAMSHELL_DEV_ID                 (0x403)
#define CHARGER_DEV_ID                   (0x404)

#define OW_DEVICE_COUNT                  (3u)                  /*! One wire device count threshold */
#define OW_KEEP_ALIVE_INTERVAL           (500u)                /*! One wire keep alive interval */
#define OW_SCAN_INTERVAL                 (1000u)               /*! One wire device scan intarval */
#define FLUSH_TIMEOUT_MSEC               (MSEC_100)            /*! Uart flush timeout */
#define MAX_OWSTARTUP_ERRORS             (5u)                  /// Maximum one wire startup errors
#define NO_DEVICE_ONBUS                  (0xFF)
/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
extern AM_ADAPTER_IF 	AdapterInterface;
extern AM_CLAMSHELL_IF 	ClamshellInterface;
extern AM_RELOAD_IF 	ReloadInterface;
extern AM_CARTRIDGE_IF 	CartridgeInterface;
extern AM_HANDLE_IF 	HandleInterface;
extern COMM_IF 			*pAdapterComm;
OS_STK AdapterMngrTaskStack[ADAPTER_MNGR_TASK_STACK_SIZE + MEMORY_FENCE_SIZE_DWORDS];  /* Adapter Manager Task Stack */
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef struct                                         /*! Adapter Manager data */
{
    AM_HANDLER       EventHandler;                     /*! Event Handler */
    AM_STATE         AmState;                          /*! Adapter Manager state */
    AM_DEVICE_INFO   AmDeviceData[AM_DEVICE_COUNT];    /*! Device information */
} AD_APP_STATE;

typedef struct                      /*! One wire message */
{
    ONEWIRE_EVENT     Event;        /*! Device event */
    DEVICE_UNIQUE_ID  Device;       /*! Device ID */
} AM_OW_MSG;

/// \todo 11/03/2021 CPK Fault handler structures could be moved to a common location within platform - they may not belong to AM
typedef struct                                     /// Fault Handler mapping to Onewire status
{
    ONEWIRE_STATUS OwStatus;                       ///< Onewire status
    ERROR_CAUSE    ErrorCause;                           ///< Error Cause based on the error returned by onewire
} FAULT_HANDLER_OWSTATUS;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
/* Map onewire status to Error and Error causes defined in FaultHandler */
static const FAULT_HANDLER_OWSTATUS HandleStartupErrors[MAX_OWSTARTUP_ERRORS] =
{
    /* ONEWIRE_STATUS_READ/WRITE_ERROR is set when read/write fails during Handle authentication*/
    {ONEWIRE_STATUS_WRITE_ERROR,        PERMFAIL_ONEWIRE_WRITEFAIL},
    {ONEWIRE_STATUS_READ_ERROR,         PERMFAIL_ONEWIRE_READFAIL },
    /* ONEWIRE_STATUS_ERROR is set when Authentication fails*/
    {ONEWIRE_STATUS_ERROR,              PERMFAIL_ONEWIRE_AUTHFAIL },
    /* ONEWIRE_STATUS_BUS_ERROR is set when there is a short identified on onewire bus*/
    {ONEWIRE_STATUS_BUS_ERROR,          ERR_PERMANENT_FAIL_ONEWIRE_SHORT    },
    /* ONEWIRE_STATUS_NVMTEST_ERROR is set when NVM Test failed */
    {ONEWIRE_STATUS_NVMTEST_ERROR ,     ONEWIRE_NVM_TESTFAIL      }
};

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
//#pragma location=".sram"
static AD_APP_STATE AdapMgrDevData;                /*! Adapter Manager device data */
static OS_EVENT *pAdapterMgrMutex;                 /*! Adapter Manager mutex */
//static AM_SM_STATE  AppState;                      /*! Adapter Manager application state */
static OS_EVENT  *pAdapMgrQ;                       /*! Request Q */
//static bool AdapterUartCommsState;
static uint8_t  ReadData[OW_MEMORY_TOTAL_SIZE];            /* EEPROM read data */


/// \todo 01/24/2022 DAZ - Again, this is dependent on the order of the AM_DEVICE enums. Should also be const so it is saved in flash.
/// \todo 01/24/2022 DAZ - Why not add battery interface?
/// Don't Change this Function pointers order
/*! Pointer to the device handlers */
static void *DeviceHandler[AM_DEVICE_COUNT] =
{
    &HandleInterface,
    &ClamshellInterface,
    &AdapterInterface,
    &ReloadInterface,
    &CartridgeInterface,
    NULL
};

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void AdapterManagerTask(void *p_arg);
static void ConnectionProcessor(void);
static void AdapterMngrStateMachine(void);
static void AmOnewireEventHandler(ONEWIRE_EVENT OwEvent, DEVICE_UNIQUE_ID OwDevice);
bool IsAdapterConnected(void);
static void UpdateDeviceConnStatus(DEVICE_UNIQUE_ID Device, ONEWIRE_EVENT Event);
static void UpdateDeviceConnection(DEVICE_UNIQUE_ID Device, uint16_t DeviceID);
static AM_OW_MSG *GetNextAmReqMsgSlot(void);
static ONEWIRE_STATUS DeviceWriteTest(AM_DEVICE_INFO *pDeviceData);
static void SetOneWireFaultStatus(AM_DEVICE DevId, ONEWIRE_STATUS OwStatus);
static AM_STATUS GenericEepRead(ONEWIRE_DEVICE_ID Device, uint8_t *pData);
static AM_STATUS ConfigureOneWireBus(ONEWIRE_BUS Bus);
/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Adapter connection status
 *
 * \details Adapter connection status flag
 *
 * \param   < None >
 *
 * \return  bool - Adapter connection status
 * \retval  true  - If Adapter is connected
 * \retval  false - If Adapter is not connected
 *
 * ========================================================================== */
static bool IsAdapterConnected(void)
{
    bool Connected;
    AM_DEVICE_STATE state;
    state = AdapMgrDevData.AmDeviceData[AM_DEVICE_ADAPTER].State;
    Connected = ((AM_DEVICE_STATE_ACTIVE == state) || (AM_DEVICE_STATE_INVALID == state) )? true : false;

    return Connected;
}

/* ========================================================================== */
/**
 * \brief   Adapter manager state machine
 *
 * \details This function runs the Adapter Manager state machine and process all
 *          the states.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void AdapterMngrStateMachine(void)
{
    AM_STATE    State;

    /* Update the Adapter manager state */
    RunAdapterComSM();

    /* Check for Clamshell */
    if (AM_DEVICE_STATE_ACTIVE == AdapMgrDevData.AmDeviceData[AM_DEVICE_CLAMSHELL].State)
    {
        State = AM_STATE_CLAMSHELL_ARMED;
    }

    /* Check for Adapter */
    if (AM_DEVICE_STATE_ACTIVE == AdapMgrDevData.AmDeviceData[AM_DEVICE_ADAPTER].State)
    {
       State = AM_STATE_ADAPTER_ARMED;
    }

    /* Check for Reload */
    if (AM_DEVICE_STATE_ACTIVE == AdapMgrDevData.AmDeviceData[AM_DEVICE_RELOAD].State)
    {
       State = AM_STATE_RELOAD_ARMED;
    }

    /* Check for Cartridge */
    if (AM_DEVICE_STATE_ACTIVE == AdapMgrDevData.AmDeviceData[AM_DEVICE_CARTRIDGE].State)
    {
       State = AM_STATE_CARTRIDGE_ARMED;
    }

    AdapMgrDevData.AmState = State;

    TM_Hook(HOOK_ADAPTERMANAGER, (void*)&AdapMgrDevData.AmDeviceData);

}



/* ========================================================================== */
/**
 * \brief   Device Connection processor
 *
 * \details This function runs the Device state machine.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void ConnectionProcessor(void)
{
    AM_DEVICE      DevId;               /* Device index */
    ONEWIRE_STATUS OwStatus;            /* One wire status */
    AM_DEVICE_INFO *pDeviceData;        /* Pointer to device data */
    AM_EVENT       Event;               /* Adapter Manager event */

    OwStatus = ONEWIRE_STATUS_ERROR;


    /* Run the device state machine */
    for (DevId = (AM_DEVICE)0; DevId < AM_DEVICE_COUNT; DevId++) /// \todo 01/20/2022 DAZ - Did start at adapter. Always start from beginning of enum
    {
        Event = AM_EVENT_COUNT;
        pDeviceData = &AdapMgrDevData.AmDeviceData[DevId];

        switch (pDeviceData->State)
        {
           case AM_DEVICE_STATE_NO_DEVICE:
               if (pDeviceData->Present)
               {
                    pDeviceData->State = AM_DEVICE_STATE_AUTHENTICATE;
               }
               break;

           case AM_DEVICE_STATE_AUTHENTICATE:
               if(pDeviceData->DeviceUnsupported)  //Device is Unsupported?
               {
                  pDeviceData->State = AM_DEVICE_STATE_INVALID;
                  Event = AM_EVENT_NEW_DEVICE;
               }
               else
               {
                  OwStatus = L3_OneWireAuthenticate(pDeviceData->DeviceUID);

                  if ((AM_DEVICE_ADAPTER == pDeviceData->Device) && (OwStatus == ONEWIRE_STATUS_ERROR))
                  {
                      FaultHandlerSetFault(ADAPTER_AUTH_FAIL, SET_ERROR);
                  }
                  // Clamshell
                  if ((AM_DEVICE_CLAMSHELL == pDeviceData->Device) && (OwStatus == ONEWIRE_STATUS_ERROR))
                  {
                      FaultHandlerSetFault(ERRSHELL_CLAMSHELL_AUTHFAIL,SET_ERROR);
                  }

                  if(ONEWIRE_STATUS_OK != OwStatus)
                  {
                      //clear the authentic boolean flag for all 1-wire device
                      pDeviceData->Authentic = false;

                      /* set onewire errors detected during device authentication */
                      SetOneWireFaultStatus(pDeviceData->Device, OwStatus);

                      Log(ERR, "AdapterManager: 1-w authenticate failed %d",OwStatus);

                      /* The HANDLE software shall log the Unauthenticated devices connection attempts to the SECURITY_LOG File. */
                      SecurityLog("Device Authentication failed - DeviceId:%llx",pDeviceData->DeviceUID);
                  }
                  else
                  {
                      //set the authentic boolean flag for all 1-wire device
                      pDeviceData->Authentic = true;

                      OwStatus = DeviceWriteTest(pDeviceData);
                      pDeviceData->DeviceWriteTest = ( ONEWIRE_STATUS_OK == OwStatus ) ? true : false;
                      if (!pDeviceData->DeviceWriteTest)
                      {
                          SetOneWireFaultStatus(pDeviceData->Device, OwStatus);
                      }
                  }

                  if ((AM_DEVICE_RELOAD != pDeviceData->Device) || (AM_DEVICE_CARTRIDGE != pDeviceData->Device))
                  {
                      pDeviceData->State = (ONEWIRE_STATUS_OK == OwStatus) ? AM_DEVICE_STATE_ACTIVE
                                                                           : AM_DEVICE_STATE_INVALID;
                  }
                  else
                  {
                      //update the device state for Reload/Cartridge
                      pDeviceData->State = AM_DEVICE_STATE_ACTIVE;
                  }

                  if (AM_DEVICE_HANDLE == pDeviceData->Device)
                  {
                      // Create System Log file after Handle is Authenticated
                      CreateSystemLogFile();
                  }
                  Event = AM_EVENT_NEW_DEVICE;
               }
                break;


            case AM_DEVICE_STATE_ACTIVE:
                /* Update the device state when not connected */
                if (!pDeviceData->Present)
                {
                    pDeviceData->State = AM_DEVICE_STATE_NO_DEVICE;
                    Event = AM_EVENT_LOST_DEVICE;
                }
                break;

            case AM_DEVICE_STATE_INVALID:
                if (!pDeviceData->Present)
                {
                  if(pDeviceData->DeviceUnsupported) //Device is Unsupported?
                   {
                      pDeviceData->DeviceUnsupported = false;
                   }
                   Event = AM_EVENT_LOST_DEVICE;
                   pDeviceData->State = AM_DEVICE_STATE_NO_DEVICE;
                }
                break;

            case AM_DEVICE_STATE_SHORT:
                /* Update the device state when shorted */
                if (!pDeviceData->Present)
                {
                    pDeviceData->State = AM_DEVICE_STATE_NO_DEVICE;
                    Event = AM_EVENT_LOST_DEVICE;
                }
                break;

            default:
                pDeviceData->State = AM_DEVICE_STATE_NO_DEVICE;
                break;
        }

        /* Notify user about device events  */
        if (Event < AM_EVENT_COUNT)
        {
            if (NULL != AdapMgrDevData.EventHandler)
            {
                AdapMgrDevData.EventHandler(Event, &pDeviceData->DeviceUID);
            }

            // Publish signal as required
            Signia_AdapterMgrEventPublish(Event, pDeviceData);

            /* Invalidate the device id after notifying user for events other than AM_EVENT_NEW_DEVICE */
            if (AM_EVENT_NEW_DEVICE != Event)
            {
                pDeviceData->DeviceUID = ONEWIRE_DEVICE_ID_INVALID;
            }
        }

        //  Do not process any other devices until handle has been successfully processed.
        if ((AM_DEVICE_HANDLE == DevId) && (AM_DEVICE_STATE_NO_DEVICE == pDeviceData->State))
        {
            break;
        }
    }
}


/* ========================================================================== */
/**
 * \brief   Device Write Test
 *
 * \details This function runs the One wire Write test for the detected Device.
 *
 * \param   *pDevData       - pointer to Device Data
 *
 * \return  ONEWIRE_STATUS  - status of the Test
 *
 * \todo 09/09/2021 BS - In case of Write test system to be moved to ST_ERR_PERM_FAIL_WOP.
 * ========================================================================== */
static ONEWIRE_STATUS DeviceWriteTest(AM_DEVICE_INFO *pDevData)
{
    static uint8_t   ReadData[OW_MEMORY_TOTAL_SIZE];   /* EEPROM read data */
    OW_EEP_STATUS    EepromStatus;                     /* EEPROM status */
    BasicOneWireMemoryLayout_Ver2 *pOneWireMemFormat;  /* Pointer to basic one wire memory format */
    uint8_t          writetest;                        /* temp variable to hold the write test value */
    ONEWIRE_STATUS   Status;

    Status = ONEWIRE_STATUS_OK;
    do
    {
        /* Read the EEPROM Data 64bytes*/
        EepromStatus = L3_OneWireEepromRead(pDevData->DeviceUID, OW_EEPROM_PAGE_NUM, ReadData);
        EepromStatus = L3_OneWireEepromRead(pDevData->DeviceUID, OW_EEPROM_PAGE_NUM2, &ReadData[OW_EEPROM_PAGE_OFFSET]);
        if (OW_EEP_STATUS_OK != EepromStatus)
        {
            Status = ONEWIRE_STATUS_READ_ERROR;
            Log(DBG, "Error in EEPROM Read");
            break;
        }

        pOneWireMemFormat = (BasicOneWireMemoryLayout_Ver2*)ReadData;

        /* Store the WriteTest value to local copy */
        writetest = pOneWireMemFormat->writeTest;

        /* Increment the WriteTest and write back the Data to EEPROM with corrected CRC */
        pOneWireMemFormat->writeTest++;
        pOneWireMemFormat->crc = CRC16(0,ReadData,(size_t)(OW_MEMORY_TOTAL_SIZE-2));
        EepromStatus = L3_OneWireEepromWrite(pDevData->DeviceUID, OW_EEPROM_PAGE_NUM2, &ReadData[OW_EEPROM_PAGE_OFFSET]);
        if (OW_EEP_STATUS_OK != EepromStatus)
        {
            Status = ONEWIRE_STATUS_WRITE_ERROR;
            Log(DBG, "Error in EEPROM Write");
            break;
        }

        pOneWireMemFormat->writeTest = 0;

        /* Read the EEPROM page with new WriteTest data */
        EepromStatus = L3_OneWireEepromRead(pDevData->DeviceUID, OW_EEPROM_PAGE_NUM2, &ReadData[OW_EEPROM_PAGE_OFFSET]);
        if (OW_EEP_STATUS_OK != EepromStatus)
        {
            Status = ONEWIRE_STATUS_READ_ERROR;
            Log(DBG, "Error in EEPROM Read");
            break;
        }

        pOneWireMemFormat = (BasicOneWireMemoryLayout_Ver2*)ReadData;

        /* Test Hook to fail the NVM Test */
        TM_Hook(HOOK_ONEWIRENVMTEST, pOneWireMemFormat);

        /* Compare the Read value with the incremented local copy both should match */
        if((++writetest) != pOneWireMemFormat->writeTest)
        {
            Log(DBG, "Error in EEPROM Test, Device type: %d  (Read value: %d, Expected value: %d)", pDevData->DeviceType, pOneWireMemFormat->writeTest, writetest);
            Status = ONEWIRE_STATUS_NVMTEST_ERROR;
            break;
        }
        Log(DBG, "EEPROM Test successful, Device type: %d, ID: 0x%16llX", pDevData->DeviceType, pDevData->DeviceUID);
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Adapter Manager Task.
 *
 * \details The task executes every 100 msec to run the device state machine and
 *          Adapter Manager state machine.
 *
 * \param   p_arg - Task arguments
 *
 * \return  None
 *
 * ========================================================================== */
static void AdapterManagerTask(void *p_arg)
{
    AM_OW_MSG *pRequest;     /* One wire message */
    uint8_t   Error;         /* Error status */

    while (true)
    {
        /* Wait for device connection/disconnection events */
        do
        {
            /* Wait on Q for new request */
            pRequest = (AM_OW_MSG*) OSQPend(pAdapMgrQ, MSG_Q_TIMEOUT_TICKS, &Error);
            if (OS_ERR_NONE != Error)
            {
                /* Proceed the task after timeout */
                if (OS_ERR_TIMEOUT != Error)
                {
                    Log(ERR, "AadpterManager: Q Error on wait for new request");
                    /* /// \todo 03/11/2021 PR Add exception handler here */
                    break;
                }
            }

            if (NULL == pRequest)
            {
                break;
            }
            /* Update device connection status */
            UpdateDeviceConnStatus(pRequest->Device, pRequest->Event);

        } while ( false );

        /* Run the device state machine and module state machine */
        ConnectionProcessor();
        AdapterMngrStateMachine();
    }
}

/* ========================================================================== */
/**
 * \brief   Update connected device details
 *
 * \details This function is called to update the Adapter Manager device data
 *          structure when a connection is detected.
 *
 * \param   DeviceUniqueAddr - 64 bit device ID
 * \param   DeviceID    - Signia Device ID (2 byte ID read from EEPROM)
 *
 * \return  None
 *
 * ========================================================================== */
static void UpdateDeviceConnection(DEVICE_UNIQUE_ID DeviceUniqueAddr, uint16_t DeviceID)
{
    AM_DEVICE_INFO  *pDeviceData;           // Pointer to device data
    AM_DEVICE DeviceClass;                  // Class of device
    DEVICE_INSTANCE_MISC DeviceInstance;    // Signia Device instance. Need to cast for specifc device type.
    DEVICE_TYPE DeviceType;                 // Signia Device Type
    uint8_t DataVer;                        // Clamshell Data Version to know supported or unsupported Clamshell
    DeviceClass = AM_DEVICE_COUNT;
    DeviceType = (DEVICE_TYPE)ONEWIRE_ID_TYPE(DeviceID);                    // Extract device type & instance from ID
    DeviceInstance = (DEVICE_INSTANCE_MISC)ONEWIRE_INSTANCE(DeviceID);

    switch (DeviceType)
    {
        case DEVICE_TYPE_ADAPTER:

            AdapterSetDeviceId(DeviceUniqueAddr, &ReadData[0]);
            /// \todo 01/13/2022 DAZ - The following SHOULD be done in App to keep device dependence out of platform:

            if (DEVICE_ADAPTER_COUNT <= (DEVICE_INSTANCE_ADAPTER)DeviceInstance)
            {
                FaultHandlerSetFault(UNKNOWN_ADAPTER_DETECTED, SET_ERROR);
            }
            else if (DEVICE_ADAPTER_EGIA != (DEVICE_INSTANCE_ADAPTER)DeviceInstance)// To do: JA 19-Nov-2021 update for other supported types of adapter
            {
                Log(REQ, "UNSUPPORTED ADAPTER Connected: Serial Number = %llx",DeviceUniqueAddr);
                DeviceType = DEVICE_TYPE_UNKNOWN1;
                DeviceClass = AM_DEVICE_ADAPTER;
                pDeviceData = &AdapMgrDevData.AmDeviceData[DeviceClass];
                pDeviceData->DeviceUnsupported = true;
            }
            else
            {
                Log(REQ, "Adapter Connected: Serial Number = 0x%16llX",DeviceUniqueAddr);
                DeviceClass = AM_DEVICE_ADAPTER;
                AdapterDataFlashInitialize();
                L4_AdapterUartComms(true);
            }
            break;

        case DEVICE_TYPE_MISC:
            do
            {
                if (DEVICE_BATTERY == DeviceInstance)
                {
                    DeviceClass = AM_DEVICE_BATTERY;
                    BatterySetDeviceId(DeviceUniqueAddr, &ReadData[0]);
                    Log(REQ, "Battery Connected: Serial Number = 0x%16llX",DeviceUniqueAddr);
                    break;
                }
                if (DEVICE_CLAMSHELL == DeviceInstance)
                {
                    DeviceClass = AM_DEVICE_CLAMSHELL;
                    Log(REQ, "Clamshell Connected: Serial Number = 0x%16llX",DeviceUniqueAddr);
                    ClamshellSetDeviceId(DeviceUniqueAddr, &ReadData[0]);
                    /* Read the Data Version of Clamshell OW ID */
                    OW_READ(ClamshellInterface, DataVersion, DataVer);
                    if ( CLAMSHELL_DATA_VERSION != DataVer )
                    {
                        /* Publish Clamshell Unsupported Error Signal to App */
                        Log(ERR, "AdapterManager: Clamshell Unsupported - Data Version Mismatch");
                        FaultHandlerSetFault(ERRSHELL_UNSUPPORTED_CLAMSHELL,SET_ERROR);
                        break;
                    }
                    else
                    {
                        ;
                    }
                    break;
                }
                if (DEVICE_HANDLE == DeviceInstance)
                {
                    DeviceClass = AM_DEVICE_HANDLE;
                    HandleSetDeviceId(DeviceUniqueAddr, &ReadData[0]);
                    Log(REQ, "Handle Connected: Serial Number = 0x%16llX",DeviceUniqueAddr);
                    CheckHandleStartupErrors();
                    ConfigureOneWireBus( ONEWIRE_BUS_CLAMSHELL );//look for clamshell after handle is detected
                    ConfigureOneWireBus( ONEWIRE_BUS_CONNECTORS );
                    break;
                }
            } while (false);
            break;

        case DEVICE_TYPE_EGIA_SULU:
        /* Fall through is intentional because all are Reload types */
        case DEVICE_TYPE_EGIA_MULU:
        case DEVICE_TYPE_EGIA_RADIAL:
        case DEVICE_TYPE_EEA_RELOAD:
            DeviceClass = AM_DEVICE_RELOAD;
            Log(REQ, "Reload Connected: Serial Number = 0x%16llX",DeviceUniqueAddr);
            ReloadSetDeviceId(DeviceUniqueAddr, &ReadData[0]);
            break;

        case DEVICE_TYPE_EGIA_CART:
            DeviceClass = AM_DEVICE_CARTRIDGE;
            Log(REQ, "Cartridge Connected: Serial Number = %llx",DeviceUniqueAddr);
            CartridgeSetDeviceId(DeviceUniqueAddr, &ReadData[0]);
            break;

        default:
            DeviceClass = AM_DEVICE_COUNT;
            break;
    }

    // Register device changes
    if (DeviceClass < AM_DEVICE_COUNT)
    {
        pDeviceData = &AdapMgrDevData.AmDeviceData[DeviceClass];
        pDeviceData->DeviceUID = DeviceUniqueAddr;
        pDeviceData->Device = DeviceClass;      /// \todo 01/25/2022 DAZ - Structure member should be called DeviceClass to avoid confusion
        pDeviceData->DeviceType = DeviceType;
        pDeviceData->Present = true;
    }
}

/* ========================================================================== */
/**
 * \brief   Update device connection status
 *
 * \details This function is called to update the devices connection
 *          status and update Device informaton structure.
 *
 * \note    Since 'BasicOneWireMemoryLayout_Ver2' is a packed structure,
 *          memcpy is used to copy the structure members.
 *
 * \param   Device - 64 bit Device ID
 * \param   Event  - Device event
 *
 * \return  None
 *
 * ========================================================================== */
static void UpdateDeviceConnStatus(DEVICE_UNIQUE_ID Device, ONEWIRE_EVENT Event)
{
    AM_STATUS        EepromStatus;                     /* EEPROM status */
    uint16_t         OneWireID;
    uint16_t         DevIndex;                         /* Device Index */
    AM_DEVICE_INFO   *pDeviceData;                     /* Pointer to the device info */
    BasicOneWireMemoryLayout_Ver2 *pOneWireMemFormat;  /* Pointer to basic one wire memory format */
    EepromStatus = AM_STATUS_ERROR;

    switch (Event)
    {
        case ONEWIRE_EVENT_NEW_DEVICE:
            EepromStatus = GenericEepRead(Device, ReadData);

            /* Read the One wire device type from the EEPROM memory format */
            pOneWireMemFormat = (BasicOneWireMemoryLayout_Ver2*)ReadData;
            /// \todo 01/21/2022 DAZ - Misnomer. This is not OneWireID - It's really Signia Device ID
            memcpy(&OneWireID, &(pOneWireMemFormat->oneWireID), sizeof(uint16_t));
            UpdateDeviceConnection(Device, OneWireID);

            pDeviceData = &AdapMgrDevData.AmDeviceData[AM_DEVICE_ADAPTER];

            if ((AM_STATUS_CRC_FAIL == EepromStatus) || (AM_STATUS_DATA_CRC_FAIL == EepromStatus))
            {
                if(AM_DEVICE_ADAPTER == pDeviceData->Device)
                {
                   pDeviceData->DeviceCrcFail = true;
                    /* Set the Fault for Adapter CRC Fail*/
                   FaultHandlerSetFault(ADAPTER_CRC_FAIL, SET_ERROR);
                }
                Log(ERR, "EEPROM Read CRC Fail");
                /* Log the CRC Fail devices connection */
                Log(REQ, "Device CRC Fail: Serial Number = 0x%16llX",Device);
                break;
            }
            break;

        case ONEWIRE_EVENT_LOST_DEVICE:

            for (DevIndex = 0; DevIndex < AM_DEVICE_COUNT; DevIndex++)
            {
               pDeviceData = &AdapMgrDevData.AmDeviceData[DevIndex];
               if (pDeviceData->DeviceUID == Device)
               {
                   if (DevIndex == AM_DEVICE_ADAPTER)
                   {
                        L4_AdapterUartComms(false);
                        AdapterDataFlashInitialize();
                   }
                   /* Check for the device and update */
                   pDeviceData->Present = false;
                   break;
               }
            }
            break;

        case ONEWIRE_EVENT_BUS_SHORT:
        /* Intentional fall through to process all Bus faults */
        case ONEWIRE_EVENT_BUS_ERROR:
            /* Device set to NO_DEVICE_ONBUS only for no device on Clamshell and Connectore BUS */
            if (Device == NO_DEVICE_ONBUS)
            {
                if (AM_DEVICE_STATE_ACTIVE == AdapMgrDevData.AmDeviceData[AM_DEVICE_CLAMSHELL].State)
                {
                    if (( AM_DEVICE_STATE_ACTIVE != AdapMgrDevData.AmDeviceData[AM_DEVICE_ADAPTER].State) &&
                      (AM_DEVICE_STATE_ACTIVE == AdapMgrDevData.AmDeviceData[AM_DEVICE_HANDLE].State))
                    {
                        FaultHandlerSetFault(ADAPTER_ONEWIRE_SHORT,SET_ERROR);
                    }
                }
                else /* No device on clamshell or Connector Bus */
                {
                    FaultHandlerSetFault(ONEWIRE_SHORT_NO_DEVICE,SET_ERROR);
                }
            }//Device present on Clamshell/Connector Bus and short notified
            else
            {
                for (DevIndex = 0; DevIndex < AM_DEVICE_COUNT; DevIndex++)
                {
                    pDeviceData = &AdapMgrDevData.AmDeviceData[DevIndex];

                    if ((pDeviceData->DeviceUID == Device) && (AM_DEVICE_STATE_ACTIVE == pDeviceData->State)
                          && (CHRG_MNGR_STATE_DISCONNECTED == Signia_ChargerManagerGetState()))
                    {
                        //Not on Charger/Device ID not matching
                        if (AM_DEVICE_CLAMSHELL == DevIndex)
                        {
                            FaultHandlerSetFault(ERR_CLAMSHELL_ONEWIRE_SHORT,SET_ERROR);
                            pDeviceData->State = AM_DEVICE_STATE_SHORT;
                            break;
                        }
                        if ((AM_DEVICE_HANDLE == DevIndex) || (AM_DEVICE_BATTERY == DevIndex))
                        {
                            FaultHandlerSetFault(ERR_PERMANENT_FAIL_ONEWIRE_SHORT,SET_ERROR);
                            pDeviceData->State = AM_DEVICE_STATE_SHORT;
                            break;
                        }
                        if (AM_DEVICE_ADAPTER == DevIndex)
                        {
                            FaultHandlerSetFault(ADAPTER_ONEWIRE_SHORT,SET_ERROR);
                            break;
                        }
                        if ((AM_DEVICE_RELOAD == DevIndex) || (AM_DEVICE_CARTRIDGE == DevIndex))
                        {
                            break;
                        }
                    }
                }
           }
           Log(ERR, "Detected Bus Error Event on Device 0x%16llx", Device);
           break;

        default:
            /* Other events are ignored */
            break;
    }
}

/* ========================================================================== */
/**
 * \brief   Gets the next available memory slot
 *
 * \details  Function to check if the request queue has a place to hold the request
 *
 * \param   < None >
 *
 * \return  AM_OW_MSG - Ptr to the next slot of memory, otherwise NULL
 *
 * ========================================================================== */
static AM_OW_MSG *GetNextAmReqMsgSlot(void)
{
    uint8_t          OsError;                           /* OS Error Variable */
    static uint8_t   MsgReqPoolIndex;                   /* Message Req Pool Index */
    static AM_OW_MSG MsgReqPool[MAX_AM_REQUESTS];       /* Message Req Pool */
    AM_OW_MSG        *pRequest;                         /* Pointer to the next message */

    /* Mutex lock  */
    OSMutexPend(pAdapterMgrMutex, OS_WAIT_FOREVER, &OsError);
    if(OS_ERR_NONE != OsError)
    {
        Log(ERR, "GetNextAmReqMsgSlot: OSMutexPend error");
        /* /// \todo 03/15/2021 PR Add exception handler here */
    }

    /* Is the current index pointing to max storage? */
    if (++MsgReqPoolIndex >= MAX_AM_REQUESTS)
    {
        /* Circle back to first */
        MsgReqPoolIndex = 0;
    }

    /* Get the slot based on the Index */
    pRequest = &MsgReqPool[MsgReqPoolIndex];

    /* Release the Mutex */
    OSMutexPost(pAdapterMgrMutex);

    return pRequest;
}

/* ========================================================================== */
/**
 * \brief   One wire event handler
 *
 * \details Handle the one wire events and post the message to the Adapter
 *          manager task for the state machine to process the events
 *
 * \param   OwEvent    - One wire event
 * \param   OwDevice   - One wire device id
 *
 * \return  None
 *
 * ========================================================================== */
static void AmOnewireEventHandler(ONEWIRE_EVENT OwEvent, DEVICE_UNIQUE_ID OwDevice)
{
    uint8_t       Error;        /* Error status */
    AM_OW_MSG    *pRequest;     /* Pointer to the Adapter manager message */

    /* Check if we have slots on Q */
    pRequest = GetNextAmReqMsgSlot();
    if (NULL != pRequest)
    {
        pRequest->Event = OwEvent;
        pRequest->Device = OwDevice;

        Error = OSQPost(pAdapMgrQ, pRequest);
        if (OS_ERR_Q_FULL == Error)
        {
            Log(ERR, "AdapterManager: Message Queue is Full");
            /* /// \todo 03/11/2021 PR Add exception handler here */
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Initialize one wire bus
 *
 * \details Configure the one wire options and register for one wire events
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Adapter Manager Status
 * \retval  AM_STATUS_OK     - Successful in initializing one wire bus
 * \retval  AM_STATUS_ERROR  - Error in initializing one wire bus
 *
 * ========================================================================== */
static AM_STATUS ConfigureOneWireBus(ONEWIRE_BUS Bus)
{
    uint8_t           Index;               /* Device index */
    ONEWIREOPTIONS    Options;             /* One wire config options */
    ONEWIRE_STATUS    OwStatus;            /* One wire status */
    AM_STATUS         AmStatus;            /* Adapter manager Status */

    OwStatus = ONEWIRE_STATUS_ERROR;
    AmStatus = AM_STATUS_ERROR;

    do
    {
        /* Configure and register for One wire events */
         Options.DeviceCount = OW_DEVICE_COUNT;
         Options.KeepAlive = OW_KEEP_ALIVE_INTERVAL;
         Options.pHandler = &AmOnewireEventHandler;
         Options.ScanInterval  = OW_SCAN_INTERVAL;
         Options.Speed = ONEWIRE_SPEED_OD;

         for (Index = 0; Index < ONEWIRE_MAX_DEVICE_FAMILY; Index++)
         {
             Options.Family[Index] = ONEWIRE_DEVICE_FAMILY_EEPROM;
         }


        Options.Speed = ONEWIRE_SPEED_OD;
        Options.Bus = Bus;
        OwStatus = L3_OneWireBusConfig(&Options);
        if (ONEWIRE_STATUS_OK != OwStatus)
        {
            break;
        }
        AmStatus = AM_STATUS_OK;

    } while ( false );

    return AmStatus;
}

/* ========================================================================== */
/**
 * \brief   Set the Fault based on the one wire status received
 *
 * \details This function sets the Fault based on the error received from the onewire
 *          interface. Appropriate error status is set based on the Device and the cause of the
 *          Error.
 *
 * \param   DevId    - Onewire Device Types - Handle, Clamshell, Reload etc.
 * \param   OwStatus - OneWire status returned by the OneWire interface
 *
 * \return  None
 *
 * ========================================================================== */
static void SetOneWireFaultStatus(AM_DEVICE DevId, ONEWIRE_STATUS OwStatus)
{
    uint8_t ErrorIndex;

    /* Map onewire status to Error and Error causes defined in FaultHandler */
    static const FAULT_HANDLER_OWSTATUS BatteryStartupErrors[MAX_OWSTARTUP_ERRORS] =
    {
        /* ONEWIRE_STATUS_READ/WRITE_ERROR is set when read/write fails during Handle authentication*/
        {ONEWIRE_STATUS_WRITE_ERROR,    REQRST_BATTONEWIRE_WRITEFAIL  },
        {ONEWIRE_STATUS_READ_ERROR ,    REQRST_BATTONEWIRE_READFAIL   },
        /* ONEWIRE_STATUS_ERROR is set when Authentication fails*/
        {ONEWIRE_STATUS_ERROR      ,    PERMFAIL_BATT_ONEWIRE_AUTHFAIL},
        /* ONEWIRE_STATUS_BUS_ERROR is set when there is a short identified on onewire bus*/
        {ONEWIRE_STATUS_BUS_ERROR  ,    PERMFAIL_BATT_ONEWIRE_SHORT   },
        /* ONEWIRE_STATUS_NVMTEST_ERROR is set when NVM Test failed */
        {ONEWIRE_STATUS_NVMTEST_ERROR , ONEWIRE_NVM_TESTFAIL          }
    };

    switch(DevId)
    {
        /*Fault handling for Battery onewire*/
        case AM_DEVICE_BATTERY:
            for(ErrorIndex=0;ErrorIndex<MAX_OWSTARTUP_ERRORS;ErrorIndex++)
            {
                if(OwStatus == BatteryStartupErrors[ErrorIndex].OwStatus)
                {
                    FaultHandlerSetFault(BatteryStartupErrors[ErrorIndex].ErrorCause, SET_ERROR);
                    break;
                }
            }
            break;

        /*Fault handling for Handle onewire*/
        case AM_DEVICE_HANDLE:
            for(ErrorIndex=0;ErrorIndex<MAX_OWSTARTUP_ERRORS;ErrorIndex++)
            {
                if(OwStatus == HandleStartupErrors[ErrorIndex].OwStatus)
                {
                    FaultHandlerSetFault(HandleStartupErrors[ErrorIndex].ErrorCause, SET_ERROR);
                    break;
                }
            }
            break;

        case AM_DEVICE_CLAMSHELL:
        case AM_DEVICE_RELOAD:
        case AM_DEVICE_CARTRIDGE:
            if ( ONEWIRE_STATUS_NVMTEST_ERROR == OwStatus )
            {
                FaultHandlerSetFault( ONEWIRE_NVM_TESTFAIL , SET_ERROR );
            }
            break;

        default:
            break;
    }
}

/* ========================================================================== */
/**
 * \brief   EEPROM read on New device
 *
 * \details Read 1-wire EEPROM memory
 *
 * \param   Device - 1-Wire device address
 * \param   *pData - Data pointer to 1-Wire Eep Data
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in reading EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in reading EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS GenericEepRead(ONEWIRE_DEVICE_ID Device, uint8_t *pData)
{
    AM_STATUS Status;         /* Function status */
    OW_EEP_STATUS OwEepStatus;
    uint16_t CalcCRC;
    uint16_t TempCrc;


    Status = AM_STATUS_OK;
    CalcCRC = 0;
    TempCrc = 0;

    OwEepStatus  = L3_OneWireEepromRead(Device, 0, pData);
    OwEepStatus |= L3_OneWireEepromRead(Device, 1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);
    do
    {
        if (OW_EEP_STATUS_OK != OwEepStatus)
        {
            Log(ERR, "1-Wire EEPROM Data Read Error: %d", OwEepStatus);
            Status = AM_STATUS_CRC_FAIL;
            break;
        }

        /* Check for Data integrity */
        CalcCRC = CRC16(CalcCRC, pData, ONEWIRE_MEMORY_DATA_SIZE);
        memcpy(&TempCrc, &pData[ONEWIRE_MEMORY_DATA_SIZE], ONEWIRE_MEMORY_DATA_CRC_SIZE);

        if (TempCrc != CalcCRC)
        {
            Log(ERR, "EEPRead: EEPROM CRC validation failed");
            Status = AM_STATUS_DATA_CRC_FAIL;
        }

        Log(DBG, "Device ID 0x%016llX  Memory =", Device);
        Log(DBG, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", pData[ 0], pData[ 1], pData[ 2], pData[ 3], pData[ 4], pData[ 5], pData[ 6], pData[ 7], pData[ 8], pData[ 9]);
        Log(DBG, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", pData[10], pData[11], pData[12], pData[13], pData[14], pData[15], pData[16], pData[17], pData[18], pData[19]);
        Log(DBG, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", pData[20], pData[21], pData[22], pData[23], pData[24], pData[25], pData[26], pData[27], pData[28], pData[29]);
        Log(DBG, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", pData[30], pData[31], pData[32], pData[33], pData[34], pData[35], pData[36], pData[37], pData[38], pData[39]);
        Log(DBG, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", pData[40], pData[41], pData[42], pData[43], pData[44], pData[45], pData[46], pData[47], pData[48], pData[49]);
        Log(DBG, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", pData[50], pData[51], pData[52], pData[53], pData[54], pData[55], pData[56], pData[57], pData[58], pData[59]);
        Log(DBG, "%02X %02X %02X %02X",                               pData[60], pData[61], pData[62], pData[63]);

    } while (false);

    return Status;
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Initialization of Adapter Manager
 *
 * \details Initialize Adapter Manager module and internal structures
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Adapter Manager Status
 * \retval  AM_STATUS_OK     - No error in initilalizing
 * \retval  AM_STATUS_ERROR  - Error in initializing
 *
 * ========================================================================== */
AM_STATUS L4_AdapterManagerInit(void)
{
    AM_STATUS          Status;                                              /* Adapter Manager Status */
    uint8_t            OsError;                                             /* OS Error Variable */
    static bool        AdapterMngrInitDone  = false;                        /* To avoid multiple initialization */
    static void*       pAmQStorage[MAX_AM_REQUESTS];                        /* Request Q storage */
    AM_DEVICE          DevId;                                               /* Device index */
    AM_DEVICE_INFO     *pDeviceData;                                        /* Pointer to device data */

    Status = AM_STATUS_ERROR;

    do
    {   /* Protect against multiple init calls */
        if (AdapterMngrInitDone)
        {
            Status = AM_STATUS_OK;
            break;
        }

        /* Initialize request message Q */
        pAdapMgrQ = SigQueueCreate(pAmQStorage, MAX_AM_REQUESTS);

        if (NULL == pAdapMgrQ)
        {
            /* Something went wrong with message Q creation, mark error flag */
            /*\todo: Add exception handler */
            Log(ERR, "Adapter Manager: Message Q Creation Error");
            break;
        }

        /* Create an OS Mutex for Adapter Manager */
        pAdapterMgrMutex = SigMutexCreate("L4-AdapterMgr-Mutex", &OsError);
        if ((NULL == pAdapterMgrMutex) || (OsError != OS_ERR_NONE))
        {
            /* Failed to create Mutex */
            /* \todo: Add exception handler */
            Log(ERR, "AdapterMgr: Init Falied, Mutex Create Error - %d", OsError);
            break;
        }

        /* Create the Adapter Manager task */
        OsError = SigTaskCreate(&AdapterManagerTask,
                                NULL,
                                &AdapterMngrTaskStack[0],
                                TASK_PRIORITY_L4_ADAPTER_MANAGER,
                                ADAPTER_MNGR_TASK_STACK_SIZE,
                                "AdapterMgr");

        if (OsError != OS_ERR_NONE)
        {
            /* Couldn't create task, exit with error */
            Log(ERR, "AdapterManager: Init Falied, Task Create Error - %d", OsError);
            break;
        }

        Status = ConfigureOneWireBus( ONEWIRE_BUS_LOCAL );
        if ( AM_STATUS_OK !=  Status )
        {
            break;
        }

        AdapMgrDevData.AmState = AM_STATE_DISARMED;

        for (DevId = (AM_DEVICE)0; DevId < AM_DEVICE_COUNT; DevId++)
        {
            pDeviceData = &AdapMgrDevData.AmDeviceData[DevId];

            pDeviceData-> pDevHandle = DeviceHandler[DevId];
            pDeviceData->DeviceUID = ONEWIRE_DEVICE_ID_INVALID;
            pDeviceData->State = AM_DEVICE_STATE_NO_DEVICE;
            pDeviceData->Writable = false;
            pDeviceData->Present = false;
        }

        if (UART_STATUS_OK != L2_UartInit(ADAPTER_UART, ADAPTER_BAUD_RATE))
        {
            break;
        }

        if (GPIO_STATUS_OK != L3_GpioCtrlClearSignal(GPIO_EN_5V))
        {
            break;
        }

        if (AM_STATUS_OK != AdapterDefnInit())
        {
            break;
        }

        /* Set the init Flag to true. */
        AdapterMngrInitDone = true;

        Status = AM_STATUS_OK;

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Register an Adapter Manager Handle
 *
 * \details Register for call back from Adapter manager
 *
 * \param   pHandler  -  Handle to register
 *
 * \return  AM_STATUS - Adapter Manager Status
 * \retval  AM_STATUS_OK        - No error in registering
 * \retval  AM_STATUS_ERROR     - Error in registering
 *
 * ========================================================================== */
AM_STATUS Signia_AdapterManagerRegisterHandler(AM_HANDLER pHandler)
{
   AM_STATUS AmStatus;     /* Adapter manager Status */

   AmStatus = AM_STATUS_ERROR;

   if (NULL != pHandler)
   {
       AdapMgrDevData.EventHandler = pHandler;
       AmStatus = AM_STATUS_OK;
   }

   return AmStatus;
}

/* ========================================================================== */
/**
 * \brief   Get the Adapter Manager state
 *
 * \details This functions returns the current state of Adapter Manager
 *
 * \param   < None >
 *
 * \return  AM_STATE - Adapter Manager state
 *
 * ========================================================================== */
AM_STATE Signia_AdapterManagerGetState(void)
{
    return AdapMgrDevData.AmState;
}

/* ========================================================================== */
/**
 * \brief   Get the Adapter Manager device handle
 *
 * \details This functions returns the handle corresponding to the device
 *
 * \param   Device    - Adapter manager device
 *
 * \return  void* - device handle
 *
 * ========================================================================== */
void* Signia_AdapterManagerDeviceHandle(AM_DEVICE Device)
{
    void *pHandle;

        switch(Device)
        {
            case AM_DEVICE_HANDLE:
                pHandle = DeviceHandler[AM_DEVICE_HANDLE];
                break;

            case AM_DEVICE_ADAPTER:
                pHandle = DeviceHandler[AM_DEVICE_ADAPTER];
                break;

            case AM_DEVICE_CLAMSHELL:
                pHandle = DeviceHandler[AM_DEVICE_CLAMSHELL];
                break;

            case AM_DEVICE_RELOAD:
                pHandle = DeviceHandler[AM_DEVICE_RELOAD];
                break;

            case AM_DEVICE_CARTRIDGE:
                pHandle = DeviceHandler[AM_DEVICE_CARTRIDGE];
                break;
            default:
                /* Should never reach */;
                pHandle = NULL;
                break;
        }

    return pHandle;
}

/* ========================================================================== */
/**
 * \brief   Get the Adapter Manager device information
 *
 * \details This functions returns the information corrosponding to the device
 *
 * \param   Device    - Adapter manager device
 * \param   pInfo     - Pointer to the Device information
 *
 * \return  AM_STATUS        - Function status
 * \retval  AM_STATUS_OK     - Successful in returning the device info
 * \retval  AM_STATUS_ERROR  - Error in returning the device info
 *
 * ========================================================================== */
AM_STATUS Signia_AdapterManagerGetInfo(AM_DEVICE Device, AM_DEVICE_INFO *pInfo)
{
    AM_STATUS       Status;        /* Function status */
    AM_DEVICE_INFO  *pDeviceData;  /* Pointer to Device data */

    Status = AM_STATUS_ERROR;

    if (Device < AM_DEVICE_COUNT)
    {
        pDeviceData = &AdapMgrDevData.AmDeviceData[Device];
        memcpy(pInfo, pDeviceData, sizeof(AM_DEVICE_INFO));
        Status = AM_STATUS_OK;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Get the Reload Connected Status
 *
 * \details This functions returns the Reload Connected or not
 *
 * \param   < None >
 *
 * \return  bool - reload connection status
 * \retval  True  -  Reload Connected
 * \retval  False -  Reload Not Connected
 * ========================================================================== */
bool Signia_IsReloadConnected(void)
{
    bool Connected;
    Connected = (AM_DEVICE_STATE_ACTIVE == AdapMgrDevData.AmDeviceData[AM_DEVICE_RELOAD].State) ? true : false;
    return Connected;
}

/* ========================================================================== */
/**
 * \brief   Get Adapter Uart Comms
 *
 * \details Hijack the Adapter UART communication for reading the adapter type
 *          in case of 1-wire failure
 *
 * \param   state
 *          true:  Enable adapter uart comms
 *          false:  Disable adapter uart comms
 *
 * \return  none
 * ========================================================================== */

void L4_AdapterUartComms(bool state)
{
    if (state)
    {
        pAdapterComm = L4_CommManagerConnOpen(COMM_CONN_UART0, &ProcessAdapterUartResponse);
    }
    else
    {
        L4_CommManagerConnClose(pAdapterComm);
    }
    L4_AdapterComSMReset();
}

/* ========================================================================== */
/**
 * \brief   Get the attached Adapter type
 *
 * \details Get the attached Adapter type
 *
 *
 * \param   < None >
 *
 * \return  true   = Unsupported Device
 *          false  = Good Adapter
 * ========================================================================== */
bool Signia_GetAdapterStatus(void)
{
    AM_DEVICE_INFO *pDeviceData;

    pDeviceData = &AdapMgrDevData.AmDeviceData[DEVICE_TYPE_ADAPTER];
    return pDeviceData->DeviceUnsupported;
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

