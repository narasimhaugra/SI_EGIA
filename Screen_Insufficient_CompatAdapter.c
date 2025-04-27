#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI template screen
 *
 * \details Define Adapter Compatible screen and associated action methods.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_Insufficient_CompatAdapter.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_Insufficient_CompatAdapter.h"
#include "UI_externals.h"

static bool b_Switch = true;
//static bool b_Switch = false;

static UI_SCREEN Screen_Insufficient_CompatAdapter[] =
{   
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage_0,
  &HandleWithoutGreenButtonImage,
  &EGIAReloadImage,
  &EEAreloadImage,
  &GreenCircleImage_1,
  &GreenCircleImage_2,
  &TextRemCount,
  NULL
 };

static UI_SCREEN Screen_InsuffBatt2[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage_0,
  &TextLeftMiddle,
  &TextForX,
  &BigReloadImage,
  NULL
};

static UI_SEQUENCE Sequence_Insufficient_CompatAdapter1[] =
  {
    Screen_Insufficient_CompatAdapter,
    NULL
  };   

static UI_SEQUENCE Sequence_Insufficient_CompatAdapter2[] =
  {
    Screen_InsuffBatt2,
    NULL
  };   

/******************************************************************************/
/*                                 Global Functions                           */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Shows alternate screens for Insufficient Batt and No-Clamshell condition
 *  
 * \details This function updates alternate Screen
 *  
 * \param   < None >
 *  
 * \return  None
 * 
 * ========================================================================== */

void Gui_AlterInsuffBattNoClamshell_Screen(void)
{
    if(UI_ReturnToDefaultParameters())
    {
    BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_GRAY;
    HandleWithoutGreenButtonImage.ObjBitmap.X = 55;
    HandleWithoutGreenButtonImage.ObjBitmap.Y = 35;
    EGIAReloadImage.ObjBitmap.X = 25;
    EGIAReloadImage.ObjBitmap.Y = 35;
    snprintf(TextLeftMiddle.ObjText.Text, MAX_TEXT_SIZE, "0");
    snprintf(TextForX.ObjText.Text, MAX_TEXT_SIZE, "x");
    
    if(b_Switch)
    {
    L4_DmShowScreen_New(SCREEN_ID_INSUFFICIENT_COMPAT_ADAPTER,
                        
                        UI_SEQUENCE_DEFAULT_REFRESH_RATE, 
    
                        Sequence_Insufficient_CompatAdapter1 );
    }
    else
    {
      L4_DmShowScreen_New(SCREEN_ID_INSUFFICIENT_COMPAT_ADAPTER,
                        
                        UI_SEQUENCE_DEFAULT_REFRESH_RATE, 
    
                        Sequence_Insufficient_CompatAdapter2 );
    }
    
    b_Switch = !b_Switch;
    OSTimeDly(MSEC_500);
    }
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

