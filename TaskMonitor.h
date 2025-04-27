#ifndef TASKMONITOR_H
#define TASKMONITOR_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/*================================================================================================*/
/**
 * \addtogroup TaskMonitor
 * \{
 *
 * \brief Brief Description.
 *
 * \details   This header file provides the public interface for the
 *            TaskMonitor module, including symbolic constants, enumerations,
 *            type definitions, and function prototypes.
 *
 * \bug  None
 *
 * \copyright  Copyright 2019 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * \file  TaskMonitor.h
 *
 * ================================================================================================*/
/**************************************************************************************************/
/*                                         Includes                                               */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                    Global Define(s) (Macros)                                   */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                    Global Type(s)                                              */
/**************************************************************************************************/
typedef enum                            /// Taskmonitor Function call status.         
{                               
   TASKMONITOR_OK,                          ///<  Status Ok
   TASKMONITOR_ERROR,                       ///<  Status Error
   TASKMONITOR_INVALD_PARAM,                ///<  Status Invalid Param
   TASKMONITOR_STATUS_ENABLED,              ///<  Task Monitor Enabled
   TASKMONITOR_STATUS_DISABLED,             ///<  Task Monitor Disabled
   TASKMONITOR_LAST                         ///<  Status Last
} TASKMONITOR_STATUS;

/**************************************************************************************************/
/*                                   Global Constant Declaration(s)                               */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                   Global Variable Declaration(s)                               */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                   Global Function Prototype(s)                                 */
/**************************************************************************************************/
TASKMONITOR_STATUS TaskMonitorInit( void );
TASKMONITOR_STATUS TaskMonitorTaskCheckin(uint8_t u8TaskPriority);
TASKMONITOR_STATUS TaskMonitorRegisterTask(uint32_t checkInTimeout);
TASKMONITOR_STATUS TaskMonitorUnRegisterTask(void);
TASKMONITOR_STATUS TaskMonitorDisable( void );
TASKMONITOR_STATUS TaskMonitorEnable( void );
void TaskMonitorSetLogPeriod(uint8_t Seconds);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* __TASKMONITOR_H__ */

