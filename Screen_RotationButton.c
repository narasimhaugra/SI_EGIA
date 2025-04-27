#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ==========================================================================*/
 /**
 *
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI Rotation Button Screen
 *
 * \details Defines the Implemenation of  Linear EGIA Rotation Enable-Disable Scre
 *          ens and associated action methods. 
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_RotationButton.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_RotationButton.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
  
/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                     /*! Text identfiers  */
{       
    TEXT_OUTER_BOX1,
    OUTER_BOX_TEXT,
    TEXT_OUTER_BOX2,
    TEXT_OUTER_BOX3,
    TEXT_OUTER_BOX4,
    OUTER_BOX1,
    OUTER_BOX2,
} EGIA_TEXT_ID;  

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
#define EGIA_RELOAD_LENGTH      ("60")

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/*! Text List to be drawn as part of animation */
DM_OBJ_TEXT Rotation_TextList[] = 
{
    /*! Text to show Reload Length Intelligent Screen - White BG */
    {OUTER_BOX1,        { 4,21, 88, 17, SIG_COLOR_BLACK, SIG_COLOR_WHITE,0, SIG_COLOR_TRANSPARENT, SIG_FONT_13B_1, ""}, true, false},
    /*! Text to show Reload Length Intelligent Screen - Purple */
    {OUTER_BOX2,        { 6,23, 84, 13, SIG_COLOR_BLACK, SIG_COLOR_PURPLE,0, SIG_COLOR_BLACK, SIG_FONT_13B_1, ""}, true, false}, 
    {OUTER_BOX_TEXT,   {73, 23, 16, 13, SIG_COLOR_BLACK, SIG_COLOR_PURPLE, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_13B_1, ""}, true, false},
    {TEXT_OUTER_BOX2,  {0,   0, 96, 96, SIG_COLOR_BLACK, SIG_COLOR_GREEN, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_13B_1, ""}, false, false},
    {TEXT_OUTER_BOX3,  {3,   3, 90, 90, SIG_COLOR_BLACK, SIG_COLOR_BLACK, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_20B_1, ""}, false, false},
    {INVALID_ID,  {0, 0, 0, 0, SIG_COLOR_YELLOW, SIG_COLOR_BLUE, 1, SIG_COLOR_YELLOW, SIG_FONT_13B_1, " "}, false}
};

/*! Text to update in Right side Rotation Count */
DM_OBJ_TEXT CountR_TextList[] = 
{
    /*! Text to show Reload Length Intelligent Screen - White BG */
    {OUTER_BOX1,        { 6,21, 85, 17, SIG_COLOR_BLACK, SIG_COLOR_WHITE,0, SIG_COLOR_TRANSPARENT, SIG_FONT_13B_1, ""}, true, false},
    /*! Text to show Reload Length Intelligent Screen - Purple */
    {OUTER_BOX2,        { 8,23, 81, 13, SIG_COLOR_BLACK, SIG_COLOR_PURPLE,0, SIG_COLOR_BLACK, SIG_FONT_13B_1, ""}, true, false}, 
    {OUTER_BOX_TEXT,   {73, 25, 15, 9, SIG_COLOR_BLACK, SIG_COLOR_PURPLE, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_13B_1, ""}, true, false},
    {TEXT_OUTER_BOX2,  {0,   0, 96, 96, SIG_COLOR_BLACK, SIG_COLOR_GREEN, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_13B_1, ""}, false, false},
    {TEXT_OUTER_BOX3,  {3,   3, 90, 90, SIG_COLOR_BLACK, SIG_COLOR_BLACK, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_13B_1, ""}, false, false},
    {TEXT_OUTER_BOX4,  {72, 61, 12, 10, SIG_COLOR_WHITE, SIG_COLOR_BLACK, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_20B_1, ""}, true, false},
    {INVALID_ID,  {0, 0, 0, 0, SIG_COLOR_YELLOW, SIG_COLOR_BLUE, 1, SIG_COLOR_YELLOW, SIG_FONT_13B_1, " "}, false}
};

/*! Text to update in Left side Rotation Count */
DM_OBJ_TEXT CountL_TextList[] = 
{
    /*! Text to show Reload Length Intelligent Screen - White BG */
    {OUTER_BOX1,        { 6,21, 85, 17, SIG_COLOR_BLACK, SIG_COLOR_WHITE,0, SIG_COLOR_TRANSPARENT, SIG_FONT_13B_1, ""}, true, false},
    /*! Text to show Reload Length Intelligent Screen - Purple */
    {OUTER_BOX2,        { 8,23, 81, 13, SIG_COLOR_BLACK, SIG_COLOR_PURPLE,0, SIG_COLOR_BLACK, SIG_FONT_13B_1, ""}, true, false}, 
  
    {OUTER_BOX_TEXT,   {73, 25, 15, 9, SIG_COLOR_BLACK, SIG_COLOR_PURPLE, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_13B_1, ""}, true, false},
    {TEXT_OUTER_BOX2,  {0,   0, 96, 96, SIG_COLOR_BLACK, SIG_COLOR_GREEN, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_13B_1, ""}, false, false},
    {TEXT_OUTER_BOX3,  {3,   3, 90, 90, SIG_COLOR_BLACK, SIG_COLOR_BLACK, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_13B_1, ""}, false, false},
    {TEXT_OUTER_BOX4,  {12, 61, 12, 10, SIG_COLOR_WHITE, SIG_COLOR_BLACK, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_20B_1, ""}, true, false},
    {INVALID_ID,  {0, 0, 0, 0, SIG_COLOR_YELLOW, SIG_COLOR_BLUE, 1, SIG_COLOR_YELLOW, SIG_FONT_13B_1, " "}, false}
};

/*! Progress bar object to indicate battery level */
DM_OBJ_PROGRESS ProgressBar[] =
{
    {ADAPTER_TEXT_BATT_LEVEL, { BAT_PB_X_POS, BAT_PB_Y_POS, BAT_PB_H_VAL, BAT_PB_W_VAL, SIG_COLOR_BLACK, SIG_COLOR_GREEN, 0,100, 0}, false, false},  //50, 5, 37,  8
    {INVALID_ID, { 0,  0, 0,  0, SIG_COLOR_BLACK, SIG_COLOR_DARKGREEN, 0,100, 0}, false}
};

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ===========================================================================*/
 /**
 *
 * \brief   This function updatse the Reload Length as part of animation on active screen
 *
 * \details This function update the Reload Length 
 *
 * \param   pRB_Screen - pointer to screen
 *
 * \return  None
 *
* ==========================================================================*/
void Gui_UpdateReloadLength(DM_SCREEN *pRB_Screen)
{
    uint8_t String[MAX_TEXT_SIZE];  
    pRB_Screen->pTextList->Hide = false;
    pRB_Screen->pTextList->Redraw = true;
    sprintf((char *)String, (char *)EGIA_RELOAD_LENGTH);
    L4_DmTextUpdate(OUTER_BOX_TEXT, String);
    L4_DmTextHide(OUTER_BOX1, false);
    L4_DmTextHide(OUTER_BOX2, false);
}

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

