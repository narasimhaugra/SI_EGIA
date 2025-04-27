#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   Adapter Request screen
 *
 * \details Adapter Request screen and associated action methods.
 *          screen plays animation request user to connect Adapter.
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_AdapterRequest.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_AdapterRequest.h"
//#include "L4_ChargerManager.h"
#include "UI_externals.h"

static uint8_t YSHIFT = 6;


static UI_SCREEN Screen_AdapterRequest_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideWhiteBox,
  &LeftGreenBoxOfThree,
  &BatteryImage,
  &BatteryProgressBar,
  &HandleWithGreenButtonImage,
  &AdapterImage,
  &GreenArrowLeftImage,
  &TextOnLeftPanelBottom,
  NULL
};

static UI_SEQUENCE Sequence_AdapterRequest_Sequence[] =
  {
    Screen_AdapterRequest_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Adapter Request screen template test.
 *
 * \details Executes Adapter Request screen template test.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void Show_AdapterRequestScreen(uint16_t ProcedureRemaining)
{
  if(UI_ReturnToDefaultParameters())
  {
    LeftGreenBoxOfThree.ObjText.Y = LeftGreenBoxOfThree.ObjText.Y - YSHIFT;
    LeftGreenBoxOfThree.ObjText.Height = LeftGreenBoxOfThree.ObjText.Height + YSHIFT;
    HandleWithGreenButtonImage.ObjBitmap.Y = HandleWithGreenButtonImage.ObjBitmap.Y - YSHIFT;
    AdapterImage.ObjBitmap.X = 62;
    AdapterImage.ObjBitmap.Y = AdapterImage.ObjBitmap.Y - YSHIFT + 1;
    AdapterImage.pFunction = NULL;
    GreenArrowLeftImage.ObjBitmap.X = 40;
    GreenArrowLeftImage.ObjBitmap.Y = 40;  
    L4_DmShowScreen_New(SCREEN_ID_ADAPTER_REQUEST, 400,
    Sequence_AdapterRequest_Sequence);
    snprintf(TextOnLeftPanelBottom.ObjText.Text, MAX_TEXT_SIZE, "%d", ProcedureRemaining);
   }
}

/**
 * \} Doxygen group end tag
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

