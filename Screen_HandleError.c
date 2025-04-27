#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a Handle Error screen
 *
 * \details Define Handle Error Screen and associated action methods.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_HandleError.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
//#include "L4_DisplayManager.h"
#include "L3_DispPort.h"
#include "L3_GuiWidgets.h"  
#include "Screen_HandleError.h"
#include "UI_externals.h"
  
extern const unsigned char _acAlert_Error[];
extern const unsigned char _acPowerPack[];
extern const unsigned char _acErrorCircle[];
static GUI_WIDGET_IMAGE CurrentImage;  
  
/*
static UI_SCREEN Screen_HandleError_Screen[] =
{
  &WhiteBoxAround,
  &TriangleAboveRightPanel,
  &HandleWithoutGreenButtonImage,
  &ErrorCircleImage,
  NULL
};

static UI_SEQUENCE Sequence_HandleError_Sequence[] =
  {
    Screen_HandleError_Screen,
    NULL
  };
*/
/******************************************************************************/
/*                                 Global Functions                           */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Display Handle Error Screen
 *  
 * \details This function displays the Handle Error Screen and Permanently Locks the active screen
 *  
 * \param   < None >
 *  
 * \return  None  
 *  
 * ========================================================================== */
void Gui_HandleErrorScreen(void)
{
   //if(UI_ReturnToDefaultParameters())
   //{
    /* 
    WhiteBoxAround.ObjText.BackColor = SIG_COLOR_GRAY;
    TriangleAboveRightPanel.ObjBitmap.X = 35;
    TriangleAboveRightPanel.ObjBitmap.Y = 16;
    HandleWithoutGreenButtonImage.ObjBitmap.X = 33;
    HandleWithoutGreenButtonImage.ObjBitmap.Y = 38;
    */
    // Unlock if Screen is Temporarily locked
    //L4_DmScreenLockUnLock_New(Sequence_HandleError_Sequence, SCREEN_LOCK_OFF);
    L4_DmCurrentScreenLockUnLock_New(SCREEN_LOCK_OFF);
    // Display HANDLE_ERROR_SCREEN 
    //L4_DmShowScreen_New(SCREEN_ID_HANDLE_ERROR,
    //UI_SEQUENCE_DEFAULT_REFRESH_RATE, Sequence_HandleError_Sequence );
    
    L3_DisplayOn(true);
    L3_DispClear();
    
    L3_DispSetColor(DISP_COLOR_YELLOW);
    L3_DispFillRect(DISPXPOS,DISPXPOS,DISPYPOS,DISPYPOS);
    
    L3_DispSetColor(DISP_COLOR_GRAY);
    L3_DispFillRect(3,3,91,91);

    CurrentImage = (GUI_WIDGET_IMAGE){35, 15, 25, 14,(BITMAP*)_acAlert_Error};
    L3_WidgetImageDraw(&CurrentImage);

    CurrentImage = (GUI_WIDGET_IMAGE){33, 38, 28, 38,(BITMAP*)_acPowerPack};
    L3_WidgetImageDraw(&CurrentImage);
    
    CurrentImage = (GUI_WIDGET_IMAGE){22, 32, 52, 48,(BITMAP*)_acErrorCircle};
    L3_WidgetImageDraw(&CurrentImage);
    
    L3_DispMemDevCopyToLCD();  
    
    // Lock the screen Permanently to display HANDLE ERROR SCREEN always 
    //L4_DmScreenLockUnLock_New(Sequence_HandleError_Sequence,SCREEN_LOCK_PERMANENT);
    L4_DmCurrentScreenLockUnLock_New(SCREEN_LOCK_PERMANENT);
//}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

