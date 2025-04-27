#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI template for Adapter Verification Screen one
 *
 * \details Define Adapter Verification - one of two screen and associated action methods.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_AdapterVerifyOne.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_AdapterVerify.h"
#include "UI_externals.h"
  
static UI_SCREEN Screen_AdapterVerify1_Screen[] =
{
  &BlackBoxAround,
  &LeftGreenBoxOfThree,
  &BatteryImage,
  &BatteryProgressBar,
  &HandleWithGreenButtonImage,
  &AdapterImage,
  &TextOnLeftPanelBottom,
  NULL
};

static UI_SEQUENCE Sequence_AdapterVerify1_Sequence[] =
  {
    Screen_AdapterVerify1_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Global Functions                           */
/******************************************************************************/
/* ===========================================================================*/
/**
 * \brief   Adapter Verification - one of two
 *
 * \details This function updates the procedure count
 *          on the Adapter Verification - one of two screen
 *
 * \param   ProcedureCount - procedure counter
 *
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success
 * ===========================================================================*/
DM_STATUS Gui_AdapterVerifyScreenOne(uint16_t ProcedureCount)
{
  if(UI_ReturnToDefaultParameters())
  {
    g_uiNumberForTextOnLeftPanelBottom = ProcedureCount; // to adjust X-position
    snprintf(TextOnLeftPanelBottom.ObjText.Text, MAX_TEXT_SIZE, "%d", ProcedureCount);
    AdjustPannelsVerticalPositions();
    
    L4_DmShowScreen_New(SCREEN_ID_ADAPTER_VERIFY_ONE,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_AdapterVerify1_Sequence);
  }
    return DM_STATUS_OK;
}

/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

