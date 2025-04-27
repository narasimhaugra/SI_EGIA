#ifndef OSAL_H
#define OSAL_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Utils
 * \{
 * \brief   Public interface for OS Abstraction
 *
 * \details This module defines OS Abstraction APIs for the application to use
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Osal.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         ///< Import common definitions such as types, etc.

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/// Individual device Fault ID
typedef enum
{
    OSAL_FAULT_NONE,
    OSAL_FAULT_MEM,
    OSAL_FAULT_TASK,
    OSAL_FAULT_TIMER,
    OSAL_FAULT_SEM,
    OSAL_FAULT_QUEUE,
    OSAL_FAULT_MUTEX,
    OSAL_FAULT_LAST
} OSAL_FAULT_ID;

/// OS abstraction layer error event
typedef struct
{
    QEvt                 Event;         ///< QPC event header
    OSAL_FAULT_ID         Id;           ///< Device Fault ID
} QEVENT_OSAL_ERROR;


/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
OS_EVENT *SigSemCreate(uint8_t u8Count, uint8_t *pu8Name, uint8_t *pu8OsErr);
OS_EVENT *SigMutexCreate (uint8_t *pu8Name, uint8_t *pu8OsErr);
uint8_t SigTaskCreate(void (*pTaskFunc)(void *pArg), void *pArg, OS_STK *pbos,
                      uint8_t u8TaskID, uint32_t u32StkSize, uint8_t *pu8Name);
OS_TMR *SigTimerCreate (uint32_t u32Delay, uint32_t u32Period, uint8_t u8TimerType,
                        OS_TMR_CALLBACK pcallback, uint8_t *pu8Name, uint8_t *pu8OsErr);
OS_EVENT *SigQueueCreate (void *start, INT16U size);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* OSAL_H */
