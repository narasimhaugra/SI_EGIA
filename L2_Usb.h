#ifndef L2_USB_H
#define L2_USB_H

#ifdef __cplusplus      /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Usb
 * \{
 * \brief   Interface for USB.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 20120 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_Usb.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         // Import common definitions such as types, etc.

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
///\todo to replace this with a typedef for timestamp value which can be used systemwide.
typedef uint32_t TIMESTAMP;          ///< Timestamp in Unix UTC(32 bit seconds counter) format.

typedef enum                         ///< List of possible return status values.
{
    USB_STATUS_OK,
    USB_STATUS_FAIL,
    USB_STATUS_INVALID_PARAM,
    USB_STATUS_TIMEOUT,
    USB_STATUS_INVALID_STATE
} USB_STATUS;

typedef enum                        ///< List of possible speeds.
{
    USB_SPEED_INVALID,              ///< Invalid USB speed.
    USB_SPEED_LOW,                  ///< The USB Low-Speed signaling bit rate is 1.5 Mb/s.
    USB_SPEED_FULL,                 ///< The USB Full-Speed signaling bit rate is 12 Mb/s.
    USB_SPEED_COUNT
} USB_SPEED;

typedef enum                        /// List of possible USB Events.
{
    USB_EVENT_RESET,                ///< USB reset event.
    USB_EVENT_CONNECT,              ///< USB connect event.
    USB_EVENT_DISCCONNECT,          ///< USB disconnect event.
    USB_EVENT_SUSPEND,              ///< USB suspend event.
    USB_EVENT_RESUME                ///< USB resume event.
} USB_EVENT;

typedef void (*USB_EVENT_HNDLR)(USB_EVENT);     ///< Callback function to report events to the upper layers.

typedef  struct                     /// List of USB Device configuration elements.
{
    uint16_t   VendorID;            ///< Vendor  ID.
    uint16_t   ProductID;           ///< Product ID.
    uint16_t   DeviceRelNum;        ///< Device release number.
    char      *pManufacturerStr;    ///< Manufacturer  string.
    char      *pProductStr;         ///< Product string.
    char      *pSerialNbrStr;       ///< Serial number string.
    uint16_t   MaxPower;            ///< Bus power required for this device
    USB_SPEED  UsbSpeed;            ///< USB Speed.
    USB_EVENT_HNDLR pHandler;       ///< Configuration speed.
} USB_DEV_CFG;

typedef  struct                     /// List of USB class configuration elements.
{
    uint32_t    BaudRate;           ///< Baud rate.
    uint8_t     StopBits;           ///< Stop bit.
    uint8_t     DataBits;           ///< Data bits.
    uint16_t    LineStateInterval;  ///< Line state notification interval in milliseconds.
} USB_CLASS_CFG;

/******************************************************************************/
/*                             Global Variable(s)                             */
/******************************************************************************/
extern const char USBEvent[5][11];

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern USB_STATUS L2_UsbInit(USB_DEV_CFG *pUsbDevCfg, USB_CLASS_CFG *pUsbClassCfg);
extern USB_STATUS L2_UsbSend(uint8_t *pDataOut, uint16_t DataCount, uint16_t Timeout, uint16_t *pSentCount);
extern USB_STATUS L2_UsbReceive(uint8_t *pDataIn,  uint16_t DataCount, uint16_t Timeout, uint16_t *pReceivedCount);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L2_USB_H */
