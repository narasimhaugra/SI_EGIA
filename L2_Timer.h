#ifndef L2_TIMER_H
#define L2_TIMER_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif
  
/* ========================================================================== */
/**
 * \addtogroup L2_Timer
 * \{
 * \brief   Public interface for periodic timers
 *
 * \details This file has periodic timer related symbolic constants and function prototypes.
 *          This implementation uses only periodic timer module in the MK20 Processor.
 *          Other timer modules such as LPT, PDB are not considered in the implementation.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_Timer.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         // Import common definitions such as types, etc.

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef  void (*TIMER_EVT_HNDLR)(void);             // Timer expiry callback function

typedef enum {              // Periodic timer IDs
  TIMER_ID1,                ///< Periodic timer 1
  TIMER_ID2,                ///< Periodic timer 2
  TIMER_ID3,                ///< Periodic timer 3
  TIMER_ID4,                ///< Periodic timer 3
  TIMER_ID_LAST             ///< Periodic timer range indicator
}TIMER_ID;

typedef enum {              // Timer status
  TIMER_STATUS_STOPPED,     ///< Timer is not running
  TIMER_STATUS_RUNNING,     ///< Timer is running
  TIMER_STATUS_DISABLED,    ///< Timer disabled
  TIMER_STATUS_ERROR,       ///< Error Status
  TIMER_STATUS_LAST         ///< Timer status rnage indicator
}TIMER_STATUS;

typedef enum {              // Timer mode
  TIMER_MODE_ONESHOT,       ///<  One shot timer, stop after first expiry
  TIMER_MODE_PERIODIC,      ///<  Periodic timer, auto-reloads after expiry and runs.
  TIMER_MODE_LAST           ///< Timer mode range indicator
}TIMER_MODE;

typedef struct              //  Periodic Timer Control structure 
{  
  TIMER_ID        TimerId;  ///< Timer ID
  TIMER_MODE      Mode;     ///< Timer mode, One shot or Periodic
  uint32_t        Value;    ///< Timer reload value
  TIMER_EVT_HNDLR pHandler; ///< Timer expiry call back handler
}TimerControl;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void L2_TimerInit(void);
extern TIMER_STATUS L2_TimerConfig(TimerControl *pControl);
extern TIMER_STATUS L2_TimerStart(TIMER_ID TimerId);
extern TIMER_STATUS L2_TimerRestart(TIMER_ID TimerId, uint32_t Duration);
extern TIMER_STATUS L2_TimerStop(TIMER_ID TimerId);
extern TIMER_STATUS L2_TimerStatus(TIMER_ID TimerId);

/**
 * \}  <If using addtogroup above>
 */
 
#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L2_TIMER_H */

/* End of file */
