#ifndef L3_USB_H
#define L3_USB_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_USB
 * \{
 * \brief   Public interface for USB control routines
 *
 * \details This file has all USB related symbolic constants and function prototypes
 *          This implementation uses Layer 2 USB interface
 *
 * \copyright  Copyright 2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         /* Import common definitions such as types, etc. */
#include "L2_Usb.h"         /* Import all L2 USB declarations */

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern USB_STATUS L3_UsbInit(USB_EVENT_HNDLR pHandler);
extern USB_STATUS L3_UsbSend(uint8_t *pDataOut, uint16_t DataCount, uint16_t Timeout, uint16_t *pSentCount);
extern USB_STATUS L3_UsbReceive(uint8_t *pDataIn,  uint16_t DataCount, uint16_t Timeout, uint16_t *pReceivedCount);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_USB_H */
