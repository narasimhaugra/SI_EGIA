#ifndef CPU_CYCLES_H
#define CPU_CYCLES_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup CpuCycles
 * \{
 * \brief   References for cycle counting utilities
 *
 * \details These routines handle the DWT (Data Watchpoint and Trace) module
 *          in the K20. The module has additional capabilities that are not
 *          supported here. (Comparators, events, etc.)
 *
 * \copyright  Copyright 2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * \file  CpuCycles.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Board.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define CORE_DEBUG_ENABLE_MASK      (0x01000000u)
#define CORE_DEBUG_RESET_COUNT      (0u)

#define CPU_COUNTER_ENABLE()        ((DEMCR) |= (CORE_DEBUG_ENABLE_MASK))       // Enable cycle counter
#define CPU_COUNTER_DISABLE()       ((DEMCR) &= ~(CORE_DEBUG_ENABLE_MASK))      // Disable
#define CPU_COUNTER_RESET()         ((DWT_CYCCNT) = (CORE_DEBUG_RESET_COUNT))   // Restart counter
#define CPU_COUNTER_READ()          ((DWT_CYCCNT)/ (120u))                      // Value in micro seconds

/******************************************************************************/
/*                             Global Type(s)                                 */
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
void CpuTimeLogInit(void);
void CpuTimeLogAndRestart(uint32_t Value);
void CpuTimeLog(uint32_t Value);
void CpuTimeLogDump(uint32_t DumpThreshold);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* CPU_CYCLES_H */
