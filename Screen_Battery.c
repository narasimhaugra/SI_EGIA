#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI Battery level set Screen
 *
 * \details Shows Battery charge level on active screen and associated action methods.
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_Battery.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Screen_Battery.h"
#include "L4_DisplayManager.h"
#include "L3_GuiWidgets.h"
#include "L3_DispPort.h"
#include "Signia_ChargerManager.h"
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
extern const unsigned char _acBattery_0[];
extern const unsigned char _acBattery_10[];
extern const unsigned char _acBattery_100[];

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER            ( LOG_GROUP_DISPLAY)
#define NORMAL_BATTERY_LEVEL            (25u)   /*! Battery charge >25% */
#define LOWLEVEL_BATTERY                (10u)   /*! Low level Battery >9% and <25% */
#define INSUFFICIENT_BATTERY            (9u)    /*! Battery Charge Level <=9% */
#define BAT_FULL_PERCENTAGE             (100u)  /*! Battery Full percentage value*/


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

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Shows battery image on the active screen (Progress Bar attached-Run time)
 *
 * \details This function shows battery image
 *
 * \param   BatteryLevel - Battery Level to be updated
 * \param   ScreenRefresh - Screen Refresh on every Battery Read Event
 *
 * \return  None
 *
 * ========================================================================== */
void Gui_BatteryImageUpdate(uint32_t BatteryLevel, bool ScreenRefresh)
{
    static bool level;
    static bool Low;
    static bool Insuf;
    uint16_t Color;
    uint8_t ScreenId;

    ScreenId = 0;
    ScreenId = GetScreenID();
    if ((SCREEN_ID_INSERT_CLAMSHELL == ScreenId) || (SCREEN_ID_ADAPT_COMPAT == ScreenId) || (SCREEN_ID_PROCEDURE_REMAIN == ScreenId))
    {
        Color = SIG_COLOR_GRAY;       /*! Set the battery level */
    }
    else
    {
        Color = SIG_COLOR_BLACK;
    }

    if ((BatteryLevel > NORMAL_BATTERY_LEVEL) && ((!level) || (ScreenRefresh)))
    {
        level = true;
        L3_WidgetPaintWindow((SIG_COLOR_PALETTE)Color, 43, 3, 50, 20);
        L4_DmProgressBarUpdateColors(1, (SIG_COLOR_PALETTE)SIG_COLOR_GREEN,(SIG_COLOR_PALETTE)Color);       /*! Set the battery level */
        L4_DmImageUpdate(1,(BITMAP *)_acBattery_100);
    }
    else if (((BatteryLevel <= NORMAL_BATTERY_LEVEL) && (BatteryLevel > LOWLEVEL_BATTERY)) && ((!Low) || (ScreenRefresh)))
    {
        L3_WidgetPaintWindow((SIG_COLOR_PALETTE)Color, 43, 3, 50, 20);
        L4_DmProgressBarUpdateColors(1, (SIG_COLOR_PALETTE)SIG_COLOR_YELLOW,(SIG_COLOR_PALETTE)Color);       /*! Set the battery level */
        L4_DmImageUpdate(1,(BITMAP *)_acBattery_10);
        Low = true;
    }
    else if(((BatteryLevel <= LOWLEVEL_BATTERY) && (BatteryLevel > 0)) && ((!Insuf) || (ScreenRefresh)))
    {
        L3_WidgetPaintWindow((SIG_COLOR_PALETTE)Color, 43, 3, 50, 20);
        L4_DmProgressBarUpdateColors(1, (SIG_COLOR_PALETTE)SIG_COLOR_RED,(SIG_COLOR_PALETTE)Color);       /*! Set the battery level */
        L4_DmImageUpdate(1,(BITMAP *)_acBattery_0);
        Insuf = true;
    }
    L4_DmProgressBarUpdate(1,(uint8_t)(BAT_FULL_PERCENTAGE-BatteryLevel));
}

/* ========================================================================== */
/**
 * \brief   Update battery image on the active screen (During First time Call)
 *
 * \details This function Updates battery image
 *
 * \param   Color - Back ground color for the Progress Bar
 *
 * \return  None
 *
 * ========================================================================== */
void UpdateBatteryImage(uint16_t Color)
{
    static bool Refresh;
    static bool RefreshLow;
    static bool RefreshInsuf;
    uint8_t BatteryLevel;

    Signia_ChargerManagerGetBattRsoc(&BatteryLevel);
    if (BatteryLevel >= NORMAL_BATTERY_LEVEL)
    {
        if (!Refresh)
        {
            L3_WidgetPaintWindow((SIG_COLOR_PALETTE)Color, 43, 3, 50, 20);
            Refresh = true;
        }
        L4_DmImageUpdate(1,(BITMAP *)_acBattery_100);
        L4_DmProgressBarUpdateColors(1, (SIG_COLOR_PALETTE)SIG_COLOR_GREEN,(SIG_COLOR_PALETTE)Color);       /*! Set the battery level */
        L4_DmProgressBarUpdate(1, BAT_FULL_PERCENTAGE-BatteryLevel);

    }
    else if ((BatteryLevel < NORMAL_BATTERY_LEVEL) && (BatteryLevel >= LOWLEVEL_BATTERY))
    {
        if (!RefreshLow)
        {
            L3_WidgetPaintWindow((SIG_COLOR_PALETTE)Color, 43, 3, 50, 20);
            RefreshLow = true;
        }
        L4_DmProgressBarUpdateColors(1, (SIG_COLOR_PALETTE)SIG_COLOR_YELLOW,(SIG_COLOR_PALETTE)Color);
        L4_DmImageUpdate(1,(BITMAP *)_acBattery_10);
        L4_DmProgressBarUpdate(1, BAT_FULL_PERCENTAGE-BatteryLevel);/*! Set the battery level */
    }
    else
    {
        if (!RefreshInsuf)
        {
            L3_WidgetPaintWindow((SIG_COLOR_PALETTE)Color, 43, 3, 50, 20);
            RefreshInsuf = true;
        }
        L4_DmImageUpdate(1,(BITMAP *)_acBattery_0);
        L4_DmProgressBarUpdateColors(1, (SIG_COLOR_PALETTE)SIG_COLOR_RED,(SIG_COLOR_PALETTE)Color);       /*! Set the battery level */
        L4_DmProgressBarUpdate(1, BAT_FULL_PERCENTAGE-BatteryLevel);
    }
}

/* ========================================================================== */
/**
 * \brief   Update battery Level for the Animated Screens
 *
 * \details This function Updates battery Level of the Progress Bar
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void UpdateBatteryLevel(void)
{
    uint8_t BatteryLevel;

    Signia_ChargerManagerGetBattRsoc(&BatteryLevel);
    L4_DmProgressBarUpdate(1, BAT_FULL_PERCENTAGE-BatteryLevel);
}

/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

