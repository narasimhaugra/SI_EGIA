#ifndef L3_ONEWIRERTC_H
#define L3_ONEWIRERTC_H

#ifdef __cplusplus      /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_OneWire
 * \{
 * \brief       Public interface for One Wire module.
 *
 * \details     Symbolic types and constants are included with the interface prototypes
 *
 * \copyright   2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file        L3_OneWireRtc.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"
#include "L2_OnchipRtc.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/*****************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum
{                               /*! 1-Wire RTC status */
  BATT_RTC_STATUS_OK,           /*! RTC clock function status - All good */
  BATT_RTC_STATUS_PARAM_ERROR,  /*! Invalid parameter supplied */
  BATT_RTC_STATUS_STOPPED,      /*! RTC clock stopped */
  BATT_RTC_STATUS_NOT_FOUND,    /*! RTC clock device not found */
  BATT_RTC_STATUS_ERROR,        /*! RTC clock error */
} BATT_RTC_STATUS;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern BATT_RTC_STATUS L3_BatteryRtcInit (void);
extern BATT_RTC_STATUS L3_BatteryRtcRead (RTC_SECONDS *pTime);
extern BATT_RTC_STATUS L3_BatteryRtcWrite (RTC_SECONDS *pTime);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_ONEWIRERTC_H */
