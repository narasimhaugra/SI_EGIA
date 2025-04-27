#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_OneWire
 * \{
 *
 * \brief   One Wire Bus primitives
 *
 * \details This module supplies functions necessary for manipulating devices on
 *          the One Wire bus(es). Functions are provided to achieve the following
 *          necessary One Wire bus operations:
 *              - Initialize interface hardware
 *              - Select One Wire Bus to operate on
 *              - Scan selected bus to detect all connected devices
 *              - Select a specified device on a specified bus
 *              - Transfer data to/from selected device
 *              - Provide device list from a bus/family
  *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_OneWireController.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_GpioCtrl.h"
#include "L3_OneWireTransport.h"
#include "L3_OneWireController.h"
#include "L3_OneWireAuthenticate.h"
#include "L3_I2c.h"
#include "TestManager.h"

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER            (LOG_GROUP_1W)

#define MAX_OW_REQUESTS                 (10u)                ///< Maximum 1-Wire requests that can be queued
#define ONEWIRE_TASK_STACK              (1024u)               ///< Task stack size
#define OW_TIMER_MS                     (100u)               ///< Timer in ms
#define OW_TIMER_TICKS                  (OW_TIMER_MS/(1000u/OS_TMR_CFG_TICKS_PER_SEC))    ///< Equivalent ticks

#define OW_DEFAULT_SCAN_TIME            (0u)              ///< Default bus scan duration
#define OW_DEFAULT_CHECK_TIME           (1000u)             ///< Default device check duration
#define OW_DEFAULT_DEVICE_COUNT_ON_BUS  (2u)                 ///< Default number of expected device on a bus
#define OW_CHECK_DURATION               (200u)               ///< Device check duration
#define ONEWIRE_BUS_INVALID             ((ONEWIRE_BUS)(0xFFu))              ///< Invalid bus
#define OW_YIELD_WHEN_DEAD              (1000u)              ///< Delay used to yield during critical errors

#define ONEWIRE_MEMORY_TOTAL_SIZE    (64u)                                                      ///< Memory bank maximum size to store 1-wire data
#define MAX_SHORT_COUNT              (3u)
#define NO_DEVICE_ONBUS              (0xFF)
#define NOTIFY_HALT                  (0xFF)  
#define UNKNOWN_DEVICE               (0u)  
/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
OS_STK OWTaskStack[ONEWIRE_TASK_STACK+MEMORY_FENCE_SIZE_DWORDS];          		///< 1-Wire task stack
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
/*! \enum OW_PROC_STATE
 *   Request processor state-machine State.
 */
typedef enum
{
    OW_PROC_STATE_IDLE,         ///< The processor is not executing any requests
    OW_PROC_STATE_REQUEST,      ///< External Request
    OW_PROC_STATE_NOTIFY,       ///< Compose response on transfer complete or timeout
    OW_PROC_STATE_SCAN,         ///< Performing scan
    OW_PROC_STATE_CHECK_ALL,    ///< Checking all device connectivity
    OW_PROC_STATE_FAULT,        ///< Request processor encountered irrecoverable error
    OW_PROC_STATE_DISABLED,     ///< 1-Wire controller disabled, request processing paused
    OW_PROC_STATE_LAST          ///< State range indicator
} OW_PROC_STATE;

/*! \enum OW_REQUEST_TYPE
 *   1-Wire Request type.
 */
typedef enum
{
    OW_REQUEST_TRANSFER,        ///< Data Transfer
    OW_REQUEST_CHECK,           ///< Device connection check
    OW_REQUEST_LISTBY_FAMILY,   ///< Device list by family ID
    OW_REQUEST_LISTBY_BUS,      ///< Device list by Bus ID
    OW_REQUEST_TIMER,           ///< 1-Wire Controller processor internal timer request
    OW_REQUEST_CONFIG,          ///< Configuration request
    OW_REQUEST_AUTHENTICATE,    ///< Authentication request from Slave
    OW_REQUEST_COUNT
} OW_REQUEST_TYPE;

/*! \struct OWREQUEST
 *   1-Wire request structure.
 */
typedef struct
{
    OW_REQUEST_TYPE ReqType;    ///< Request type
    void *pMessage;             ///< 1-Wire request message
    OS_EVENT *pSema;            ///< Semaphore to pend during the request processing
    ONEWIRE_STATUS Status;      ///< Request process status
} OWREQUEST;

/*! \struct OWBUSSIGNALPAIR
 *   Structure to map 1-Wire Bus with corresponding GPIO signal.
 */
typedef struct
{
    ONEWIRE_BUS Bus;            ///< Bus ID
    GPIO_SIGNAL Signal;         ///< Associated GPIO signal to control the bus
} OWBUSSIGNALPAIR;

/*! \struct OWDEVICEQUERYMSG
 *   Device query message type.
 */
typedef struct
{
    OWDEVICEINFO Info;      ///< Device search criteria - either by family or bus. device ID ignored
    uint8_t Count;               ///< Device Count
    ONEWIRE_DEVICE_ID *pList;    ///< Device list
} OWDEVICEQUERYMSG;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static ONEWIREOPTIONS OwBusGroup[ONEWIRE_BUS_COUNT];    ///< Bus object list
static uint32_t OwBusScanDueTime[ONEWIRE_BUS_COUNT];    ///< Next scan due time counter
static uint32_t OwBusCheckTime[ONEWIRE_BUS_COUNT];      ///< Device check timer, indexed by bus ID
static OS_EVENT  *pOneWireQ;                            ///< Request Q
static OWREQUEST RequestPool[MAX_OW_REQUESTS];          ///< Requests
static OWDEVICEINFO OwDeviceList[ONEWIRE_MAX_DEVICES];         ///< All detected devices
static OS_TMR *pTimerDeviceMon;                         ///< Device monitoring timer
static OS_EVENT *pMutex_OneWire = NULL;                 ///< 1-Wire request pool mutex
static bool OnewireEnabled = false;

///\todo move the below to SRAM memory. To be done post system level memory design
uint8_t oneWireTempData[ONEWIRE_MEMORY_TEMPDATA_SIZE + MEMORY_FENCE_SIZE_BYTES];  ///< Memory bank to store 1-wire temporary data
uint8_t slaveMAC[ONEWIRE_MEMORY_BANK_SIZE + MEMORY_FENCE_SIZE_BYTES];             ///< Memory bank to store 1-wire Slave MAC data
uint8_t slaveEEPROMPage[ONEWIRE_MEMORY_BANK_SIZE + MEMORY_FENCE_SIZE_BYTES];      ///< Memory bank to store 1-wire Slave EEPROM data
uint8_t challengeData[ONEWIRE_MEMORY_BANK_SIZE + MEMORY_FENCE_SIZE_BYTES];        ///< Memory bank to store 1-wire Challenge data to be used for MAC computation
uint8_t masterMAC[ONEWIRE_MEMORY_BANK_SIZE + MEMORY_FENCE_SIZE_BYTES];            ///< Memory bank to store 1-wire Master MAC data

static uint8_t BusShortCounter[ONEWIRE_BUS_COUNT];
static bool BusShorted[ONEWIRE_BUS_COUNT] = {false};

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static ONEWIRE_STATUS OwProcessDeviceCheckRequest(ONEWIRE_DEVICE_ID Device);
static ONEWIRE_STATUS OwProcessRequests(OWREQUEST *pRequest);
static ONEWIRE_STATUS OwProcessListByFamilyRequest(OWDEVICEQUERYMSG *pMessage);
static ONEWIRE_STATUS OwProcessListByBusRequest(OWDEVICEQUERYMSG *pMessage);
static ONEWIRE_STATUS OwDeviceListByBus(ONEWIRE_BUS Bus, ONEWIRE_DEVICE_ID *pDeviceList, uint8_t *pCount);
static ONEWIRE_STATUS OwProcessConfigRequest(ONEWIREOPTIONS *pOptions);
static ONEWIRE_STATUS OwPostRequest(void *pMessage, OW_REQUEST_TYPE RequestType);
static GPIO_SIGNAL OwGetSignalByBusID(ONEWIRE_BUS Bus);
static OWREQUEST *OwRequestSlotGet(void);
static void OwRequestSlotRelease(OWREQUEST *pRequest);
static void OwDefaultEventHandler(ONEWIRE_EVENT Event, ONEWIRE_DEVICE_ID Device);
static void OwDefaultBusInfo(ONEWIREOPTIONS *pOption, ONEWIRE_BUS  Bus);
static void  OwTask(void *pArg);
static OWREQUEST *OwWaitForRequest(OW_PROC_STATE *pNextState);
static ONEWIRE_STATUS OwProcessTransferRequest(ONEWIREFRAME *pFrame);
static void OwReleaseCaller(OWREQUEST *pRequest);
static ONEWIRE_STATUS OwSelectBusByDevice(ONEWIRE_DEVICE_ID Device);
static void OwCheckDevices(void);
static void OwNotifyBusUser(ONEWIRE_EVENT Event, ONEWIRE_BUS Bus, ONEWIRE_DEVICE_ID Device);
static ONEWIREOPTIONS  *OwBusConfigGet(ONEWIRE_BUS Bus);
static void OwProcessScan(void);
static void OwTimerStart(void);
static void OwTimerHandler (void *pThis, void *pArgs);
static ONEWIRE_STATUS OwBusSelect(ONEWIRE_BUS Bus);
static ONEWIRE_STATUS OwDeviceRegistryAdd(ONEWIRE_DEVICE_ID Device, ONEWIRE_BUS Bus);
/* Below functions prototypes for OneWire Authentication functionality implementation */
static ONEWIRE_STATUS OWSlaveRwScratchpad(ONEWIRE_DEVICE_ID Device,uint8_t CmdParam);
static ONEWIRE_STATUS OWSlaveReadMAC(ONEWIRE_DEVICE_ID Device);
static ONEWIRE_STATUS OWComputeAuthMac(ONEWIRE_DEVICE_ID Device, uint8_t *pManufacturerId);
static ONEWIRE_STATUS OWComputeMasterSecret(ONEWIRE_DEVICE_ID Device, uint8_t *pManufacturerId);
static ONEWIRE_STATUS OwProcessDeviceAuthenticateRequest( ONEWIRE_DEVICE_ID Device);
static ONEWIRE_STATUS OneWireVerifySecret(ONEWIRE_DEVICE_ID Device);
static ONEWIRE_STATUS OneWireSaveSlaveBindData(ONEWIRE_DEVICE_ID Device, uint8_t Page, uint8_t *pBuffer);
static ONEWIRE_STATUS OWSlaveGetManufacturerId(ONEWIRE_DEVICE_ID Device, uint8_t *pManufacturerId);
static bool OwTransferHandlerBindingData(uint8_t PacketIndex, uint8_t* pRxData);
static bool OWTransferHandlerScratchpad(uint8_t PacketIndex, uint8_t* pRxData);
static bool OWTransferHandler(uint8_t PacketIndex, uint8_t* pRxData);
static bool OWTransferHandlerSlaveMac(uint8_t PacketIndex, uint8_t* pRxData);
static void OwFrameClear(ONEWIREFRAME* pFrame );
static ONEWIRE_STATUS L3_CheckConnectorBus(void);
static ONEWIRE_STATUS L3_CheckClamshellBus(void);
/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/* ========================================================================== */
/**
 * \fn      void  OwTask(void *pArg)
 *
 * \brief   1-Wire controller task
 *
 * \details The task process all 1-Wire requests, runs a state-machine to handle
 *          request processing activities.
 *
 * \param   pArg - Task arguments
 *
 * \return  None
 *
 * ========================================================================== */
static void  OwTask(void *pArg)
{
    uint8_t Index;
    bool Error;
    uint8_t u8OsError;                  /* OS Error Code */
    OWREQUEST* pRequest;
    static OW_PROC_STATE OwProcState;                           /* 1-Wire processor state */
    static void* pOwQStorage[MAX_OW_REQUESTS];              /*! Request Q storage */

    Error = false;

    /* Initialize request message Q */
    pOneWireQ = SigQueueCreate(pOwQStorage, MAX_OW_REQUESTS);

    if ( NULL == pOneWireQ )
    {
        /* Something went wrong with message Q creation, mark error flag */
        /*\todo: Add exception handler */
        Error = true;
        Log(ERR, "OwTask: Message Q Creation Error");
    }

    /* Create semaphore pool. Used to block caller until request is processed */

    /* Default initialization for each bus */
    for (Index=0; Index < MAX_OW_REQUESTS; Index++)
    {
       RequestPool[Index].pMessage = NULL;           /* No message by default */
       RequestPool[Index].pSema = SigSemCreate(0, "OwTask-Sem", &u8OsError);    /* Semaphore to block by default */

       if (NULL == RequestPool[Index].pSema)
       {
           /* Something went wrong with message Q creation, return with error */
           Error = true;
           Log(ERR, "OwTask: Create Semaphore Error");
           break;
       }
    }

    do
    {
        /* \todo: replace this trap by ASSERT */
        OSTimeDly(OW_YIELD_WHEN_DEAD);
    }
    while (Error);

    /* Create a OS Timer for monitoring devices */
    OwTimerStart();

    /* Default the processor is disabled */
    OwProcState = OW_PROC_STATE_DISABLED;

    /* All good, let's start processing requests */
    pRequest = OwWaitForRequest(&OwProcState); // Force initialize pRequest for MISRA compliance

    while (true)
    {
        switch(OwProcState)
        {
            case OW_PROC_STATE_IDLE:
                /* Wait for a new request. Valid request would move through rest of the state-machine.
                   Invalid request is discarded and state-machine waits for a new request again */
                pRequest = OwWaitForRequest(&OwProcState);
                break;

            case OW_PROC_STATE_REQUEST:
                /* Process the request */
                pRequest->Status = OwProcessRequests(pRequest);
                // break;        /*Intentional drop, retaining notify state for future enhancements */

            case OW_PROC_STATE_NOTIFY:
                /* Any post command processing work here - Release the blocked requestor */
                OwReleaseCaller(pRequest);
                OwProcState = OW_PROC_STATE_IDLE;
                break;

            case OW_PROC_STATE_CHECK_ALL:
                /* Do all device check here */
                OwCheckDevices();
                OwTimerStart();    /* Monoshot timer, re-enable it */
                OwProcState = OW_PROC_STATE_SCAN;
                break;

            case OW_PROC_STATE_SCAN:
                /* Perform device scan */
                OwProcessScan();
                OwProcState = OW_PROC_STATE_IDLE;
                break;

            case OW_PROC_STATE_FAULT:
                /* Do nothing, just yield to other tasks. \todo: add logs when LOG feature available */
                /* \todo: Add appropriate exception handler */
                OSTimeDly(OW_YIELD_WHEN_DEAD);
                break;

            case OW_PROC_STATE_DISABLED:
                /*Loop here until the module is re-enabled.
                  Timer need not be disabled as it is monoshot timer */

                while(false == OnewireEnabled)
                {
                    OSTimeDly(OW_YIELD_WHEN_DEAD);    /*\todo: Suspend task here */
                }
                OSTimeDly(3000);
                OwTimerStart();    /* Monoshot timer, re-enable it */

                /* Module is re-enabled, start processing the requests */
                OwProcState = OW_PROC_STATE_IDLE;
                break;


            default:
                /* Do nothing, just yield to other tasks. \todo: Add exception handler? */
                OSTimeDly(OW_YIELD_WHEN_DEAD);
                break;
        }

        /* Move to Disabled state if requested */
        if(!OnewireEnabled)
        {
            OwProcState = OW_PROC_STATE_DISABLED;
        }
    }
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OwProcessRequests(OWREQUEST *pRequest)
 *
 * \brief   Process received request
 *
 * \details This functions extracts request information from the message and invoke
 *          appropriate request handler
 *
 * \param   pRequest - pointer to Request
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwProcessRequests(OWREQUEST *pRequest)
{
    ONEWIRE_STATUS Status;
    ONEWIREFRAME *pFrame;               /* Needed for transfer request processing */
    ONEWIRE_DEVICE_ID DeviceAddress;    /* Needed for transfer device check processing */
    OWDEVICEQUERYMSG *pDevQueryMsg;     /* Needed for device list query processing */
    ONEWIREOPTIONS *pBusOptions;        /* Needed for bus configuration request processing */

    switch(pRequest->ReqType)
    {
        case OW_REQUEST_TRANSFER:
            pFrame = (ONEWIREFRAME*)pRequest->pMessage;

            /* Select the required bus before starting the transfer */
            Status = OwSelectBusByDevice(pFrame->Device);

            if (ONEWIRE_STATUS_OK != Status)
            {
                /* Something went wrong, Bus select failed. Respond to requestor with an error */
                Status = ONEWIRE_STATUS_ERROR;
            }
            else
            {
                /* Required bus selected, good to initiate transfer */
                Status = OwProcessTransferRequest(pFrame);
            }
            break;

        case OW_REQUEST_LISTBY_BUS:
            /* Extract device query info from the request message */
            pDevQueryMsg = (OWDEVICEQUERYMSG*)pRequest->pMessage;

            /* Process the device list query */
            Status = OwProcessListByBusRequest(pDevQueryMsg);
            break;

        case OW_REQUEST_LISTBY_FAMILY:
            /* Extract device query info from the request message */
            pDevQueryMsg = (OWDEVICEQUERYMSG*)pRequest->pMessage;

            /* Process the device list query */
            Status = OwProcessListByFamilyRequest(pDevQueryMsg);
            break;

        case OW_REQUEST_CONFIG:
            /* Extract bus configuration info from the request message */
            pBusOptions = (ONEWIREOPTIONS*)pRequest->pMessage;

            /* Process the config request */
            Status = OwProcessConfigRequest(pBusOptions);
            break;

        case OW_REQUEST_CHECK:
            /* The device ID is in message, retrieve it */
            DeviceAddress = *((ONEWIRE_DEVICE_ID*)pRequest->pMessage);

            /* Process the device check request */
            Status = OwProcessDeviceCheckRequest(DeviceAddress);
            break;

        case OW_REQUEST_AUTHENTICATE:
            /* The device ID is in message, retrieve it */
            DeviceAddress = *((ONEWIRE_DEVICE_ID*)pRequest->pMessage);

            /* Process the device Authenticate request */
            Status = OwProcessDeviceAuthenticateRequest(DeviceAddress);
            break;

        default:
            /* Do nothing, \todo: Add exception handler? */
            Status = ONEWIRE_STATUS_PARAM_ERROR;
            break;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \fn      void OwCheckDevices(void)
 *
 * \brief   Check device status
 *
 * \details Check connection status for all devices in the registry
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void OwCheckDevices(void)
{
    ONEWIRE_STATUS Status;
    uint8_t DeviceIndex;
    uint8_t BusIndex;

    ONEWIREOPTIONS *pBusOpt;
    OWDEVICEINFO *pDevInfo;

    for (BusIndex = 0; BusIndex < ONEWIRE_BUS_COUNT ; BusIndex++)
    {
        do
        {
            /* Check reducing timer to see if it's time for check */
            if (OwBusCheckTime[BusIndex] > OW_TIMER_MS)
            {
                /* Not yet. Just update the counter, skip checking*/
                OwBusCheckTime[BusIndex] -= OW_TIMER_MS;
                break;
            }

            /* Reload check time from bus info */
            pBusOpt = OwBusConfigGet((ONEWIRE_BUS)BusIndex);
            if (NULL == pBusOpt)
            {
                /* Should never come here, just a fail-safe handling */
                break;
            }

            OwBusCheckTime[BusIndex] = pBusOpt->KeepAlive;

            Status = OwBusSelect(pBusOpt->Bus);
            if( ONEWIRE_STATUS_OK != Status)
            {
                /* Bus selection error */
                break;
            }
            OSTimeDly(10);

            pDevInfo = OwDeviceList;  /* Start with the base of device list */

            /* Required bus is selected, Now run the device check */
            for (DeviceIndex = 0; DeviceIndex < ONEWIRE_MAX_DEVICES; DeviceIndex++)
            {
                if ((pDevInfo->Bus == BusIndex) && (ONEWIRE_DEVICE_ID_INVALID != pDevInfo->Device))
                {
                    Status = OwTransportCheck(&pDevInfo->Device);
                    if (ONEWIRE_STATUS_NO_DEVICE == Status)
                    {
                        OwNotifyBusUser(ONEWIRE_EVENT_LOST_DEVICE, pDevInfo->Bus, pDevInfo->Device);

                        /* Remove device from the list */
                        pDevInfo->Device = ONEWIRE_DEVICE_ID_INVALID;
                        pDevInfo->Bus = ONEWIRE_BUS_COUNT;
                        pDevInfo->Family = ONEWIRE_DEVICE_FAMILY_LAST;
                        BusShorted[BusIndex] = false;
                    }
                    if(ONEWIRE_STATUS_BUS_ERROR == Status)
                    {
                        OwNotifyBusUser(ONEWIRE_EVENT_BUS_SHORT, pDevInfo->Bus, pDevInfo->Device);
                    }
                }
                pDevInfo++; /* Point to the next device item in the device list */
            }
        } while (false);
    }
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OwDeviceRegistryAdd(ONEWIRE_DEVICE_ID Device, ONEWIRE_BUS Bus)
 *
 * \brief   Add new device to registry
 *
 * \details This function adds device to device registry
 *
 * \param   Device - Device to be added to the list
 * \param   Bus - Bus on which the device is found
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwDeviceRegistryAdd(ONEWIRE_DEVICE_ID Device, ONEWIRE_BUS Bus)
{
    ONEWIRE_STATUS Status;
    ONEWIREOPTIONS *pBusOpt;
    ONEWIRE_EVENT Event;
    uint8_t Index;
    uint8_t Empty;

    Empty = ONEWIRE_MAX_DEVICES;    /* Invalid by default */
    Status =  ONEWIRE_STATUS_OK;    /* Default Status */

    do
    {
        /* Add the device to the list if not present already */
        for (Index = 0; Index < ONEWIRE_MAX_DEVICES; Index++)
        {
            if ((ONEWIRE_MAX_DEVICES == Empty) && (ONEWIRE_DEVICE_ID_INVALID == OwDeviceList[Index].Device))
            {
                /* Found an empty slot, will use if no entry found */
                Empty = Index;
            }

            /* Check if the device already present */
            if (Device == OwDeviceList[Index].Device)
            {
                /* Device already present in the list */
                break;
            }
        }

        /* Device in the list already? */
        if (Index < ONEWIRE_MAX_DEVICES)
        {
            /* device Already present, just exit */
            break;
        }

        /* Check if have found an empty slot in the registry to store new device */
        if (Empty >= ONEWIRE_MAX_DEVICES)
        {
            /* The list is full, unlikely to happen */
            Status = ONEWIRE_STATUS_ERROR;
            break;
        }

        /* We found an empty slot and no duplicates, add it to the registry */
        OwDeviceList[Empty].Device = Device;
        OwDeviceList[Empty].Bus = Bus;
        OwDeviceList[Empty].Family = (ONEWIRE_DEVICE_FAMILY)(Device & (ONEWIRE_DEVICE_ID)0xFF);  /* Lower byte has family code */

        /* Let's find out if the device family is registered */
        pBusOpt = OwBusConfigGet(Bus);
        if (NULL != pBusOpt)
        {
            Event = ONEWIRE_EVENT_UNKNOWN_DEVICE;    /* Default notification */

            /* Check if the device is from a known family */
             for (Index = 0; Index < ONEWIRE_MAX_DEVICE_FAMILY; Index++)
             {
                if ((pBusOpt->Family[Index] == OwDeviceList[Empty].Family) \
                    || (pBusOpt->Family[Index] == ONEWIRE_DEVICE_FAMILY_ALL))
                {
                    /* Yes, the device family is in the known list*/
                    Event = ONEWIRE_EVENT_NEW_DEVICE;
                    break;
                }
             }

            /* Now, push the notification to the bus owner */
            OwNotifyBusUser(Event, Bus, Device);
        }

        /* Let's return OK even if the notification fails, device list can always be queried */
        Status =  ONEWIRE_STATUS_OK;
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \fn      void OwNotifyBusUser(ONEWIRE_EVENT Event, ONEWIRE_BUS Bus, ONEWIRE_DEVICE_ID Device)
 *
 * \brief   Notify bus
 *
 * \details This function notifies the bus owner with an event along with
 *          device address and bus ID information
 *
 * \param   Event - Event to be notified
 * \param   Bus - Bus on which the device is found
 * \param   Device - Onewire Device ID
 *
 * \return  None
 *
 * ========================================================================== */
static void OwNotifyBusUser(ONEWIRE_EVENT Event, ONEWIRE_BUS Bus, ONEWIRE_DEVICE_ID Device)
{
    ONEWIREOPTIONS *pBusOpt;
    pBusOpt = OwBusConfigGet(Bus);
    ONEWIRE_STATUS ClamStatus;
    ONEWIRE_STATUS ConnStatus;
    
    do
    {
        if (NULL == pBusOpt)
        {
            break;
        }
        if (NULL == pBusOpt->pHandler)
        {
            break;
        }
        /* Check for Connector Bus */
        if (UNKNOWN_DEVICE == Device)
        {
            pBusOpt->pHandler(Event, Device);   
            break;
        }
        /* Process New Device or Lost Device Events here */
        if (ONEWIRE_EVENT_BUS_SHORT != Event) 
        {
            if ((Event == ONEWIRE_EVENT_LOST_DEVICE) && (Bus == ONEWIRE_BUS_CONNECTORS) && (BusShorted[Bus]) )
            {
                BusShorted[Bus] = false;
            }
  
            pBusOpt->pHandler(Event, Device);
            Log(DBG,"1-Wire %s, Bus: %d, ID: 0x%16llX", ((Event == ONEWIRE_EVENT_NEW_DEVICE)? "New Device" : "Lost Device"), Bus, Device);
			break;
        }
        
        /* To avoid repeated Bus short notification */
        BREAK_IF(BusShorted[Bus]);
        
        ConnStatus = L3_CheckConnectorBus();
        ClamStatus = L3_CheckClamshellBus();
        /* Connector Bus has No device */

        if ((ONEWIRE_STATUS_OK == ConnStatus) && (Device == NO_DEVICE_ONBUS) && (ONEWIRE_EVENT_BUS_SHORT == Event))
        {
            /* Device on Clamshell Bus and No Device on Connector bus */
            if (ONEWIRE_STATUS_OK != ClamStatus)
            {
                pBusOpt->pHandler(Event, Device); 
                BusShorted[Bus] = true;
                Log(DBG," Event: %s, on %d: 0x%llx", "Device Shorted", Bus, Device);
                break;
            }
            /* No Device on Clamshell and Connector Bus */
            else if (ONEWIRE_STATUS_OK == ClamStatus) 
            {
                pBusOpt->pHandler(Event, Device); 
                BusShorted[Bus] = true;
                Log(DBG," Event: %s, on %d: 0x%llx", "Device Shorted", Bus, Device);
                break;
            }
        }
        /* Already devices on clamshell and Local Bus */
        else if ((ONEWIRE_STATUS_OK == ConnStatus) && (Device != NO_DEVICE_ONBUS) && (ONEWIRE_EVENT_BUS_SHORT == Event))
        {
            /* Device on Clamshell Bus - Local/Clamshell Bus Short */
            if (ONEWIRE_STATUS_OK != ClamStatus) 
            {
                pBusOpt->pHandler(Event, Device); 
                BusShorted[Bus] = true;
                Log(DBG," Event: %s, on %d: 0x%llx", "Device Shorted", Bus, Device);
                break;
            }
            /* Device on Local Bus and Shorted */
            else if (ONEWIRE_STATUS_OK == ClamStatus) 
            {
                pBusOpt->pHandler(Event, Device); 
                BusShorted[Bus] = true;
                Log(DBG," Event: %s, on %d: 0x%llx", "Device Shorted", Bus, Device);
                break;
            }
        }
        /* Device on Local/Clamshell/Connector Bus - Short on any Bus */
        else if ((ONEWIRE_STATUS_OK != ClamStatus) && (ONEWIRE_STATUS_OK != ConnStatus) && (ONEWIRE_EVENT_BUS_SHORT == Event))      
        {
            pBusOpt->pHandler(Event, Device); 
            BusShorted[Bus] = true;
            Log(DBG," Event: %s, on %d: 0x%llx", "Device Shorted", Bus, Device);
            break;
        }      
    }while (false);
    return;
}

/* ========================================================================== */
/**
 * \fn      void OwProcessScan(void)
 * \brief   Scan bus to detect new devices
 *
 * \details This function checks if it is time to scan a/any bus and bus still
 *          does not have expected number of device found on it. If bus already
 *          has required number of devices, scan on this bus is skipped.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void OwProcessScan(void)
{
    static ONEWIRE_DEVICE_ID NewOwDeviceList[ONEWIRE_MAX_DEVICES];
    ONEWIRE_STATUS Status;
    uint8_t NewDeviceCount;         /* How many more we want to find?*/
    uint8_t DeviceCount;            /* How many we already have */
    uint8_t Index;
    ONEWIRE_BUS BusToScan;          /* Bus to be scanned */
    ONEWIREOPTIONS *pBusOptions;
    BusToScan = ONEWIRE_BUS_CLAMSHELL;

    do
    {
        /* Update counter, avoid rollback, check if it's time to scan */
        OwBusScanDueTime[BusToScan] = (OwBusScanDueTime[BusToScan] > OW_TIMER_MS)? (OwBusScanDueTime[BusToScan] - OW_TIMER_MS) : 0;

        do
        {
            if (OwBusScanDueTime[BusToScan] > 0)
            {
                break;  /* Not yet time for a scan on this bus */
            }

            /* Get bus options to check if it's time to scan */
            pBusOptions = OwBusConfigGet(BusToScan);
            if (NULL == pBusOptions || !pBusOptions->ScanInterval)
            {
                break;  /* Invalid bus options returned, just skip the scan */
                         /*scanInterval ==0 indicates bus is not selected*/
            }


            OwBusScanDueTime[BusToScan] = pBusOptions->ScanInterval;    /* Reload the counter */

            OwDeviceListByBus(BusToScan, NULL, &DeviceCount);   /* Get the device count alone */

            /* Do we already have required number of device? */
            if (DeviceCount >= pBusOptions->DeviceCount)
            {
                /* Yes, bus has got expected number of devices, skip scan */
                break;
            }

            /* Select the bus before the scan */
            Status = OwBusSelect(BusToScan);
            if (ONEWIRE_STATUS_OK != Status)
            {
               break;
            }

            /*Request total device count expected on the bus, ensure the count is not more than we can hold */
            NewDeviceCount = (pBusOptions->DeviceCount <= ONEWIRE_MAX_DEVICES) ? pBusOptions->DeviceCount: ONEWIRE_MAX_DEVICES;

            /* Start the scan */
            Status = OwTransportScan(BusToScan, OW_SCAN_TYPE_FULL, NewOwDeviceList, &NewDeviceCount);

            /* Scan complete, did we find more devices? Or encountered errors? */
            switch(Status)
            {
                case ONEWIRE_STATUS_OK:
                    /* Update device list */
                    Index = 0;
                    while(Index < NewDeviceCount)
                    {
                        OwDeviceRegistryAdd(NewOwDeviceList[Index], BusToScan);          /* What if device list already full?*/
                        Index++;  
                        BusShortCounter[BusToScan] = 0;
                    };
                    break;

                case ONEWIRE_STATUS_NO_DEVICE:
                    /* Do nothing*/
                    break;

                case ONEWIRE_STATUS_BUSY:
                    /* Do anything about this? */
                    break;

                case ONEWIRE_STATUS_BUS_ERROR:
                    /* Checking Bus Short for 3 time and then Notify */
                    if (NOTIFY_HALT != BusShortCounter[BusToScan])
                    {
                        BusShortCounter[BusToScan]++;         
                        if (MAX_SHORT_COUNT <= BusShortCounter[BusToScan]) 
                        {
                            OwNotifyBusUser(ONEWIRE_EVENT_BUS_SHORT, BusToScan, NO_DEVICE_ONBUS);
                            BusShortCounter[BusToScan] = NOTIFY_HALT;
                        }
                    }                 
                    break;

                case ONEWIRE_STATUS_ERROR:
                    /* Do anything about this? */
                    break;

                default:
                    /* Do nothing*/
                    break;
            }/* end of switch(Status) */

        } while (false);

    } while (++BusToScan < ONEWIRE_BUS_COUNT); /* Pick next bus and check */

    return;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OwProcessTransferRequest(ONEWIREFRAME *pFrame)
 *
 * \brief   Executes a 1-Wire transfer
 *
 * \details This function is invoked from OwTask Process to execute transfer request.
 *          The function also invokes client supplied callback to allow processing
 *          received interim data.
 *
 * \param   pFrame - pointer to 1-Wire request frame
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwProcessTransferRequest(ONEWIREFRAME *pFrame)
{
    ONEWIREPACKET *pOwPacket;
    ONEWIRE_STATUS Status;
    uint8_t Index;
    ONEWIRE_DEVICE_ID *pAddr;

    Status = ONEWIRE_STATUS_ERROR;  /* Default status */

    /* pFrame is already validated by the caller, it's safe to access */
    pAddr = &pFrame->Device; /* Needed to control device select command which is sent only once */

    Index = 0;
    do
    {
        pOwPacket = &pFrame->Packets[Index];

        /* Check if this is end of the frame (typically with NULL/0 fields) */
        if ((0 == pOwPacket->nTxSize) && (0 == pOwPacket->nRxSize) &&\
            (NULL == pOwPacket->pTxData) && (NULL == pOwPacket->pRxData))
        {
            /* If first packet is invalid, return error. Else all packets are processed, return OK */
            Status = (0 == Index) ? ONEWIRE_STATUS_PARAM_ERROR: ONEWIRE_STATUS_OK;
            break;
        }

        if (pOwPacket->nTxSize > 0)
        {
            /* Transmit data */
            Status = OwTransportSend(pAddr, pOwPacket->pTxData, pOwPacket->nTxSize);
            if (ONEWIRE_STATUS_OK != Status)
            {
                /* Error occurred, Exit transfer */
                Log(ERR, "OwProcessTransferRequest: Transmit data Error");
                break;
            }
            pAddr = NULL;    /* Address to be transferred only once to select the device */
        }

        /* Receive data */
        if (pOwPacket->nRxSize > 0)
        {
            Status = OwTransportReceive(pOwPacket->pRxData, pOwPacket->nRxSize);
            /* Error occurred, Exit transfer */
            if (ONEWIRE_STATUS_OK != Status)
            {
                /* Error occurred, Exit transfer */
                Log(ERR, "OwProcessTransferRequest: Receive data Error");
                break;
            }
        }

        if (pFrame->pHandler)
        {
             /* Notify caller and allow process interim data Rx */
             if (pFrame->pHandler(Index, pOwPacket->pRxData))
             {
                /* Caller requested to abort */
                Status = ONEWIRE_STATUS_ERROR;
                break;
             }
        }

        /* Update index to pick next packet in the frame */
        Index++;
    } while (Index < ONEWIRE_MAX_PACKETS);

    /* terminate the transfer */
    OwTransportReceive(NULL, 0);

    return Status;
}

/* ========================================================================== */
/**
 * \fn      OWREQUEST* OwWaitForRequest(OW_PROC_STATE *pNextState)
 *
 * \brief   Wait for request message
 *
 * \details Waits for request queue. Valid request kicks in the state-machine
 *          to process the request.
 *
 * \param    pNextState - pointer to Current/Updated state of the state-machine
 *
 * \return  OWREQUEST - Request message, NULL if any errors
 *
 * ========================================================================== */
static OWREQUEST *OwWaitForRequest(OW_PROC_STATE *pNextState)
{
    OWREQUEST *pRequest;
    uint8_t Error;
    ONEWIRE_STATUS Status;

    pRequest = NULL;                /* Default return value */
    Status = ONEWIRE_STATUS_ERROR;  /* Default processing status */

    do
    {
        /* Wait on Q for new request, perform device check/scan if idle */
        pRequest = (OWREQUEST*) OSQPend(pOneWireQ, 0, &Error);

        /* Check if any error while dequeuing an item */
        if (OS_ERR_NONE != Error)
        {
            /* Q Error, this should never happen. Yield and retry. \todo: Exception? */
            Log(ERR, "OwWaitForRequest: Q Error on wait for new request");
            OSTimeDly(OW_YIELD_WHEN_DEAD);
            break;
        }

        /* Validate the request pointer */
        if (NULL == pRequest)
        {
            /* Invalid Request. Exit, return, come back, wait for new request */
            break;
        }

        /* Check if the request contains a valid info */
        if (NULL == pRequest->pMessage)
        {
            /* If any parameter issue, report it back to the caller */
            Status = ONEWIRE_STATUS_PARAM_ERROR;

            /* Invalid message info, report it to the caller */
            pRequest->Status = Status;
            OwReleaseCaller(pRequest);

            pRequest = NULL; /*Actually not needed, just a fail-safe value */
            break;  /* Exit, state not changed, would come back and wait for new request */
        }

        /* Valid message info, change state based request type */
        switch(pRequest->ReqType)
        {
            case OW_REQUEST_CHECK:
            case OW_REQUEST_TRANSFER:
            case OW_REQUEST_LISTBY_BUS:
            case OW_REQUEST_CONFIG:
            case OW_REQUEST_LISTBY_FAMILY:
            case OW_REQUEST_AUTHENTICATE:
                *pNextState = OW_PROC_STATE_REQUEST;
                break;

            case OW_REQUEST_TIMER:
                *pNextState  = OW_PROC_STATE_CHECK_ALL;
                break;

            default:
                *pNextState  = OW_PROC_STATE_FAULT;
                break;
        }
    } while (false);

   return pRequest;
}

/* ========================================================================== */
/**
 * \fn      void OwReleaseCaller(OWREQUEST *pRequest)
 *
 * \brief   Release blocked caller task
 *
 * \details Release 1-Wire interface function caller by release the semaphore
 *          on which the caller is blocking
 *
 * \param   pRequest - pointer to pending semaphore and request processing status
 *
 * \return  None
 *
 * ========================================================================== */
static void OwReleaseCaller(OWREQUEST *pRequest)
{
    uint8_t Error;

    /* Initialize the variables */
    Error = 0u;

    if (NULL != pRequest->pSema)
    {
       Error = OSSemPost(pRequest->pSema);  /*Release the caller, and take it back */
    }

    if (OS_ERR_NONE != Error)
    {
        Log(ERR, "OwReleaseCaller: Error on OSSemPost");
       /*\todo: Add exception handler here */
    }
    return;
}

/* ========================================================================== */
/**
 * \fn      ONEWIREOPTIONS*  OwBusConfigGet(ONEWIRE_BUS Bus)
 *
 * \brief   Get 1-Wire bus configuration
 *
 * \details Retrieves 1-Wire bus configuration based on the bus specified
 *
 * \param   Bus - Bus
 *
 * \return  ONEWIREOPTIONS - pointer to Bus configuration
 *
 * ========================================================================== */
static ONEWIREOPTIONS *OwBusConfigGet(ONEWIRE_BUS Bus)
{
    uint8_t Index;
    ONEWIREOPTIONS *BusOptions;

    BusOptions = NULL;

    for (Index=0; Index < ONEWIRE_BUS_COUNT; Index++)
    {
        if (Bus == OwBusGroup[Index].Bus)
        {
            BusOptions = &OwBusGroup[Index];
            break;
        }
    }
    return BusOptions;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OwProcessListByFamilyRequest(OWDEVICEQUERYMSG *pMessage)
 *
 * \brief   Retrieve 1-Wire device discovered from a specific family
 *
 * \details Retrieves list of 1-Wire devices from specified family discovered
 *          on a bus.
 *
 * \note    This function is invoked by the request processor.
 *
 * \param   pMessage - pointer to Message containing search criteria and list to store device
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwProcessListByFamilyRequest(OWDEVICEQUERYMSG *pMessage)
{
    uint8_t DevIdx;
    uint8_t DeviceCopied;
    ONEWIRE_STATUS Status;

    Status = ONEWIRE_STATUS_PARAM_ERROR;

    if (NULL != pMessage)
    {
        DeviceCopied = 0;
        Status = ONEWIRE_STATUS_OK;

        for (DevIdx = 0; (DevIdx < ONEWIRE_MAX_DEVICES) && (DeviceCopied < pMessage->Count) ; DevIdx++)
        {
           if ((ONEWIRE_DEVICE_FAMILY_ALL == pMessage->Info.Family) || (OwDeviceList[DevIdx].Family == pMessage->Info.Family))
           {
               *pMessage->pList = OwDeviceList[DevIdx].Device;
               pMessage->pList++;
               DeviceCopied++;
           }
        }

        pMessage->Count = DeviceCopied;
    }
    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OwProcessListByBusRequest(OWDEVICEQUERYMSG *pMessage)
 *
 * \brief   Retrieve 1-Wire device discovered on a specific bus.
 *
 * \details Retrieves list of 1-Wire devices from specified bus.
 *
 * \note    This function is invoked by the request processor.
 *
 * \param   pMessage - pointer to Message containing search criteria and list to store device
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwProcessListByBusRequest(OWDEVICEQUERYMSG *pMessage)
{
    ONEWIRE_STATUS Status;

    Status = ONEWIRE_STATUS_PARAM_ERROR;

    if (NULL != pMessage)
    {
        if (NULL != pMessage->pList)
        {
            Status = OwDeviceListByBus(pMessage->Info.Bus, pMessage->pList, &pMessage->Count);
        }
    }

    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OwProcessConfigRequest(ONEWIREOPTIONS *pOptions)
 *
 * \brief   Process 1-Wire bus configuration
 *
 * \details Configures a 1-Wire bus with the options specified by the caller
 *
 * \note    This function is invoked by the request processor.
 *
 * \param   pOptions - Message containing configuration information
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwProcessConfigRequest(ONEWIREOPTIONS *pOptions)
{
    ONEWIRE_STATUS Status;
    uint8_t Index;
    ONEWIREOPTIONS *pOptTemp;

    do
    {
        /* pOptions is already validated before posting the request, validate bus ID */
        if (pOptions->Bus >= ONEWIRE_BUS_COUNT)
        {
            Status = ONEWIRE_STATUS_PARAM_ERROR;
            break;
        }

        Status = ONEWIRE_STATUS_OK;  /* Default */
        pOptTemp = &OwBusGroup[0]; // Force initialization for MISRA compliance

        /* Find out the option item in the option list to update.
            Bus is already validated, finding group element is guaranteed */

        for (Index=0; Index < ONEWIRE_BUS_COUNT; Index++)
        {
            pOptTemp = &OwBusGroup[Index];
            if (pOptTemp->Bus == pOptions->Bus)
            {
                OwBusScanDueTime[Index] = pOptTemp->ScanInterval;
                break;
            }
        }

        /* Update the group element with the option */

        pOptTemp->DeviceCount = pOptions->DeviceCount;
        pOptTemp->KeepAlive = pOptions->KeepAlive;
        pOptTemp->pHandler = pOptions->pHandler;
        pOptTemp->ScanInterval = pOptions->ScanInterval;
        pOptTemp->Speed = pOptions->Speed;

        for (Index=0; Index< ONEWIRE_MAX_DEVICE_FAMILY; Index++)
        {
           pOptTemp->Family[Index] = pOptions->Family[Index];
        }

    } while (false);

    return Status;

}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OwDeviceListByBus(ONEWIRE_BUS Bus, ONEWIRE_DEVICE_ID *pDeviceList, uint8_t *pCount)
 *
 * \brief   Retrieves list of  1-Wire devices from  specific bus.
 *
 * \note    This function is invoked by the request processor.
 *
 * \param   Bus - Bus ID
 * \param   pDeviceList - pointer to Device list to be updated
 * \param   pCount - pointer to expected numnber of device. Updated with actual device count
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwDeviceListByBus(ONEWIRE_BUS Bus, ONEWIRE_DEVICE_ID *pDeviceList, uint8_t *pCount)
{
    uint8_t DevIdx;
    uint8_t DeviceCopied;
    ONEWIRE_STATUS Status;

    DeviceCopied = 0;
    Status = ONEWIRE_STATUS_PARAM_ERROR;

    if ((NULL != pCount) && (Bus < ONEWIRE_BUS_COUNT ))
    {
        Status = ONEWIRE_STATUS_OK;

        for (DevIdx = 0; (DevIdx < ONEWIRE_MAX_DEVICES) && (DeviceCopied < *pCount) ; DevIdx++)
        {
           if (OwDeviceList[DevIdx].Bus == Bus)
           {
                if (NULL != pDeviceList)
                {
                    *pDeviceList = OwDeviceList[DevIdx].Device;
                    pDeviceList++;
                }
               DeviceCopied++;
           }
        }
        *pCount = DeviceCopied;
    }
    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OwProcessDeviceCheckRequest(ONEWIRE_DEVICE_ID Device)
 *
 * \brief   Check if devices is still connected
 *
 * \details Process request to check if device is responding.
 *
 * \note    This function is called by the 1-Wire request processor which is called
 *          by the function to L3_OneWireDeviceCheck().
 *
 * \param   Device - Device to check
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwProcessDeviceCheckRequest( ONEWIRE_DEVICE_ID Device)
{
    ONEWIRE_STATUS Status;

    Status = OwSelectBusByDevice(Device);

    /* Select the bus */
    if (ONEWIRE_STATUS_OK == Status)
    {
        /* Now bus is selected, check if the device still present */
        Status = OwTransportCheck(&Device);
    }

    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OwProcessDeviceAuthenticateRequest(ONEWIRE_DEVICE_ID Device)
 *
 * \brief   Request the slave device to be Authenticated
 *
 * \details Process request to Authenticate the slave device.
 *
 * \note    This function is called by the 1-Wire request processor which is called
 *          by the function to L3_OneWireAuthenticate().
 *
 * \param   Device - Device to Authenticate
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwProcessDeviceAuthenticateRequest(ONEWIRE_DEVICE_ID Device)
{
    ONEWIRE_STATUS Status;

    Status = OwSelectBusByDevice(Device);


    /* Select the bus */
    if (ONEWIRE_STATUS_OK == Status)
    {
        /* Now bus is selected, check if the device still present */
        Status = OneWireVerifySecret(Device);
    }

    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OwPostRequest(void *pMessage, OW_REQUEST_TYPE RequestType)
 *
 * \brief   Post request to request processor task
 *
 * \details Post request to request processor task and wait for the response.
 *
 * \param   pMessage - pointer to Request message
 * \param   RequestType - 1-Wire transfer frame, may contain multiple packet segments
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwPostRequest(void *pMessage, OW_REQUEST_TYPE RequestType)
{
    ONEWIRE_STATUS Status;
    OWREQUEST *pRequest;
    uint8_t Error;

    do
    {
        Status = ONEWIRE_STATUS_PARAM_ERROR; /* Default error */

        /* Return error if 1-Wire module is disabled */
        if (false == OnewireEnabled)
        {
            Status = ONEWIRE_STATUS_DISABLED;
            break;
        }

        if (NULL == pMessage)
        {
            break;
        }

        /* Check if we have slots on Q */
        pRequest = OwRequestSlotGet();

        if (NULL == pRequest)
        {
            Log(ERR, "OwPostRequest: Message Q is full");
            Status = ONEWIRE_STATUS_Q_FULL;
            break;
        }

        /* Slot available. Now update the request, post and wait.
        The slot is released by the request processor once done */

        pRequest->pMessage = pMessage;
        pRequest->ReqType = RequestType;

        Error = OSQPost(pOneWireQ, pRequest);
        if (OS_ERR_TIMEOUT == Error)
        {
            Log(ERR, "OwPostRequest: Message Post time out error");
            Status = ONEWIRE_STATUS_ERROR;
            break;
        }

        /* wait for the message to be processed */
        OSSemPend(pRequest->pSema, 0, &Error);     /* Timeout more than worst possible delay */
        if (OS_ERR_NONE != Error)
        {
            Log(ERR, "OwPostRequest: OSSemPend time out error");
            Status = ONEWIRE_STATUS_TIMEOUT;
            break;
        }

        /* Return the status from request updated by the request processor */
        Status = pRequest->Status;

        /* Response is available in message, release the requst now*/
        OwRequestSlotRelease(pRequest);

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \fn      OWREQUEST* OwRequestSlotGet(void)
 *
 * \brief   Get a slot for placing request on queue
 *
 * \details Utility function to check if the request queue has a place to hold
 *          the request. The implementation uses the request slot array to decide
 *          if the request can be enqueued.
 *
 * \param   < None >
 *
 * \return  OWREQUEST - pointer to reserved request slot if available, otherwise NULL.
 *
 * ========================================================================== */
static OWREQUEST *OwRequestSlotGet(void)
{
    OWREQUEST *Slot;
    uint8_t Index;
    uint8_t OsError;

    Slot = NULL;    /* Default return value */

    OSMutexPend(pMutex_OneWire, OS_WAIT_FOREVER, &OsError);

    if(OS_ERR_NONE != OsError)
    {
        Log(ERR, "OwRequestSlotGet: OSMutexPend error");
        /* \todo: Exception?? future work */
    }
    else
    {
        /* Got mutex, good to access request pool */
        for (Index=0; Index < MAX_OW_REQUESTS; Index++)
        {
            if (NULL == RequestPool[Index].pMessage)
            {
                RequestPool[Index].pMessage = (void*)true;  /* Just reserving */
                Slot = &RequestPool[Index];
                break;
            }
        }
        OSMutexPost(pMutex_OneWire);
    }

    return Slot;
}

/* ========================================================================== */
/**
 * \fn      void OwRequestSlotRelease(OWREQUEST *pRequest)
 *
 * \brief   Release a slot for back to pool
 *
 * \details
 *
 * \param   pRequest - pointer to request
 *
 * \return  OWREQUEST - pointer to reserved request slot if available, otherwise NULL.
 *
 * ========================================================================== */
static void OwRequestSlotRelease(OWREQUEST *pRequest)
{
    uint8_t OsError;

    OSMutexPend(pMutex_OneWire, OS_WAIT_FOREVER, &OsError);

    if(OS_ERR_NONE != OsError)
    {
        Log(ERR, "OwRequestSlotRelease: OSMutexPend error");
        /* \todo: Add Exception when the feature is available?? */
    }
    else
    {
        /* Freeing the slot, contains request response */
        if (NULL != pRequest->pMessage)
        {
            pRequest->pMessage = NULL;
        }
    }

    OSMutexPost(pMutex_OneWire);

    return;
}

/* ========================================================================== */
/**
 * \fn      void OwDefaultBusInfo(ONEWIREOPTIONS *pOption, ONEWIRE_BUS  Bus)
 *
 * \brief   Initialize Bus configuration to default value
 *
 * \details Sets default configuration values to each option associated with
 *          each 1-Wire bus on the system
 *
 * \param   pOption - pointer to Bus options to initialize
 * \param   Bus - Associated bus ID.
 *
 * \return  None
 *
 * ========================================================================== */
static void OwDefaultBusInfo(ONEWIREOPTIONS *pOption, ONEWIRE_BUS  Bus)
{
    uint8_t Index;

    pOption->Bus  = Bus;                                        /* Bus ID */
    pOption->DeviceCount = OW_DEFAULT_DEVICE_COUNT_ON_BUS;      /* Look for only 2 device */
    pOption->ScanInterval = OW_DEFAULT_SCAN_TIME;               /* Scan duration in mS */
    pOption->KeepAlive = OW_DEFAULT_CHECK_TIME;                 /* Connectivity check duration in mS */
    pOption->Speed = ONEWIRE_SPEED_OD;                         /* Speed preference request */


    if(Bus == ONEWIRE_BUS_EXP)                                  /*Only RTC supports standard speed*/
    {
      pOption->Speed = ONEWIRE_SPEED_STD;
    }

    /* Default family options, 0 overrides all preference */
    for (Index=0; Index < ONEWIRE_MAX_DEVICE_FAMILY ; Index++)
    {
        pOption->Family[Index] = ONEWIRE_DEVICE_FAMILY_ALL;  /* By default support all devices */
    }

    /* Default event handler to process events from the bus */
    pOption->pHandler = &OwDefaultEventHandler;

    return;
}

/* ========================================================================== */
/**
 * \fn      void OwDefaultEventHandler(ONEWIRE_EVENT Event, ONEWIRE_DEVICE_ID Device)
 *
 * \brief   Default 1-Wire events handler
 *
 * \details Dummy event handler associated with each bus
 *
 * \param   Event - 1-Wire event
 * \param   Device - device id
 *
 * \return  None
 *
 * ========================================================================== */
static void OwDefaultEventHandler(ONEWIRE_EVENT Event, ONEWIRE_DEVICE_ID Device)
{
    Log(DBG, "OwDefaultEventHandler: Event = %d, Device ID = 0x%16llX", Event, Device);
    return;
}

/* ========================================================================== */
/**
 * \fn      GPIO_SIGNAL OwGetSignalByBusID(ONEWIRE_BUS Bus)
 *
 * \brief   Get GPIO control signal associated with the bus
 *
 * \details Each 1-Wire bus is selected via multiplexer to connect to 1-Wire
 *          master device. The GPIO signal selects a bus to connect to 1-Wire
 *          master.
 *
 * \param   Bus - 1-Wire Bus
 *
 * \return  GPIO_SIGNAL - Associated GPIO signal, GPIO_SIGNAL_LAST if errors.
 *
 * ========================================================================== */
static GPIO_SIGNAL OwGetSignalByBusID(ONEWIRE_BUS Bus)
{
    uint8_t Index;
    GPIO_SIGNAL Signal;

    /* 1-Wire Bus and GPIO signal mapping */
    static const OWBUSSIGNALPAIR BusSignalGroup[] =
    {
        {ONEWIRE_BUS_LOCAL,         GPIO_1W_BATT_ENABLE},
        {ONEWIRE_BUS_CLAMSHELL,     GPIO_1W_SHELL_EN},
        {ONEWIRE_BUS_CONNECTORS,    GPIO_1W_AD_EN},
        {ONEWIRE_BUS_EXP,           GPIO_1W_EXP_ENABLE},
        {ONEWIRE_BUS_INVALID,       GPIO_SIGNAL_LAST}    /*! End of array marker */
    };

    Signal = GPIO_SIGNAL_LAST;

    /* Loop through the look up table to find out associated GPIO signal
       ONEWIRE_BUS_INVALID is used as last item marked, don't change it */
    for (Index = 0; BusSignalGroup[Index].Bus != ONEWIRE_BUS_INVALID; Index++)
    {
        if (BusSignalGroup[Index].Bus == Bus)
        {
            Signal = BusSignalGroup[Index].Signal;
            break;
        }
    }

    return Signal;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OwBusSelect(ONEWIRE_BUS Bus)
 *
 * \brief   Connect the selected bus to the One Wire bus master (DS2465)
 *
 * \details Clear all GPIO lines that control bus selection except for the
 *          specified bus, then assert the GPIO line for the specified bus.
 *
 * \note    Bus reset not issued as anyway the transfer/search will issue it
 *
 * \param   Bus - Bus to select
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
 static ONEWIRE_STATUS OwBusSelect(ONEWIRE_BUS Bus)
{

    GPIO_SIGNAL Signal;
    static ONEWIRE_BUS ActiveBus = ONEWIRE_BUS_INVALID;    /* Defaulted to invalid */
    ONEWIRE_STATUS Status;
    ONEWIREOPTIONS *pBusOptions;

    do
    {
        Status = ONEWIRE_STATUS_OK;

        if (Bus >= ONEWIRE_BUS_COUNT)
        {
            /* Invalid bus request, return with error */
            Status = ONEWIRE_STATUS_PARAM_ERROR;
            break;
        }

       if (ActiveBus == Bus)
       {
            /* Requested bus already active, do nothing */
            Status = ONEWIRE_STATUS_OK;
            break;
       }

       Signal = OwGetSignalByBusID(ActiveBus);

       if (GPIO_SIGNAL_LAST != Signal)
       {
            if (GPIO_STATUS_OK != L3_GpioCtrlClearSignal(Signal))
            {
                /* Something went wrong, return with error */
               Log(ERR, "OwBusSelect: GPIO Clear Signal Err! Signal = %d", Signal);
                Status = ONEWIRE_STATUS_ERROR;
                break;
            }
       }

        /* Now all bus are disabled, enable required bus */

        Signal = OwGetSignalByBusID(Bus);

        if (GPIO_SIGNAL_LAST != Signal)
        {
            if (GPIO_STATUS_OK != L3_GpioCtrlSetSignal(Signal))
            {
                /* Something went wrong, return with error */
               Log(ERR, "OwBusSelect: GPIO Set Signal Err! Signal = %d", Signal);
                Status = ONEWIRE_STATUS_ERROR;
                break;
            }
            OSTimeDly(1);   /* Allow some switching and settling time */
        }

        /* Load configuration here - Speed, etc. */
        pBusOptions = OwBusConfigGet(Bus);

        if(pBusOptions)
        {
            Status = OwTransportSpeed(pBusOptions->Speed);
        }

        ActiveBus = Bus;        /* Update active bus with new bus ID */
    } while (false);

    if(Status != ONEWIRE_STATUS_OK)
    {
        Log(ERR, "Bus Selection Failed [%d] ",Status);
    }
    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OwSelectBusByDevice(ONEWIRE_DEVICE_ID Device)
 *
 * \brief   Connect the bus associated with device to the One Wire bus master
 *
 * \details Clear all GPIO lines that control bus selection except for the
 *          specified bus, then assert the GPIO line for the specified bus.
 *
 * \note    Bus reset not issued as anyway the transfer/search will issue it
 *
 * \param   Device - Device on the bus
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwSelectBusByDevice(ONEWIRE_DEVICE_ID Device)
{
    ONEWIRE_STATUS Status;
    uint8_t Index;
    ONEWIRE_BUS Bus;

    Bus = ONEWIRE_BUS_INVALID;
    Status = ONEWIRE_STATUS_ERROR;

    /* Find the bus on which the device was found */
    for (Index = 0; Index < ONEWIRE_MAX_DEVICES; Index++)
    {
        if (OwDeviceList[Index].Device == Device)
        {
            /* Found the associated bus */
            Bus = OwDeviceList[Index].Bus;
            break;
        }
    }

    /* Select the bus if valid */
    if (ONEWIRE_BUS_INVALID != Bus)
    {
        if (ONEWIRE_STATUS_OK == OwBusSelect(Bus))
        {
            Status = ONEWIRE_STATUS_OK;
        }
        else
        {
            /* Something went wrong, returning with error */
        }
    }

    return Status;
}

/* ========================================================================== */
/**
 * \fn      void OwTimerStart(void)
 *
 * \brief   Start a timer to monitor 1-Wire slave device
 *
 * \details This function creates the timer. On expiry, a special timer request
 *          is sent to request processor to trigger device scan/monitor
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void OwTimerStart(void)
{
    uint8_t Error;

    OSTmrStart(pTimerDeviceMon, &Error);

    if (OS_ERR_NONE != Error)
    {
        Error = Error;
        Log(ERR, "OwTimerStart: Error is %d",Error);
       /* \todo: Fatal error */
    }
}

 /* ========================================================================== */
/**
 * \fn      void OwTimerHandler (void *pThis, void *pArgs)
 *
 * \brief   Time callback handler
 *
 * \details This timer callback handler posts a special timer request to the
 *          request processor to trigger device scan/monitor
 *
 * \param   pThis - pointer to timer handler
 * \param   pArgs - pointer to arguments
 *
 * \return  void
 *
 * ========================================================================== */
static void OwTimerHandler (void *pThis, void *pArgs)
{
    static OWREQUEST Request;  /* Want to preserve the variable, hence static */
    uint8_t Error;
    uint8_t DummyMessage;

    Request.ReqType = OW_REQUEST_TIMER;
    Request.pMessage = &DummyMessage;   /* Request processor wants a valid message */
    Request.pSema = NULL;               /* This is called in the request processor context, hence NULL */

    Error = OSQPost(pOneWireQ, &Request);

    if (Error != OS_ERR_NONE)
    {
        Log(ERR, "OwTimerHandler: Q Error is %d",Error);
        /* \todo: Fatal error, add exception here */
    }
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OneWireVerifySecret(ONEWIRE_DEVICE_ID Device)
 *
 * \brief   Verify secret and authenticate the 1-wire device
 *
 * \details Selects the device on associated 1-Wire bus to check if it is a valid device.
 * \note    This is an internal interface function.
 *
 * \param   Device - Device to check
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OneWireVerifySecret(ONEWIRE_DEVICE_ID Device)
{
    uint8_t  u8ByteIndex;
    uint8_t u8Crc8;
    static uint8_t DataRead[ONEWIRE_MEMORY_TOTAL_SIZE];
    ONEWIRE_STATUS OwStatus;
    uint8_t u8ManufacturerId[2] = {0x0,0x0};

    OwStatus=ONEWIRE_STATUS_ERROR;

    // Ensure no other task accesses I2C bus from this point on
    L3_I2cClaim();

    do
    {
        /* get the manufacturer id from the Slave using the get status slave command*/
        OwStatus = OWSlaveGetManufacturerId(Device, u8ManufacturerId);
        if (ONEWIRE_STATUS_OK != OwStatus)
        {
            break;
        }
        /*compute master secret at the Master controller using constant data */
        OwStatus = OWComputeMasterSecret(Device,u8ManufacturerId);
        if (ONEWIRE_STATUS_OK != OwStatus)
        {
            break;
        }
        /*  Read and save the Slaves(28E15's) binding data */
        OwStatus = OneWireSaveSlaveBindData(Device,0,DataRead);
        if (ONEWIRE_STATUS_OK != OwStatus)
        {
            break;
        }
        for (u8ByteIndex = 0; u8ByteIndex < ONEWIRE_MEMORY_BANK_SIZE; u8ByteIndex++)
        {
            slaveEEPROMPage[u8ByteIndex] = DataRead[u8ByteIndex];
        }
        /* Write to slave scratchpad with challenge data */
        u8Crc8 = (uint8_t)OSTimeGet();
        for (u8ByteIndex = 0; u8ByteIndex < ONEWIRE_MEMORY_BANK_SIZE; u8ByteIndex++)
        {
            u8Crc8 = DoCRC8(u8Crc8, u8ByteIndex);
            challengeData[u8ByteIndex] = u8Crc8;
        }
        /* write Challenge data the Slave scratchpad  */
        OwStatus = OWSlaveRwScratchpad(Device,CMD_PARAM_SCRATCHPAD_WRITE);
        if (ONEWIRE_STATUS_OK != OwStatus)
        {
            /* Write to scratchpad failed - Set onewire Write Error */
            OwStatus = ONEWIRE_STATUS_WRITE_ERROR;
            break;
        }
        memset(oneWireTempData, 0, ONEWIRE_MEMORY_TEMPDATA_SIZE);
        ///\todo 14-jan-2021 cpk comment out after review and testing - Debug purpose
        OwStatus = OWSlaveRwScratchpad(Device,CMD_PARAM_SCRATCHPAD_READ);
        memset(slaveMAC, 0, ONEWIRE_MEMORY_BANK_SIZE);
        if (ONEWIRE_STATUS_OK != OwStatus)
        {
            /* Read from scratchpad failed - Set onewire Read Error*/
            OwStatus = ONEWIRE_STATUS_READ_ERROR;
            break;
        }
        /* Have the 28E15 compute the MAC from the challenge data and then send the MAC to page0*/
        OwStatus = OWSlaveReadMAC(Device);
        if (ONEWIRE_STATUS_OK != OwStatus)
        {
            break;
        }
        OSTimeDly(MSEC_1);
        /*compute the MAC in master */
        OwStatus = OWComputeAuthMac(Device, &u8ManufacturerId[0]);
        if (ONEWIRE_STATUS_OK != OwStatus)
        {
            break;
        }
        /* compare slave and master MAC */
        for (u8ByteIndex = 0; u8ByteIndex < ONEWIRE_MEMORY_BANK_SIZE; u8ByteIndex++)
        {
            if (slaveMAC[u8ByteIndex] != masterMAC[u8ByteIndex])
            {
                OwStatus = ONEWIRE_STATUS_ERROR;
                break;
            }
            OwStatus = ONEWIRE_STATUS_OK;
        }
    } while (false);

    // Other tasks may access the I2C bus now
    L3_I2cRelease();

    return OwStatus;
}

/* ========================================================================== */
/**
 * \brief   Reads binding data from 1 Wire EEPROM device
 *
 * \details This function reads from specified 1-Wire EEPROM device.
 *
 * \param   Device - 64 Bit 1-Wire ROM ID.
 * \param   Page - EEPROM page number.
 * \param   pBuffer - pointet to Buffer to store data.
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OneWireSaveSlaveBindData(ONEWIRE_DEVICE_ID Device, uint8_t Page, uint8_t *pBuffer)
{
    ONEWIRE_STATUS Status;
    ONEWIREFRAME EepromFrame;
    ONEWIREPACKET* pPacket;
    uint8_t SendBuffer[ONEWIRE_CMD_PACKET_SIZE];
    uint8_t CrcBuffer[ONEWIRE_CRC_BUF_SIZE];
    uint8_t *pBuf;
    uint16_t u16Crc16Val;
    ///\todo FUTURE_ENHANCEMENT 12-jan-2021 cpk can we comeup with a design to pass the command/data/required paraments to a single function and process OwProcessTransferRequest?
    do
    {
        if ((NULL == pBuffer) || (ONEWIRE_EEPROM_NUM_PAGES <= Page))
        {
            Status = ONEWIRE_STATUS_ERROR;
            Log(ERR, "OneWireSaveSlaveBindData: Invalid Parameter");
            break;
        }
        if(ONEWIRE_STATUS_OK != OwProcessDeviceCheckRequest(Device))
        {
            Status = ONEWIRE_STATUS_NO_DEVICE;
            Log(ERR, "OneWireSaveSlaveBindData: Device check failed");
            break;
        }
        OwFrameClear(&EepromFrame);                         /* Clear the Frame before preparing the packets */
        pBuf = SendBuffer;                                  /* Helper pointer */
        *pBuf++ = ONEWIRE_EEPROM_CMD_READ;                       /* Command for Read Memory */
        *pBuf++ = Page;                                     /* Page number to Read the data */
        /* Prepare packet for Command and Packet byte */
        pPacket = &EepromFrame.Packets[0];
        pPacket->pTxData = SendBuffer;
        pPacket->nTxSize = ONEWIRE_CMD_PACKET_SIZE;
        pPacket->pRxData = CrcBuffer;
        pPacket->nRxSize = ONEWIRE_CRC_BUF_SIZE;

        /* Prepare Packet to receive the data (32 Bytes data) */
        pPacket++;
        pPacket->pTxData = NULL;
        pPacket->nTxSize = 0;
        pPacket->pRxData = pBuffer;
        pPacket->nRxSize = ONEWIRE_CMD_MEMORY_PAGE_SIZE;
        /* Prepare Packet to receive the CRC bytes (2 Bytes CRC) */
        pPacket++;
        pPacket->pTxData = NULL;
        pPacket->nTxSize = 0;
        pPacket->pRxData = CrcBuffer;
        pPacket->nRxSize = ONEWIRE_CRC_BUF_SIZE;
        /* Mark end of frame by adding invalid packet */
        pPacket++;
        pPacket->pTxData = NULL;
        pPacket->nTxSize = 0;
        pPacket->pRxData = NULL;
        pPacket->nRxSize = 0;
        EepromFrame.Device = Device;
        EepromFrame.pHandler = &OwTransferHandlerBindingData; /* Registering callback handler to process any interim data/response */
        Status = OwProcessTransferRequest(&EepromFrame);
        /* Computing the CRC on 34 bytes (32 data bytes + 2 CRC bytes), the resultant CRC value should be 0xB001 for a valid read */
        /* Refer to https:www.maximintegrated.com/en/design/technical-documents/app-notes/2/27.html for CRC computation */
        u16Crc16Val = CRC16(0, pBuffer, ONEWIRE_CMD_MEMORY_PAGE_SIZE);
        u16Crc16Val = CRC16(u16Crc16Val, CrcBuffer, ONEWIRE_CRC_BUF_SIZE);

        if (ONEWIRE_EEPROM_CRC_CONST_VAL != u16Crc16Val)
        {
            Status = ONEWIRE_STATUS_ERROR;
            Log(ERR, "OneWireSaveSlaveBindData: CRC check failed on the read data");
        }
    } while(false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Handle interim data received during the Read operation from EEPROM.
 *
 * \param   PacketIndex - Packet index in the frame
 * \param   pRxData - Pointer to data
 *
 * \return  bool - status
 *
 * ========================================================================== */
static bool OwTransferHandlerBindingData(uint8_t PacketIndex, uint8_t *pRxData)
{
    if (PACKETINDEX_1 == PacketIndex)
    {
            /* Allow some delay for read  command */
            OSTimeDly(ONEWIRE_EEPROM_TXFER_WAIT);
    }

    return false;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS OWSlaveRwScratchpad(ONEWIRE_DEVICE_ID Device,uint8_t CmdParam)
 *
 * \brief   Write to the Onewire Scratchpad register
 *
 * \details Used to write challenge data to the specific device scratchpad registers
 * \note    This is an internal interface function.
 *
 * \param   Device - Device to Read/Write
 *          CmdParam Parameter to identify Read or Write to scratchpad
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OWSlaveRwScratchpad(ONEWIRE_DEVICE_ID Device,uint8_t CmdParam)
{
    ONEWIRE_STATUS OwStatus = ONEWIRE_STATUS_ERROR;
    ONEWIREFRAME Frame;
    uint8_t u8TxData[2];
    ONEWIREPACKET* pPacket;
    uint8_t u8CrcBuffer[ONEWIRE_CRC_BUF_SIZE];
    uint8_t u8SlvCrcBuffer[ONEWIRE_CRC_BUF_SIZE];

    OwStatus = ONEWIRE_STATUS_ERROR;
    do
    {
        OwFrameClear(&Frame);
        Frame.Device = Device;
        Frame.pHandler = &OWTransferHandlerScratchpad;     /* Handler to process interim Rx data */
        pPacket = &Frame.Packets[0];    /* Working on first packet */
        u8TxData[0] = ONEWIRE_SLV_CMD_READ_WRITE_SCRATCH;
        u8TxData[1] = CmdParam;
        pPacket->nTxSize =ONEWIRE_CMD_PACKET_SIZE;
        pPacket->pTxData = &u8TxData[0];
        pPacket->nRxSize = ONEWIRE_CRC_BUF_SIZE;
        pPacket->pRxData = &u8CrcBuffer[0];
        if(CmdParam == CMD_PARAM_SCRATCHPAD_WRITE)
        {
            pPacket = &Frame.Packets[1];
            pPacket->nTxSize = ONEWIRE_CMD_MEMORY_PAGE_SIZE;
            pPacket->pTxData = &challengeData[0];
            pPacket->nRxSize = ONEWIRE_CRC_BUF_SIZE;
            pPacket->pRxData = &u8SlvCrcBuffer[0];
        }
        else if(CmdParam == CMD_PARAM_SCRATCHPAD_READ)
        {
            pPacket = &Frame.Packets[1];
            pPacket->nTxSize = 0;
            pPacket->pTxData = NULL;
            pPacket->nRxSize = ONEWIRE_CRC_BUF_SIZE;
            pPacket->pRxData = &oneWireTempData[0];
        }
        else
        {
            /* do nothing*/
        }
        OwStatus = OwProcessTransferRequest(&Frame);
        if( ONEWIRE_STATUS_OK != OwStatus)
        {
            Log(ERR, "OWSlaveRwScratchpad:  Error %x",OwStatus);
            break;
        }
    } while (false);

    return OwStatus;
}

/* ========================================================================== */
/**
 * \brief   Handle interim data received during the transfer
 *
 * \details Specific transaction response handler
 *
 * \param   PacketIndex - Packet index in the frame
 * \param   pRxData - Pointer to data
 *
 * \return  bool - status
 *
 * ========================================================================== */
static bool OWTransferHandlerScratchpad(uint8_t PacketIndex, uint8_t *pRxData)
{
    if (PACKETINDEX_0 == PacketIndex)
    {
            /* delay after the scratchpad command */
            OSTimeDly(MSEC_1);
    }

    if (PACKETINDEX_1 == PacketIndex)
    {
            /* delay for read/write to scratchpad 2*tcsha 4ms. Add 1mS for OSTimeDly uncertainty. */
            OSTimeDly(ONEWIRE_TCSHA_DELAY + 1);
    }

    return false;
}

/* ========================================================================== */
/**
 * \fn      OWSlaveReadMAC(ONEWIRE_DEVICE_ID Device)
 *
 * \brief   Builds packet to send the ONEWIRE_SLV_CMD_READ_PAGE_MAC command to Slave Device
 *
 * \details Command to calculate the Slave MAC. command success and CRC indicate Success of this call
 * \note    Internal Function
 *
 * \param   Device - Device to check
 *          CmdParam Parameter to identify strong pullup Enable/Disable
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OWSlaveReadMAC(ONEWIRE_DEVICE_ID Device)
{
    ONEWIRE_STATUS OwStatus;
    ONEWIREFRAME Frame;
    uint8_t u8TxData[ONEWIRE_CMD_PACKET_SIZE];
    uint8_t u8CrcBuffer[ONEWIRE_CRC_BUF_SIZE];
    uint8_t    u8CmdSuccess;
    uint8_t    u8ByteIndex;
    uint8_t u8SlvCrcBuffer[ONEWIRE_CRC_BUF_SIZE];
    uint16_t crc16;
    ONEWIREPACKET* pPacket;

    OwStatus = ONEWIRE_STATUS_ERROR;
    do
    {
        OwFrameClear(&Frame);
        Frame.Device = Device;
        Frame.pHandler = &OWTransferHandlerSlaveMac;

        pPacket = &Frame.Packets[0];
        u8TxData[0] = ONEWIRE_SLV_CMD_READ_PAGE_MAC;
        u8TxData[1] = 0x0;
        pPacket->nTxSize =ONEWIRE_CMD_PACKET_SIZE;
        pPacket->pTxData = &u8TxData[0];
        pPacket->nRxSize = ONEWIRE_CRC_BUF_SIZE;
        pPacket->pRxData = &u8CrcBuffer[0];

        pPacket = &Frame.Packets[1];
        pPacket->nTxSize = 0;
        pPacket->pTxData = NULL;
        pPacket->nRxSize = 1;
        pPacket->pRxData = &u8CmdSuccess;

        pPacket = &Frame.Packets[2];
        pPacket->nTxSize = 0;
        pPacket->pTxData = NULL;
        pPacket->nRxSize = ONEWIRE_CMD_MEMORY_PAGE_SIZE;
        pPacket->pRxData = slaveMAC;

        pPacket = &Frame.Packets[3];
        pPacket->nTxSize = 0;
        pPacket->pTxData = NULL;
        pPacket->nRxSize = ONEWIRE_CRC_BUF_SIZE;
        pPacket->pRxData = &u8SlvCrcBuffer[0];
        OwStatus = OwProcessTransferRequest(&Frame);

#ifdef UNUSED_CODE       // { 04/15/2021 DAZ -
        // The following is an unwound version of a single OwProcessTransferRequest call. It has been
        // unwound to allow the insertion of a delay (for computation) between the 1st & 2nd receive packets.

        OwStatus = OwTransportSend(&Frame.Device, Frame.Packets[0].pTxData, Frame.Packets[0].nTxSize);    // Send Calculate/read MAC command to 28E15
        OwStatus = OwTransportReceive(Frame.Packets[0].pRxData, Frame.Packets[0].nRxSize);                // Get CRC

        OSTimeDly(ONEWIRE_TCSHA_DELAY + 1);   // Time for processing to complete. Add 1mS for OSTimeDly uncertainty.

        OwStatus = OwTransportReceive(Frame.Packets[1].pRxData, Frame.Packets[1].nRxSize);  // Get command success byte
        OwStatus = OwTransportReceive(Frame.Packets[2].pRxData, Frame.Packets[2].nRxSize);  // Read memory page (Slave MAC)
        OwStatus = OwTransportReceive(Frame.Packets[3].pRxData, Frame.Packets[3].nRxSize);  // Read slave CRC
        OwStatus = OwTransportReceive(NULL, 0);                                             // Reset the one wire bus
#endif                   // }

        /*  Validate the CRC16 sent by the 28E15. (CRC is of the command and page number) */
        crc16 = DoCRC16(0,     (uint16_t)ONEWIRE_SLV_CMD_READ_PAGE_MAC & 0xff);
        crc16 = DoCRC16(crc16, (uint16_t)0x0 & 0xff);
        crc16 = DoCRC16(crc16, (uint16_t)u8CrcBuffer[0] & 0xff);
        crc16 = DoCRC16(crc16, (uint16_t)u8CrcBuffer[1] & 0xff);
        if (ONEWIRE_EEPROM_CRC_CONST_VAL != crc16)
        {
              OwStatus = ONEWIRE_STATUS_ERROR;
              Log(ERR, "OWSlaveReadMAC : CRC Error %x",crc16);
              break;
        }
        /* Read the Command Success Indicator (CS) from the ONEWIRE_SLV_CMD_READ_PAGE_MAC command...Should be 0xAA */
        if (ONEWIRE_CS_SUCCESS != u8CmdSuccess)
        {
            OwStatus = ONEWIRE_STATUS_ERROR;
            Log(ERR, "OWSlaveReadMAC CS Error %x",u8CmdSuccess);
            break;
        }
        /* Validate the CRC16 sent by the 28E15 (CRC is of the data page sent) */
        crc16=0x0;
        for (u8ByteIndex = 0; u8ByteIndex < ONEWIRE_MEMORY_BANK_SIZE; u8ByteIndex++)
        {
            crc16 = DoCRC16(crc16, (uint16_t)slaveMAC[u8ByteIndex] & 0xff);
        }
        crc16 = DoCRC16(crc16, (uint16_t)u8SlvCrcBuffer[0] & 0xff);
        crc16 = DoCRC16(crc16, (uint16_t)u8SlvCrcBuffer[1] & 0xff);
        if (ONEWIRE_EEPROM_CRC_CONST_VAL != crc16)
        {
            OwStatus = ONEWIRE_STATUS_ERROR;
            Log(ERR, "OWSlaveReadMAC : CRC Error %x",crc16);
            break;
        }
        OwStatus = ONEWIRE_STATUS_OK;
    } while (false);

    return OwStatus;
}

/* ========================================================================== */
/**
 * \brief   Handle inter-packet processing during slave read MAC transfer
 *
 * \details The purpose of this function is to insert a delay to account for
 *          computing time between the reading of the CRC confirming the
 *          validity of the ONEWIRE_SLV_CMD_READ_PAGE_MAC command and the
 *          reading of the command success byte confirming the success of
 *          the command.
 *
 * \param   PacketIndex - Packet index in the frame
 * \param   pRxData - Pointer to data
 *
 * \return  bool - status
 *
 * ========================================================================== */
static bool OWTransferHandlerSlaveMac(uint8_t PacketIndex, uint8_t* pRxData)
{
    if (PACKETINDEX_0 == PacketIndex)
    {
            /* delay for reading back the data 4ms Add 1mS for OSTimeDly uncertainty. */
            OSTimeDly(ONEWIRE_TCSHA_DELAY + 1);
    }

    return false;
}

/* ========================================================================== */
/**
 * \fn      OWComputeMasterSecret(ONEWIRE_DEVICE_ID Device)
 *
 * \brief   Builds packet to compute slave secret at the master 1-wire controller
 *
 * \details Command to calculate the  MAC. command at the master controller
 * \note    Internal Function
 *
 * \param   Device - Device to check
 * \param   pManufacturerId - pointer to manufacturer id
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OWComputeMasterSecret(ONEWIRE_DEVICE_ID Device, uint8_t *pManufacturerId)
{
    static const uint8_t constant2465Data[ONEWIRE_MEMORY_BANK_SIZE] = { 0x65,0x27,0xA9,0x6A,0x37,0x9E,0xBE,0x6F,
                                                                        0x90,0xEB,0xA9,0x65,0xBD,0xFC,0x17,0xC9,
                                                                        0x2C,0xA7,0xBC,0xD8,0x7E,0x56,0xAD,0xF4,
                                                                        0x2B,0xEA,0x72,0x1F,0x73,0x37,0xBC,0xF0 };
    I2CDataPacket Packet;
    I2C_STATUS I2cStatus;
    uint8_t u8RegData[2];
    ONEWIRE_STATUS OwStatus;
    ONEWIREFRAME Frame;
    uint8_t u8TxData[2];

    OwStatus = ONEWIRE_STATUS_ERROR;
    do
    {
        OwFrameClear(&Frame);
        memset(oneWireTempData, 0, ONEWIRE_MEMORY_TEMPDATA_SIZE);
        memcpy(&oneWireTempData[ONEWIRE_MEMORY_BANK_SIZE], constant2465Data, ONEWIRE_MEMORY_BANK_SIZE);
        memcpy(&oneWireTempData[ONEWIRE_MEMORY_TOTAL_SIZE], &Device, ONEWIRE_ADDR_LENGTH);
        oneWireTempData[ONEWIRE_MANUFACURER_ID_OFFSET0] = *(pManufacturerId);
        oneWireTempData[ONEWIRE_MANUFACURER_ID_OFFSET1] = *(++pManufacturerId);
        oneWireTempData[ONEWIRE_PAGENO_OFFSET] = 0x0;
        u8RegData[0]=0x0;
        u8RegData[1]= ONEWIRE_MST_COMMAND_REG;
        /* copy constant data to master scratchpad*/
        Packet.Address = ONEWIRE_MASTER_DS2465_ADDRESS;
        Packet.nRegSize = 1;
        Packet.pReg = &u8RegData[0];
        Packet.nDataSize = ONEWIRE_MEMORY_TEMPDATA_SIZE ;
        Packet.pData = oneWireTempData;
        I2cStatus = L3_I2cWrite(&Packet);
        if( I2C_STATUS_SUCCESS != I2cStatus)
        {
            OwStatus = ONEWIRE_STATUS_ERROR;
            Log(ERR, "OWComputeMasterSecret:  Error %x",I2cStatus);
            break;
        }
        /* Allow some delay for the transfer. Add 1mS for OSTimeDly uncertainty. */
        OSTimeDly(ONEWIRE_TCSHA_DELAY + 1);
        /* command master to compute slave secret*/
        Packet.Address = ONEWIRE_MASTER_DS2465_ADDRESS;
        Packet.nRegSize = 1;
        Packet.pReg = &u8RegData[1];
        u8TxData[0] = ONEWIRE_MST_CMD_COMPUTE_S_SECRET;
        u8TxData[1] = CMD_PARAM_COMPUTE_S_SECRET;
        Packet.nDataSize = ONEWIRE_CMD_PACKET_SIZE;
        Packet.pData = &u8TxData[0];
        I2cStatus = L3_I2cWrite(&Packet);
        if( I2C_STATUS_SUCCESS != I2cStatus)
        {
            OwStatus = ONEWIRE_STATUS_ERROR;
            Log(ERR, "OWComputeMasterSecret:  Error %x",I2cStatus);
            break;
        }
        /* Allow some delay for master to settle. Add 1mS for OSTimeDly uncertainty. */
        OSTimeDly(ONEWIRE_TCSHA_DELAY + 1);
        OwStatus = ONEWIRE_STATUS_OK;
    } while (false);

    return OwStatus;
}

/* ========================================================================== */
/**
 * \fn      OWComputeAuthMac(ONEWIRE_DEVICE_ID Device)
 *
 * \brief   Builds packet to compute MAC at the master 1-wire controller
 *
 * \details Command to compute MAC. at the master controller. This is compared with the slave MAC.
 * \note    Internal Function
 *
 * \param   Device - Device to check
 * \param   pManufacturerId - pointer to manufacturer id
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OWComputeAuthMac(ONEWIRE_DEVICE_ID Device, uint8_t *pManufacturerId)
{
    ONEWIRE_STATUS OwStatus;
    uint8_t u8TxData[ONEWIRE_CMD_PACKET_SIZE];
    static uint8_t tempMasterMAC[ONEWIRE_MEMORY_BANK_SIZE];
    uint8_t index;
    I2CDataPacket Packet;
    I2C_STATUS I2cStatus;
    uint8_t u8RegData1[4];

    OwStatus = ONEWIRE_STATUS_ERROR;

    // TestHook to simulate One wire Authentication failure in Test mode
    TM_Hook( HOOK_ONEWIREAUTH, slaveEEPROMPage );

    do
    {
        memset(oneWireTempData, 0, ONEWIRE_MEMORY_TEMPDATA_SIZE);
        memcpy(&oneWireTempData[0], slaveEEPROMPage, ONEWIRE_MEMORY_BANK_SIZE);
        memcpy(&oneWireTempData[ONEWIRE_MEMORY_BANK_SIZE], challengeData, ONEWIRE_MEMORY_BANK_SIZE);
        memcpy(&oneWireTempData[64], &Device, 8);
        oneWireTempData[ONEWIRE_MANUFACURER_ID_OFFSET0] = *(pManufacturerId);
        oneWireTempData[ONEWIRE_MANUFACURER_ID_OFFSET1] = *(++pManufacturerId);
        oneWireTempData[ONEWIRE_PAGENO_OFFSET] = 0x0;

        u8RegData1[0]=0x0;
        u8RegData1[1]=ONEWIRE_MST_COMMAND_REG;
        u8RegData1[2]=0x0;
        u8RegData1[3]=0x0;
        /* copy challenge to master scratchpad*/
        Packet.Address = ONEWIRE_MASTER_DS2465_ADDRESS;
        Packet.nRegSize = 1;
        Packet.pReg = &u8RegData1[0];
        Packet.nDataSize = ONEWIRE_MEMORY_TEMPDATA_SIZE ;
        Packet.pData = oneWireTempData;
        I2cStatus = L3_I2cWrite(&Packet);
        if( I2C_STATUS_SUCCESS != I2cStatus)
        {
            Log(ERR, "OWComputeAuthMac:  Error %x",I2cStatus);
            OwStatus = ONEWIRE_STATUS_ERROR;
            break;
        }
        OSTimeDly(ONEWIRE_TCSHA_DELAY + 1);         // Add 1mS for OSTimeDly uncertainty.
        /* command master to compute Auth MAC */
        Packet.Address = ONEWIRE_MASTER_DS2465_ADDRESS;
        Packet.nRegSize = 1;
        Packet.pReg = &u8RegData1[1];
        u8TxData[0] = ONEWIRE_MST_CMD_COMPUTE_S_AUTHEN_MAC;
        u8TxData[1] = CMD_PARAM_COMPUTE_S_AUTHEN_MAC;

        Packet.nDataSize = ONEWIRE_CMD_PACKET_SIZE;
        Packet.pData = &u8TxData[0];
        I2cStatus = L3_I2cWrite(&Packet);
        if( I2C_STATUS_SUCCESS != I2cStatus)
        {
            Log(ERR, "OWComputeMasterSecret:  Error %x",I2cStatus);
            OwStatus = ONEWIRE_STATUS_ERROR;
            break;
        }
        /*  delay for master to settle - 2*Tcsha. Add 1mS for OSTimeDly uncertainty. */
        OSTimeDly(ONEWIRE_TCSHA_DELAY + 1);

        /* Read result from Master */
        Packet.Address = ONEWIRE_MASTER_DS2465_ADDRESS;
        Packet.nRegSize = 0;
        Packet.pReg = NULL;
        Packet.nDataSize = ONEWIRE_CMD_MEMORY_PAGE_SIZE;
        Packet.pData = &tempMasterMAC[0];
        I2cStatus = L3_I2cBurstRead(&Packet);
        if( I2C_STATUS_SUCCESS != I2cStatus)
        {
            Log(ERR, "computeSAuthMAC:  Error %x",I2cStatus);
            OwStatus = ONEWIRE_STATUS_ERROR;
            break;
        }
        OSTimeDly(ONEWIRE_TCSHA_DELAY + 1);     // Add 1mS for OSTimeDly uncertainty.
        for(index=0;index<ONEWIRE_CMD_MEMORY_PAGE_SIZE;index++)
        {
            masterMAC[index] = tempMasterMAC[index];
        }
        OwStatus = ONEWIRE_STATUS_OK;
    } while (false);

    return OwStatus;
}

/* ========================================================================== */
/**
 * \brief   Handle interim data received during the transfer
 *
 * \details Specific transaction response handler
 *
 * \param   PacketIndex - Packet index in the frame
 * \param   pRxData - Pointer to data
 *
 * \return  bool - status
 *
 * ========================================================================== */
static bool OWTransferHandler(uint8_t PacketIndex, uint8_t* pRxData)
{
    if (PACKETINDEX_0 == PacketIndex)
    {
            /* delay after the status command */
            OSTimeDly(MSEC_1);
    }

    return false;
}

/* ========================================================================== */
/**
 * \brief   TBD
 *
 * \details TBD
 *
 * \param   Device - device id
 * \param   pManufacturerId - Pointer to manufacturer id
 *
 * \return  ONEWIRE_STATUS - status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OWSlaveGetManufacturerId(ONEWIRE_DEVICE_ID Device, uint8_t *pManufacturerId)
{
    ONEWIRE_STATUS OwStatus;
    ONEWIREFRAME Frame;
    uint8_t u8TxData[2];
    uint8_t u8RxData[8];
    uint16_t u16CrcData;
    ONEWIREPACKET* pPacket;

    OwFrameClear(&Frame);
    do
    {
        Frame.Device = Device;
        Frame.pHandler = &OWTransferHandler;
        pPacket = &Frame.Packets[0];    /* Working on first packet */
        u8TxData[0] = ONEWIRE_SLV_CMD_READ_STATUS;
        u8TxData[1] = CMD_PARAM_STATUS_PBI;
        pPacket->nTxSize =ONEWIRE_CMD_PACKET_SIZE;
        pPacket->pTxData = &u8TxData[0];
        pPacket->nRxSize = ONEWIRE_STATUS_CMD_PACKET_SIZE;
        pPacket->pRxData = u8RxData;

        OwStatus = OwProcessTransferRequest(&Frame);
        if( ONEWIRE_STATUS_OK != OwStatus)
        {
            Log(ERR, "OWSlaveGetManufacturerId:    Error %x",OwStatus);
            break;
        }
        u16CrcData = ((u8RxData[0] <<8) | (u8RxData[1]));
        if( ONEWIRE_MID_CRC_CONST_VAL != u16CrcData)
        {
            Log(ERR, "CRC Err:Exp %x Rcvd %x ", ONEWIRE_MID_CRC_CONST_VAL,u16CrcData);
            OwStatus = ONEWIRE_STATUS_ERROR;
            break;
        }
        (*(pManufacturerId))   = u8RxData[STATUS_CMD_MANUFACURER_ID_OFFSET0];
        (*(++pManufacturerId)) = u8RxData[STATUS_CMD_MANUFACURER_ID_OFFSET1];
        OwStatus = ONEWIRE_STATUS_OK;
    } while (false);

    return OwStatus;
}

/* ========================================================================== */
/**
 * \brief   Default packet data
 *
 * \details Initialize all the packets in the frame to default values
 *
 * \param   pFrame - pointer to Frame
 *
 * \return  None
 *
 * ========================================================================== */
static void OwFrameClear(ONEWIREFRAME *pFrame )
{
    ONEWIREPACKET* pPacket;
    uint8_t Count;

   for (Count = 0; Count < ONEWIRE_MAX_PACKETS; Count++)
    {
         pPacket = &pFrame->Packets[Count];
         pPacket->nRxSize = 0;
         pPacket->pRxData = NULL;
         pPacket->nTxSize = 0;
         pPacket->pTxData = NULL;
    }
}

/******************************************************************************/
/*                             Global Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS L3_OneWireInit(void)
 *
 * \brief   Initialize 1-Wire controller
 *
 * \details Setup I2C communication with DS2465, 1-Wire properties.
 *
 * \param   < None >
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS L3_OneWireInit(void)
{
    uint8_t         Index;
    uint8_t         Error;      // OS error return temporary
    ONEWIRE_STATUS  Status;     // Operation status
    static bool     OneWireInitDone = false;        // to mark initialization, avoid multiple inits

    do
    {
        Status = ONEWIRE_STATUS_OK;     /* Default Status */

        if (true == OneWireInitDone)
        {
            /* Repeated initialization attempt, do nothing */
            break;
        }

        /* Initialize the device list with default(invalid) values*/
        for (Index=0; Index < ONEWIRE_MAX_DEVICES; Index++)
        {
            OwDeviceList[Index].Device = ONEWIRE_DEVICE_ID_INVALID;
            OwDeviceList[Index].Bus = ONEWIRE_BUS_INVALID;
            OwDeviceList[Index].Family = ONEWIRE_DEVICE_FAMILY_LAST;
        }

        /* 1-Wire transport layer initialization */
        if (ONEWIRE_STATUS_OK != OwTransportInit())
        {
            /* 1-Wire protocol initialization failed, return error */
            Log(ERR, "L3_OneWireInit: 1-Wire protocol initialization failed ");
            Status = ONEWIRE_STATUS_ERROR;
            break;
        }

        /* Activate disabled state */
        L3_OneWireEnable(false);

        /* Default initialization for each bus */
        for (Index=0; Index < ONEWIRE_BUS_COUNT; Index++)
        {
           OwDefaultBusInfo(&OwBusGroup[Index], (ONEWIRE_BUS)Index);

           L3_GpioCtrlClearSignal(OwGetSignalByBusID((ONEWIRE_BUS)Index));
        }

        Error = SigTaskCreate( &OwTask,
                                    NULL,
                                    &OWTaskStack[0],
                                    TASK_PRIORITY_L3_ONEWIRE,
                                    ONEWIRE_TASK_STACK,
                                    "OneWire");

        if (Error != OS_ERR_NONE)
        {
            /* Couldn't create task, exit with error */
            Status = ONEWIRE_STATUS_ERROR;
            Log(ERR, "L3_OneWireInit: OwTask Create Error - %d", Error);
            break;
        }

        pMutex_OneWire = SigMutexCreate("L3-I2C", &Error);

        if (NULL == pMutex_OneWire)
        {
          /* Couldn't create mutex, exit with error */
          Status = ONEWIRE_STATUS_ERROR;
          Log(ERR, "L3_OneWireInit: Onewire Mutex Create Error - %d", Error);
          break;
        }

        /* Create timer used for scan and device connection checks */
        pTimerDeviceMon = SigTimerCreate(OW_TIMER_TICKS, 0, OS_TMR_OPT_ONE_SHOT, &OwTimerHandler, "OwTimer", &Error);

        if (NULL == pTimerDeviceMon)
        {
            /* Couldn't create Timer, exit with error */
            Status = ONEWIRE_STATUS_ERROR;
            Log(ERR, "L3_OneWireInit: Onewire Timer Create Error - %d", Error);
            break;
        }

        /* Mark init done to avoid repeated initialization */
        OneWireInitDone = true;

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS L3_OneWireBusConfig(ONEWIREOPTIONS *pOptions)
 *
 * \brief   Configure a 1-Wire bus
 *
 * \details Configures a 1-Wire bus with the options specified by the caller
 *
 * \param   pOptions - pointer to Bus configuration options
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS L3_OneWireBusConfig(ONEWIREOPTIONS *pOptions)
{
    ONEWIRE_STATUS Status;

    Status = ONEWIRE_STATUS_PARAM_ERROR; /* Default error */

    if (NULL != pOptions)
    {
        /* Place the request into the processor Q */
        Status = OwPostRequest(pOptions, OW_REQUEST_CONFIG);
    }

    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS L3_OneWireTransfer(ONEWIREFRAME *pFrame)
 *
 * \brief   Transfer 1-Wire frame
 *
 * \details Forwards the 1-Wire frame to the request process for processing.
 *          Waits on a semaphore until the request is processed and response
 *          is available or a timeout occurred.
 *
 * \param   pFrame - pointer to 1-Wire transfer frame, may contain multiple packet segments
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS L3_OneWireTransfer(ONEWIREFRAME *pFrame)
{
    ONEWIRE_STATUS Status;

    Status = ONEWIRE_STATUS_PARAM_ERROR; /* Default error */

    if (NULL != pFrame)
    {
        Status = OwPostRequest(pFrame, OW_REQUEST_TRANSFER);
    }

    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS L3_OneWireDeviceGetByFamily(ONEWIRE_DEVICE_FAMILY Family, ONEWIRE_DEVICE_ID *pList, uint8_t *pCount)
 *
 * \brief   Device list query
 *
 * \details Post a request to request processor to get list of 1-Wire devices
 *          from a specific device family
 *
 * \note    Caller is responsible for allocating memory for the device as
 *          specified by the count. The count pointed by 'pCount' is updated with
 *          the available number of devices from the family.
 *
 * \param   Family - Device family
 * \param   pList - pointer to List for copying devices
 * \param   pCount - pointer to Device count requested/found
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS L3_OneWireDeviceGetByFamily(ONEWIRE_DEVICE_FAMILY Family, ONEWIRE_DEVICE_ID *pList, uint8_t *pCount)
{
    ONEWIRE_STATUS Status;
    OWDEVICEQUERYMSG Message;

    do
    {
        Status = ONEWIRE_STATUS_PARAM_ERROR;

        if (NULL == pList)
        {
            /* Invalid device list pointer */
            break;
        }

        if (NULL == pCount)
        {
            /* Invalid count pointer */
            break;
        }

        /* Family validation not required, Prepare message request */

        Message.Info.Family = Family;       /* Search criteria, bus will be ignored */
        Message.pList = pList;              /* This is where the list will be update */
        Message.Count = *pCount;            /* Pass the device request count */

        /* Place the request into the processor Q */
        Status = OwPostRequest(&Message, OW_REQUEST_LISTBY_FAMILY);
        *pCount = Message.Count;    /* return device count */

    } while(false);

    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS L3_OneWireDeviceGetByBus(ONEWIRE_BUS Bus, ONEWIRE_DEVICE_ID *pList, uint8_t *pCount)
 *
 * \brief   Device list query
 *
 * \details Post a request to request processor to get list of 1-Wire devices
 *          discovered on specified bus
 *
 * \note    Caller is responsible for allocating memory for the device as
 *          specified by the count. The count is updated with the available
 *          number of devices from the bus.
 *
 * \param   Bus - 1-Wire Bus
 * \param   pList - pointer to List for copying devices
 * \param   pCount - pointer to Device count requested
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS L3_OneWireDeviceGetByBus(ONEWIRE_BUS Bus, ONEWIRE_DEVICE_ID *pList, uint8_t *pCount)
{
    ONEWIRE_STATUS Status;
    OWDEVICEQUERYMSG Message;

    do
    {
        Status = ONEWIRE_STATUS_PARAM_ERROR;

        if (NULL == pList)
        {
           /* Invalid device list pointer */
           break;
        }

        if (NULL == pCount)
        {
           /* Invalid count pointer */
           break;
        }

        if(Bus >= ONEWIRE_BUS_COUNT)
        {
            /* Invalid bus ID */
            break;
        }

        /* Prepare message request */

        Message.Info.Bus = Bus;       /* Search criteria, Family will be ignored */
        Message.pList = pList;        /* This is where the list will be updated */
        Message.Count = *pCount;      /* Pass the expected device request count */

        /* Place the request into the processor Q */
        Status = OwPostRequest(&Message, OW_REQUEST_LISTBY_BUS);

        *pCount = Message.Count;    /* return device count */

    }while(false);

    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS L3_OneWireDeviceCheck(ONEWIRE_DEVICE_ID Device)
 *
 * \brief   Check if devices is still connected
 *
 * \details Selects the device on associated 1-Wire bus to check if it is responding.
 * \note    This is an external interface function called by user of this module.
 *
 * \param   Device - Device to check
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS L3_OneWireDeviceCheck(ONEWIRE_DEVICE_ID Device)
{
    ONEWIRE_STATUS Status;

    Status = OwPostRequest(&Device, OW_REQUEST_CHECK);

    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS L3_OneWireEnable(bool Enable)
 *
 * \brief   Control 1-Wire bus state
 *
 * \details Enable/disable 1-Wire bus.
 *
 * \note    Currently only 1-Wire master is moved to power down state.
 *
 * \param   Enable - New state. If true = enabled, false = disabled
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS L3_OneWireEnable(bool Enable)
{
    ONEWIRE_STATUS Status;

    Status = OwTransportEnable(Enable);

    if (ONEWIRE_STATUS_OK == Status)
    {
        OnewireEnabled = Enable;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS L3_OneWireAuthenticate(ONEWIRE_DEVICE_ID Device)
 *
 * \brief   Authenticate the 1-Wire Slave with the 1-Wire Master controller
 *
 * \details Selects the device on associated 1-Wire bus to Authenticate the device
 * \note    This is an external interface function called by user of this module.
 *
 * \param   Device - Device to check
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */

ONEWIRE_STATUS L3_OneWireAuthenticate(ONEWIRE_DEVICE_ID Device)
{
    ONEWIRE_STATUS Status;

    Status = OwPostRequest(&Device, OW_REQUEST_AUTHENTICATE);

    return Status;
}

/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS L3_CheckConnectorBus(void)
 *
 * \brief   Checks device list has the Connectors Bus
 *
 * \details Checks entire list for connector bus devices 
 *
 * \param   < None >
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS L3_CheckConnectorBus(void)
{
    ONEWIRE_STATUS Status;
    OWDEVICEINFO *pDevInfo;
    uint8_t DeviceIndex;
    pDevInfo = OwDeviceList;  /* Start with the base of device list */
    
    Status = ONEWIRE_STATUS_OK;
    for (DeviceIndex = 0; DeviceIndex < ONEWIRE_MAX_DEVICES; DeviceIndex++)
    {
        if (ONEWIRE_BUS_CONNECTORS == pDevInfo->Bus)
        {
            Status = ONEWIRE_STATUS_ERROR;
            break;
        }
        pDevInfo++;        
    }
    return Status;
}


/* ========================================================================== */
/**
 * \fn      ONEWIRE_STATUS L3_CheckClamshellBus(void)
 *
 * \brief   Checks device list has the Clamshell Bus
 *
 * \details Checks entire list for clamshell bus devices 
 *
 * \param   < None >
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS L3_CheckClamshellBus(void)
{
    ONEWIRE_STATUS Status;
    OWDEVICEINFO *pDevInfo;
    uint8_t DeviceIndex;
    pDevInfo = OwDeviceList;  /* Start with the base of device list */
    
    Status = ONEWIRE_STATUS_OK;
    for (DeviceIndex = 0; DeviceIndex < ONEWIRE_MAX_DEVICES; DeviceIndex++)
    {
        if (ONEWIRE_BUS_CLAMSHELL == pDevInfo->Bus)
        {
            Status = ONEWIRE_STATUS_ERROR;
            break;
        }
        pDevInfo++;        
    }
    return Status;
}



/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

