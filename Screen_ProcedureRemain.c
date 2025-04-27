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
 * \details Define  Procedure Remain screen and associated action methods. 
 *         
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_ProcedureRemain.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_ProcedureRemain.h"
#include "UI_externals.h"
  
static UI_SCREEN Screen_ProcedureRemain_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &HandleWithoutGreenButtonImage,
  &TextOnCenterPanelBottom_Bold,
  NULL
};

static UI_SEQUENCE Sequence_ProcedureRemain_Sequence[] =
  {
    Screen_ProcedureRemain_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ========================================================================== */

/**
 * \brief   Shows the Remain Procedure count on active screen 
 *  
 * \details This function shows the Number of procedures Remain on active screen.
 *  
 * \param   < None >
 *  
 * \return  DM_STATUS - status
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success 
 * ========================================================================== */
DM_STATUS Gui_ProcedureRemain(void)
{
    uint16_t ProcedureCount;
    ProcedureCount = 10;
    
    if(UI_ReturnToDefaultParameters())
    {
    g_uiNumberForTextOnCenterPanelBottom = ProcedureCount; // to adjust X-position
    HandleWithoutGreenButtonImage.ObjBitmap.X = 31;
    HandleWithoutGreenButtonImage.ObjBitmap.Y = 32;
    BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_GRAY;
    snprintf(TextOnCenterPanelBottom_Bold.ObjText.Text, MAX_TEXT_SIZE, "%d", ProcedureCount);
      
    L4_DmShowScreen_New(SCREEN_ID_PROCEDURE_REMAIN,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_ProcedureRemain_Sequence);    
    }
    return DM_STATUS_OK;
}

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

