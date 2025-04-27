#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI Handle EOL Screen template
 *
 * \details This shows the Handle EOL Screen when the Handle Remain Procedures becomes zero. 
 *
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_HandleEndOfLife.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_HandleEndOfLife.h"
#include "UI_externals.h"

static UI_SCREEN Screen_HandleEndOfLife_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &YellowWrenchImage,
  &HandleWithoutGreenButtonImage,
  &TextOnCenterPanelBottom,
  NULL
};

static UI_SEQUENCE Sequence_HandleEndOfLife_Sequence[] =
  {
    Screen_HandleEndOfLife_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/**
 * \brief   Shows Handle EOL Screen when Handle Remain procedures becomes zero 
 *  
 * \details Shows Handle End of Life screen 
 *  
 * \param   < None >
 *  
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success 
 * ========================================================================== */
DM_STATUS Gui_HandleEndOfLife_Screen(void)
{
  if(UI_ReturnToDefaultParameters())
  {
    BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_GRAY;
    BatteryProgressBar.ObjText.BackColor = SIG_COLOR_GRAY;
    HandleWithoutGreenButtonImage.ObjBitmap.X = 31;
    HandleWithoutGreenButtonImage.ObjBitmap.Y = 30;
    L4_DmShowScreen_New(SCREEN_ID_HANDLE_END_OF_LIFE,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_HandleEndOfLife_Sequence );
  }
    return DM_STATUS_OK;
}

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

