#ifndef  L4_DISPLAYMANAGER_H
#define  L4_DISPLAYMANAGER_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L4_DisplayManager
 * \{
 * \brief   Public interface for Display Manager module.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_DisplayManager.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "L3_GuiWidgets.h"
#include "ScreenIDs.h"  
//#include "Common.h"         /* Import common definitions such as types, etc. */

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define DISPLAY_REFRESH_INTERVAL    (70u)   /*! Display refresh interval */
#define DM_MAX_SCREENS              (50u)   /*! Maximum supported screens by display manager */
#define DM_DEFAULT_BRIGHTNESS       (10u)   /*! Default display brightness */
#define DM_SCREEN_REFRESH_AUTO      (true)  /*! The display is periodically updated if ‘true' */
#define INVALID_ID                  (0xFFu) /*! Invalid object id */
#define BAT_PB_X_POS  (51u)
#define BAT_PB_Y_POS  (7u)
#define BAT_PB_H_VAL  (36u)
#define BAT_PB_W_VAL  (7u)

#define BAT_IMAGE_X_POS   (45u)
#define BAT_IMAGE_Y_POS   (3u)
#define BAT_IMAGE_H_VAL   (48u)
#define BAT_IMAGE_W_VAL   (16u)
/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef bool (*DM_SCREEN_FUNCTION)(uint8_t ScreenId);      /*! Screen entry/exit handler provisioned to execute pre-screen
                                                               loading and post-screen unloading instructions */

typedef enum                            /*! Display Manager function call status codes */
{
    DM_STATUS_OK,                       /*! No error */
    DM_STATUS_INVALID_PARAM,            /*! Invalid parameter */
    DM_STATUS_ERROR,                    /*! Error */
    DM_STATUS_COUNT
} DM_STATUS;

typedef enum                            /*! Screen Lock */
{
    SCREEN_LOCK_OFF,                    /*! Screen is Not Locked */
    SCREEN_LOCK_PERMANENT,              /*! Screen is Permanently Locked */
    SCREEN_LOCK_TEMPORARY               /*! Screen is Temporarily Locked */
} SCREEN_LOCK;

typedef struct                          /*! Text object used in the screen definition */
{
    uint8_t Id;                         /*! Object identifier */
    GUI_WIDGET_TEXT Text;               /*! Text widget definition */
    bool Redraw;                        /*! Indicates if the object is modified and a redraw needed */
    bool Hide;                          /*! Indicates if the object should be shown/hidden */
} DM_OBJ_TEXT;

typedef struct                          /*! Image object used in the screen definition */
{
    uint8_t Id;                         /*! Object identifier */
    GUI_WIDGET_IMAGE Image;             /*! Image widget definition */
    bool Redraw;                        /*! Indicates if the object is modified and a redraw needed */
    bool Hide;                          /*! Indicates if the object should be shown/hidden */
} DM_OBJ_IMAGE;

typedef struct                          /*! Progress bar object used in the screen definition */
{
    uint8_t Id;                         /*! Object identifier */
    GUI_WIDGET_PROGRESS_BAR Progress;   /*! Progress bar widget definition */
    bool Redraw;                        /*! Indicates if the object is modified and a redraw needed */
    bool Hide;                          /*! Indicates if the object should be shown/hidden */
} DM_OBJ_PROGRESS;

typedef struct                          /*! Clip object used in the screen definition */
{
    uint8_t Id;                         /*! Object identifier */
    GUI_WIDGET_CLIP Clip;               /*! Clip widget definition */
    bool Redraw;                        /*! Indicates if the object is modified and a redraw needed */
    bool Hide;                          /*! Indicates if the object should be shown/hidden */
} DM_OBJ_CLIP;

typedef struct                          /*! Movie object used in the screen definition */
{
    uint8_t Id;                         /*! Object identifier */
    GUI_WIDGET_MOVIE Movie;             /*! Movie widget definition */
    bool Redraw;                        /*! Indicates if the object is modified and a redraw needed */
    bool Hide;                          /*! Indicates if the object should be shown/hidden */
} DM_OBJ_MOVIE;

typedef struct                          /*! Screen definition, essentially a collection of widgets */
{
    uint8_t Id;                         /*! Screen identifier */
    DM_OBJ_TEXT *pTextList;             /*! List of text objects  */
    DM_OBJ_IMAGE *pImageList;           /*! List of Image objects  */
    DM_OBJ_PROGRESS *pProgressList;     /*! List of Progress bar objects  */
    DM_OBJ_CLIP *pClipList;             /*! List of Clip objects  */
    DM_OBJ_MOVIE *pMovieList;           /*! List of Video objects  */
    DM_SCREEN_FUNCTION pPrepare;        /*! Function hook called before loading screen  */
    DM_SCREEN_FUNCTION pPeriodic;       /*! Function hook called every screen refresh tick */
    DM_SCREEN_FUNCTION pWindup;         /*! Function hook called after unloading(switching) screen  */
} DM_SCREEN;

typedef struct                           /*! Temp Screen attributes*/
{
   uint16_t  Periodinmsec;               /*! Time period * DM_REFRESH_TICKS for recursive call to the pPEriodic hook function.*/
   DM_SCREEN_FUNCTION pPeriodic;        /*! Function hook called periodically */
} DM_TEMP_SCREEN;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern DM_STATUS L4_DmInit(void);
//extern void L4_DmClear(void);
//extern DM_STATUS L4_DmScreen_Save(void);
//extern DM_STATUS L4_DmScreen_Restore(void);
extern void L4_DmDrawColor(uint32_t Color);
extern void L4_DmDrawCircle(int8_t x, int8_t y, int8_t Radius);
extern void L4_DmFillRectangle(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2);
//extern DM_STATUS L4_DmDrawBitmap(DM_OBJ_IMAGE *pImageObj);
extern bool L4_DmIsScreenLocked_New(void);
extern bool L4_DmScreenUnlockTemp_New(void);
extern DM_STATUS L4_DmShowScreen_New(SCREEN_ID ScreenID, uint32_t RefreshRate, UI_SEQUENCE* pSequence);
extern DM_STATUS L4_DmScreenLockUnLock_New(UI_SEQUENCE *pSequence, SCREEN_LOCK ScreenLock);
DM_STATUS L4_DmCurrentScreenLockUnLock_New(SCREEN_LOCK ScreenLock);
//extern DM_STATUS L4_DmProgressBarUpdateColors(uint8_t ProgressBarId, SIG_COLOR_PALETTE BackColor, SIG_COLOR_PALETTE ForeColor);
extern uint8_t GetScreenID(void);
//extern DM_STATUS L4_DmImageUpdate(uint8_t ImageId, BITMAP *pNewImage);
/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_DISPLAYMANAGER_H */

