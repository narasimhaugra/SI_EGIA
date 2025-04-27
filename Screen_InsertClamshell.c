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
 * \details Define Insert Clamshell screen and associated action methods.
 *          screen plays animation request user to connect clampshell.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_InsertClamshell.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_InsertClamshell.h"
#include "UI_externals.h"

extern const unsigned char _acRequestClamshell_ClamshellBack[];
extern const unsigned char _acRequestClamshell_Guide[];
extern const unsigned char _acRequestClamshell_PowerPack[];
extern const unsigned char _acPowerPack[];
extern const unsigned char _acRequestClamshell_ClamshellDoor_01[];
extern const unsigned char _acRequestClamshell_ClamshellDoor_02[];
extern const unsigned char _acRequestClamshell_ClamshellDoor_03[];
extern const unsigned char _acRequestClamshell_ClamshellDoor_04[];   

static UI_OBJECT Ppack1 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  28, // uiXpos
  18, // uiYpos
  39, // uiXsize
  28, // uiYsize
  (BITMAP*)&_acRequestClamshell_PowerPack
  //(BITMAP*)&_acPowerPack    
  }
};

static UI_OBJECT Ppack2 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  28, // uiXpos
  30, // uiYpos
  39, // uiXsize
  28, // uiYsize
  (BITMAP*)&_acRequestClamshell_PowerPack
  //(BITMAP*)&_acPowerPack
  }
};

static UI_OBJECT Ppack3 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  28, // uiXpos
  42, // uiYpos
  39, // uiXsize
  28, // uiYsize
  (BITMAP*)&_acRequestClamshell_PowerPack
  //(BITMAP*)&_acPowerPack
  }
};

static UI_OBJECT Ppack4 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  28, // uiXpos
  54, // uiYpos
  39, // uiXsize
  28, // uiYsize
  (BITMAP*)&_acRequestClamshell_PowerPack
  //(BITMAP*)&_acPowerPack
  }
};

static UI_OBJECT Guide1 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  5, // uiXpos
  40, // uiYpos
  54, // uiXsize
  18, // uiYsize
  (BITMAP*)&_acRequestClamshell_Guide
  }
};

static UI_OBJECT Guide2 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  8, // uiXpos
  45, // uiYpos
  54, // uiXsize
  18, // uiYsize
  (BITMAP*)&_acRequestClamshell_Guide
  }
};

static UI_OBJECT Guide3 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  12, // uiXpos
  51, // uiYpos
  54, // uiXsize
  18, // uiYsize
  (BITMAP*)&_acRequestClamshell_Guide
  }
};

static UI_OBJECT Guide4 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  16, // uiXpos
  53, // uiYpos
  54, // uiXsize
  18, // uiYsize
  (BITMAP*)&_acRequestClamshell_Guide
  }
};

static UI_OBJECT Guide5 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  16, // uiXpos
  55, // uiYpos
  54, // uiXsize
  18, // uiYsize
  (BITMAP*)&_acRequestClamshell_Guide
  }
};

static UI_OBJECT Door1 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  62, 22, 15, 42,
  (BITMAP*)&_acRequestClamshell_ClamshellDoor_01
  }
};

static UI_OBJECT Door2 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  51, 25, 27, 39,
  (BITMAP*)&_acRequestClamshell_ClamshellDoor_02
  }
};

static UI_OBJECT Door3 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  36, 35, 40, 29,
  (BITMAP*)&_acRequestClamshell_ClamshellDoor_03
  }
};

static UI_OBJECT Door4 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  28, 50, 41, 17,
  (BITMAP*)&_acRequestClamshell_ClamshellDoor_04
  }
};

static UI_OBJECT Back =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  28, 59, 40, 25,
  (BITMAP*)&_acRequestClamshell_ClamshellBack
  }
};

static UI_SCREEN Screen_InsertClamshell1_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door1,
  NULL
};

static UI_SCREEN Screen_InsertClamshell2_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door1,
  &Guide1,
  NULL
};

static UI_SCREEN Screen_InsertClamshell3_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door1,
  &Guide2,
  NULL
};

static UI_SCREEN Screen_InsertClamshell4_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Guide3,
  &Door1,
  NULL
};

static UI_SCREEN Screen_InsertClamshell5_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door1,
  &Guide4,
  NULL
};

static UI_SCREEN Screen_InsertClamshell6_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door1,
  &Guide5,
  NULL
};

static UI_SCREEN Screen_InsertClamshell7_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Ppack1,
  &Door1,
  &Guide5,
  NULL
};

static UI_SCREEN Screen_InsertClamshell8_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Ppack2,
  &Door1,
  &Guide5,
  NULL
};

static UI_SCREEN Screen_InsertClamshell9_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Ppack3,
  &Door1,
  &Guide5,
  NULL
};

static UI_SCREEN Screen_InsertClamshell10_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Ppack4,
  &Door1,
  &Guide5,
  NULL
};

static UI_SCREEN Screen_InsertClamshell11_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door1,
  &Ppack4,
  &Guide4,
  NULL
};

static UI_SCREEN Screen_InsertClamshell12_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door1,
  &Ppack4,
  &Guide3,
  NULL
};

static UI_SCREEN Screen_InsertClamshell13_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door1,
  &Ppack4,
  &Guide2,
  NULL
};

static UI_SCREEN Screen_InsertClamshell14_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door1,
  &Ppack4,
  &Guide1,
  NULL
};

static UI_SCREEN Screen_InsertClamshell15_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door1,
  &Ppack4,
  NULL
};

static UI_SCREEN Screen_InsertClamshell16_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door2,
  &Ppack4,
  NULL
};

static UI_SCREEN Screen_InsertClamshell17_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door3,
  &Ppack4,
  NULL
};

static UI_SCREEN Screen_InsertClamshell18_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &Back,
  &Door4,
  &Ppack4,
  NULL
};

static UI_SEQUENCE Sequence_InsertClamshell_Sequence[] =
  {
    Screen_InsertClamshell1_Screen,
    Screen_InsertClamshell1_Screen,
    Screen_InsertClamshell2_Screen,
    Screen_InsertClamshell3_Screen,
    Screen_InsertClamshell4_Screen,
    Screen_InsertClamshell5_Screen,
    Screen_InsertClamshell6_Screen,
    Screen_InsertClamshell7_Screen,
    Screen_InsertClamshell8_Screen,
    Screen_InsertClamshell9_Screen,
    Screen_InsertClamshell10_Screen,
    Screen_InsertClamshell11_Screen,
    Screen_InsertClamshell12_Screen,
    Screen_InsertClamshell13_Screen,
    Screen_InsertClamshell14_Screen,
    Screen_InsertClamshell15_Screen,
    Screen_InsertClamshell16_Screen,
    Screen_InsertClamshell17_Screen,
    Screen_InsertClamshell18_Screen,
    Screen_InsertClamshell18_Screen,
    Screen_InsertClamshell18_Screen,
    Screen_InsertClamshell18_Screen,
    NULL
  };
 
/* ========================================================================== */
/**
 * \brief   Insert Clamshell screen template test.
 *
 * \details Execute sample screen template test.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void Show_InsertClamshellScreen(void)
{
  if(UI_ReturnToDefaultParameters())
  {
    BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_GRAY;
    BatteryProgressBar.ObjText.BackColor = SIG_COLOR_GRAY;
    L4_DmShowScreen_New(SCREEN_ID_INSERT_CLAMSHELL,
    400, Sequence_InsertClamshell_Sequence);
  }    
}

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/

/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

