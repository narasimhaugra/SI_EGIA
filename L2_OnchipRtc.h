#ifndef L2_ONCHIP_RTC
#define L2_ONCHIP_RTC

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_OnchipRtc
 * \{
 * \brief   Public interface file for On-Chip RTC routines
 *
 * \details This file has all On-Chip RTC related symbolic constants and
 *          function prototypes.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_OnchipRtc.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"
#include "clk.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
typedef uint32_t RTC_SECONDS;    /* 32 bit real time seconds counter */

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef void (*OC_RTC_HANDLER)(uint32_t Seconds);     

typedef enum                    /* Status Codes */
{
    OC_RTC_STATUS_OK,           /*! No error */
    OC_RTC_STATUS_ERROR,        /*! General error */
    OC_RTC_STATUS_INVALID_PARAM,/*! Invalid input */
    OC_RTC_STATUS_OVERFLOW,     /*! Second counter overflow */
    OC_RTC_STATUS_LAST          /*! Status code end indicator */
} OC_RTC_STATUS;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
void          L2_OnchipRtcInit(void);
OC_RTC_STATUS L2_OnchipRtcConfig(bool Enable, OC_RTC_HANDLER pHandler);
void          L2_OnchipRtcWrite(uint32_t Seconds);
RTC_SECONDS   L2_OnchipRtcRead(void);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L2_ONCHIP_RTC */
