#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Utils
 * \{
 *
 * \brief   OS Abstraction Layer
 *
 * \details This module is a OS abstraction layer.
 *          Whenever there is a request from the application, the OSAL makes the
 *          appropriate call to the UCOS-II and provide the results.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Osal.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Osal.h" 
#include "ActiveObject.h"
#include "TestManager.h"
#include "FaultHandler.h"
  
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_GENERAL) ///< Log Group Identifier


/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/


/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
// OSAL Error
static OSAL_FAULT_ID         OsalErrId;

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   OSAL Semaphore Create Function
 *
 * \details Creates a Semaphore and returns to the application
 *
 * \param   u8Count - Semaphore Count
 * \param   pu8Name - pointer to Name to the Semaphore
 * \param   pu8OsErr - Pointer to OS Error
 *
 * \return  OS_EVENT - On successful creation, NULL if no Event Control Block available
 *
 * ========================================================================== */
OS_EVENT *SigSemCreate(uint8_t u8Count, uint8_t *pu8Name, uint8_t *pu8OsErr)
{
    OS_EVENT *pOsEvent;     /* Pointer to OSEvent */
    uint8_t OsErr;
    pOsEvent = OSSemCreate(u8Count);
    do
    {
        /* Validate pOsEvent */
        if (NULL == pOsEvent)
        {
            // \todo 01/19/2022 DAZ - NOT REENTRANT!
            OsalErrId = ((*pu8OsErr >= OS_ERR_MEM_INVALID_PART) && (*pu8OsErr <= OS_ERR_MEM_NAME_TOO_LONG))
                               ? OSAL_FAULT_MEM : OSAL_FAULT_SEM;
            Log(DBG,"SigSemCreate Error : OSSemCreate %d ",OsalErrId);
            FaultHandlerSetFault(REQRST_MOO_SYSTEM_FAULT,SET_ERROR);
            break;
        }
        /* Set the Name to Semaphore. */
        OSEventNameSet(pOsEvent, pu8Name, &OsErr);
        if (OS_ERR_NONE != OsErr)
        {
            OsalErrId = ((*pu8OsErr >= OS_ERR_MEM_INVALID_PART) && (*pu8OsErr <= OS_ERR_MEM_NAME_TOO_LONG) )
                               ? OSAL_FAULT_MEM : OSAL_FAULT_SEM;
            Log(DBG,"SigSemCreate Error : %d ",OsalErrId);
            FaultHandlerSetFault(REQRST_MOO_SYSTEM_FAULT,SET_ERROR);
            *pu8OsErr = OsErr;
            break;
        }
        *pu8OsErr = OS_ERR_NONE;
    } while ( false );
    return (pOsEvent);
}

/* ========================================================================== */
/**
 * \brief   This function Creates a Mutex
 *
 * \details Creates a Mutex
 *
 * \param   pu8Name  - Pointer to Name of the Mutex
 * \param   pu8OsErr - Pointer to OS Error
 *
 * \return  OS_EVENT - On successful creation, NULL if no Event Control Block available
 *
 * ========================================================================== */
OS_EVENT *SigMutexCreate (uint8_t *pu8Name, uint8_t *pu8OsErr)
{
    OS_EVENT *pOsEvent;     /* Pointer to OSEvent */
    uint8_t  OsErr;

    pOsEvent = OSMutexCreate(OS_PRIO_MUTEX_CEIL_DIS, &OsErr);
    /* Validate pu8OsErr */
    TM_Hook(HOOK_OSMUTEXFAIL, &OsErr);

    do
    {
        if (OS_ERR_NONE != OsErr)
        {
            OsalErrId = ((*pu8OsErr >= OS_ERR_MEM_INVALID_PART) && (*pu8OsErr <= OS_ERR_MEM_NAME_TOO_LONG))
                               ? OSAL_FAULT_MEM : OSAL_FAULT_MUTEX;
            Log(DBG,"SigMutexCreate Error : OSMutexCreate %d ",OsalErrId);
            FaultHandlerSetFault(REQRST_MOO_SYSTEM_FAULT,SET_ERROR);
            *pu8OsErr = OsErr;
            break;
        }
        /* Set the Name to Mutex. */
        OSEventNameSet(pOsEvent, pu8Name, &OsErr);
        if(OS_ERR_NONE != OsErr)
        {
            OsalErrId = ((*pu8OsErr >= OS_ERR_MEM_INVALID_PART) && (*pu8OsErr <= OS_ERR_MEM_NAME_TOO_LONG))
                               ? OSAL_FAULT_MEM : OSAL_FAULT_MUTEX;
            Log(DBG,"SigMutexCreate Error : %d ",OsalErrId);
            FaultHandlerSetFault(REQRST_MOO_SYSTEM_FAULT,SET_ERROR);
            *pu8OsErr = OsErr;
            break;
        }
        *pu8OsErr = OS_ERR_NONE;
    } while ( false );
    return (pOsEvent);
}

/* ========================================================================== */
/**
 * \brief   This function Create a Task
 *
 * \details Create a Task and register to the task monitor
 *
 * \param   pTaskFunc - Task function
 * \param   pArg - Arguments to the Task
 * \param   u8TaskID - Priority of the task
 * \param   pbos - Address of the stack
 * \param   u32StkSize - stack size
 * \param   pu8Name - Name to be given to the task
 *
 * \return  uint8_t - ucos-II Error Code for task creation
 * ========================================================================== */
