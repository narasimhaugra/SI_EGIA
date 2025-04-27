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
 * \brief   This is a GUI Rotation Button Deactivated Screen
 *
 * \details 
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rb_deactivated_right.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_RotationButton.h"
#include "UI_externals.h"

static UI_SCREEN Screen_rb_deactivated_right_Screen[] =
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
  &LeftRing,
  &RightRing,
  NULL
};
 
static UI_SEQUENCE Sequence_rb_deactivated_right_Sequence[] =
  {
    Screen_rb_deactivated_right_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
 /**
 *
 * \brief   Set ROTATION_DEACT_RIGHT_SCREEN. Rotation Buttons Deactivated Right
 *
 * \details This function Set the EGIA Rotation Buttons Deactivated Right.
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/
void Gui_RotateDeactRight_ScreenSet( void )
{
  if(UI_ReturnToDefaultParameters())
  {
   BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
  RightRing.ObjCircle.BorderColor = SIG_COLOR_WHITE;
  L4_DmShowScreen_New(SCREEN_ID_RB_DEACTIVATED_RIGHT,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rb_deactivated_right_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

