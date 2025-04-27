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
 * \details Define Set Battery level on active screen and associated action methods.
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_SetBatteryLevel.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_SetBatteryLevel.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
extern const unsigned char _acBattery_100[];

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER            (LOG_GROUP_DISPLAY) /*! Log Group Identifier */
#define BAT_FULL_PERCENTAGE             (100u)              /*! Battery Full percentage value */
#define PBAR_ID_BATT_LEVEL              (1u)                /*! Progress bar Image ID */
#define TEXT_BACKGROUND                 (1u)
#define SUFFICIENT_BATTERY              (25u)

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                     /*! Image identfiers  */
{
    IMG_ID_BATT_LEVEL,           /*! Battery level id */
    IMG_ID_LAST                  /*! Last image id */
} IMG_ID;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/*! Battery case image List */
static DM_OBJ_IMAGE Bat_Image[] =
{
    /* Battery case image */
    {IMG_ID_BATT_LEVEL,   {43, 2, 48, 16, (BITMAP*)_acBattery_100}, false, false},
    {INVALID_ID,          {0,  0,  0,  0, NULL}, false, false}
};

/*! Progress bar object to indicate battery level */
static DM_OBJ_PROGRESS ProgressList[] =
{
    /* Battery Level*/
    {PBAR_ID_BATT_LEVEL,  { 51, 6, 35, 7, SIG_COLOR_BLACK, SIG_COLOR_GREEN, 0,100, 0}, false, false},
    {INVALID_ID,          { 0,  0, 0,  0, SIG_COLOR_BLACK, SIG_COLOR_TRANSPARENT, 0,100, 0}, false, false}
};

/*! Set Battery level Screen*/
DM_SCREEN BatLevelScreen =
{
    SCREEN_ID_SET_BATTERY_LEVEL,         /*! Screen ID */
    NULL,
    Bat_Image,                   /*! Battery case image List */
    ProgressList,                /*! battery level */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Set the Sufficient battery >25% level Screen in Green
 *
 * \details This function Set the sufficient battery level on active screen
 *
 * \param   ScreenId - Screen id to show battery lavel
 * \param   BatteryLevel - Battery Level Percentage to set
 *
 * \return  None
 * ========================================================================== */
void Gui_SetBatteryLevel(uint8_t ScreenId, uint8_t BatteryLevel)
{
    L4_DmShowScreen(&BatLevelScreen);    /*! This should be removed later for generic use */
    if (BatteryLevel >= SUFFICIENT_BATTERY)
    {
        BatteryLevel = (BAT_FULL_PERCENTAGE - BatteryLevel);
        L4_DmProgressBarUpdate(ScreenId, BatteryLevel);       /*! Set the battery level */
        L4_DmProgressBarHide(ScreenId, false);                /*! Hide Progressbar */
    }
    else
    {
      Log(REQ, " Battery Level <= 25% ");
    }
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

