#ifndef  SCREEN_ADAPTERVERIFY_H         
#define  SCREEN_ADAPTERVERIFY_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 * \brief   Public interface for Adapter Verify Screen
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_AdapterVerify.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         /* Import common definitions such as types, etc. */

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER            (LOG_GROUP_DISPLAY) /*! Log Group Identifier */
#define BATTERY_LEVEL                   (1u)
#define DM_PROCEDURE_SIZE               (3u)
#define SINGLE_DIGIT                    (1u)
#define DOUBLE_DIGIT                    (2u)
#define TRIPLE_DIGIT                    (3u)

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
extern const unsigned char _acBattery_100[];
extern const unsigned char _acAdapterBM[];
extern const unsigned char _acHandleBM90[];

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/
extern DM_STATUS Gui_AdapterVerifyScreenOne(uint16_t ProcedureCount);
extern DM_STATUS Gui_AdapterVerifyScreenTwo(uint16_t ProcedureCount);

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SCREEN_ADAPTERVERIFY_H */

