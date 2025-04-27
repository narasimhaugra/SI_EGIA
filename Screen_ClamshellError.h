#ifndef  SCREEN_CLAMSHELLERROR_H       
#define  SCREEN_CLAMSHELLERROR_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 * \brief   Public interface for Clamshell Screen
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_ClamshellError.h
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
extern DM_SCREEN ClamshellErrorScreen;
/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/
extern DM_STATUS Gui_InsertClamshellErrorScreen(void);

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SCREEN_CLAMSHELLERROR_H */

