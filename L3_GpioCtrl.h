#ifndef L3_GPIOCTRL_H
#define L3_GPIOCTRL_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_GpioCtrl
 * \{
 * \brief Public interface for GPIO control routines
 *
 * \details This module defines symbolic constants as well as function prototypes
 *
 * \copyright  Copyright 2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * \file  L3_GpioCtrl.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         ///< Import common definitions such as types, etc.
#include "L2_Gpio.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum  GPIO_SIGNALS       ///  GPIO Signals
{
    GPIO_MOT0_ADC_TRIG,          ///< 00, GPIO uP Signal INPUT,  PORT A, Pin 5,  MOT0_ADC_TRIG
    GPIO_1W_SHELL_EN,            ///< 01, GPIO uP Signal OUTPUT, PORT A, Pin 7,  1W_SHELL_EN
    GPIO_WIFI_FORCE_AWAKE,       ///< 02, GPIO uP Signal OUTPUT, PORT A, Pin 11, WIFI_FORCE_AWAKE
    GPIO_SLP_1Wn,                ///< 03, GPIO uP Signal OUTPUT, PORT A, Pin 12, SLP_1Wn
    GPIO_DUAL_ACCEL_INT,         ///< 04, GPIO uP Signal INPUT,  PORT A, Pin 13, DUAL_ACCEL_INT
    GPIO_EXTRA_IO_uC0,           ///< 05, GPIO uP Signal OUTPUT, PORT A, Pin 24, EXTRA_IO_uC0
    GPIO_1W_AD_EN,               ///< 06, GPIO uP Signal OUTPUT, PORT A, Pin 25, 1W_AD_EN
    GPIO_PERIPHERAL_WUn,         ///< 07, GPIO uP Signal INPUT,  PORT B, Pin 0,  PERIPHERAL_WUn
    GPIO_MOT1_ADC_TRIG,          ///< 08, GPIO uP Signal INPUT,  PORT B, Pin 1,  MOT1_ADC_TRIG
    GPIO_WIFI_ENn,               ///< 09, GPIO uP Signal OUTPUT, PORT B, Pin 4,  WIFI_ENn
    GPIO_WIFI_RESETn,            ///< 10, GPIO uP Signal OUTPUT, PORT B, Pin 5,  WIFI_RESETn
    GPIO_EN_BATT_15V,            ///< 11, GPIO uP Signal OUTPUT, PORT B, Pin 6,  EN_BATT_15V
    GPIO_GN_LED,                 ///< 12, GPIO uP Signal OUTPUT, PORT B, Pin 8,  GN_LED
    GPIO_GN_KEY1n,               ///< 13, GPIO uP Signal INPUT,  PORT B, Pin 9,  GN_KEY1n
    GPIO_GN_KEY2n,               ///< 14, GPIO uP Signal INPUT,  PORT B, Pin 10, GN_KEY2n
    GPIO_EXTRA_IO_uC3,           ///< 15, GPIO uP Signal INPUT,  PORT B, Pin 11, EXTRA_IO_uC3
    GPIO_KEY_WAKEn,              ///< 16, GPIO uP Signal INPUT,  PORT C, Pin 3,  KEY_WAKEn
    GPIO_OPEN_KEYn,              ///< 17, GPIO uP Signal INPUT,  PORT C, Pin 12, OPEN_KEYn
    GPIO_LEFT_CW_KEYn,           ///< 18, GPIO uP Signal INPUT,  PORT C, Pin 13, LEFT_CW_KEYn
    GPIO_LEFT_CCW_KEYn,          ///< 19, GPIO uP Signal INPUT,  PORT C, Pin 18, LEFT_CCW_KEYn
    GPIO_RIGHT_CW_KEYn,          ///< 20, GPIO uP Signal INPUT,  PORT C, Pin 19, RIGHT_CW_KEYn
    GPIO_GPIO_INTn,              ///< 21, GPIO uP Signal INPUT,  PORT D, Pin 7,  GPIO_INTn
    GPIO_IM_GOOD,                ///< 22, GPIO uP Signal OUTPUT, PORT D, Pin 8,  IM_GOOD
    GPIO_RIGHT_CCW_KEYn,         ///< 23, GPIO uP Signal INPUT,  PORT D, Pin 9,  RIGHT_CCW_KEYn
    GPIO_LEFT_ARTIC_KEYn,        ///< 24, GPIO uP Signal INPUT,  PORT D, Pin 11, LEFT_ARTIC_KEYn
    GPIO_RIGHT_ARTIC_KEYn,       ///< 25, GPIO uP Signal INPUT,  PORT D, Pin 12, RIGHT_ARTIC_KEYn
    GPIO_CLOSE_KEYn,             ///< 26, GPIO uP Signal INPUT,  PORT D, Pin 13, CLOSE_KEYn
    GPIO_EXTRA_IO_uC2,           ///< 27, GPIO uP Signal INPUT,  PORT E, Pin 6,  EXTRA_IO_uC2
    GPIO_MOT2_ADC_TRIG,          ///< 28, GPIO uP Signal INPUT,  PORT E, Pin 7,  MOT2_ADC_TRIG
    GPIO_EXTRA_IO_uC1,           ///< 29, GPIO uP Signal OUTPUT, PORT E, Pin 10, EXTRA_IO_uC1
    GPIO_SDHC0_LED,              ///< 30, GPIO uP Signal OUTPUT, PORT E, Pin 12, SDHC0_LED

    GPIO_EN_5V,                  ///< 31, GPIO Expander Signal OUTPUT, EXP0_0, EN_5V,
    GPIO_LCD_RESET,              ///< 32, GPIO Expander Signal OUTPUT, EXP0_1, LCD_RESET,
    GPIO_EXP_IO0,                ///< 33, GPIO Expander Signal OUTPUT, EXP0_2, EXP_IO0,
    GPIO_EXP_IO1,                ///< 34, GPIO Expander Signal OUTPUT, EXP0_3, EXP_IO1,
    GPIO_EXP_IO2,                ///< 35, GPIO Expander Signal INPUT,  EXP0_4, EXP_IO2,
    GPIO_1W_BATT_ENABLE,         ///< 36, GPIO Expander Signal OUTPUT, EXP0_5, 1W_BATT_ENABLE,
    GPIO_1W_EXP_ENABLE,          ///< 37, GPIO Expander Signal OUTPUT, EXP0_6, 1W_EXP_ENABLE,
    GPIO_EN_3V,                  ///< 38, GPIO Expander Signal OUTPUT, EXP0_7, EN_3V,

    GPIO_EN_2P5V,                ///< 39, GPIO Expander Signal OUTPUT, EXP1_0, EN_2P5V,
    GPIO_EN_VDISP,               ///< 40, GPIO Expander Signal OUTPUT, EXP1_1, EN_VDISP,
    GPIO_PZT_EN,                 ///< 41, GPIO Expander Signal OUTPUT, EXP1_2, PZT_EN,
    GPIO_FPGA_READY,             ///< 42, GPIO Expander Signal INPUT,  EXP1_3, FPGA_READY,
    GPIO_FPGA_SLEEP,             ///< 43, GPIO Expander Signal OUTPUT, EXP1_4, FPGA_SLEEP,
    GPIO_EN_SMB,                 ///< 44, GPIO Expander Signal OUTPUT, EXP1_5, EN_SMB,
    GPIO_FPGA_SPI_RESET,         ///< 45, GPIO Expander Signal OUTPUT, EXP1_6, FPGA_SPI_RESET,
    GPIO_FPGA_SAFETY,            ///< 46, GPIO Expander Signal INPUT,  EXP1_7, FPGA_SAFETY,
    GPIO_SIGNAL_LAST             ///< 47, GPIO Signal Last
} GPIO_SIGNAL;


typedef struct{                 /// Data for test hook
  GPIO_SIGNAL Signal;           ///< Signal Name
  bool *pValue;                ///< pointer to signal state
}GPIO_TM_DATA;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern GPIO_STATUS L3_GpioCtrlInit(void);

extern GPIO_STATUS L3_GpioCtrlSetSignal(GPIO_SIGNAL eSignal);
extern GPIO_STATUS L3_GpioCtrlClearSignal(GPIO_SIGNAL eSignal);
extern GPIO_STATUS L3_GpioCtrlToggleSignal(GPIO_SIGNAL eSignal);

extern GPIO_STATUS L3_GpioCtrlGetSignal(GPIO_SIGNAL eSignal, bool *pbGetValue);

extern GPIO_STATUS L3_GpioCtrlEnableCallBack(GPIO_SIGNAL eSignal, GPIO_uP_PIN_INT_CONFIG_T *pxIntConfigIn);
extern GPIO_STATUS L3_GpioCtrlDisableCallBack(GPIO_SIGNAL eSignal);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_GPIOCTRL_H */
