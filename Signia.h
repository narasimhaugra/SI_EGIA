#ifndef SIGNIA_H
#define SIGNIA_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif
/* ========================================================================== */
/**
 * \addtogroup Signia Platform interface file
 * \{

 * \brief   Public interface for Signia platform services and events.
 *
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"
#include "ActiveObject.h"
#include "Signals.h"
#include "Handle.h"
#include "L4_DisplayManager.h"
#include "Signia_Accelerometer.h"
#include "Signia_AdapterEvents.h"
#include "Signia_AdapterManager.h"
#include "Signia_BatteryHealthCheck.h"
#include "Signia_ChargerManager.h"
#include "Signia_CommManager.h"
#include "Signia_FaultEvents.h"
#include "Signia_Keypad.h"
#include "Signia_KeypadEvents.h"
#include "Signia_Motor.h"
#include "Signia_PowerControl.h"
#include "Signia_SoundManager.h"

// Screens defined in Platform:

#include "Screen_AdapterError.h"
#include "Screen_AdapterCheck.h"
#include "Screen_AdapterCompatible.h"
#include "Screen_AdapterRequest.h"
#include "Screen_AdapterVerify.h"
#include "Screen_BatConnectionErr.h"
//#include "Screen_Battery.h"
#include "Screen_ClamshellError.h"
#include "Screen_DepletedBatt.h"
//#include "Screen_GeneralError.h"
#include "Screen_HandleEndOfLife.h"
#include "Screen_HandleError.h"
//#include "Screen_HandleStatus.h"
#include "Screen_InsertClamshell.h"
//#include "Screen_InsufficientBattPP1.h"
//#include "Screen_InsufficientBattPP2.h"
//#include "Screen_LowBattPR1.h"
#include "Screen_LowBattPR2.h"
//#include "Screen_Main.h"
#include "Screen_ProcedureRemain.h"
#include "Screen_ReqReset1.h"
#include "Screen_ReqReset2.h"
#include "Screen_ReqReset3.h"
#include "Screen_ReqReset4.h"
#include "Screen_ReqReset5.h"
#include "Screen_ReqReset6.h"
#include "Screen_ReqResetErr.h"
#include "Screen_ResetErr.h"
#include "Screen_RotationButton.h"
//#include "Screen_SetBatteryLevel.h"
#include "Screen_UsedClamShell.h"
#include "WelcomeScreen.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/**
 * \}
 */

#endif /* SIGNIA_H */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
