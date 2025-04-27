#ifndef  SCREEN_USEDCS_H
#define  SCREEN_USEDCS_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 * \brief   Public interface for Used Clamshell Screen
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_UsedClamShell.h
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
extern DM_SCREEN UsedCSScreen;
/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
DM_STATUS Gui_Used_CS_Screen(void);

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SCREEN_USEDCS_H */

