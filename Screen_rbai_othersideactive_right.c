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
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rbai_othersideactive_right.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_RotationButton.h"
#include "UI_externals.h"

static UI_SCREEN Screen_rbai_othersideactive_right_Screen[] =
{
  &GreenBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &RotateHandleImage,
  &UpperWhiteBar,
  &UpperMagentaBar,
  &TextOnUpperMagentaBar,
  &RightGreenButton,
  &LeftWhiteButton,
  &RightGreenCircle,
  &RightRing,
  &LeftUpperWhiteArrow,
  &LeftLowerWhiteArrow,
  NULL
};
 
static UI_SEQUENCE Sequence_rbai_othersideactive_right_Sequence[] =
  {
    Screen_rbai_othersideactive_right_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
 /**
 *
 * \brief   Set ROTATE_ACTIVATE_RIGHT_ACTIVE_SCREEN. Rotation Button Activate  
 *			Indicator While Right Side Active 
 *
 * \details This function set the Rotation Button Activate Indicator While Right Side Active scren
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/
void Gui_RotateActivateRightActive_ScreenSet( void )
{
  if(UI_ReturnToDefaultParameters())
  {
    BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
    L4_DmShowScreen_New(SCREEN_ID_RBAI_RIGHT,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rbai_othersideactive_right_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

