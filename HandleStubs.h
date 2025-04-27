#ifndef HANDLESTUBS_H
#define HANDLESTUBS_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Handle
 * \{
 *
 * \brief   Prototypes for external references (EGIA/EEA)
 *
 * \details These are the prototypes for the top level states in EGIA, EEA, etc.
 *          both weak & strong
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    HandleStubs.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Common.h"

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern QState EGIA_Operate(Handle * const me, QEvt const * const e);
extern QState EEA_Operate(Handle * const me, QEvt const * const e);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* APPSTUBS_H */
