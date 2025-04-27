#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

  /* ========================================================================== */
/**
 * \addtogroup Screen Template
 * \{
 *
 * \brief   This is a GUI template screen
 *
 * \details Demonstrate how to define screen and associated action methods. This
 *          Template screen plays animation for Welcome Screen.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    WelcomeScreen.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "WelcomeScreen.h"
#include "L3_DispPort.h"
#include "UI_externals.h"

#define DELAY_BETWEEN_WELCOME_SCREENS 50
#define DELAY_AFTER_SCREENS 800
  
extern void L4_SetPlaySequenceOnlyOnce(void);  
  
extern const unsigned char _animinwc01[];          // Welcome Screen Animation 
extern const unsigned char _animinwc02[];
extern const unsigned char _animinwc03[];
extern const unsigned char _animinwc04[];
extern const unsigned char _animinwc05[];
extern const unsigned char _animinwc06[];
extern const unsigned char _animinwc07[];
extern const unsigned char _animinwc08[];
extern const unsigned char _animinwc09[];
extern const unsigned char _animinwc10[];

static UI_OBJECT WelcomeImage01 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  0,0,96,96,
  (BITMAP*)&_animinwc01
  }
};

static UI_OBJECT WelcomeImage02 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  0,0,96,96,
  (BITMAP*)&_animinwc02
  }
};

static UI_OBJECT WelcomeImage03 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  0,0,96,96,
  (BITMAP*)&_animinwc03
  }
};

static UI_OBJECT WelcomeImage04 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  0,0,96,96,
  (BITMAP*)&_animinwc04
  }
};

static UI_OBJECT WelcomeImage05 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  0,0,96,96,
  (BITMAP*)&_animinwc05
  }
};

static UI_OBJECT WelcomeImage06 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  0,0,96,96,
  (BITMAP*)&_animinwc06
  }
};

static UI_OBJECT WelcomeImage07 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  0,0,96,96,
  (BITMAP*)&_animinwc07
  }
};

static UI_OBJECT WelcomeImage08 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  0,0,96,96,
  (BITMAP*)&_animinwc08
  }
};

static UI_OBJECT WelcomeImage09 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  0,0,96,96,
  (BITMAP*)&_animinwc09
  }
};

static UI_OBJECT WelcomeImage10 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  0,0,96,96,
  (BITMAP*)&_animinwc10
  }
};

static UI_OBJECT TextVersion = 
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  12, 
  55, 
  25, 
  10, 
  SIG_COLOR_BLACK,  
  SIG_COLOR_TRANSPARENT, 
  0, 
  SIG_COLOR_TRANSPARENT, 
  SIG_FONT_13B_1, 
  "100"
  }
};

static UI_SCREEN Screen_Welcome01_Screen[] =
{
  &WelcomeImage01,
  NULL
};

static UI_SCREEN Screen_Welcome02_Screen[] =
{
  &WelcomeImage02,
  NULL
};

static UI_SCREEN Screen_Welcome03_Screen[] =
{
  &WelcomeImage03,
  NULL
};

static UI_SCREEN Screen_Welcome04_Screen[] =
{
  &WelcomeImage04,
  NULL
};

static UI_SCREEN Screen_Welcome05_Screen[] =
{
  &WelcomeImage05,
  NULL
};

static UI_SCREEN Screen_Welcome06_Screen[] =
{
  &WelcomeImage06,
  NULL
};

static UI_SCREEN Screen_Welcome07_Screen[] =
{
  &WelcomeImage07,
  NULL
};

static UI_SCREEN Screen_Welcome08_Screen[] =
{
  &WelcomeImage08,
  NULL
};

static UI_SCREEN Screen_Welcome09_Screen[] =
{
  &WelcomeImage09,
  NULL
};

static UI_SCREEN Screen_Welcome10_Screen[] =
{
  &WelcomeImage10,
  NULL
};

static UI_SCREEN Screen_Welcome11_Screen[] =
{
  &WelcomeImage10,
  &TextVersion,
  NULL
};

static UI_SEQUENCE Sequence_Welcome_Sequence[] =
  {
    Screen_Welcome01_Screen,
    Screen_Welcome02_Screen,
    Screen_Welcome03_Screen,
    Screen_Welcome04_Screen,
    Screen_Welcome05_Screen,
    Screen_Welcome06_Screen,
    Screen_Welcome07_Screen,
    Screen_Welcome08_Screen,
    Screen_Welcome09_Screen,
    Screen_Welcome10_Screen,
    Screen_Welcome11_Screen,
    NULL
  };

// ========================================================================== 
/**
 * \brief   GetPowerpackVersion
 *
 * \details This function gets the Power pack version and update on the active screen.
 *
 * \param   pVersion - Pointer to software version to be displayed at the end 
 *                     of the banner animation
 *
 * \return  None
 * ========================================================================= */
void WelcomeScreenShow(uint8_t *pVersion)
{
  snprintf(TextVersion.ObjText.Text, MAX_TEXT_SIZE, "%s", (char*)pVersion);
  L4_SetPlaySequenceOnlyOnce();
    
  L4_DmShowScreen_New(SCREEN_ID_WELCOME,
    DELAY_BETWEEN_WELCOME_SCREENS, Sequence_Welcome_Sequence);    
  
  OSTimeDly(DELAY_AFTER_SCREENS);
  
  /*  
  L3_WelcomeStaticScreen(0);
  OSTimeDly(DELAY_BETWEEN_WELCOME_SCREENS);
  L3_WelcomeStaticScreen(1);
  OSTimeDly(DELAY_BETWEEN_WELCOME_SCREENS);
  L3_WelcomeStaticScreen(2);
  OSTimeDly(DELAY_BETWEEN_WELCOME_SCREENS);
  L3_WelcomeStaticScreen(3);
  OSTimeDly(DELAY_BETWEEN_WELCOME_SCREENS);
  L3_WelcomeStaticScreen(4);
  OSTimeDly(DELAY_BETWEEN_WELCOME_SCREENS);
  L3_WelcomeStaticScreen(5);
  OSTimeDly(DELAY_BETWEEN_WELCOME_SCREENS);
  L3_WelcomeStaticScreen(6);
  OSTimeDly(DELAY_BETWEEN_WELCOME_SCREENS);
  L3_WelcomeStaticScreen(7);
  OSTimeDly(DELAY_BETWEEN_WELCOME_SCREENS);
  L3_WelcomeStaticScreen(8);
  OSTimeDly(DELAY_BETWEEN_WELCOME_SCREENS);
  L3_WelcomeStaticScreen(9);
  OSTimeDly(DELAY_BETWEEN_WELCOME_SCREENS);
  L3_WidgetTextDraw_New(&TextVersion.ObjText);
  L3_DispMemDevCopyToLCD();
  OSTimeDly(DELAY_BETWEEN_AFTER_SCREENS);
*/
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

