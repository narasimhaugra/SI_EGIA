#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI Battery Connection Error Screen template
 *
 * \details This shows the Battery Connection Error Screen. 
 *
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_BatConnectionErr.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_BatConnectionErr.h"
#include "UI_externals.h"
  
static UI_SCREEN Screen_BatConnectionErr_Screen[] =
{
  &YellowBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryErrorImage,
  &TriangleAboveRightPanel,
  &HandleWithoutGreenButtonImage,
  NULL
};  

static UI_SEQUENCE Sequence_BatConnectionErr_Sequence[] =
  {
    Screen_BatConnectionErr_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/**
 * \brief   Shows Battery Connection Error Screen 
 *  
 * \details Shows Battery Connection Error Screen  
 *  
 * \param   < None >
 *  
 * \return  None
  * ========================================================================== */
void Gui_BattConnectionErr_Screen(void)
{
  if(UI_ReturnToDefaultParameters())
{
    BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_GRAY;
    BatteryProgressBar.ObjText.BackColor = SIG_COLOR_GRAY;
    TriangleAboveRightPanel.ObjBitmap.X = 36;
    TriangleAboveRightPanel.ObjBitmap.Y = 23;
    HandleWithoutGreenButtonImage.ObjBitmap.X = 31;
    HandleWithoutGreenButtonImage.ObjBitmap.Y = 43;
    L4_DmShowScreen_New(SCREEN_ID_BATT_CONNECTION_ERR,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_BatConnectionErr_Sequence );
    //L4_DmScreenLockUnLock_New(Sequence_BatConnectionErr_Sequence, SCREEN_LOCK_TEMPORARY);
    L4_DmCurrentScreenLockUnLock_New(SCREEN_LOCK_TEMPORARY);
}
}
/**
 * \brief   Unlock Battery Connection Error Screen 
 *  
 *  
 * \param   < None >
 *  
 * \return  None
  * ========================================================================== */
void Gui_BattConnectionErr_ScreenUnlock(void)
{
    L4_DmCurrentScreenLockUnLock_New(SCREEN_LOCK_OFF);
}
/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

