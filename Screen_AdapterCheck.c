#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI Adapter Check Screen
 *
 * \details Define Adapter Check screen and associated action methods. 
 *          screen plays animation when adapter connection has issue.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_AdapterCheck.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_AdapterCheck.h"
#include "UI_externals.h"
   
static UI_SCREEN Screen_AdapterCheck1_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideWhiteBox,
  &BatteryImage,
  &BatteryProgressBar,  
  &GreenLoading_1,
  &HandleWithoutGreenButtonImage,
  NULL
};

static UI_SCREEN Screen_AdapterCheck2_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideWhiteBox,
  &BatteryImage,
  &BatteryProgressBar,  
  &GreenLoading_2,
  &HandleWithoutGreenButtonImage,
  NULL
};

static UI_SCREEN Screen_AdapterCheck3_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideWhiteBox,
  &BatteryImage,
  &BatteryProgressBar,  
  &GreenLoading_3,
  &HandleWithoutGreenButtonImage,
  NULL
};

static UI_SCREEN Screen_AdapterCheck4_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideWhiteBox,
  &BatteryImage,
  &BatteryProgressBar,  
  &GreenLoading_4,
  &HandleWithoutGreenButtonImage,
  NULL
};

static UI_SCREEN Screen_AdapterCheck5_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideWhiteBox,
  &BatteryImage,
  &BatteryProgressBar,  
  &GreenLoading_5,
  &HandleWithoutGreenButtonImage,
  NULL
};

static UI_SCREEN Screen_AdapterCheck6_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideWhiteBox,
  &BatteryImage,
  &BatteryProgressBar,  
  &GreenLoading_6,
  &HandleWithoutGreenButtonImage,
  NULL
};
   
UI_SEQUENCE Sequence_AdapterCheck_Sequence[] =
  {
    Screen_AdapterCheck1_Screen,
    Screen_AdapterCheck2_Screen,
    Screen_AdapterCheck3_Screen,
    Screen_AdapterCheck4_Screen,
    Screen_AdapterCheck5_Screen,
    Screen_AdapterCheck6_Screen,
    NULL
  };


void Gui_AdapterCheck_Screen(void)
{
  if(UI_ReturnToDefaultParameters())
  {
    HandleWithoutGreenButtonImage.ObjBitmap.X = HandleWithoutGreenButtonImage.ObjBitmap.X + 2;
    HandleWithoutGreenButtonImage.ObjBitmap.Y = HandleWithoutGreenButtonImage.ObjBitmap.Y - 3;
    
    L4_DmShowScreen_New(SCREEN_ID_ADAPTER_CHECK,
                        600, Sequence_AdapterCheck_Sequence);
  }
}

/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

