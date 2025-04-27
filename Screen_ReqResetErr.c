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
 * \file    Screen_ReqResetErr.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_ReqResetErr.h"
#include "UI_externals.h"
  
extern UI_SEQUENCE Sequence_ResetErr_Sequence[];

/******************************************************************************/
/*                                 Local Functions                            */
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
void Show_ReqResetErrScreen(void)
{
  UI_ReturnToDefaultParameters();
    BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
    TriangleAboveRightPanel.ObjBitmap.X = 35;
    TriangleAboveRightPanel.ObjBitmap.Y = 12;
    HandleWithoutGreenButtonImage.ObjBitmap.X = 33;
    HandleWithoutGreenButtonImage.ObjBitmap.Y = 38;
    L4_DmShowScreen_New(SCREEN_ID_REQ_RESET_ERR,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_ResetErr_Sequence );
}

/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
