#ifndef  SCREEN_BATTERY_H
#define  SCREEN_BATTERY_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
  * \addtogroup Screens
 * \{
 * \brief   Public interface to show Battery charge level on active Screen
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_Battery.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         /* Import common definitions such as types, etc. */

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
extern void UpdateBatteryLevel(void);
extern void Gui_BatteryImageUpdate(uint32_t BatteryLevel, bool ScreenRefresh);
extern void UpdateBatteryImage(uint16_t Color);

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SCREEN_BATTERY_H */

