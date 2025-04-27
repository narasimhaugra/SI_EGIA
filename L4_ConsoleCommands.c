#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L4_ConsoleManager
 * \{
 *
 * \brief   Console Manager Functions
 *
 * \details Signia Handle communicates with external software applications
 *          through various commands.The console manager facilitates in receiving
 *          these commands and transmitting responses back to the external
 *          applications through the communication manager interface.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_ConsoleManager.h"
#include "L4_ConsoleCommands.h"
#include "Signia_KeypadEvents.h"
#include "L3_OneWireRtc.h"
#include "L2_OnchipRtc.h"
#include "FileSys.h"
#include "Signia_Accelerometer.h"
#include "L4_BlobHandler.h"
#include "Signia_PowerControl.h"
#include "Signia_AdapterManager.h"
#include "Aes.h"
#include "clk.h"
#include "ActiveObject.h"
#include "TestManager.h"
#include "Kvf.h"
#include "DSA.h"
#include "NoInitRam.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER         (LOG_GROUP_CONSOLE)     /*! Log Group Identifier */
#define VAR_NAME_SIZE                (35u)                /*! Changeable variable name size */
#define CHANGEABLE_VARS_COUNT        (sizeof(ChangeableVars) / sizeof(ChangeableVars[0])) /*! Var count */
#define FILE_SIZE_RD_OVERHEAD        (20u)                /*! File size read overhead */
#define DEFAULT_STREAM_RATE          (10u)                /*! Default Stream Rate */
/*! Streaming Variable Count */
#define STREAMING_VARS_COUNT         (sizeof(StreamingVarsList)  / sizeof(StreamingVarsList[0]))
#define BLOB_FILE_NAME               ("\\BlobFile")                                    ///< Fixed Blob File name in SD Card
/// \todo 06/01/2021 CPK The below structure to be referring to actual Signal structure in Signal.h - retained for debug purposes
#define TEMP_LAST_SIG               (0x5u)                                             ///< Temporary definition for Signal count
#define STATUS_VARS_COUNT           (sizeof(StatusVars)  / sizeof(StatusVars[0]))      ///< Total number of status Vars
#define VALID_SIGNAL                (1u)                                               ///< Indicates a valid signal
#define BITS_8                      (8u)                                               ///< defines 8-bits
#define STATUS_VARS_MAX_SIZE        (256u)                                             ///< Max bytes for status response
#define DEFAULT_OFFSET              (0xFFFFFFFFu)                                      ///< Default Offset Value
#define DATA_OFFSET                 (4u)                                               ///< Data Offset
#define RESET_DELAY                 (5000u)                                            ///< Reset Delay
#define DATA_OFFSET_START           (0u)                                               ///< Data offset is 0x0 when download starts

#define BAT_CMD_OFFSET                     (0u)            ///< Offset for Command
#define BAT_CMD_DATA_OFFSET                (3u)            ///< Offset for Data
#define BAT_CMD_SHUTDOWN_LOWBYTE           (0x10u)         ///< Battery command data for Shutdown mode(0x0010) Lowbyte
#define BAT_CMD_SHUTDOWN_HIGHBYTE          (0u)            ///< Battery command data for Shutdown mode(0x0010) Highbyte
#define BAT_CMD_MANUFACTURING_ACCESS_BYTE  (0u)            ///< Battery command MANUFACTURER ACCESS

#define OW_NUM_OF_DEVICES                  (0x6u)          ///< One wire devices
#define OW_DEVICE_CONNECTED                (0x1u)          ///< One wire connected bit
#define OW_DEVICE_AUTHENTICATED            (0x2u)          ///< One wire authenticated bit
#define DATE_STR_LEN                       (23u)           ///< Length of Date string
#define HARDWARE_VERSION_1                 (0x1u)          ///< Version -1
#define DUMMY_LAST_SIG                     (0xA5u)         ///< dummy value for signal data
#define TESTDATA_OFFSET                    (3u)            ///< Offset for Test data in the Test command frame
#define TESTID_OFFSET                      (2u)            ///< Offset for Test Id in Test command frame
#define MAX_CHAR                           (0xFF)          ///< Max number of chars for KVF description

#define SOFTWARE_VERSION                   (0x0001)        ///< temporary? (from legacy)
#define RXBUFF_FILE_INDEX                  (6u)            ///< File name index for security log
#define RXBUFF_FILE_NAME_INDEX             (10u)           ///< File name index in rx buff
#define RXBUFF_DATA_START_INDEX            (8u)            ///< data start index in rx buff

#define FIELD_SIZEOF(t, f) (sizeof(((t*)0)->f))            ///< Size of a stucture element

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef struct                                                    /*! File data repository */
{
    FS_DIR *       OpenDir;                                       /*! Pointer to a directory */
    FS_DIR_ENTRY   NextDirEntry;                                  /*! Directory entry info */
    uint8_t        WorkingFileName[FS_CFG_MAX_FULL_NAME_LEN + 1]; /*! File name to store */
} CONS_MGR_FILE_REPO;

typedef struct                         /*! Changeable variable data structure */
{
   uint8_t    VarSize;                 /*! Variable size */
   void       *pVarAddress;            /*! Variable Address */
   VAR_TYPE   VarType;                 /*! Variable type */
   uint8_t    VarName[VAR_NAME_SIZE];  /*! Variable name */
} CHANGEABLE_VAR;

typedef struct                         /*! Changeable variable data repository */
{
    uint8_t   ChangeableVarsCount;     /*! Changeable variable count */
    bool      NewSulu;                 /*! New SULU status */
    bool      UsedSulu;                /*! Used SULU status */
} CHANGEABLE_VAR_REPO;

typedef enum                         ///< Status variable type repository
{
    STATUS_TYPE_UNKNOWN,             ///< Undefined Status Type
    STATUS_TYPE_BATTERY,             ///< Status Type Battery
    STATUS_TYPE_CLAMSHELL,           ///< Status Type Clamshell
    STATUS_TYPE_HANDLE,              ///< Status Type Handle
    STATUS_TYPE_WIFI,                ///< Status Type WIFI
    STATUS_TYPE_ADAPTER,             ///< Status Type Adapter
    STATUS_TYPE_RELOAD,              ///< Status Type Reload
    STATUS_TYPE_MULU,                ///< Status Type MULU
    STATUS_TYPE_CARTRIDGE,           ///< Status Type Cartridge
    STATUS_TYPE_CHARGER,             ///< Status Type Charger
    STATUS_TYPE_COUNT                ///< Status Type Count
} STATUS_TYPE;                       ///< Status Type Enumarator

typedef struct                       ///< Error Status variable type repository
{
    uint8_t varSize;                                        /*! Streaming Var Size */
    void *varAddress;                                       /*! Streaming Var Address */
    bool isStreaming;                                       /*! Streaming Var has to stream or not */
    VAR_TYPE varType;                                       /*! Streaming Var type */
    uint8_t varName[VAR_NAME_SIZE];                         /*! Streaming Var Name */
} STREAMINGVAR;

typedef struct                       ///< Error Status variable type repository
{
   STATUS_TYPE StatusType;           ///< Type of the Status parameter
   uint32_t    WarningFlags;         ///< cumilative warning flags
   uint32_t    ErrorFlags;           ///< cumilative error flags
} CONS_ERROR_STATUS;

/// \todo 06/01/2021 CPK Below structure to be referring to actual Signal table structure in Signal.h - added for debug purposes
typedef struct
{
    SIGNAL sig;                                                             ///< Name of signal
    bool   log;                                                             ///< True if signal is to be logged
    uint8_t  *pString;                                                         ///< Signal description string
    void (*pFunc)( char *buf, INT8U bufSize, void const * const pEvent );   ///<
} TEMP_SIG_TABLE;

typedef struct
{
    // Version 1 data
    int32_t Flags;
    int32_t BlobTimestamp;
    int32_t HandleTimestamp;
    int32_t HandleBLTimestamp;
    int32_t JedTimestamp;
    int32_t BlobHandleTimestamp;
    int32_t BlobHandleBLTimestamp;
    int32_t BlobJedTimestamp;
    int32_t BlobAdaptBLTimestamp;
    int32_t BlobEgiaTimestamp;
    int32_t BlobEeaTimestamp;
    // Version 2 data
    int32_t BlobVersion;
    char    BlobAgileNumber[20];
    char    BlobPowerPack_Rev[20];
    char    BlobPowerPack_BL_Rev[20];
    char    BlobJed_Rev[20];
    char    BlobAdapter_BL_Rev[20];
    char    BlobAdapterEGIA_Rev[20];
    char    BlobAdapterEEA_Rev[20];
    // Version 3 data
    char   BlobSystemVersion[20];
} DEVICE_PROPERTIES;

typedef union
{
    uint32_t Word;
    uint8_t  Byte[4];
} UWORD;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static AXISDATA AccelAxisData;                              /*! Accelerometer instance to get Accel Data */
static uint32_t streamingDataRate = DEFAULT_STREAM_RATE;    /*! Streaming Data Rate */
static uint32_t systemMilliSeconds;                         /*! Milliseconds to stream (From Legacy - Can be moved/update later) */
static uint8_t StreamingVarCount;                           /*! Streaming Variable count to stream */
static bool IsStreamingVarsEnabled;                         /*! Streaming is Enabled or not */
static uint16_t PacketDataSize;                             /*! Data size */
static bool USBPortProtectedMode;                           /*! Protected Mode for USB Port */
static bool PasswordReceived;                               /*! Password received flag True/False */

//#pragma location=".sram"
static char const* const SerialCmdName[] =
{
    "SERIALCMD_UNKNOWN",
    #define X(a) #a,
    #include "SerialCommands.def"
    #undef X
    "SERIALCMD_COUNT",
    0
};

static CONS_MGR_FILE_REPO ConsFileRepo = {NULL, NULL, false, {0}};
static CHANGEABLE_VAR_REPO ChangeableVarRepo = {0, false, false};

static CHANGEABLE_VAR ChangeableVars[] =
{
   { sizeof(ChangeableVarRepo.NewSulu), &ChangeableVarRepo.NewSulu, VAR_TYPE_BOOL, "NewSulu" },
   { sizeof(ChangeableVarRepo.UsedSulu), &ChangeableVarRepo.UsedSulu, VAR_TYPE_BOOL, "UsedSulu" },
};

/*! A list of the streaming variables specifing its size, address variable type and name as null terminated string.*/
/*! To be added new Streaming variable list */

static STREAMINGVAR StreamingVarsList[] =
{
    { sizeof(systemMilliSeconds),        &systemMilliSeconds,    false, VAR_TYPE_INT32U, "systemMilliSeconds" },
    { sizeof(AccelAxisData.x_axis),      &AccelAxisData.x_axis,  false, VAR_TYPE_INT16S, "Accel X Data" },
    { sizeof(AccelAxisData.y_axis),      &AccelAxisData.y_axis,  false, VAR_TYPE_INT16S, "Accel Y Data" },
    { sizeof(AccelAxisData.z_axis),      &AccelAxisData.z_axis,  false, VAR_TYPE_INT16S, "Accel Z Data" },
};

/* status command related definitions */
static bool     StatusVarsEnabled           = false;    ///< True when status variables are enabled
static bool     StatusVarHandleMoving       = true;     ///< True when Handle movement is detected in realtime
static bool     StatusVarBatteryConnected   = false;    ///< True when Battery is detected on handle
static bool     StatusVarAdapterConnected   = true;     ///< True when Adapter is detected on handle
static bool     StatusVarAdapterCalibrated  = false;    ///< True when Adapter is calibrated
static bool     StatusVarClamShellConnected = true;     ///< True when Clamshell is detected
static bool     StatusVarReloadConnected    = false;    ///< True when Reload is detected
static bool     StatusVarCartridgeConnected = true;     ///< True when Cartridge is detected
static bool     StatusVarReloadClamped      = true;     ///< True when Reload clamping is successful
static bool     StatusVarReloadFullyOpen    = false;    ///< True when Clamshell is detected on one wire bus
static uint32_t StatusDataRate              = 0x0;      ///< Rate(in milliseconds) at which Status data is sent to external tools(MCP)
static uint32_t NextStatusMilliSeconds;                 ///< Variable used to identufy next status transfer time based on StatusDataRate

/*
  *   A list of the status variables specifing Type, error flags and warning flags to be shared with external tools(MCP) .
*/
static CONS_ERROR_STATUS ConsoleManagerStatuses[]  =  ///< cummilative Error Status variable
{
    STATUS_TYPE_UNKNOWN,  0, 0,       ///< Error Status UNKNOWN
    STATUS_TYPE_BATTERY,  1, 1,       ///< Error Status BATTERY
    STATUS_TYPE_CLAMSHELL,0, 1,       ///< Error Status CLAMSHELL
    STATUS_TYPE_HANDLE,   1, 1,       ///< Error Status HANDLE
    STATUS_TYPE_WIFI,     0, 1,       ///< Error Status WIFI
    STATUS_TYPE_ADAPTER,  0, 0,       ///< Error Status ADAPTER
    STATUS_TYPE_RELOAD,   0, 0,       ///< Error Status RELOAD
    STATUS_TYPE_MULU,     0, 0,       ///< Error Status MULU
    STATUS_TYPE_CARTRIDGE,0, 0,       ///< Error Status CARTRIDGE
    STATUS_TYPE_CHARGER,   0, 1       ///< Error Status CHARGER
};

/*
  *   A list of the status variables specifing its size, address variable type
  *    and name as null terminated string.
*/
static const CHANGEABLE_VAR StatusVars[] =
   {
   { sizeof(StatusVarHandleMoving),       &StatusVarHandleMoving,       VAR_TYPE_BOOL, "Handle Moving" },
   { sizeof(StatusVarBatteryConnected),   &StatusVarBatteryConnected,   VAR_TYPE_BOOL, "Battery Connected" },
   { sizeof(StatusVarAdapterConnected),   &StatusVarAdapterConnected,   VAR_TYPE_BOOL, "Adapter Connected" },
   { sizeof(StatusVarAdapterCalibrated),  &StatusVarAdapterCalibrated,  VAR_TYPE_BOOL, "Adapter Calibrated" },
   { sizeof(StatusVarClamShellConnected), &StatusVarClamShellConnected, VAR_TYPE_BOOL, "Clamshell Connected" },
   { sizeof(StatusVarReloadConnected),    &StatusVarReloadConnected,    VAR_TYPE_BOOL, "Reload Connected" },
   { sizeof(StatusVarCartridgeConnected), &StatusVarCartridgeConnected, VAR_TYPE_BOOL, "Cartridge Connected" },
   { sizeof(StatusVarReloadClamped),      &StatusVarReloadClamped,      VAR_TYPE_BOOL, "Reload Clamped" },
   { sizeof(StatusVarReloadFullyOpen),    &StatusVarReloadFullyOpen,    VAR_TYPE_BOOL, "Reload Fully Open" },

   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_HANDLE].WarningFlags),    &ConsoleManagerStatuses[STATUS_TYPE_HANDLE].WarningFlags,    VAR_TYPE_INT32U, "Handle Warnings" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_HANDLE].ErrorFlags),      &ConsoleManagerStatuses[STATUS_TYPE_HANDLE].ErrorFlags,      VAR_TYPE_INT32U, "Handle Errors" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_ADAPTER].WarningFlags),   &ConsoleManagerStatuses[STATUS_TYPE_ADAPTER].WarningFlags,   VAR_TYPE_INT32U, "Adapter Warnings" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_ADAPTER].ErrorFlags),     &ConsoleManagerStatuses[STATUS_TYPE_ADAPTER].ErrorFlags,     VAR_TYPE_INT32U, "Adapter Errors" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_RELOAD].WarningFlags),    &ConsoleManagerStatuses[STATUS_TYPE_RELOAD].WarningFlags,    VAR_TYPE_INT32U, "Reload Warnings" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_RELOAD].ErrorFlags),      &ConsoleManagerStatuses[STATUS_TYPE_RELOAD].ErrorFlags,      VAR_TYPE_INT32U, "Reload Errors" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_MULU].WarningFlags),      &ConsoleManagerStatuses[STATUS_TYPE_MULU].WarningFlags,      VAR_TYPE_INT32U, "MULU Warnings" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_MULU].ErrorFlags),        &ConsoleManagerStatuses[STATUS_TYPE_MULU].ErrorFlags,        VAR_TYPE_INT32U, "MULU Errors" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_BATTERY].WarningFlags),   &ConsoleManagerStatuses[STATUS_TYPE_BATTERY].WarningFlags,   VAR_TYPE_INT32U, "Battery Warnings" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_BATTERY].ErrorFlags),     &ConsoleManagerStatuses[STATUS_TYPE_BATTERY].ErrorFlags,     VAR_TYPE_INT32U, "Battery Errors" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_CLAMSHELL].WarningFlags), &ConsoleManagerStatuses[STATUS_TYPE_CLAMSHELL].WarningFlags, VAR_TYPE_INT32U, "Clamshell Warnings" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_CLAMSHELL].ErrorFlags),   &ConsoleManagerStatuses[STATUS_TYPE_CLAMSHELL].ErrorFlags,   VAR_TYPE_INT32U, "Clamshell Errors" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_WIFI].WarningFlags),      &ConsoleManagerStatuses[STATUS_TYPE_WIFI].WarningFlags,      VAR_TYPE_INT32U, "WiFi Warnings" },
   { sizeof(ConsoleManagerStatuses[STATUS_TYPE_WIFI].ErrorFlags),        &ConsoleManagerStatuses[STATUS_TYPE_WIFI].ErrorFlags,        VAR_TYPE_INT32U, "WiFi Errors" },
   };

