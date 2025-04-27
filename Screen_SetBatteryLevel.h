#ifndef  SCREEN_SETBATTERYLEVEL_H   
#define  SCREEN_SETBATTERYLEVEL_H         

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 * \brief   Public interface for Insert Power Pack status Screen
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_SetBatteryLevel.h
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

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void Gui_SetBatteryLevel(uint8_t ScreenId, uint8_t BatteryLevel);

/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SCREEN_SETBATTERYLEVEL_H */
