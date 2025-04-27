#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_CommunicationsManager
 * \{
 *
 * \brief   Communication Manager Functions
 *
 * \details Signia Handle has various communication interfaces such as USB, WLAN,
 *          Infrared(IR) via UART to communicate with external software
 *          applications or sub-systems. The Communication Manager abstracts
 *          different communication interfaces and provides a unified interface
 *          for applications to use
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Signia_CommManager.h"
#include "L3_Usb.h"
#include "L3_Wlan.h"
#include "L3_Uart0Proxy.h"
#include "CirBuff.h"
#include "ActiveObject.h"
#include "L4_ConsoleManager.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER                (LOG_GROUP_COMM)     ///< Log Group Identifier
#define COMM_MGR_TASK_STACK                 (512u)              ///< Task stack size
#define COMM_POLL_PERIOD                    (MSEC_100)
#define TWO_MIN_TIMEOUT                     (MIN_2)              ///< Timeout count for 2 Min
#define FIVE_SEC_COUNT                      (50u)                ///< Timeout count for 5 sec
#define MAX_COUNT                           (65535u)             ///< Maximum 16byte value
#define VALUE_1                             (1u)
#define MAX_RETRY                           (10u)
#define DEFAULT_VALUE                       (0xFF)
#define WIFI_CHECK_TIMEOUT                  (SEC_10)
/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
OS_STK CommMgrTaskStack[COMM_MGR_TASK_STACK+MEMORY_FENCE_SIZE_DWORDS];        /* Communication Manager task stack */
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef COMM_MGR_STATUS (*COMM_INTERFACE)(void);                  /*! Connection interface function */
typedef bool (*INTERFACE_TXFR)(uint8_t *pData, uint16_t *pCount); /*! Interface specific send/receive */

typedef enum                                           /*! Communication interface types */
{
    COMM_WLAN,                                          /*! WLAN connection */
    COMM_USB,                                           /*! USB connection */
    COMM_IR,                                            /*! IR connection */
    COMM_UART0,                                         /*! Uart0 connection */
    COMM_COUNT
} COMM_TYPE;

typedef struct                                        /*! Communication interface object returned when a connection is open */
{
    CIR_BUFF *pRxCirBuff;                               /*! Rx data circular buffer */
    uint8_t  *pRxDataBuffer;                            /*! Rx data buffer */
    CIR_BUFF *pTxCirBuff;                               /*! Tx data circular buffer */
    uint8_t  *pTxDataBuffer;                            /*! Tx data buffer */
    COMM_HANDLER pCommHandler;                          /*! Connection event handler */
    COMM_IF *pCommIf;                                   /*! Connection interface */
    COMM_INTERFACE pInit;                               /*! Initialize connection interface */
    INTERFACE_TXFR pRead;                               /*! Interface specific read */
    INTERFACE_TXFR pWrite;                              /*! Interface specific write */
    COMM_INTERFACE pClose;                              /*! Close connection interface */
    bool IsConnected;                                   /*! Connection status */
} COMM_MGR_INFO;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
#pragma location=".sram"                                // Store buffer in external RAM
uint8_t  UsbRxDataBuffer[MAX_DATA_BYTES + MEMORY_FENCE_SIZE_BYTES];        // Rx data buffer
#pragma location=".sram"                                // Store buffer in external RAM
uint8_t  UsbTxDataBuffer[MAX_DATA_BYTES + MEMORY_FENCE_SIZE_BYTES];        // Tx data buffer
#pragma location=".sram"                                // Store buffer in external RAM
uint8_t  WlanRxDataBuffer[MAX_DATA_BYTES + MEMORY_FENCE_SIZE_BYTES];       // Rx data buffer
#pragma location=".sram"                                // Store buffer in external RAM
uint8_t  WlanTxDataBuffer[MAX_DATA_BYTES + MEMORY_FENCE_SIZE_BYTES];       // Tx data buffer
#pragma location=".sram"                                // Store buffer in external RAM
uint8_t  Uart0RxDataBuffer[MAX_DATA_BYTES + MEMORY_FENCE_SIZE_BYTES];      // Rx data buffer
#pragma location=".sram"                                // Store buffer in external RAM
uint8_t  Uart0TxDataBuffer[MAX_DATA_BYTES + MEMORY_FENCE_SIZE_BYTES];      // Tx data buffer

/* The function prototypes are declared here as the definition of COMM_MGR_INFO requires it */
static COMM_MGR_STATUS UsbInit(void);
static COMM_MGR_STATUS UsbSend(uint8_t *pData, uint16_t *pSize);
static COMM_MGR_STATUS UsbReceive(uint8_t *pData, uint16_t *pSize);
static COMM_MGR_STATUS UsbPeek(uint16_t *pCount);
static bool UsbRx(uint8_t *pData, uint16_t *pCount);
static bool UsbTx(uint8_t *pData, uint16_t *pCount);

static COMM_MGR_STATUS WlanInit(void);
static COMM_MGR_STATUS WlanSend(uint8_t *pData, uint16_t *pSize);
static COMM_MGR_STATUS WlanReceive(uint8_t *pData, uint16_t *pSize);
static COMM_MGR_STATUS WlanPeek(uint16_t *pCount);
static bool WlanRx(uint8_t *pData, uint16_t *pCount);
static bool WlanTx(uint8_t *pData, uint16_t *pCount);
static COMM_MGR_STATUS WlanClose(void);

static COMM_MGR_STATUS Uart0Init(void);
static COMM_MGR_STATUS Uart0Send(uint8_t *pData, uint16_t *pSize);
static COMM_MGR_STATUS Uart0Receive(uint8_t *pData, uint16_t *pSize);
static COMM_MGR_STATUS Uart0Peek(uint16_t *pCount);
static bool Uart0Rx(uint8_t *pData, uint16_t *pCount);
static bool Uart0Tx(uint8_t *pData, uint16_t *pCount);

static OS_EVENT *pMutexCommMgr = NULL;                          // Communication manager mutex
static CIR_BUFF UsbRxCirBuff;                                   // Rx data circular buffer
static CIR_BUFF UsbTxCirBuff;                                   // Tx data circular buffer

static CIR_BUFF WlanRxCirBuff;                                  // Rx data circular buffer
static CIR_BUFF WlanTxCirBuff;                                  // Tx data circular buffer

static CIR_BUFF Uart0RxCirBuff;                                 // Rx data circular buffer
static CIR_BUFF Uart0TxCirBuff;                                 // Tx data circular buffer

static COMM_IF Connection[COMM_COUNT] =
{
    { WlanSend,  WlanReceive,  WlanPeek  },                     // WLAN communication object interface
    { UsbSend,   UsbReceive,   UsbPeek   },                     // USB communication object interface
    { NULL,      NULL,         NULL      },                     // IR communication object interface
    { Uart0Send, Uart0Receive, Uart0Peek }                      // Uart0 communication object interface
};

static COMM_MGR_INFO CommMgrInfo[COMM_COUNT] =
{
    /* WLAN definitions */
    { &WlanRxCirBuff, WlanRxDataBuffer, &WlanTxCirBuff, WlanTxDataBuffer, NULL, &Connection[COMM_WLAN], &WlanInit, &WlanRx, &WlanTx, &WlanClose, false },
    /* USB definitions */
    { &UsbRxCirBuff,  UsbRxDataBuffer,  &UsbTxCirBuff,  UsbTxDataBuffer,  NULL, &Connection[COMM_USB],  &UsbInit,  &UsbRx,  &UsbTx,  NULL,       false },
    /* IR definitions */
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false },
    /* Uart0 definitions */
    { &Uart0RxCirBuff, Uart0RxDataBuffer, &Uart0TxCirBuff, Uart0TxDataBuffer, NULL, &Connection[COMM_UART0], &Uart0Init, &Uart0Rx, &Uart0Tx, NULL, false }
};

