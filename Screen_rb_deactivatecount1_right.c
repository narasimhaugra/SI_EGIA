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
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rb_deactivatecount1_right.c
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
 * \brief   Set ROTATE_DEACT_RIGHT_C1_SCREEN. Rotation Buttons Deactivate Countdown Right 1 
 *
 * \details This function Set the Rotation Buttons Deactivate Countdown Right 1 screen.
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/

void Gui_RotateDeactRightCount1_ScreenSet( void )
{
  if(UI_ReturnToDefaultParameters())
  {
  BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
   L4_DmShowScreen_New(SCREEN_ID_RB_DEACTIVATION_COUNT1_RIGHT,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rotate_deactivate_right_countdown_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

