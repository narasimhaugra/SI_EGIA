#ifndef  SCREEN_ROTATIONBUTTON_H
#define  SCREEN_ROTATIONBUTTON_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 * \brief   Public interface for GUI Rotation Button Screenen
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_RotationButton.h
 *
 * ========================================================================== */ 
/******************************************************************************/ 
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         /* Import common definitions such as types, etc. */
#include "L3_DispPort.h"
/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/
extern void Gui_RotateActivateRight_ScreenSet( void );
extern void Gui_RotateActivateLeft_ScreenSet( void );
extern void Gui_RotateActivateLeftCount1_ScreenSet( void );
extern void Gui_RotateActivateLeftCount2_ScreenSet( void );
extern void Gui_RotateActivateLeftCount3_ScreenSet( void );
extern void Gui_RotateActivateRightCount1_ScreenSet( void );
extern void Gui_RotateActivateRightCount2_ScreenSet( void );
extern void Gui_RotateActivateRightCount3_ScreenSet( void );
extern void Gui_RotationActivatedBoth_ScreenSet( void );
extern void Gui_RotationACtivatedLeft_ScreenSet( void );
extern void Gui_RotationACtivatedRight_ScreenSet( void );
extern void Gui_RotateActivateLeftActive_ScreenSet( void );
extern void Gui_RotateActivateRightActive_ScreenSet( void );
extern void Gui_RotateDeactLeftCount1_ScreenSet( void );
extern void Gui_RotateDeactRightCount1_ScreenSet( void );
extern void Gui_RotateDeactLeftCount2_ScreenSet( void );
extern void Gui_RotateDeactRightCount2_ScreenSet( void );
extern void Gui_RotateDeactLeftCount3_ScreenSet( void );
extern void Gui_RotateDeactRightCount3_ScreenSet( void );
extern void Gui_RotateDeactLeft_ScreenSet( void );
extern void Gui_RotateDeactRight_ScreenSet( void );

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SCREEN_ROTATIONBUTTON_H */