static  COMM_TYPE ActiveConn = COMM_COUNT;              // Active connection - Intentionally initializing to invalid channel
static  bool USBActivity = false;                       // USB Activity Check flag - intentionally initializing to false
static COMM_IF  *pActiveInterface;                      // Active Interface pointer.

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void CommMgrTask(void *pArg);
static void UsbCallback(USB_EVENT Event);
static void Dispatcher(void);
static void InterfaceInit(void);
static COMM_MGR_STATUS CommSend(COMM_CONN Type, uint8_t *pData, uint16_t *pSize);
static COMM_MGR_STATUS CommReceive(COMM_CONN Type, uint8_t *pData, uint16_t *pSize);
static COMM_MGR_STATUS CommPeek(COMM_CONN Type, uint16_t *pCount);
static void UpdateActiveConnection(void);
static void ClearUSBFlags(void);
static void WlanCallback(WLAN_EVENT Event);

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Communication Manager task
 *
 * \details The task performs communication management functions.
 *
 * \param   pArg - Task arguments
 *
 * \return  None
 *
 * ========================================================================== */
static void CommMgrTask(void *pArg)
{
    uint32_t StartTime;
    uint32_t ElapsedTime;
    uint32_t WifiInitTime;
    uint32_t WifiCheckTime;

    WifiInitTime = SigTime();

    while (true)
    {
        /* Call the dispatchers for all the interfaces */
        Dispatcher();
        do
        {
            WifiCheckTime = SigTime();
            /* check for Wifi connection every 10 seconds */
            /// \\todo 03/18/2022 CPK Check below only when enabled in KVF file. Currently not available
            if ((WifiCheckTime - WifiInitTime) > WIFI_CHECK_TIMEOUT)
            {
                L3_WlanCheckConnection();
                WifiInitTime = SigTime();
            }

            /* No USB Connected */
            BREAK_IF(L4_USBConnectionStatus() == false)

            /* Check Password Command is received and Any activity found on USB port */
            if ((GetUSBPortMode() == false) || (USBActivity))
            {
                /* Reset the start time since new activity is found on USB Port */
                StartTime = SigTime();
                /* Clear the flag for next iteration */
                USBActivity = false;
            }

            /* Calculate the elapsed time */
            ElapsedTime = SigTime() - StartTime;

            /* On 2 Min Inactivity */
            if (TWO_MIN_TIMEOUT <= ElapsedTime)
            {
                /* Logged out of secure mode due to Inactivity on USB Port */
                ClearUSBFlags();
                Log(DBG, "Secure Mode Inactive - Need Password");
            }
        } while (false);

        /// \todo 02/17/2022 SE - Reduced time out from 100ms to 1ms for MCP. Instead of delay, can have some  protection using semaphore
        /* Wait for 1 msec */
        OSTimeDly(MSEC_1);
    }
}

