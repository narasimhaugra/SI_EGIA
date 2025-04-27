#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_Wlan
 * \{
 *
 * \brief   Layer 3 WLAN control routines
 *
 * \details This file contains WLAN interfaces implementation.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_Wlan.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_Wlan.h"
#include "L3_GpioCtrl.h"
#include "L2_Uart.h"

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER            (LOG_GROUP_WIFI)            ///< Log group Identifier
#define WLAN_DATA_RECV_TIME_ELAPSE      (5u)                        ///< Uart Rx read time iteration value
#define WLAN_CHANNEL_MIN                (0u)                        ///< WLAN minimum channel number
#define WLAN_CHANNEL_MAX                (13u)                       ///< WLAM maximum channel number

/* AP configuration command definitions */
#define WLAN_TX_MAX_POWER               (12u)                       ///< WLAN maximum Tx power
#define WLAN_JOIN_CMD_CREATE_AP         (7u)                        ///< WLAN set wlan join value to create an AP
#define WLAN_DISABLE_AUTO_JOIN          (0u)                        ///< Disable Auto join
#define WLAN_AP_MODE_DHCP_LEASE_TIME    (2000u)                     ///< DHCP lease time
#define WLAN_DEFAULT_IP                 ("169.254.1.105")            ///< Default IP address of the AP
#define WLAN_DEFAULT_GATEWAY            ("192.168.1.1")             ///< Default gateway
#define WLAN_IP_NETMASK                 ("255.255.255.0")           ///< Subnet mask
#define WLAN_ENABLE_DHCP_SERVER_AP      (4u)                        ///< Enable DHCP sever
#define WLAN_IP_LOCALPORT               (2000u)                     ///< Local port
#define WLAN_DHCP_ENABLE                (1u)                        ///< DHCP On
#define WLAN_DHCP_DISABLE               (0u)                        ///< DHCP Off
#define WLAN_SET_IP_TCP_SERVER_CLIENT   (2u)                        ///< IP prootcol-TCP Sever and Client
#define WLAN_BAUD_RATE_230400           (230400u)                   ///< WLAN baud rate 230400
#define WLAN_TCP_IDLE_TIMER_VAL         (1u)                        ///< TCP Idle timer value. Disconnect after 1 sec of inactivity
#define WLAN_DEFAULT_SSID_NAME          ("UltraGen2")               ///< Default AP name
#define WLAN_DEFAULT_SSID_PASSWORD      ("Welcome1")                ///< Default pass phrase
#define WLAN_FIRMWARE_VERSION_LEN       (4u)                        ///< Firmware Version string length
#define WLAN_RSSI_VAL_SIZE              (2u)                        ///< Signal Strength string size
#define WLAN_MAC_ADDR_READ_SIZE         (17u)                       ///< Mac address string size
#define WLAN_RSSI_STRING_READ_OFFSET    (7u)                        ///< RSSI string read offset value
#define WLAN_VER_STRING_READ_OFFSET     (5u)                        ///< Firmware string read offset value
#define WLAN_MAC_ADDR_READ_OFFSET       (5u)                        ///< Mac address read offset value
#define WLAN_DHCP_IP_ADDR_OFFSET        (3u)                        ///< DHCP IP address read offset value
#define WLAN_ASSOC_DEV_READ_TOKEN       ("show associated")         ///< Token to read the associated devices
#define WLAN_ASSOC_DEV_READ_OFFSET      (3u)                        ///< Offset to read the response string
#define WLAN_CMD_SEND_MAX_RETRY         (3u)                        ///< Maximun retry count for sending commands

/* Time Delay definitions */
#define WLAN_DHCP_SCAN_TIME             (SEC_10)                    ///< DHCP scan for 10sec
#define WLAN_IP_SCAN_TIME               (SEC_3)                     ///< IP scan for 3 sec
#define UART_TIMEOUT_MSEC               (100u)                      ///< Uart Send Time out in ms

/* Application IP Series */
#define WLAN_APP1_IP_SERIES             ("192.168.")                ///< Application 1 IP series
#define WLAN_APP2_IP_SERIES             ("169.254.")                ///< Application 2 IP series

#define MAX_COUNT_BUFFER_CLEAR_TRY      (10u)

#define RN171_CMD_DELAY                 (MSEC_500)                  ///<Before and after entering 'command mode' 
                                                                    ///<250ms is recommended in RN171. We added 500ms 
                                                                    ///<because of some intermittent errors.

#define WLAN_CONNECTION_1                   '1'                     ///< connection number returned by show associated upon first connection

#define WLAN_POWERON_DLY     (MSEC_250)   ///< Power-on Delay
#define WLAN_UART_FLUSH_DLY  (MSEC_10)    ///< UART Flush Delay
#define WLAN_CMD_DLY         (MSEC_100)   ///< Delay after WLAN Command
#define WLAN_SEND_CMD_DLY    (MSEC_100)   ///< Delay after WLAN Send Command
#define WLAN_SEND1_CMD_DLY   (MSEC_300)   ///< Additional Delay after WLAN Send Command
#define WLAN_EXIT_CMD_DLY    (MSEC_10)    ///< Delay after WLAN Exit Command
#define WLAN_WAIT_RESP_DLY   (MSEC_250)   ///< Delay to wait for Command response
#define WLAN_DHCP_CHECK_DLY  (MSEC_500)   ///< Delay after WLAN DHCP Command
#define WLAN_POWER_OFF_DLY   (MSEC_500)   ///< Power-Off Delay
#define WLAN_UART_INIT_DLY   (SEC_1)      ///< Uart Initialization Delay
#define WLAN_UART_PKT_DLY    (25UL)       ///< Uart PACKET Delay
#define WLAN_REBOOT_DLY      (SEC_1)      ///< Delay after WLAN Reboot Command
#define WLAN_CREATE_AP_DLY   (MSEC_500)   ///< Delay after WLAN Create-AP Command
#define WLAN_SAVE_CMD_DLY    (MSEC_500)   ///< Delay after WLAN Save Command
#define WLAN_LEAVE_AP_DLY    (SEC_1)      ///< Delay after WLAN Leave AP Command
#define WLAN_JOIN_AP_DLY     (MSEC_250)   ///< Delay after WLAN Join-AP Command
#define WLAN_IP_SETTLE_DLY   (MSEC_200)   ///< Delay after Setting IP
#define WLAN_AUTH_MODE_DLY   (MSEC_20)    ///< Delay after WLAN Auth-Mode Command
#define WLAN_CLEAR_RSP_DLY   (MSEC_10)    ///< Delay after WLAN Clear Response Command

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
/// \todo 01/08/21 PR -  The native data type char is used in this file to avoid the typecast 
/// \todo 01/08/21 PR -  while copying. This will revisit later
    
typedef struct
{
    char ApName[WLAN_MAX_SSID_SIZE];              ///< Acess point name
    char ApPassword[WLAN_MAX_PASSWORD_SIZE];      ///< Access point password
    WLAN_AUTH ApAuthType;                         ///< Access point authentication type
    bool IsApConfigured;                          ///< User configuration status
} WLAN_AP_CONFIG;

typedef struct
{
    char AccessPointToJoin[WLAN_MAX_SSID_SIZE];   ///< Access point to join
    char ApJoinPassword[WLAN_MAX_PASSWORD_SIZE];  ///< Password of the Access point to join
    char RemoteAddress[WLAN_IP_ADDR_SIZE];        ///< Remote host's IP address
    bool   IsRemoteHostConnected;                 ///< Remote host connection status
    bool   IsClientConfigured;                    ///< User configuration status
} WLAN_CLIENT_CONFIG;

typedef struct
{
    WLAN_AP_CONFIG WlanApConfig;                  ///< AP configuration
    WLAN_CLIENT_CONFIG WlanClientConfig;          ///< WLAN Client Configuration
    bool IsInApMode;                              ///< Status of WLAN AP mode
    bool IsInClientMode;                          ///< Status of WLAN Client mode
    bool IsWlanPoweredOn;                         ///< WLAN Power On status
    char LocalIpAddress[WLAN_IP_ADDR_SIZE];       ///< WiFI module's IP address
    char Macaddress[WLAN_MAC_ADDR_SIZE];          ///< Mac address of the module
    WLAN_EVENT_HNDLR pHandler;
} WLAN_TOTAL_CONFIG;

typedef enum                                      ///< WLAN commands
{
    WLAN_CMD_SET_CHANNEL,                         ///< Set channel command
    WLAN_CMD_TX_POWER,                            ///< WiFi Tx power command
    WLAN_CMD_AP_CREATE,                           ///< AP create join command
    WLAN_CMD_DHCP_LEASE,                          ///< DHCP lease command
    WLAN_CMD_TCP_IDLE_TIMER,                      ///< TCP connection idle timer command
    WLAN_CMD_AP_IP_ADDR,                          ///< AP network IP address command
    WLAN_CMD_AP_GATEWAY,                          ///< AP network gateway command
    WLAN_CMD_AP_NET_MASK,                         ///< AP network mask command
    WLAN_CMD_AP_SSID,                             ///< AP mode SSID command
    WLAN_CMD_AP_PASS_PHRASE,                      ///< Pass phrase command
    WLAN_CMD_JOIN_PHRASE,                         ///< Pass phrase command to join access point
    WLAN_CMD_AP_AUTH_MODE,                        ///< Authentication mode command
    WLAN_CMD_DHCP_SERVER,                         ///< Enable/Disable DHCP server command
    WLAN_CMD_LOCAL_PORT,                          ///< Set local port command
    WLAN_CMD_BAUD_RATE,                           ///< Baud rate set command
    WLAN_CMD_AP_PROTOCOL,                         ///< Set IP protocol command
    WLAN_CMD_OPT_DEV_ID,                          ///< Set opt device id command
    WLAN_CMD_REMOTE_HOST,                         ///< Set the remote host command
    WLAN_CMD_REMOTE_PORT,                         ///< Set the remote port command
    WLAN_CMD_SAVE,                                ///< Save command
    WLAN_CMD_REBOOT,                              ///< Reboot command
    WLAN_CMD_FW_VERSION,                          ///< Firmware version command
    WLAN_CMD_GET_MAC,                             ///< Display the mac address command
    WLAN_CMD_LEAVE_AP,                            ///< Leave access point command
    WLAN_CMD_AP_JOIN,                             ///< Join access point command
    WLAN_CMD_GET_IP,                              ///< Display the IP address command
    WLAN_CMD_OPEN,                                ///< Opens TCP connection command
    WLAN_CMD_CLOSE,                               ///< Close TCP connection command
    WLAN_CMD_SHOW_ASSOC,                          ///< Show associated command
    WLAN_CMD_SHOW_RSSI,                           ///< Last received signal strength command
    WLAN_CMD_CMD_MODE_ENTER,                      ///< Enter command mode
    WLAN_CMD_CMD_MODE_EXIT,                       ///< Exit command mode
    WLAN_CMD_COUNT
} WLAN_CMD;

typedef enum                                      ///< Enter/Continue/Exit command mode 
{
	CMD_FIRST,                                    ///< Enter command mode
	CMD_CONT,                                     ///< Continue in command mode
	CMD_FIRST_LAST,                               ///< Only one command. Enter and exit command mode
	CMD_LAST                                      ///< Exit command mode                                                                   
} CMD_MODE;

typedef struct                                    ///< Command response table entries
{
    WLAN_CMD CmdId;                               ///< Command Id
    char     CmdString[WLAN_TX_BUFF_SIZE];        ///< Command string to send
    char     *pRespString;                        ///< Response string
} WLAN_CMD_RESP_TABLE;

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static WLAN_STATUS WlanPowerOff(void);
static WLAN_STATUS WlanPowerOn(void);
static WLAN_STATUS WlanSendCommand(const char *pCommandStr);
static WLAN_STATUS WlanEnterCommandMode(void);
static WLAN_STATUS WlanExitCommandMode(void);
static WLAN_STATUS WlanFlushUart(void);
static WLAN_STATUS WlanPowerModeEnable(void);
static WLAN_STATUS WlanPowerModeDisable(void);
static WLAN_STATUS WlanCreateApNetwork(char *pApName, char *pApPassword, WLAN_AUTH AuthType);
static WLAN_STATUS WlanConfigApNetworkIpAddress(void);
static WLAN_STATUS WlanSwitchToApMode(void);
static WLAN_STATUS WlanSwitchToClientMode(void);
static WLAN_STATUS WlanReadMacAddr(void);
static bool WlanCheckForDhcp(void);
static bool WlanCheckIPFromDhcp(void);
static void WlanClearResponseBuffer(void);
static void WlanUpdateLocalApConfig(char *pApName, char *pApPassword, WLAN_AUTH AuthType);
static void WlanUpdateDefaultApConfig(void);
static uint16_t WlanRecvCmdResponse(char *pRspBuffer, uint16_t SizeCmdBuffer);
static WLAN_STATUS WlanProcessCmdResp(const char *pCmdString, WLAN_CMD CmdIndex, CMD_MODE CmdMode);
static WLAN_STATUS WlanProcessAuthMode(char *pApPassword, WLAN_AUTH AuthType);
static WLAN_STATUS WlanSendCmdCheckResp(const char *pCmdString, WLAN_CMD CmdIndex);

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static WLAN_TOTAL_CONFIG WlanTotalConfig;           /* Store WiFi total configuration */
static WLAN_AP_CONFIG *pWlanApConfig;               /* Pointer to Ap config */
static WLAN_CLIENT_CONFIG *pWlanClientConfig;       /* Pointer to Client Config */

/* Command response table used to frame the command to send.
 * All the successful 'set' command response "AOK" are verified. */
static const WLAN_CMD_RESP_TABLE CmdRespTable[WLAN_CMD_COUNT] =
{
    /* CmdId                         CmdString                        pRespString */
    {WLAN_CMD_SET_CHANNEL,           "set wlan channel",              "AOK"},
    {WLAN_CMD_TX_POWER,              "set wlan tx",                   "AOK"},
    {WLAN_CMD_AP_CREATE,             "set wlan join",                 "AOK"},
    {WLAN_CMD_DHCP_LEASE,            "set dhcp lease",                "AOK"},
    {WLAN_CMD_TCP_IDLE_TIMER,        "set comm idle 0\r",             "AOK"},
    {WLAN_CMD_AP_IP_ADDR,            "set ip address",                "AOK"},
    {WLAN_CMD_AP_GATEWAY,            "set ip gateway",                "AOK"},
    {WLAN_CMD_AP_NET_MASK,           "set ip netmask",                "AOK"},
    {WLAN_CMD_AP_SSID,               "set apmode ssid",               "AOK"},
    {WLAN_CMD_AP_PASS_PHRASE,        "set apmode passphrase",         "AOK"},
    {WLAN_CMD_JOIN_PHRASE,           "set wlan phrase",               "AOK"},
    {WLAN_CMD_AP_AUTH_MODE,          "set wlan auth",                 "AOK"},
    {WLAN_CMD_DHCP_SERVER,           "set ip dhcp",                   "AOK"},
    {WLAN_CMD_LOCAL_PORT,            "set ip localport",              "AOK"},
    {WLAN_CMD_BAUD_RATE,             "set uart baud",                 "AOK"},
    {WLAN_CMD_AP_PROTOCOL,           "set ip protocol",               "AOK"},
    {WLAN_CMD_OPT_DEV_ID,            "set opt deviceid",              "AOK"},
    {WLAN_CMD_REMOTE_HOST,           "set ip host",                   "AOK"},
    {WLAN_CMD_REMOTE_PORT,           "set ip remote",                 "AOK"},
    {WLAN_CMD_SAVE,                  "save\r",                        NULL},
    {WLAN_CMD_REBOOT,                "reboot\r",                      NULL},
    {WLAN_CMD_FW_VERSION,            "ver\r",                         NULL},
    {WLAN_CMD_GET_MAC,               "get mac\r",                     NULL},
    {WLAN_CMD_LEAVE_AP,              "leave\r",                       NULL},
    {WLAN_CMD_AP_JOIN,               "join",                          NULL},
    {WLAN_CMD_GET_IP,                "get ip",                        NULL},
    {WLAN_CMD_OPEN,                  "open\r",                        NULL},
    {WLAN_CMD_CLOSE,                 "close\r",                       NULL},
    {WLAN_CMD_SHOW_ASSOC,            "show associated\r",             NULL},
    {WLAN_CMD_SHOW_RSSI,             "show rssi\r",                   NULL},
    {WLAN_CMD_CMD_MODE_ENTER,        "$$$",                           NULL},
    {WLAN_CMD_CMD_MODE_EXIT,         "exit\r",                        NULL}
};

/* Mapping of Authentication type to respective values as per RN171
 * Command reference manual */
static uint8_t AuthLookup[] = {0, 1, 2, 4};
#pragma location=".sram"   // Store wlan list in external RAM
char WifiList[400];


static char  WlanRecvdData[WLAN_MAX_BUFF_SIZE];

static char CmdString[WLAN_TX_BUFF_SIZE];        /* Command to send */

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Function to Power Off the WLAN.
 *
 * \details Power off the WLAN. Change WiFi Tx and Rx to GPIO.
 *
 * \param   None
 *
 * \return  WLAN_STATUS - WLAN status
 * \return  WLAN_STATUS_OK      - Power Off Successful
 * \return  WLAN_STATUS_ERROR   - otherwise
 *
 * ========================================================================== */
static WLAN_STATUS WlanPowerOff(void)
{
    WLAN_STATUS WlanStatus;

    WlanStatus = WLAN_STATUS_ERROR;
    
    do
    {
        /* Power off the WLAN */
        if ( GPIO_STATUS_OK != L3_GpioCtrlSetSignal(GPIO_WIFI_ENn) )
        {
            Log(ERR, "WlanPowerOff: Set GPIO_WIFI_ENn Failed");
            break;
        }
        
        if ( GPIO_STATUS_OK != L3_GpioCtrlClearSignal(GPIO_WIFI_RESETn) )
        {
            Log(ERR, "WlanPowerOff: Clear GPIO_WIFI_RESETn Failed");
            break;
        }
        
        if ( GPIO_STATUS_OK != L3_GpioCtrlClearSignal(GPIO_WIFI_FORCE_AWAKE) )
        {
            Log(ERR, "WlanPowerOff: Clear GPIO_WIFI_FORCE_AWAKE Failed");
            break;
        }

        WlanStatus = WLAN_STATUS_OK;

        /* Update the power off status */
        WlanTotalConfig.IsWlanPoweredOn = false;

        Log(DBG, "WlanPowerOff: Power Off Successful");

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Function to Power On the WLAN.
 *
 * \details Power On the WLAN. Change WiFi Tx and Rx to UART.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - WLAN status
 * \retval  WLAN_STATUS_OK      - Power On Successful
 * \retval  WLAN_STATUS_ERROR   - otherwise
 *
 * ========================================================================== */
static WLAN_STATUS WlanPowerOn(void)
{
    WLAN_STATUS WlanStatus;

    WlanStatus = WLAN_STATUS_ERROR;

    do
    {
        /* Release WiFi Reset */
        if ( GPIO_STATUS_OK != L3_GpioCtrlSetSignal(GPIO_WIFI_RESETn) )
        {
            Log(ERR, "WlanPowerOn: Set GPIO_WIFI_RESETn Failed");
            break;
        }

        if ( GPIO_STATUS_OK != L3_GpioCtrlSetSignal(GPIO_WIFI_FORCE_AWAKE) )
        {
            Log(ERR, "WlanPowerOff: Clear GPIO_WIFI_FORCE_AWAKE Failed");
            break;
        }
        WlanStatus = WLAN_STATUS_OK;

        /* Wifi Power on */
        if ( GPIO_STATUS_OK != L3_GpioCtrlClearSignal(GPIO_WIFI_ENn) )
        {
            Log(ERR, "WlanPowerOn: Clear GPIO_WIFI_ENn Failed");
            break;
        }

        /*Update the power on status */
        WlanTotalConfig.IsWlanPoweredOn = true;

        Log(DBG, "WlanPowerOn: Power On Successful");

    } while ( false );

    OSTimeDly(WLAN_POWERON_DLY );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Update the Local Access point configuration
 *
 * \details The WLAN_TOTAL_CONFIG is configured with the user provided configuration
 *
 * \param   pApName      - Pointer to the AP name
 * \param   pApPassword  - Pointer to Access point password
 * \param   AuthType     - Authentication type
 *
 * \return  None
 *
 * ========================================================================== */
static void WlanUpdateLocalApConfig(char *pApName, char *pApPassword, WLAN_AUTH AuthType)
{    
    /* Store the user provided configuration */
    strcpy(pWlanApConfig->ApName, pApName);
    strcpy(pWlanApConfig->ApPassword, pApPassword);
    pWlanApConfig->ApAuthType = AuthType;

    /* Update the status of the configuration */
    pWlanApConfig->IsApConfigured = true;
}

/* ========================================================================== */
/**
 * \brief   Update the Local Access point configuration with the default values
 *
 * \details The WLAN_TOTAL_CONFIG is configured with the default values
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void WlanUpdateDefaultApConfig(void)
{
    /* \todo 03/25/21 PR - Restore this configuration from NVM storage when available */
    /* \todo 03/25/21 PR - Until then the Handle will use hardcoded values */

    /* Update the default configuration */
    strcpy(pWlanApConfig->ApName, WLAN_DEFAULT_SSID_NAME);
    strcpy(pWlanApConfig->ApPassword, WLAN_DEFAULT_SSID_PASSWORD);

    /* Default authentication type */
    pWlanApConfig->ApAuthType = WLAN_AUTH_OPEN;
    pWlanApConfig->IsApConfigured = true;

    /* Update the default Client configuration */
    strcpy(pWlanClientConfig->AccessPointToJoin, "");
    strcpy(pWlanClientConfig->ApJoinPassword, "");
    strcpy(pWlanClientConfig->RemoteAddress, WLAN_INVALID_IP);
    pWlanClientConfig->IsRemoteHostConnected = false;
    pWlanClientConfig->IsClientConfigured = false;

    strcpy(WlanTotalConfig.LocalIpAddress, WLAN_DEFAULT_IP);
    WlanTotalConfig.IsInApMode = false;
    WlanTotalConfig.IsInClientMode = false;

    /* Read the mac address and store in the configuration */
    WlanReadMacAddr();
}

/* ========================================================================== */
/**
 * \brief   Flush the UART5.
 *
 * \details Flush the UART5. L2_UartGetRxByteCount() is called to check the buffer
 *          is completely flushed.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - UART5 Flush status
 * \retval  WLAN_STATUS_OK      - Flush successfully
 * \retval  WLAN_STATUS_ERROR   - Error in flushing the UART5
 *
 * ========================================================================== */
static WLAN_STATUS WlanFlushUart(void)
{
    UART_STATUS UartStatus;  /* Uart Status */
    WLAN_STATUS WlanStatus;  /* Wlan Status */

    UartStatus = UART_STATUS_INVALID_UART;
    WlanStatus = WLAN_STATUS_ERROR;

    do
    {
        OSTimeDly(WLAN_UART_FLUSH_DLY);

        /* Make sure that the UART5 channel is flushed */
        UartStatus = L2_UartFlush(UART5);
        if ( UART_STATUS_OK != UartStatus )
        {
            break;
        }

        WlanStatus = WLAN_STATUS_OK;

    } while ( L2_UartGetRxByteCount(UART5) );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Update the Local Access point configuration with the default values.
 *
 * \details The WLAN_TOTAL_CONFIG is configured with the default values.
 *
 * \param   pCommandStr  - Pointer to the command string.
 *
 * \return  WLAN_STATUS - Command send status
 * \retval  WLAN_STATUS_OK              - Command sent successfully
 * \retval  WLAN_STATUS_INVALID_PARAM   - Invalid arguments
 *
 * ========================================================================== */
static WLAN_STATUS WlanSendCommand(const char *pCommandStr)
{
    uint16_t   DataOutCount;        /* Command string length */
    uint16_t   UartSentCount;       /* UART sent data count */
    uint16_t   UartTotalSentCount;  /* Total Uart sent count */
    uint32_t   UartWriteTimeout;    /* Max time to wait for uart write */
    UART_STATUS UartStatus;         /* UART status */
    WLAN_STATUS WlanStatus;         /* WLAN status */
    uint16_t    UartSentBytes;      /* Number of bytes to send */

    /* Initialize the variables */
    UartStatus = UART_STATUS_INVALID_UART;
    WlanStatus = WLAN_STATUS_INVALID_PARAM;

    do
    {
        if (NULL ==  pCommandStr)
        {
            break;
        }
        if (strlen(pCommandStr) > WLAN_TX_BUFF_SIZE)
        {
            break;
        }

        /* Clear the response buffer */
        WlanClearResponseBuffer();

        DataOutCount = strlen(pCommandStr);

        /* Max time wait for the Uart write to complete */
        UartWriteTimeout = OSTimeGet() + UART_TIMEOUT_MSEC;

        do
        {
            /* Timeout check */
            if (OSTimeGet() >= UartWriteTimeout)
            {
                break;
            }

            UartSentBytes = DataOutCount;
            UartTotalSentCount = 0;

            /* Send the data via UART5 */
            UartStatus = L2_UartWriteBlock(UART5,
                                           (uint8_t*)(pCommandStr + UartTotalSentCount),
                                           UartSentBytes,
                                           &UartSentCount);

            UartSentBytes -= UartSentCount;
            UartTotalSentCount += UartSentCount;

        } while ( UartSentBytes );

        if ( UART_STATUS_OK != UartStatus )
        {
            break;
        }

        /* No error here */
        WlanStatus = WLAN_STATUS_OK;

    } while ( false );

    /* Add a delay for the command to take effect */
    OSTimeDly(WLAN_CMD_DLY);

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Response to the command send
 *
 * \details The received response is stored in the receive buffer WlanRecvdData[].
 *
 * \param   pCmdBuffer        - Pointer to the response receive buffer
 * \param   SizeCmdBuffer     - Size of the buffer
 *
 * \return  uint16_t - received data count
 *
 * ========================================================================== */
static uint16_t WlanRecvCmdResponse(char *pRspBuffer, uint16_t SizeCmdBuffer)
{
    uint16_t   UartRecvdCount;          /* UART Rx count */
    uint16_t   RecvdResponseCount;      /* Total received data count */  
    uint32_t   UartReadTimeout;         /* Max time to wait for the Uart read */
    UART_STATUS UartStatus;             /* UART status */

    /* Initialize the variables */
    UartRecvdCount = 0;
    RecvdResponseCount = 0;  
    UartStatus = UART_STATUS_INVALID_UART;

    do
    {
        if ( NULL == pRspBuffer)
        {
            Log(DBG, "WlanRecvCmdResponse: Invalid buffer");
            break;
        }

        if ( 0 == SizeCmdBuffer )
        {
            Log(ERR, "WlanRecvCmdResponse: Empty buffer");
            break;

        }

        /* Max time wait for the Uart read to complete */
        UartReadTimeout = OSTimeGet() + UART_TIMEOUT_MSEC;
        RecvdResponseCount = 0;

        do
        {
            /*   the number of bytes read from the UART is available in UartRecvdCount */
            UartStatus = L2_UartReadBlock(UART5, (uint8_t*)(pRspBuffer + RecvdResponseCount),
                                          SizeCmdBuffer - 1 - RecvdResponseCount,
                                          &UartRecvdCount);

            RecvdResponseCount += UartRecvdCount;
             /*check timeout */
            if ( OSTimeGet() > UartReadTimeout )
            {
                /* Log(DBG,"WlanRecvCmdResponse: L2_UartReadBlock timedout "); */
                break;
            }
        } while ( UartRecvdCount );
        
        if ( UART_STATUS_OK != UartStatus )
        {
            if ( UART_STATUS_RX_BUFFER_EMPTY == UartStatus )
            {
                break;
            }
            else
            {
                Log(ERR, "WlanRecvCmdResponse: Error in reading UART buffer");
                break;
            }
        }      
    } while ( false );

    /* Received buffer null terminated */
    pRspBuffer[RecvdResponseCount] = '\0';

    return RecvdResponseCount;
}

/* ========================================================================== */
/**
 * \brief   Clear the response buffer
 *
 * \details The UART response buffer is cleared and no stale Rx data is present.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void WlanClearResponseBuffer(void)
{
    uint16_t     DataRecvdCount;                           /* Received data count */
    
    /* Initialize all the variables */
    DataRecvdCount = 0;
    memset(WlanRecvdData, 0, sizeof(WlanRecvdData));
    WlanRecvdData[WLAN_MAX_BUFF_SIZE - 1] = '\0';

    DataRecvdCount = WlanRecvCmdResponse(WlanRecvdData, sizeof(WlanRecvdData) - 1);

    /* Clear the response buffer */
    while (DataRecvdCount > 0)
    {
        DataRecvdCount = WlanRecvCmdResponse(WlanRecvdData, sizeof(WlanRecvdData) - 1);
    }
}

/* ========================================================================== */
/**
 * \brief   Exit the command mode
 *
 * \details Exit the Command mode by sending the exit command.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - Command mode Exit status
 * \retval  WLAN_STATUS_OK      - Exit command mode successfully
 * \retval  WLAN_STATUS_ERROR   - Error in exiting command mode
 *
 * ========================================================================== */
static WLAN_STATUS WlanExitCommandMode(void)
{    
    uint16_t   UartSentCount;         /* Pointer to hold the sent data count */
    uint16_t   DataOutCount;          /* Command string length */
    WLAN_STATUS WlanStatus;           /* WLAN status */
    UART_STATUS UartStatus;           /* UART status */

    /* Initializing the variables */
    UartSentCount = 0;
    DataOutCount = 0;
    WlanStatus = WLAN_STATUS_OK;
    UartStatus = UART_STATUS_TX_BUSY;

    DataOutCount = strlen(CmdRespTable[WLAN_CMD_CMD_MODE_EXIT].CmdString);

    /* Send the exit command */
    UartStatus = L2_UartWriteBlock(UART5, (uint8_t*)CmdRespTable[WLAN_CMD_CMD_MODE_EXIT].CmdString,
                                   DataOutCount, &UartSentCount);
    if ( (UART_STATUS_OK != UartStatus) || (UartSentCount != DataOutCount) )
    {
        WlanStatus = WLAN_STATUS_ERROR;
    }

    OSTimeDly(WLAN_EXIT_CMD_DLY);
    /* Clear the response buffer */
    WlanClearResponseBuffer();

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Enter the command mode
 *
 * \details Enter the Command mode by sending the $$$ command.Retry WLAN_CMD_SEND_MAX_RETRY
 *          times if there is any error in entering the command mode.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - Command mode enter status
 * \retval  WLAN_STATUS_OK      - Enter command mode successfully
 * \retval  WLAN_STATUS_ERROR   - Error in entering command mode
 *
 * ========================================================================== */
static WLAN_STATUS WlanEnterCommandMode(void)
{
    uint16_t        DataRecvdCount;                       /* Received data count */
    uint16_t        UartSentCount;                        /* UART sent data count */
    uint16_t        DataOutCount;                         /* Sent data count */
    WLAN_STATUS     WlanStatus;                           /* WLAN status */
    UART_STATUS     UartStatus;                           /* UART status */
    uint8_t         CmdRetryCount;                        /* Retry Counter variable */

    /* Initialize the variables */
    DataRecvdCount = 0;
    UartSentCount = 0;
    DataOutCount = 0;
    WlanStatus = WLAN_STATUS_ERROR;
    UartStatus = UART_STATUS_INVALID_UART;
    memset(WlanRecvdData, 0, sizeof(WlanRecvdData));
    WlanRecvdData[WLAN_MAX_BUFF_SIZE - 1] = '\0';

    /* Set the retry count to max */
    CmdRetryCount = WLAN_CMD_SEND_MAX_RETRY;

    do
    {
        /* Delay before and after sending $$$ */
        OSTimeDly(WLAN_SEND_CMD_DLY);

        WlanClearResponseBuffer();

        DataOutCount = strlen(CmdRespTable[WLAN_CMD_CMD_MODE_ENTER].CmdString);

        UartStatus = L2_UartWriteBlock(UART5, (uint8_t*)CmdRespTable[WLAN_CMD_CMD_MODE_ENTER].CmdString,
                                       DataOutCount, &UartSentCount);

        /* Delay before and after sending $$$ */
        OSTimeDly(WLAN_SEND1_CMD_DLY);

        if ((UART_STATUS_OK == UartStatus) && (UartSentCount == DataOutCount))
        {
            DataRecvdCount = WlanRecvCmdResponse(WlanRecvdData, sizeof(WlanRecvdData) - 1);

            /* Module will respond to '$$$' command with either '$$$' or 'CMD' */
            /// \todo 03/24/2022 CPK consider strcmp instead of byte compare 
            if ( (DataRecvdCount == DataOutCount) &&
                ((WlanRecvdData[0] == '$') && (WlanRecvdData[1] == '$') && (WlanRecvdData[2] == '$')) ||
                ((WlanRecvdData[0] == 'C') && (WlanRecvdData[1] == 'M') && (WlanRecvdData[2] == 'D')) )
            {
                WlanStatus = WLAN_STATUS_OK;
                break;
            }
        }

    } while (CmdRetryCount--);

    OSTimeDly(WLAN_CLEAR_RSP_DLY);
    /* Clear the response buffer */
    WlanClearResponseBuffer();

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Enable WLAN Power mode
 *
 * \details This function is getting called through public function L3_WlanPowerSet()
 *          when the calling application wants to enable the Power mode(WLAN_POWER_ENABLED).
 *          In this mode, if we alreday have an AP configuration available, use that to
 *          start the access point.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - Status of the WLAN power mode
 * \retval  WLAN_STATUS_OK      - Enable the WLAN Power successful
 * \retval  WLAN_STATUS_ERROR   - Error in Enabling the WLAN Power
 *
 * ========================================================================== */
static WLAN_STATUS WlanPowerModeEnable(void)
{
    WLAN_STATUS WlanStatus;    /* Wlan Status */

    WlanStatus = WLAN_STATUS_ERROR;

    do
    {
        if (WlanTotalConfig.IsWlanPoweredOn)
        {
            /*Already in AP mode. No need to do anything */
            if (WlanTotalConfig.IsInApMode)
            {
                WlanStatus = WLAN_STATUS_OK;
                break;
            }

            if (!pWlanApConfig->IsApConfigured)
            {
                /* No config exists */
                Log(DBG, "No AP configuration exists to start the AP");
                break;
            }

            /* Start the AP with existing configuration */
            WlanStatus = L3_WlanStartAccessPoint((char*)&pWlanApConfig->ApName,
                                                 (char*)&pWlanApConfig->ApPassword,
                                                 pWlanApConfig->ApAuthType);
        }
    /* Already Power off scenario */
        else
        {
            /* Power On and start the access point if configuration exists */
            WlanStatus = WlanPowerOn();
            if (WLAN_STATUS_OK != WlanStatus)
            {
                break;
            }

            /*Already in AP mode. No need to do anything */
            if (WlanTotalConfig.IsInApMode)
            {
                WlanStatus = WLAN_STATUS_OK;
                break;
            }

            if (!pWlanApConfig->IsApConfigured)
            {
                /* No config exists */
                Log(DBG, "No AP configuration exists to start the AP");
                break;
            }

            /* Start the AP with existing configuration */
            WlanStatus = L3_WlanStartAccessPoint((char*)&pWlanApConfig->ApName,
                                                 (char*)&pWlanApConfig->ApPassword,
                                                 pWlanApConfig->ApAuthType);
        }

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Disable WLAN Power mode
 *
 * \details This function is getting called through public function L3_WlanPowerSet()
 *          when the calling application wants to enable the Power mode(WLAN_POWER_DISABLED).
 *          In this mode, we Power off the WLAN modue and keep the Handle in low power
 *          mode.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - Status of the WLAN power mode
 * \retval  WLAN_STATUS_OK      - Disable the WLAN Power successful
 * \retval  WLAN_STATUS_ERROR   - Error in Disabling the WLAN Power
 *
 * ========================================================================== */
static WLAN_STATUS WlanPowerModeDisable(void)
{
    WLAN_STATUS WlanStatus; /* Wlan Status */

    WlanStatus = WLAN_STATUS_ERROR;

    do
    {
        if (!WlanTotalConfig.IsWlanPoweredOn)
        {
            /* No action required. Already powered off state */
            Log(REQ, "WlanPowerModeDisable: Already in Powered Off State");
            WlanStatus = WLAN_STATUS_OK;
            break;

        }
        /* If already powered on, power off the Wlan */
        WlanStatus = WlanPowerOff();
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Reinitialize the client config  */
        WlanTotalConfig.IsInClientMode = false;
        pWlanClientConfig->IsRemoteHostConnected = false;
        strcpy(pWlanClientConfig->RemoteAddress, WLAN_INVALID_IP);

        Log(REQ, "WlanPowerModeDisable: Powered Off");

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   IP address configuration of the WLAN
 *
 * \details Configure the Access Point network IP address using appropriate commnads.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - Status of starting the access point
 * \retval  WLAN_STATUS_OK             - Successful in configuring the IP address
 * \retval  WLAN_STATUS_ERROR          - Error in configuring the IP address
 *
 * ========================================================================== */
static WLAN_STATUS WlanConfigApNetworkIpAddress(void)
{
    WLAN_STATUS WlanStatus;                          /* WLAN status */ 

    /* Initialize the variables */
    WlanStatus = WLAN_STATUS_ERROR;
    memset(CmdString, 0, sizeof(CmdString));

    do
    {
        /* Set the IP address */
        sprintf(CmdString, "%s %s\r", CmdRespTable[WLAN_CMD_AP_IP_ADDR].CmdString, WLAN_DEFAULT_IP);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_AP_IP_ADDR, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Set the Gateway address */
        sprintf(CmdString, "%s %s\r", CmdRespTable[WLAN_CMD_AP_GATEWAY].CmdString, WLAN_DEFAULT_GATEWAY);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_AP_GATEWAY, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Set the IP subnet mask */
        sprintf(CmdString, "%s %s\r", CmdRespTable[WLAN_CMD_AP_NET_MASK].CmdString, WLAN_IP_NETMASK);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_AP_NET_MASK, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        WlanStatus = WLAN_STATUS_OK;

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Start the WLAN Access Point network
 *
 * \details Create the Access Point network by configuring the RN171 module
 *          with appropriate commnads.
 *
 * \param   pApName      - Pointer to Access point name
 * \param   pApPassword  - Pointer to Access point password
 * \param   AuthType     - Security mode
 *
 * \return  WLAN_STATUS - Status of starting the access point
 * \retval  WLAN_STATUS_OK          - Successful in creating the access point network
 * \retval  WLAN_STATUS_ERROR       - Error in creating the access point network
 *
 * ========================================================================== */
static WLAN_STATUS WlanCreateApNetwork(char *pApName, char *pApPassword, WLAN_AUTH AuthType)
{
    WLAN_STATUS WlanStatus;                     /* WLAN status */
    
    WlanStatus = WLAN_STATUS_ERROR; /* Initialize the variables */
    memset(CmdString, 0, sizeof(CmdString));

    do
    {
        /* Set the WiFi transmit power in dBm:  1 (lowest) - 12 (highest) */
        sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_TX_POWER].CmdString, WLAN_TX_MAX_POWER);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_TX_POWER, CMD_FIRST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Create an AP network using the SSID, IP address, netmask, channel, etc.*/
        sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_AP_CREATE].CmdString, WLAN_JOIN_CMD_CREATE_AP);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_AP_CREATE, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Set the AP mode DHCP lease time */
        sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_DHCP_LEASE].CmdString, WLAN_AP_MODE_DHCP_LEASE_TIME);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_DHCP_LEASE, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Disable TCP connection timer */
        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_TCP_IDLE_TIMER].CmdString, WLAN_CMD_TCP_IDLE_TIMER, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Set Wlan Channel to 1 */
        /// \\todo 03/24/2022 CPK if channels are fixed consider making this a const string
        sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_SET_CHANNEL].CmdString, 0x1);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_SET_CHANNEL, CMD_FIRST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        WlanStatus = WlanConfigApNetworkIpAddress();
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Set the SSID for the AP (new for firmware version 4.41) */
        sprintf(CmdString, "%s %s\r", CmdRespTable[WLAN_CMD_AP_SSID].CmdString, pApName);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_AP_SSID, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Give the module time to recalculate the PSK */
        OSTimeDly(WLAN_WAIT_RESP_DLY);

        /* Enables DHCP server in AP mode */
        sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_DHCP_SERVER].CmdString, WLAN_ENABLE_DHCP_SERVER_AP);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_DHCP_SERVER, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Set the local port */
        sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_LOCAL_PORT].CmdString, WLAN_IP_LOCALPORT);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_LOCAL_PORT, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Set the Wlan baud rate */
        sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_BAUD_RATE].CmdString, WLAN_BAUD_RATE_230400);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_BAUD_RATE, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Set authentication commands */
        WlanStatus = WlanProcessAuthMode(pApPassword, AuthType);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Give the module time to process authmode */
        OSTimeDly(WLAN_AUTH_MODE_DLY);

        /* Store the settings */
        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_SAVE].CmdString, WLAN_CMD_SAVE, CMD_LAST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* No error here */
        WlanStatus = WLAN_STATUS_OK;

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Switch to Access point mode.
 *
 * \details This function is getting called through public function L3_WlanNetworkModeSet()
 *          when the calling application wants to switch to AP mode(WLAN_MODE_AP).
 *          In this mode, if we alreday have an AP configuration available, use that to
 *          start the access point.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in switching to AP mode.
 * \retval  WLAN_STATUS_ERROR           - Error in switching to AP mode.
 *
 * ========================================================================== */
