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
 * \brief   This is a GUI Rotation Button Screen
 *
 * \details The screen shall show based on the side that the
 *          inactive rotation button is pressed.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rbai_right.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_RotationButton.h"
#include "UI_externals.h"

static UI_SCREEN Screen_rb_ai_right_Screen[] =
{
  &GreenBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &RotateHandleImage,
  &UpperWhiteBar,
  &UpperMagentaBar,
  &TextOnUpperMagentaBar,
  &RightWhiteButton,
  &LeftWhiteButton,
  &LeftUpperWhiteArrow,
  &LeftLowerWhiteArrow,
  NULL
};
 
static UI_SEQUENCE Sequence_rb_ai_right_Sequence[] =
  {
    Screen_rb_ai_right_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
 /**
 *
 * \brief   Set ROTATE_ACTIVATE_RIGHT_SCREEN. Rotation Button Activate Indicator Left
 *
 * \details This function Set the Rotation button Activate Indicator Right Screen
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/
void Gui_RotateActivateRight_ScreenSet( void )
{
  if(UI_ReturnToDefaultParameters())
  {
    BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
    L4_DmShowScreen_New(SCREEN_ID_RBAI_RIGHT,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rb_ai_right_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