uint8_t SigTaskCreate(void (*pTaskFunc)(void *pArg), void *pArg, OS_STK *pbos,
                      uint8_t u8TaskID, uint32_t u32StkSize, uint8_t *pu8Name)
{
    uint8_t u8OsErr;    /* OS Error */

    u8OsErr = OSTaskCreateExt(pTaskFunc, pArg, &pbos[u32StkSize + MEMORY_FENCE_SIZE_DWORDS - 1], u8TaskID, (uint16_t)u8TaskID, &pbos[MEMORY_FENCE_SIZE_DWORDS],
                                   u32StkSize, 0, OS_TASK_OPTIONS);
    
    TM_Hook(HOOK_OSTASKFAIL, &u8OsErr);
    do
    {
        /* Validate pu8OsErr */
        if (OS_ERR_NONE != u8OsErr)
        {
            OsalErrId = ((u8OsErr >= OS_ERR_MEM_INVALID_PART) && (u8OsErr <= OS_ERR_MEM_NAME_TOO_LONG))
                               ? OSAL_FAULT_MEM : OSAL_FAULT_TASK;
            Log(DBG,"SigTaskCreate Error : OSTaskCreateExt %d ",OsalErrId);
            FaultHandlerSetFault(REQRST_MOO_SYSTEM_FAULT,SET_ERROR);
            break;
        }
        /* Set the Task Name */
        OSTaskNameSet(u8TaskID, pu8Name, &u8OsErr);
        if(OS_ERR_NONE != u8OsErr)
        {
           OsalErrId = ((u8OsErr >= OS_ERR_MEM_INVALID_PART) && (u8OsErr <= OS_ERR_MEM_NAME_TOO_LONG))
                              ? OSAL_FAULT_MEM : OSAL_FAULT_TASK;
           Log(DBG,"SigTaskCreate Error :  %d ",OsalErrId);
           FaultHandlerSetFault(REQRST_MOO_SYSTEM_FAULT,SET_ERROR);
           break;
        }
    } while ( false );
    return u8OsErr;
}

/* ========================================================================== */
/**
 * \brief   This function Create a Timer
 *
 * \details Create a Timer and register to the task monitor
 *
 * \param   u32Delay - Initial delay.
 * \param   u32Period - The 'period' being repeated for the timer
 * \param   u8TimerType - Specify One shot or periodic timer
 * \param   callback - pointer to callback function
 * \param   pu8Name - Pointer to name of the timer
 * \param   pu8OsErr - Pointer to an error code
 *
 * \return  OS_TMR - Return to an OS_TMR data structure
 * ========================================================================== */
OS_TMR *SigTimerCreate (uint32_t u32Delay, uint32_t u32Period, uint8_t u8TimerType,
                        OS_TMR_CALLBACK callback, uint8_t *pu8Name, uint8_t *pu8OsErr)
{
    OS_TMR *pTimer; /*Pointer to OS Timer */

    do
    {
        if(NULL == callback)
        {
             OsalErrId = ((*pu8OsErr >= OS_ERR_MEM_INVALID_PART) && (*pu8OsErr <= OS_ERR_MEM_NAME_TOO_LONG))
                               ? OSAL_FAULT_MEM : OSAL_FAULT_TIMER;
            pTimer = NULL;
            Log(DBG,"SigTimerCreate Error :  %d ",OsalErrId);
            FaultHandlerSetFault(REQRST_MOO_SYSTEM_FAULT,SET_ERROR);
            break;
        }
        pTimer = OSTmrCreate(u32Delay, u32Period, u8TimerType, callback, NULL, pu8Name, pu8OsErr);

        if (NULL == pTimer)
        {
            OsalErrId = ((*pu8OsErr >= OS_ERR_MEM_INVALID_PART) && (*pu8OsErr <= OS_ERR_MEM_NAME_TOO_LONG))
                               ? OSAL_FAULT_MEM : OSAL_FAULT_TIMER;
            Log(DBG,"SigTimerCreate Error : OSTmrCreate %d ",OsalErrId);
            FaultHandlerSetFault(REQRST_MOO_SYSTEM_FAULT,SET_ERROR);
            break;
        }
    } while ( false );
    return pTimer;
}

/* ========================================================================== */
/**
 * \brief   This function Creates a Message Queue
 *
 * \details Creates a Message Queue and sends System Failure signal if fails
 *
 * \param   start - message queue storage pointer
 * \param   size - message queue size
 *
 * \return  OS_EVENT - returns message queue pointer, failure if NULL
 * ========================================================================== */
OS_EVENT *SigQueueCreate (void *start, INT16U size)
{
    OS_EVENT *pQ;
    
    do
    {
        if ( ( NULL == start )  || (size == 0 ) )
        {
            OsalErrId = OSAL_FAULT_QUEUE;
            Log(DBG,"SigQueueCreate Error :  %d ",OsalErrId);
            FaultHandlerSetFault(REQRST_MOO_SYSTEM_FAULT,SET_ERROR);
            pQ = NULL;
            break;
        }

        pQ = OSQCreate(start, size);
        TM_Hook(HOOK_OSQUEFAIL, &pQ);
        if (NULL == pQ)
        {
           OsalErrId = OSAL_FAULT_QUEUE;
           Log(DBG,"SigQueueCreate Error : OSQCreate %d ",OsalErrId);
           FaultHandlerSetFault(REQRST_MOO_SYSTEM_FAULT,SET_ERROR);
           break;
        }
    } while ( false );
    return pQ;
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
