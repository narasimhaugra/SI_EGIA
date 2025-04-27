#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI template for Adapter Verification Screen two
 *
 * \details Define Adapter Verification - two of two screen and associated action methods.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_AdapterVerifyTwo.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_AdapterVerify.h"
#include "UI_externals.h"
  
static UI_SCREEN Screen_AdapterVerify2_Screen[] =
{
  &BlackBoxAround,
  &LeftGreenBoxOfThree,
  &CenterGreenBoxOfThree,
  &BatteryImage,
  &BatteryProgressBar,
  &HandleWithGreenButtonImage,
  &AdapterImage,
  &TextOnLeftPanelBottom,
  NULL
};

UI_SEQUENCE Sequence_AdapterVerify2_Sequence[] =
  {
    Screen_AdapterVerify2_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Show Adapter Verification - two of two screen
 *
 * \details This function updates the procedure count
 *          on the Adapter Verification - two of two screen
 *
 * \param   ProcedureCount - procedure counter
 *
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success
 * ========================================================================== */
DM_STATUS Gui_AdapterVerifyScreenTwo(uint16_t ProcedureCount)
{
    if(UI_ReturnToDefaultParameters())
    {
    g_uiNumberForTextOnLeftPanelBottom = ProcedureCount; // to adjust X-position
    snprintf(TextOnLeftPanelBottom.ObjText.Text, MAX_TEXT_SIZE, "%d", ProcedureCount);
    AdjustPannelsVerticalPositions();
    
    L4_DmShowScreen_New(SCREEN_ID_ADAPTER_VERIFY_TWO,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_AdapterVerify2_Sequence);
    }
    return DM_STATUS_OK;
}

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/

/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

