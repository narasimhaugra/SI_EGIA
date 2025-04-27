#ifndef WELCOMESCREEN_H
#define WELCOMESCREEN_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif  
/* ========================================================================== */
/**
 * \addtogroup WelcomeScreen
 * \{

 * \brief   Public interface for Welcome Screen.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    WelcomeScreen.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"                      /*! Import common definitions such as types, etc */

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
void WelcomeScreenShow(uint8_t *pVersion);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_ADAPTERMANAGER_H */

