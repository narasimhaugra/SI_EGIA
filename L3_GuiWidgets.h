#ifndef  L3_GUIWIDGETS_H 
#define  L3_GUIWIDGETS_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_GuiWidgets
 * \{
 * \brief   Public interface for GUI Widgets module.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_GuiWidgets.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         /* Import common definitions such as types, etc. */

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define MAX_TEXT_SIZE               (20u)   /*! Maximum supported text size */
#define MAX_CLIP_IMAGES             (11u)   /*! Maximum number of images supported in a Clip widget */
#define GUI_WIDGET_UPDATE_PERIOD    (1u)    /*! Period at which the widgets are refreshed */
#define WIDGET_SERVER_RUN_TIME      (50u)   /*! 50 msec */

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef uint8_t BITMAP;                    /*! Bitmap image data */

typedef enum
{
    L3_GUI_WIDGET_TYPE_TEXT,                /*! Text Widget */
    L3_GUI_WIDGET_TYPE_IMAGE,               /*! Image Widget */
    L3_GUI_WIDGET_TYPE_PROGRESS,            /*! Progress bar Widget */
    L3_GUI_WIDGET_TYPE_CLIP,                /*! Clip Widget */
    L3_GUI_WIDGET_TYPE_MOVIE,               /*! Movie Widget */
    L3_GUI_WIDGET_TYPE_ALL,      
    L3_GUI_WIDGET_TYPE_LAST      
} L3_GUI_WIDGET_TYPE;

typedef enum                                /*! GUI Widget function status codes */
{  
    L3_GUI_WIDGET_STATUS_OK,                /*! No error */
    L3_GUI_WIDGET_STATUS_INVALID_PARAM,     /*! Invalid parameter */
    L3_GUI_WIDGET_STATUS_ERROR,             /*! Error */
    L3_GUI_WIDGET_STATUS_LAST               
} L3_GUI_WIDGET_STATUS;

typedef enum                    /*! Font types being used, can be expanded as needed. */
{
    SIG_FONT_13B_1,             /*! Corresponds to font name GUI_FONT_13B_1. Refer to uC-GUI user manual for more details. */
    SIG_FONT_20B_1,             /*! Corresponds to font name GUI_FONT_20B_1. Refer to uC-GUI user manual for more details. */
    SIG_FONT_LAST                           
} SIG_FONT;

typedef enum                    /*! Color palette. */
{
    SIG_COLOR_BLACK,            /*! Black */
    SIG_COLOR_WHITE,            /*! White */
    SIG_COLOR_YELLOW,           /*! Yellow */
    SIG_COLOR_RED,              /*! Red */
    SIG_COLOR_GREEN,            /*! Green */
    SIG_COLOR_LIGHT_GREEN,      /*! Light Green */
    SIG_COLOR_BLUE,             /*! Blue */
    SIG_COLOR_DARKGREEN,        /*! Dark Green */
    SIG_COLOR_GRAY,             /*! Gray */
    SIG_COLOR_GRAY_ALT,         /*! Alternate gray  */
    SIG_COLOR_TAN,              /*! Tan  */
    SIG_COLOR_PURPLE,           /*! Purple  */
    SIG_COLOR_PINK,             /*! Pink  */
    SIG_COLOR_PINK_ALT,         /*! Alternate Pink  */
    SIG_COLOR_CYAN,             /*! Cyan  */
    SIG_COLOR_TRANSPARENT,      /*! Transparent  */
    SIG_COLOR_LAST
} SIG_COLOR_PALETTE;

typedef struct                      /*! Text Widget */     
{ 
    uint8_t X;                      /*! X coordinate of the point  */
    uint8_t Y;                      /*! Y coordinate of the point  */
    uint8_t Width;                  /*! Width of the widget  */
    uint8_t Height;                 /*! Height of the widget  */
    SIG_COLOR_PALETTE TextColor;    /*! Text display color  */
    SIG_COLOR_PALETTE BackColor;    /*! Background color  */
    uint8_t BorderSize;             /*! Border size in pixel  */
    SIG_COLOR_PALETTE BorderColor;  /*! Border color  */
    SIG_FONT FontType;              /*! Font type  */
    uint8_t Text[MAX_TEXT_SIZE];    /*! Display string  */
} GUI_WIDGET_TEXT;

/*! \struct GUI_WIDGET_IMAGE
 *  Image widget data.
 */
typedef struct                  /*! Image Widget */
{ 
    uint8_t X;                  /*! X coordinate of the point */
    uint8_t Y;                  /*! Y coordinate of the point */
    uint8_t Width;              /*! Width of the widget */
    uint8_t Height;             /*! Height of the widget */
    BITMAP  *pBitmap;           /*! Bitmap image data */
} GUI_WIDGET_IMAGE;

typedef struct                      /*! Progress Bar Widget */
{ 
    uint8_t X;                      /*! X coordinate of the point */
    uint8_t Y;                      /*! Y coordinate of the point */
    uint8_t Width;                  /*! Width  of the widget */
    uint8_t Height;                 /*! Height of the widget */
    SIG_COLOR_PALETTE ForeColor;    /*! Foreground color */
    SIG_COLOR_PALETTE BackColor;    /*! Background color */
    uint8_t Min;                    /*! Lower range */
    uint8_t Max;                    /*! Upper range */
    uint8_t Value;                  /*! Current value */
} GUI_WIDGET_PROGRESS_BAR;

typedef struct                      /*! Image deck - collection of images with show for each image */
{
    BITMAP *pImage;                 /*! Bitmap image */
    uint32_t Duration;              /*! Image show duration in milliseconds with 50 mS granularity */
} GUI_IMAGE_DECK;

typedef struct                      /*! Clip Widget */
{ 
    uint8_t X;                      /*! X coordinate of the point */
    uint8_t Y;                      /*! Y coordinate of the point */
    uint8_t Width;                  /*! Width  of the widget */
    uint8_t Height;                 /*! Height of the widget */
    GUI_IMAGE_DECK *ImageDeck;      /*! NULL Terminated Bitmap image collection */
    uint8_t ImageCount;             /*! Image count */
    uint8_t Current;                /*! Current image to be displayed */
    uint8_t Pause;                  /*! Image index to pause. Clip continues if set to 0 */
} GUI_WIDGET_CLIP;

typedef struct                      /*! Movide Widget - A place holder for future implementation */
{
    uint8_t pData;                  /*! Movie data */
} GUI_WIDGET_MOVIE;

// UI Types:

#define UI_SEQUENCE_DEFAULT_REFRESH_RATE (300UL)

typedef bool (*UI_SCREEN_FUNCTION)(uint8_t ScreenId);      

typedef enum
{
  UI_TYPE_TEXT,
  UI_TYPE_SQUARE,
  UI_TYPE_CIRCLE,
  UI_TYPE_BITMAP,
  UI_TYPE_PROGRESS
 } UI_OBJECT_TYPE;

typedef struct
{
  uint8_t X;                      /*! X coordinate of the point  */
  uint8_t Y;                      /*! Y coordinate of the point  */
  uint8_t Width;                  /*! Width of the widget  */
  uint8_t Height;                 /*! Height of the widget  */
  SIG_COLOR_PALETTE TextColor;    /*! Text display color  */
  SIG_COLOR_PALETTE BackColor;    /*! Background color  */
  uint8_t BorderSize;             /*! Border size in pixel  */
  SIG_COLOR_PALETTE BorderColor;  /*! Border color  */
  SIG_FONT FontType;              /*! Font type  */
  char Text[MAX_TEXT_SIZE];    /*! Display string  */
}UI_OBJECT_TEXT;

typedef struct
{
    uint8_t X;                      /*! X coordinate of the point  */
    uint8_t Y;                      /*! Y coordinate of the point  */
    uint8_t Width;                  /*! Width of the widget  */
    uint8_t Height;                 /*! Height of the widget  */
    bool bFill;
    SIG_COLOR_PALETTE BackColor;    /*! Background color  */
    uint8_t BorderSize;             /*! Border size in pixel  */
    SIG_COLOR_PALETTE BorderColor;  /*! Border color  */
 }UI_OBJECT_SQUARE;

typedef struct
{
    uint8_t X;                      /*! X coordinate of the point  */
    uint8_t Y;                      /*! Y coordinate of the point  */
    uint8_t Radius;
    bool bFill;
    SIG_COLOR_PALETTE BackColor;    /*! Background color  */
    uint8_t BorderSize;             /*! Border size in pixel  */
    SIG_COLOR_PALETTE BorderColor;  /*! Border color  */
}UI_OBJECT_CIRCLE;

typedef struct
{
    uint8_t X;                  /*! X coordinate of the point */
    uint8_t Y;                  /*! Y coordinate of the point */
    uint8_t Width;              /*! Width of the widget */
    uint8_t Height;             /*! Height of the widget */
    BITMAP  *pBitmap;           /*! Bitmap image data */
}UI_OBJECT_BITMAP;

typedef struct
{ 
    bool bHorizontal;
    uint8_t X;                      /*! X coordinate of the point */
    uint8_t Y;                      /*! Y coordinate of the point */
    uint8_t Width;                  /*! Width  of the widget */
    uint8_t Height;                 /*! Height of the widget */
    SIG_COLOR_PALETTE ForeColor;    /*! Foreground color */
    SIG_COLOR_PALETTE BackColor;    /*! Background color */
    uint8_t Min;                    /*! Lower range */
    uint8_t Max;                    /*! Upper range */
    uint8_t Value;                  /*! Current value */
}UI_OBJECT_PROGRESS;

typedef struct
{
  UI_OBJECT_TYPE OurObjectType;
  UI_SCREEN_FUNCTION pFunction;
  union
  {
  UI_OBJECT_TEXT ObjText;
  UI_OBJECT_SQUARE ObjSquare;
  UI_OBJECT_CIRCLE ObjCircle;
  UI_OBJECT_BITMAP ObjBitmap;
  UI_OBJECT_PROGRESS ObjProgress;
  };// Object; // no name
} UI_OBJECT;

typedef struct
{
  UI_OBJECT* OurUIObject;  
} UI_SCREEN;

typedef struct
{
  UI_SCREEN* OurUIScreen;  
} UI_SEQUENCE;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

//extern L3_GUI_WIDGET_STATUS L3_WidgetTextDraw(GUI_WIDGET_TEXT *pObject);
//extern L3_GUI_WIDGET_STATUS L3_WidgetProgressBarDraw(GUI_WIDGET_PROGRESS_BAR *pObject);
extern L3_GUI_WIDGET_STATUS L3_WidgetImageDraw(GUI_WIDGET_IMAGE *pObject);
//extern L3_GUI_WIDGET_STATUS L3_WidgetClipShow(GUI_WIDGET_CLIP *pObject);
//extern L3_GUI_WIDGET_STATUS L3_WidgetClipPause(GUI_WIDGET_CLIP *pObject);
//extern L3_GUI_WIDGET_STATUS L3_WidgetMovieShow(GUI_WIDGET_MOVIE *pObject);
//extern L3_GUI_WIDGET_STATUS L3_WidgetMoviePause(GUI_WIDGET_MOVIE *pObject);

void L3_WidgetPaintWindow(SIG_COLOR_PALETTE Color, uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height);
extern L3_GUI_WIDGET_STATUS L3_WidgetServerRun (void);

L3_GUI_WIDGET_STATUS L3_WidgetTextDraw_New(UI_OBJECT_TEXT *pObject);
L3_GUI_WIDGET_STATUS L3_WidgetImageDraw_New(UI_OBJECT_BITMAP *pObject);
L3_GUI_WIDGET_STATUS L3_WidgetProgressBarDraw_New(UI_OBJECT_PROGRESS *pObject);
L3_GUI_WIDGET_STATUS L3_WidgetCircleDraw(UI_OBJECT_CIRCLE *pObject);
/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_GUIWIDGETS_H */

