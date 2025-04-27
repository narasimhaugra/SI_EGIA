#ifndef L2_ADC_H
#define L2_ADC_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Adc
 * \{
 * \brief   Public interfaces for ADC control routines
 *
 * \details This module defines symbolic constants as well as function prototypes
 *
 * \copyright 2019 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_Adc.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum
{
    ADC_STATUS_OK,                      ///< No error
    ADC_STATUS_BUSY,                    ///< Conversion in progress - last known value returned
    ADC_STATUS_DATA_OLD,                ///< No conversion in progress - last known value returned
    ADC_STATUS_DATA_NEW,                ///< ADC conversion complete - updated value returned
    ADC_STATUS_CAL_FAIL,                ///< ADC self cal fail
    ADC_STATUS_INVALID_PARAMETER,       ///< Invalid/unsupported ADC instance / channel
    ADC_STATUS_COUNT                    ///< Number of status return values
} ADC_STATUS;

typedef enum
{
    ADC0,                               ///< ADC0 - Motor 0
    ADC1,                               ///< ADC1 - Aux (Thermistor / battery)
    ADC2,                               ///< ADC2 - Motor 2
    ADC3,                               ///< ADC3 - Motor 1
    ADC_COUNT                           ///< Number of ADC modules
} ADC_INSTANCE;

/**
 * \details For layer 3 reference, ADC channel assignments are as follows:
 *              - ADC0
 *                  - Channel 0 - SE16: Motor 0 avg current
 *                  - Channel 1 - SE22: Motor 0 peak current
 *                  - Channel 2 - SE23: Motor 0 temp
 *              - ADC1
 *                  - Channel 0 - SE13: Battery voltage
 *                  - Channel 1 - SE23: Thermistor voltage
 *                  - Channel 2 - DP1:  Hardware version
 *              - ADC2
 *                  - Channel 0 - SE12: Motor 2 avg current
 *                  - Channel 1 - SE13: Motor 2 peak current
 *                  - Channel 2 - SE14: Motor 2 temp
 *              - ADC3
 *                  - Channel 0 - SE4a: Motor 1 avg current
 *                  - Channel 1 - SE5a: Motor 1 peak current
 *                  - Channel 2 - SE6a: Motor 1 temp
 *                  .
*/

typedef enum
{
    ADC_CH0,                ///< ADC channel 0
    ADC_CH1,                ///< ADC channel 1
    ADC_CH2,                ///< ADC channel 2
    ADC_CH_COUNT            ///< Number of ADC channels supported.
} ADC_CHANNEL;              /// ADC channel

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
ADC_STATUS L2_AdcInit(void);
ADC_STATUS L2_AdcConfig(ADC_INSTANCE Num,  ADC_CHANNEL Chan);
ADC_STATUS L2_AdcSetOffset(ADC_INSTANCE Num,  uint16_t Ofst);
ADC_STATUS L2_AdcGetStatus(ADC_INSTANCE Num);
ADC_STATUS L2_AdcStart(ADC_INSTANCE Num, bool HwTrig);
ADC_STATUS L2_AdcRead(ADC_INSTANCE Num,  uint16_t *pResult);

/**
 * \}  <If using addtogroup above>
 */
 
#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L2_ADC_H */