/* ========================================================================== */
/**
 * \brief   Common dispatcher for the interfaces
 *
 * \details The function invokes interface specific read/write functions.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void Dispatcher(void)
{
    uint8_t  Index;                             // Contains iteration index
    uint8_t  OsError;                           // contains the OS error status
    uint16_t DataLen;                           // Data Length
    bool     IsTransferOk;                      // Transfer Ok flag
    static uint8_t  CommData[MAX_DATA_BYTES];   // Data in buffer

    uint16_t ReceivedCount;                     // Received count
    uint16_t ActDataLen;                        // Actual data Length
    uint16_t CircBufspace;                      // Space remaining in circular buffer

    ReceivedCount = MAX_DATA_BYTES;
    DataLen = 0;

    /* Call the dispatchers for all the interfaces */
    for (Index = COMM_WLAN; Index < COMM_COUNT; Index++)
    {
        IsTransferOk = true;
        ReceivedCount = MAX_DATA_BYTES;
        /* Proceed only if connected */
        if (CommMgrInfo[Index].IsConnected)
        {
            /* Read data from specific interface */
            IsTransferOk = CommMgrInfo[Index].pRead(CommData, &ReceivedCount);

            /* If new data available, then store in respective queue and notify application */
            CircBufspace = CirBuffFreeSpace(CommMgrInfo[Index].pRxCirBuff);

            if ((!IsTransferOk) && (ReceivedCount > 0) && (ReceivedCount <= CircBufspace))
            {
                /*  Mutex lock */
                OSMutexPend(pMutexCommMgr, OS_WAIT_FOREVER, &OsError);

                CirBuffPush(CommMgrInfo[Index].pRxCirBuff, CommData, ReceivedCount);

                /* Mutex release */
                OSMutexPost(pMutexCommMgr);

                if (NULL != CommMgrInfo[Index].pCommHandler)
                {
                    /* Notify application */
                    CommMgrInfo[Index].pCommHandler(COMM_MGR_EVENT_NEWDATA);
                }

                /* Set the USBActivity flag to true to indicate the Communication ongoing */
                USBActivity = true;
            }

            IsTransferOk = true;
            /* Check if data is available in the transmit buffer */
            DataLen = CirBuffCount(CommMgrInfo[Index].pTxCirBuff);
            if (DataLen > 0)
            {
                /* Read data from the circular buffer */
                ActDataLen = CirBuffPeek(CommMgrInfo[Index].pTxCirBuff, CommData, DataLen);

                /* Send data to specific interface */
                IsTransferOk = CommMgrInfo[Index].pWrite(CommData, &ActDataLen);

                if (!IsTransferOk)
                {
                    /*  Mutex lock */
                    OSMutexPend(pMutexCommMgr, OS_WAIT_FOREVER, &OsError);

                    /* Remove data from the circular buffer */
                    CirBuffPop(CommMgrInfo[Index].pTxCirBuff, ActDataLen);

                    /* Mutex release */
                    OSMutexPost(pMutexCommMgr);
                }
            }
        }
    }
}

/* ========================================================================== */
/**
 * \brief   USB Receive
 *
 * \details The function invokes USB specific receive function
 *
 * \note   No NULL check added for pCount. Its taken care before invoking this
 *         function.
 *
 * \param   pData -  Pointer to buffer
 * \param   pCount -  pointer to Bytes received
 *
 * \return  bool  - Receive status - false if no error, true otherwise
 *
 * ========================================================================== */
static bool UsbRx(uint8_t *pData, uint16_t *pCount)
{
    bool TransferStatus;    // Transfer status

    TransferStatus = true;  // Default to error

    if (USB_STATUS_OK == L3_UsbReceive(pData,  MAX_DATA_BYTES, MSEC_100, pCount))
    {
        TransferStatus = false;
        ActiveConn = COMM_USB;
    }

    return TransferStatus;
}

/* ========================================================================== */
/**
 * \brief   USB Transmit
 *
 * \details The function invokes USB specific transmit function
 *
 * \note   No NULL check added for pCount. Its taken care before invoking this
 *         function.
 *
 * \param   pData -  Pointer to buffer
 * \param   pCount -  pointer to Bytes to be transmitted
 *
 * \return  bool - Transmit status - false if no error, true otherwise
 *
 * ========================================================================== */
static bool UsbTx(uint8_t *pData, uint16_t *pCount)
{
    uint16_t SentCount;                         // Sent count
    bool TransferStatus;                        // Transfer status

    SentCount = 0;
    TransferStatus = true;      // Default to error condition

    if (USB_STATUS_OK == L3_UsbSend(pData, *pCount, MSEC_100, &SentCount))
    {
        TransferStatus = false;
        ActiveConn = COMM_USB;
        *pCount = SentCount;
    }

    return TransferStatus;
}

/* ========================================================================== */
/**
 * \brief   USB callback function
 *
 * \details USB callback with event
 *
 * \param   Event - USB event
 *
 * \return  None
 *
 * ========================================================================== */
