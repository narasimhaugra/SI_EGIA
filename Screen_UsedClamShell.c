#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is Used Clamshell Screen template
 *
 * \details This shows the Used Clamshell Screen when the Clamshell usage is expired. 
 *
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_UsedClamShell.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_UsedClamShell.h"
#include "UI_externals.h"

static UI_SCREEN Screen_UsedClamshell_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &LeftGreenBoxOfThree,
  &HandleWithGreenButtonImage,
  &TextOnLeftPanelBottom,
  NULL
};

static UI_SEQUENCE Sequence_UsedClamshell_Sequence[] =
  {
    Screen_UsedClamshell_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/**
 * \brief   Shows Used Clamshell Screen  
 *  
 * \details Shows Used Clamshell screen when Ckamshell usage is expired 
 *  
 * \param   < None >
 *  
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success 
 * ========================================================================== */
DM_STATUS Gui_Used_CS_Screen(void)
{
  if(UI_ReturnToDefaultParameters())
  {
   BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_BLACK;
   LeftGreenBoxOfThree.ObjText.BackColor = SIG_COLOR_WHITE;
   AdjustPannelsVerticalPositions();
   TextOnLeftPanelBottom.ObjText.TextColor = SIG_COLOR_RED;
   snprintf(TextOnLeftPanelBottom.ObjText.Text, MAX_TEXT_SIZE, "0");
   g_uiNumberForTextOnLeftPanelBottom = 0;
   L4_DmShowScreen_New(SCREEN_ID_USED_CLAMSHELL,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_UsedClamshell_Sequence );
  }
    return DM_STATUS_OK;
}

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

