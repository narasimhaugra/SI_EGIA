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
 * \brief   This is a GUI Rotation Deactivation count 3 Screen
 *
 * \details 
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rb_deactivatecount3_right.c
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
 * \brief   Set ROTATE_DEACT_RIGHT_C3_SCREEN. Rotation Buttons Deactivate Countdown3 Right 
 *
 * \details This function Set the Rotation Buttons Deactivate Countdown3 Right screen.
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/
void Gui_RotateDeactRightCount3_ScreenSet( void )
{
  if(UI_ReturnToDefaultParameters())
  {
   BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
   snprintf(TextInRightRing.ObjText.Text, MAX_TEXT_SIZE, "3");
   L4_DmShowScreen_New(SCREEN_ID_RB_DEACTIVATION_COUNT3_RIGHT,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rotate_deactivate_right_countdown_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

