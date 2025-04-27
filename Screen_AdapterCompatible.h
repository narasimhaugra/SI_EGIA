#ifndef  SCREEN_ADAPTER_COMPATIBLE_H
#define  SCREEN_ADAPTER_COMPATIBLE_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 * \brief   Public interface for Adapter Compatible Screen
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_AdapterCompatible.h
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

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void Show_AdapterCompatibleScreen (uint16_t ProcedureRemainCnt);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SCREEN_ADAPTER_COMPATIBLE_H */

