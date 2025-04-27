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
 * \brief   This is a GUI Rotation Button Right side activated Screen 
 *
 * \details Define If Right side Rotation button is activated 
 *          then the screen shall show the Right side activated Screen.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rb_activated_right.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_RotationButton.h"
#include "UI_externals.h"

UI_SCREEN Screen_rb_activated_right_Screen[] =
{
  &GreenBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &RotateHandleImage,
  &UpperWhiteBar,
  &UpperMagentaBar,
  &TextOnUpperMagentaBar,
  &LeftWhiteButton,
  &RightGreenButton,
  &RightGreenCircle,
  NULL
};
 
UI_SEQUENCE Sequence_rb_activated_right_Sequence[] =
  {
    Screen_rb_activated_right_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
 /**
 *
 * \brief   Set ROTATION_ACTIVATED_RIGHT_SCREEN. Rotation Buttons Activated Right
 *
 * \details This function Show the Rotation Buttons Activated Right Screen 
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/
void Gui_RotationACtivatedRight_ScreenSet( void )
{
  if(UI_ReturnToDefaultParameters())
  {
  BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
  L4_DmShowScreen_New(SCREEN_ID_RB_ACTIVATED_RIGHT,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rb_activated_right_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