static uint32_t StatusVarsCount             = STATUS_VARS_COUNT; ///< Number of Status variables to be sent to external tools(MCP)

 /// \todo 06/01/2021 CPK The below structure to be referring to actual Signal table in Signal.h - added for debug purposes
#if 0
static const TEMP_SIG_TABLE TempSigTable[] =
{
    /*  sig                             log    *pString                        *pFunc */
    { R_EMPTY_SIG,                       false, "R_SIG_EMPTY",                   0 },
    { R_ENTRY_SIG,                       false, "R_SIG_ENTRY",                   0 },
    { R_EXIT_SIG,                        false, "R_SIG_EXIT",                    0 },
    { R_INIT_SIG,                        false, "R_SIG_INIT",                    0 },
    { R_USER_SIG,                        false, "R_SIG_USER",                    0 },
 };
#endif
/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static uint16_t To16U(uint8_t *pRawData);
/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ========================================================================== */

/******************************************************************************/
/*                                 Global Functions                           */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Format the Device address to match with the address read from device memory
 *
 * \details This function is used to reverse the byte order of the Device address to keep the format inline with Legacy address.
 *          CAUTION: The resultant OutputId is only intended  to be sent to MCP  and not to be
 *                   used for further processing within handle.
 *          This can be replaced if a better method to get the device address is identified.
 *
 * \param  InputId  - pointer to Input Address fed to the function and expects a formatted OutputId
 * \param  OutputId - pointer to formatted Output sent to calling function
 *
 * \return  None
 *
 * ========================================================================== */
void FormatDeviceAddr(uint8_t *InputId, uint8_t *OutputId)
{
    uint8_t count1;
    uint8_t count2;

    count2 = DEV_ADDR_LENGTH - 1;
    for(count1=0; count1 <DEV_ADDR_LENGTH; count1++)
    {
        OutputId[count2] = InputId[count1];
        count2--;
    }
}

/**
 * \brief   process the incoming commands
 *
 * \details Process all the incoming commands and take action
 *
 * \param   pDataRx - pointer to the received data from comms manager
 * \note    todo: command process error handling is pending.
 *
 * \return  CONS_MGR_STATE: next state in the console task
 * \retval  CONS_MGR_STATE_WAIT_FOR_EVENT: wait for new data event
 * \retval  CONS_MGR_STATE_SEND_RESPONSE: Send response packet created
 *
 * ========================================================================== */
