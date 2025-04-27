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
 * \brief   This is a GUI Rotation Button  Left side activated Screen 
 *
 * \details Define If Left side Rotation button is activated 
 *          then the screen shall show the Left side activated Screen.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rb_activated_left.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_RotationButton.h"
#include "UI_externals.h"

static UI_SCREEN Screen_rb_activated_left_Screen[] =
{
  &GreenBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &RotateHandleImage,
  &UpperWhiteBar,
  &UpperMagentaBar,
  &TextOnUpperMagentaBar,
  &LeftGreenButton,
  &RightWhiteButton,
  &LeftGreenCircle,
  NULL
};
 
static UI_SEQUENCE Sequence_rb_activated_left_Sequence[] =
  {
    Screen_rb_activated_left_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
 /**
 *
 * \brief   Set ROTATION_ACTIVATED_LEFT_SCREEN. Rotation Buttons Activated Left
 *
 * \details This function show the Rotation Buttons Activated Left Screen 
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/
void Gui_RotationACtivatedLeft_ScreenSet( void )
{
  if(UI_ReturnToDefaultParameters())
  {
  BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
  L4_DmShowScreen_New(SCREEN_ID_RB_ACTIVATED_LEFT,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rb_activated_left_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

