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
 * \file    Screen_AdapterCompatible.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_AdapterCompatible.h"
#include "UI_externals.h"

UI_SCREEN Screen_AdapterCompatible[] =
{   
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &HandleWithoutGreenButtonImage,
  &EGIAReloadImage,
  &EEAreloadImage,
  &GreenCircleImage_1,
  &GreenCircleImage_2,
  &TextRemCount,
  NULL
 };

static UI_SEQUENCE Sequence_AdapterCompatible_Sequence[] =
  {
    Screen_AdapterCompatible,
    NULL
  };  

/******************************************************************************/
/*                                 Global Functions                           */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Adapter Compatible Screen 
 *
 * \details Showing Adatper compatible screen 
 *
 * \param   ProcedureRemainCnt - Procedure Remaining Count
 *
 * \return  None
 *
 * ========================================================================== */
void Show_AdapterCompatibleScreen (uint16_t ProcedureRemainCnt)
{
    if(UI_ReturnToDefaultParameters())
    {
        BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_GRAY;
        BatteryProgressBar.ObjText.BackColor = SIG_COLOR_GRAY;
        HandleWithoutGreenButtonImage.ObjBitmap.X = 55;
        HandleWithoutGreenButtonImage.ObjBitmap.Y = 35;
        EGIAReloadImage.ObjBitmap.X = 25;
        EGIAReloadImage.ObjBitmap.Y = 35;

        L4_DmShowScreen_New(SCREEN_ID_ADAPT_COMPAT,

                            UI_SEQUENCE_DEFAULT_REFRESH_RATE, 
                            
                            Sequence_AdapterCompatible_Sequence );

        snprintf(TextRemCount.ObjText.Text, MAX_TEXT_SIZE, "%d", ProcedureRemainCnt);
    }
}


/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

