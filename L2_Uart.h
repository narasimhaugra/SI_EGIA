#ifndef L2_UART_H
#define L2_UART_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Uart
 * \{
 * \brief   Public interface for UART control routines
 *
 * \details This module defines symbolic constants as well as function prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_Uart.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         ///< Import common definitions such as types, etc

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
// Error flag bits returned from L2_UartGetError()

#define UART_PF_MASK    UART_S1_PF_MASK     ///< Parity Error
#define UART_FE_MASK    UART_S1_FE_MASK     ///< Framing Error
#define UART_NF_MASK    UART_S1_NF_MASK     ///< Noise Flag
#define UART_OR_MASK    UART_S1_OR_MASK     ///< Overrun Error

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/// Available UART list
typedef enum                       
{
    UART0,                          ///< UART 0
    UART4,                          ///< UART 4
    UART5,                          ///< UART 5
    UART_COUNT                      ///< Number of supported UART channels
} UART_CHANNEL;

/// Initialization and IO status
typedef enum                        
{
    UART_STATUS_OK,                 ///< No error
    UART_STATUS_BAUD_ERR,           ///< Invalid baud rate for specified clock
    UART_STATUS_INVALID_UART,       ///< Unsupported UART channel
    UART_STATUS_INVALID_PTR,        ///< Invalid pointer
    UART_STATUS_RX_BUFFER_EMPTY,    ///< No data in Rx Buffer
    UART_STATUS_TX_BUSY             ///< Tx busy
} UART_STATUS;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern UART_STATUS L2_UartInit(UART_CHANNEL Chan, uint32_t Baud);
extern UART_STATUS L2_UartFlush(UART_CHANNEL Chan);
extern UART_STATUS L2_UartReadBlock(UART_CHANNEL Chan, uint8_t *pDataIn, uint16_t MaxDataCount, uint16_t *pBytesRcd);
extern UART_STATUS L2_UartWriteBlock(UART_CHANNEL Chan, uint8_t *pDataOut, uint16_t MaxDataCount, uint16_t *pBytesQueued);
extern uint16_t L2_UartGetTxByteCount(UART_CHANNEL Chan);
extern uint16_t L2_UartGetRxByteCount(UART_CHANNEL Chan);
extern uint8_t  L2_UartGetError(UART_CHANNEL Chan);

extern void L2_UartLoopbackEnable(UART_CHANNEL Chan);
extern void L2_UartLoopbackDisable(UART_CHANNEL Chan);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif  /* L2_UART_H */
