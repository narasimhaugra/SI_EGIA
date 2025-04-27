#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI Reset Error Screen template
 *
 * \details This shows the Reset Error Screen.
 *
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_ResetErr.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_ResetErr.h"
#include "UI_externals.h"

static UI_SCREEN Screen_ResetErr_Screen[] =
{
  &YellowBoxAround,
  &BlackBoxInsideGreenBox_2,
  &TriangleAboveRightPanel,
  &HandleWithoutGreenButtonImage,
  &ErrorCircleImage,
  NULL
};

UI_SEQUENCE Sequence_ResetErr_Sequence[] =
  {
    Screen_ResetErr_Screen,
    NULL
  };

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/**
 * \brief   Shows Reset Error Screen
 *
 * \details Shows Reset Error Screen
 *
 * \param   < None >
 *
 * \return  None
  * ========================================================================== */
void Show_ResetErrScreen(void)
{
   if (!UI_DefaultParametersCreated())
   {
      UI_CreateDefaultParameters();
   }
   if(UI_ReturnToDefaultParameters())
   {
    BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
    TriangleAboveRightPanel.ObjBitmap.X = 35;
    TriangleAboveRightPanel.ObjBitmap.Y = 12;
    HandleWithoutGreenButtonImage.ObjBitmap.X = 33;
    HandleWithoutGreenButtonImage.ObjBitmap.Y = 38;
    L4_DmShowScreen_New(SCREEN_ID_RESET_ERR,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_ResetErr_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