static void UsbCallback(USB_EVENT Event)
{
    COMM_MGR_EVENT  CommEvent;
    QEVENT_USB      *pUsbEvent;
    SIGNAL          UsbEvent;

    UsbEvent = LAST_SIG;

    Log(DBG, "USB Callback: %s", &USBEvent[Event][0]);

    switch (Event)
    {
        case USB_EVENT_CONNECT:
            CommMgrInfo[COMM_USB].IsConnected = true;
            CommEvent = COMM_MGR_EVENT_CONNECT;
            UsbEvent = P_USB_CONNECTED_SIG;
            /*The HANDLE software shall log connection attempts to the USB interface in the SECURITY_LOG File.*/
            SecurityLog("USB Connected to Handle");
            break;
        case USB_EVENT_DISCCONNECT:
            CommMgrInfo[COMM_USB].IsConnected = false;
            CommEvent = COMM_MGR_EVENT_DISCONNECT;
            UsbEvent = P_USB_REMOVED_SIG;
            /*The HANDLE software shall log disconnection attempts to the USB interface in the SECURITY_LOG File.*/
            SecurityLog("USB Disconnected from Handle");
            /* To clear the USB mode flags */
            ClearUSBFlags();
            break;
        case USB_EVENT_SUSPEND:
            CommEvent = COMM_MGR_EVENT_SUSPEND;
            UsbEvent = P_USB_REMOVED_SIG;
            /* Clear the USB mode flags */
            ClearUSBFlags();
            break;
        default:
            CommEvent = COMM_MGR_EVENT_ERROR;
            break;
    }

    if (NULL != CommMgrInfo[COMM_USB].pCommHandler)
    {
        CommMgrInfo[COMM_USB].pCommHandler(CommEvent);
    }

    /* publish valid signals */
    if (P_LAST_SIG > UsbEvent)
    {
        pUsbEvent = AO_EvtNew(UsbEvent, sizeof(QEVENT_USB));
        if (pUsbEvent)
        {
            AO_Publish(pUsbEvent, NULL);
        }
        else
        {
            Log(DBG, "UsbCallback :Signia Event allocation error");
        }
    }

    UpdateActiveConnection();

    /* Update the USB Active interface in Console manager */
    /* This is required since the WLAN/USB may be initialzed even after console manager initialization */
    pActiveInterface = L4_CommManagerConnOpen(COMM_CONN_ACTIVE, &CommEventHandler);
    if (NULL != pActiveInterface)
    {
        L4_ConsoleMgrUpdateInterface(pActiveInterface);
    }
}

/* ========================================================================== */
/**
 * \brief   Generic send function
 *
 * \details Interface function to send data
 *
 * \param   Type - Connection type
 * \param   pData - pointer to Data to be transferred
 * \param   pSize - pointer to Data size, will be updated as per actual transferred data
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_INVALID_PARAM - Invalid parameter
 * \retval  COMM_MGR_STATUS_Q_FULL - Queue is full
 * \retval  COMM_MGR_STATUS_DISCONNECTED - Interface disconnected
 * \retval  COMM_MGR_STATUS_OK - Success
 *
 * ========================================================================== */
