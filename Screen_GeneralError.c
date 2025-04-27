#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   This is a GUI General Error Screen
 *
 * \details Define General Error screen and associated action methods.
 *          screen Display General Error screen when Handle has issue.
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_GeneralError.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_GeneralError.h"
#include "Screen_Battery.h"
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
extern const unsigned char _acHandleBM90[];
extern const unsigned char _acAdapterBM[];
extern const unsigned char _acBattery_100[];
extern const unsigned char _acAlert_Error[];

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define MAIN_SCREEN_PROGRESS_BATTERY (1u)

#define SCREEN_WIDTH          96
#define SCREEN_HEIGHT         96
#define SCREEN_MAX_PIXEL      95
#define BORDER_WIDTH          4

#define BORDER_LEFT_X1        (BORDER_WIDTH - 1)
#define SWIM_LANE_1_X0        (BORDER_LEFT_X1 + 2)
#define SWIM_LANE_1_Y0        SWIM_LANE_TOP
#define SWIM_LANE_WIDTH       (26)
#define SWIM_LANE_HEIGHT      (SCREEN_MAX_PIXEL - BORDER_WIDTH -1 - SWIM_LANE_TOP)
#define SWIM_LANE_TOP         (37 + 2)

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                    ///< Text identfiers
{
    HANDLE_TEXT_OUTER_BOX1,
    HANDLE_TEXT_OUTER_BOX2,
    HANDLE_TEXT_OUTER_BOX3,
    TEXT_ID_LAST,
} HANDLE_ERROR_TEXT;

/* Handle Error Screen Image IDs */
typedef enum
{
    HANDLE_SCREEN_ERROR_NONE,
    MAIN_SCREEN_IMAGE_BATTERY,
    HANDLE_GEN_ERROR_SCREEN_IMG_HANDLE90,
    HANDLE_GEN_ERROR_SCREEN,
    SCREEN_ADPT_CAL_IMG_LAST,
} HANDLE_ERROR_IMG;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/*! List of Images to be drawn as part of Screen */
static DM_OBJ_IMAGE ImageList[] =
{
    {MAIN_SCREEN_IMAGE_BATTERY,  {BAT_IMAGE_X_POS, BAT_IMAGE_Y_POS, BAT_IMAGE_H_VAL, BAT_IMAGE_W_VAL,   (BITMAP*)_acBattery_100},    false, false},
    {HANDLE_GEN_ERROR_SCREEN_IMG_HANDLE90,{ 7, 37, 31, 41, (BITMAP*)_acHandleBM90}, false, false},
    /* Handle Error */
    {HANDLE_GEN_ERROR_SCREEN, {7, 20, 25, 14, (BITMAP*)_acAlert_Error}, true, false},
    /* End of image list */
    {INVALID_ID, {0, 0, 0, 0, NULL}, false, false}
};

/* Text object */
static DM_OBJ_TEXT TextList[] =
{
    /*! Yellow color Border*/
    {HANDLE_TEXT_OUTER_BOX1,  {0, 0, 96, 96, SIG_COLOR_TRANSPARENT, SIG_COLOR_YELLOW, 1, SIG_COLOR_TRANSPARENT, SIG_FONT_20B_1, ""}, true,false},
    /*! Black Background !*/
    {HANDLE_TEXT_OUTER_BOX2,  {3, 3, 90, 90, SIG_COLOR_BLACK, SIG_COLOR_BLACK, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_20B_1, ""}, true,false},
    /*! Yello Background for Power handle */
    {HANDLE_TEXT_OUTER_BOX3, {5, 38, 28, 54, SIG_COLOR_BLACK, SIG_COLOR_YELLOW, 0, SIG_COLOR_BLACK, SIG_FONT_20B_1, ""}, false,false},
    {INVALID_ID,        {0, 0, 0, 0, SIG_COLOR_YELLOW, SIG_COLOR_BLUE, 1, SIG_COLOR_YELLOW, SIG_FONT_13B_1, ""}, false}
};

/*! Dummy clip to trigger periodic processing. \todo: Include a periodic function in screen object? */
static DM_OBJ_CLIP ClipList[] =
{
    {INVALID_ID, NULL, false, false}
};

/* Progress bar object to indicate battery level */
static DM_OBJ_PROGRESS ProgressList[] =
{
    {MAIN_SCREEN_PROGRESS_BATTERY, { BAT_PB_X_POS, BAT_PB_Y_POS, BAT_PB_H_VAL, BAT_PB_W_VAL, SIG_COLOR_BLACK, SIG_COLOR_GREEN, 0,85, 0}, false, false},
    {INVALID_ID, { 0,  0, 0,  0, SIG_COLOR_BLACK, SIG_COLOR_DARKGREEN, 0,85, 0}, false}
};

/*! Handle General Error Screen definition */
DM_SCREEN GeneralErrorScreen =
{
    SCREEN_ID_HANDLE_ERROR,
    TextList,
    ImageList,
    ProgressList,
    ClipList,
    NULL,
    NULL,
    NULL,
    NULL
};

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

