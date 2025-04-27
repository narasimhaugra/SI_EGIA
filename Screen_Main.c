#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI Adapter Check Screen
 *
 * \details Define Adapter Check screen and associated action methods. 
 *          screen plays animation when adapter connection has issue.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_AdapterCheck.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/
#define MAIN_SCREEN_IMAGE_BATTERY        (0u)
#define MAIN_SCREEN_PROGRESS_BATTERY     (0u)
#define MAIN_SCREEN_BORDER         (0u)

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
extern const unsigned char _acBattery_100[];

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/*! List of Images to be drawn as part of animation */
static DM_OBJ_IMAGE ImageList[] = 
{ 
    /* Battery case image */
    {MAIN_SCREEN_IMAGE_BATTERY,{43, 6, 48, 16, (BITMAP*)_acBattery_100}, false, false},
    /* End of image list */
    {INVALID_ID, {0, 0, 0, 0, NULL}, false, false}
};

///* Progress bar object to indicate battery level */
static DM_OBJ_PROGRESS ProgressList[] =
{
    {MAIN_SCREEN_PROGRESS_BATTERY, { 50, 9, 37,  8, SIG_COLOR_BLACK, SIG_COLOR_GREEN, 0,85, 0}, false, false},  
    {INVALID_ID, { 0,  0, 0,  0, SIG_COLOR_BLACK, SIG_COLOR_DARKGREEN, 0,85, 0}, false}
};

/* Text object */
static DM_OBJ_TEXT TextList[] =
{    
    {MAIN_SCREEN_BORDER,  {0, 0, 96, 96, SIG_COLOR_TRANSPARENT, SIG_COLOR_BLACK, 4, SIG_COLOR_WHITE, SIG_FONT_20B_1, ""}, false, false},
    {INVALID_ID,  {0, 0, 0, 0, SIG_COLOR_YELLOW, SIG_COLOR_BLUE, 1, SIG_COLOR_YELLOW, SIG_FONT_13B_1, ""}, false}
};
/*! main background screen */
DM_SCREEN ScreenStaticMain =
{
    SCREEN_ID_MAIN,
    TextList,
    ImageList,
    ProgressList,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/

/******************************************************************************/
/*                                 Global Functions                           */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Battery level update
 *  
 * \details This function updates the RSOC level in the main screen
 *  
 * \param   BatteryLevel - Battery level (percentage) to be displayed.
 *  
 * \return  None
 *  
 * ========================================================================== */
void ScreenMainSetBatteryLevel(uint8_t BatteryLevel)
{
    static uint16_t PrevBatteryLevel;
    
    BatteryLevel = (BatteryLevel> 100) ? 100: BatteryLevel;
    
    if (PrevBatteryLevel != BatteryLevel)
    {
        L4_DmProgressBarUpdate(MAIN_SCREEN_PROGRESS_BATTERY, 100 - BatteryLevel);
        PrevBatteryLevel = BatteryLevel;
    }
    return;
}

/* ========================================================================== */
/**
 * \brief   Unlocks the Temporary Lock for UsedReload/MULU EOL Screen
 *
 *
 * \param   < None >
 *
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK    - On success
 *
 * ========================================================================== */
bool ScreenLockStatus(void)
{
    bool Status;
       
    // True if Permanent Locked
    if ( L4_DmScreenUnLock() )
    {
        Status = false;
    }
    else // False if Temporary Locked
    {
        Status = true;
    }
    return Status;
}

/**
 * \}  <If using addtogroup above>
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

