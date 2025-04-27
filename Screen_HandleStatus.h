#ifndef  SCREENHANDLESTATUS_H 
#define  SCREENHANDLESTATUS_H         

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 * \brief   Public interface for Insert Power Pack status Screen
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_HandleStatus.h
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
void Set_StatusColor(SIG_COLOR_PALETTE BorderColor);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SCREENHANDLESTATUS_H */

