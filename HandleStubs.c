#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Handle
 * \{
 *
 * \brief   Stubs for Handle Active Object
 *
 * \details The Handle Active object invokes states that are defined outside of the
 *          platform repository. Stubs declared weak are defined here to allow
 *          compilation/testing of the platform by itself, without having to modify
 *          the Handle AO. When linked with applications (such as EGIA), the labels
 *          defined by them will take precedence over the stubs defined here.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    HandleStubs.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Common.h"
#include "ActiveObject.h"
#include "Handle.h"
#include "Logger.h"

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER    LOG_GROUP_GENERAL

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Stub for EGIA substates
 *
 * \details This weak function allows platform to be compiled standalone without
 *          undefined symbol error. Issues a log message when invoked.
 *
 * \param   me - Local data pointer
 * \param   e -  Event pointer
 *
 * \return  QState
 *
 * ========================================================================== */
__weak QState EGIA_Operate(Handle * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            Log(TRC, "Entered EGIA Operate stub ");
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Handle_Operate);
            break;
        }
    }
    return status_;
}

/* ========================================================================== */
/**
 * \brief   Stub for EEA substates
 *
 * \details This weak function allows platform to be compiled standalone without
 *          undefined symbol error. Issues a log message when invoked.
 *
 * \param   me - Local data pointer
 * \param   e -  Event pointer
 *
 * \return  QState
 *
 * ========================================================================== */
__weak QState EEA_Operate(Handle * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            Log(TRC, "Entered EEA Operate stub ");
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Handle_Operate);
            break;
        }
    }
    return status_;
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
