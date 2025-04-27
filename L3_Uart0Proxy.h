#ifndef  L3_UART0PROXY_H
#define  L3_UART0PROXY_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \brief   Public interface for Communication Manager module.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_Uart0Proxy.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         /* Import common definitions such as types, etc. */
#include "L2_Uart.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define ADAPTER_BAUD_RATE       (129032u)             /*! Adapter baud rate */
#define ADAPTER_UART            (UART0)               /*! UART used for Adapter communication */

#define UART0_STATUS_OK         UART_STATUS_OK        /*! No error */

#define L3_Uart0Init()                   L2_UartInit(UART0, ADAPTER_BAUD_RATE)
#define L3_Uart0Flush(UartNum)           L2_UartFlush(UartNum)
#define L3_Uart0Receive(pData, pCount)   L2_UartReadBlock(UART0, pData, *pCount, pCount)
#define L3_Uart0Send(pData, pCount)      L2_UartWriteBlock(UART0, pData, *pCount, pCount)
  
/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_UART0PROXY_H */

