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
 * \brief   This is a GUI Rotation Button Screen buth sides activated
 *
 * \details Define If both sidesthe Rotation button are activated 
 *          then the screen shall show both sides as activated.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_rb_activated_both.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_RotationButton.h"
#include "UI_externals.h"

static UI_SCREEN Screen_rb_activated_both_Screen[] =
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
  &RightGreenButton,
  &LeftGreenCircle,
  &RightGreenCircle,
  &LeftRing,
  &RightRing,
  NULL
};
 
static UI_SEQUENCE Sequence_rb_activated_both_Sequence[] =
  {
    Screen_rb_activated_both_Screen,
    NULL
  };

/* ==========================================================================*/
 /**
 *
 * \brief   Set ROTATION_ACTIVATED_BOTH_SCREEN. 
 *          Rotation Button Activate Indicator While both Side is Active
 *
 * \details This function show the Rotation Button Activate in both side screen 
 *           
 *
 * \param   < None >
 *
 * \return  None
 *
* ==========================================================================*/
void Gui_RotationActivatedBoth_ScreenSet( void )
{   
  if(UI_ReturnToDefaultParameters())
  {
    BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
    L4_DmShowScreen_New(SCREEN_ID_RB_ACTIVATED_BOTH,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_rb_activated_both_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

