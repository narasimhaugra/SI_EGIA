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
#include "Screen_AdapterUnsupported.h"
#include "UI_externals.h"
  
static UI_SCREEN Screen_AdapterUnsupported_1[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &HandleWithoutGreenButtonImage,
  &UnsupportedAdapterImage,
  &AdapterImage,
  NULL
};

static UI_SCREEN Screen_AdapterUnsupported_2[] =
{
  &WhiteBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &UnsupportedAdapterImage,
  &AdapterImage,
  NULL
};

static UI_SEQUENCE Sequence_AdapterUnsupported[] =
  {
    Screen_AdapterUnsupported_1,
    Screen_AdapterUnsupported_2,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ========================================================================== */

/**
 * \brief   Shows the Adapter Unsupported Screen 
 *  
 * \details This function shows the Adapter Unsupported Screen
 *  
 * \param   < None >
 *  
 * \return  < None >
  * ========================================================================== */
void Gui_AdapterUnsupported(void)
{
      
    if(UI_ReturnToDefaultParameters())
    {
      HandleWithoutGreenButtonImage.ObjBitmap.X = 8;
      HandleWithoutGreenButtonImage.ObjBitmap.Y = 39;
      AdapterImage.ObjBitmap.X = 60;
      AdapterImage.ObjBitmap.Y = 42;
      
      L4_DmShowScreen_New(SCREEN_ID_ADAPTER_UNSUPPORT, 500, Sequence_AdapterUnsupported);    
    }
   
}

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

