#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI template for Clamshell Error Screen
 *
 * \details Shows Clamshell error screen
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_ClamshellError.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_ClamshellError.h"
#include "UI_externals.h"

static UI_SCREEN Screen_ClamshellError_Screen[] =
{
  &YellowBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &LeftGreenBoxOfThree,
  &TriangleAboveRightPanel,
  &HandleWithGreenButtonImage,
  NULL
};

static UI_SEQUENCE Sequence_ClamshellError_Sequence[] =
  {
    Screen_ClamshellError_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Global Functions                           */
/******************************************************************************/
/* ===========================================================================*/
/**
 * \brief   Shows Clamshell error screen on Clamshell error
 *  
 * \details Shows Clamshell error screen
 *  
 * \param   < None >
 *  
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success 
 * ===========================================================================*/
DM_STATUS Gui_InsertClamshellErrorScreen(void)
{
  if(UI_ReturnToDefaultParameters())
{
  BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
  LeftGreenBoxOfThree.ObjText.BackColor = SIG_COLOR_YELLOW; 
  TriangleAboveRightPanel.ObjBitmap.X = 7;
  TriangleAboveRightPanel.ObjBitmap.Y = 22;
  AdjustPannelsVerticalPositions();
  L4_DmShowScreen_New(SCREEN_ID_CLAMSHELL_ERROR,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_ClamshellError_Sequence );
}
    return DM_STATUS_OK;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