CONS_MGR_STATE ProcessCommand(PROCESSDATA *pDataRx)
{
    /* \todo 05/24/2021 PR To reduce the variable clutter and stack size, move to respective switch cases */
    // /// \todo 12/16/2021 CPK While cleanup move large buffers in Console manager and Comm manager to SRAM
    uint8_t             *pRxData;                                    /* Receive data */
    uint8_t             *pDataIn;                                    /* Data frame address */
    uint8_t             Command;                                     /* Command */
    bool                StartuptDelay1;                              /* used to delay the response */
    bool                StartuptDelay2;                              /* used to delay the response */
    static uint8_t      ResponseData[LARGEST_PACKET_SIZE_16BIT];     /* Response buffer */
    static uint32_t     PrevDataOffset;
    static bool         DownloadStarted;                             /* Download start status */
    static uint32_t     DataOffset;                                  /* To store temp packet offset */
    CONS_MGR_STATE      ConsoleTaskNextState;
    FS_ERR              FsError;                                     /* File system error */
    CONS_MGR_FILE_REPO  *pConsFileRepo;                              /* Pointer to file repository */    
    uint16_t            PacketStartIndex;                            /* Start index within the packet */
    FS_FILE             *pTempFilePtr;                               /* Temporary file pointer */    
    uint16_t            TempValue;                                   /* To store temp data variable */    
     
    /* Adapter manager device array as declared in communication protocol */
    /// \\todo 03-22-2022 CPK : Mapped to legacy OW_TYPE structure. we need to get SULU/MULU type from AM insted of AM_DEVICE_RELOAD
    static AM_DEVICE    Device[OW_NUM_OF_DEVICES]  = { AM_DEVICE_HANDLE, AM_DEVICE_BATTERY, AM_DEVICE_CLAMSHELL, AM_DEVICE_ADAPTER,AM_DEVICE_RELOAD,AM_DEVICE_CARTRIDGE };

    ConsoleTaskNextState = CONS_MGR_STATE_SEND_RESPONSE;

    static uint8_t 					Passphrase[AES_BLOCKLEN];
    static AUTHENTICATION_STATUS 	AuthStatus = AUTHENTICATION_STATUS_UNKNOWN;
    const static uint8_t 			*pPassphrase = "SigniaHandle1234";
    
/// \\todo 02-16-2022 KA : not currently used
//    uint8_t             PasswordByte[CMD_PASSWORD_LEN];          /* To store password bytes */

    QEVENT_TEST_MSG *pTestEvt;

    do
    {
        if ( pDataRx == NULL)
        {
            break;
        }
        /* clear response buffer*/
        memset (ResponseData, 0x0, sizeof(ResponseData));
        pConsFileRepo = &ConsFileRepo;

        /* reset transfer data count */
        pDataRx->TxDataCount = 0;

        /* get pointer for transfer data */
        pDataRx->pDataOut = ResponseData;

        /* get received data pointer. ValidCommands array maintains a list of command pointers ..CommandCounter refers to the command being processed */
        pDataIn = pDataRx->pValidCommands[pDataRx->CommandCounter];
        pRxData = pDataIn + pDataRx->PacketStartIndex[pDataRx->CommandCounter] + DATA_OFFSET_16BIT;

        /* extract DataSize and command from received data */
        PacketStartIndex = pDataRx->PacketStartIndex[pDataRx->CommandCounter];

        pDataRx->DataSize = To16U( &pDataIn[ PacketStartIndex + PCKT_SIZE_OFFSET ]);
        pDataRx->DataSize -= PCKT_OVERHEAD_16BIT;
        Command = pDataIn[ PacketStartIndex + COMMAND_OFFSET_16BIT ];

        ChangeableVarRepo.ChangeableVarsCount = CHANGEABLE_VARS_COUNT;
        StreamingVarCount = STREAMING_VARS_COUNT;

        // Complete authentication before responding to all commands
        if ((AUTHENTICATION_STATUS_SUCCESS  != AuthStatus) && \
            ((SERIALCMD_PING                != Command)    && \
             (SERIALCMD_AUTHENTICATE_DEVICE != Command)))
        {
            break;
        }

        do
        {
            if ( (PasswordReceived == false) && (SERIALCMD_PING != Command) && (SERIALCMD_ENUM_INFO != Command) &&
                /*(SERIALCMD_PASSWORD != Command) &&*/ (SERIALCMD_GET_VERSION != Command) && (SERIALCMD_SET_RTC != Command) &&
                 (SERIALCMD_SIGNAL_TYPE_INFO != Command) && (SERIALCMD_SIGNAL_TYPE_COUNT != Command)  &&
                 (SERIALCMD_STATUS_VAR_INFO != Command) && (SERIALCMD_STATUS_VAR_COUNT != Command)  )
            {
                /* Password Command is not received then don't process the command other than above commands */
                /// \todo 02/17/2022 SE - Commenting out 'break' statement for proper MCP Communication. Enable break statement once Password protection implemented completely
                //break;
            }

            switch (Command)
            {
                case SERIALCMD_PING:
                    ResponseData[pDataRx->TxDataCount++] = DEVICETYPE_GEN2;
                    break;

                case SERIALCMD_DEBUG_STR:
                    /*#if COMPILE_AS_CV1 | TEST_FIXTURE*/ /* Todo: enable commented #defs with correct macros*/
                    Log(DBG,"%s", pRxData);
                    /*#endif */
                    break;

                case SERIALCMD_ENUM_INFO:
                    StartuptDelay1 = false;
                    StartuptDelay2 = false;
                    ResponseData[pDataRx->TxDataCount++] = pRxData[0];

                    if (pRxData[0] < SERIALCMD_COUNT)
                    {
                        sprintf ((char *)(ResponseData + 1), SerialCmdName[pRxData[0]]);
                        pDataRx->TxDataCount = strlen((char *)(ResponseData)) + 1;
                    }
                    else
                    {
                        ResponseData[pDataRx->TxDataCount++] = 0;
                    }
                    break;
                    
                case SERIALCMD_AUTHENTICATE_DEVICE:
                    // first authentication command has 0 bytes data    
                    if ( ( AUTHENTICATION_STATUS_UNKNOWN == AuthStatus ) || !pDataRx->DataSize )
                    {
                        AuthStatus = AUTHENTICATION_STATUS_STARTED;
                        
                        memcpy( Passphrase,pPassphrase,AES_BLOCKLEN );
                        ProcessPassphrase( Passphrase, AES_KEY_ONE, AES_OPERATION_ENCRYPT );
                        
                        memcpy( &ResponseData[pDataRx->TxDataCount], Passphrase, AES_BLOCKLEN );
                        pDataRx->TxDataCount += AES_BLOCKLEN;
                    
                    }
                    else if( AUTHENTICATION_STATUS_STARTED == AuthStatus )
                    {   
                        memcpy( Passphrase, &pDataRx->pDataIn[DATA_OFFSET_16BIT], AES_BLOCKLEN);
                        pDataRx->TxDataCount += AES_BLOCKLEN;
                      
                        ProcessPassphrase( Passphrase, AES_KEY_TWO, AES_OPERATION_DECRYPT);
                        
                        if ( !memcmp(Passphrase,pPassphrase,AES_BLOCKLEN))
                        {
                            Log(DBG,"MCP Authentication Success!!", pRxData);
                            AuthStatus = AUTHENTICATION_STATUS_SUCCESS;                            
                        }
                        else
                        {
                            Log(DBG,"MCP Authentication Failed!!", pRxData);
                            AuthStatus = AUTHENTICATION_STATUS_FAILED;                            
                        }                    
                    }
                    break;

                case SERIALCMD_GET_RTC:
                  {
                    RTC_SECONDS     RtcTime;         /* RTC time */
                    CLK_DATE_TIME    DateTime;     /* RTC Date and Time */
                    int8_t DateStr[DATE_STR_LEN];                       /* Date and Time String*/
                    RtcTime = L2_OnchipRtcRead();
                    ResponseData[pDataRx->TxDataCount++] = RtcTime & 0xFF;
                    ResponseData[pDataRx->TxDataCount++] = (RtcTime >> SHIFT_8_BITS) & 0xFF;
                    ResponseData[pDataRx->TxDataCount++] = (RtcTime >> SHIFT_16_BITS) & 0xFF;
                    ResponseData[pDataRx->TxDataCount++] = (RtcTime >> SHIFT_24_BITS) & 0xFF;

                    /* Read Date and Time */
                    Clk_GetDateTime (&DateTime);
                    Clk_DateTimeToStr (&DateTime, CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS, (char *) DateStr, sizeof(DateStr)-1);
                    Log (TRC,"Get RTC command: read 0x%08X, time %s (UTC)", RtcTime, DateStr);
                    break;
                  }
                case SERIALCMD_SET_RTC:
                  {
                    BATT_RTC_STATUS     BattStatus;  /* Battery Status */
                    RTC_SECONDS     RtcTime;         /* RTC time */
                    CLK_DATE_TIME       DateTime;     /* RTC Date and Time */
                    int8_t DateStr[DATE_STR_LEN];         /* Date and Time String*/
                    memcpy (&RtcTime, pRxData, sizeof(RtcTime));
                    L2_OnchipRtcWrite (RtcTime);                  /* handle RTC */

                    /* Read Date and Time */
                    Clk_GetDateTime (&DateTime);
                    Clk_DateTimeToStr (&DateTime, CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS, (char *) DateStr, sizeof(DateStr)-1);
                    Log (DBG, "Set RTC command: write 0x%08X, time %s (UTC)", RtcTime, DateStr);

                    BattStatus = L3_BatteryRtcWrite (&RtcTime);   /* battery RTC */
                    if (BATT_RTC_STATUS_OK == BattStatus)
                    {
                        Log (TRC,"1-Wire RTC Updated");
                    }
                    else
                    {
                        Log (ERR, "1-Wire RTC Update failed. RTC write error: %d", BattStatus);
                    }

                    Log (DBG, "SERIALCMD_SET_RTC: %s (UTC)", DateStr);
                    SecurityLog ("SERIALCMD_SET_RTC: %s (UTC)", DateStr);
                    break;
                  }
                case SERIALCMD_GET_VERSION:
                    ResponseData[pDataRx->TxDataCount++] = (SOFTWARE_VERSION >> 8) & 0xFF;
                    ResponseData[pDataRx->TxDataCount++] = SOFTWARE_VERSION & 0xFF;
                    Log (DBG, "SERIALCMD_GET_VERSION: %d ", ResponseData);
                    break;

                case SERIALCMD_BOOT_ENTER:
                    break;
                case SERIALCMD_BOOT_QUIT:
                    break;
                case SERIALCMD_FLASH_ERASE:
                    break;
                case SERIALCMD_FLASH_WRITE:
                      break;
                case SERIALCMD_FLASH_READ:
                    break;
                case SERIALCMD_SET_VERSION:
                    break;
                case SERIALCMD_ASSERT_INFO:
                    break;
                case SERIALCMD_DISPLAY_PROMPT:
                    break;

                case SERIALCMD_HARDWARE_VERSION:
                    ResponseData[pDataRx->TxDataCount++] = HARDWARE_VERSION_1;
                    pDataRx->TxDataCount++;
                    break;

                case SERIALCMD_SERIAL_BUFFER_COUNTS:
                    break;
                case SERIALCMD_LOG_TEXT:
                    break;
                case SERIALCMD_NEW_ASSERT:
                    break;

                case SERIALCMD_STREAMING_VAR_COUNT:
                    ResponseData[pDataRx->TxDataCount++] = StreamingVarCount;
                    break;

                case SERIALCMD_STREAMING_VAR_INFO:
                    TempValue = pRxData[0] & 0xFF;   // streamingVar index
                    if (TempValue < STREAMING_VARS_COUNT)
                    {
                        ResponseData[pDataRx->TxDataCount++] = pRxData[0];
                        ResponseData[pDataRx->TxDataCount++] = StreamingVarsList[TempValue].varSize & 0xFF;
                        ResponseData[pDataRx->TxDataCount++] = StreamingVarsList[TempValue].varType & 0xFF;
                        strcpy ((char *)(ResponseData + pDataRx->TxDataCount), (char *)StreamingVarsList[TempValue].varName);

                        pDataRx->TxDataCount += strlen((char *)(ResponseData + pDataRx->TxDataCount));
                    }
                    break;

                case SERIALCMD_CLEAR_STREAMING_LIST:
                    for (TempValue = 0; TempValue < StreamingVarCount; TempValue++)
                    {
                        StreamingVarsList[TempValue].isStreaming = false;
                    }
                    break;

                case SERIALCMD_ADD_STREAMING_VAR:
                    TempValue = pRxData[0] & 0xFF;   // streamingVar index
                    if (TempValue < StreamingVarCount)
                    {
                        StreamingVarsList[TempValue].isStreaming = true;
                    }
                    ResponseData[pDataRx->TxDataCount++] = pRxData[0];
                    break;

                case SERIALCMD_REMOVE_STREAMING_VAR:
                    TempValue = pRxData[0] & 0xFF;   // streamingVar index
                    if (TempValue < STREAMING_VARS_COUNT)
                    {
                        StreamingVarsList[TempValue].isStreaming = false;
                    }
                    ResponseData[pDataRx->TxDataCount++] = pRxData[0];
                    break;

                case SERIALCMD_START_STREAMING:
                    IsStreamingVarsEnabled = true;
                    break;

                case SERIALCMD_STOP_STREAMING:
                    IsStreamingVarsEnabled = false;
                    break;

                case SERIALCMD_STREAMING_RATE:
                  {
                    uint32_t  NewStreamingDataRate;   /* Update Stream Rate variable */
                    memcpy( &NewStreamingDataRate, pRxData, sizeof( NewStreamingDataRate ) );

                    // Update streaming rate
                    streamingDataRate = (NewStreamingDataRate >= DEFAULT_STREAM_RATE) ? NewStreamingDataRate : DEFAULT_STREAM_RATE;

                    // Fill Response Data
                    ResponseData[pDataRx->TxDataCount++] = streamingDataRate;
                    ResponseData[pDataRx->TxDataCount++] = ( streamingDataRate >> 8 ) & 0xFF;
                    ResponseData[pDataRx->TxDataCount++] = ( streamingDataRate >> 16 ) & 0xFF;
                    ResponseData[pDataRx->TxDataCount++] = ( streamingDataRate >> 24 ) & 0xFF;
                    break;
                  }
                case SERIALCMD_STREAMING_DATA:
                    if (IsStreamingVarsEnabled)
                    {
                        /// \todo MISSING 31-MAY-2021 SE Implemention of this command is out of scope.
                        //  this can be moved out while implementing this command as there is no request for the command>
                    }
                    break;

                case SERIALCMD_CHANGEABLE_VAR_COUNT:
                    ResponseData[pDataRx->TxDataCount++] = ChangeableVarRepo.ChangeableVarsCount;
                    break;

                case SERIALCMD_CHANGEABLE_VAR_INFO:
                  {
                    uint16_t DataLength;              /* Data length */
                    uint16_t ChangeableVarIndex;       /* Changeable variable index */
                    
                    ChangeableVarIndex = pRxData[0];   /* ChangeableVar index */
                    if (ChangeableVarIndex < ChangeableVarRepo.ChangeableVarsCount)
                    {
                        ResponseData[pDataRx->TxDataCount++] = pRxData[0];
                        ResponseData[pDataRx->TxDataCount++] = ChangeableVars[ChangeableVarIndex].VarSize & 0xFF;
                        ResponseData[pDataRx->TxDataCount++] = ChangeableVars[ChangeableVarIndex].VarType & 0xFF;
                        strncpy((char*)ResponseData + pDataRx->TxDataCount, (char*)ChangeableVars[ChangeableVarIndex].VarName, VAR_NAME_SIZE);

                        DataLength = strlen((char *)(ResponseData + pDataRx->TxDataCount));
                        pDataRx->TxDataCount += 1;

                        pDataRx->TxDataCount = DataLength + pDataRx->TxDataCount;
                    }
                    break;
                  }
                case SERIALCMD_CHANGEABLE_VAR_VALUE:
                  {
                    uint16_t DataLength;              /* Data length */
                    uint16_t ChangeableVarIndex;       /* Changeable variable index */
                    
                    ChangeableVarIndex = pRxData[0];   /* Changeable variable index */
                    if (ChangeableVarIndex < ChangeableVarRepo.ChangeableVarsCount)
                    {
                        ResponseData[pDataRx->TxDataCount++] = pRxData[0];
                        memcpy(&ResponseData[pDataRx->TxDataCount],
                               ChangeableVars[ChangeableVarIndex].pVarAddress,
                               ChangeableVars[ChangeableVarIndex].VarSize);

                        DataLength = ChangeableVars[ChangeableVarIndex].VarSize;
                        pDataRx->TxDataCount = DataLength + pDataRx->TxDataCount;
                    }
                    break;
                  }
                case SERIALCMD_CHANGEABLE_VAR_UPDATE:
                  {
                    uint16_t ChangeableVarIndex;       /* Changeable variable index */
                    
                    ChangeableVarIndex = pRxData[0];   /* Changeable variable index */
                    if (ChangeableVarIndex < ChangeableVarRepo.ChangeableVarsCount)
                    {
                        memcpy(ChangeableVars[ChangeableVarIndex].pVarAddress,
                               &pRxData[1], ChangeableVars[ChangeableVarIndex].VarSize);
                        ResponseData[pDataRx->TxDataCount++] = pRxData[0];
                    }
                    break;
                  }
                case SERIALCMD_STATUS_VAR_COUNT:
                    /* Set the StatusVarsCount to the response data */
                    ResponseData[pDataRx->TxDataCount++] = StatusVarsCount;
                    break;

                case SERIALCMD_STATUS_VAR_INFO:
                                /* Read the received offset */
                    TempValue = ResponseData[pDataRx->TxDataCount++] = pRxData[0];
                    /* check for valid offset*/
                    if (TempValue < StatusVarsCount)
                    {
                        /* populate the Response data from StatusVars */
                        ResponseData[pDataRx->TxDataCount++] = StatusVars[TempValue].VarSize & 0xFF;
                        ResponseData[pDataRx->TxDataCount++] = StatusVars[TempValue].VarType & 0xFF;
                        strncpy((char *)&(ResponseData[pDataRx->TxDataCount++]),
                                          (char const *) StatusVars[TempValue].VarName,
                                          strlen((char const *) StatusVars[TempValue].VarName));
                    }

                    /* Calculate the transmit count based on string length */
                    pDataRx->TxDataCount = pDataRx->TxDataCount + strlen((char const *) StatusVars[TempValue].VarName);

                    /*terminate with Null char */
                    ResponseData[pDataRx->TxDataCount] = '\0';
                    break;

                case SERIALCMD_STATUS_RATE:
                    /* read the status rate received and respond back with same data */
                    memcpy(&StatusDataRate, pRxData, sizeof(uint32_t));
                    memcpy(ResponseData, pRxData, sizeof(uint32_t));
                    pDataRx->TxDataCount = sizeof(uint32_t);
                    break;

                case SERIALCMD_STATUS_DATA:
                    /* only response is sent when SERIALCMD_STATUS_START is received
                       Must be handled as part of streaming task. Currently implemented in SendStatusVars function
                    */
                    break;

                case SERIALCMD_STATUS_START:
                    /* Initialize the Status transfer related variables */
                    StatusVarsEnabled = true;
                    NextStatusMilliSeconds = 0x0;
                    break;

                case SERIALCMD_STATUS_STOP:
                    /* Disable the Status transfer related variables */
                    StatusVarsEnabled = false;
                    NextStatusMilliSeconds = 0x0;
                    break;

                case SERIALCMD_DOPEN:
                    if (pConsFileRepo->OpenDir != NULL)
                    {
                        FsError = FsCloseDir(pConsFileRepo->OpenDir);
                        pConsFileRepo->OpenDir = NULL;
                    }

                    FsError = FsOpenDir((int8_t*)pRxData, &pConsFileRepo->OpenDir);
                    memcpy(ResponseData, &FsError, sizeof(FsError));
                    pDataRx->TxDataCount = sizeof(FsError);
                    break;

                case SERIALCMD_DCLOSE:
                    if (pConsFileRepo->OpenDir != NULL)
                    {
                        FsCloseDir(pConsFileRepo->OpenDir);
                        pConsFileRepo->OpenDir = NULL;
                    }
                    break;

                case SERIALCMD_FOPEN:
                    break;

                case SERIALCMD_FCLOSE:
                    break;

                case SERIALCMD_NEXT_FILE_NAME:
                  {
                    bool       DirIsEmpty; /* Directory empty status flag */
                    uint16_t   DataLength;          /* Data length */
                    DirIsEmpty = true;

                    pDataRx->TxDataCount = sizeof(FsError);
                    ResponseData[pDataRx->TxDataCount++] = FS_ENTRY_ATTRIB_NONE;
                    ResponseData[pDataRx->TxDataCount++] = DirIsEmpty;
                    ResponseData[pDataRx->TxDataCount++] = 0;   /* Empty file name */

                    do
                    {
                        if (NULL == pConsFileRepo->OpenDir)
                        {
                            /*  No directory open */
                            FsError = FS_ERR_NULL_PTR;
                            break;
                        }

                        FsError = FsReadDir(pConsFileRepo->OpenDir, &pConsFileRepo->NextDirEntry);
                        if (FS_ERR_NONE != FsError)
                        {
                            /* No more entries. Override the EOF error with a NONE error */
                            if (FS_ERR_EOF == FsError)
                            {
                                FsError = FS_ERR_NONE;
                            }
                            FsCloseDir(pConsFileRepo->OpenDir);
                            pConsFileRepo->OpenDir = NULL;
                            break;
                        }

                        /* Next valid entry */
                        memcpy(ResponseData, &FsError, sizeof(FsError));
                        pDataRx->TxDataCount = sizeof(FsError);
                        if (pConsFileRepo->NextDirEntry.Info.Attrib & FS_ENTRY_ATTRIB_DIR)
                        {
                            FsDirIsEmpty((uint8_t*)pConsFileRepo->NextDirEntry.Name, &DirIsEmpty);
                        }
                        ResponseData[pDataRx->TxDataCount++] = pConsFileRepo->NextDirEntry.Info.Attrib;
                        ResponseData[pDataRx->TxDataCount++] = DirIsEmpty;
                        strcpy((char *)(ResponseData + pDataRx->TxDataCount), pConsFileRepo->NextDirEntry.Name);

                        DataLength = strlen((char *)(ResponseData + pDataRx->TxDataCount));
                        pDataRx->TxDataCount += 1;

                        pDataRx->TxDataCount = DataLength + pDataRx->TxDataCount;

                    } while (false);

                    memcpy(ResponseData, &FsError, sizeof(FsError));
                    break;
                  }
                case SERIALCMD_CREATE_DIRECTORY:
                    FsError = FsMakeDir((int8_t*)pRxData);
                    memcpy(ResponseData, &FsError, sizeof(FsError));
                    pDataRx->TxDataCount = sizeof(FsError);
                    break;

                case SERIALCMD_CREATE_FILE:
                    FsError = FsOpen(&pTempFilePtr, (int8_t*)pRxData, FS_FILE_ACCESS_MODE_WR | FS_FILE_ACCESS_MODE_CREATE);
                    if (FS_ERR_NONE == FsError)
                    {
                        FsError = FsClose(pTempFilePtr);
                    }
                    memcpy(ResponseData, &FsError, sizeof(FsError));
                    pDataRx->TxDataCount = sizeof(FsError);
                    break;

                case SERIALCMD_FORMAT_FILESYSTEM:
                    FsError = FsFormatSDCard();
                    memcpy(ResponseData, &FsError, sizeof(FsError));
                    pDataRx->TxDataCount = sizeof(FsError);
                    break;

                case SERIALCMD_DELETE_DIRECTORY:
                    FsError = FsRemoveDir((int8_t*)pRxData);
                    memcpy(ResponseData, &FsError, sizeof(FsError));
                    pDataRx->TxDataCount = sizeof(FsError);
                    break;

                case SERIALCMD_DELETE_FILE:
                    FsError = FsDelete((int8_t*)pRxData);
                    memcpy(ResponseData, &FsError, sizeof(FsError));
                    pDataRx->TxDataCount = sizeof(FsError);
                    break;

                case SERIALCMD_RENAME_FILE:
                  {
                    uint8_t   *pNewFileName;   /* New file name */
                    uint8_t   *pOldFileName;   /* Old file name */
                    
                    pOldFileName = pRxData;
                    pNewFileName = (pRxData + strlen((char*)pOldFileName) + 1);
                    FsError = FsRename((int8_t*)pOldFileName, (int8_t*)pNewFileName);
                    memcpy(ResponseData, &FsError, sizeof(FsError));
                    pDataRx->TxDataCount = sizeof(FsError);
                    break;
                  }
                case SERIALCMD_SET_FILE_NAME:
                    if (strlen((char *)(pRxData + sizeof(uint32_t))) < FS_CFG_MAX_FULL_NAME_LEN)
                    {
                        strcpy((char*)pConsFileRepo->WorkingFileName, (char *)pRxData);
                        ResponseData[pDataRx->TxDataCount++] = 0;  /* Success indicator */
                    }
                    else
                    {
                        pConsFileRepo->WorkingFileName[0] = 0;
                        ResponseData[pDataRx->TxDataCount++] = 1;  /* Error indicator */
                    }
                    break;

                case SERIALCMD_GET_FILE_DATA:
                  {
                    uint32_t            FileOffset;                                  /* File offset */
                     uint32_t           BytesToRead;  /* Number of bytes to read */
                    uint32_t            BytesRead;     /* Number of bytes read */
                    FS_ENTRY_INFO       FsInfo;        /* File attributes */
                    bool RunDSA;                                        
                   
                    pDataRx->TxDataCount = sizeof(FsError);
                    RunDSA = false;
                    do
                    {
                        memcpy(&FileOffset, pRxData, sizeof(uint32_t));
                        FsError = FsOpen(&pTempFilePtr, (int8_t*)pRxData + sizeof(uint32_t), FS_FILE_ACCESS_MODE_RD);
                        memcpy(ResponseData, &FsError, sizeof(FsError));
                        if (FS_ERR_NONE != FsError)
                        {
                            Log(ERR, "File open error %d", FsError);
                            break;
                        }
                        if (!strncmp("securityLog",(char const*)&pRxData[RXBUFF_FILE_NAME_INDEX],strlen("securityLog")))
                        {
                           RunDSA = true;  //run DSA on security logs
                        }
                        
                        /* Get the file attributes */
                        FsError = FsGetInfo((int8_t*)pRxData + sizeof(uint32_t), &FsInfo);
                        memcpy(ResponseData, &FsError, sizeof(FsError));
                        if (FS_ERR_NONE != FsError)
                        {
                            FsError = FsClose(pTempFilePtr);
                            Log(ERR, "Error in getting file attributes %d", FsError);
                            break;
                        }

                        /* Set the file position pointer to the START */
                        FsError = FsSeek(pTempFilePtr, FileOffset, FS_FILE_ORIGIN_START);
                        memcpy(ResponseData, &FsError, sizeof(FsError));
                        if (FS_ERR_NONE != FsError)
                        {
                            FsError = FsClose(pTempFilePtr);
                            Log(ERR, "Error in setting file position to START %d", FsError);
                            break;
                        }

                        /* Read the file contents */
                        BytesToRead = MIN(FsInfo.Size - FileOffset, LARGEST_PACKET_SIZE_16BIT - FILE_SIZE_RD_OVERHEAD - PUBLIC_KEY_SIZE - SIGNATURE_SIZE);
                        FsError = FsRead(pTempFilePtr,
                                         ResponseData + sizeof(FsError) + sizeof(FileOffset) + sizeof(uint16_t),
                                         BytesToRead,
                                         (uint32_t*)&BytesRead);
                        memcpy(ResponseData, &FsError, sizeof(FsError));
                        if (FS_ERR_NONE != FsError)
                        {
                            FsError = FsClose(pTempFilePtr);
                            Log(ERR, "File read error %d", FsError);
                            break;
                        }

                        BytesToRead = (uint16_t)BytesRead;
                        if (RunDSA)
                        {
                            RunDSA = false;
                            ComputeDigitalSignature(&ResponseData[RXBUFF_DATA_START_INDEX], BytesRead);
                            BytesRead = BytesRead + PUBLIC_KEY_SIZE + SIGNATURE_SIZE;
                        }
                        /* Prepare a response with error code, offset, Data size and file data */
                        memcpy(ResponseData + sizeof(FsError), &FileOffset, sizeof(FileOffset));
                        /* File Error(2bytes) + File offset (4 bytes) */
                        memcpy(ResponseData + sizeof(FsError) + sizeof(FileOffset), (uint16_t *)&BytesRead, sizeof(uint16_t));
                        FsError = FsClose(pTempFilePtr);
                        /* Response TxCount = File Error(2bytes) + File offset (4 bytes) + Data Block Size(2 bytes) + Number of bytes Read */
                        pDataRx->TxDataCount = sizeof(FsError) + sizeof(FileOffset) + sizeof(uint16_t) + (uint16_t)BytesRead;
                        /// \\todo 02/18/2022 SE - Commenting out to decrease the load on Logger during download.
                        //Log(DBG,"Offset %ld", FileOffset);
                    } while (false);
                    break;
                  }

                case SERIALCMD_SET_FILE_DATA:
                  {
                    uint32_t            FileOffset;       /* File offset */
                    uint32_t            BytesWritten;     /* Bytes written */
                    FS_ENTRY_INFO       FsInfo;           /* File attributes */
                    ResponseData[pDataRx->TxDataCount] = SERIALCMD_SET_FILE_DATA;
                    do
                    {
                        if (0 == pConsFileRepo->WorkingFileName[0])
                        {
                            Log(ERR, "Empty file name");
                            break;
                        }
                        FsError = FS_ERR_NONE;
                        memcpy (&FileOffset, pRxData, sizeof(uint32_t));
                        memcpy (&PacketDataSize, pRxData + sizeof(uint32_t), sizeof(PacketDataSize));
                        FsError = FsOpen (&pTempFilePtr, (int8_t*)pConsFileRepo->WorkingFileName, FS_FILE_ACCESS_MODE_RDWR);
                        memcpy (ResponseData, &FsError, sizeof(FsError));
                        pDataRx->TxDataCount = sizeof(FsError);
                        memcpy (&ResponseData[pDataRx->TxDataCount], &FileOffset, sizeof(FileOffset));
                        if (FS_ERR_NONE != FsError)
                        {
                            Log (ERR, "File open error %d", FsError);
                            break;
                        }
                        FsError = FsGetInfo ((int8_t*)pConsFileRepo->WorkingFileName, &FsInfo);
                        memcpy (ResponseData, &FsError, sizeof(FsError));
                        if (FS_ERR_NONE != FsError)
                        {
                            FsError = FsClose (pTempFilePtr);
                            Log(ERR, "Error in getting file attributes %d", FsError);
                            break;
                        }

                        /* Set the file position indicator based on 'FsInfo.Size' */
                        if (FileOffset > 0)
                        {
                            if (FsInfo.Size >= FileOffset + PacketDataSize)
                            {
                                /* Set the 'FileOffset' from FS_FILE_ORIGIN_START */
                                FsError = FsSeek(pTempFilePtr, FileOffset, FS_FILE_ORIGIN_START);
                            }
                            else if (FsInfo.Size == FileOffset)
                            {
                                /* Set the offset to FS_FILE_ORIGIN_END */
                                FsError = FsSeek(pTempFilePtr, 0, FS_FILE_ORIGIN_END);
                            }
                            else
                            {
                                FsError = FS_ERR_FILE_INVALID_OFFSET;
                            }
                        }

                        memcpy(ResponseData, &FsError, sizeof(FsError));
                        if (FS_ERR_NONE != FsError)
                        {
                            FsError = FsClose(pTempFilePtr);
                            Log(ERR, "File offset error %d", FsError);
                            break;
                        }

                        FsError = FsWrite(pTempFilePtr, pRxData + sizeof(FileOffset) + sizeof(PacketDataSize), PacketDataSize, &BytesWritten);
                        memcpy (ResponseData, &FsError, sizeof(FsError));
                        if ((BytesWritten != PacketDataSize) && (FS_ERR_NONE == FsError))
                        {
                            FsError = FS_ERR_DEV_OP_FAILED;
                            memcpy (ResponseData, &FsError, sizeof(FsError));
                            Log (ERR, "File write error %d", FsError);
                        }

                        FsError = FsClose(pTempFilePtr);

                    } while (false);

                    pDataRx->TxDataCount = sizeof(FsError) + sizeof(uint32_t);
                    break;
                  }

                case SERIALCMD_GET_FILE_ATTRIB:
                  {                    
                    FS_ENTRY_INFO FsInfo;    /* File attributes */                 
                   
                    FsError = FsOpen(&pTempFilePtr, (int8_t*)pRxData, FS_FILE_ACCESS_MODE_RD);
                    if (FS_ERR_NONE == FsError)
                    {
                        FsError = FsGetInfo((int8_t*)pRxData, &FsInfo);
                        memcpy(ResponseData, &FsError, sizeof(FsError));
                        if (FS_ERR_NONE == FsError)
                        {
                            FsError = FsClose(pTempFilePtr);
                            if (!strncmp("securityLog",(char const*)&pRxData[RXBUFF_FILE_INDEX],strlen("securityLog")))
                            {                               
                               FsInfo.Size += PUBLIC_KEY_SIZE + SIGNATURE_SIZE;                                                                
                            }
                            memcpy(ResponseData + sizeof(FsError), &FsInfo, sizeof(FS_ENTRY_INFO));
                            pDataRx->TxDataCount = sizeof(FsError) + sizeof(FS_ENTRY_INFO);
                            break;
                        }

                        FsError = FsClose(pTempFilePtr);
                    }
                    
                    memcpy(ResponseData, &FsError, sizeof(FsError));
                    pDataRx->TxDataCount = sizeof(FsError);
                    break;
                  }
                case SERIALCMD_ONEWIRE_SEARCH_ALL_SLAVES:
                    break;

                case SERIALCMD_ONEWIRE_GET_CONNECTED:
                  {
                    AM_DEVICE_INFO  Info;   /* Adapter manager info */
                    for (TempValue = 0x0; TempValue < OW_NUM_OF_DEVICES ; TempValue++)
                    {
                        Signia_AdapterManagerGetInfo(Device[TempValue], &Info);
                        if (Info.State == AM_DEVICE_STATE_ACTIVE)
                        {
                            ResponseData[pDataRx->TxDataCount] |= OW_DEVICE_CONNECTED;
                            ResponseData[pDataRx->TxDataCount] |= OW_DEVICE_AUTHENTICATED;
                        }
                        else if (Info.State == AM_DEVICE_STATE_INVALID)
                        {
                             ResponseData[pDataRx->TxDataCount] |= OW_DEVICE_CONNECTED;
                             ResponseData[pDataRx->TxDataCount] &= ~OW_DEVICE_AUTHENTICATED;
                        }
                        else
                        {
                            ResponseData[pDataRx->TxDataCount] = 0x0;
                        }
                        pDataRx->TxDataCount++;
                    }
                    break;
                  }
                case SERIALCMD_ONEWIRE_GET_ADDRESS:
                  {
                    uint64_t            DeviceAddress;                               /* onewire Device address */
                    AM_DEVICE_INFO      Info; /* Adapter manager info */
                    TempValue = (uint8_t)(pRxData[0]);   /* OW_TYPE index - Refer communication protocol manual */
                    Signia_AdapterManagerGetInfo(Device[TempValue], &Info);
                    FormatDeviceAddr((uint8_t *) &(Info.DeviceUID), (uint8_t *)&DeviceAddress);
                    ResponseData[pDataRx->TxDataCount++] = pRxData[0];
                    ResponseData[pDataRx->TxDataCount++] = 0x0;
                    memcpy((ResponseData+pDataRx->TxDataCount),&(DeviceAddress), 8);
                    pDataRx->TxDataCount += (sizeof(Info.DeviceUID));
                    break;
                  }

                case SERIALCMD_ONEWIRE_GET_STATUS:
                    break;

                case SERIALCMD_ONEWIRE_WRITE_MEMORY:
                  {
                    AM_ADAPTER_IF *pDevPtr;      /* Adapter interface pointer */
                    
                    TempValue = (uint8_t)(pRxData[0]);   /* OW_TYPE index - Refer communication protocol manual */
                    pDevPtr = Signia_AdapterManagerDeviceHandle(Device[TempValue]);
                    ResponseData[pDataRx->TxDataCount++] = pRxData[0];
                    ResponseData[pDataRx->TxDataCount++] = 0x0;
                    memcpy((void *)&(pDevPtr->Data), &pRxData[1], ONEWIRE_MEMORY_TOTAL_SIZE);
                    pDevPtr->Update();
                    break;
                  }
                case SERIALCMD_ONEWIRE_READ_MEMORY:
                  {
                    AM_ADAPTER_IF *pDevPtr;      /* Adapter interface pointer */
                    TempValue = (uint8_t)(pRxData[0]);   /* OW_TYPE index - Refer communication protocol manual */
                    pDevPtr = Signia_AdapterManagerDeviceHandle(Device[TempValue]);
                    ResponseData[pDataRx->TxDataCount++] = pRxData[0];
                    ResponseData[pDataRx->TxDataCount++] = 0x0;
                    memcpy((ResponseData+pDataRx->TxDataCount), (void *)&(pDevPtr->Data), ONEWIRE_MEMORY_TOTAL_SIZE);
                    pDataRx->TxDataCount += ONEWIRE_MEMORY_TOTAL_SIZE;
                    break;
                  }
                case SERIALCMD_ONEWIRE_GET_ID:
                    break;

                case SERIALCMD_ONEWIRE_SET_ID:
                    break;

                case SERIALCMD_ONEWIRE_CLEAR_ALL_ID:
                    break;

                case SERIALCMD_ONEWIRE_DISABLE:
                    break;

                case SERIALCMD_ONEWIRE_UPLOAD_FAKE:
                    break;

                case SERIALCMD_RUN_MOTOR:
                    break;

                case SERIALCMD_COMM_TEST_SETUP:
                    break;

                case SERIALCMD_COMM_TEST_PACKET:
                    break;

                case SERIALCMD_BLOB_DATA_SETUP:
                    PrevDataOffset = DEFAULT_OFFSET;

                    /// \todo 10/20/2021 NP - Need to handle OLED OFF or better logic to be implemented
                    /// \todo 10/20/2021 NP - For now added Power Mode Standby for Switch OFF the OLED
                    Signia_PowerModeSet (POWER_MODE_STANDBY);
                    ResponseData[pDataRx->TxDataCount++] = 0;
                    ResponseData[pDataRx->TxDataCount++] = 0;
                    noInitRam.BlobValidationStatus = BLOB_VALIDATION_STATUS_UNKNOWN;
                    FSEntry_Del (BLOB_FILE_NAME, FS_ENTRY_TYPE_FILE, &FsError);
                    break;

                case SERIALCMD_BLOB_DATA_PACKET:
                  {
                    BLOB_HANDLER_STATUS Status;   /* Error status of the function */
                    memcpy(&DataOffset, pRxData, sizeof(uint32_t));

                    /* Data offset is 0x0 indicating start of download */
                    if ( DATA_OFFSET_START == DataOffset)
                    {
                        DownloadStarted = true;
                        /*The HANDLE software shall log when a HANDLE software update is attempted to the SECURITY_LOG file.- currently done for blob update from MCP */
                        SecurityLog ("Blob Update Started, offset %ld", DataOffset);
                    }

                    // Last Packet then exit
                    /// \todo 01/06/2022 CPK It is observed in the testing that the below condition is not met for last packet - can be removed?
                    if ((PrevDataOffset == DataOffset) && (DATA_OFFSET_START != PrevDataOffset))
                    {
                        memcpy (ResponseData, &DataOffset, sizeof(DataOffset));
                        pDataRx->TxDataCount = sizeof(DataOffset);
                        break;
                    }
                    PacketDataSize = pDataRx->DataSize;
                    PacketDataSize = PacketDataSize - DATA_OFFSET;
                    PrevDataOffset = DataOffset;

                    Status = L4_BlobWrite (pRxData+DATA_OFFSET, DataOffset, PacketDataSize);
                    if (Status == BLOB_STATUS_OK)
                    {
                        memcpy (ResponseData, &DataOffset, sizeof(DataOffset));
                        pDataRx->TxDataCount = sizeof(DataOffset);
                    }
                    break;
                  }

                case SERIALCMD_BLOB_DATA_VALIDATE:
                  {
                    uint8_t ValidationError = true; /* To store Validation status */
                    
                    if (L4_BlobValidate(true) == BLOB_STATUS_VALIDATED)
                    {
                       	// Handle software update and status message logged in security Log
                        SecurityLog ("HANDLE software update Validated");
                        ValidationError = false;
                    }

                    /* Blob validate is called after Blob is written using SERIALCMD_BLOB_DATA_PACKET. Using this to identify Blob write complete */
                    /* Log to SecurityLog only when Blob Update started */
                    if(DownloadStarted)
                    {
                        SecurityLog ("Blob Validate");
                        DownloadStarted = false;
                    }
                    ResponseData[pDataRx->TxDataCount++] = ValidationError; // Error indicator
                    Signia_PowerModeSet (POWER_MODE_ACTIVE);
                    break;
                  }
                case SERIALCMD_FPGA_PGM_SETUP:
                    break;

                case SERIALCMD_FPGA_PGM_ENTER_WRITE_MODE:
                    break;

                case SERIALCMD_FPGA_PGM_PACKET:
                    break;

                case SERIALCMD_FPGA_PGM_VALIDATE:
                    break;

                case SERIALCMD_ERASE_HANDLE_TIMESTAMP:
                    // Call the Erase function and return bool status true/false
                    ResponseData[pDataRx->TxDataCount++] = EraseHandleTimestamp();
                    break;

                case SERIALCMD_ERASE_HANDLE_BL_TIMESTAMP:
                    // Call the Erase function and return bool status true/false
                    ResponseData[pDataRx->TxDataCount++] = EraseHandleBLTimestamp();
                    break;

                case SERIALCMD_ERASE_JED_TIMESTAMP:
                    // Call the Erase function and return bool status true/false
                    ResponseData[pDataRx->TxDataCount++] = FPGA_EraseTimestamp();
                    break;

                case SERIALCMD_SET_JED_TIMESTAMP:
                {
                    UWORD Timestamp;
                    Timestamp.Byte[0] = pRxData[0];
                    Timestamp.Byte[1] = pRxData[1];
                    Timestamp.Byte[2] = pRxData[2];
                    Timestamp.Byte[3] = pRxData[3];
                    ResponseData[pDataRx->TxDataCount++] = FPGA_SetTimestamp(Timestamp.Word);
                    break;
                }

                case SERIALCMD_GET_JED_TIMESTAMP:
                {
                    UWORD Timestamp;
                    Timestamp.Word = FPGA_GetTimestamp();
                    ResponseData[pDataRx->TxDataCount++] = Timestamp.Byte[0];
                    ResponseData[pDataRx->TxDataCount++] = Timestamp.Byte[1];
                    ResponseData[pDataRx->TxDataCount++] = Timestamp.Byte[2];
                    ResponseData[pDataRx->TxDataCount++] = Timestamp.Byte[3];
                    break;
                }

                case SERIALCMD_ACTIVE_TIMESTAMPS:
                    break;

                case SERIALCMD_DEVICE_PROPERTIES:
                    if ( 0 == ResponseData[pDataRx->TxDataCount++] )
                    {
                        uint32_t           Flags;
                        BLOB_POINTERS      BlobPointers;
                        DEVICE_PROPERTIES *pDeviceProperties;

                        pDeviceProperties = (DEVICE_PROPERTIES *)(&ResponseData[pDataRx->TxDataCount]);

                        memset (pDeviceProperties, 0, sizeof (pDeviceProperties));

                        if ( BLOB_STATUS_VALIDATED == L4_BlobValidate (false) )
                        {
                            memcpy (&Flags, &pDeviceProperties->Flags, sizeof (uint32_t));
                            Flags |= DEVICE_PROPERTIES_MASK_BLOB_VALID;
                            memcpy (&pDeviceProperties->Flags, &Flags, sizeof (uint32_t));
                        }

                        if ( BLOB_STATUS_OK == L4_ValidateFlashActiveVersionStruct() )
                        {
                            (void) L4_GetBlobPointers (&BlobPointers);
                            (void) L4_ValidateFlashActiveVersionStruct();

                            memcpy (&Flags, &pDeviceProperties->Flags, sizeof (uint32_t));
                            Flags |= DEVICE_PROPERTIES_MASK_ACTIVE_VERSIONS_VALID;
                            memcpy (&pDeviceProperties->Flags, &Flags, sizeof(uint32_t));
                            memcpy (&pDeviceProperties->BlobVersion,           &BlobPointers.StoredBlobHeader.BlobVersion,       FIELD_SIZEOF (DEVICE_PROPERTIES,BlobVersion));
                            memcpy (&pDeviceProperties->BlobTimestamp,         &BlobPointers.StoredBlobHeader.BlobTimestamp,     FIELD_SIZEOF (DEVICE_PROPERTIES,BlobTimestamp));
                            memcpy (&pDeviceProperties->HandleTimestamp,       &BlobPointers.ActiveVersion.HandleTimestamp,      FIELD_SIZEOF (DEVICE_PROPERTIES,HandleTimestamp));
                            memcpy (&pDeviceProperties->HandleBLTimestamp,     &BlobPointers.ActiveVersion.HandleBLTimestamp,    FIELD_SIZEOF (DEVICE_PROPERTIES,HandleBLTimestamp));
                            memcpy (&pDeviceProperties->JedTimestamp,          &BlobPointers.ActiveVersion.JedTimestamp,         FIELD_SIZEOF (DEVICE_PROPERTIES,JedTimestamp));
                            memcpy (&pDeviceProperties->BlobHandleTimestamp,   &BlobPointers.StoredBlobHeader.HandleTimestamp,   FIELD_SIZEOF (DEVICE_PROPERTIES,BlobHandleTimestamp));
                            memcpy (&pDeviceProperties->BlobHandleBLTimestamp, &BlobPointers.StoredBlobHeader.HandleBLTimestamp, FIELD_SIZEOF (DEVICE_PROPERTIES,BlobHandleBLTimestamp));
                            memcpy (&pDeviceProperties->BlobJedTimestamp,      &BlobPointers.StoredBlobHeader.JedTimestamp,      FIELD_SIZEOF (DEVICE_PROPERTIES,BlobJedTimestamp));
                            memcpy (&pDeviceProperties->BlobAdaptBLTimestamp,  &BlobPointers.StoredBlobHeader.AdaptBLTimestamp,  FIELD_SIZEOF (DEVICE_PROPERTIES,BlobAdaptBLTimestamp));
                            memcpy (&pDeviceProperties->BlobEgiaTimestamp,     &BlobPointers.StoredBlobHeader.EgiaTimestamp,     FIELD_SIZEOF (DEVICE_PROPERTIES,BlobEgiaTimestamp));
                            memcpy (&pDeviceProperties->BlobEeaTimestamp,      &BlobPointers.StoredBlobHeader.EeaTimestamp,      FIELD_SIZEOF (DEVICE_PROPERTIES,BlobEeaTimestamp));
                        }

                        if ( BlobPointers.StoredBlobHeader.BlobVersion >= 2 )
                        {
                            (void) strncpy (pDeviceProperties->BlobAgileNumber,      (char *)BlobPointers.StoredBlobHeader.BlobAgileNumber,    FIELD_SIZEOF (DEVICE_PROPERTIES,BlobAgileNumber));
                            (void) strncpy (pDeviceProperties->BlobPowerPack_Rev,    (char *)BlobPointers.StoredBlobHeader.BlobPowerPackRev,   FIELD_SIZEOF (DEVICE_PROPERTIES,BlobPowerPack_Rev));
                            (void) strncpy (pDeviceProperties->BlobPowerPack_BL_Rev, (char *)BlobPointers.StoredBlobHeader.BlobPowerPackBLRev, FIELD_SIZEOF (DEVICE_PROPERTIES,BlobPowerPack_BL_Rev));
                            (void) strncpy (pDeviceProperties->BlobJed_Rev,          (char *)BlobPointers.StoredBlobHeader.BlobJedRev,         FIELD_SIZEOF (DEVICE_PROPERTIES,BlobJed_Rev));
                            (void) strncpy (pDeviceProperties->BlobAdapter_BL_Rev,   (char *)BlobPointers.StoredBlobHeader.BlobAdapterBLRev,   FIELD_SIZEOF (DEVICE_PROPERTIES,BlobAdapter_BL_Rev));
                            (void) strncpy (pDeviceProperties->BlobAdapterEGIA_Rev,  (char *)BlobPointers.StoredBlobHeader.BlobAdapterEGIARev, FIELD_SIZEOF (DEVICE_PROPERTIES,BlobAdapterEGIA_Rev));
                            (void) strncpy (pDeviceProperties->BlobAdapterEEA_Rev,   (char *)BlobPointers.StoredBlobHeader.BlobAdapterEEARev,  FIELD_SIZEOF (DEVICE_PROPERTIES,BlobAdapterEEA_Rev));
                        }
                        else
                        {
                            memset (&pDeviceProperties->BlobAgileNumber,      0, FIELD_SIZEOF (DEVICE_PROPERTIES,BlobAgileNumber));
                            memset (&pDeviceProperties->BlobPowerPack_Rev,    0, FIELD_SIZEOF (DEVICE_PROPERTIES,BlobPowerPack_Rev));
                            memset (&pDeviceProperties->BlobPowerPack_BL_Rev, 0, FIELD_SIZEOF (DEVICE_PROPERTIES,BlobPowerPack_BL_Rev));
                            memset (&pDeviceProperties->BlobJed_Rev,          0, FIELD_SIZEOF (DEVICE_PROPERTIES,BlobJed_Rev));
                            memset (&pDeviceProperties->BlobAdapter_BL_Rev,   0, FIELD_SIZEOF (DEVICE_PROPERTIES,BlobAdapter_BL_Rev));
                            memset (&pDeviceProperties->BlobAdapterEGIA_Rev,  0, FIELD_SIZEOF (DEVICE_PROPERTIES,BlobAdapterEGIA_Rev));
                            memset (&pDeviceProperties->BlobAdapterEEA_Rev,   0, FIELD_SIZEOF (DEVICE_PROPERTIES,BlobAdapterEEA_Rev));
                        }

                        if (BlobPointers.StoredBlobHeader.BlobVersion >= 3)
                        {
                            (void) strncpy (pDeviceProperties->BlobSystemVersion, (char *)BlobPointers.StoredBlobHeader.BlobSystemVersion, FIELD_SIZEOF(DEVICE_PROPERTIES,BlobSystemVersion));
                        }
                        else
                        {
                            memset (&pDeviceProperties->BlobSystemVersion, sizeof(pDeviceProperties->BlobSystemVersion), 0);
                        }

                        pDataRx->TxDataCount += sizeof (DEVICE_PROPERTIES);
                    }
                    break;

                case SERIALCMD_FAT_READENTRY:
                    break;

                case SERIALCMD_SECTOR_READ:
                    break;

                case SERIALCMD_SECTOR_WRITE:
                    break;

                case SERIALCMD_WIFI_COMMAND:
                    break;

                case SERIALCMD_KVF_DESCRIPTION:
                {
                    uint8_t       		DescriptionLength;           /*KVF description length*/	
                    uint16_t      	FileNameLength;                  /*KVF name length*/
                    KVF_ERROR     		pError;                      /*KVF file access error*/	
                    // FileNameLength includes NULL terminator
                    FileNameLength = strlen((char *)pRxData) + 1;

                    // FileNameLength
                    memcpy(ResponseData, &FileNameLength, sizeof(FileNameLength));
                    pDataRx->TxDataCount += sizeof(FileNameLength);

                    // fileName
                    memcpy(&ResponseData[pDataRx->TxDataCount], pRxData, FileNameLength);
                    pDataRx->TxDataCount += FileNameLength;

                    DescriptionLength = KvfGetDescription((char *)pRxData, (char *)(&ResponseData[ pDataRx->TxDataCount + 1 + sizeof(DescriptionLength)]), MAX_CHAR, &pError);

                    // pError
                    memcpy(&ResponseData[ pDataRx->TxDataCount], &pError, 1);
                    pDataRx->TxDataCount += 1;

                    // DescriptionLength
                    memcpy(&ResponseData[ pDataRx->TxDataCount], &DescriptionLength, sizeof(DescriptionLength));
                    pDataRx->TxDataCount += sizeof(DescriptionLength);

                    // description  (already put in place by KvfGetDescription command, above
                    pDataRx->TxDataCount += DescriptionLength;
                    ResponseData[ pDataRx->TxDataCount++] = 0;    // NULL terminator for description
                    
                    break;
                }

                case SERIALCMD_TASK_LIST:
                     /// \\todo 03/18/2022 CPK Below TASK related commands to be done during other MCP commands  - added defaults to response
                     ResponseData[pDataRx->TxDataCount++] = 0x0; /*interrupt cnt */
                     ResponseData[pDataRx->TxDataCount++] = 0x0;
                     for (TempValue = 0; TempValue <= OS_LOWEST_PRIO; TempValue++)
                     {
                         ResponseData[pDataRx->TxDataCount+TempValue] = 0x0;// task disabled
                     }
                     pDataRx->TxDataCount+= TempValue;
                     pDataRx->TxDataCount++;
                     /// \\todo 03/18/2022 CPK Below delay to overcome MCP command flooding issue. After all commands are implemented we should not need this.
                     /// \\todo 03/18/2022 CPK MCP Crashes with some responses which are not implemented. This delay overcomes that for now.
                     if (!StartuptDelay1)
                     {
                         StartuptDelay1 = true;
                         OSTimeDly(SEC_4);
                     }
                    break;

                case SERIALCMD_TASK_NAME:
                  {
                    uint16_t  TempVal1;   /* To store temp data variable */
                    /// \\todo 03/18/2022 CPK Below TASK related commands to be done during other MCP commands  - added defaults to response
                    TempValue = (uint8_t)(pRxData[0]);   /* task index */
                    if (TempValue < OS_LOWEST_PRIO)
                    {
                        ResponseData[pDataRx->TxDataCount++] = (uint8_t)TempValue;  // rcvd data
                        ResponseData[pDataRx->TxDataCount++] = 0x0;                 // No Error
                        for (TempVal1=0;TempVal1 < 4; TempVal1++)
                        {
                            ResponseData[pDataRx->TxDataCount+TempVal1] = 0x0;      // Stack size
                        }
                        pDataRx->TxDataCount+=TempVal1;                             // increment to next count
                        ResponseData[pDataRx->TxDataCount] = 0;                     // Null terminated task name

                    }
                    else //interrupt
                    {
                        ResponseData[pDataRx->TxDataCount++] = (uint8_t)TempValue;  // rcvd data
                        ResponseData[pDataRx->TxDataCount++] = 0x0;                 // No Error
                        ResponseData[pDataRx->TxDataCount] = 0;                     // Null terminated interrupt name
                    }
                    break;
                  }
                case SERIALCMD_TASK_STATS:
                    break;

                case SERIALCMD_READ_BATTERY_DATA:
                   /// \todo 11/30/2022 JA: Handle battery read command below.Below is just to avoid MCP crash                   
                   ResponseData[pDataRx->TxDataCount++] = (uint8_t)TempValue;  // rcvd data
                   ResponseData[pDataRx->TxDataCount++] = 0x0;                 // No Error
                   ResponseData[pDataRx->TxDataCount] = 0;                     // Null terminated interrupt name
                    break;

                case SERIALCMD_BATTERY_COMMAND:
                  {
                    uint8_t  *pBattData;     /* Battery data */    
                    uint8_t  BattCommand;    /* Battery command */
                    
                    BattCommand = pRxData[BAT_CMD_OFFSET];
                    if(BAT_CMD_MANUFACTURING_ACCESS_BYTE == BattCommand) // check if the command is BAT_MANUFACTURING_ACCESS_BYTE
                    {
                        pBattData = pRxData + BAT_CMD_DATA_OFFSET;
                        if((BAT_CMD_SHUTDOWN_LOWBYTE == pBattData[0]) && (BAT_CMD_SHUTDOWN_HIGHBYTE == pBattData[1])) // check if the databyte is 0x0010 (Shutdown Mode)
                        {
                            Log(DBG, "ConsoleCommands: Received Battery Command Shutdown ");
                            Signia_ShipModeReqEvent(SIGNIA_SHIPMODE_VIA_CONSOLE);
                        }
                    }
                    break;
                  }
                case SERIALCMD_BATTERY_SIMULATOR_DATA:
                    break;

                case SERIALCMD_PROFILER_TYPE_COUNT:
                    break;

                case SERIALCMD_PROFILER_TYPE_INFO:
                    break;

                case SERIALCMD_PROFILER_HISTORY_START:
                    break;

                case SERIALCMD_PROFILER_HISTORY_STOP:
                    break;

                case SERIALCMD_PROFILER_HISTORY_DATA:
                    break;

                /// \\todo 03/18/2022 CPK Below SIGNAL_TYPE related commands to be done during other MCP commands  - added defaults to response so MCP doesnt re-send the commands
                case SERIALCMD_SIGNAL_TYPE_COUNT:
                    ResponseData[pDataRx->TxDataCount++] = DUMMY_LAST_SIG;
                    ResponseData[pDataRx->TxDataCount++] = 0x00;
                     /// \\todo 03/18/2022 CPK Below delay to overcome MCP command flooding issue. After all commands are implemented we should not need this.
                     /// \\todo 03/18/2022 CPK MCP Crashes with some responses which are not implemented. This delay overcomes that for now.
                    if ( !StartuptDelay2 )
                    {
                        StartuptDelay2 = true;
                        OSTimeDly(SEC_10);
                    }
                    break;

                case SERIALCMD_SIGNAL_TYPE_INFO:
                    TempValue = pRxData[0];   /* SIGNAL index */
                    if (TempValue < DUMMY_LAST_SIG)
                    {
                        ResponseData[pDataRx->TxDataCount++] = (uint8_t)TempValue;
                        ResponseData[pDataRx->TxDataCount++] = pRxData[1];
                        /* indicates a valid SIGNAL index */
                        ResponseData[pDataRx->TxDataCount++] = VALID_SIGNAL;
                        /* Does this SIGNAL cause a log entry? */
                        ResponseData[pDataRx->TxDataCount++] = false;
                        /// \\todo 03/18/2022 CPK Use the below commented code when we have ths Signaltable defined.
                        #if 0
                        if (TempSigTable[TempValue].pString)
                        {
                            strncpy((char *)&(ResponseData[pDataRx->TxDataCount++]), (char const *)TempSigTable[TempValue].pString, strlen((char const *)TempSigTable[TempValue].pString));
                        }
                        else
                        {
                            /* No SIGNAL name */
                            ResponseData[pDataRx->TxDataCount++] = 0;
                        }
                        pDataRx->TxDataCount = pDataRx->TxDataCount + strlen((char const *)TempSigTable[TempValue].pString);
                        #endif
                        ResponseData[pDataRx->TxDataCount++] = 0;
                    }
                    else
                    {
                        /* Invalid Signal number */
                        ResponseData[pDataRx->TxDataCount++] = (uint8_t) TempValue;
                        ResponseData[pDataRx->TxDataCount++] = pRxData[1];
                        ResponseData[pDataRx->TxDataCount++] = 1;
                        ResponseData[pDataRx->TxDataCount++] = 0;
                        ResponseData[pDataRx->TxDataCount++] = 0;
                    }
                    break;

                case SERIALCMD_SIGNAL_DATA:
                    break;

                case SERIALCMD_OS_LOWEST_PRIORITY:
                    break;

                case SERIALCMD_GET_SERIALNUM:
                  {
                    AM_HANDLE_IF *pHandle;  /* Handle interface pointer */
                    uint64_t            DeviceAddress; /* onewire Device address */
                    AM_DEVICE_INFO      HandleInfo;      /* Handle information */
                    uint8_t HandleLotNumber[ONEWIRE_LOT_NUMBER_LENGTH];  /* Handle Lot Number */
                    /* Get the Handle Information */
                    Signia_AdapterManagerGetInfo(AM_DEVICE_HANDLE,&HandleInfo);
                    // Get the Handle Interface
                    pHandle = HandleGetIF();
                    // Get the Handle Lot Number
                    memcpy (&HandleLotNumber[0], &pHandle->Data.LotNumber[0], sizeof(HandleLotNumber));
                    // Convert to ASCII
                    ForceArrayToAscii(&pHandle->Data.LotNumber[0], &HandleLotNumber[0], ONEWIRE_LOT_NUMBER_LENGTH);
                    /* If Lot number is not available then Handle Serial Number is DeviceID */
                    if (0 == HandleLotNumber[0])
                    {
                        FormatDeviceAddr((uint8_t *) &(HandleInfo.DeviceUID), (uint8_t *)&DeviceAddress); /* keep the format in-line with Legacy? - adding for now */
                        sprintf ((char *)ResponseData,"%llX",(uint64_t)DeviceAddress);
                        pDataRx->TxDataCount = strlen ((char *)(ResponseData)) + 1;
                    }
                    else /* If Lot number is available then Handle Serial Number is LotNumber */
                    {
                        memcpy (&ResponseData[0], &HandleLotNumber[0], ONEWIRE_LOT_NUMBER_LENGTH);
                        pDataRx->TxDataCount = ONEWIRE_LOT_NUMBER_LENGTH + 1;
                    }                                     
                    break;
                  }
                case SERIALCMD_STRAINGAUGE:
                    break;

                case SERIALCMD_EMBED_VARS_INFO:
                    break;

                case SERIALCMD_EMBED_VARS_VALUES:
                    break;

                case SERIALCMD_TEST_CMD:
                    pTestEvt = AO_EvtNew(REQ_TEST_SIG , sizeof(QEVENT_TEST_MSG));
                    pTestEvt->size = (pDataRx->DataSize-TESTDATA_OFFSET);
                    pTestEvt->RxKey = To16U(pRxData);
                    pTestEvt->TestID = pRxData[TESTID_OFFSET];
                    memcpy (pTestEvt->Data, &pRxData[TESTDATA_OFFSET], pDataRx->DataSize);
                    AO_Post(AO_TestManager,&pTestEvt->Event,NULL);
                    ConsoleTaskNextState = CONS_MGR_STATE_WAIT_FOR_EVENT;
                    break;

                case SERIALCMD_GET_PARAMETERS:
                    break;

                case SERIALCMD_RESET_DEVICE:
                    /// \todo 10/14/2021 NP Need to handle Adapter Connected Check before Reset
                    OSTimeDly (RESET_DELAY);
                    SoftReset();    // Reset after 5 seconds to retry.
                    break;

                case SERIALCMD_ACCEL_SETTING:
                    break;

                case SERIALCMD_COUNTRY_CODE:
                  {
                    AM_HANDLE_IF *pHandle;  /* Handle interface pointer */
                    uint8_t  CmdType;     /* command Type */
                    uint16_t CountryCode; /* Country code */
                    
                    CmdType =  pRxData[0];
                    pHandle = HandleGetIF();

                    if (0 != CmdType)
                    {
                        /* Set Country Code in handle memory - received packet contains country code */
                        memcpy (&CountryCode, pRxData+1, sizeof(CountryCode));

                        // Get the Handle Interface                      
                        pHandle->FlashData.CountryCode = CountryCode;
                        pHandle->SaveFlashData();
                        Log(REQ,"Country Code write: %d", CountryCode);
                    }
                    else
                    {
                        /* Read Country Code from handle memory */
                        CountryCode = pHandle->FlashData.CountryCode;
                        Log(REQ,"Country Code read: %d", CountryCode);
                    }

                    ResponseData[pDataRx->TxDataCount++] = 0;                       /* direction */
                    ResponseData[pDataRx->TxDataCount++] = CountryCode & 0xFF;      /* add Country code to response packet */
                    ResponseData[pDataRx->TxDataCount++] = (CountryCode >> SHIFT_8_BITS) & 0xFF;
                    break;
                  }
                case SERIALCMD_GET_OPEN_FILE_DATA:
                    break;

                default:
                    ConsoleTaskNextState = CONS_MGR_STATE_WAIT_FOR_EVENT;
                    break;
            }
        }
        while (false); //To Check the Password Received flag before command processing

    } while (false);

    return ConsoleTaskNextState;
}

