#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   Low Battery Screen (image #2) template
 *
 * \details Shows Low Battery Screen (image #2) 
 *
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_LowBattPR2.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_LowBattPR2.h"
#include "UI_externals.h"

UI_SCREEN Screen_LowBattPR2[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage_10,
  &TextLeftMiddle,
  &TextForX,
  &BigReloadImage,
  NULL
};
   
static UI_SEQUENCE Sequence_LowBattPR2_Sequence[] =
  {
    Screen_LowBattPR2,
    NULL
  };   

/* ========================================================================== */
/**
 * \brief   Shows Low Battery Screen (image #2)  
 *  
 * \details Shows Low Battery Screen (image #2)  
 *  
 * \param   < None >
 *  
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success 
 * ========================================================================== */
DM_STATUS Gui_LowBattProceduresRemainTwo_Screen(void)
{
  
    if(UI_ReturnToDefaultParameters())
    {
    BlackBoxInsideGreenBox_2.ObjText.BackColor = SIG_COLOR_GRAY;
    snprintf(TextLeftMiddle.ObjText.Text, MAX_TEXT_SIZE, "2");
    snprintf(TextForX.ObjText.Text, MAX_TEXT_SIZE, "x");
    L4_DmShowScreen_New(SCREEN_ID_LOW_BATT_PR2,
    UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_LowBattPR2_Sequence );
    }
    return DM_STATUS_OK;
}

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

