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
 * \details Define Adapter Request screen and associated action methods. 
 *          screen plays animation request user to connect Adapter Request.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_HandleStatus.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_HandleStatus.h"
#include "ScreenTemplate.h" 

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

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
/// Text identifier
typedef enum                      
{
    TEXT_ID_NONE,                 ///<  Start of the list 
    TEXT_ID_STATUS_SCREEN,        ///<  Status screen id 
    TEXT_ID_LAST                  ///<  Last text id 
} TEXT_ID;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/*! Screen to set the Border */
DM_SCREEN PP_StatusScreen = 
{
    SCREEN_ID_HANDLE_STATUS, /*! Screen ID */
    NULL,                    /*! Border color Object */
    NULL,                    /*! Battery case image List */
    NULL,                    /*! Battery level */
    NULL,
    NULL,
    NULL,
    NULL, 
    NULL
};

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/

/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

