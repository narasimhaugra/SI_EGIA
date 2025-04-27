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
 * \details Define Adapter Check screen and associated action methods. 
 *          screen plays animation when adapter connection has issue.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rb_activationcount2_left.c
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
 * \brief   Set ROTATE_ACTIVATE_LEFT_C2_SCREEN. Rotation Button Activation Countdown2 Left
 *
 * \details This function Set the EGIA Rotation button activate count2 Left screen.
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/
void Gui_RotateActivateLeftCount2_ScreenSet( void )
{
  if(UI_ReturnToDefaultParameters())
  {
  BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
   LeftRing.ObjCircle.BorderColor = SIG_COLOR_WHITE;
   snprintf(TextInLeftRing.ObjText.Text, MAX_TEXT_SIZE, "2");
   L4_DmShowScreen_New(SCREEN_ID_RB_ACTIVATION_COUNT2_LEFT,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rotate_activate_left_countdown_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