static COMM_MGR_STATUS CommSend(COMM_CONN Type, uint8_t *pData, uint16_t *pSize)
{
    COMM_MGR_STATUS Status;                     /* Contains status */
    uint8_t OsError;                            /* contains the OS error status */
    COMM_TYPE CommType;                         /* Contains communication interface type */

    Status = COMM_MGR_STATUS_OK;
    CommType = COMM_COUNT;

    CommType = (COMM_CONN_ACTIVE == Type) ? ActiveConn : (COMM_TYPE)Type;

    do
    {
        if (!CommMgrInfo[CommType].IsConnected)
        {
            Status = COMM_MGR_STATUS_DISCONNECTED;
            break;
        }

        if ((NULL == pData) || (NULL == pSize) || (0 == *pSize) || (*pSize > MAX_DATA_BYTES))
        {
            Status = COMM_MGR_STATUS_INVALID_PARAM;
            break;
        }

        /* Check if space is available in the transmit buffer */
        if (*pSize > CirBuffFreeSpace(CommMgrInfo[CommType].pTxCirBuff))
        {
            Status = COMM_MGR_STATUS_Q_FULL;
            break;
        }

        /*  Mutex lock */
        OSMutexPend(pMutexCommMgr, OS_WAIT_FOREVER, &OsError);

        /* If space available in circular buffer , then store */
        *pSize = CirBuffPush(CommMgrInfo[CommType].pTxCirBuff, pData, *pSize);

        /* Mutex release */
        OSMutexPost(pMutexCommMgr);

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Generic Receive function
 *
 * \details Interface function to receive data
 *
 * \param   Type - Connection type
 * \param   pData - pointer to Data to be received
 * \param   pSize - pointer to Data size, will be updated as per actual transferred data
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_INVALID_PARAM - Invalid parameter
 * \retval  COMM_MGR_STATUS_Q_EMPTY - Queue is empty
 * \retval  COMM_MGR_STATUS_DISCONNECTED - Interface disconnected
 * \retval  COMM_MGR_STATUS_OK - Success
 *
 * ========================================================================== */
static COMM_MGR_STATUS CommReceive(COMM_CONN Type, uint8_t *pData, uint16_t *pSize)
{
    COMM_MGR_STATUS Status;                     /* Contains status */
    uint16_t BufferLen;                         /* Buffer Length */
    uint8_t OsError;                            /* contains the OS error status */
    COMM_TYPE CommType;                         /* Contains communication interface type */

    Status = COMM_MGR_STATUS_OK;
    CommType = COMM_COUNT;

    CommType = (COMM_CONN_ACTIVE == Type) ? ActiveConn : (COMM_TYPE)Type;

    do
    {
        if (!CommMgrInfo[CommType].IsConnected)
        {
            Status = COMM_MGR_STATUS_DISCONNECTED;
            break;
        }

        if ((NULL == pData) || (NULL == pSize) || (0 == *pSize))
        {
            Status = COMM_MGR_STATUS_INVALID_PARAM;
            break;
        }

        /* Check if data is available in the receive buffer */
        BufferLen = CirBuffCount(CommMgrInfo[CommType].pRxCirBuff);
        if (0 == BufferLen)
        {
            Status = COMM_MGR_STATUS_Q_EMPTY;
            break;
        }
        if ( BufferLen > *pSize )
        {
            BufferLen = *pSize;
        }
        /*  Mutex lock */
        OSMutexPend(pMutexCommMgr, OS_WAIT_FOREVER, &OsError);
        /* Read data from the circular buffer */
        *pSize = CirBuffPeek(CommMgrInfo[CommType].pRxCirBuff, pData, BufferLen);
        /* Remove data from the circular buffer */
        CirBuffPop(CommMgrInfo[CommType].pRxCirBuff, *pSize);

        /* Mutex release */
        OSMutexPost(pMutexCommMgr);

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Generic Peek function
 *
 * \details Interface function to check if new any data is available to read
 *
 * \param   Type - Connection type
 * \param   pCount - pointer to Size of unread data
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_INVALID_PARAM - Invalid parameter
 * \retval  COMM_MGR_STATUS_DISCONNECTED - Interface disconnected
 * \retval  COMM_MGR_STATUS_OK - Success
 *
 * ========================================================================== */
static COMM_MGR_STATUS CommPeek(COMM_CONN Type, uint16_t *pCount)
{
    COMM_MGR_STATUS Status;                    /* Contains status */
    COMM_TYPE CommType;                        /* Contains communication interface type */

    Status = COMM_MGR_STATUS_OK;
    CommType = COMM_COUNT;

    CommType = (COMM_CONN_ACTIVE == Type) ? ActiveConn : (COMM_TYPE)Type;

    do
    {
        if (!CommMgrInfo[CommType].IsConnected)
        {
            Status = COMM_MGR_STATUS_DISCONNECTED;
            break;
        }

        if (NULL == pCount)
        {
            Status = COMM_MGR_STATUS_INVALID_PARAM;
            break;
        }

        *pCount = CirBuffCount(CommMgrInfo[CommType].pRxCirBuff);

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   USB send function
 *
 * \details Interface function to send data
 *
 * \param   pData - pointer to Data to be transferred
 * \param   pSize - pointer to Data size, will be updated as per actual transferred data
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_INVALID_PARAM - Invalid parameter
 * \retval  COMM_MGR_STATUS_Q_FULL - Queue is full
 * \retval  COMM_MGR_STATUS_OK - Success
 *
 * ========================================================================== */
static COMM_MGR_STATUS UsbSend(uint8_t *pData, uint16_t *pSize)
{
    return CommSend(COMM_CONN_USB, pData, pSize);
}

/* ========================================================================== */
/**
 * \brief   USB Receive function
 *
 * \details Interface function to receive data
 *
 * \param   pData - pointer to Data to be received
 * \param   pSize - pointer to Data size, will be updated as per actual transferred data
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_INVALID_PARAM - Invalid parameter
 * \retval  COMM_MGR_STATUS_Q_EMPTY - Queue is empty
 * \retval  COMM_MGR_STATUS_OK - Success
 *
 * ========================================================================== */
static COMM_MGR_STATUS UsbReceive(uint8_t *pData, uint16_t *pSize)
{
    return CommReceive(COMM_CONN_USB, pData, pSize);
}

/* ========================================================================== */
/**
 * \brief   USB Peek function
 *
 * \details Interface function to check if new any data is available to read
 *
 * \param   pCount - pointer to Size of unread data
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_INVALID_PARAM - Invalid parameter
 * \retval  COMM_MGR_STATUS_OK - Success
 *
 * ========================================================================== */
static COMM_MGR_STATUS UsbPeek(uint16_t *pCount)
{
    return CommPeek(COMM_CONN_USB, pCount);
}

/* ========================================================================== */
/**
 * \brief   Inititialze the interfaces
 *
 * \details Call init functions of each interface and initialize the buffers
 *
 * \param   < None >
 *
 * \return  COMM_MGR_STATUS        - CommMgrStatus
 * \retval  COMM_MGR_STATUS_ERROR  - Error in USB initialization
 * \retval  COMM_MGR_STATUS_OK     - Successful in USB initialization
 *
 * ========================================================================== */
static COMM_MGR_STATUS UsbInit(void)
{
    COMM_MGR_STATUS Status;                     /* Contains status */
    USB_STATUS UsbStatus;                       /* USB status */

    Status = COMM_MGR_STATUS_OK;

    UsbStatus = L3_UsbInit(&UsbCallback);
    if (USB_STATUS_OK != UsbStatus)
    {
        Status = COMM_MGR_STATUS_ERROR;
        Log(ERR, "L3_UsbInit: Error - %d", UsbStatus);
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   WLAN callback function
 *
 * \details WLAN callback with event
 *
 * \param   Event - WLAN event
 *
 * \return  None
 *
 * ========================================================================== */
static void WlanCallback(WLAN_EVENT Event)
{
    COMM_MGR_EVENT CommEvent;

    /* Currently only CONNECT event is handled */
    L3_WlanSetConnectStatus(true);
    CommMgrInfo[COMM_WLAN].IsConnected = true;
    UpdateActiveConnection();

    /* Connect event */
    CommEvent = COMM_MGR_EVENT_CONNECT;

    if (NULL != CommMgrInfo[COMM_WLAN].pCommHandler)
    {
        CommMgrInfo[COMM_WLAN].pCommHandler(CommEvent);
    }

    /* Update the WLAN Active interface in Console manager */
    /* This is required since the WLAN/USB may be initialzed even after console manager initialization */
    pActiveInterface = L4_CommManagerConnOpen(COMM_CONN_ACTIVE, &CommEventHandler);
    if (NULL != pActiveInterface)
    {
        L4_ConsoleMgrUpdateInterface(pActiveInterface);
    }
}

/* ========================================================================== */
/**
 * \brief   Initialize the interface
 *
 * \details WLAN interface specific initialization
 *
 * \param   < None >
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_ERROR  - Error in WLAN initialization
 * \retval  COMM_MGR_STATUS_OK     - Successful in WLAN initialization
 *
 * ========================================================================== */
static COMM_MGR_STATUS WlanInit(void)
{
    COMM_MGR_STATUS Status;            /* Contains status */

    CommMgrInfo[COMM_WLAN].IsConnected = false;
    Status = COMM_MGR_STATUS_OK;
    L3_WlanRegisterCallback(&WlanCallback);
    return Status;
}

/* ========================================================================== */
/**
 * \brief   WLAN Receive
 *
 * \details This function invokes WLAN specific receive function
 *
 * \note   No NULL check added for pCount. Its taken care before invoking this
 *         function.
 *
 * \param   pData  -  Pointer to buffer
 * \param   pCount -  Pointer to Bytes received
 *
 * \return  bool  - Receive data status
 *
 * ========================================================================== */
static bool WlanRx(uint8_t *pData, uint16_t *pCount)
{
    bool IsReceiveOk;          /* Receive status flag */

    IsReceiveOk = true;

    if (WLAN_STATUS_OK == L3_WlanReceive(pData, pCount))
    {
        IsReceiveOk = false;
    }

    return IsReceiveOk;
}

/* ========================================================================== */
/**
 * \brief   WLAN Transmit
 *
 * \details This function invokes WLAN specific transmit function
 *
 * \note   No NULL check added for pCount. Its taken care before invoking this
 *         function.
 *
 * \param   pData  -  Pointer to buffer
 * \param   pCount -  Pointer to Bytes to be transmitted
 *
 * \return  bool  - Send data status
 *
 * ========================================================================== */
static bool WlanTx(uint8_t *pData, uint16_t *pCount)
{
    bool IsTransferOk;            /* Transfer status flag */

    IsTransferOk = true;

    if (WLAN_STATUS_OK == L3_WlanSend(pData, pCount))
    {
        IsTransferOk = false;
    }

    return IsTransferOk;
}

/* ========================================================================== */
/**
 * \brief   Close the interface
 *
 * \details Call disconnect function of WLAN interface
 *
 * \param   < None >
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_ERROR  - Error in closing WLAN connection
 * \retval  COMM_MGR_STATUS_OK     - Successful in closing WLAN connection
 *
 * ========================================================================== */
static COMM_MGR_STATUS WlanClose(void)
{
    COMM_MGR_STATUS Status;            /* Contains status */

    Status = COMM_MGR_STATUS_ERROR;

    CommMgrInfo[COMM_WLAN].IsConnected = false;

    if (WLAN_STATUS_OK == L3_WlanDisconnect())
    {
        Status = COMM_MGR_STATUS_OK;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   WLAN send function
 *
 * \details Interface function to send data
 *
 * \param   pData - Pointer to Data to be transferred
 * \param   pSize - Pointer to Data size, updated as per actual transferred data
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_INVALID_PARAM - Invalid parameter
 * \retval  COMM_MGR_STATUS_Q_FULL - Queue is full
 * \retval  COMM_MGR_STATUS_ERROR  - Error in sending data
 * \retval  COMM_MGR_STATUS_OK - Success
 *
 * ========================================================================== */
static COMM_MGR_STATUS WlanSend(uint8_t *pData, uint16_t *pSize)
{
    COMM_MGR_STATUS Status;            /* Contains status */

    Status = COMM_MGR_STATUS_ERROR;

    if (L3_WlanConnectStatus())
    {
        CommMgrInfo[COMM_WLAN].IsConnected = true;
        Status = CommSend(COMM_CONN_WLAN, pData, pSize);
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   WLAN Receive function
 *
 * \details Interface function to receive data
 *
 * \param   pData - Pointer to Data to be received
 * \param   pSize - Pointer to Data size, updated as per actual received data
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_INVALID_PARAM - Invalid parameter
 * \retval  COMM_MGR_STATUS_Q_EMPTY - Queue is empty
 * \retval  COMM_MGR_STATUS_ERROR  - Error in receiving data
 * \retval  COMM_MGR_STATUS_OK - Success
 *
 * ========================================================================== */
static COMM_MGR_STATUS WlanReceive(uint8_t *pData, uint16_t *pSize)
{
    COMM_MGR_STATUS Status;            /* Contains status */

    Status = COMM_MGR_STATUS_ERROR;

    if (L3_WlanConnectStatus())
    {
        CommMgrInfo[COMM_WLAN].IsConnected = true;
        Status = CommReceive(COMM_CONN_WLAN, pData, pSize);
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   WLAN Peek function
 *
 * \details Interface function to check if new any data is available to read
 *
 * \param   pCount - Pointer to the Size of unread data
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_INVALID_PARAM - Invalid parameter
 * \retval  COMM_MGR_STATUS_ERROR  - Error in peeking data
 * \retval  COMM_MGR_STATUS_OK - Success
 *
 * ========================================================================== */
static COMM_MGR_STATUS WlanPeek(uint16_t *pCount)
{
    COMM_MGR_STATUS Status;            /* Contains status */

    Status = COMM_MGR_STATUS_ERROR;

    if (L3_WlanConnectStatus())
    {
        CommMgrInfo[COMM_WLAN].IsConnected = true;
        Status = CommPeek(COMM_CONN_WLAN, pCount);
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Inititialze the interfaces
 *
 * \details Call init functions for Uart0 interface and initialize the buffers
 *
 * \param   < None >
 *
 * \return  COMM_MGR_STATUS        - CommMgrStatus
 * \retval  COMM_MGR_STATUS_ERROR  - Error in Uart0 initialization
 * \retval  COMM_MGR_STATUS_OK     - Successful in Uart0 initialization
 *
 * ========================================================================== */
static COMM_MGR_STATUS Uart0Init(void)
{
    COMM_MGR_STATUS Status = COMM_MGR_STATUS_OK;  /* Contains status */

    if (UART0_STATUS_OK != L3_Uart0Init())
    {
        Status = COMM_MGR_STATUS_ERROR;

    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Uart0 send function
 *
 * \details Interface function to send data
 *
 * \param   pData - pointer to Data to be transferred
 * \param   pSize - pointer to Data size, will be updated as per actual transferred data
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_INVALID_PARAM - Invalid parameter
 * \retval  COMM_MGR_STATUS_Q_FULL - Queue is full
 * \retval  COMM_MGR_STATUS_OK - Success
 *
 * ========================================================================== */
static COMM_MGR_STATUS Uart0Send(uint8_t *pData, uint16_t *pSize)
{
    return CommSend(COMM_CONN_UART0, pData, pSize);
}

/* ========================================================================== */
/**
 * \brief   Uart0 Receive function
 *
 * \details Interface function to receive data
 *
 * \param   pData - Pointer to Data to be received
 * \param   pSize - Pointer to Data size, updated as per actual received data
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_INVALID_PARAM - Invalid parameter
 * \retval  COMM_MGR_STATUS_Q_EMPTY - Queue is empty
 * \retval  COMM_MGR_STATUS_ERROR  - Error in receiving data
 * \retval  COMM_MGR_STATUS_OK - Success
 *
 * ========================================================================== */
static COMM_MGR_STATUS Uart0Receive(uint8_t *pData, uint16_t *pSize)
{
    return CommReceive(COMM_CONN_UART0, pData, pSize);
}

/* ========================================================================== */
/**
 * \brief   Uart0 Peek function
 *
 * \details Interface function to check if new any data is available to read
 *
 * \param   pCount - Pointer to the Size of unread data
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_INVALID_PARAM - Invalid parameter
 * \retval  COMM_MGR_STATUS_ERROR  - Error in peeking data
 * \retval  COMM_MGR_STATUS_OK - Success
 *
 * ========================================================================== */
static COMM_MGR_STATUS Uart0Peek(uint16_t *pCount)
{
    return CommPeek(COMM_CONN_UART0, pCount);
}

/* ========================================================================== */
/**
 * \brief   Uart0 Receive
 *
 * \details This function invokes Uart0 specific receive function
 *
 * \note   No NULL check added for pCount. Its taken care before invoking this
 *         function.
 *
 * \param   pData  -  Pointer to buffer
 * \param   pCount -  Pointer to Bytes received
 *
 * \return  bool  - Receive data status
 *
 * ========================================================================== */
static bool Uart0Rx(uint8_t *pData, uint16_t *pCount)
{
    bool IsReceiveOk;          /* Receive status flag */

    IsReceiveOk = true;

    if (UART0_STATUS_OK == L3_Uart0Receive(pData, pCount))
    {
        IsReceiveOk = false;
    }

    return IsReceiveOk;
}

/* ========================================================================== */
/**
 * \brief   Uart0 Transmit
 *
 * \details This function invokes WLAN specific transmit function
 *
 * \note   No NULL check added for pCount. Its taken care before invoking this
 *         function.
 *
 * \param   pData  -  Pointer to buffer
 * \param   pCount -  Pointer to Bytes to be transmitted
 *
 * \return  bool - Send data status
 *
 * ========================================================================== */
static bool Uart0Tx(uint8_t *pData, uint16_t *pCount)
{

    bool IsTransferOk;            /* Transfer status flag */

    IsTransferOk = true;

    if (UART0_STATUS_OK == L3_Uart0Send(pData, pCount))
    {
        IsTransferOk = false;
    }

    return IsTransferOk;
}

/* ========================================================================== */
/**
 * \brief   Active connection type
 *
 * \details Function to check the active connection type. USB has the highest priority
 *          followed by WLAN and then IR as the last.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void UpdateActiveConnection(void)
{
    if (CommMgrInfo[COMM_USB].IsConnected)
    {
        ActiveConn = COMM_USB;
    }
    else if (CommMgrInfo[COMM_WLAN].IsConnected)
    {
        ActiveConn = COMM_WLAN;
    }
    else
    {
        ActiveConn = COMM_COUNT;
    }
}

/* ========================================================================== */
/**
 * \brief   Inititialze the interfaces
 *
 * \details Call init functions of each interface and initialize the buffers
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void InterfaceInit(void)
{
    uint8_t Index;                              /* contains iteration index */

    for (Index = COMM_WLAN; Index < COMM_COUNT; Index++)
    {
        /* Initialize the circular buffers */
        if ((NULL != CommMgrInfo[Index].pRxCirBuff) && (NULL != CommMgrInfo[Index].pRxDataBuffer))
        {
            CirBuffInit(CommMgrInfo[Index].pRxCirBuff, CommMgrInfo[Index].pRxDataBuffer, MAX_DATA_BYTES);
        }
        if ((NULL != CommMgrInfo[Index].pTxCirBuff) && (NULL != CommMgrInfo[Index].pTxDataBuffer))
        {
            CirBuffInit(CommMgrInfo[Index].pTxCirBuff, CommMgrInfo[Index].pTxDataBuffer, MAX_DATA_BYTES);
        }
        if (NULL != CommMgrInfo[Index].pInit)
        {
            CommMgrInfo[Index].pInit();
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Function to clear all the USB flags used for USB inactivity check
 *
 * \details This function will Clearing flags i.e. Logging out of secure mode
 *          due to Inactivity on USB Port
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void ClearUSBFlags(void)
{
    /* Set Protected Mode */
    SetUSBPortMode(false);
    /* Clear the Password received flag to indicate logged out of secure mode */
    ClearPasswordReceived();
}

/******************************************************************************/
/*                                 Global Functions                           */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Function to initialize the Communication Manager
 *
 * \details This function creates the communication manager task, mutex and timer
 *
 * \param   < None >
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_ERROR - On Error
 * \retval  COMM_MGR_STATUS_OK - On success
 *
 * ========================================================================== */
COMM_MGR_STATUS L4_CommManagerInit(void)
{
    COMM_MGR_STATUS       CommMgrStatus;                        /* contains status */
    uint8_t                OsError;                             /* contains status of error */
    static bool     CommMgrInitDone = false;                    /* to avoid multiple init */

    do
    {
        CommMgrStatus = COMM_MGR_STATUS_OK;     /* Default Status */

        if (CommMgrInitDone)
        {
            /* Repeated initialization attempt, do nothing */
            break;
        }
        pMutexCommMgr = SigMutexCreate("L4-CommMgr", &OsError);

        if (NULL == pMutexCommMgr)
        {
            /* Couldn't create mutex, exit with error */
            CommMgrStatus = COMM_MGR_STATUS_ERROR;
            Log(ERR, "L4_CommManagerInit: Comm Manager Mutex Create Error - %d", OsError);
            break;
        }

        /* Inititialze the interfaces */
        InterfaceInit();

        /* Create the Communication Manager Task */
        OsError = SigTaskCreate(&CommMgrTask,
                                NULL,
                                &CommMgrTaskStack[0],
                                TASK_PRIORITY_L4_COMM_MANAGER,
                                COMM_MGR_TASK_STACK,
                                "CommsMgr");

        if (OsError != OS_ERR_NONE)
        {
            /* Couldn't create task, exit with error */
            CommMgrStatus = COMM_MGR_STATUS_ERROR;
            Log(ERR, "L4_CommManagerInit: CommMgrTask Create Error - %d", OsError);
            break;
        }

        /* Initialization done */
        CommMgrInitDone = true;

    } while (false);

    return CommMgrStatus;
}

/* ========================================================================== */
/**
 * \brief   Function to open specified connection
 *
 * \details This function creates a connection to specified type
 *
 * \param   Type -  Connection interface to be opened
 * \param   pHandler -  pointer to Event handler function
 *
 * \return  COMM_IF - Communication interface object returned when a connection is open
 * \retval  NULL - On Error
 *
 * ========================================================================== */
COMM_IF* L4_CommManagerConnOpen(COMM_CONN Type, COMM_HANDLER pHandler)
{
    COMM_IF *pConnection;                       /* pointer to Communication object interface */
    COMM_TYPE CommType;                         /* Communication interface type */

    CommType = COMM_COUNT;
    pConnection = NULL;

    CommType = (COMM_CONN_ACTIVE == Type) ? ActiveConn : (COMM_TYPE)Type;

    do
    {
        if ((NULL == pHandler) || (CommType >= COMM_COUNT))
        {
            break;
        }

        if (COMM_UART0 == CommType)
        {
            CommMgrInfo[COMM_UART0].IsConnected = true;
        }

        CommMgrInfo[CommType].pCommHandler = pHandler;
        pConnection = CommMgrInfo[CommType].pCommIf;

        /* Update the Active connection type */
        UpdateActiveConnection();

    } while (false);

    return pConnection;
}

/* ========================================================================== */
/**
 * \brief   Function to close the open connection
 *
 * \details This function closes the connection
 *
 * \param   pConnection -  Pointer to the Connection interface
 *
 * \return  COMM_MGR_STATUS - CommMgrStatus
 * \retval  COMM_MGR_STATUS_ERROR - On Error
 * \retval  COMM_MGR_STATUS_OK - On success
 *
 * ========================================================================== */
COMM_MGR_STATUS L4_CommManagerConnClose(COMM_IF *pConnection)
{
    COMM_MGR_STATUS   CommMgrStatus;           /* contains status */
    uint8_t           Index;                   /* Iteration index */
    COMM_MGR_INFO     *pCommHandler;           /* Pointer to the Communication Interface object */

    CommMgrStatus = COMM_MGR_STATUS_ERROR;

    do
    {
        if (NULL == pConnection)
        {
            break;
        }
        /* Find and close the interfaces */
        for (Index = COMM_WLAN; Index < COMM_COUNT; Index++)
        {
            pCommHandler = &CommMgrInfo[Index];

            if (pConnection == pCommHandler->pCommIf)
            {
                if (NULL != pCommHandler->pClose)
                {
                    CommMgrStatus = pCommHandler->pClose();
                }
                CommMgrStatus = COMM_MGR_STATUS_OK;
                pConnection = NULL;
                pCommHandler->IsConnected = false;

                /* Update the Active connection type */
                UpdateActiveConnection();
                break;
            }
        }

    } while (false);

    return CommMgrStatus;
}

/* ========================================================================== */
/**
 * \brief   Function to read the USB connection status
 *
 * \details This function will return the USB connection status
 *
 * \param   < None >
 *
 * \return  uint8_t - return CommMgrInfo[COMM_USB].IsConnected value
 *
 * ========================================================================== */
uint8_t L4_USBConnectionStatus(void)
{
    uint8_t Status;
    Status = CommMgrInfo[COMM_USB].IsConnected;
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Function to read connection status
 *
 * \details This function will return the Active connection status. if no active connections
 *          this function will return Error else Success
 *
 * \param   < None >
 *
 * \return  bool - CommMgrInfo[COMM_USB].IsConnected value
 *
 * ========================================================================== */
bool L4_CommStatusActive(void)
{
    uint8_t Status;
    Status = (CommMgrInfo[COMM_USB].IsConnected | CommMgrInfo[COMM_WLAN].IsConnected);

    return (Status); /* success indicated with false */
}

/**
 *\}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

