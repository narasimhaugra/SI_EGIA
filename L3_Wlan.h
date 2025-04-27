#ifndef L3_WLAN_H
#define L3_WLAN_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_Wlan
 * \{
 *
 * \brief   Public interface for WLAN
 *
 * \details Symbolic types and constants are included with the interface prototypes.
 *         
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_Wlan.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define WLAN_INVALID_IP             ("0.0.0.0")        ///< Invalid IP address
#define WLAN_IP_ADDR_SIZE           (16u)              ///< IP Address string size
#define WLAN_MAC_ADDR_SIZE          (18u)              ///< Mac Address string size

/* The RN171 module can support a maximum SSID length of 32 characters.
   The size 16 characters is added as per the design */
#define WLAN_MAX_SSID_SIZE          (16u)              ///< Maximum size of access point name
#define WLAN_MIN_PASSWORD_SIZE      (8u)               ///< Minimum size of password
#define WLAN_MAX_PASSWORD_SIZE      (64u)              ///< Maximum size of password
#define WLAN_AP_DEV_LIST_SIZE       (400u)             ///< Maximum size of the list of devices in AP
#define WLAN_FW_VER_SIZE            (5u)               ///< Maximum size of the firmware version string
#define WLAN_TX_BUFF_SIZE           (50u)              ///< WLAN Tx command buffer
#define WLAN_RX_BUFF_SIZE           (500u)             ///< Command response receive buffer size
#define WLAN_MAX_BUFF_SIZE          (1025u)             ///< WLAN maximum buffer size

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum                                // WLAN modes
{
  WLAN_MODE_AP,                             ///< Access Point mode
  WLAN_MODE_CLIENT,                         ///< Client Mode
  WLAN_MODE_ADHOC,                          ///< Ad-hoc Mode
  WLAN_MODE_COUNT                           ///< Number of WLAN modes
} WLAN_MODE;

typedef enum                                // WLAN Security modes
{
  WLAN_AUTH_OPEN,                           ///< Authentication mode Open
  WLAN_AUTH_WEP,                            ///< Authentication mode WEP-128
  WLAN_AUTH_WPA,                            ///< Authentication mode WPA-1
  WLAN_AUTH_WPA2,                           ///< Authentication mode WAP2-PSK
  WLAN_AUTH_COUNT                           ///< Number of Authentication modes
} WLAN_AUTH;

typedef enum                                // Status of WLAN
{
  WLAN_STATUS_OK,                           ///< No error
  WLAN_STATUS_INVALID_PARAM,                ///< Invalid Parameter
  WLAN_STATUS_NO_CONNECTION,                ///< No Connection
  WLAN_STATUS_FAILED,                       ///< Failed Status
  WLAN_STATUS_ERROR,                        ///< Error
  WLAN_STATUS_COUNT                         ///< Total number of status
} WLAN_STATUS;

typedef enum                                // Wi-Fi Module Power mode
{
  WLAN_POWER_ENABLED,                       ///< Enabling the Power mode
  WLAN_POWER_DISABLED,                      ///< Low power mode
  WLAN_POWER_SLEEP,                         ///< Module in Sleep mode
  WLAN_POWER_COUNT                          ///< Number of Power modes
} WLAN_POWER_MODE;

typedef enum                        /// List of possible WLAN Events.
{
    WLAN_EVENT_CONNECT,              ///< WLAN connect event.
    WLAN_EVENT_DISCCONNECT,          ///< WLAN disconnect event.
} WLAN_EVENT;

typedef void (*WLAN_EVENT_HNDLR)(WLAN_EVENT);     ///< Callback function to report events to the upper layers.

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern WLAN_STATUS L3_WlanInit(void);
extern WLAN_STATUS L3_WlanStartAccessPoint(char *pApName, char *pApPassword, WLAN_AUTH AuthType);
extern WLAN_STATUS L3_WlanStopAccessPoint(void);
extern WLAN_STATUS L3_WlanChannelSet(uint8_t Channel);
extern WLAN_STATUS L3_WlanListDevice(char *pDeviceList);
extern WLAN_STATUS L3_WlanGetSignalStrength(uint8_t *pRssiStrength);
extern WLAN_STATUS L3_WlanDhcpEnable(bool DhcpMode);
extern WLAN_STATUS L3_WlanPowerSet(WLAN_POWER_MODE PowerMode);
extern WLAN_STATUS L3_WlanReboot(void);
extern WLAN_STATUS L3_WlanJoinAccessPoint(char *pAccessPoint, char *pPassword);
extern WLAN_STATUS L3_WlanLeaveAccessPoint(void);
extern WLAN_STATUS L3_WlanConnect(char *pAddress, uint8_t *pPort);
extern WLAN_STATUS L3_WlanDisconnect(void);
extern bool L3_WlanConnectStatus(void);
extern WLAN_STATUS L3_WlanSend(uint8_t *pData, uint16_t *pCount);
extern WLAN_STATUS L3_WlanReceive(uint8_t *pData, uint16_t *pCount);
extern WLAN_STATUS L3_WlanGetLocalAddr(char *pAddress);
extern WLAN_STATUS L3_WlanGetRemoteAddr(char *pAddress);
extern WLAN_STATUS L3_WlanGetFirmwareVersion(char *pVersion);
extern WLAN_STATUS L3_WlanGetMacAddr(char *pMacAddress);
extern WLAN_STATUS L3_WlanNetworkModeSet(WLAN_MODE Mode);
extern  WLAN_STATUS L3_WlanGetConnStatus(char *pVersion);
extern void  L3_WlanSetConnectStatus(bool Status);
extern WLAN_STATUS L3_WlanRegisterCallback(WLAN_EVENT_HNDLR pHandler);
extern void L3_WlanCheckConnection(void);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_WLAN_H */

