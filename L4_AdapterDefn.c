#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{
 *
 * \brief   Adapter Definition functions
 *
 * \details The Adapter Definition defines all the interfaces used for
            communication between Handle and Adapter.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_AdapterDefn.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_AdapterDefn.h"
#include "Signia_CommManager.h"
#include "L4_ConsoleCommands.h"
#include "L2_Uart.h"
#include "L3_GpioCtrl.h"
#include "CirBuff.h"
#include "L4_BlobHandler.h"
#include "FaultHandler.h"
#include <math.h>
#include "TestManager.h"
#include "Aes.h"
#include "ActiveObject.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER             (LOG_GROUP_ADAPTER)   /*! Log Group Identifier */
#define PACKET_START                     (0xAA)                /*! Packet start byte */
#define PACKET_OVERHEAD                  (4u)                  /*! Packet size not including the data */
#define COMMAND_BYTE_MASK_BOOTLOADER     (0xE0)                /*! Bootloader byte mask as per Adapter's code */
#define COMMAND_BYTE_MASK_MAINAPP        (0xC0)                /*! Main app byte mask as per Adapter's code */
#define FLUSH_TIMEOUT_MSEC               (MSEC_100)            /*! Uart flush timeout */
#define MIN_PACKET_SIZE                  (4u)                  /*! Minimum packet size */
#define ADAPTER_COMMAND_MASK             (0x1Fu)               /*! Adapter command mask */
#define CMD_INDEX_OFFSET                 (2u)                  /*! Command index offset in the received packet */
#define CMD_DATA_OFFSET                  (1u)                  /*! Data index offset of the command */
#define RESPONSE_TIMEOUT                 (MSEC_500)            /*! Maximum time to wait for the command response */
#define STREAM_RESPONSE_TIMEOUT          (MSEC_100)             /*! Maximum time to wait for the Stream data response */
#define INVALID_BYTE_MASK                (0xFFu)               /*! Invalid byte mask */
#define MAX_PACKET_SIZE                  (250u)                /*! Largest packet size */
#define INVALID_RESP_CODE                (0xFFu)               /*! Invalid response code */
#define MAX_BOOT_RETRY_COUNT             (2u)                  /*! Retry count to enter/exit boot mode */
#define MAX_BUFFER_INDEX_FLASH_READ_CMD  (6u)                  /*! Maximum buffer size for Flash Read command packet generation */
#define COMMAND_RETRY_COUNT              (5u)
#define ADAPTER_DATA_BLOCK_SIZE          (64u)
#define MASK_BOOTLOADER_MAINAPP_TOGGLE   (0x20u)
#define GTIN_CHAR_COUNT                  (20u)                 /*!  GTIN (Global Trade Item Number) char count*/
#define EGIA_FLASH_CONFIG_DATA_SIZE      (sizeof(EGIA_FACTORY_DATAFLASH) - sizeof(AdapterTimeStamps))   /*! Adapter Data Flash Factory size */
#define COMMAND_RETRY_COUNT              (5u)
#define RECEIVE_STATUS_INDEX             (0u)
#define RECEIVE_DATA_INDEX               (1u)
#define MAX_FLASH_READ_RETRY_COUNT       (50u)                  /*! Maximum Number of retries for reading adapter flash */
#define FLASH_READS_INTERVAL_DELAY_MS    (50u)                  /*! delay between successive retries for reading flash */
#define DEST_ADDR_SIZE                   (4u)                   /*! Adapter flash block start address to be written to */

#define MAX_ADAPTERQ_REQUESTS            (20u)
#define ADAP_SUPPLYOFFTIME               (50u)

#define ADAPTER_IN_BOOT                   (1u)
#define ADAPTER_IN_MAIN                   (0u)
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
///< Return code from Adapter DATA_FLASH functions
typedef enum
{
    DATAFLASH_SUCCESS = 0,
    DATAFLASH_ILLEGAL_ADDRESS,  ///< Unknown data flash address terr
    DATAFLASH_OUT_OF_RANGE,     ///< Out of Data flash config area
    DATAFLASH_READ_FAILURE,     ///< Read Failure
    DATAFLASH_WRITE_FAILURE,    ///< Write Failure
    DATAFLASH_UNDEFINED         ///< Undefines Data Flash
} ADAPT_FLASH_ERR;

///< Adapter 16bit unsigned error codes for the adapter object to signal
typedef enum
   {
   ADAPTER_OBJECT_NO_ERROR = 0,
   ADAPTER_OBJECT_STARTUP_ERROR,
   ADAPTER_OBJECT_CLAMP_TEST_ERROR,
   ADAPTER_OBJECT_TARE_ERROR,
   ADAPTER_OBJECT_COEF_ERROR,
   ADAPTER_OBJECT_UART_ERROR,
   ADAPTER_OBJECT_ONEWIRE_ID_ERROR,
   ADAPTER_OBJECT_MOT_TIMOUT_ERROR,       ///<  Proximal Movement Timeout Error
   ADAPTER_OBJECT_MOT_CURLIMIT_ERROR,    ///<  Current Limit Error
   ADAPTER_OBJECT_MOT_SG_ERROR,           ///<  Strain Gauge Error
   ADAPTER_OBJECT_MOT_DISTAL_TO_ERROR,  ///<  Distal Movement Timeout Error
   ADAPTER_OBJECT_HARDWARE_VER_ERROR,  ///< the hardware vrsion could not be determined
   ADAPTER_OBJECT_ERROR_COUNT
} ADAPTER_OBJECT_ERRORS;

typedef enum
{
  ADAPTER_COM_STATE_IDLE,            ///< Adapter communication in Idle
  ADAPTER_COM_STATE_CHECKQ,          ///< Adapter communication in Q checking
  ADAPTER_COM_STATE_INPROGRESS,      ///< Adapter communication in-progress
  ADAPTER_COM_STATE_WAIT,            ///< Adapter communication in wait
  ADAPTER_COM_STATE_COUNT
}ADAPTER_COM_STATES;


typedef enum
{
  ADAPTER_CMD_STATE_SEND,           ///< Adapter command send in progress
  ADAPTER_CMD_STATE_WAIT_FOR_RESPONSE, ///< Wait for adapter response
  ADAPTER_CMD_STATE_COUNT
}ADAPTER_CMD_STATES;


/// Adapter Data Flash Factory Strain Gauge Calibration Parameters
typedef struct
{
   uint32_t   StructChecksum;            ///< Strain Gauge calibration param checksum
   uint32_t   Gain;                     ///< Strain Gauge Calibration Gain value
   uint32_t   Offset;                    ///< Strain Gauge Calibration Offset value
   uint32_t   SecondOrderCoef;         ///< Strain Gauge 2nd Order Coefficients
} FACTORY_STRAINGAUGE_CAL;

/// Factory Adapter Calibration Parameters
typedef struct
{
   uint32_t   StructChecksum;
   uint32_t   FirerodBacklashTurns;
   uint32_t   FirerodCalibrationTurns;    ///< FireRod Calibration turns from hardstop
   uint32_t   ClampTurns;                  ///< Clamp Turns from hardstop
   uint32_t   ArticCalibrationTurns;      ///< Artic Calibration turns from hardstop
   uint32_t   ArticMaxLeftTurns;         ///< Max Turns Left from midpoint
   uint32_t   ArticMaxRightTurns;        ///< Max Turns Right from midpoint
   uint32_t   RotateMaxTurns;             ///< Turns to Rotate before stopping
} FACTORY_ADAPTER_CALPARMS;                 ///< FactoryAdapterCalParams;

/// Adapter Board Parameters
typedef struct
{
   uint32_t   StructChecksum;
   uint32_t   TareDriftHigh;              ///<  maximum positive drift allowable to tare off
   uint32_t   tare_drift_low;               ///<  maximum drift allowable to tare off
   uint32_t   ZbCountCeiling;             ///<  maximum value for zero pound count (before tare at rod calibration)
   uint32_t   ZbCountFloor;               ///<  maximum value for zero pound count (before tare at rod calibration)
} FACTORY_ADAPTER_BOARDPARMS;               ///< factoryAdapterBoardParams;  //paramters variable with iDDi board

/// Factory LOT Number
typedef struct
{
   uint32_t   StructChecksum;
   char     LotNumber[ADAPTER_LOT_CHARS];   ///<  ASCII Lot Number, 15 chars, 16th char is NULL
} FACTORY_ADAPTER_LOT;                      ///< factoryAdapterLot;

/// Factory GTIN Number
typedef struct
{
   uint32_t   StructChecksum;
   char     Number[GTIN_CHAR_COUNT];        ///<  ASCII Lot Number, 15 chars, 16th char is NULL
} FACTORY_GTIN;

/// items in adapter flash memory are structures whose first member is an uint32_t checksum, calculatyed with CRC32()
typedef struct
{
    char            VarSize;             ///< includes the 4 byte checksum
    void *          VarAddress;          ///< point to variable containing the item to be stored in adapter FLASH
    uint32_t        VarFlashAddress;     ///< the physical FLASH memory address for the item
    ADAPT_FLASH_ERR status;              ///< indicates an error in CRC calulation for a read item
} DATAFLASH_MEMORY_FORMAT;

typedef struct
{
    char     VarSize;                 ///< Variable size to copy
    void *   VarAddress;              ///< the address of the runtime variable to copy/load
    void *   DataFlashVar;            ///< the variable in FLASH to be loaded from or copied to runtime variables
    void *   flashItemAddress;        ///< the address of the item in FLASH where the variable is located
} ADAPTER_DATA_VARS;

typedef struct
{
    uint8_t           Cmd;                             ///<  Command to send
    uint8_t           CmdMask;                         ///<  Byte mask for the command
    uint8_t           DataOut[MAX_PACKET_SIZE];        ///<  Data to send out
    uint8_t           DataSize;                        ///<  Data size to send out
    bool              CmdToSend;                       ///<  Command send status
    bool              RespReceived;                    ///<  Response received status
    OS_EVENT          *pSema;                          ///<  Semaphore for command processing
    uint8_t           ResponseStatus;                  ///<  Status of the response received
    uint8_t           RespData[MAX_PACKET_SIZE];       ///<  Data to send out
    uint8_t           CmdRetry;
    bool              RespTimeOut;
} ADAPTER_CMD_DATA;

/// This structure specifies the size of the area in data flash for EGIA factory paramters
typedef struct
{
  AdapterTimeStamps           Timestamps;              ///< Adapter Timestamps will go here
  FACTORY_STRAINGAUGE_CAL     Straingauge;             ///< Factory Strain Gauge Calibration values will go here
  FACTORY_ADAPTER_CALPARMS    AdapterCal;             ///< Factory Calibration values will go here
  FACTORY_ADAPTER_LOT         Lot;                     ///< Factory Lot Number will go here
  FACTORY_ADAPTER_BOARDPARMS  AdapterBoard;            ///< Factory Board Params will go here
  FACTORY_GTIN                GTINNumber;             ///< GTIN Number will go here, the space for LOT# will be the 10 digit serial Number, not used in power pack
} EGIA_FACTORY_DATAFLASH;

typedef struct
{
    uint16_t                     HardwareVersion;                  ///< Adapter Hardware version
    bool                         HwVersionStatus;                  ///< Flag for the version availability
    AdapterTimeStamps            TimeStamps;                       ///< Programmed Adapter firmware timestamps
    bool                         VerChecksumStatus;                ///< Flag to check the version checksum
    SG_FORCE                     StrainGaugeData[2];               ///< Strain gauge active and backup data
    SG_FORCE                     *pStrainGaugeNewData;             ///< Strain gauge data ptr
    SG_FORCE                     *pStrainGaugeOldData;             ///< Strain gauge data ptr
    uint16_t                     ForceTareOffset;                  ///< Tare Offset
    DEVICE_UNIQUE_ID             AdapterAddress;                   ///< Adapter identifier
    SWITCH_DATA                  SwitchData;                       ///< non-intelligent Reload state
    bool                         AdapterFlashParmStatus;
    uint8_t                      AdapterState;                     ///< InternalState of Adapter
    uint16_t 		         AdapterType;
    bool                         AdapterTypeStatus;
} ADAPTER_DEFN_REPO;

typedef enum
{
    ADAPTER_RESPONSE_STATUS = 0,
    ADAPTER_RESPONSE_LOWBYTE,
    ADAPTER_RESPONSE_HIGHBYTE
} ADAPTER_RESPONSE_DATA;




/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static AM_STATUS AdapterProcessCmdResp(uint8_t Command, uint8_t *pData, uint8_t DataSize, uint8_t CmdMask, uint8_t *pRespStatus);
static void SendAdapterUartCommand(uint8_t Command, uint8_t *pDataOut, uint8_t DataSize, uint8_t CmdMask);
static void ProcessAdapterUartStream(uint8_t *pDataPacket, uint16_t DataCount);
static void ProcessAdapterDataFlashPacket(uint8_t Command, uint8_t *pRecvData, uint8_t DataSize);
static void PacketAssembler(uint8_t *pDataPacket, uint16_t DataCount, uint16_t DataIndex);
static void ProcessAdapterPacket(void);

static AM_STATUS AmFlushUart(void);

