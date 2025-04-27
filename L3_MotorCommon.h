#ifndef MOTORCOMMON_H
#define MOTORCOMMON_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \brief   Public interface for Motor control.
 *
 * \details Header file to define common symbols
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_MotorCommon.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/


/// Motor IDs
typedef enum {
    MOTOR_ID0,                      ///< Motor 0 (A0)
    MOTOR_ID1,                      ///< Motor 1 (B1)
    MOTOR_ID2,                      ///< Motor 2 (C2)
    MOTOR_COUNT                     ///< Motor count
} MOTOR_ID;

/// Motor return status
typedef enum {
    MOTOR_STATUS_OK,                ///< No error
    MOTOR_STATUS_ERROR,             ///< General Error
    MOTOR_STATUS_INVALID_PARAM,     ///< Invalid input parameter
} MOTOR_STATUS;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/
/* None */
/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/
/* None */
/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/


#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
#endif


