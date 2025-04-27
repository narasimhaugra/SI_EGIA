#ifndef L3_DISPPORT_H
#define L3_DISPPORT_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_Disp
 * \{
 * \brief Header file for Level 3 Display Port Module.
 *        This module has API's to initialize the Display and API's to draw shapes
 *        or images on display.
 *
 * \copyright  Copyright 2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * \file  L3_DispPort.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         ///< Import common definitions such as types, etc
#include <GUI.h>
#include <GUIDRV_FlexColor.h>

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
// defines for color values.
#define DISP_COLOR_BLACK        (0x000000u)
#define DISP_COLOR_WHITE        (0xFFFFFFu)
#define DISP_COLOR_YELLOW       (0xF4F425u)
#define DISP_COLOR_RED          (0xFF0000u)
#define DISP_COLOR_GREEN        (0x00CD00u)
#define DISP_COLOR_LIGHT_GREEN  (0x40FF40u)
#define DISP_COLOR_BLUE         (0x0000FFu)
#define DISP_COLOR_DARKGREEN    (0x124412u)
#define DISP_COLOR_GRAY         (0x92979Bu)
#define DISP_COLOR_GRAY_ALT     (0x878787u)
#define DISP_COLOR_TAN          (0xFF8635u)
#define DISP_COLOR_PURPLE       (0xB200FFu)
#define DISP_COLOR_PINK         (0xCC00CCu)
#define DISP_COLOR_PINK_ALT     (0xE57EE8u)
#define DISP_COLOR_CYAN         (0x00FFFFu)
#define DISP_COLOR_TRANSPARENT  (0xFF00DCu)
  
#define DISPXPOS  (0u)
#define DISPYPOS  (96u)

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef GUI_RECT  DISP_RECT;         ///< Structure for coordinates for rectangle.
typedef GUI_POINT DISP_POINT;        ///< Structure for coordinates for point.

/*! \enum FONT_TYPE
 * Font types being used, can be expanded as needed.
 */
typedef enum                         
{
    FONT_13B_1,                     ///< Corresponds to font name GUI_FONT_13B_1. Refer to uC-GUI user manual for more details.
    FONT_20B_1,                     ///< Corresponds to font name GUI_FONT_20B_1. Refer to uC-GUI user manual for more details.
} FONT_TYPE;


/*! \enum FONT_TYPE
 * Display Initialization modes
 */
typedef enum 
{
    DISP_INIT   = 0,                    ///< Enum for Initialization.
    DISP_REINIT = 1                     ///< Enum for Re-Initialization.
} DISP_INIT_MODE;

/*! \enum FONT_TYPE
 * enum for Battery level in percentage
 */
typedef enum                         
{ 
    DISP_BAT_LEVEL_0,
    DISP_BAT_LEVEL_10,
    DISP_BAT_LEVEL_25, 
    DISP_BAT_LEVEL_40,
    DISP_BAT_LEVEL_50,
    DISP_BAT_LEVEL_55,
    DISP_BAT_LEVEL_70,
    DISP_BAT_LEVEL_75,
    DISP_BAT_LEVEL_85,
    DISP_BAT_LEVEL_100,
    DISP_BAT_LEVEL_LAST
} DISP_BAT_LEVEL;

/*! \enum FONT_TYPE
 * enum for WiFi level
 */
typedef enum                    
{
    DISP_WIFI_LEVEL_0,
    DISP_WIFI_LEVEL_25,
    DISP_WIFI_LEVEL_50,
    DISP_WIFI_LEVEL_75,
    DISP_WIFI_LEVEL_100,
    DISP_WIFI_LEVEL_LAST
} DISP_WIFI_LEVEL;

/*! \enum FONT_TYPE
 * enum for Arrow direction
 */
typedef enum                        
{
    DISP_ARROW_LEFT,
    DISP_ARROW_RIGHT,
    DISP_ARROW_UP,
    DISP_ARROW_DOWN,
    DISP_ARROW_LAST
} DISP_ARROW_DIRECTION;

/*! \enum FONT_TYPE
 * enum for drawing method used for drawing the bitmap.
 */
typedef enum                        
{
    DISP_BITMAP_DRAW_METHOD_RLE4,
    DISP_BITMAP_DRAW_METHOD_RLE8,
    DISP_BITMAP_DRAW_METHOD_RLE16,
    DISP_BITMAP_DRAW_METHOD_RLEM16,
    DISP_BITMAP_DRAW_METHOD_RLE32,
    DISP_BITMAP_DRAW_METHOD_RLEAlpha,
    DISP_BITMAP_DRAW_METHOD_555,
    DISP_BITMAP_DRAW_METHOD_M555,
    DISP_BITMAP_DRAW_METHOD_565,
    DISP_BITMAP_DRAW_METHOD_M565
} DISP_BITMAP_DRAW_METHOD;

/*! \enum FONT_TYPE
 *   Display Port API Status.
 */
typedef enum                                      
{
    DISP_PORT_STATUS_OK,                 ///< No error
    DISP_PORT_STATUS_INVALID_STATE,      ///< Invalid state.
    DISP_PORT_STATUS_ERROR               ///< Read/Write/Initialization Error.
} DISP_PORT_STATUS;

typedef struct 
{                                   /// Structure hold the details of a bitmap to be drawn.
    uint8_t Width;
    uint8_t Height;
    uint8_t  * pData;
    DISP_BITMAP_DRAW_METHOD DrawMethod; 
} Disp_Bitmap;

typedef struct 
{                                   /// Hold the details of a bitmap and the location to be drawn.
    Disp_Bitmap *pBitmap;                          ///< Pointer to bitmap image.
    DISP_POINT   Location;                     ///< Location where bitmap needs to be drawn.
} Disp_AnimationBitmap; 

typedef struct 
{                                   /// Structure to hold the details of all the bitmaps and other variables used for animation.
    Disp_AnimationBitmap  **pBitmapArray;          ///< Pointer to array of bitmaps.
    uint16_t              *pFrameTimeArray;        ///< Pointer to array of time duration for each bitmap to be shown..
    uint16_t               BitmapCount;            ///< Number of bitmaps to be used in Animation.
} Disp_Animation;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/
extern const unsigned char _animinwc01[];           /*! Welcome Screen Animation */
extern const unsigned char _animinwc02[];
extern const unsigned char _animinwc03[];
extern const unsigned char _animinwc04[];
extern const unsigned char _animinwc05[];
extern const unsigned char _animinwc06[];
extern const unsigned char _animinwc07[];
extern const unsigned char _animinwc08[];
extern const unsigned char _animinwc09[];
extern const unsigned char _animinwc10[];

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern DISP_PORT_STATUS L3_DispInit (void);
extern FONT_TYPE L3_DispSetFont(FONT_TYPE Font);
extern uint8_t L3_DispSetPenSize(uint8_t PenSize);
extern void L3_DispSetColor(uint32_t Color);
extern void L3_DispSetBkColor(uint32_t Color);
extern int8_t L3_DispGotoXY(int8_t X, int8_t Y);
extern void L3_DispChar(uint8_t Char);
extern void L3_DispString(int8_t const *pString);
extern void L3_DispClear(void);
extern void L3_DispDrawLine(int8_t X1, int8_t Y1, int8_t X2, int8_t Y2);
extern void L3_DispDrawHLine(int8_t Y, int8_t X1, int8_t X2);
extern void L3_DispDrawVLine(int8_t X, int8_t Y1, int8_t Y2);
extern void L3_DispDrawRect(int8_t X1, int8_t Y1, int8_t X2, int8_t Y2);
extern void L3_DispFillRect(int8_t X1, int8_t Y1, int8_t X2, int8_t Y2);
extern void L3_DispClearRect(int8_t X1, int8_t Y1, int8_t X2, int8_t Y2);
extern void L3_DispDrawCircle(int8_t X1, int8_t Y1, int8_t Radius);
extern void L3_DispFillCircle(int8_t X1, int8_t Y1, int8_t Radius);
extern void L3_DispDrawBitmap( Disp_Bitmap *pDispBmp, int8_t X, int8_t Y);
extern void L3_DispDrawRectBorders(uint32_t Color, int8_t X, int8_t Y, int8_t BorderWidth, int8_t XLength, int8_t YLength);
extern void L3_DispDrawBattery(uint32_t Color, DISP_BAT_LEVEL Level);
extern void L3_DispDrawArrow(uint32_t Color, int8_t X, int8_t Y, DISP_POINT *pDispPoint);
extern void L3_DispDrawWifiBars(uint32_t Color, DISP_WIFI_LEVEL Level, int8_t XPos, int8_t YPos);
extern void L3_DispStringAtXY(FONT_TYPE Font, uint32_t FgColor, uint32_t BkColor, int8_t X, int8_t Y, const int8_t *pString);
extern void L3_DispRectFillColor(uint32_t Color, int8_t X1, int8_t X2, int8_t Y1, int8_t Y2);
extern void L3_DispDrawAnimation(Disp_Animation *pAnimation);
extern void L3_DispMemDevCopyToLCD(void);
extern void L3_DisplayOn(bool DisplayIsOn);
extern DISP_PORT_STATUS L3_WelcomeStaticScreen(uint8_t ScreenNo);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_DISPPORT_H */
