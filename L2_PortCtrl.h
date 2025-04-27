#ifndef L2_PORTCTRL_H
#define L2_PORTCTRL_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif
    
/* ========================================================================== */
/**
 * \addtogroup L2_PortCtrl 
 * \{ 
 * \brief Public interface for GPIO control routines
 *
 * \details This module defines symbolic constants as well as function prototypes
 *
 * \copyright  Copyright 2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * \file  L2_PortCtrl.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "L2_Gpio.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
#define ALT_0_ANALOG   (PORT_PCR_MUX(0)) ///< K20 Signal Mux Type 0 - Analog Signals
#define ALT_1_GPIO     (PORT_PCR_MUX(1)) ///< K20 Signal Mux Type 1 - GPIO Signals
#define ALT_2_SPI_I2C  (PORT_PCR_MUX(2)) ///< K20 Signal Mux Type 2 - SPI/I2C Signals
#define ALT_3_UART     (PORT_PCR_MUX(3)) ///< K20 Signal Mux Type 3 - UART Signals
#define ALT_3_SPI      (PORT_PCR_MUX(3)) ///< K20 Signal Mux Type 3 - SPI, Special case for SPI0_CS5n 
#define ALT_4_SDHC     (PORT_PCR_MUX(4)) ///< K20 Signal Mux Type 4 - SDHC Signals
#define ALT_5_FLEXBUS  (PORT_PCR_MUX(5)) ///< K20 Signal Mux Type 5 - FlexBus 1 Signals
#define ALT_6_FLEXBUS  (PORT_PCR_MUX(6)) ///< K20 Signal Mux Type 6 - FlexBus 2 Signals
    
/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void L2_PortCtrlInit(void);
extern void L2_PortCtrlConfigPin(PORT_MemMapPtr pPCR, GPIO_PIN ePin, uint32_t u32PCRConfig);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L2_PORTCTRL_H */
