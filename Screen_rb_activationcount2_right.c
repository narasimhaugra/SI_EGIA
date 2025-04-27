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
 * \brief   This is a GUI Rotation activation count 2 Screen
 *
 * \details 
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rb_activationcount2_right.c
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
 * \brief   Set ROTATE_ACTIVATE_RIGHT_C2_SCREEN. Rotation Button Activation Countdown2, Right
 *
 * \details This function Set the Rotation Button Activation Countdown2 Right screen.
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/
void Gui_RotateActivateRightCount2_ScreenSet( void )
{
  if(UI_ReturnToDefaultParameters())
  {
   BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
   RightRing.ObjCircle.BorderColor = SIG_COLOR_WHITE;
   snprintf(TextInRightRing.ObjText.Text, MAX_TEXT_SIZE, "2");
   L4_DmShowScreen_New(SCREEN_ID_RB_ACTIVATION_COUNT2_RIGHT,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rotate_activate_right_countdown_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

