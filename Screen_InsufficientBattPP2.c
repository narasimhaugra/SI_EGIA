#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   Insufficient Battery Screen (image #2) template
 *
 * \details Shows Insufficient Battery Screen (image #2) 
 *
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_InsufficientBattPP2.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_InsufficientBattPP2.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
extern const unsigned char _acRequestClamp2[];
extern const unsigned char _acBattery_0[];

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER            (LOG_GROUP_DISPLAY) /*! Log Group Identifier */

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                       /*! Image identfiers */
{
    INS_BATT_2_BATTERY,
    INS_BATT_2_CLAMP,
} INS_BATT_2_SCREEN;

typedef enum                       /*! Text identfiers  */
{         
    INS_BATT_2_TEXT_OUTER_BOX1,      /*! Outer Border	*/
    INS_BATT_2_TEXT_OUTER_BOX2,	   /*! Background color	*/
    INS_BATT_2_TEXT_OUTER_BOX3,	   /*! text */
} INS_BATT_2_TEXT;        

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
static DM_OBJ_IMAGE ImageList_InsBatt2[] = 
{
    /*! Battery Image */
    {INS_BATT_2_BATTERY,       {41, 6, 48, 16, (BITMAP*)_acBattery_0}, false, false},
    /*! Power Handle */
    {INS_BATT_2_CLAMP,         {33, 43, 55, 20, (BITMAP*)_acRequestClamp2}, false, false},
    /*! Last image */
    {INVALID_ID,                   {0, 0, 0, 0, NULL}, false, false}
};

/*! Handle EOL Text object */
static DM_OBJ_TEXT InsBatt2List[] =
{    
    /*! Outer screen Border*/
    {INS_BATT_2_TEXT_OUTER_BOX1,  {0, 0, 96, 96, SIG_COLOR_TRANSPARENT, SIG_COLOR_WHITE, 1, SIG_COLOR_TRANSPARENT, SIG_FONT_20B_1, ""}, false,false},
    /*! Background !*/
    {INS_BATT_2_TEXT_OUTER_BOX2,  {3, 3, 89, 89, SIG_COLOR_BLACK, SIG_COLOR_GRAY, 0, SIG_COLOR_TRANSPARENT, SIG_FONT_20B_1, ""}, false,false},
    /*! Update Procedures Value */
    {INS_BATT_2_TEXT_OUTER_BOX3, {6, 46, 20, 12, SIG_COLOR_WHITE, SIG_COLOR_GRAY,0, SIG_COLOR_TRANSPARENT, SIG_FONT_20B_1, ""}, false, true},
    {INVALID_ID,  {0, 0, 0, 0, SIG_COLOR_YELLOW, SIG_COLOR_BLUE, 1, SIG_COLOR_YELLOW, SIG_FONT_13B_1, ""}, false}
};

/*! Screen */
DM_SCREEN InsufficientBatt_PowerPackTwo = 
{
    SCREEN_ID_INSUFFICIENT_BATT_PP2,       /*! Screen ID */
    InsBatt2List,                          /*! Text Object */
    ImageList_InsBatt2,                    /*! Image List */
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
 * \brief   InsufficientBatt_PowerPackTwo_ShowProcedures 
 *  
 * \details Shows remaining procedures number on the screen
 *  
 * \param   ProcedureCount - procedures remain
 *  
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success 
 * ========================================================================== */
DM_STATUS InsufficientBatt_PowerPackTwo_ShowProcedures(uint16_t ProcedureCount)
{
    DM_STATUS Status;
    uint8_t TextToShow[10];                           
    
    sprintf((char *)TextToShow, "%dx", ProcedureCount); 
    Status = L4_DmTextUpdate(INS_BATT_2_TEXT_OUTER_BOX3, TextToShow);   
    if (Status != DM_STATUS_OK)
    {
        Log(ERR, "L4_DmTextUpdate: Error - %d", Status);
    }
    
    L4_DmTextHide(INS_BATT_2_TEXT_OUTER_BOX3, false);                 
    if (Status != DM_STATUS_OK)
    {
        Log(ERR, "L4_DmTextHide: Error - %d", Status);
    }
    
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Shows Insufficient Battery Screen (image #2)  
 *  
 * \details Shows Insufficient Battery Screen (image #2)  
 *  
 * \param   < None >
 *  
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success 
 * ========================================================================== */
DM_STATUS Gui_InsufficientBatt_PowerPackTwo_Screen(void)
{
    DM_STATUS Status;
    
    L4_DmShowScreen( &InsufficientBatt_PowerPackTwo );
    Status = InsufficientBatt_PowerPackTwo_ShowProcedures(0);
    
    return Status;
}

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

