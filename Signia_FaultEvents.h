#ifndef SIGNIA_FAULTEVENTS_H
#define SIGNIA_FAULTEVENTS_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_FaultEvents
 * \{

 * \brief   Public interface for Signia Adapter Publish events.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_FaultEvents.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"
#include "FaultHandler.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/*
   All the Error causes defined in FaultHandler.h are having Error Signals(Signals.h).
   These Error Signals are having the same Qevent Structure(defined below) to publish
   Note: In any case for any signal if new Qevent structure is created, need to handle the same in APP.c for that respective signal */
/// Signia Fault Handler Event
typedef struct
{
    QEvt               Event;              ///< QPC event header
    ERROR_CAUSE        ErrorCause;         ///< Holds Error Cause
    bool               ErrorStatus;        ///< Error is Set or Clear
} QEVENT_FAULT;

extern const CAUSETOSIG CauseToSig_Table[LAST_ERROR_CAUSE];

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
void Signia_StartupErrorEventPublish(void);
bool Signia_ErrorEventPublish(ERROR_CAUSE Cause, bool ErrorStatus);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SIGNIA_FAULTEVENTS_H */

