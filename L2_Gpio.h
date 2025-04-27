#ifndef L2_GPIO_H
#define L2_GPIO_H

#ifdef __cplusplus  /* Header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Gpio
 * \{
 * \brief Public interface for GPIO control routines
 *
 * \details This module defines symbolic constants as well as function prototypes
 *
 * \copyright  Copyright 2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * \file  L2_Gpio.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         ///< Import common definitions such as types, etc.

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

#define GPIO_MASK_PIN(xPin)  (1<<(xPin))  ///< Pin Masks

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum                     ///  GPIO Status
{
    GPIO_STATUS_OK,              ///< No Error.
    GPIO_STATUS_INVALID_INPUT,   ///< Invalid Input
    GPIO_STATUS_NOT_CONFIGURED,  ///< Not Configured
    GPIO_STATUS_NOT_INIT,        ///< Not Initialized
    GPIO_STATUS_ERROR            ///< Error
} GPIO_STATUS;

/* From Micro datasheet, section: PORTx_PCRn field descriptions */
/*   Port Control Interrupt Configuration (IRQC) - bits 19–16
 *   The pin interrupt configuration is valid in all digital pin
 *   muxing modes. The corresponding pin is configured to generate
 *   interrupt / DMA Request as follows:
 *   0000 Interrupt/DMA Request disabled.
 *   0001 DMA Request on rising edge.
 *   0010 DMA Request on falling edge.
 *   0011 DMA Request on either edge.
 *   0100 Reserved.
 *   1000 Interrupt when logic zero.
 *   1001 Interrupt on rising edge.
 *   1010 Interrupt on falling edge.
 *   1011 Interrupt on either edge.
 *   1100 Interrupt when logic one.
 *   Others Reserved. */
typedef enum                                /// GPIO uP Interrupt Options
{
    GPIO_uP_INT_DISABLED = 0x00,            ///< Flag is disabled.
    GPIO_uP_INT_DMA_RISING_EDGE = 0x01,     ///< Flag and DMA request on rising edge.
    GPIO_uP_INT_DMA_FALLING_EDGE = 0x02,    ///< Flag and DMA request on falling edge.
    GPIO_uP_INT_DMA_BOTH_EDGE = 0x03,       ///< Flag and DMA request on either edge.
    GPIO_uP_INT_LOGIC_LOW = 0x08,           ///< Flag and Interrupt when logic 0.
    GPIO_uP_INT_RISING_EDGE = 0x09,         ///< Flag and Interrupt on rising-edge.
    GPIO_uP_INT_FALLING_EDGE = 0x0A,        ///< Flag and Interrupt on falling-edge.
    GPIO_uP_INT_BOTH_EDGE = 0x0B,           ///< Flag and Interrupt on either edge.
    GPIO_uP_INT_LOGIC_HIGH = 0x0C,          ///< Flag and Interrupt when logic 1.
    GPIO_uP_INT_COUNT = 0xFF                ///< Last Count.
} GPIO_uP_INT_TYPE;

typedef enum                 /// GPIO Direction Enum
{
    GPIO_DIR_INPUT,          ///< GPIO Direction Input
    GPIO_DIR_OUTPUT,         ///< GPIO Direction Output
    GPIO_DIR_LAST            ///< GPIO Direction Last
} GPIO_DIR;

typedef enum                    /// GPIO uP Port Enum
{
    GPIO_uP_PORT_A,             ///< GPIO uP Port A
    GPIO_uP_PORT_B,             ///< GPIO uP Port B
    GPIO_uP_PORT_C,             ///< GPIO uP Port B
    GPIO_uP_PORT_D,             ///< GPIO uP Port D
    GPIO_uP_PORT_E,             ///< GPIO uP Port E
    GPIO_uP_PORT_F,             ///< GPIO uP Port F
    GPIO_uP_PORT_LAST           ///< GPIO uP Port Last
} GPIO_uP_PORT;

typedef enum                    /// GPIO Pin Enum
{
    GPIO_PIN_00,		        ///< GPIO Pin 00
    GPIO_PIN_01,		        ///< GPIO Pin 01
    GPIO_PIN_02,		        ///< GPIO Pin 02
    GPIO_PIN_03,		        ///< GPIO Pin 03
    GPIO_PIN_04,		        ///< GPIO Pin 04
    GPIO_PIN_05,		        ///< GPIO Pin 05
    GPIO_PIN_06,		        ///< GPIO Pin 06
    GPIO_PIN_07,		        ///< GPIO Pin 07
    GPIO_PIN_08,		        ///< GPIO Pin 08
    GPIO_PIN_09,		        ///< GPIO Pin 09
    GPIO_PIN_10,		        ///< GPIO Pin 10
    GPIO_PIN_11,		        ///< GPIO Pin 11
    GPIO_PIN_12,		        ///< GPIO Pin 12
    GPIO_PIN_13,		        ///< GPIO Pin 13
    GPIO_PIN_14,		        ///< GPIO Pin 14
    GPIO_PIN_15,		        ///< GPIO Pin 15
    GPIO_PIN_16,		        ///< GPIO Pin 16
    GPIO_PIN_17,		        ///< GPIO Pin 17
    GPIO_PIN_18,		        ///< GPIO Pin 18
    GPIO_PIN_19,		        ///< GPIO Pin 19
    GPIO_PIN_20,		        ///< GPIO Pin 20
    GPIO_PIN_21,		        ///< GPIO Pin 21
    GPIO_PIN_22,		        ///< GPIO Pin 22
    GPIO_PIN_23,		        ///< GPIO Pin 23
    GPIO_PIN_24,		        ///< GPIO Pin 24
    GPIO_PIN_25,		        ///< GPIO Pin 25
    GPIO_PIN_26,		        ///< GPIO Pin 26
    GPIO_PIN_27,		        ///< GPIO Pin 27
    GPIO_PIN_28,		        ///< GPIO Pin 28
    GPIO_PIN_29,		        ///< GPIO Pin 29
    GPIO_PIN_30,		        ///< GPIO Pin 30
    GPIO_PIN_31,		        ///< GPIO Pin 31
    GPIO_PIN_LAST               ///< GPIO Pin Last
} GPIO_PIN;

typedef void (*GPIO_uP_INT_CALLBACK_T)(void);       /// GPIO Interrupt Call Back

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/
typedef struct              /// GPIO uP Pin Interrupt Config.
{
    GPIO_uP_INT_TYPE eInterruptType;                ///< GPIO uP Interrupt Levels
    GPIO_uP_INT_CALLBACK_T pxInterruptCallback;     ///< GPIO uP Interrupt Call Back
} GPIO_uP_PIN_INT_CONFIG_T;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern GPIO_STATUS L2_GpioConfigPin(GPIO_uP_PORT ePort,
                                    GPIO_PIN ePin,
                                    GPIO_DIR ePinDirection,
                                    GPIO_uP_PIN_INT_CONFIG_T * pxIntConfig);

extern GPIO_STATUS L2_GpioSetPin(GPIO_uP_PORT ePort, GPIO_PIN ePin);
extern GPIO_STATUS L2_GpioClearPin(GPIO_uP_PORT ePort, GPIO_PIN ePin);
extern GPIO_STATUS L2_GpioTogglePin(GPIO_uP_PORT ePort, GPIO_PIN ePin);

extern GPIO_STATUS L2_GpioGetPin(GPIO_uP_PORT ePort, GPIO_PIN ePin, volatile bool *pbGetValue);

extern GPIO_uP_INT_CALLBACK_T L2_GpioGetPinConfig(GPIO_uP_PORT ePort, GPIO_PIN ePin);


/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* Header compatible with C++ project */
}
#endif

#endif /* L2_GPIO_H */

/* End of file */
