#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ==========================================================================*/
 /**
 *
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI Rotation Deactivation count 1 Screen
 *
 * \details 
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rb_deactivatecount1_left.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_RotationButton.h"
#include "UI_externals.h"

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
 /**
 *
 * \brief   Set ROTATE_DEACT_LEFT_C1_SCREEN. Rotation Buttons Deactivate Countdown1 Left
 *
 * \details This function Set the Rotation Buttons Deactivate Countdown1 Left screen
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/
void Gui_RotateDeactLeftCount1_ScreenSet( void )		
{
  if(UI_ReturnToDefaultParameters())
  {
   BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
   L4_DmShowScreen_New(SCREEN_ID_RB_DEACTIVATION_COUNT1_LEFT,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rotate_deactivate_left_countdown_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

