#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_GuiWidgets
 * \{
 * \brief   GUI Widgets Functions
 *
 * \details GUI widgets are building blocks needed to construct screens to 
 *          display information as needed by applications. This Interface provides 
 *          functions for the appliction to draw widgets like Text, Progress Bar, 
 *          Images, Clips.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_GuiWidgets.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_GuiWidgets.h"
#include "L3_DispPort.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/
  
/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define CLIP_REFRESH_DURATION          (100u)    /*! Clip refresh duration in mS */
#define MAX_WIDGET_CLIPS               (3u)      /*! Maximum number of clips that can be played simultaneously */
#define INVALID_WIDGETINDEX            (0xFFu)   /*! Invalid index */
#define DISP_WIDTH                     (96u)     /*! Display Width */
#define DISP_HEIGHT                    (96u)     /*! Display Height */
#define PALETTE_MAX_TABLE_ENTRIES      (16u)     /*! Maximum entries in palette table */
#define WIDGET_TEXT_OFFSET             (2u)      /*! Text offset in Pixels from widget start point */       

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef struct                  /*! Extended Clip structure used by Widget server */
{
    GUI_WIDGET_CLIP *pClip;     /*! Reference to Clip */
    uint32_t Wait;              /*! Wait time for the current image in the Clip */
} CLIP_ITEM;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/* Active Clip list; will be used by the Widget Server */
static CLIP_ITEM ClipList[MAX_WIDGET_CLIPS]; 

/* Color palette lookup table */
static const uint32_t ColorPaletteLookup[PALETTE_MAX_TABLE_ENTRIES] =
{
    0x000000u, /* Black */
    0xFFFFFFu, /* White */
    0xF4F425u, /* Yellow */
    0xFF0000u, /* Red */
    0x00CD00u, /* Green */
    0x40FF40u, /* Light Green */
    0x0000FFu, /* Blue */
    0x124412u, /* Dark Green */
    0x92979Bu, /* Gray */
    0x878787u, /* Alternate Gray */
    0xFF8635u, /* Tan */
    0xB200FFu, /* Purple */
    0xCC00CCu, /* Pink */
    0xE57EE8u, /* Alternate Pink */
    0x00FFFFu, /* Cyan */
    0xFF00DCu, /* Transparent */  
};

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Paint a window with specified color   
 *
 * \details This function draws a specified color filled rectable with no borders
 *
 * \todo    This function to be moved to Display port/interface module
 *            
 * \param   Color - Window color.
 * \param   X - Horizontal window position.
 * \param   Y - Vertical window position.
 * \param   Width - Window width.
 * \param   Height - Window height.
 *  
 * \return  None
 *
 * ========================================================================== */
void L3_WidgetPaintWindow(SIG_COLOR_PALETTE Color, uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height)
{
    /* Draw window */
    L3_DispRectFillColor(ColorPaletteLookup[Color], X, Y, X+Width, Y+Height);
      
    return;
}
//#######################################################################################################
L3_GUI_WIDGET_STATUS L3_WidgetCircleDraw(UI_OBJECT_CIRCLE *pObject)
{
  L3_GUI_WIDGET_STATUS WidgetStatus;
  
    L3_DispSetBkColor(ColorPaletteLookup[pObject->BackColor]);  
    L3_DispSetColor(ColorPaletteLookup[pObject->BackColor]);
    
    if(pObject->bFill == true)
    {
      L3_DispFillCircle(pObject->X, pObject->Y, pObject->Radius);
    }
    
    L3_DispSetBkColor(ColorPaletteLookup[pObject->BorderColor]);  
    L3_DispSetColor(ColorPaletteLookup[pObject->BorderColor]);
    
    L3_DispDrawCircle(pObject->X, pObject->Y, pObject->Radius);
        
  WidgetStatus = L3_GUI_WIDGET_STATUS_OK;
  return WidgetStatus;
}
//#######################################################################################################
/* ========================================================================== */
/**
 * \brief   Function to draw Text widget 
 *
 * \details This function uses L3 Display Port module API's to draw the text 
 *          as passed text object. The size mentioned in the object is inclusive
 *          of border. The order of drawing elements is background color, border
 *          and then finally the text.
 *
 * \param   pObject - pointer to Text widget object to display
 *  
 * \return  L3_GUI_WIDGET_STATUS - WidgetStatus
 *
 * ========================================================================== */
//###########################################################################################################
L3_GUI_WIDGET_STATUS L3_WidgetTextDraw_New(UI_OBJECT_TEXT *pObject)
{
    L3_GUI_WIDGET_STATUS WidgetStatus;
    GUI_COLOR OldColor;
    uint8_t BorderWidth;

    /* Initialize the variables */
    WidgetStatus = L3_GUI_WIDGET_STATUS_ERROR;
    BorderWidth = 0u;

    OldColor = GUI_GetColor();
    do
    {
        if ( NULL == pObject )
        {
            break;
        }

        /* Draw the background box */
        if(SIG_COLOR_TRANSPARENT != pObject->BackColor)
        {
            L3_DispSetColor(ColorPaletteLookup[pObject->BackColor]);
            L3_DispFillRect(pObject->X, pObject->Y, pObject->X + pObject->Width, pObject->Y + pObject->Height);
        }

        L3_DispSetColor(ColorPaletteLookup[pObject->BorderColor]);
                        
        /* Draw the border box - Note this condition will always be false, as border width support not implemented */
        if ((SIG_COLOR_TRANSPARENT != pObject->BorderColor) && (BorderWidth > 0))
        {
            /* \todo: Display port does have support for line thinkness. Border width support to be revisited */
            for ( BorderWidth = 0; BorderWidth < pObject->BorderSize; BorderWidth++)
            { 
                L3_DispDrawRect(pObject->X + BorderWidth , pObject->Y + BorderWidth, 
                pObject->X + pObject->Width - BorderWidth -1, pObject->Y + pObject->Height - BorderWidth -1);
            }
        }

        /* Draw the text \todo: display string function to be modified to support transparent background */
        L3_DispStringAtXY((FONT_TYPE)pObject->FontType, ColorPaletteLookup[pObject->TextColor], ColorPaletteLookup[pObject->BackColor],
                            pObject->X + WIDGET_TEXT_OFFSET, pObject->Y, (int8_t*)pObject->Text);
        
        WidgetStatus = L3_GUI_WIDGET_STATUS_OK;
    
    } while ( false );

    L3_DispSetBkColor(OldColor);

    return WidgetStatus;
}
//###########################################################################################################
/* ========================================================================== */
/**
 * \fn      L3_GUI_WIDGET_STATUS L3_WidgetProgressBarDraw(GUI_WIDGET_PROGRESS_BAR *pObject)   
 *
 * \brief   Function to draw Progress bar widget 
 *
 * \details This function uses L3 Display Port module API's to 
 *          draw the progress bar which consists of 2 filled rectangles 
 *            
 * \param   pObject - pointer to Progress Bar widget object to display
 *  
 * \return  L3_GUI_WIDGET_STATUS - WidgetStatus
 *
 * ========================================================================== */
//##########################################################################################################
L3_GUI_WIDGET_STATUS L3_WidgetProgressBarDraw_New(UI_OBJECT_PROGRESS *pObject)
{
    L3_GUI_WIDGET_STATUS WidgetStatus;
    uint8_t Progress;
    GUI_COLOR OldColor;
    uint32_t temp;    
    OldColor = GUI_GetColor();
     
    do
    {        
        if ( NULL == pObject )
        {
            WidgetStatus = L3_GUI_WIDGET_STATUS_INVALID_PARAM;
            break;
        }

        if(pObject->Max <= pObject->Min)
        {
            WidgetStatus = L3_GUI_WIDGET_STATUS_INVALID_PARAM;
            break;
        }

        temp = (pObject->Width * pObject->Value)/(pObject->Max - pObject->Min);
        Progress = (uint8_t)temp;
        Progress = (Progress > pObject->Width) ? pObject->Width : Progress;
        
        /* Draw the progress box */
        L3_DispSetColor(ColorPaletteLookup[pObject->ForeColor]);
        L3_DispFillRect(pObject->X, pObject->Y, pObject->X + Progress, pObject->Y + pObject->Height);

        /* Draw the remainder box */
        L3_DispSetColor(ColorPaletteLookup[pObject->BackColor]);
        L3_DispFillRect(pObject->X + Progress, pObject->Y, pObject->X + pObject->Width, pObject->Y + pObject->Height);
        
        WidgetStatus = L3_GUI_WIDGET_STATUS_OK;
        
    } while (false);
    
    L3_DispSetBkColor(OldColor);
    return WidgetStatus;
}
//##########################################################################################################
/* ========================================================================== */
/**
 * \fn      L3_GUI_WIDGET_STATUS L3_WidgetImageDraw(GUI_WIDGET_IMAGE *pObject)   
 *
 * \brief   Function to draw Image widget 
 *
 * \details This function uses L3 Display Port module API's to 
 *          draw an image according to the input 
 *            
 * \param   pObject - pointer to Image widget object to display
 *  
 * \return  L3_GUI_WIDGET_STATUS - WidgetStatus
 *
 * ========================================================================== */
L3_GUI_WIDGET_STATUS L3_WidgetImageDraw(GUI_WIDGET_IMAGE *pObject)
{
    L3_GUI_WIDGET_STATUS WidgetStatus;
    Disp_Bitmap DispBmp;
    
    if ( NULL == pObject )
    {
        WidgetStatus = L3_GUI_WIDGET_STATUS_ERROR;
    }
    else
    {        
        DispBmp.Width = (pObject->Width > DISP_WIDTH) ? DISP_WIDTH : pObject->Width;
        DispBmp.Height = (pObject->Height > DISP_HEIGHT) ? DISP_HEIGHT : pObject->Height;
                
        DispBmp.pData = (uint8_t*)pObject->pBitmap; 
        DispBmp.DrawMethod = DISP_BITMAP_DRAW_METHOD_RLE16; // Need to confirm
         
        L3_DispDrawBitmap(&DispBmp, pObject->X, pObject->Y);        

        WidgetStatus = L3_GUI_WIDGET_STATUS_OK;
    }
    
    return WidgetStatus;
}
//###########################################################################################################
L3_GUI_WIDGET_STATUS L3_WidgetImageDraw_New(UI_OBJECT_BITMAP *pObject)
{
    L3_GUI_WIDGET_STATUS WidgetStatus;
    Disp_Bitmap DispBmp;
    
    if ( NULL == pObject )
    {
        WidgetStatus = L3_GUI_WIDGET_STATUS_ERROR;
    }
    else
    {        
        DispBmp.Width = (pObject->Width > DISP_WIDTH) ? DISP_WIDTH : pObject->Width;
        DispBmp.Height = (pObject->Height > DISP_HEIGHT) ? DISP_HEIGHT : pObject->Height;
                
        DispBmp.pData = (uint8_t*)pObject->pBitmap; 
        DispBmp.DrawMethod = DISP_BITMAP_DRAW_METHOD_RLE16; // Need to confirm
         
        L3_DispDrawBitmap(&DispBmp, pObject->X, pObject->Y);        

        WidgetStatus = L3_GUI_WIDGET_STATUS_OK;
    }
    
    return WidgetStatus;
}
//#############################################################################################################
/* ========================================================================== */
/**
 * \fn      L3_GUI_WIDGET_STATUS L3_WidgetServerRun (void)   
 *
 * \brief   Function to update dynamic objects periodically. Should be called from 
 *          display manager task context at a periodicity set by GUI_WIDGET_UPDATE_PERIOD.
 *
 * \details This function uses L3 Display Port module API's to display all the
 *          clips. A Clip is displayed only if it hasn't reached the end or if
 *          it hasn't reached the pause frame 
 *            
 * \param   < None >
 *  
 * \return  L3_GUI_WIDGET_STATUS
 *
 * ========================================================================== */
L3_GUI_WIDGET_STATUS L3_WidgetServerRun (void)
{
    L3_GUI_WIDGET_STATUS WidgetStatus;
    WidgetStatus = L3_GUI_WIDGET_STATUS_ERROR;

    Disp_Bitmap DispBmp;
    
    static CLIP_ITEM *Item;
    GUI_IMAGE_DECK *pImageDeck;
    uint8_t Index;
    
    /* Loop through all the clips */
    for ( uint8_t i = 0; i < MAX_WIDGET_CLIPS; i++)
    {                        
        Item = &ClipList[i];
        do
        { 
            if (NULL == Item->pClip)
            {
                break;
            }

            /* if the clip paused, no need to update the image, just return */
            if ((Item->pClip->Current >= Item->pClip->Pause) && (Item->pClip->Pause >0))
            {
                break;
            }

            /* Update running timer */
            Item->Wait = (Item->Wait > CLIP_REFRESH_DURATION)? (Item->Wait-CLIP_REFRESH_DURATION) : 0 ;
            
            /* Check if the running timer reached down to zero */
            if (Item->Wait > 0)
            {
                break;    
            }        

            /* Time to refresh the clip with next image */

            Item->pClip->Current++;
            if (Item->pClip->Current >= Item->pClip->ImageCount)
            {
                Item->pClip->Current = 0;       /* Rollover */                
            }

            /* Extract Image deck and find the image as per the 'Current' index */
            pImageDeck = Item->pClip->ImageDeck;
            for(Index=0; Index < Item->pClip->Current ; Index++) 
            {
                pImageDeck++;
            }

            /* Check for a valid image pointer */
            if( NULL == pImageDeck)
            {
                break;
            }

            /* Got image to display, also extract the duration to display the image */

            DispBmp.pData = (uint8_t*) pImageDeck->pImage;
            Item->Wait = pImageDeck->Duration;            
            
            /* Populate the Disp_Bitmap structure */
            DispBmp.Width = (Item->pClip->Width > DISP_WIDTH) ? DISP_WIDTH : Item->pClip->Width;
            DispBmp.Height = (Item->pClip->Height > DISP_HEIGHT) ? DISP_HEIGHT : Item->pClip->Height;

            DispBmp.DrawMethod = DISP_BITMAP_DRAW_METHOD_RLE16; /* \todo: pick it from the BMP structure */

            L3_DispDrawBitmap(&DispBmp, Item->pClip->X, Item->pClip->Y);           
            
            WidgetStatus = L3_GUI_WIDGET_STATUS_OK;
             
        } while (false);
    }

    return WidgetStatus;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

