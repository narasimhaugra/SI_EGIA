#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   Low Battery Screen (image #1) template
 *
 * \details Shows Low Battery Screen (image #1) 
 *
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_LowBattPR1.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_LowBattPR1.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
extern const unsigned char _acHandleBM90[];
extern const unsigned char _acBattery_10[];

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER            (LOG_GROUP_DISPLAY) /*! Log Group Identifier */

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                       /*! Image identfiers */
{
    LOW_BATT_1_BATTERY,
    LOW_BATT_1_ADPT_BM90,
} LOW_BATT_1_SCREEN;

typedef enum                       /*! Text identfiers  */
{         
    LOW_BATT_1_TEXT_OUTER_BOX1,      /*! Outer Border	*/
    LOW_BATT_1_TEXT_OUTER_BOX2,	   /*! Background color	*/
    LOW_BATT_1_TEXT_OUTER_BOX3,	   /*! Procedures remain */
} LOW_BATT_1_TEXT;        

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/*! List of Images to be drawn */

/*! Handle EOL Image list */
static DM_OBJ_IMAGE ImageList_LowBatt1[] = 
{
    /*! Battery Image */
    {LOW_BATT_1_BATTERY,           {41, 6, 48, 16, (BITMAP*)_acBattery_10}, false, false},
     /*! Power Handle */
    {LOW_BATT_1_ADPT_BM90,         {30, 28, 31, 41, (BITMAP*)_acHandleBM90}, false, false},
    /*! Last image */
    {INVALID_ID,                   {0, 0, 0, 0, NULL}, false, false}
};

/*! Handle EOL Text object */
static DM_OBJ_TEXT LowBatt1List[] =
{    
    /*! Outer screen Border*/
    {LOW_BATT_1_TEXT_OUTER_BOX1,  {0, 0, 96, 96, SIG_COLOR_TRANSPARENT, SIG_COLOR_WHITE, 1, SIG_COLOR_TRANSPARENT, SIG_FONT_20B_1, ""}, false,false},
    /*! Background !*/
    {LOW_BATT_1_TEXT_OUTER_BOX2,  {3, 3, 89, 89, SIG_COLOR_BLACK, SIG_COLOR_GRAY, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_20B_1, ""}, false,false},
    /*! Update Procedures Value */
    {LOW_BATT_1_TEXT_OUTER_BOX3, {30, 68, 20, 10, SIG_COLOR_BLACK, SIG_COLOR_GRAY,0, SIG_COLOR_TRANSPARENT, SIG_FONT_20B_1, ""}, false, true},
    {INVALID_ID,  {0, 0, 0, 0, SIG_COLOR_YELLOW, SIG_COLOR_BLUE, 1, SIG_COLOR_YELLOW, SIG_FONT_13B_1, ""}, false}
};

/*! Screen */
DM_SCREEN LowBattProceduresRemainOneScreen = 
{
    SCREEN_ID_LOW_BATT_PR1,       /*! Screen ID */
    LowBatt1List,               /*! Text Object */
    ImageList_LowBatt1,         /*! Image List */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, 
    NULL
};

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/**
 * \brief   LowBatt_ShowProceduresRemainOne 
 *  
 * \details Shows the number of remaining procedures 
 *  
 * \param   ProceduresRemain - remaining procedures number to show
 *  
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success 
 * ========================================================================== */
DM_STATUS LowBatt_ShowProceduresRemainOne(uint16_t ProceduresRemain)
{
    DM_STATUS Status;
    uint8_t TextToShow[10];                           
    
    sprintf((char *)TextToShow, "%d", ProceduresRemain); 
    Status = L4_DmTextUpdate(LOW_BATT_1_TEXT_OUTER_BOX3, TextToShow);   
    if (Status != DM_STATUS_OK)
    {
        Log(ERR, "L4_DmTextUpdate: Error - %d", Status);
    }
    
    L4_DmTextHide(LOW_BATT_1_TEXT_OUTER_BOX3, false);                 
    if (Status != DM_STATUS_OK)
    {
        Log(ERR, "L4_DmTextHide: Error - %d", Status);
    }
    
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Shows Low Battery Screen (image #1)  
 *  
 * \details Shows Low Battery Screen (image #1)  
 *  
 * \param   < None >
 *  
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success 
 * ========================================================================== */
DM_STATUS Gui_LowBattProceduresRemainOne_Screen(void)
{
    DM_STATUS Status;
    
    L4_DmShowScreen( &LowBattProceduresRemainOneScreen );
    Status = LowBatt_ShowProceduresRemainOne(300);
    
    return Status;
}

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

