#ifndef MCUX_H
#define MCUX_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup MCUX
 * \{
 * \brief   Public interface for MCU Exceptions Module
 *
 * \details This module defines symbolic constants as well as function
 *          prototypes for MCU Exception Handling Module
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    McuX.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"
#include "Board.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define BYTES_PER_DWORD           (8u)                      ///< Bytes per Double Word
#define STACK_DUMP_SIZE           (8u * BYTES_PER_DWORD)    ///< Stack Dump Size (16 Double Words - 64 bytes)

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum
{
    SYSTEM_STATUS_BATTERY_SHUTDOWN,         ///< system shutdown due to low battery
    SYSTEM_STATUS_LLS_RESET,                ///< system reset by LLWU
    SYSTEM_STATUS_DEEP_SLEEP_ACTIVATED,     ///< system deep sleep activated
    SYSTEM_STATUS_WFI_HARDFAULT,            ///< hard fault in WFI
    SYSTEM_STATUS_TESTMODE,                 ///< TestMode Status
    SYSTEM_STATUS_PROCEDURE_HAS_FIRED_FLAG, ///< Handle has fired since last taken off charger
    SYSTEM_STATUS_LAST                      ///< system status last
} SYSTEM_STATUS;

typedef struct                              ///  ARM Exception Stack Frame
{
    uint32_t R0;                            ///< r0 - a1 - Argument / result / scratch register 1.
    uint32_t R1;                            ///< r1 - a2 - Argument / result / scratch register 2.
    uint32_t R2;                            ///< r2 - a3 - Argument / scratch register 3.
    uint32_t R3;                            ///< r3 - a4 - Argument / scratch register 4.
    uint32_t R12;                           ///< r12 - IP - The Intra-Procedure-call scratch register.
    uint32_t LR;                            ///< r14 - LR - The Link Register
    uint32_t PC;                            ///< r15 - PC - The Program Counter.
    uint32_t xPSR;                          ///< xPSR - Program status register
} McuXArmStackFrame;

typedef struct                              ///  MCU Exception Software Dump
{
    uint8_t  u8DumpReady;                   ///< A number to check the dump memory validity
    McuXArmStackFrame ArmStkFrame;          ///< Arm Core Stack Frame.
    uint32_t MSP;                           ///< Main Stack Pointer
    uint32_t PSP;                           ///< Process Stack Pointer
    uint32_t CFSR;                          ///< Configurable Fault Status Register
    uint32_t HFSR;                          ///< HardFault Status Register
    uint32_t DFSR;                          ///< Data Fault Status Register
    uint32_t AFSR;                          ///< Auxiliary Fault Status Register
    uint32_t MMFAR;                         ///< MemManage Fault Address Register
    uint32_t BFAR;                          ///< BusFault Address Register
    uint8_t  u8StackDump[STACK_DUMP_SIZE];  ///< Stack Dump
    uint32_t u32McuXReason;                 ///< MCU Exception Reason.
    uint32_t TaskPrio;                      ///< Last Task Priority
    OS_STK  *StkPtr;                        ///< Pointer to current top of stack
    OS_STK  *StkBtm;                        ///< Pointer to bottom of stack
    OS_STK  *StkBase;                       ///< Pointer to the beginning of the task stack
    uint32_t StkSize;                       ///< Size of task stack
    uint32_t StkUsed;                       ///< Number of stack bytes used
} McuXCoreDump;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void McuXInit (void);
extern void McuXGetPrevResetReason (void);
extern void McuXLogSWDump (void);
extern void SetSystemStatus( SYSTEM_STATUS status);
extern bool GetSystemStatus( SYSTEM_STATUS status);
extern void ClearSystemStatus(void);
extern void ClearNoinitProcedureHasFiredFlag(void);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* MCUX_H */

