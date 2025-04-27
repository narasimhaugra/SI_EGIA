#ifndef  SCREEN_ADAPTERREQUEST_H
#define  SCREEN_ADAPTERREQUEST_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 * \brief   Public interface for Adapter Request Screen
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_AdapterRequest.h
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
extern DM_SCREEN AdapterReqScreen;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void Show_AdapterRequestScreen(uint16_t ProcedureRemaining);

/**
 * \} Doxygen group end tag
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SCREEN_ADAPTERREQUEST_H */