static AM_STATUS AdapterEepUpdate(void);
static AM_STATUS AdapterDefnEnterMain(uint8_t *CmdMask);
static AM_STATUS AdapterEnableOneWire(bool Enable);
static AM_STATUS AdapterBootEnter(void);
static AM_STATUS AdapterEGIASwitchEventsEnable(bool Enable);
static AM_STATUS AdapterEGIAGetSwitchData(void);
static AM_STATUS AdapterGetSwitchState(SWITCH_DATA *pSwitch);
static AM_STATUS AdapterForceStreamStart(bool Start);
static AM_STATUS AdapterFlashErase(uint8_t *pData, uint8_t DataSize);
static AM_STATUS AdapterFlashWrite(uint8_t *pData, uint8_t DataSize);
static AM_STATUS AdapterWriteVersion(uint8_t *pData, uint8_t DataSize);
static AM_STATUS WriteAdapterFlash(void);
static AM_STATUS AdapterUpdateMainApp(AdapterTimeStamps *pAdapterVersion);
static AM_STATUS AdapterVersionGet(void);
static AM_STATUS AdapterHwVersionGet(void);
static SG_STATUS AdapterForceGet(SG_FORCE *pForce);
static AM_STATUS AdapterForceTare(void);
static AM_STATUS AdapterForceLimitsReset(void);
static AM_STATUS AdapterEepRead(void);
static AM_STATUS Read_AdapterDataFlash(void);
static AM_STATUS ReadAdapterFactoryValues(uint8_t *pRecvData, uint8_t );
static AM_STATUS AdapterFlashCalibParameters(uint8_t *pCalibParam);

static void AdapterComTimeout (void *pThis, void *pArgs);
static bool AdapterTimeoutTimerStart(void);
static bool AdapterTimeoutTimerStop(void);
static void AdapterSGStreamTimeout(void *pThis, void *pArgs);
static bool AdapterSGTimeoutTimerStart(void);
static bool AdapterSGTimeoutTimerStop(void);

static AM_STATUS AdapterSupplyON(void);
static AM_STATUS AdapterSupplyOFF(void);
static AM_STATUS AdapterRestart(void);
static bool AdapterIsComPending(void);
static void AdapterComSigPublish(SIGNAL Sig, ADAPTER_COMMANDS AdapterCmd);


/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static OS_EVENT *pAdapComQ;                     ///<  Request Q
static OS_TMR   *pAdapterTmOutTmr;               ///<  Adapter TimeoutTimer
static OS_TMR   *pAdapSGStreamTmOutTmr;         ///<  Adapter SG Stream Timeout Timer

static ADAPTER_CMD_DATA         AdapterCmdData;         /*! Command send to Adapter */
static OS_EVENT                 *pAdapterDefnMutex;     /*! Adapter Manager mutex */
static ADAPTER_DEFN_REPO        AdapterDefnRepo;        /*! Adapter data repository */

static FACTORY_STRAINGAUGE_CAL      StrainGauge_Flash;        ///<strain gauge item in adapter FLASH
static FACTORY_ADAPTER_CALPARMS     AdapterCalParams_Flash;   ///<adapter params
static FACTORY_ADAPTER_LOT          AdapterLot_Flash;         ///<Adapter Board Parameters
static FACTORY_ADAPTER_BOARDPARMS   AdapterBoard_Flash;       ///<params for iDDi board
static FACTORY_GTIN                 AdapterGTIN_Flash;        ///<Holds the GTIN (Global Trade Item Number)
                                                               ///<GTIN is more like a globally recognized reorder code.identifies the product for sale
//#pragma location=".sram"
ADAPTER_RESPONSE PartialResponse;

// Adapter FLASH format
// Adapter variables location in DATA FLASH response from adapter
// The GTIN address in FLASH is not to be crc checked as the FLASH in EGIA from factory have an incorrect checksum
// The space for GTIN in FLASH is 20 bytes, The factory fixtures writes 16 chars and 2 null resulting in a different CRC
#define GTIN_ADDRESS_INVALID_CRC    (DATA_FLASH_BOARD_PARAM_ADDRESS+sizeof(AdapterBoard_Flash))
static DATAFLASH_MEMORY_FORMAT AdapterFlash[] =
{   ///< includes the 4 byte checksum              ///< point to variable containing the item to be stored in adapter FLASH
    //VarSize;                                     VarAddress;               VarFlashAddress;     ///< the physical FLASH memory address for the item
    {sizeof(StrainGauge_Flash),                    &StrainGauge_Flash,       DATA_FLASH_STRAIN_GAUGE_ADDRESS},
    {sizeof(AdapterCalParams_Flash),               &AdapterCalParams_Flash,  DATA_FLASH_ADAPTER_PARAM_ADDRESS},
    {(ADAPTER_LOT_CHARS+FLASH_ITEM_CHECKSUM_SIZE),  &AdapterLot_Flash,        DATA_FLASH_LOT_ADDRESS},
    {sizeof(AdapterBoard_Flash),                   &AdapterBoard_Flash,      DATA_FLASH_BOARD_PARAM_ADDRESS},
    {(GTIN_CHAR_COUNT+FLASH_ITEM_CHECKSUM_SIZE),    &AdapterGTIN_Flash,       GTIN_ADDRESS_INVALID_CRC},
    {0,                                            NULL,                     0 }
};

static DEVICE_UNIQUE_ID 		AdapterAddress;                ///<  Adapter unique address
static APP_CALLBACK_HANDLER 	AdapterAppHandler[ADAPTER_APP_MAX];
static bool 					ErrorSet;
uint8_t  				        adapterOutgoingData[ADAPTER_TX_BUFF_SIZE + MEMORY_FENCE_SIZE_BYTES];
static uint8_t 					AdapterOutgoingData[ADAPTER_TX_BUFF_SIZE];
//#pragma location=".sram"
static uint8_t                  AdapterIncomingData[ADAPTER_RX_BUFF_SIZE];
static uint8_t                  AdapterFlashUpdateBuffer[ADAPTER_TX_BUFF_SIZE];
static uint8_t 					AESReadBuffer[2*AES_BLOCKLEN];
static BLOB_POINTERS 			BlobPointers;


static ADAPTER_COM_STATES AdapterComState;
static ADAPTER_CMD_STATES AdapterCmdState;
static uint32_t CmdRequested;


static ADAPTER_COM_MSG ComMsgReqPool[MAX_ADAPTERQ_REQUESTS];       /* Message Req Pool */
static const uint8_t AdapterCmdMask[2] = {COMMAND_BYTE_MASK_MAINAPP,COMMAND_BYTE_MASK_BOOTLOADER};



/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

// \todo: Add flash access support.
AM_ADAPTER_IF AdapterInterface =                ///<  Adapter Object Interface
{
    {0},
    &AdapterEepUpdate,
    AM_STATUS_OK,
    &AdapterGetSwitchState,
    &AdapterForceGet,
    &AdapterForceTare,
    &AdapterForceLimitsReset,
    &AdapterFlashCalibParameters,
    &AdapterSupplyON,
    &AdapterSupplyOFF,
    &AdapterIsComPending
};

COMM_IF *pAdapterComm;

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Flush the ADAPTER_UART.
 *
 * \details Flush the ADAPTER_UART. L2_UartGetRxByteCount() is called to check
 *          the buffer is completely flushed.
 *
 * \param   < None >
 *
 * \return  AM_STATUS - UART Flush status
 * \return  AM_STATUS_OK      - Flush successfully
 * \return  AM_STATUS_ERROR   - Error in flushing
 *
 * ========================================================================== */
static AM_STATUS AmFlushUart(void)
{
    UART_STATUS UartStatus;      /* Uart Status */
    AM_STATUS   AmStatus;        /* Function status */
    uint32_t    FlushTimeout;    /* Max time to wait for uart Flush */

    AmStatus = AM_STATUS_OK;

    FlushTimeout = OSTimeGet() + FLUSH_TIMEOUT_MSEC;

    do
    {
        if (OSTimeGet() >= FlushTimeout)
        {
            break;
        }

       UartStatus = L2_UartFlush(ADAPTER_UART);
       if ( UART_STATUS_OK != UartStatus )
       {
           AmStatus = AM_STATUS_ERROR;
           break;
       }

    } while ( L2_UartGetRxByteCount(ADAPTER_UART) );

    return AmStatus;
}

/* ========================================================================== */
/**
 * \brief   Sends the commands to Uart circular buffer
 *
 * \details Command to be sent is
 *
 * \param   Command   - Command to add
 * \param   pData     - Pointer to the data to send out
 * \param   DataSize  - Data size
 * \param   CmdMask  - Command byte mask
 *
 * \return  AM_STATUS       - Function status
 * \retval  AM_STATUS_OK    - No error in Command sending and response received
 * \retval  AM_STATUS_ERROR - Error in Command sending and response received
 *
 * ========================================================================== */
static AM_STATUS AdapterSendCmd(uint8_t Command, uint8_t *pData, uint8_t DataSize, uint8_t CmdMask)
{
    ADAPTER_CMD_DATA   *pCmdData;    /* Pointer to the command data */
    AM_STATUS          AmStatus;     /* Function status */

    AmStatus = AM_STATUS_ERROR;

    do
    {

        pCmdData = &AdapterCmdData;

        /* Add to the command data */
       if (!pCmdData->CmdToSend)
       {
           pCmdData->Cmd = Command;
           if (NULL != pData)
           {
               memcpy(pCmdData->DataOut, pData, DataSize);
           }
           pCmdData->DataSize = DataSize;
           pCmdData->CmdMask = CmdMask;
           pCmdData->CmdToSend = true;

           AmStatus = AM_STATUS_OK;
       }

        if (AM_STATUS_OK != AmStatus)
        {
            break;
        }
        SendAdapterUartCommand(pCmdData->Cmd, pCmdData->DataOut, pCmdData->DataSize, pCmdData->CmdMask);
        pCmdData->ResponseStatus = INVALID_RESP_CODE;
        pCmdData->RespReceived = false;
        AmStatus = AM_STATUS_OK;
    } while (false);

    return AmStatus;
}

/* ========================================================================== */
/**
 * \brief  Wait for the Response
 *
 * \details check if the Response is received over Uart or Timeout has occured
 *
 * \param   pData       - Pointer to the receiver data
 * \param   pRespStatus - Pointer to Response Status
 *
 * \return  AM_STATUS       - Function status
 * \retval  AM_STATUS_OK    - No error in Command sending and response received
 * \retval  AM_STATUS_ERROR - Error in Command sending and response received
 *
 * ========================================================================== */
static AM_STATUS AdapterChkCmdResp(uint8_t *pData, uint8_t *pRespStatus)
{
    ADAPTER_CMD_DATA   *pCmdData;    /* Pointer to the command data */
    AM_STATUS          AmStatus;     /* Function status */

    AmStatus = AM_STATUS_WAIT;
    pCmdData = &AdapterCmdData;

    if (pCmdData->RespReceived)
    {
        pCmdData->CmdToSend = false;
        pCmdData->Cmd = SERIALCMD_UNKNOWN;
        *pRespStatus = pCmdData->ResponseStatus;
        if(NULL != pData)
        {
             pData[0] = pCmdData->RespData[0];
             pData[1] = pCmdData->RespData[1];
        }
        AmStatus = AM_STATUS_OK;
    }
    else if(pCmdData->RespTimeOut)
    {
        AmStatus = AM_STATUS_TIMEOUT;
        pCmdData->CmdToSend = false;
        pCmdData->Cmd = SERIALCMD_UNKNOWN;
        pCmdData->RespTimeOut = false;
    }
    else
    {
        AmStatus = AM_STATUS_WAIT;
    }
    return AmStatus;
}



/* ========================================================================== */
/**
 * \brief  Adapter Uart Com Response Timeout callback
 *
 * \details Callback function for Adapter communication Response timeout
 *
 *
 * \return  None
 *
 * ========================================================================== */
static void AdapterComTimeout (void *pThis, void *pArgs)
{
    ADAPTER_CMD_DATA   *pCmdData;    /* Pointer to the command data */
    pCmdData = &AdapterCmdData;
    bool Retry;
    Retry = false;

    if (pCmdData->CmdRetry < COMMAND_RETRY_COUNT)
    {
        pCmdData->CmdRetry++;
        SendAdapterUartCommand(pCmdData->Cmd, pCmdData->DataOut, pCmdData->DataSize, pCmdData->CmdMask);
        if (AdapterTimeoutTimerStart())
        {
            Retry = true;
        }
    }

    if( !Retry )
    {

        /*Update CmdData structure with timeout*/
        pCmdData->RespReceived = false;
        pCmdData->RespTimeOut  = true;
        pCmdData->CmdRetry = 0;
    }
}

/* ========================================================================== */
/**
 * \brief  Start Adapter Timeout timer
 *
 * \details Start or retrigger the Timer used to monitor the Uart communication for timeout
 *
 * \param   < None >
 *
 * \return  bool       - Function status
 * \retval  true       - No error in starting the timer
 * \retval  false      - Error in starting the timer
 *
 * ========================================================================== */
