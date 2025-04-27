#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   Depleted Battery Screen template 
 *
 * \details Shows Depleted Battery Screen  
 *
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_DepletedBatt.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_DepletedBatt.h"
#include "UI_externals.h"

static UI_SCREEN Screen_DepletedBatt1_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox,
  &BatteryImage_0,
  NULL
};
   
static UI_SCREEN Screen_DepletedBatt2_Screen[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox,
  NULL
};   
   
static UI_SEQUENCE Sequence_DepletedBatt_Sequence[] =
  {
    Screen_DepletedBatt1_Screen,
    Screen_DepletedBatt2_Screen,
    NULL
  };

/* ========================================================================== */
/**
 * \brief   Shows Depleted Battery Screen   
 *  
 * \details Shows Depleted Battery Screen    
 *  
 * \param   < None >
 *  
 * \return  None
  * ========================================================================== */
void Gui_DepletedBatt_Screen(void)
{
  if(UI_ReturnToDefaultParameters())
  {
     BatteryImage_0.ObjBitmap.X = 24;
     BatteryImage_0.ObjBitmap.Y = 37;
     L4_DmShowScreen_New( SCREEN_ID_DEPLETED_BATT, 500, Sequence_DepletedBatt_Sequence );
  }
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