/* ========================================================================== */
/**
 * \brief   Converts data buffer to 16-bit number
 *
 * \details Function takes 16-bit data buffer and returns 16-bit unsigned number
 *
 * \param   pRawData - pointer to Data to be Converted
 *
 * \return   uint16_t - converted data
 *    <<< NOT CURRENTLY USED - RETAINED FOR FUTURE REFERENCE
 * ========================================================================== */
static uint16_t To16U(uint8_t *pRawData)
{
    return (((uint16_t) pRawData[1] << BITS_8) | ((uint16_t)pRawData[0]));
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Send Staus variables periodically
 *
 * \details This function sends the status variables to MCP periodically
 *          at rate defined in StatusDataRate
 *
 * \param   pArg - Task argument passed while creating the task. The OS
 *                 schedule passes this value when the task is run.
 *
 * \return   None
 * ========================================================================== */
/// \todo 06/01/2021 CPK Below function to be Integrated to streaming task - retained for debug purposes
void SendStatusVars(void *pArg)
{
    uint32_t MilliSeconds;                     /* current milliseconds */
    uint16_t VarIndex;                         /* Variable Index */
    uint16_t NextByteIndex;                    /* Byte Index */
    static uint8_t StatusResponseData[STATUS_VARS_MAX_SIZE];    /* local response data for Status vars */

    while(true)
    {
        /* perform below when SERIALCMD_STATUS_START is received and at StatusDataRate rate */
        MilliSeconds = OSTimeGet();
        if ((StatusVarsEnabled) && (MilliSeconds >= NextStatusMilliSeconds))
        {
            /* clear previous Response */
            memset(StatusResponseData, 0x0, sizeof(StatusResponseData));
            /* Get current OS millisecond tick */
            NextStatusMilliSeconds = MilliSeconds + StatusDataRate;
            NextByteIndex = 0;
            /// \todo 06/01/2021 CPK Replace below with realtime StatusVars read
            for (VarIndex = 0; VarIndex < StatusVarsCount; VarIndex++)
            {
                if (NextByteIndex + StatusVars[VarIndex].VarSize < LARGEST_PACKET_SIZE_16BIT)
                {
                    memcpy(&StatusResponseData[NextByteIndex], StatusVars[VarIndex].pVarAddress, StatusVars[VarIndex].VarSize);
                    NextByteIndex += StatusVars[VarIndex].VarSize;
                }
            }
            /* check if we have data to send */
            if (NextByteIndex > 0)
            {
                L4_ConsoleMgrSendRequest(COMM_CONN_ACTIVE, SERIALCMD_STATUS_DATA, StatusResponseData, NextByteIndex);
            }
        }
    /* delay for some time in this continuous loop */
    OSTimeDly(MSEC_100);  /// \todo 11/15/2021 KIA why 100msec?
    }
}

/* ========================================================================== */
/**
 * \brief   To Set the status of USB Port Protected Mode
 *
 * \details Function Sets the USB Port Protected Mode status
 *
 * \param   Value - Protected Mode true/false
 *
 * \return  None
 *
 * Notes :  In Protected Mode i.e. Password is required for any communication
 *   if USBPortProtectedMode is set to false(default) to indicate the Secure Mode inactive - password required
 *   if No activity after Aunthentication for 2 min USBPortProtectedMode is set to fasle again to indicate the Inactive Secure Mode
 *   if Password command is validated then USBPortProtectedMode is set to true to indicate the Secure Mode Active
 * ========================================================================== */

void SetUSBPortMode(bool Value)
{
    /* To set the USB Mode - Protected/Non-Protected as per the input value */
    USBPortProtectedMode = Value;
}

/* ========================================================================== */
/**
 * \brief   To Get the status of USB Port Protected Mode
 *
 * \details Function returns the USB Port Protected Mode status
 *
 * \param   < None >
 *
 * \return   bool - Secure Mode status
 * ========================================================================== */
bool GetUSBPortMode(void)
{
    /* To return the USB Mode - Protected/Non-Protected */
    return USBPortProtectedMode;
}

/* ========================================================================== */
/**
 * \brief   To Clear the Password Received Variable
 *
 * \details Function clears the "PasswordReceived" Variable
 *
 * \param   < None >
 *
 * \return   None
 * ========================================================================== */
void ClearPasswordReceived(void)
{
    /* Clear the password received flag */
    PasswordReceived = false;
}

/**
 *\}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

