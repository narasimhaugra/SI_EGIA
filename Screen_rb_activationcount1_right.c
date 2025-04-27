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
 * \brief   This is a GUI Rotation activation count1 Screen
 *
 * \details 
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rb_activationcount1_right.c
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
 * \brief   Set ROTATE_ACTIVATE_RIGHT_C1_SCREEN. Rotation Button Activation Countdown1 Right 
 *
 * \details This function Set the Rotation Button Activation Countdown1 Right screen.
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/

void Gui_RotateActivateRightCount1_ScreenSet( void )
{
  if(UI_ReturnToDefaultParameters())
  {
   BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
   RightRing.ObjCircle.BorderColor = SIG_COLOR_WHITE;
   L4_DmShowScreen_New(SCREEN_ID_RB_ACTIVATION_COUNT1_RIGHT,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rotate_activate_right_countdown_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

