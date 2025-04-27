#ifndef  SCREEN_HANDLE_ERROR_H
#define  SCREEN_HANDLE_ERROR_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 * \brief   Public interface for Handle Error Screen
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_HandleError.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "L4_DisplayManager.h"
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
extern DM_SCREEN HandleErrorScreen;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
void Gui_HandleErrorScreen(void);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SCREEN_HANDLE_ERROR_H */