static bool AdapterTimeoutTimerStart(void)
{
    uint8_t OsError;
    bool Status;
    Status = true;
    OSTmrStart(pAdapterTmOutTmr, &OsError);

    if (OS_ERR_NONE != OsError)
    {
        Log(ERR, "AdapterCom Timeout Timer Start Error: %d",OsError);
        Status = false;
       /* \todo: Fatal error */
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief  Stop Adapter Timeout timer
 *
 * \details Stop the Timer used to monitor the Uart communication for timeout
 *
 * \param   < None >
 *
 * \return  bool       - Function status
 * \retval  true       - No error in stopping the timer
 * \retval  false      - Error in stopping the timer
 *
 * ========================================================================== */
static bool AdapterTimeoutTimerStop(void)
{
    uint8_t OsError;
    bool Status;
    Status = true;
    OSTmrStop(pAdapterTmOutTmr,OS_TMR_OPT_NONE, NULL, &OsError);

    if (OS_ERR_NONE != OsError && OS_ERR_TMR_STOPPED != OsError)
    {
        Log(ERR, "AdapterCom TimerStop Error:  %d",OsError);
        Status = false;
       /* \todo: Fatal error */
    }
    return Status;
}
/* ========================================================================== */
/**
 * \brief  Adapter SG Stream Response Timeout callback
 *
 * \details Callback function for Adapter communication Response timeout
 *
 *
 * \return  None
 *
 * ========================================================================== */
static void AdapterSGStreamTimeout(void *pThis, void *pArgs)
{
    /* Publish Adapter Retry Failed signal */
    L4_AdapterComSMReset();

    AdapterComSigPublish(P_ADAPTER_COM_RETRY_FAIL_SIG,ADAPTER_START_SGSTREAM);
}

/* ========================================================================== */
/**
 * \brief  Start Adapter Timeout timer
 *
 * \details Start or retrigger the Timer used to monitor the SG Stream for timeout
 *
 * \param   < None >
 *
 * \return  bool       - Function status
 * \retval  true       - No error in starting the timer
 * \retval  false      - Error in starting the timer
 *
 * ========================================================================== */
static bool AdapterSGTimeoutTimerStart(void)
{
    uint8_t Error;
    bool Status;
    Status = true;
    OSTmrStart(pAdapSGStreamTmOutTmr, &Error);

    if (OS_ERR_NONE != Error)
    {
        Error = Error;
        Log(ERR, "AdapterStream Timeout Timer Start Error: Error is %d",Error);
        Status = false;
       /* \todo: Fatal error */
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief  Stop Adapter Timeout timer
 *
 * \details Stop the Timer used to monitor the SG Stream for timeout
 *
 * \param   < None >
 *
 * \return  bool       - Function status
 * \retval  true       - No error in stopping the timer
 * \retval  false      - Error in stopping the timer
 *
 * ========================================================================== */
static bool AdapterSGTimeoutTimerStop(void)
{
    uint8_t Error;
    bool Status;
    Status = true;
    OSTmrStop(pAdapSGStreamTmOutTmr,OS_TMR_OPT_NONE, NULL, &Error);

    if (OS_ERR_NONE != Error && OS_ERR_TMR_STOPPED != Error)
    {
        Error = Error;
        Log(ERR, "AdapterStream TimerStop Error: Error is %d",Error);
        Status = false;
       /* \todo: Fatal error */
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Add command to the Command list and wait for the response
 *
 * \details The command list will be processed by the Adapter Manager state machine for
 *          sending command and response processing.
 *
 * \param   Command   - Command to add
 * \param   pData     - Pointer to the data to send out
 * \param   DataSize  - Data size
 * \param   CmdMask  - Command byte mask
 * \param   pRespStatus - pointer to response status
 *
 * \return  AM_STATUS       - Function status
 * \retval  AM_STATUS_OK    - No error in Command sending and response received
 * \retval  AM_STATUS_ERROR - Error in Command sending and response received
 *
 * ========================================================================== */
static AM_STATUS AdapterProcessCmdResp(uint8_t Command, uint8_t *pData, uint8_t DataSize, uint8_t CmdMask, uint8_t *pRespStatus)
{
    ADAPTER_CMD_DATA   *pCmdData;    /* Pointer to the command data */
    AM_STATUS          AmStatus;     /* Function status */

    AmStatus = AM_STATUS_ERROR;
    switch( AdapterCmdState )
    {

        case ADAPTER_CMD_STATE_SEND:
            AmStatus = AdapterSendCmd( Command, pData, DataSize, CmdMask );
            if(AM_STATUS_OK == AmStatus)
            {
                AmStatus = AM_STATUS_WAIT;
                AdapterCmdState = ADAPTER_CMD_STATE_WAIT_FOR_RESPONSE;

            }
            break;

        case ADAPTER_CMD_STATE_WAIT_FOR_RESPONSE:
            AmStatus = AdapterChkCmdResp(pData,pRespStatus);
            BREAK_IF(AM_STATUS_WAIT == AmStatus);
            pCmdData = &AdapterCmdData;
            pCmdData->CmdToSend = false;
            pCmdData->Cmd = SERIALCMD_UNKNOWN;
            pCmdData->CmdRetry = 0;
            AdapterCmdState = ADAPTER_CMD_STATE_SEND;
            break;

        default:
            AdapterCmdState = ADAPTER_CMD_STATE_SEND;
            break;
    }

    return AmStatus;
}

/* ========================================================================== */
/**
 * \brief   Read the response from the Adapter
 *
 * \details Processes the received buffer based on start packet.
 * \details Upon identifying start packet, inturn calls Packet Assember to construct the Packet and if Packet is not partial performs Packet Processing. If packet is partial breaks
 * \details Runs throughout the buffer to identify start packet and handle's all the multiple packets.
 *
 * \note    Packet layout: ..
 *          PACKET_START - 1 byte  - 0xAA ..
 *          packetSize   - 1 byte - includes PACKET_START, packetSize,
 *                         command, data size (may be zero), and checksum ..
 *          command      - 1 byte  - as defined in SerialCommands.def ..
 *          data         - variable size - optional ..
 *          checksum     - 1 byte
 *
 * \param   pDataPacket  - pointer to Incoming data packet from the Adapter Circular Buffer
 * \param   DataCount    - Received data size
 *
 * \return  None
 *
 * ========================================================================== */
static void ProcessAdapterUartStream(uint8_t *pDataPacket, uint16_t DataCount)
{
    uint16_t DataIndex;                /* Incoming packet data index */
    DataIndex         = 0;
    do
    {
        if ( NULL == pDataPacket )
        {
            break;
        }
        /* Received Zero data */
        if ( 0 == DataCount )
        {
            break;
        }
        /* Breaks on finding PACKET_START or breaks upon reaching last byte */
        while ( pDataPacket[DataIndex++] != PACKET_START )
        {
            BREAK_IF(DataIndex == DataCount);
        }
        /* Note:
           Scenario: Partial Frame is available and reached Start Packet
           Only in this case, Partail Frame is reconstructed and processed.
           It may skip the data of identified start packet.
           In order to avoid this specific scenario, decrementing DataIndex by 1 */
        if ( PartialResponse.IsFramePartial && pDataPacket[DataIndex-1] ==  PACKET_START )
        {
            /* Decrement DataIndex by 1, to avoid skip of only the first start packet of buffer */
            /* In the next iteration the first Start Packet bytes are framed and Processed */
            DataIndex--;
        }
        /* Assemble the Packet */
        PacketAssembler(pDataPacket,DataCount,DataIndex);
        if ( PartialResponse.IsFramePartial )
        {
            break;
        }
        ProcessAdapterPacket();
    } while ( DataIndex < DataCount );
}

/* ========================================================================== */
/**
 * \brief   Process the response from Adapter
 *
 * \details The serial commands are processed for the response hanndling.
 *
 * \note    Packet layout: ..
 *          PACKET_START - 1 byte  - 0xAA ..
 *          packetSize   - 1 byte - includes PACKET_START, packetSize,
 *                         command, data size (may be zero), and checksum ..
 *          command      - 1 byte  - as defined in SerialCommands.def ..
 *          data         - variable size - optional ..
 *          checksum     - 1 byte
 *
 * \param   Command   - Serial command received from Adapter
 * \param   pRecvData - Pointer to the data received from Adapter
 * \param   DataSize   - Data size
 *
 * \return  None
 *
 * ========================================================================== */
static void ProcessAdapterDataFlashPacket(uint8_t Command, uint8_t *pRecvData, uint8_t DataSize)
{
    ADAPTER_CMD_DATA  *pCmdData;         /* Pointer to the command data */
    uint32_t          TestChecksum;      /* Test the received checksum */
    SG_FORCE          *pSgForce;         /* Pointer to strain gauge force */
    ADAPTER_DEFN_REPO *pAdapterRepo;     /* Pointer to Adapter repository */

    pCmdData = &AdapterCmdData;
    pAdapterRepo = &AdapterDefnRepo;
    AdapterTimeoutTimerStop();
    switch (Command)
    {
        case SERIALCMD_GET_VERSION:
            pAdapterRepo->VerChecksumStatus = true;
            memcpy(&pAdapterRepo->TimeStamps, pRecvData, sizeof(AdapterTimeStamps));
            TestChecksum = SlowCRC16(0, (uint8_t *)(&pAdapterRepo->TimeStamps.TimeStampBoot), sizeof(pAdapterRepo->TimeStamps) - sizeof(pAdapterRepo->TimeStamps.Checksum));
            if (TestChecksum != pAdapterRepo->TimeStamps.Checksum)
            {
                pAdapterRepo->VerChecksumStatus = false;
                pAdapterRepo->TimeStamps.TimeStampMain = 0;
            }
            else
            {
              Log(DBG,"AdapterCom: Version = %d", pAdapterRepo->TimeStamps.TimeStampMain);
            }
            break;

        case SERIALCMD_HARDWARE_VERSION:
            memcpy(&pAdapterRepo->HardwareVersion, pRecvData, sizeof(pAdapterRepo->HardwareVersion));
            pAdapterRepo->HwVersionStatus = true;
            Log(DBG,"AdapterCom: HardwareVersion  = %d",pAdapterRepo->HardwareVersion);
            break;

        case SERIALCMD_ADAPT_LOADCELL_DATA:
            AdapterSGTimeoutTimerStart();
            pSgForce = AdapterDefnRepo.pStrainGaugeOldData;
            TM_Hook(HOOK_STRAINGUAGE1VAL,pRecvData);
            pSgForce->Current = ((uint16_t)pRecvData[1] << 8) | (uint16_t)pRecvData[0];

            /// \todo 12/17/2021 AR - The Min and Max Strain Gauge values in Pounds?
            if ( pSgForce->Current > pSgForce->Max )
            {
                pSgForce->Max = pSgForce->Current;
            }

            if ( pSgForce->Current < pSgForce->Min )
            {
                pSgForce->Min = pSgForce->Current;
            }

            AdapterDefnRepo.pStrainGaugeOldData = AdapterDefnRepo.pStrainGaugeNewData;
            AdapterDefnRepo.pStrainGaugeNewData = pSgForce;         // save pointer to the latest data
            pSgForce->NewDataFlag = true;                          // New data available

            //execute app specific callback functions
            if ( NULL != AdapterAppHandler[ADAPTER_APP_STRAIN_GAUGE_INDEX] )
            {
                /* execute a registered handler */
                AdapterAppHandler[ADAPTER_APP_STRAIN_GAUGE_INDEX](pSgForce);
            }
            /*  Enter ST_ERR_ADAPTER if an attached ADAPTER returns no strain gauge coefficients */
            if( SG_STATUS_ZERO_ADC_DATA & pSgForce->Status )
            {
                /* Raise or Clear fault whenever there is a Status change */
                if( !ErrorSet)
                {
                    FaultHandlerSetFault(ADAPTER_SGCOEFF_ZERO, SET_ERROR);
                    ErrorSet = SET_ERROR;
                }
            }
            else
            {
                if( ErrorSet )
                {
                    /* Clear Fault whenever there is a Status change */
                    FaultHandlerSetFault(ADAPTER_SGCOEFF_ZERO, CLEAR_ERROR);
                    ErrorSet = CLEAR_ERROR;
                }
            }
            /* Release the semaphore when we get the first response */
            if (!pCmdData->RespReceived)
            {
                pCmdData->RespReceived = true;
                if ( pCmdData->Cmd ==  SERIALCMD_ADAPT_LOADCELL_START_STREAM )
                {
                    Log(DBG,"AdapterCom: Adapter SG Stream Enabled");
                }
            }
            break;

        case SERIALCMD_ADAPT_EGIA_RELOAD_SWITCH_DATA:
            AdapterDefnRepo.SwitchData.TimeStamp = OSTimeGet();
            AdapterDefnRepo.SwitchData.State = (AdapterSwitchState)pRecvData[0];
            if (NULL != AdapterAppHandler[ADAPTER_APP_RELOADSWITCH_INDEX])
            {
                /* execute a registered handler */
                AdapterAppHandler[ADAPTER_APP_RELOADSWITCH_INDEX](&AdapterDefnRepo.SwitchData);
            }
            Log(DBG,"AdapterCom: Adapter EGIA Switch data received");
            break;

        case SERIALCMD_FLASH_READ:
            /* Flash commands not supported, return error */
            pRecvData[RECEIVE_STATUS_INDEX] = DATAFLASH_READ_FAILURE;
            //first byte is status, length of data is not part of response from adapter
            if(AM_STATUS_OK != ReadAdapterFactoryValues(&pRecvData[RECEIVE_DATA_INDEX], DataSize-1))
            {
                AdapterDefnRepo.AdapterFlashParmStatus = false;
                break;
            }
            AdapterDefnRepo.AdapterFlashParmStatus = true;
            Log(DBG,"AdapterCom: Adapter Flash parameters Read Successful");
            break;

        case SERIALCMD_FLASH_WRITE:
            /* Flash commands not supported, return error */
            pRecvData[RECEIVE_STATUS_INDEX] = DATAFLASH_READ_FAILURE;
            break;

        case SERIALCMD_BOOT_ENTER:
            /* Boot enter command - received as response to command from handle or on Adatper connected */
            if ( ADAPT_COMM_NOERROR == pRecvData[RECEIVE_STATUS_INDEX] )
            {
                AdapterDefnRepo.AdapterState = pRecvData[DataSize-1];
                Log(DBG,"AdapterCom: Adapter Boot Entered");

            }
            else
            {
                Log(DBG, "Not allowed to jump back to bootloader from the Main App");
            }
            break;

        case SERIALCMD_BOOT_QUIT:
            BREAK_IF(ADAPT_COMM_NOERROR != pRecvData[RECEIVE_STATUS_INDEX]);
            if (AdapterDefnRepo.AdapterState) // Adapter is in Boot mode
            {

                   AdapterDefnRepo.AdapterState = 0u; //Adapter in MainMode
                   Log(DBG,"AdapterCom: Adapter Main Entered");
            }
            else
            {
                //get Adapter Type
                AdapterDefnRepo.AdapterType = pRecvData[2];
                AdapterDefnRepo.AdapterType <<= 8;
                AdapterDefnRepo.AdapterType |= pRecvData[1];
                AdapterDefnRepo.AdapterTypeStatus = true;
                Log(DBG,"AdapterCom: AdapterType = 0x%x",AdapterDefnRepo.AdapterType);


            }
            break;

        case SERIALCMD_ADAPT_LOADCELL_STOP_STREAM:
            if ( ADAPT_COMM_NOERROR == pRecvData[RECEIVE_STATUS_INDEX] )
            {
              /*
                if (AM_STATUS_OK != AmFlushUart())
                {
                    break;
                }
              */
            }
            break;


        case SERIALCMD_ADAPT_OW_ENABLE:
            Log(DBG,"AdapterCom: Adapter Onewire switch Enabled");
            break;

        case SERIALCMD_ADAPT_OW_DISABLE:
            Log(DBG,"AdapterCom: Adapter Onewire switch Dissabled");
            break;

        default:
            break;
    }

    if (pCmdData->Cmd == Command)
    {
        pCmdData->RespReceived   = true;
        pCmdData->ResponseStatus = pRecvData[ADAPTER_RESPONSE_STATUS];    // Status
        pCmdData->RespData[0] 	 = pRecvData[ADAPTER_RESPONSE_LOWBYTE];   // Low byte
        pCmdData->RespData[1] 	 = pRecvData[ADAPTER_RESPONSE_HIGHBYTE];  // High byte
    }

}

/* ========================================================================== */
/**
 * \brief   Process the receive buffer and construct the packet
 *
 * \details This function construct the full packet or identifies the partial packet, copy the partial packet info and mark it as partial packet for further processing
 * \details Following scenarios are covered for packet assembly.
 *          i.  Full Packet is received, packet is constructed and sent for processing
 *          ii. Only part of packet is received with Packet size( second byte ). Marked as partial packet
 *          iii.Only Start packet is received without Packet size. Marked as partial packet
 *          Already Partial Frame available, received other part
 *          i.  If full packet size is available, marked it to no partial packet (or)
 *          ii. Still haven't received full packet, continuos to be partial frame

 * \note    Packet layout: ..
 *          PACKET_START - 1 byte  - 0xAA ..
 *          packetSize   - 1 byte - includes PACKET_START, packetSize,
 *                         command, data size (may be zero), and checksum ..
 *          command      - 1 byte  - as defined in SerialCommands.def ..
 *          data         - variable size - optional ..
 *          checksum     - 1 byte
 *
 * \param   pDataPacket   - Pointer to buffer received
 * \param   DataCount     - Total data received
 * \param   DataIndex     - Current Index to Data buffer
 *
 * \return  None
 *
 * ========================================================================== */
static void PacketAssembler(uint8_t *pDataPacket, uint16_t DataCount, uint16_t DataIndex)
{
    uint16_t  PacketSize;
    uint16_t RemainingBytesToCopy;
    PacketSize        = 0;
    do
    {
        /* Is partial frame is received in earlier buffer */
        if ( PartialResponse.IsFramePartial )
        {
            /* Scenario 1: Partial Frame might have packet size. Packet size is available in FrameSize
               Scenario 2: Partial Frame is having only Start packet, Packet size is the starting byte of received buffer */
            /* CurrentSize holds the size of the received Partial packet bytes
               If CurrentSize is 1 then received only Start packet, DataPacket[0] holds the Packet Size
               CurrentSize is greater than 1, then FrameSize holds the Packet size                          */
            PacketSize = (PartialResponse.CurrentSize == 1)?pDataPacket[0]:PartialResponse.FrameSize;
            /* Remaining Bytes to Copy to partial Frame */
            RemainingBytesToCopy = PacketSize - PartialResponse.CurrentSize;
            /* Limit Remaining bytes to DataCount */
            if ( RemainingBytesToCopy > DataCount )
            {
                RemainingBytesToCopy = DataCount;
            }
            memcpy(&PartialResponse.Buffer[PartialResponse.CurrentSize],&pDataPacket[0],RemainingBytesToCopy);
            /* Increment Current Size */
            PartialResponse.CurrentSize += RemainingBytesToCopy;
            /* Is current size equal to Packetsize reached || PacketSize is less than minimum size */
            if ( (uint16_t)PacketSize <=  PartialResponse.CurrentSize )
            {
                /* Clear Partial Frame */
                PartialResponse.IsFramePartial = false;
            }
            break;
        }

        /* Clear FrameSize, Current Size, Buffer */
        PartialResponse.FrameSize      = 0;
        PartialResponse.CurrentSize    = 0;
        PartialResponse.IsFramePartial = false;
        memset(PartialResponse.Buffer,0,ADAPTER_RX_BUFF_SIZE);

        /* Is current byte is not start packet and reached end of buffer, then Ignore */
        if ( (pDataPacket[DataIndex-1] != PACKET_START) && (DataIndex == DataCount) )
        {
            break;
        }
        /* START PACKET is at the last packet in the current Buffer */
        if ( (pDataPacket[DataIndex-1] == PACKET_START) && ( DataIndex == DataCount ) )
        {
            /* Copy last packet to Buffer and update CurrentSize to One */
            PartialResponse.Buffer[0]   = pDataPacket[DataIndex-1];
            PartialResponse.CurrentSize = 1;
            /* Set Partial Frame received */
            PartialResponse.IsFramePartial = true;
            break;
        }
        /* Get Packet Size */
        PacketSize = pDataPacket[DataIndex];
        /* Is remaining Data is less than the Packet size */
        if ( (DataCount - (DataIndex - 1) ) < PacketSize )
        {
            /* Copy to Buffer and set Frame received Partial */
            memcpy(&PartialResponse.Buffer[0],&pDataPacket[DataIndex-1],(DataCount - (DataIndex-1) ));
            PartialResponse.FrameSize      = PacketSize;
            PartialResponse.CurrentSize    = DataCount - (DataIndex-1);
            /* Set Partial Frame received */
            PartialResponse.IsFramePartial = true;
            break;
        }
        /* Reached here means: Complete packet is avaialble in received DataPacket Buffer */
        memcpy(&PartialResponse.Buffer[0],&pDataPacket[DataIndex-1],PacketSize);
        PartialResponse.CurrentSize    = PacketSize;
        PartialResponse.IsFramePartial = false;
    } while ( false );
}
/* ========================================================================== */
/**
 * \brief   Process the constructed packet and checks for Invalid commands, CRC, packet size
 *
 * \details This function process the constructed packet. Checks minimum packet, validity of command and CRC of data.
 *
 * \param   None
 *
 * \return  None
 *
 * ========================================================================== */
static void ProcessAdapterPacket(void)
{
    uint8_t PacketSize;
    uint8_t CmdIndex;
    uint8_t Command;
    uint8_t CheckSum;
    uint8_t ReceivedCheckSum;
    do
    {
        /* Get Packet size */
        PacketSize       = PartialResponse.Buffer[1];
        /* Ignore data if Packet size less than Minimum Size */
        if ( PacketSize < MIN_PACKET_SIZE )
        {
            break;
        }
        /* Get Command Index */
        CmdIndex = CMD_INDEX_OFFSET;
        /* Copy Command */
        Command  = PartialResponse.Buffer[CmdIndex];
        /* Strip off the Command Byte Mask */
        Command  &= ADAPTER_COMMAND_MASK;
        /* Ignore if Command is UNKNOWN or greater than expected commands*/
        if ( (Command == SERIALCMD_UNKNOWN) || (Command >= SERIALCMD_COUNT) )
        {
            break;
        }
        CheckSum = CRC8(0, (&PartialResponse.Buffer[0]), (PacketSize - sizeof(uint8_t)));
        ReceivedCheckSum = PartialResponse.Buffer[PacketSize - sizeof(uint8_t)];
        /* Test Hook to fail checksum */
        TM_Hook(HOOK_ADAPTERCRCFAIL, &CheckSum);
        /* Ignore if Received checksum doesn't match calculated checksum */
        if (ReceivedCheckSum != CheckSum)
        {
            break;
        }
        /* Pass the packet to the command handler */
        ProcessAdapterDataFlashPacket(Command, &PartialResponse.Buffer[CmdIndex + CMD_DATA_OFFSET], PacketSize - PACKET_OVERHEAD);
    } while ( false );
}
/* ========================================================================== */
/**
 * \brief   Send command to the Adapter
 *
 * \details Prepare a command packet and send to the Adapter.
 *
 * \note    Packet layout:
 *          PACKET_START - 1 byte  - 0xAA
 *          packetSize   - 1 byte - includes PACKET_START, packetSize,
 *                         command, data size (may be zero), and checksum
 *          command      - 1 byte  - as defined in SerialCommands.def
 *          data         - variable size - optional
 *          checksum     - 1 byte
 *
 * \param   Command      - Command to send
 * \param   pDataOut     - Pointer to the data to send
 * \param   DataSize     - Data size
 * \param   CmdMask      - Byte mask for the command
 *
 * \return  None
 *
 * ========================================================================== */
static void SendAdapterUartCommand(uint8_t Command, uint8_t *pDataOut, uint8_t DataSize, uint8_t CmdMask)
{
    uint8_t     Checksum;                         /* Checksum of the packet */
    uint8_t     StartByte;                        /* Packet start byte */
    uint8_t     PacketSize;                       /* Data packet size */
    uint16_t    DataCount;                        /* Data count */
    uint8_t     Count;
    uint8_t     *pData;
    memset(adapterOutgoingData, 0x00, ADAPTER_TX_BUFF_SIZE);

    DataCount = 0;
    pData = pDataOut;

    do
    {

      StartByte = PACKET_START;
      adapterOutgoingData[DataCount++] = StartByte;

      PacketSize = DataSize + PACKET_OVERHEAD;
      adapterOutgoingData[DataCount++] =  PacketSize;

      Command = Command | CmdMask;
      adapterOutgoingData[DataCount++] = Command;

      if ((DataSize > 0) && (pData != NULL))
      {
          for(Count = 0; Count < DataSize; Count++)
          {
            adapterOutgoingData[DataCount++] = *pData++;
          }
      }

        /* Calculate Checksum for the packet */
        Checksum = CRC8(0, &StartByte, sizeof(StartByte));
        Checksum = CRC8(Checksum, &PacketSize, sizeof(PacketSize));
        Checksum = CRC8(Checksum, (uint8_t*)&Command, sizeof(Command));
        Checksum = CRC8(Checksum, pDataOut, DataSize);


      adapterOutgoingData[DataCount++] = Checksum;

      pAdapterComm->Send(adapterOutgoingData, &DataCount);


    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Enter Adapter to Main application
 *
 * \details Enable the Adapter to Main App by sending the command SERIALCMD_BOOT_QUIT
 *
 * \param   Pointer to command mask
 *
 * \return  AM_STATUS       - Function status
 * \retval  AM_STATUS_OK    - Commad and response processed successfully
 * \retval  AM_STATUS_ERROR - Error in Commad and response processing
 *
 * ========================================================================== */
static AM_STATUS AdapterDefnEnterMain(uint8_t *CmdMask)
{
    AM_STATUS    Status;              /* Function status */
    uint8_t      RespStatus;          /* Response Status */
    uint8_t      RespData[2];          /* Retry Count */
    uint8_t      Mask;

    if ( NULL == CmdMask )
    {
         Mask = COMMAND_BYTE_MASK_BOOTLOADER;
         CmdMask = &Mask;
    }

    Status = AM_STATUS_ERROR;

    Status = AdapterProcessCmdResp(SERIALCMD_BOOT_QUIT, RespData, 0, *CmdMask, &RespStatus);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Enable/Disable the Adapter one wire
 *
 * \details Enable the Adapter one wire by sending the command SERIALCMD_ADAPT_OW_ENABLE
 *          Disable the one wire by issuing the command SERIALCMD_ADAPT_OW_DISABLE
 *
 * \param   Enable - Boolean true value if one wire should be enabled
 *
 * \return  AM_STATUS       - Function status
 * \retval  AM_STATUS_OK    - Commad and response processed successfully
 * \retval  AM_STATUS_ERROR - Error in Commad and response processing
 *
 * ========================================================================== */
static AM_STATUS AdapterEnableOneWire(bool Enable)
{
    AM_STATUS    Status;  /* Function status */
    uint8_t Command;      /* Command to send */
    uint8_t RespStatus;   /* Response status */

    Command = Enable ? SERIALCMD_ADAPT_OW_ENABLE : SERIALCMD_ADAPT_OW_DISABLE;

    Status = AdapterProcessCmdResp(Command, NULL, 0, AdapterCmdMask[AdapterDefnRepo.AdapterState], &RespStatus);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Adapter Boot enter command
 *
 * \details Enter the boot enter by issuing the command SERIALCMD_BOOT_ENTER
 *          If the adapter is in MainApp, resend the command by changing the mask
 *          to COMMAND_BYTE_MASK_MAINAPP
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in entering bootloader
 * \retval  AM_STATUS_ERROR  - Error in entering bootloader
 *
 * ========================================================================== */
static AM_STATUS AdapterBootEnter(void)
{
    uint8_t    RespStatus;       /* Response Status */
    AM_STATUS  Status;           /* Function status */
    uint8_t    CmdMask;          /* Command Mask */

    Status = AM_STATUS_ERROR;
    CmdMask = AdapterCmdMask[AdapterDefnRepo.AdapterState];//COMMAND_BYTE_MASK_BOOTLOADER;


    Status = AdapterProcessCmdResp(SERIALCMD_BOOT_ENTER, NULL, 0, CmdMask, &RespStatus);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Enable/Disable EGIA adapter event switching
 *
 * \details Enable the EGIA adapter event switching by using the command
 *          SERIALCMD_ADAPT_EGIA_RELOAD_SWITCH_START_EVENTS.
 *          Disable the switching using SERIALCMD_ADAPT_EGIA_RELOAD_SWITCH_STOP_EVENTS
 *
 * \param   Enable  - Enable/Disable flag
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in Enable/Disable events
 * \retval  AM_STATUS_ERROR  - Error in Enable/Disable events
 *
 * ========================================================================== */
static AM_STATUS AdapterEGIASwitchEventsEnable(bool Enable)
{
    uint8_t Command;      /* Command to send */
    uint8_t RespStatus;   /* Response status */

    Command = Enable ? SERIALCMD_ADAPT_EGIA_RELOAD_SWITCH_START_EVENTS : SERIALCMD_ADAPT_EGIA_RELOAD_SWITCH_STOP_EVENTS;
    return (AdapterProcessCmdResp(Command, NULL, 0, AdapterCmdMask[AdapterDefnRepo.AdapterState], &RespStatus));
}

/* ========================================================================== */
/**
 * \brief   Get EGIA adapter switch data
 *
 * \details Get curAdapterSendCmdrent state of the adapter switch. For non-intelligent reloads
 *          the state of switch is used to detect presence/absence
 *
 * \param   <None>
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in command execution
 * \retval  AM_STATUS_ERROR  - Error in command execution
 *
 * ========================================================================== */
static AM_STATUS AdapterEGIAGetSwitchData(void)
{
    uint8_t RespStatus;   // Response status

    return (AdapterProcessCmdResp(SERIALCMD_ADAPT_EGIA_RELOAD_SWITCH_DATA, NULL, 0, AdapterCmdMask[AdapterDefnRepo.AdapterState], &RespStatus));
}

/* ========================================================================== */
/**
 * \brief   Start/Stop the Strain gauge streaming
 *
 * \details Start the Strain gauge streaming by using the command SERIALCMD_ADAPT_LOADCELL_START_STREAM.
 *          Stop the streaming using SERIALCMD_ADAPT_LOADCELL_STOP_STREAM
 *
 * \param   Start  - Start/Stop flag
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in Start/Stop streaming
 * \retval  AM_STATUS_ERROR  - Error in Start/Stop streaming
 *
 * ========================================================================== */
static AM_STATUS AdapterForceStreamStart(bool Start)
{
    uint8_t    Command;       /* Command to send */
    uint8_t    RespStatus;    /* Response status */
    AM_STATUS  Status;        /* Function status */

    Status = AM_STATUS_ERROR;

    do
    {
        Command = Start ? SERIALCMD_ADAPT_LOADCELL_START_STREAM : SERIALCMD_ADAPT_LOADCELL_STOP_STREAM;
        Status = AdapterProcessCmdResp(Command, NULL, 0, AdapterCmdMask[AdapterDefnRepo.AdapterState], &RespStatus);
        if (AM_STATUS_OK != Status)
        {
            break;
        }

        if (SERIALCMD_ADAPT_LOADCELL_STOP_STREAM != Command)
        {
            break;
        }

        /* Flush the UART once the streaming stopped */
        if (AM_STATUS_OK != AmFlushUart())
        {
            Status = AM_STATUS_ERROR;
            break;
        }

        Status = AM_STATUS_OK;

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   serial flash erase
 *
 * \details erase adapter program flash memory
 * \param   pData - pointer to command data
            DataSize - size of command data
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in command execution
 * \retval  AM_STATUS_ERROR  - Error in erasing flash
 *
 * ========================================================================== */
static AM_STATUS AdapterFlashErase(uint8_t *pData, uint8_t DataSize)
{
    uint8_t   RespStatus;       /* Response status */
    AM_STATUS Status;           /* Function status */

     Status = AM_STATUS_ERROR;

    /* Proceed sending command */
    Status = AdapterProcessCmdResp(SERIALCMD_FLASH_ERASE, pData, DataSize, AdapterCmdMask[AdapterDefnRepo.AdapterState], &RespStatus);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   serial flash write
 *
 * \details write adapter program flash memory
 * \param   pData - pointer to command data
            DataSize - size of command data
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in command execution
 * \retval  AM_STATUS_ERROR  - Error in writing flash
 *
 * ========================================================================== */
static AM_STATUS AdapterFlashWrite(uint8_t *pData, uint8_t DataSize)
{
    uint8_t   RespStatus;       /* Response status */
    AM_STATUS Status;           /* Function status */

    Status = AM_STATUS_ERROR;

    /* Proceed sending command */
    Status = AdapterProcessCmdResp(SERIALCMD_FLASH_WRITE, pData, DataSize, AdapterCmdMask[AdapterDefnRepo.AdapterState], &RespStatus);


    return Status;
}

/* ========================================================================== */
/**
 * \brief   serial flash set version
 *
 * \details write adapter main app version
 * \param   pData - pointer to command data
            DataSize - size of command data
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in command execution
 * \retval  AM_STATUS_ERROR  - Error in writing flash
 *
 * ========================================================================== */
static AM_STATUS AdapterWriteVersion(uint8_t *pData, uint8_t DataSize)
{
    uint8_t   RespStatus;       /* Response status */
    AM_STATUS Status;           /* Function status */

    Status = AM_STATUS_ERROR;

    /* Proceed sending command */
    Status = AdapterProcessCmdResp(SERIALCMD_SET_VERSION, pData, DataSize, AdapterCmdMask[AdapterDefnRepo.AdapterState], &RespStatus);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Write Adapter Flash
 *
 * \details Write to the Adapter Flash memory. the function is implemented in blocking manner
 *
 * \param   pMainAppPtr  - Pointer to the adapter main app binary header
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error
 * \retval  AM_STATUS_ERROR  - Error in updateing adpater app
 *
 * ========================================================================== */
static AM_STATUS WriteAdapterFlash(void)
{
    uint32_t BlockIndex;
    uint32_t SourceOffset;
    uint32_t DataSize;
    uint32_t DestPtr;
    uint32_t EndDestPtr;
    uint32_t BytesRead;

    AM_STATUS Status;
    BLOB_HANDLER_STATUS BlobStatus;
    uint8_t DecryptBlockAlignBytes;
    uint8_t DecryptBlockAlignOffset;

    PROGRAM_BLOCK_INFO  NextBlockInfo;

    /*Initialize Variables*/
    memset(AESReadBuffer,0x00,sizeof(AESReadBuffer));
    Status          = AM_STATUS_ERROR;
    BlobStatus      = BLOB_STATUS_ERROR;
    BlockIndex      = BlobPointers.StoredEgiaHeader.BlockCount;

    DestPtr         = 0x00;
    SourceOffset    = 0x00;
    BytesRead       = 0x00;

    DecryptBlockAlignOffset = sizeof(PROGRAM_BLOCK_INFO);

   do
   {
       BlobStatus = L4_BlobRead(BLOB_SECTION_EGIA_MAIN, AESReadBuffer, (SourceOffset - IV_OFFSET), (AES_BLOCKLEN + IV_OFFSET), &BytesRead);
       if ( BLOB_STATUS_OK != BlobStatus )
       {
          break;
       }

       if( BlobPointers.StoredBlobHeader.Encryption.bit.EGIAEncrypted )
       {
           DecryptBinaryBuffer(AESReadBuffer,AES_BLOCKLEN,true);
       }
       memcpy((void*)&NextBlockInfo,(void*)&AESReadBuffer[IV_OFFSET],sizeof(PROGRAM_BLOCK_INFO));

       // Source of the data locally (from the blob, on the MK20)
       // Destination of the data on the adapter
       DestPtr     = NextBlockInfo.AbsoluteAddress;
       EndDestPtr  = DestPtr + NextBlockInfo.Length;

       while (DestPtr < EndDestPtr)
       {
         // Align the read data to 16 bytes for decryption
          DecryptBlockAlignBytes = (AES_BLOCKLEN*5) - ADAPTER_DATA_BLOCK_SIZE;

          // Clip DataSize to a max of ADAPTER_DATA_BLOCK_SIZE
          if ( ( EndDestPtr - DestPtr ) >= ADAPTER_DATA_BLOCK_SIZE )
          {
             DataSize = ADAPTER_DATA_BLOCK_SIZE;
          }
          else
          {
             DataSize = EndDestPtr - DestPtr;
          }

          memset(AdapterFlashUpdateBuffer,0x00,sizeof(AdapterFlashUpdateBuffer));

          BlobStatus = L4_BlobRead(BLOB_SECTION_EGIA_MAIN, AdapterFlashUpdateBuffer, (SourceOffset - IV_OFFSET), (DataSize + DecryptBlockAlignBytes + IV_OFFSET), &BytesRead);

          if (BLOB_STATUS_OK != BlobStatus)
          {
             break;
          }

          if( BlobPointers.StoredBlobHeader.Encryption.bit.EGIAEncrypted )
          {
              DecryptBinaryBuffer(AdapterFlashUpdateBuffer , BytesRead,true);
          }

          *((uint32_t*)(AdapterFlashUpdateBuffer + IV_OFFSET + DecryptBlockAlignOffset - DEST_ADDR_SIZE )) = DestPtr;    //get the address

          AdapterTimeoutTimerStart();
          do
          {
              Status = AdapterFlashWrite((AdapterFlashUpdateBuffer + IV_OFFSET + DecryptBlockAlignOffset - DEST_ADDR_SIZE ) , (DataSize + DEST_ADDR_SIZE));
              OSTimeDly(MSEC_3);
          } while(Status == AM_STATUS_WAIT);

          if ( AM_STATUS_OK != Status )
          {
              break;
          }
          SourceOffset += DataSize;
          DestPtr += DataSize;
       }

       if (AM_STATUS_OK != Status || BLOB_STATUS_OK != BlobStatus)
       {
           Status = AM_STATUS_ERROR;
           break;
       }

       BlockIndex -=1;

   } while (BlockIndex != 0);

   return Status;
}

/* ========================================================================== */
/**
 * \brief   Update adapter app
 *
 * \details Update the adapter main application. The fucntion is a blocking call.
 *          i.e, will return only after updating complete
 *
 * \param   pAdapterVersion  - Pointer to the Version timestamps data
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error
 * \retval  AM_STATUS_ERROR  - Error in updateing adpater app
 *
 * ========================================================================== */
static AM_STATUS AdapterUpdateMainApp(AdapterTimeStamps *pAdapterVersion)
{
   AM_STATUS Status;
   uint32_t BlobAdapterAppTimeStamp;
   uint8_t startSector;
   uint8_t endSector;
   uint8_t sectorCount;
   uint8_t AdapterFlashUpdateBuffer[2];
   uint8_t DataSize;

   DataSize = 0;
   Status = AM_STATUS_OK;

   L4_GetBlobPointers(&BlobPointers);

   BlobAdapterAppTimeStamp = BlobPointers.StoredBlobHeader.EgiaTimestamp;

   if ( (pAdapterVersion->TimeStampMain == 0xFFFFFFFF) || ( pAdapterVersion->TimeStampMain != BlobAdapterAppTimeStamp ))
   {
      SecurityLog("Adapter Software Update: Started");

      // Set up to erase flash between programLowAddress and programHighAddress
      startSector = BlobPointers.StoredEgiaHeader.ProgramLowAddress / 1024;
      endSector   = BlobPointers.StoredEgiaHeader.ProgramHighAddress / 1024;
      sectorCount = endSector - startSector;

      AdapterFlashUpdateBuffer[DataSize++] = startSector;
      AdapterFlashUpdateBuffer[DataSize++] = sectorCount;

      // Erase adapter flash
      do
      {
          Status = AdapterFlashErase(AdapterFlashUpdateBuffer, DataSize);
          OSTimeDly(MSEC_3);
      } while(Status == AM_STATUS_WAIT);

      if ( AM_STATUS_ERROR != Status)
      {
          Log(DBG, "Adapter Flash Erased Successfully!");

          // write code to flash
          Status = WriteAdapterFlash();

          if (AM_STATUS_ERROR != Status)
          {
              Log(DBG, "Adapter FW Written Successfully!");

              *(uint32_t *)(AdapterFlashUpdateBuffer) = BlobAdapterAppTimeStamp;

			  AdapterTimeoutTimerStart();

              // Set Adapter FW timestamp
              do
              {
                  Status = AdapterWriteVersion(AdapterFlashUpdateBuffer,sizeof(BlobAdapterAppTimeStamp));
                  OSTimeDly(MSEC_3);
              } while(Status == AM_STATUS_WAIT);

              if (Status != AM_STATUS_OK)
              {
                  Log(DBG, " Adapter Version Write failed");
              }
          }
      }
       SecurityLog("Adapter Software Update : completed. Status = %d",Status);
   }
   else
   {
      Log(DBG,"AdapterDef: Adapter Software is Up to Date");
   }
   if (AM_STATUS_OK != Status)
   {
      Status = AM_STATUS_ERROR_UPGRADE;
   }

   return Status;
}

/* ========================================================================== */
/**
 * \brief   Get the Version Timestamps
 *
 * \details Copy the Version Timestamps from Adapter definiton data
 *
 * \param   pAdapterVersion  - Pointer to the Version Timestamps data
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in getting the Version Timestamps
 * \retval  AM_STATUS_ERROR  - Error in getting the Version Timestamps
 *
 * ========================================================================== */
static AM_STATUS AdapterVersionGet(void)
{
    uint8_t RespStatus;                   /* Response status */
    AM_STATUS Status;                     /* Function status */

    Status = AM_STATUS_ERROR;

    /* Otherwise proceed sending command */
    Status = AdapterProcessCmdResp(SERIALCMD_GET_VERSION, NULL, 0, COMMAND_BYTE_MASK_BOOTLOADER, &RespStatus);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Get the Hardware version
 *
 * \details Copy the Hardware version from Adapter definiton data
 *
 * \param   pVersion  - Pointer to the Hardware version
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in getting the Hardware version
 * \retval  AM_STATUS_ERROR  - Error in getting the Hardware version
 *
 * ========================================================================== */
static AM_STATUS AdapterHwVersionGet(void)
{
    uint8_t            RespStatus;       /* Response status */
    AM_STATUS          Status;           /* Function status */
    Status = AM_STATUS_ERROR;

    Status = AdapterProcessCmdResp(SERIALCMD_HARDWARE_VERSION, NULL, 0, COMMAND_BYTE_MASK_MAINAPP, &RespStatus);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Get the strain gauge force
 *
 * \details Copy the strain gauge force from Adapter definition data
 *
 * \param   pSGData  - Pointer to the strain gauge data
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in getting the strain gauge data
 * \retval  AM_STATUS_ERROR  - Error in getting the strain gauge data
 *
 * ========================================================================== */
static SG_STATUS AdapterForceGet(SG_FORCE *pSGData)
{
    SG_FORCE *pStrainGaugeData; /* Pointer to strain gauge data */

    do
    {
        if ( NULL == pSGData )
        {
            break;
        }

        pStrainGaugeData = AdapterDefnRepo.pStrainGaugeNewData; /* always read from the new data pointer */

        if ( NULL == !pStrainGaugeData ) //valid data pointer present
        {
             if ( pStrainGaugeData->NewDataFlag ) //no active writes are going on
             {
                  pStrainGaugeData->NewDataFlag = false; //finished reading
             }
             else
             {
                 pStrainGaugeData->Status |= SG_STATUS_STALE_DATA;
             }

             memcpy(pSGData, pStrainGaugeData, sizeof(*pStrainGaugeData));
        }

    } while ( false );

    return pStrainGaugeData->Status;
}

/* ========================================================================== */
/**
 * \brief   Get the adapter switch state
 *
 * \details Copy the adapter switch data from Adapter definiton data
 *
 * \param   pSwitch  - Pointer to the adapter switch data
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in getting the adapter switch data
 * \retval  AM_STATUS_ERROR  - Error in getting the adapter switch data
 *
 * ========================================================================== */
static AM_STATUS AdapterGetSwitchState(SWITCH_DATA *pSwitch)
{
    ADAPTER_DEFN_REPO  *pAdapterRepo;         /* Pointer to Adapter repository */
    AM_STATUS          Status = AM_STATUS_ERROR; /* Function Status */

    do
    {
        if ( NULL == pSwitch )
        {
            break;
        }

        pAdapterRepo = &AdapterDefnRepo;
        memcpy(pSwitch, &pAdapterRepo->SwitchData, sizeof(pAdapterRepo->SwitchData));
        Status = AM_STATUS_OK;

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Tare the Force value
 *
 * \details Tare the currrent strain gauge force value
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in force tare
 * \retval  AM_STATUS_ERROR  - Error in force tare
 *
 * ========================================================================== */
static AM_STATUS AdapterForceTare(void)
{
    ADAPTER_DEFN_REPO *pAdapterRepo;      /* Pointer to Adapter repository */
    SG_FORCE          *pStrainGaugeData;  /* Pointer to the Strain gauge data */
    AM_STATUS         Status;             /* Function status */

    Status = AM_STATUS_ERROR;

    pAdapterRepo = &AdapterDefnRepo;
    pStrainGaugeData = AdapterDefnRepo.pStrainGaugeNewData;

    if ( NULL != pStrainGaugeData )
    {
         pAdapterRepo->ForceTareOffset = pStrainGaugeData->Current;
    }
    Status = AM_STATUS_OK;

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Reset Min and Max Strain gauge value
 *
 * \details Reset Min and Max Strain gauge value to current value
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in limits reset
 * \retval  AM_STATUS_ERROR  - Error in limits reset
 *
 * ========================================================================== */
static AM_STATUS AdapterForceLimitsReset(void)
{
    SG_FORCE          *pStrainGaugeData;  /* Pointer to the Strain gauge data */
    AM_STATUS         Status;             /* Function status */

    Status = AM_STATUS_ERROR;
    pStrainGaugeData = AdapterDefnRepo.pStrainGaugeNewData;

    pStrainGaugeData->Min = pStrainGaugeData->Current;
    pStrainGaugeData->Max = pStrainGaugeData->Current;

    Status = AM_STATUS_OK;

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Adapter EEPROM read
 *
 * \details Read Adapter 1-wire EEPROM memory
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in reading EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in reading EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS AdapterEepRead(void)
{
    AM_STATUS Status;         /* Function status */
    OW_EEP_STATUS OwEepStatus;
    uint8_t *pData;
    uint16_t CalcCRC;

    Status = AM_STATUS_OK;
    CalcCRC = 0;
    pData = (uint8_t*)&AdapterInterface.Data;

    OwEepStatus  = L3_OneWireEepromRead(AdapterAddress, 0, pData);
    OwEepStatus |= L3_OneWireEepromRead(AdapterAddress, 1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);

    do
    {
        if (OW_EEP_STATUS_OK != OwEepStatus)
        {
            Status = AM_STATUS_ERROR;
            break;
        }
        /*Calculate the CRC*/
        CalcCRC = CRC16(CalcCRC, pData,(size_t)(ONEWIRE_MEMORY_TOTAL_SIZE - sizeof( AdapterInterface.Data.Crc )));
        /*Check for Data integrity*/
        if (AdapterInterface.Data.Crc != CalcCRC)
        {
            Log(DBG, "Adapter EEPRead: EEPROM CRC validation failed");
            Status = AM_STATUS_ERROR;
        }

    } while( false );


    return Status;
}

/* ========================================================================== */
/**
 * \brief   Adapter EEPROM write
 *
 * \details Write Adapter 1-wire EEPROM memory
 *
 * \param   pData  - Pointer to the data to write
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in writing EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in writing EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS AdapterEepWrite(uint8_t *pData)
{
    AM_STATUS Status;           /* Function status */
    OW_EEP_STATUS OwEepStatus;  /* 1-Wire Eep function call status */

    MemoryLayoutGenericAdapter_Ver2 *pOneWireMemFormat;  /* Pointer to basic one wire memory format */

    Status = AM_STATUS_OK;

    pOneWireMemFormat = (MemoryLayoutGenericAdapter_Ver2*)pData;

    /*Update the calculated CRC in the WriteData buffer*/
    pOneWireMemFormat->Crc = CRC16(0, pData,(size_t)(ONEWIRE_MEMORY_TOTAL_SIZE - sizeof(pOneWireMemFormat->Crc)));

    OwEepStatus  = L3_OneWireEepromWrite(AdapterAddress, 0, pData);
    OwEepStatus |= L3_OneWireEepromWrite(AdapterAddress, 1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);

    if (OW_EEP_STATUS_OK != OwEepStatus)
    {
        Status = AM_STATUS_ERROR;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Flush out RAM content to EEPROM
 *
 * \details Write Adapter 1-wire EEPROM memory with the local data
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function return Status
 *
 * ========================================================================== */
static AM_STATUS AdapterEepUpdate(void)
{
    AdapterInterface.Status = AdapterEepWrite((uint8_t*)&AdapterInterface.Data);
    return AdapterInterface.Status;
}


/* ========================================================================== */
/**
 * \brief Switch the Adapter supply ON
 *
 * \details Sets the GPIO_EN_5V pin thereby enabling supply for Adapter
 *
 * \param <None>
 *
 * \return  AM_STATUS - Function return Status
 *
 * ========================================================================== */
static AM_STATUS AdapterSupplyON(void)
{
    AM_STATUS Status;
    Status = AM_STATUS_OK;
    if (GPIO_STATUS_OK !=L3_GpioCtrlSetSignal(GPIO_EN_5V))
    {
        Log(ERR, "AdapterDef: GPIO_EN_5V Set Failed");
        Status = AM_STATUS_ERROR;
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief Switch the Adapter supply OFF
 *
 * \details Clears the GPIO_EN_5V pin thereby dissabling supply for Adapter
 *
 * \param <None>
 *
 * \return  AM_STATUS - Function return Status
 *
 * ========================================================================== */
static AM_STATUS AdapterSupplyOFF(void)
{
    AM_STATUS Status;
    Status = AM_STATUS_OK;
    if (GPIO_STATUS_OK != L3_GpioCtrlClearSignal(GPIO_EN_5V))
    {
        Log(ERR, "AdapterDef: GPIO_EN_5V Clear Failed");
        Status = AM_STATUS_ERROR;
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief Adapter Reset Request
 *
 * \details the function performs a Adapter power cycle by switching OFF and ON the
 *              Adapter power ( GPIO_EN_5V pin )
 *
 * \param <None>
 *
 * \return  AM_STATUS - Function return Status
 *
 * ========================================================================== */
static AM_STATUS AdapterRestart(void)
{
    AM_STATUS Status;
    bool Adap5VStatus;
    static uint32_t OffTime;
    Status = AM_STATUS_WAIT;
    do
    {   // Read the Adapter 5v signal status (GPIO_EN_5V)
        if (GPIO_STATUS_OK != L3_GpioCtrlGetSignal(GPIO_EN_5V, &Adap5VStatus))
        {
            Log(ERR, "AdapterDef: GPIO_EN_5V Read Failed");
            Status = AM_STATUS_ERROR;
            break;
        }
        // if power is ON switch off the Adapter 5V and return Status as Wait
        if (Adap5VStatus)
        {
            if (AM_STATUS_OK != AdapterSupplyOFF())
            {
                Log(ERR, "AdapterDef: Failed to switch Adapter Supply Off");
                Status = AM_STATUS_ERROR;
                break;
            }
            OffTime = SigTime();
          Log(DBG, "Adapter Power OFF");
        }
        // if Adapter 5v is OFF and Offtime is not zero check for OFF time elapsed and Switch on the Adapter 5V
        // return status as Wait if Offtime not elapsed, else return OK also update AdapterState to ADAPTER_IN_BOOT
        if (!Adap5VStatus && OffTime)
        {
            BREAK_IF(ADAP_SUPPLYOFFTIME > (SigTime()- OffTime));
            if (AM_STATUS_OK != AdapterSupplyON())
            {
                Log(ERR, "AdapterDef: Failed to switch Adapter Supply Off");
                Status = AM_STATUS_ERROR;
                break;
            }
            if (GPIO_STATUS_OK != L3_GpioCtrlGetSignal(GPIO_EN_5V, &Adap5VStatus))
            {
                Log(ERR, "AdapterDef: GPIO_EN_5V Read Failed");
                Status = AM_STATUS_ERROR;
                break;
            }

            OffTime = 0;
            Status = AM_STATUS_OK;
            AdapterDefnRepo.AdapterState = ADAPTER_IN_BOOT;
            Log(DBG, "Adapter Power ON");
        }
    } while (false);
    return Status;
}

/* ========================================================================== */
/**
 * \brief Adapter Communication manager
 *
 * \details handles the Requested command by calling appropriate function
 *
 * \param ADAP_COM_MSG* - pointer to Com Command request message
 *
 * \return AM_STATUS - Function return Status
 *
 * ========================================================================== */


AM_STATUS AdapterComManager(ADAPTER_COM_MSG *pAdapCmd)
{
    AM_STATUS   AmStatus;     /* Function status */
    uint8_t CmdMask;
    AmStatus = AM_STATUS_ERROR;

    //call appropriate function
    switch(pAdapCmd->Cmd)
    {
        case ADAPTER_ENTERBOOT:
            AmStatus = AdapterBootEnter();
            break;

        case ADAPTER_ENTERMAIN:
        case ADAPTER_GET_TYPE:
            CmdMask = AdapterCmdMask[AdapterDefnRepo.AdapterState];
            if(ADAPTER_GET_TYPE == pAdapCmd->Cmd)
            {
                CmdMask = COMMAND_BYTE_MASK_MAINAPP;
            }
            AmStatus = AdapterDefnEnterMain(&CmdMask);
            break;

        case ADAPTER_GET_VERSION:
            AmStatus = AdapterVersionGet();
            break;

        case ADAPTER_UPDATE_MAIN:
            AmStatus = AdapterUpdateMainApp(&AdapterDefnRepo.TimeStamps);
            break;

        case ADAPTER_GET_FLASHDATA:
            AmStatus = Read_AdapterDataFlash();
            break;

        case ADAPTER_GET_HWVERSION:
            AmStatus = AdapterHwVersionGet();
            break;

        case ADAPTER_ENABLE_ONEWIRE:
            AmStatus = AdapterEnableOneWire(true);
            break;

        case ADAPTER_DISBALE_ONEWIRE:
            AmStatus = AdapterEnableOneWire(false);
            break;
        case ADAPTER_ENABLE_SWEVENTS:
            AmStatus = AdapterEGIASwitchEventsEnable(true);
            break;
        case ADAPTER_DISSABLE_SWENVENTS:
            AmStatus = AdapterEGIASwitchEventsEnable(false);
            break;
        case ADAPTER_GET_EGIA_SWITCHDATA:
            AmStatus = AdapterEGIAGetSwitchData();
            break;
        case ADAPTER_START_SGSTREAM:
            AmStatus = AdapterForceStreamStart(true);
            break;
        case ADAPTER_STOP_SGSTREAM:
            AmStatus = AdapterForceStreamStart(false);
            break;
        case ADAPTER_RESTART:
            AmStatus = AdapterRestart();
            break;
        default:
            AmStatus = AM_STATUS_ERROR;
            break;
    }

    return AmStatus;
}


/* ========================================================================== */
/**
 * \brief   Gets the next available com memory slot
 *
 * \details  Function to check if the request queue has a place to hold the request
 *
 * \param   < None >
 *
 * \return  ADAP_COM_MSG* - Ptr to the next slot of memory, otherwise NULL
 *
 * ========================================================================== */
static ADAPTER_COM_MSG *GetNextAdapComReqMsgSlot(void)
{
    uint8_t             OsError;                           /* OS Error Variable */
    static uint8_t      ComMsgReqPoolIndex;                   /* Message Req Pool Index */
    ADAPTER_COM_MSG        *pRequest;                         /* Pointer to the next message */
    pRequest = NULL;

    /* Mutex lock  */
    OSMutexPend(pAdapterDefnMutex, OS_WAIT_FOREVER, &OsError);
    if(OS_ERR_NONE != OsError)
    {
        Log(ERR, "GetNextAmReqMsgSlot: OSMutexPend error");
        /* /// \todo 03/15/2021 PR Add exception handler here */
    }

    /* Is the current index pointing to max storage? */
    if (++ComMsgReqPoolIndex >= MAX_ADAPTERQ_REQUESTS)
    {
        /* Circle back to first */
        ComMsgReqPoolIndex = 0;
    }
    ComMsgReqPool[ComMsgReqPoolIndex].Cmd = ADAPTER_NO_COMMAND;
    ComMsgReqPool[ComMsgReqPoolIndex].DelayInMsec = 0;

    /* Get the slot based on the Index */
    pRequest = &ComMsgReqPool[ComMsgReqPoolIndex];

    /* Release the Mutex */
    OSMutexPost(pAdapterDefnMutex);

    return pRequest;
}

/* ========================================================================== */
/**
 * \brief Publish the Adpater com signal
 *
 * \details Function publishes the Adapter com status signals
 *
 * \param SIGNAL - signal to be published
 *
 * \param ADAPTERCOMMANDS - Command information
 *
 * \return  None
 *
 * ========================================================================== */
static void AdapterComSigPublish(SIGNAL Sig, ADAPTER_COMMANDS AdapterCmd)
{
    QEVENT_ADAPTERCOM *pEvent;
    /* Publish Adapter Retry Failed signal */
    pEvent = AO_EvtNew(Sig, sizeof(QEVENT_ADAPTERCOM));
    pEvent->AdapterCmd = AdapterCmd;
    AO_Publish(pEvent, NULL);
}


/* ========================================================================== */
/**
 * \brief   Adpater com status is returned
 *
 * \details Checks if any unservised Adapter com requests are present
 *
 * \param   < None >
 *
 * \return  Adapter com status : true if Adapter com in progress, else false
 *
 * ========================================================================== */

static bool AdapterIsComPending(void)
{

  return CmdRequested?true:false;
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
* \brief   Adapter Command sending and processing response
 *
 * \details Send the 'CmdData' and processing the response
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void AdapterComm(void)
{
    ADAPTER_CMD_DATA    *pCmdData;       /* Pointer to the Adapter command data */

    pCmdData = &AdapterCmdData;

    if (pCmdData->CmdToSend)
    {
        /* Send command to Adapter */
        SendAdapterUartCommand(pCmdData->Cmd, pCmdData->DataOut, pCmdData->DataSize, pCmdData->CmdMask);
        pCmdData->CmdToSend = false;
    }
}

/* ========================================================================== */
/**
 * \brief   Handle the Adapter Connection
 *
 * \details Initialize the data up on adapter connection
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function Status
 * \retval  AM_STATUS_OK     - No error in handling the adapter connection
 * \retval  AM_STATUS_ERROR  - Error in handling the adapter connection
 *
 * ========================================================================== */
AM_STATUS AdapterConnected(void)
{
    AM_STATUS Status;

    Status = AM_STATUS_OK;

    if (AM_STATUS_OK != AmFlushUart())
    {
        Status = AM_STATUS_ERROR;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Set the device id
 *
 * \details Set the Adapter unique 1-wire identifier
 *
 * \param   DeviceAddress - 1-Wire device address
 * \param   pData - pointer to data
 *
 * \return  None
 *
 * ========================================================================== */
void AdapterSetDeviceId(DEVICE_UNIQUE_ID DeviceAddress, uint8_t *pData)
{

    // Set device unique address, map update function
    AdapterAddress = DeviceAddress;
    AdapterInterface.Update = &AdapterEepUpdate;

    // Read and update local data repository. User is allowed update all attributes
    memcpy((uint8_t *)&AdapterInterface.Data, &pData[0], ONEWIRE_MEMORY_TOTAL_SIZE);

    return;
}

/* ========================================================================== */
/**
 * \brief   Initialization of Adapter Definition
 *
 * \details Initialize Adapter Definition module and internal structures
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Adapter Definition Status
 * \retval  AM_STATUS_OK     - No error in initilalizing
 * \retval  AM_STATUS_ERROR  - Error in initializing
 *
 * ========================================================================== */
AM_STATUS AdapterDefnInit(void)
{
    AM_STATUS          Status;            /* Adapter Definition Status */
    uint8_t            OsError;           /* OS Error Variable */
    static void*       pAmComQStorage[MAX_ADAPTERQ_REQUESTS];                        /* Request Q storage */

    Status = AM_STATUS_ERROR;

    do
    {
        /* Create an OS Mutex for Adapter Defn */
        pAdapterDefnMutex = OSMutexCreate(OS_PRIO_MUTEX_CEIL_DIS, &OsError);
        OSEventNameSet(pAdapterDefnMutex, "L4-AdapterDefn-Mutex", &OsError);
        if (OsError != OS_ERR_NONE)
        {
            /* Failed to create Mutex */
            /* \todo: Add exception handler */
            Log(ERR, "AdapterDefn: Init Falied, Mutex Create Error - %d", OsError);
            break;
        }
        pAdapterTmOutTmr = SigTimerCreate((RESPONSE_TIMEOUT/OS_TMR_CFG_TICKS_PER_SEC), 0, OS_TMR_OPT_ONE_SHOT,&AdapterComTimeout,"AdapterComTimer",&OsError);
        if (OsError != OS_ERR_NONE)
        {
            /* Failed to create timer */
            /* \todo: Add exception handler */
            Log(ERR, "AdapterDefn: Init Falied, Timer Create Error - %d", OsError);
            break;
        }

        pAdapSGStreamTmOutTmr = SigTimerCreate((STREAM_RESPONSE_TIMEOUT/OS_TMR_CFG_TICKS_PER_SEC), 0, OS_TMR_OPT_ONE_SHOT,&AdapterSGStreamTimeout,"AdapterSGStreamTimer",&OsError);
        if (OsError != OS_ERR_NONE)
        {
            /* Failed to create Timer */
            /* \todo: Add exception handler */
            Log(ERR, "AdapterDefn: Init Falied, Timer Create Error - %d", OsError);
            break;
        }

        pAdapComQ = SigQueueCreate(pAmComQStorage, MAX_ADAPTERQ_REQUESTS);


        /* Initilaize the Adapter data structure */
        Status = AdapterDataFlashInitialize();
        if (AM_STATUS_OK != Status)
        {
            break;
        }
        /* initialize the ErrorSet */
        ErrorSet = CLEAR_ERROR;
	} while (false);

	return Status;
}

/* ========================================================================== */
/**
 * \brief   Read the response from the Adapter
 *
 * \details Read the UART response and push that to Circular buffer for further processing.
 *
 * \param   Event - Communication manager event
 *
 * \return  None
 *
 * ========================================================================== */
void ProcessAdapterUartResponse(COMM_MGR_EVENT Event)
{

    uint16_t          DataCount;                         /* Data count */
    memset(AdapterIncomingData, 0x0, ADAPTER_TX_BUFF_SIZE);
    do
    {
        DataCount = 0;
        if ( COMM_MGR_EVENT_NEWDATA == Event )
        {
           pAdapterComm->Peek(&DataCount);
           /* Is the read Ok? */
           if (0 == DataCount)
           {
               break;
           }
           pAdapterComm->Receive(AdapterIncomingData, &DataCount);
           /* Assemble the incoming data stream */
           ProcessAdapterUartStream(AdapterIncomingData, DataCount);
        }
    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Read_AdapterDataFlash
 *
 * \details Read Adapter FLASH memory
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in reading FLASH memory
 * \retval  AM_STATUS_ERROR  - Error in reading FLASH memory
 *
 * ========================================================================== */
static AM_STATUS Read_AdapterDataFlash(void)
{
    AM_STATUS          Status;           /* Function status */
    static uint8_t DataBuffer[DATA_FLASH_ADDRESS_WIDTH + DATA_FLASH_NUMBERBYTES_WIDTH];
    int32_t DataFlashGaugeAddress;
    uint8_t RespStatus;          /* Response Status */
    int16_t ByteCount;

    Status       = AM_STATUS_ERROR;


        DataFlashGaugeAddress = DATA_FLASH_STRAIN_GAUGE_ADDRESS;
        ByteCount = sizeof(EGIA_FACTORY_DATAFLASH);

        *((uint32_t *)(DataBuffer)) = DataFlashGaugeAddress;
        *(DataBuffer +sizeof(DataFlashGaugeAddress)) = (uint8_t)(ByteCount & 0xff);
        *(DataBuffer +sizeof(DataFlashGaugeAddress)+1) = (uint8_t)((ByteCount & 0xff00) >> 8);


    Status = AdapterProcessCmdResp(SERIALCMD_FLASH_READ, DataBuffer,  sizeof(DataBuffer) , COMMAND_BYTE_MASK_MAINAPP, &RespStatus);

    return Status;
}

/* ========================================================================== */
/**
 * \brief    Parse the Adapter Response for SERIALCMD_FLASH_READ command
 *
 * \details  Function to pars the adapter response from
 *           SERIALCMD_FLASH_READ command to the corresponding adapter variables
 *
 * \param    pRecvData - Pointer to the byte array of the adapter data
 *                       flash variables
 * \param    DataSize - The Number of bytes read from adapter flash
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK - No error in reading FLASH memory
 * \retval  AM_STATUS_ERROR - Error in reading FLASH memory
 *
 ============================================================================== */
static AM_STATUS ReadAdapterFactoryValues(uint8_t *pRecvData, uint8_t DataSize)
{
    uint32_t FlashAddress;     ///< Read Data Flash address
    uint16_t FlashIndex;       ///< Read Data FlashIndex
    uint16_t ReceiveIndex;     ///< Receive Data Index
    uint32_t ItemChecksum;     ///< Item Checksum
    uint32_t DataChecksum;     ///< Data checksum
    AM_STATUS Status;          ///< For any CRC error return error condition

    FlashAddress = 0;
    FlashIndex = 0;
    ReceiveIndex = 0;
    ItemChecksum = 0;
    Status = AM_STATUS_OK;

    memcpy(&FlashAddress, pRecvData, sizeof(FlashAddress));
    ReceiveIndex = sizeof(FlashAddress);

    for (FlashIndex = 0;AdapterFlash[FlashIndex].VarAddress != NULL && ReceiveIndex > 0; FlashIndex++)
    {
        if (AdapterFlash[FlashIndex].VarFlashAddress == FlashAddress)
        {
            AdapterFlash[FlashIndex].status = DATAFLASH_SUCCESS;
            memcpy(AdapterFlash[FlashIndex].VarAddress, &pRecvData[ReceiveIndex], AdapterFlash[FlashIndex].VarSize); //copy item from response
            //check item CRC
            DataChecksum = 0;
            DataChecksum = CRC32(DataChecksum, (uint8_t*)AdapterFlash[FlashIndex].VarAddress + FLASH_ITEM_CHECKSUM_SIZE, AdapterFlash[FlashIndex].VarSize - FLASH_ITEM_CHECKSUM_SIZE);

            memcpy (&ItemChecksum, AdapterFlash[FlashIndex].VarAddress, FLASH_ITEM_CHECKSUM_SIZE);

            if ( ( ItemChecksum != DataChecksum )  && ( FlashAddress != GTIN_ADDRESS_INVALID_CRC ) )
            {
              // EGIA adapters from the factory have an incorrect checksum for GTIN#
              // GTIN# does not contain data critical to operation,
              // checksum error for the GTIN Item will be not be considered an indicator of bad FLASH data
                AdapterFlash[FlashIndex].status = DATAFLASH_READ_FAILURE;
                Status = AM_STATUS_ERROR;         //any items CRC error causes a returned error condition
            }
        }

        ReceiveIndex += AdapterFlash[FlashIndex].VarSize;
        if (ReceiveIndex >= DataSize)
        {
            ReceiveIndex = 0; //no more data in response
        }

        FlashAddress += AdapterFlash[FlashIndex].VarSize;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief    Read Adapter calibration parameters
 *
 * \details  This function shares the adapter flash calibration parameters with the
 *           application structure by copying required parameters to passed structure.
 *
 * \param    pFlashParam - Pointer to the application structure
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK - No error in reading FLASH memory
 * \retval  AM_STATUS_ERROR - NULL pointer passed to the function
 *
 ============================================================================== */
AM_STATUS AdapterFlashCalibParameters( uint8_t *pFlashParam )
{
    AM_STATUS Status;

    Status = AM_STATUS_ERROR;

    do
    {
        if ( NULL == pFlashParam)
        {
            break;
        }

        memcpy(pFlashParam, (uint8_t*)&StrainGauge_Flash + FLASH_ITEM_CHECKSUM_SIZE, sizeof(StrainGauge_Flash) - FLASH_ITEM_CHECKSUM_SIZE);
        pFlashParam += sizeof(StrainGauge_Flash) - FLASH_ITEM_CHECKSUM_SIZE;

        memcpy(pFlashParam, (uint8_t*)&AdapterCalParams_Flash + FLASH_ITEM_CHECKSUM_SIZE, sizeof(AdapterCalParams_Flash) - FLASH_ITEM_CHECKSUM_SIZE);
        pFlashParam += sizeof(AdapterCalParams_Flash) - FLASH_ITEM_CHECKSUM_SIZE;

        memcpy(pFlashParam, (uint8_t*)&AdapterLot_Flash + FLASH_ITEM_CHECKSUM_SIZE, sizeof(AdapterLot_Flash) - FLASH_ITEM_CHECKSUM_SIZE);
        pFlashParam += sizeof(AdapterLot_Flash) - FLASH_ITEM_CHECKSUM_SIZE;

        memcpy(pFlashParam, (uint8_t*)&AdapterBoard_Flash + FLASH_ITEM_CHECKSUM_SIZE , sizeof(AdapterBoard_Flash) - FLASH_ITEM_CHECKSUM_SIZE);
        pFlashParam += sizeof(AdapterBoard_Flash) - FLASH_ITEM_CHECKSUM_SIZE;

        memcpy(pFlashParam, (uint8_t*)&AdapterGTIN_Flash + FLASH_ITEM_CHECKSUM_SIZE, sizeof(AdapterGTIN_Flash) - FLASH_ITEM_CHECKSUM_SIZE);

        Status = AM_STATUS_OK;

    } while( false );

    return Status;
}


/* ========================================================================== */
/**
 * \brief   Register  adapter specific callbacks
 *
 * \details Use this function to register App specific calls
 *
 * \param   pCallbackHandler - Adapter event handler function.
 * \param   AppCallBackIndex - Index Location to register the Call Back. Used to call the registered call back api.
 *
 * \return  AM_STATUS - callback register status
 *          AM_STATUS_OK - handler registered successfully
 *          AM_STATUS_ERROR - failed to register handler
 * ========================================================================== */
AM_STATUS L4_RegisterAdapterAppCallback(APP_CALLBACK_HANDLER pCallbackHandler, ADAPTER_APP_INDEX AppCallBackIndex)
{
    AM_STATUS Status = AM_STATUS_ERROR;

    if ((AppCallBackIndex < ADAPTER_APP_MAX ) && ( NULL != pCallbackHandler))
    {
        AdapterAppHandler[AppCallBackIndex] = pCallbackHandler;
        Status = AM_STATUS_OK;
    }
    return Status;
}
/* ========================================================================== */
/**
 * \brief   Initialize App callback Handle structure
 *
 * \details Use this function to initialise handler structure to NULL. this should be done
 *          on every adapter connection/disconnection
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void InitAppHandler(void)
{
    ADAPTER_APP_INDEX HandlerIndex;

    for (HandlerIndex = ADAPTER_APP_STRAIN_GAUGE_INDEX; HandlerIndex < ADAPTER_APP_MAX; HandlerIndex++)
    {
       AdapterAppHandler[HandlerIndex] = NULL;
    }
}

/* ========================================================================== */
/**
 * \brief Post the Adapter Com Request to Q
 *
 * \details Post the Adapter com Request to Q
 *
 * \param   ADAP_COM_MSG - Com command to be posted to Q
 *
 * \return  AM_STATUS       - Function status
 * \retval  AM_STATUS_OK    - No error in initializing
 * \retval  AM_STATUS_ERROR - Error in initializing
 *
 * ========================================================================== */
AM_STATUS L4_AdapterComPostReq(ADAPTER_COM_MSG Msg)
{
    uint8_t       Error;        /* Error status */
    ADAPTER_COM_MSG  *pComReq;
    AM_STATUS     Status;

    Status = AM_STATUS_ERROR;
    pComReq = GetNextAdapComReqMsgSlot();
    if(NULL != pComReq)
    {
        pComReq->Cmd = Msg.Cmd;
        pComReq->DelayInMsec = Msg.DelayInMsec;
        Error = OSQPost(pAdapComQ, pComReq);
        if ( OS_ERR_Q_FULL == Error )
        {
            Log(ERR, "AdatperComManger: UART Com Message Queue is Full");
        }
        else
        {
            CmdRequested |= (1<<Msg.Cmd);
            Status = AM_STATUS_OK;
        }
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief Adapter power Status
 *
 * \details functions reads the Adapter 5V signal GPIO_EN_5V
 *
 * \param   < None >
 *
 * \return  GPIO_EN_5V pin status
 *
 * ========================================================================== */
bool IsAdapterPowered(void)
{
    bool Status;
    Status = false;
    if (GPIO_STATUS_OK != L3_GpioCtrlGetSignal(GPIO_EN_5V, &Status))
    {
      Log(ERR, "AdapterMgr: GPIO_EN_5V status Not known");
    }
    return Status;

}

/* ========================================================================== */
/**
 * \brief Adapter com Statemachine
 *
 * \details Handles the Adapter uart communication by reading the Requested command from Q
 *          The function is implemented as non blocking
 *          ADAPTER_COM_STATE_CHECKQ: Checks the Adapter Com Q for any requests.Triggers the
 *               appropriate serial commands to serve the request
 *
 *          ADAPTER_COM_STATE_INPROGRESS: Waits in this state until
 *               the response for the command is received or
 *               Timeout occurs
 *
 *          ADAPTER_COM_STATE_WAIT: Wait time for inter command processing.
 *                This is needed in cases where adapter needs time before it can serice next command
 *
 * \param   < None >
 *
 * \return  AM_STATUS       - Function status
 * \retval  AM_STATUS_OK    - No error in initializing
 * \retval  AM_STATUS_ERROR - Error in initializing
 *
 * ========================================================================== */
AM_STATUS RunAdapterComSM(void)
{
    AM_STATUS          AmStatus;     /* Function status */
    ADAPTER_CMD_DATA *pCmdData;
    static ADAPTER_COM_MSG *AdapCmd;
    uint8_t Error;
    static uint32_t TimeInMsec;

    pCmdData = &AdapterCmdData;

    AmStatus = AM_STATUS_OK;
    do
    {

        switch(AdapterComState)
        {
            /* check the Que for command request. if no requests break, else process the command*/
            case ADAPTER_COM_STATE_CHECKQ:
                AdapCmd = (ADAPTER_COM_MSG *)OSQAccept(pAdapComQ,&Error);
                BREAK_IF(NULL == AdapCmd);
                AdapterComState = ADAPTER_COM_STATE_INPROGRESS;
                AdapterCmdState = ADAPTER_CMD_STATE_SEND;
                AmFlushUart();
                AmStatus = AdapterComManager(AdapCmd);
                if(AM_STATUS_OK == AmStatus){
                    CmdRequested &= ~(1<<AdapCmd->Cmd);
                    AdapterComSigPublish(P_ADAPTER_COM_RESP_RECEIVED_SIG,AdapCmd->Cmd);
                    if(AdapCmd->DelayInMsec)
                    {
                        AdapterComState = ADAPTER_COM_STATE_WAIT;
                        TimeInMsec = SigTime();
                        break;
                    }
                    AdapterComState = ADAPTER_COM_STATE_CHECKQ;
                    break;
                }
                AdapterTimeoutTimerStart();

                break;
            /* Command is in progress, either sending or waiting for response or timeout*/
            case ADAPTER_COM_STATE_INPROGRESS:
                AmStatus = AdapterComManager(AdapCmd);
                BREAK_IF(AM_STATUS_WAIT == AmStatus);
                AdapterComState = ADAPTER_COM_STATE_CHECKQ;
                CmdRequested &= ~(1<<AdapCmd->Cmd);
                if(AM_STATUS_TIMEOUT == AmStatus)
                {
                   AdapterComSigPublish(P_ADAPTER_COM_RETRY_FAIL_SIG,AdapCmd->Cmd);
                   L4_AdapterComSMReset();
                   break;
                }
                AdapterComSigPublish(P_ADAPTER_COM_RESP_RECEIVED_SIG,AdapCmd->Cmd);
                if(AdapCmd->DelayInMsec)
                {
                    AdapterComState = ADAPTER_COM_STATE_WAIT;
                    TimeInMsec = SigTime();
                    break;
                }
                break;
            /* Inter command wait state*/
            case ADAPTER_COM_STATE_WAIT:
                BREAK_IF(SigTime() - TimeInMsec < AdapCmd->DelayInMsec);
                AdapterComState = ADAPTER_COM_STATE_CHECKQ;
                break;

            default:
                pCmdData->CmdToSend = false;
                pCmdData->Cmd = 0;
                AdapterComState = ADAPTER_COM_STATE_CHECKQ;
                pCmdData->CmdRetry = 0;
                break;
        }
    } while (false);
    return AmStatus;
}


/* ========================================================================== */
/**
 * \brief Reset Adapter com status variables
 *
 * \details Reset Adapter com status variables, Flush the CommandQ, Flush uart buffer
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L4_AdapterComSMReset(void)
{
    OSQFlush(pAdapComQ);
    memset(ComMsgReqPool,0,sizeof(ADAPTER_COM_MSG)*MAX_ADAPTERQ_REQUESTS);
    CmdRequested = 0;
    AdapterComState = ADAPTER_COM_STATE_CHECKQ;
    AdapterCmdState = ADAPTER_CMD_STATE_SEND;
    AdapterCmdData.Cmd = SERIALCMD_UNKNOWN;
    AdapterCmdData.CmdToSend = false;
    AdapterCmdData.RespReceived = false;
    AdapterCmdData.RespTimeOut = false;
    /* Initilaize the Adapter data structure */
    AdapterDefnRepo.AdapterState = ADAPTER_IN_BOOT;
    AmFlushUart();
    AdapterSGTimeoutTimerStop();
    AdapterTimeoutTimerStop();

}

/* ========================================================================== */
/**
 * \brief reads the Adapter Type
 *
 * \details Return the adapter type as per the adapter flash
 *
 * \param   < None >
 *
 * \return  AM_STATUS       - Function status
 * \retval  AM_STATUS_OK    - No error in initializing
 * \retval  AM_STATUS_ERROR - Error in initializing
 *
 * ========================================================================== */
AM_STATUS AdapterGetType(uint16_t *pAdapterType)
{
    AM_STATUS Status;
    Status = AM_STATUS_ERROR;
    if(AdapterDefnRepo.AdapterTypeStatus){
        *pAdapterType = AdapterDefnRepo.AdapterType;
        Status = AM_STATUS_OK;
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Initilalize the Adapter Data
 *
 * \details Initialize the ADAPTER_CMD_DATA data structure
 *
 * \param   < None >
 *
 * \return  AM_STATUS       - Function status
 * \retval  AM_STATUS_OK    - No error in initializing
 * \retval  AM_STATUS_ERROR - Error in initializing
 *
 * ========================================================================== */
AM_STATUS AdapterDataFlashInitialize(void)
{
    AM_STATUS          Status;            /* Adapter Definition Status */
    ADAPTER_CMD_DATA   *pCmdData;         /* Pointer to the command to send buffer */

    Status = AM_STATUS_OK;
    pCmdData = &AdapterCmdData;

    pCmdData->Cmd = SERIALCMD_UNKNOWN;
    pCmdData->CmdMask = INVALID_BYTE_MASK;
    memset(pCmdData->DataOut, 0x00, sizeof(pCmdData->DataOut));
    memset(pCmdData->RespData, 0x00, sizeof(pCmdData->RespData));
    memset(&AdapterDefnRepo.StrainGaugeData, 0x00, sizeof(AdapterDefnRepo.StrainGaugeData));
    AdapterDefnRepo.SwitchData.State     = SWITCH_STATE_UNKNOWN;
    AdapterDefnRepo.SwitchData.TimeStamp = 0;

    pCmdData->DataSize              = 0;
    pCmdData->CmdToSend             = false;
    pCmdData->RespReceived          = false;
    pCmdData->ResponseStatus        = INVALID_RESP_CODE;
    AdapterDefnRepo.HwVersionStatus = false;
    AdapterDefnRepo.ForceTareOffset = 0;
    AdapterDefnRepo.pStrainGaugeNewData = &AdapterDefnRepo.StrainGaugeData[0];
    AdapterDefnRepo.pStrainGaugeOldData = &AdapterDefnRepo.StrainGaugeData[1];
    AdapterDefnRepo.AdapterFlashParmStatus = false;
    AdapterDefnRepo.AdapterTypeStatus = false;
    AdapterDefnRepo.AdapterType = 0;
    AdapterDefnRepo.AdapterState = ADAPTER_IN_BOOT;
    pCmdData->pSema = NULL;

    AdapterSGTimeoutTimerStop();
    AdapterTimeoutTimerStop();

    return Status;
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

