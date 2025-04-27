#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI Adapter Error Screen for Platform
 *
 * \details Define Adapter Error screen and associated action methods.
 *          screen plays animation when adapter software update fails & 1 wire short.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_AdapterError.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_AdapterError.h"
#include "UI_externals.h"

static UI_SCREEN Screen_Adapter_ERROR_Screen[] =
{
  &WhiteBoxAround,
  &ThinGreenBoxAround,
  &BatteryImage,
  &BatteryProgressBar,
  &CenterGreenBoxOfThree,
  &HandleWithoutGreenButtonImage,
  &AdapterImage,
  &TriangleAboveRightPanel,
  NULL
 };


static UI_SEQUENCE Sequence_Adapter_ERROR_Sequence[] =
  {
    Screen_Adapter_ERROR_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Custom animation procedure
 *
 * \details This function runs the 'Adapter Check' animation when started and 
 *          calls Platform related Adapter Errors
 *
 * \param   None
 *
 * \return  None
 *
 * Note: 
 * ========================================================================== */
void Gui_AdapterErrorScreen (void)
{
 if(UI_ReturnToDefaultParameters())
 {
    ThinGreenBoxAround.ObjText.BackColor = SIG_COLOR_BLACK;
    
    CenterGreenBoxOfThree.ObjText.BackColor = SIG_COLOR_YELLOW;
    HandleWithoutGreenButtonImage.ObjBitmap.X = HandleWithGreenButtonImage.ObjBitmap.X;
    HandleWithoutGreenButtonImage.ObjBitmap.Y = HandleWithGreenButtonImage.ObjBitmap.Y;
    TriangleAboveRightPanel.ObjBitmap.X = CenterGreenBoxOfThree.ObjText.X;
    AdapterImage.ObjBitmap.Y = AdapterImage.ObjBitmap.Y + 3;
    
    L4_DmShowScreen_New(SCREEN_ID_ADAPTER_ERROR,UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_Adapter_ERROR_Sequence);  
 }
}

/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

