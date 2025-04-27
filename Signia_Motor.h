#ifndef  SIGNIA_MOTOR_H
#define  SIGNIA_MOTOR_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \brief   Public interface for Motor Manager module.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_Motor.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         /* Import common definitions such as types, etc. */
#include "L3_Motor.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern MM_STATUS L4_MotorInit(void);
extern MM_STATUS Signia_MotorStart(MOTOR_ID MotorId, int32_t Position, uint16_t Speed, uint16_t TimeDelay, uint16_t Timeout, uint16_t CurrentTrip, uint16_t CurrentLimit, bool InitCurrent, MOTOR_SUPPLY MotorVoltage, uint32_t ReportInterval);
extern MM_STATUS Signia_MotorStop(MOTOR_ID MotorId);
extern MM_STATUS Signia_MotorUpdateSpeed(MOTOR_ID MotorId, uint16_t Speed, MOTOR_SUPPLY MotorVoltage);
extern MM_STATUS Signia_MotorEnable(MOTOR_ID MotorId);
extern MM_STATUS Signia_MotorDisable(MOTOR_ID MotorId);
extern MM_STATUS Signia_MotorSetPos(MOTOR_ID MotorId, int32_t Ticks);
extern MM_STATUS Signia_MotorGetPos(MOTOR_ID MotorId, int32_t *pPosition);
extern MM_STATUS Signia_MotorIsStopped(MOTOR_ID MotorId, bool *pHasStopped);
extern MM_STATUS Signia_MotorSetExternalProcess(MOTOR_ID MotorId, MOTOR_PROCESS_FUNCTION pFunction);

extern bool      Signia_AnyMotorRunning(void);

extern void Signia_MotorSetCurrentTripProfile(MOTOR_ID MotorId, MOT_CURTRIP_PROFILE *pProfile );

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SIGNIA_MOTOR_H */

