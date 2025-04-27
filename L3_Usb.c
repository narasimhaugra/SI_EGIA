#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_USB
 * \{
 * \brief   Layer 3 USB Control Routines
 *
 * \details This file is a stub to L2_Usb functions
 *
 * \copyright 2020 Medtronic - Surgical Innovations. All Rights Reserved.
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_Usb.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define VENDOR_ID                           (0x1264u)            /*! Vendor  ID */
#define PROD_ID                             (0x0500u)            /*! Product ID (CDC) */
#define DEV_REL_NUM                         (0x0100u)            /*! Device release number */
#define MANUF_STR                           ("Covidien")         /*! Manufacturer string */
#define PROD_STR                            ("Covidien Gen2")    /*! Product string */
#define SERIAL_STR                          ("123")              /*! Serial number string */
#define MAX_POWER                           (100u)               /*! Maximum power */
#define USB_BAUD_RATE                       (9600u)              /*! Baud Rate */
#define STOP_BIT_0                          (0u)                 /*! Stop bits */
#define DATA_BIT_8                          (8u)                 /*! Data bits */
#define LINE_STATE_INTERVAL                 (100u)               /*! Line state interval */

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

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Function to to initializes and configures the USB port.
 *
 * \details This function configures the USB port with device configuraton and
 *          class configuration.
 *
 * \param   pHandler - Event handler
 *
 * \return  USB_STATUS - status
 *
 * ========================================================================== */
USB_STATUS L3_UsbInit(USB_EVENT_HNDLR pHandler)
{
    USB_DEV_CFG  UsbDevCfg =
    {
       VENDOR_ID,
       PROD_ID,
       DEV_REL_NUM,
       MANUF_STR,
       PROD_STR,
       SERIAL_STR,
       MAX_POWER,
       USB_SPEED_FULL,
       pHandler
    };

    USB_CLASS_CFG  UsbClassCfg =
    {
       USB_BAUD_RATE,
       STOP_BIT_0,
       DATA_BIT_8,
       LINE_STATE_INTERVAL
    };

    return L2_UsbInit(&UsbDevCfg, &UsbClassCfg);
}

/* ========================================================================== */
/**
 * \brief   Function to transmit data via USB port.
 *
 * \param   pDataOut  - Pointer to the data to be transmitted.
 * \param   DataCount - Size of the data buffer in bytes.
 * \param   Timeout   - Timeout value in milliseconds. If the Timeout value is 0 then
 *                      this becomes indefinite blocking call.
 * \param   pSentCount - Pointer to Variable which holds the value of number of bytes
 *                       actually transmitted..
 *
 * \note    \todo Based on the upper layers usage if required we can use a single argument
 *                instead of two arguments DataCount and pSentCount.
 *
 * \return  USB_STATUS - status
 *
 * ========================================================================== */
USB_STATUS L3_UsbSend(uint8_t *pDataOut, uint16_t DataCount, uint16_t Timeout, uint16_t *pSentCount)
{
    return L2_UsbSend(pDataOut, DataCount, Timeout, pSentCount);
}

/* ========================================================================== */
/**
 * \brief   Function to receive data via USB port.
 *
 * \param   pDataIn   - Pointer to the data buffer to store the received data.
 * \param   DataCount - Size of the data buffer to receive.
 * \param   Timeout   - Timeout value in milliseconds. If the Timeout value is 0 then
 *                      this becomes indefinite blocking call.
 * \param   pReceivedCount - Pointer to Variable which holds the value of number of bytes
 *                           actually received.
 *
 * \return  USB_STATUS - status
 *
 * ========================================================================== */
USB_STATUS L3_UsbReceive(uint8_t *pDataIn,  uint16_t DataCount, uint16_t Timeout, uint16_t *pReceivedCount)
{
    return L2_UsbReceive(pDataIn,  DataCount, Timeout, pReceivedCount);
}

/**
 *\}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
