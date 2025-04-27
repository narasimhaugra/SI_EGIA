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
 * \brief This is a GUI Rotation Button Activate Indicator while other Side is Active
 *
 * \details The screen shall show based on which side is active and inactive.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rbai_othersideactive_left.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_RotationButton.h"
#include "UI_externals.h"

static UI_SCREEN Screen_rbai_othersideactive_left_Screen[] =
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
  &RightUpperWhiteArrow,
  &RightLowerWhiteArrow,
  NULL
};
 
static UI_SEQUENCE Sequence_rbai_othersideactive_left_Sequence[] =
  {
    Screen_rbai_othersideactive_left_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
 /**
 *
 * \brief   Set ROTATE_ACTIVATE_LEFT_ACTIVE_SCREEN. 
 *          Rotation Button Activate Indicator While Left Side Active
 *
 * \details This function Set the Rotation Button Activate Indicator While Left Side Active screen
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/
void Gui_RotateActivateLeftActive_ScreenSet( void )
{
  if(UI_ReturnToDefaultParameters())
  {
    BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
    L4_DmShowScreen_New(SCREEN_ID_RBAI_OTHER_SIDE_ACTIVE_LEFT,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rbai_othersideactive_left_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