static WLAN_STATUS WlanSwitchToApMode(void)
{
    WLAN_STATUS WlanStatus;       /* Wlan Status */

    WlanStatus = WLAN_STATUS_ERROR;

    do
    {
        /* Check the Wlan is in AP mode */
        if (WlanTotalConfig.IsInApMode)
        {
           Log(DBG, "WlanSwitchToApMode:Already in access point mode");
           WlanStatus = WLAN_STATUS_OK;
           break;
        }

        /* If AP is configured, start the access point */
        if(!pWlanApConfig->IsApConfigured)
        {
            Log(DBG, "WlanSwitchToApMode:No AP configuration available");
            break;
        }

        WlanStatus = L3_WlanStartAccessPoint((char*)&pWlanApConfig->ApName,
                                             (char*)&pWlanApConfig->ApPassword,
                                             pWlanApConfig->ApAuthType);


    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Switch to Client mode.
 *
 * \details This function is getting called through public function L3_WlanNetworkModeSet()
 *          when the calling application wants to switch to Client mode(WLAN_MODE_CLIENT).
 *          In this mode, if we alreday have a Client configuration available, use that to
 *          join the access point.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in switching to AP mode.
 * \retval  WLAN_STATUS_ERROR           - Error in switching to AP mode.
 *
 * ========================================================================== */
static WLAN_STATUS WlanSwitchToClientMode(void)
{
    WLAN_STATUS WlanStatus;       /* Wlan Status */

    WlanStatus = WLAN_STATUS_ERROR;

    do
    {
        /* if in AP mode switch to Client mode */
        if (WlanTotalConfig.IsInClientMode)
        {
            Log(DBG, "WlanSwitchToClientMode:Already in Client mode");
            WlanStatus = WLAN_STATUS_OK;
            break;
        }

        /* If Client is configured, join the access point */
        if (!pWlanClientConfig->IsClientConfigured)
        {
            Log(DBG, "No Client configuration available");
            break;
        }

        WlanStatus = L3_WlanJoinAccessPoint(pWlanClientConfig->AccessPointToJoin, pWlanClientConfig->ApJoinPassword);

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   DHCP received status
 *
 * \details After join an access point, the access point reply has a "DHCP in " string
 *
 * \param   < None >
 *
 * \return  bool - Status of "DHCP in " received
 *
 * ========================================================================== */
static bool WlanCheckForDhcp(void)
{
    uint32_t       DhcpScanTime;                     /* DHCP scan time */
    uint16_t       DataRecvdCount;                   /* Data received count */
    bool           IsDhcpReceived;                   /* Check the DHCP status */
    bool           IsAssociated;                     /* Check for the AP association */
    char           *pDhcpIp;                         /* Pointer to the version token string */
    char           *pIpToken;                        /* Pointer to the version token string */

    DhcpScanTime = 0;
    DataRecvdCount = 0;
    IsDhcpReceived = false;
    IsAssociated = false;
    memset(WlanRecvdData, 0, sizeof(WlanRecvdData));
    WlanRecvdData[WLAN_MAX_BUFF_SIZE - 1] = '\0';

    /* Scan for "DHCP in" string in the reply. Break if we get early */
    DhcpScanTime = OSTimeGet() + WLAN_DHCP_SCAN_TIME;  /* Scan for 10s */

    do
    {
        DataRecvdCount = WlanRecvCmdResponse(WlanRecvdData, sizeof(WlanRecvdData) - 1);
        if ( DataRecvdCount > 0)
        {
            /* Check for the 'Associated' string in the response */
            if ( !IsAssociated )
            {
                if ( strstr(WlanRecvdData, "Associated!") )
                {
                    IsAssociated = true;
                    DhcpScanTime += WLAN_DHCP_SCAN_TIME;
                }
            }
            /* Check for the DHCP string in the response */
            if ( !IsDhcpReceived )
            {
                if ( strstr(WlanRecvdData, "DHCP in ") )
                {
                    IsDhcpReceived = true;

                    /* Also read the IP */
                    /* The response string comes with a 'IP=xxx.xxx.xxx.xxx:' for the version information */
                    pDhcpIp = strstr(WlanRecvdData, "IP");
                    if( pDhcpIp )
                    {
                        pIpToken = strtok(pDhcpIp, ":");
                        strncpy(WlanTotalConfig.LocalIpAddress, pIpToken+WLAN_DHCP_IP_ADDR_OFFSET, strlen(pIpToken));

                        while (pIpToken != NULL )
                        {
                            pIpToken = strtok(NULL, ":");
                        }
                    }

                    break;
                }
            }

            /* Check for connection failed message */
            if ( (strstr((char const*)WlanRecvdData, "Disconn from ")) ||
                 (strstr((char const*)WlanRecvdData, "FAILED"))        ||
                 (strstr((char const*)WlanRecvdData, "AUTH-ERR")))
            {
                break;
            }
        }

        OSTimeDly(WLAN_DHCP_CHECK_DLY);

    } while ( (OSTimeGet() < DhcpScanTime) );

    return IsDhcpReceived;
}

/* ========================================================================== */
/**
 * \brief   IP received status
 *
 * \details After join an access point, the access point reply with a valid IP address
 *
 * \param   < None >
 *
 * \return  bool - Status of valid IP address received
 *
 * ========================================================================== */
static bool WlanCheckIPFromDhcp(void)
{
    uint32_t       IpScanTime;                        /* IP adress scan time */
    uint16_t       DataRecvdCount;                    /* Data received count */
    bool           IsIpReceived;                      /* IP received status */

    IpScanTime = 0;
    DataRecvdCount = 0;
    IsIpReceived = false;
    memset(WlanRecvdData, 0, sizeof(WlanRecvdData));
    WlanRecvdData[WLAN_MAX_BUFF_SIZE - 1] = '\0';

    /* Scan IP for 3 sec */
    IpScanTime = OSTimeGet() + WLAN_IP_SCAN_TIME;
    do
    {
        DataRecvdCount = WlanRecvCmdResponse(WlanRecvdData, sizeof(WlanRecvdData) - 1);
        if ( DataRecvdCount > 0 )
        {
            if ( (strstr((char const*)WlanRecvdData, WLAN_APP1_IP_SERIES)) ||
                 (strstr((char const*)WlanRecvdData, WLAN_APP2_IP_SERIES)) )
            {
                IsIpReceived = true;
                break;
            }
        }

    } while ( OSTimeGet() < IpScanTime );

    return IsIpReceived;
}

/* ========================================================================== */
/**
 * \brief   Process the command send and response received.
 *
 * \details All the 'set' commands are verified against the "AOK" response.
 *
 * \param   pCmdString  - pointer to Command string to send
 * \param   CmdIndex    - Index of the command in the 'CmdRespTable'
 *
 * \return  WLAN_STATUS - Status
 * \retval  WLAN_STATUS_OK          - Successful in processing the command and response.
 * \retval  WLAN_STATUS_ERROR       - Error in processing the command and response.
 *
 * ========================================================================== */
static WLAN_STATUS WlanSendCmdCheckResp(const char *pCmdString, WLAN_CMD CmdIndex)
{
    uint16_t      DataRecvdCount;                    /* Response received count */
    char          *pRespToken;                       /* Pointer to the response token string */
    WLAN_STATUS   WlanStatus;                        /* WLAN Status */

    WlanStatus = WLAN_STATUS_ERROR;
    memset(WlanRecvdData, 0, sizeof(WlanRecvdData));

    do
    {
        /* Send the command */
        WlanStatus = WlanSendCommand(pCmdString);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Process the response for all the 'set' commands */
        if ( CmdRespTable[CmdIndex].pRespString )
        {
            WlanStatus = WLAN_STATUS_ERROR;

            OSTimeDly(WLAN_SEND_CMD_DLY);
            DataRecvdCount = WlanRecvCmdResponse(WlanRecvdData, sizeof(WlanRecvdData) - 1);
            if ( DataRecvdCount > 0 )
            {
                pRespToken = strstr(WlanRecvdData, CmdRespTable[CmdIndex].pRespString);
                /* Debug purpose ..to be removed after testing */
                /*Log(DBG,"R %s",CmdRespTable[CmdIndex].pRespString);*/
                if ( pRespToken )
                {
                    WlanStatus = WLAN_STATUS_OK;
                }
            }
        }

    } while (false);

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Process the command send and response received.
 *
 * \details Process the command and response.Retry the command response If there
 *          is any error.
 *
 * \param   pCmdString  - Command string to send
 * \param   CmdIndex    - Index of the command in the 'CmdRespTable'
 * \param   CmdMode     - Enter/Continue/Exit in command mode
 *
 * \return  WLAN_STATUS - Status
 * \retval  WLAN_STATUS_OK          - Successful in processing the command and response.
 * \retval  WLAN_STATUS_ERROR       - Error in processing the command and response.
 *
 * ========================================================================== */
static WLAN_STATUS WlanProcessCmdResp(const char *pCmdString, WLAN_CMD CmdIndex, CMD_MODE CmdMode)
{
    WLAN_STATUS   WlanStatus;          /* WLAN Status */
    uint8_t       CmdRetryCount;       /* Retry Counter variable */

    WlanStatus = WLAN_STATUS_ERROR;

    /* Set the retry count to max */
    CmdRetryCount = WLAN_CMD_SEND_MAX_RETRY;

    do
    {
        /* Enter Command mode for the first command */
        if ( (CmdMode == CMD_FIRST) || (CmdMode == CMD_FIRST_LAST) )
        {
            WlanStatus = WlanEnterCommandMode();
            if ( WLAN_STATUS_OK != WlanStatus )
            {
                Log(DBG, "Error in enter CMD mode");
                break;
            }
        }
        // Log(DBG,"wlan cmd: %s ", pCmdString);
        /* Sending command and response checking. Retry this, if any error */
        do
        {
            WlanStatus = WlanSendCmdCheckResp(pCmdString, CmdIndex);
            if (WLAN_STATUS_OK == WlanStatus)
            {
                break;
            }
        } while (CmdRetryCount--);

        if (WLAN_STATUS_OK != WlanStatus)
        {
            Log(ERR, "Error in WLAN Command Response");
            break;
        }

        /* Exit Command mode for the last command */
        if ( (CmdMode == CMD_LAST) || (CmdMode == CMD_FIRST_LAST) )
        {
            WlanStatus = WlanExitCommandMode();
            if ( WLAN_STATUS_OK != WlanStatus )
            {
                break;
            }
        }

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Process the various Authentication modes.
 *
 * \details Different commands are set based on the authentication modes.
 *
 * \param   pApPassword - Pointer to password to set
 * \param   AuthType    - Authentication type
 *
 * \return  WLAN_STATUS - Status
 * \retval  WLAN_STATUS_OK          - Successful in setting the authentication mode.
 * \retval  WLAN_STATUS_ERROR       - Error in setting the authentication mode.
 *
 * ========================================================================== */
static WLAN_STATUS WlanProcessAuthMode(char *pApPassword, WLAN_AUTH AuthType)
{
    WLAN_STATUS WlanStatus;                     /* WLAN status */

    WlanStatus = WLAN_STATUS_ERROR;
    memset(CmdString, 0, sizeof(CmdString));

    do
    {
        /* Set authentication mode */
        sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_AP_AUTH_MODE].CmdString, AuthLookup[AuthType]);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_AP_AUTH_MODE, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        if ( WLAN_AUTH_OPEN !=  AuthType )
        {
            sprintf(CmdString, "%s %s\r", CmdRespTable[WLAN_CMD_AP_PASS_PHRASE].CmdString, pApPassword);           
        }
        else
        {
            sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_AP_PASS_PHRASE].CmdString, NULL);
        }

        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_AP_PASS_PHRASE, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        WlanStatus = WLAN_STATUS_OK;

    } while ( false );

    return WlanStatus;
}

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Read the Mac address.
 *
 * \details Read the WiFi module's mac address and store in the configuration.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK         - Successful in retrieving the Mac address.
 * \retval  WLAN_STATUS_ERROR      - Error in retrieving the Mac address.
 *
 * ========================================================================== */
WLAN_STATUS WlanReadMacAddr(void)
{
    uint16_t    DataRecvdCount;                    /* Command response received count */
    char        *pMacToken;                        /* Pointer to the mac address string */
    WLAN_STATUS WlanStatus;                        /* WLAN status */    

    /* Initializing the variables */
    DataRecvdCount = 0;
    WlanStatus = WLAN_STATUS_ERROR;
    memset(WlanRecvdData, 0, sizeof(WlanRecvdData));   
    WlanRecvdData[WLAN_MAX_BUFF_SIZE - 1] = '\0';

    do
    {
        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_GET_MAC].CmdString, WLAN_CMD_GET_MAC, CMD_FIRST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            Log(DBG, "L3_WlanGetMacAddr: Send command error");
            break;
        }

        DataRecvdCount = WlanRecvCmdResponse(WlanRecvdData, sizeof(WlanRecvdData) - 1);

        if (DataRecvdCount > 0)
        {
            /* The response string comes with a 'Mac Addr=xx:xx:xx:xx:xx:xx' for the Mac address */
            pMacToken = strstr(WlanRecvdData, "Addr");
            strncpy(WlanTotalConfig.Macaddress, pMacToken+WLAN_MAC_ADDR_READ_OFFSET, WLAN_MAC_ADDR_READ_SIZE);
        }

        WlanStatus = WlanExitCommandMode();
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            Log(DBG, "L3_WlanGetMacAddr:Error in exiting command mode");
            break;
        }

        WlanStatus = WLAN_STATUS_OK;

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Function to check the WLAN connections.
 *
 * \details checks the connections associated with the module.
 *         As per the RN171 command reference 
 *            use the show associated command to see a list of devices associated with the module.
 *
 *            The command output is in the following format with commas delimiting the fields:
 *            Connection number, Host MAC Address, Received byte count, Transmitted byte count, Seconds since last packet received
 *            Exmple output : 1,f0:cb:a1:2b:63:59,36868,0,7
 *            
 * \param   < None >
 *  
 * \return  WLAN_STATUS - Status of the initialization
 * \retval  WLAN_STATUS_OK      - Initialization successful
 * \retval  WLAN_STATUS_ERROR   - Error in Initializing
 *
 * ========================================================================== */
void L3_WlanCheckConnection(void)
{
    char *pWifiList;
    WLAN_STATUS WlanStatus;
    
    if( true == WlanTotalConfig.IsWlanPoweredOn )
    {
        pWifiList = (char *)WifiList;
        /// \\todo 03/18/2022 CPK check also for KVF WLAN configuration (Enabled/Disabled). 
        /// \\todo 03/18/2022 CPK Add WLAN_EVENT_DISCONNECT handling
        
        WlanStatus = L3_WlanListDevice(pWifiList);

        if ( WLAN_STATUS_OK == WlanStatus )
        {
            //Log(DBG, "Wifi List %s", pWifiList);
        }
        
        if ( pWifiList[0] == WLAN_CONNECTION_1 )
        { /// \\todo 09/05/2022 JA: Check if this code can be optimised. Not sure of the original intent.
            WlanTotalConfig.pHandler(WLAN_EVENT_CONNECT);
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Function to initialize the WLAN.
 *
 * \details Initialize the WLAN.
 *            
 * \param   < None >
 *  
 * \return  WLAN_STATUS - Status of the initialization
 * \retval  WLAN_STATUS_OK      - Initialization successful
 * \retval  WLAN_STATUS_ERROR   - Error in Initializing
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanInit(void)
{
    WLAN_STATUS WlanStatus;     /* WLAN status */
    UART_STATUS UartStatus;     /* Uart Status */

    WlanStatus = WLAN_STATUS_ERROR;
    UartStatus = UART_STATUS_BAUD_ERR;

    do
    {
        /* Initialize the WLAN Configuration */
        memset(&WlanTotalConfig, 0x00, sizeof(WlanTotalConfig));
        pWlanApConfig = &WlanTotalConfig.WlanApConfig;
        pWlanClientConfig = &WlanTotalConfig.WlanClientConfig;

        /* Power off the WLAN */
        WlanStatus = WlanPowerOff();
        
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        OSTimeDly(WLAN_POWER_OFF_DLY);

        /* Power On the WLAN */
        WlanStatus = WlanPowerOn();
        
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Initialize the UART to set to high baud rate */
        UartStatus = L2_UartInit(UART5, WLAN_BAUD_RATE_230400);

        if ( UART_STATUS_OK != UartStatus )
        {
            break;
        }

        OSTimeDly(WLAN_UART_INIT_DLY);
        
        /* Flush the UART5 before use */
        WlanStatus = WlanFlushUart();
        
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Update the default WiFi Total configuration */
        WlanUpdateDefaultApConfig();
        WlanStatus = L3_WlanPowerSet(WLAN_POWER_ENABLED);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

       /* No error here */
       WlanStatus = WLAN_STATUS_OK;

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   WLAN Power mode
 *
 * \details To Enable or Disable the WLAN Power.
 *
 * \param   PowerMode  - Enable/Disable the power mode.
 *
 * \return  WLAN_STATUS - Status of the WLAN power mode
 * \retval  WLAN_STATUS_OK      - Enable/Disable the WLAN Power successful
 * \retval  WLAN_STATUS_ERROR   - Error in Enable/Disable the WLAN Power
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanPowerSet(WLAN_POWER_MODE PowerMode)
{
    WLAN_STATUS WlanStatus;   /* WLAN status */

    /* Initialize the variable */
    WlanStatus = WLAN_STATUS_ERROR;

    switch ( PowerMode )
    {
        case WLAN_POWER_ENABLED:
            /* Enable the Power mode */
            WlanStatus = WlanPowerModeEnable();
            break;
        case WLAN_POWER_DISABLED:
            /* Disable the Power mode */
            WlanStatus = WlanPowerModeDisable();
            break;
        default:
            Log(DBG, "This power mode is not supported");
            break;
    }

  return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   WLAN Channel setting
 *
 * \details To set a WLAN channnel.
 *
 * \param   Channel  - The channel number to set.
 *
 * \return  WLAN_STATUS - Channel set status
 * \retval  WLAN_STATUS_OK              - Successful in setting the desired channel
 * \retval  WLAN_STATUS_INVALID_PARAM   - Invalid Channel number
 * \retval  WLAN_STATUS_ERROR           - Error in setting the channel
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanChannelSet(uint8_t Channel)
{
    WLAN_STATUS WlanStatus;                     /* WLAN status */

    /* Initialize the variables */
    WlanStatus = WLAN_STATUS_ERROR;
    memset(CmdString, 0, sizeof(CmdString));

    do
    {
        /* Check the channel validity */
        if ( Channel > WLAN_CHANNEL_MAX )
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }
        /* Send the command */
        sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_SET_CHANNEL].CmdString, Channel);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_SET_CHANNEL, CMD_FIRST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }
        /* Save the settings */
        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_SAVE].CmdString, WLAN_CMD_SAVE, CMD_LAST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

    } while ( false );

  return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   List the associated devices
 *
 * \details The list of decives associated with the Access Point.
 *
 * \param   pDeviceList  - Pointer to the list of devices.
 *
 * \return  WLAN_STATUS - Status of getting the list
 * \retval  WLAN_STATUS_OK          - Successful in getting the devices list
 * \retval  WLAN_STATUS_ERROR       - Error in getting the device list
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanListDevice(char *pDeviceList)
{
    uint16_t      DataRecvdCount;                       /* Command response received count */
    WLAN_STATUS   WlanStatus;                           /* WLAN status */
    char          *pDevListToken;                       /* Device List token */
    uint8_t       ReadTokenLength;                      /* Token length */
    uint16_t      Index;                                /* Device list index */

    char DeviceList[WLAN_AP_DEV_LIST_SIZE];    /* Device list buffer */
    
    /* Initializing the variables */
    DataRecvdCount = 0;
    WlanStatus = WLAN_STATUS_ERROR;
    memset(WlanRecvdData, 0, sizeof(WlanRecvdData));
    memset(DeviceList, 0, sizeof(DeviceList));
    WlanRecvdData[WLAN_MAX_BUFF_SIZE - 1] = '\0';

    do
    {
        if ( NULL == pDeviceList )
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }

        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_SHOW_ASSOC].CmdString, WLAN_CMD_SHOW_ASSOC, CMD_FIRST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        WlanStatus = WLAN_STATUS_ERROR;

        DataRecvdCount = WlanRecvCmdResponse(WlanRecvdData, sizeof(WlanRecvdData) - 1);

        if ( DataRecvdCount > 0 )
        {
            /* Read the list with token WLAN_ASSOC_DEV_READ_TOKEN
             * e.g list: "show associated\r\r\n< dev_list data >\r\n" */
            ReadTokenLength = strlen(WLAN_ASSOC_DEV_READ_TOKEN);
            pDevListToken =  strstr(WlanRecvdData, WLAN_ASSOC_DEV_READ_TOKEN);
            if ( pDevListToken != NULL )
            {
                WlanStatus = WLAN_STATUS_OK;

                /* Copy only the device list information */
                strncpy(DeviceList, pDevListToken + ReadTokenLength  + WLAN_ASSOC_DEV_READ_OFFSET,
                        WLAN_AP_DEV_LIST_SIZE);

                /* Prepare a semicolon separated device string */
                for ( Index = 0; Index < WLAN_AP_DEV_LIST_SIZE; Index++ )
                {
                    if ( DeviceList[Index] == '\n' )
                    {
                        DeviceList[Index] = ';';
                    }
                }

                strncpy(pDeviceList, DeviceList, WLAN_AP_DEV_LIST_SIZE);
            }
        }

        WlanStatus = WlanExitCommandMode();
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }
    } while ( false );

  return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   WLAN Signal strength
 *
 * \details Function to get the Signal strength of the WLAN
 *
 * \note    \todo The signal strength was always coming as 0 dBm from the module. This function can be removed if its a module issue.
 *
 * \param   pRssiStrength - Pointer to WiFi received signal strength indicator
 *
 * \return  WLAN_STATUS - status
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanGetSignalStrength(uint8_t *pRssiStrength)
{
    uint16_t    DataRecvdCount;                    /* Command response received count */
    static char *pRssiToken;                       /* Pointer to the rssi token string */
    WLAN_STATUS WlanStatus;                        /* WLAN status */
    static char RssiString[WLAN_RSSI_VAL_SIZE];    /* RSSI string */

    /* Initializing the variables */
    DataRecvdCount = 0;
    WlanStatus = WLAN_STATUS_ERROR;
    memset(WlanRecvdData, 0, sizeof(WlanRecvdData));
    WlanRecvdData[WLAN_MAX_BUFF_SIZE - 1] = '\0';
    memset(RssiString, 0, sizeof(RssiString));

    do
    {
        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_SHOW_RSSI].CmdString, WLAN_CMD_SHOW_RSSI, CMD_FIRST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            Log(DBG, "L3_WlanGetSignalStrength: Send command error");
            break;
        }

        DataRecvdCount = WlanRecvCmdResponse(WlanRecvdData, sizeof(WlanRecvdData) - 1);

        if (DataRecvdCount > 0)
        {
            /* The signal strength has a 'RSSI' token e.g. RSSI=(-50) dBm*/
            pRssiToken = strstr(WlanRecvdData, "RSSI");
            if ( pRssiToken )
            {
                strncpy(RssiString, pRssiToken+WLAN_RSSI_STRING_READ_OFFSET, WLAN_RSSI_VAL_SIZE);
                *pRssiStrength = atoi(RssiString);
            }
        }

        WlanStatus = WlanExitCommandMode();
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            Log(DBG, "L3_WlanGetSignalStrength:Error in exiting command mode");
            break;
        }

        WlanStatus = WLAN_STATUS_OK;

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Enable/Disable DHCP
 *
 * \details If enabled along with Access point configuration, enables the local
 *          DHCP server. If disable, set back to the default state.
 *
 * \param   DhcpMode   - Enable/Disable the local DHCP server
 *
 * \return  WLAN_STATUS - Status of DHCP Enable/Disable
 * \retval  WLAN_STATUS_OK         - Successful in Enable/Disable the DHCP Server
 * \retval  WLAN_STATUS_ERROR      - Error in Enable/Disable the DHCP Server
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanDhcpEnable(bool DhcpMode)
{
    WLAN_STATUS WlanStatus;                      /* WLAN status */

     /* Initilaze the variables */
    WlanStatus = WLAN_STATUS_ERROR;
    memset(CmdString, 0, sizeof(CmdString));

    do
    {
        if(DhcpMode)
        {
            /* Enable the DHCP Server */
            sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_DHCP_SERVER].CmdString, WLAN_ENABLE_DHCP_SERVER_AP);
            WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_DHCP_SERVER, CMD_FIRST);
        }
        else
        {
            /* Set back to DHCP On(default) */
            sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_DHCP_SERVER].CmdString, WLAN_DHCP_ENABLE);
            WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_DHCP_SERVER, CMD_FIRST);
        }

        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Save the settings */
        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_SAVE].CmdString, WLAN_CMD_SAVE, CMD_LAST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* No error here */
        WlanStatus = WLAN_STATUS_OK;

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Reboot the WLAN.
 *
 * \details Reboot the WLAN by sending the reboot command.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - Status of reboot
 * \retval  WLAN_STATUS_OK         - Successful in rebooting the WLAN
 * \retval  WLAN_STATUS_ERROR      - Error in rebooting the WLAN
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanReboot(void)
{
    WLAN_STATUS WlanStatus;                      /* WLAN status */

     /* Initilaze the variables */
     WlanStatus = WLAN_STATUS_OK;
    
    /* Reboot the WLAN */
     WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_REBOOT].CmdString, WLAN_CMD_REBOOT, CMD_FIRST);
    if ( WLAN_STATUS_OK != WlanStatus )
    {
        Log(DBG, "L3_WlanReboot: Send command error");
        WlanStatus = WLAN_STATUS_ERROR;
    }

    OSTimeDly(WLAN_REBOOT_DLY);

    if ( WLAN_STATUS_OK == WlanStatus )
    {
        /* Clear the response */
        WlanClearResponseBuffer();
    }

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Start the WLAN Access Point
 *
 * \details Start the Access Point by the user provided values for
 *          access point name, password and authentication type
 *
 * \note    WEP and WPA authentication modes not supported.
 *
 * \param   pApName       - Pointer to access point name
 * \param   pApPassword   - Pointer to access point password
 * \param   AuthType     - Security mode
 *
 * \return  WLAN_STATUS - Status of starting the access point
 * \retval  WLAN_STATUS_OK             - Successful in starting the access point
 * \retval  WLAN_STATUS_INVALID_PARAM  - Invalid arguments
 * \retval  WLAN_STATUS_ERROR          - Error in starting the access point
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanStartAccessPoint(char *pApName, char *pApPassword, WLAN_AUTH AuthType)
{
    WLAN_STATUS WlanStatus;                      /* WLAN status */

    /* Initialize the variables */
    WlanStatus = WLAN_STATUS_ERROR;
    memset(CmdString, 0, sizeof(CmdString));

    do
    {
        /* Check the input parameters validity. WEP and WPA modes not supported */
        if ( (NULL == pApName) || (strlen(pApName) > WLAN_MAX_SSID_SIZE) ||
             (WLAN_AUTH_WEP == AuthType) || (WLAN_AUTH_WPA == AuthType) || (AuthType >= WLAN_AUTH_COUNT) )
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }

        if ( WLAN_AUTH_OPEN != AuthType )
        {
            if ( (NULL == pApPassword) ||
                 (strlen(pApPassword) < WLAN_MIN_PASSWORD_SIZE) ||
                 (strlen(pApPassword) > WLAN_MAX_PASSWORD_SIZE) )
            {
                WlanStatus = WLAN_STATUS_INVALID_PARAM;
                break;
            }
        }

        /* Store the configuration locally */
        WlanUpdateLocalApConfig(pApName, pApPassword, AuthType);

        /* If in Client mode, leave the Access point */
        if (WlanTotalConfig.IsInClientMode)
        {
            WlanStatus = L3_WlanLeaveAccessPoint();
            if ( WLAN_STATUS_OK != WlanStatus )
            {
                break;
            }
        }

        /* Create Access point network */
        WlanStatus = WlanCreateApNetwork(pApName, pApPassword, AuthType);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        OSTimeDly(WLAN_CREATE_AP_DLY);
        /* Reboot the module in AP mode */
        WlanStatus = L3_WlanReboot();
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* No error here */
        WlanStatus = WLAN_STATUS_OK;

        /* Update the mode as AP */
        WlanTotalConfig.IsInApMode = true;
        strcpy(WlanTotalConfig.LocalIpAddress, WLAN_DEFAULT_IP);

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Stop the WLAN Access Point
 *
 * \details Stop the already created Access Point
 *
 * \note    The module will switch to non-AP mode
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - Status of stoping the access point
 * \retval  WLAN_STATUS_OK     - Successful in stoping the access point
 * \retval  WLAN_STATUS_ERROR  - Error in stoping the access point
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanStopAccessPoint(void)
{
    WLAN_STATUS WlanStatus;                      /* WLAN status */

    /* Initialize the variables */
    WlanStatus = WLAN_STATUS_ERROR;
    memset(CmdString, 0, sizeof(CmdString));

    do
    {
        /* Do not try to associate with a network automatically. */
        sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_AP_CREATE].CmdString, WLAN_DISABLE_AUTO_JOIN);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_AP_CREATE, CMD_FIRST);

        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }
        /* Turns DHCP on. The module attempts to obtain an IP address and gateway from the access point.*/
        sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_DHCP_SERVER].CmdString, WLAN_DHCP_ENABLE);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_DHCP_SERVER, CMD_CONT);

        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* TCP server and client (default). */
        sprintf(CmdString, "%s %d\r", CmdRespTable[WLAN_CMD_AP_PROTOCOL].CmdString, WLAN_SET_IP_TCP_SERVER_CLIENT);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_AP_PROTOCOL, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Set the configurable Device ID */
        sprintf(CmdString, "%s %s\r", CmdRespTable[WLAN_CMD_OPT_DEV_ID].CmdString, pWlanApConfig->ApName);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_OPT_DEV_ID, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Set an Invalid IP address */
        sprintf(CmdString, "%s %s\r", CmdRespTable[WLAN_CMD_AP_IP_ADDR].CmdString, WLAN_INVALID_IP);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_AP_IP_ADDR, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }
        /* Store the settings */
        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_SAVE].CmdString, WLAN_CMD_SAVE, CMD_LAST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        OSTimeDly(WLAN_SAVE_CMD_DLY);
        /* Reboot the module in non-AP mode */
        WlanStatus = L3_WlanReboot();
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* No Error here */
        WlanStatus = WLAN_STATUS_OK;

        /* Update the mode as non-AP */
        WlanTotalConfig.IsInApMode = false;
        strcpy(WlanTotalConfig.LocalIpAddress, WLAN_INVALID_IP);

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Join an access point
 *
 * \details When in Client mode, this function can be called to Join an access point.
 *
 * \param   pAccessPoint  - pointer to Access point name
 * \param   pPassword     - pointer to Password to connect to the access point
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in join the access point
 * \retval  WLAN_STATUS_INVALID_PARAM   - Invalid parameters
 * \retval  WLAN_STATUS_ERROR           - Error in join the access point
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanJoinAccessPoint(char *pAccessPoint, char *pPassword)
{
    WLAN_STATUS    WlanStatus;                          /* WLAN status */
    bool           IsDhcpReceived;                      /* DHCP received status */
    bool           IsIPReceived;                        /* IP received status */

    IsDhcpReceived = false;
    IsIPReceived = false;
    WlanStatus = WLAN_STATUS_ERROR;
    memset(CmdString, 0, sizeof(CmdString));

    /* Store the details locally */
    strcpy(pWlanClientConfig->AccessPointToJoin, pAccessPoint);
    strcpy(pWlanClientConfig->ApJoinPassword, pPassword);

    do
    {
        if ( NULL == pAccessPoint )
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }

        if ( (strlen(pAccessPoint) > WLAN_MAX_SSID_SIZE) ||
            (strlen(pPassword) > WLAN_MAX_PASSWORD_SIZE) )
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }

        /* Stop the Access point before joining other access point */
        WlanStatus = L3_WlanStopAccessPoint();
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Leave the access point */
        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_LEAVE_AP].CmdString, WLAN_CMD_LEAVE_AP, CMD_FIRST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }
        OSTimeDly(WLAN_LEAVE_AP_DLY);

        /* Set the password if its valid */
        if ( NULL != pPassword )
        {
            sprintf(CmdString, "%s %s\r", CmdRespTable[WLAN_CMD_JOIN_PHRASE].CmdString, pPassword);
            WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_JOIN_PHRASE, CMD_CONT);
            if ( WLAN_STATUS_OK != WlanStatus )
            {
                break;
            }
        }

        /* Join the access point */
        sprintf(CmdString, "%s %s\r", CmdRespTable[WLAN_CMD_AP_JOIN].CmdString, pAccessPoint);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_AP_JOIN, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        OSTimeDly(WLAN_JOIN_AP_DLY);

        IsDhcpReceived = WlanCheckForDhcp();
        if ( !IsDhcpReceived )
        {
            WlanStatus = WLAN_STATUS_ERROR;
            break;
        }

        /* Wait for settling ip */
        OSTimeDly(WLAN_IP_SETTLE_DLY);

        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_GET_IP].CmdString, WLAN_CMD_GET_IP, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        IsIPReceived = WlanCheckIPFromDhcp();
        if ( !IsIPReceived )
        {
            WlanStatus = WLAN_STATUS_ERROR;
            break;
        }
        /* Exit Command mode */
        WlanStatus = WlanExitCommandMode();
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        WlanStatus = WLAN_STATUS_OK;
        WlanTotalConfig.IsInClientMode = true;
        pWlanClientConfig->IsClientConfigured = true;

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Leave an access point
 *
 * \details When in Client mode, this function can be called to leave an access point.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in leaving the access point
 * \retval  WLAN_STATUS_ERROR           - Error in leaving the access point
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanLeaveAccessPoint(void)
{
    WLAN_STATUS WlanStatus;                             /* Wlan Status */

    WlanStatus = WLAN_STATUS_ERROR;

    do
    {
        /* Leave Access Point */
        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_LEAVE_AP].CmdString, WLAN_CMD_LEAVE_AP, CMD_FIRST_LAST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* If remote host is connected disconnect that */
        if(pWlanClientConfig->IsRemoteHostConnected)
        {
            WlanStatus = L3_WlanDisconnect();
            if ( WLAN_STATUS_OK != WlanStatus )
            {
                break;
            }
        }

        WlanStatus = WLAN_STATUS_OK;

        WlanTotalConfig.IsInClientMode = false;
        strcpy(WlanTotalConfig.LocalIpAddress, WLAN_INVALID_IP);

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Connect to a remote host
 *
 * \details When in Client mode, this function can be called to connect to a remote host
 *
 * \param   pAddress  - pointer to IP address of the remote host
 * \param   pPort     - pointer to Port id of the application running on remote host
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in connecting remote host
 * \retval  WLAN_STATUS_INVALID_PARAM   - Invalid parameters
 * \retval  WLAN_STATUS_ERROR           - Error in connecting remote host
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanConnect(char *pAddress, uint8_t *pPort)
{
    WLAN_STATUS    WlanStatus;                     /* WLAN status */

    WlanStatus = WLAN_STATUS_ERROR;
    memset(CmdString, 0, sizeof(CmdString));

    do
    {
        if ( (NULL == pAddress) || (NULL == pPort) )
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }
        if ( (strlen(pAddress) > WLAN_IP_ADDR_SIZE) )
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }

        sprintf(CmdString, "%s %s\r", CmdRespTable[WLAN_CMD_REMOTE_HOST].CmdString, pAddress);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_REMOTE_HOST, CMD_FIRST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        sprintf(CmdString, "%s %s\r", CmdRespTable[WLAN_CMD_REMOTE_PORT].CmdString, pPort);
        WlanStatus = WlanProcessCmdResp(CmdString, WLAN_CMD_REMOTE_PORT, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Disable TCP connection timer */
        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_TCP_IDLE_TIMER].CmdString, WLAN_CMD_TCP_IDLE_TIMER, CMD_CONT);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        /* Opens a TCP connection */
        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_OPEN].CmdString, WLAN_CMD_OPEN, CMD_LAST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            break;
        }

        WlanStatus = WLAN_STATUS_OK;

        /* Update the connection status */
        pWlanClientConfig->IsRemoteHostConnected = true;

        /* Store the remote host's IP */
        strcpy(pWlanClientConfig->RemoteAddress, pAddress);

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Connect to a remote host
 *
 * \details This function can be called to disconnect from a remote host.
 *
 * \param   < None >
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in disconnecting remote host
 * \retval  WLAN_STATUS_INVALID_PARAM   - Invalid parameters
 * \retval  WLAN_STATUS_ERROR           - Error in disconnecting remote host
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanDisconnect(void)
{
    WLAN_STATUS    WlanStatus;                  /* WLAN status */

    WlanStatus = WLAN_STATUS_OK;
    memset(CmdString, 0, sizeof(CmdString));

    do
    {
        /* Already disconnected */
        if ( !pWlanClientConfig->IsRemoteHostConnected )
        {
            break;
        }

        /* Close TCP connection. */
        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_CLOSE].CmdString, WLAN_CMD_CLOSE, CMD_FIRST_LAST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            WlanStatus = WLAN_STATUS_ERROR;
            break;
        }

        /* Update the connection status */
        pWlanClientConfig->IsRemoteHostConnected = false;
        strcpy(pWlanClientConfig->RemoteAddress, WLAN_INVALID_IP);

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Local host's IP address
 *
 * \details Retrieve the local host's IP address stored in the WLAN local configuration
 *
 * \param   pAddress  - Pointer to hold the IP address.
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in retrieving the IP address
 * \retval  WLAN_STATUS_INVALID_PARAM   - Invalid parameter
 * \retval  WLAN_STATUS_ERROR           - Error in retrieving the IP address
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanGetLocalAddr(char *pAddress)
{
    WLAN_STATUS WlanStatus;          /* Wlan Status */

    WlanStatus = WLAN_STATUS_ERROR;

    do
    {
        if ( NULL ==  pAddress)
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }
        /* Copy the IP from the stored configuration */
        strcpy(pAddress, WlanTotalConfig.LocalIpAddress);

        WlanStatus = WLAN_STATUS_OK;

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Remote host's IP address
 *
 * \details Retrieve the remote host's IP address stored in the WLAN local configuration
 *
 * \param   pAddress  - Pointer to hold the IP address.
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in retrieving the IP address
 * \retval  WLAN_STATUS_INVALID_PARAM   - Invalid parameter
 * \retval  WLAN_STATUS_ERROR           - Error in retrieving the IP address
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanGetRemoteAddr(char *pAddress)
{
    WLAN_STATUS WlanStatus;         /* Wlan Status */

    WlanStatus = WLAN_STATUS_ERROR;

    do
    {
        if ( NULL ==  pAddress)
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }
        /* Copy the IP from the stored configuration */
        strcpy(pAddress, pWlanClientConfig->RemoteAddress);

        WlanStatus = WLAN_STATUS_OK;

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Read the firmware version.
 *
 * \details Retrieve the WiFi module's firmware version.
 *
 * \param   pVersion  - Pointer to hold the firmware version.
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in retrieving the firmware version
 * \retval  WLAN_STATUS_INVALID_PARAM   - Invalid parameter
 * \retval  WLAN_STATUS_ERROR           - Error in retrieving the firmware version
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanGetFirmwareVersion(char *pVersion)
{
    uint16_t    DataRecvdCount;                    /* Command response received count */
    char        *pVersionToken;                    /* Pointer to the version token string */ 
    WLAN_STATUS WlanStatus;                        /* WLAN status */

    /* Initializing the variables */
    DataRecvdCount = 0;
    WlanStatus = WLAN_STATUS_ERROR;
    memset(WlanRecvdData, 0, sizeof(WlanRecvdData)); 
    WlanRecvdData[WLAN_MAX_BUFF_SIZE - 1] = '\0';

    do
    {
        if ( NULL == pVersion )
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }

        WlanStatus = WlanProcessCmdResp(CmdRespTable[WLAN_CMD_FW_VERSION].CmdString, WLAN_CMD_FW_VERSION, CMD_FIRST);
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            Log(DBG, "L3_WlanGetFirmwareVersion: Send command error");
            break;
        }

        DataRecvdCount = WlanRecvCmdResponse(WlanRecvdData, sizeof(WlanRecvdData) - 1);

        if (DataRecvdCount > 0)
        {
            /* The response string comes with a 'Ver: x.xx' for the version information */
            pVersionToken = strstr(WlanRecvdData, "Ver:");
            if( pVersionToken )
            {
               strncpy(pVersion, pVersionToken+WLAN_VER_STRING_READ_OFFSET, WLAN_FIRMWARE_VERSION_LEN);
            }
        }
        else
        {
            Log(DBG, "L3_WlanGetFirmwareVersion: No data received");
            break;
        }

        WlanStatus = WlanExitCommandMode();
        if ( WLAN_STATUS_OK != WlanStatus )
        {
            Log(DBG, "L3_WlanGetFirmwareVersion:Error in exiting command mode");
            break;
        }

        WlanStatus = WLAN_STATUS_OK;

    } while ( false );
    
    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Read the Mac address.
 *
 * \details Read the WiFi module's mac address.
 *
 * \param   pMacAddress  - Pointer to hold the Mac address.
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in retrieving the Mac address.
 * \retval  WLAN_STATUS_INVALID_PARAM   - Invalid parameter
 * \retval  WLAN_STATUS_ERROR           - Error in retrieving the Mac address.
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanGetMacAddr(char *pMacAddress)
{
    WLAN_STATUS WlanStatus;         /* Wlan Status */

    WlanStatus = WLAN_STATUS_ERROR;

    do
    {
        if ( NULL == pMacAddress)
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }

        /* Copy the Mac address from the configuration */
        strcpy(pMacAddress, WlanTotalConfig.Macaddress);

        WlanStatus = WLAN_STATUS_OK;

    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Receive data from the remote host.
 *
 * \details Receive data from the remote host when the WiFi module is in Client mode.
 *
 * \param   pData     - Pointer to hold the received data. Should be at least 1 byte bigger than value pointed to by pCount (upon calling)
 * \param   pCount    - Pointer to Data bytes count to receive. This will get updated with actual bytes received.
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in receiving the data.
 * \retval  WLAN_STATUS_INVALID_PARAM   - Invalid parameters
 * \retval  WLAN_STATUS_ERROR           - Error in receiving the data.
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanReceive(uint8_t *pData, uint16_t *pCount)
{
    uint16_t     UartRecvdCount;          /* UART Rx count */
    uint16_t     RecvdResponseCount;      /* Response count */
    uint32_t     UartReadTimeout;         /* Max time wait for read */
    UART_STATUS  UartStatus;              /* UART status */
    WLAN_STATUS  WlanStatus;              /* Wlan Status */
    uint16_t     UartReadBytes;          /* UART Rx count */

    /* Initialize the variables */
    UartRecvdCount = 0;
    RecvdResponseCount = 0;
    UartStatus = UART_STATUS_INVALID_UART;
    WlanStatus = WLAN_STATUS_OK;
    UartReadBytes = 0;

    do
    {
        /* Check the arguments validity */
        if ( (NULL == pData) || (NULL == pCount) )
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }
        if ( *pCount > (WLAN_MAX_BUFF_SIZE - 1) )
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }

        /* Max time wait for the Uart read to complete */
        UartReadTimeout = OSTimeGet() + UART_TIMEOUT_MSEC;
        RecvdResponseCount = 0;

        do
        {
            UartReadBytes = *pCount;

            /*   the number of bytes read from the UART is available in UartRecvdCount */
            UartStatus = L2_UartReadBlock(UART5, pData + RecvdResponseCount, UartReadBytes, &UartRecvdCount);
            RecvdResponseCount += UartRecvdCount;
            UartReadBytes -= UartRecvdCount;

            /* Timeout check Retaining so we dont endup in a loop in special unforeseen cases*/
            if ( OSTimeGet() > UartReadTimeout )
            {
                /*
                Log(DBG,"WlanRecv: L2_UartReadBlock timedout %ld", (OSTimeGet() - UartReadTimeout));
                */
                break;
            }
        } while ( UartRecvdCount ); /* loop to check if we have more data to read */

        if ( UART_STATUS_OK != UartStatus )
        {
            if ( UART_STATUS_RX_BUFFER_EMPTY == UartStatus )
            {
                break;
            }
            else
            {
                WlanStatus = WLAN_STATUS_ERROR;
                break;
            }
        }
    } while ( false );

	/* when no data to receive delay for next packet arrival */ 
    if(RecvdResponseCount == 0x0)
    {
        OSTimeDly(WLAN_UART_PKT_DLY);
    }

    /* set the response params */
    pData[RecvdResponseCount] = '\0';
    *pCount = RecvdResponseCount;
    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Send data to the remote host.
 *
 * \details Send data to the remote host when the WiFi module is in Client mode.
 *
 * \param   pData     - Pointer to the data to send.
 * \param   pCount    - Data bytes to send.This will get updated with actual bytes sent.
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in sending the data.
 * \retval  WLAN_STATUS_INVALID_PARAM   - Invalid parameters
 * \retval  WLAN_STATUS_ERROR           - Error in sending the data.
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanSend(uint8_t *pData, uint16_t *pCount)
{
    uint16_t    UartSentCount;       /* UART sent data count */
    uint16_t    UartTotalSentCount;  /* Uart total sent data count */
    UART_STATUS UartStatus;          /* UART status */
    WLAN_STATUS WlanStatus;          /* WLAN status */
    uint32_t   UartWriteTimeout;     /* Max time to wait for uart write */
    uint16_t    UartSentBytes;       /* UART sent data count */

    /* Initialize the variables */
    UartSentCount = 0;
    UartStatus = UART_STATUS_INVALID_UART;
    WlanStatus = WLAN_STATUS_ERROR;

    do
    {
        /* Check the arguments validity */
        if ( (NULL ==  pData) || (NULL == pCount) )
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }
        if ( *pCount > WLAN_MAX_BUFF_SIZE - 1 )
        {
            WlanStatus = WLAN_STATUS_INVALID_PARAM;
            break;
        }

        /* Max time wait for the Uart write to complete */
        UartWriteTimeout = OSTimeGet() + UART_TIMEOUT_MSEC;
        UartSentCount = 0;

        UartSentBytes = *pCount;
        UartTotalSentCount = 0;
        do
        {
            /* Send the data via UART5 */
            UartStatus = L2_UartWriteBlock(UART5, pData + UartTotalSentCount, UartSentBytes, &UartSentCount);

            UartSentBytes -= UartSentCount;
            UartTotalSentCount += UartSentCount;

            /* Timeout check */
            if ( OSTimeGet() > UartWriteTimeout )
            {
                /*
                Log(DBG,"WlanSend: L2_UartWriteBlock timedout %ld", (OSTimeGet() - UartWriteTimeout));  
                */
                break;
            }
        } while ( UartSentBytes ); /* continue until we have bytes to send */

        if ( UART_STATUS_OK != UartStatus )
        {
            break;
        }

        *pCount = UartTotalSentCount;

        /* No error here */
        WlanStatus = WLAN_STATUS_OK;
    } while ( false );

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Connection status
 *
 * \details The WiFi module connection status to the remote host.
 *
 * \param   < None >
 *
 * \return  bool - Status of the connection.
 *
 * ========================================================================== */
bool L3_WlanConnectStatus(void)
{
    return pWlanClientConfig->IsRemoteHostConnected;
}

/* ========================================================================== */
/**
 * \brief   Set the remote host Connection status
 *
 * \details Sets the WiFi module connection status to the remote host.
 *
 * \param    Status - Connection status (True- Connected False-Disconnected)
 *
 * \return  None
 *
 * ========================================================================== */
void  L3_WlanSetConnectStatus(bool Status)
{
    pWlanClientConfig->IsRemoteHostConnected = Status;
}

/* ========================================================================== */
/**
 * \brief   WLAN Network mode
 *
 * \details To switch the WLAN in to AP mpde or Client mode.
 *
 * \param   Mode  - The network mode to set.
 *
 * \return  WLAN_STATUS - Status of the Network mode set
 * \retval  WLAN_STATUS_OK      - Successful in setting the network mode
 * \retval  WLAN_STATUS_ERROR   - Error in setting the network mode
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanNetworkModeSet(WLAN_MODE Mode)
{   
    WLAN_STATUS WlanStatus;       /* Wlan Status */

    WlanStatus = WLAN_STATUS_ERROR;

    switch(Mode)
    {
        case WLAN_MODE_AP:
            WlanStatus = WlanSwitchToApMode();
            break;
        case WLAN_MODE_CLIENT:
            WlanStatus = WlanSwitchToClientMode();
            break;
        default:
            Log(DBG, "L3_WlanNetworkModeSet:Mode not supported");
            break;
    }

    return WlanStatus;
}

/* ========================================================================== */
/**
 * \brief   Function to register Wlan interface callback
 *
 * \details Function to register Wlan interface callback
 *
 * \param   pHandler - Callback handle
 *
 * \return  WLAN_STATUS - Status of the function
 * \retval  WLAN_STATUS_OK              - Successful in receiving the data.
 * \retval  WLAN_STATUS_INVALID_PARAM   - Invalid parameters
 * \retval  WLAN_STATUS_ERROR           - Error in receiving the data.
 *
 * ========================================================================== */
WLAN_STATUS L3_WlanRegisterCallback(WLAN_EVENT_HNDLR pHandler)
{
    WlanTotalConfig.pHandler = pHandler;
    return WLAN_STATUS_OK;
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
 }
#endif

