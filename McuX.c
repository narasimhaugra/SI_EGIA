#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup MCUX
 * \{
 * \brief      MCU Exception Handler Module
 *
 * \details    This module implements the MK20’s / Arm Cortex M4 exception
 *             handling features which can be used to identify the exception
 *             types and the areas from which they are originated.
 *
 * \copyright  2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    McuX.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Logger.h"
#include "McuX.h"
#include "TestManager.h"
#include "NoInitRam.h"

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER      (LOG_GROUP_MCUX)          ///< Log Group Identifier
#define MCUX_TYPE_MASK(xValue)    ((uint32_t)(1<<(xValue))) ///< Exception Reason Bit Mask
#define MAX_REASON_STRING_LEN     (50u)                     ///< Max Length possible for a Reason String
#define MAX_STRING_LEN            (30u)                     ///< Maximum Task Name Length
#define CORE_DUMP_READY_NUMBER    (0xA5u)                   ///< A Unique number to make sure the core dump is ready or not
#define BYTES_PER_DWORD           (8u)                      ///< Bytes per Double Word
#define STACK_DUMP_SIZE           (8u * BYTES_PER_DWORD)    ///< Stack Dump Size (16 Double Words - 64 bytes)

/*  The below macro updates the MCUx Code Dump with the exception parameters.
    This macro is called from all ESR Handlers. We cannot make this macro as a
    function as calling a function within the ESR changes the parameters we are
    looking for.

    The main updates it does are
    1)  Gets the Arm Exception Stack Frame Registers
    2)  Gets the PSP and MSP Pointers
    3)  Gets all the Fault Registers
    4)  Gets the current TCB details.

    Refer to the following notes and links for more details.

    Note 1: When the processor takes an exception, the processor pushes
    information onto the current stack. This operation is referred to as
    stacking and the structure of eight data words is referred as the stack
    frame.
                                <previous>  <-SP points here before interrupt
                |   SP + 0x1C     xPSR
                |   SP + 0x18     PC
    Decreasing  |   SP + 0x14     LR
    Memory      |   SP + 0x10     R12
    Address     |   SP + 0x0C     R3
                |   SP + 0x08     R2
                |   SP + 0x04     R1
                V   SP + 0x00     R0        <-SP points here after interrupt

    Reference 1: "https://developer.arm.com/documentation/dui0552/a/the-
    cortex-m3-processor/exception-model/exception-entry-and-return?lang=en"
    common for core-m3 and above cores.

    Note 2: Adv Exception Topic - Exception Entry & Exit - The favorite parts about
    ARM exception entry is that the hardware itself implements the ARM Architecture
    Procedure Calling Standard (AAPCS). The AAPCS specification defines a set of
    conventions that must be followed by compilers. One of these requirements is
    around the registers which must be saved by a C function when calling another
    function. When an exception is invoked, the hardware will automatically stack
    the registers that are caller-saved. The hardware will then encode in the link
    register ($lr) a value known as the EXC_RETURN value. This value tells the ARM
    core that a return from an exception is taking place and the core can then
    unwind the stack and return correctly to the code which was running before
    the exception took place

    By leveraging these features, exceptions and thread mode code can share the
    same set of registers and exception entries can be regular C functions! For
    other architectures, exception handlers often have to be written in assembly.

    Reference 2: "https://interrupt.memfault.com/blog/arm-cortex-m-exceptions-and-nvic" */

#define UPDATE_MCUX_CORE_DUMP()                                                              \
    {                                                                                        \
        /* Get the ARM Stack Frame Registers */                                              \
        noInitRam.xLastMcuXCoreDump.ArmStkFrame.R0   = *(uint32_t *)(__get_PSP() + 0x00u);   \
        noInitRam.xLastMcuXCoreDump.ArmStkFrame.R1   = *(uint32_t *)(__get_PSP() + 0x04u);   \
        noInitRam.xLastMcuXCoreDump.ArmStkFrame.R2   = *(uint32_t *)(__get_PSP() + 0x08u);   \
        noInitRam.xLastMcuXCoreDump.ArmStkFrame.R3   = *(uint32_t *)(__get_PSP() + 0x0Cu);   \
        noInitRam.xLastMcuXCoreDump.ArmStkFrame.R12  = *(uint32_t *)(__get_PSP() + 0x10u);   \
        noInitRam.xLastMcuXCoreDump.ArmStkFrame.LR   = *(uint32_t *)(__get_PSP() + 0x14u);   \
        noInitRam.xLastMcuXCoreDump.ArmStkFrame.PC   = *(uint32_t *)(__get_PSP() + 0x18u);   \
        noInitRam.xLastMcuXCoreDump.ArmStkFrame.xPSR = *(uint32_t *)(__get_PSP() + 0x1Cu);   \
                                                                                             \
        /* Get the PSP and MSP */                                                            \
        noInitRam.xLastMcuXCoreDump.PSP = (__get_PSP());                                     \
        noInitRam.xLastMcuXCoreDump.MSP = (__get_MSP());                                     \
                                                                                             \
        /* Get the Fault Registers */                                                        \
        noInitRam.xLastMcuXCoreDump.CFSR  = SCB_CFSR;                                        \
        noInitRam.xLastMcuXCoreDump.HFSR  = SCB_HFSR;                                        \
        noInitRam.xLastMcuXCoreDump.DFSR  = SCB_DFSR;                                        \
        noInitRam.xLastMcuXCoreDump.AFSR  = SCB_AFSR;                                        \
        noInitRam.xLastMcuXCoreDump.MMFAR = SCB_MMFAR;                                       \
        noInitRam.xLastMcuXCoreDump.BFAR  = SCB_BFAR;                                        \
                                                                                             \
        /* Get the Current TCB details. */                                                   \
        noInitRam.xLastMcuXCoreDump.TaskPrio =(uint32_t)OSTCBCur->OSTCBPrio;                 \
        noInitRam.xLastMcuXCoreDump.StkPtr   = OSTCBCur->OSTCBStkPtr;                        \
        noInitRam.xLastMcuXCoreDump.StkBtm   = OSTCBCur->OSTCBStkBottom;                     \
        noInitRam.xLastMcuXCoreDump.StkBase  = OSTCBCur->OSTCBStkBase;                       \
        noInitRam.xLastMcuXCoreDump.StkSize  = OSTCBCur->OSTCBStkSize;                       \
        noInitRam.xLastMcuXCoreDump.StkUsed  = OSTCBCur->OSTCBStkUsed;                       \
                                                                                             \
        /* Create a small dump of Stack. Copy the content */                                 \
        uint8_t u8Cnt = 0;                                                                   \
        uint8_t *pSrc = (uint8_t *)(__get_MSP());                                            \
        uint8_t *pDest = (uint8_t *)(noInitRam.xLastMcuXCoreDump.u8StackDump);               \
        while(u8Cnt++ < STACK_DUMP_SIZE)                                                     \
        {                                                                                    \
            *pDest++ = *pSrc++;                                                              \
        }                                                                                    \
    }

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                                ///  Mcu Exception Fault Types
{
    MCUX_TYPE_HARD_FAULT,                   ///< 0 - HardFault
    MCUX_TYPE_MEMMAN_FAULT,                 ///< 1 - MemManFault
    MCUX_TYPE_BUS_FAULT,                    ///< 2 - BusFault
    MCUX_TYPE_USAGE_FAULT,                  ///< 3 - UsageFault
    MCUX_TYPE_LAST,                         ///< Last Mcu Exception Fault Type
} MCUX_TYPE;

typedef enum                                ///  Mcu Exception Reasons
{
    MCUX_REASON_HARD_FLT_FORCED,            ///< 00 - HardFault, FORCED, Fault escalated to a hard fault.
    MCUX_REASON_HARD_FLT_VECTTBL,           ///< 01 - HardFault, VECTTBL, Bus error on a vector read.
    MCUX_REASON_MEMMAN_FLT_IACCVIOL,        ///< 02 - MemManFault, IACCVIOL, On instruction access.
    MCUX_REASON_MEMMAN_FLT_DACCVIOL,        ///< 03 - MemManFault, DACCVIOL, On data access.
    MCUX_REASON_MEMMAN_FLT_MUNSTKERR,       ///< 04 - MemManFault, MUNSTKERR, During exception stacking.
    MCUX_REASON_MEMMAN_FLT_MSTKERR,         ///< 05 - MemManFault, MSTKERR, During exception unstacking.
    MCUX_REASON_MEMMAN_FLT_MLSPERR,         ///< 06 - MemManFault, MLSPERR, During lazy floating-point state preservation.
    MCUX_REASON_BUS_FLT_IBUSERR,            ///< 07 - BusFault, IBUSERR, During instruction prefetch.
    MCUX_REASON_BUS_FLT_PRECISERR,          ///< 08 - BusFault, PRECISERR, Precise data bus error.
    MCUX_REASON_BUS_FLT_IMPRECISERR,        ///< 09 - BusFault, IMPRECISERR, Imprecise data bus error.
    MCUX_REASON_BUS_FLT_UNSTKERR,           ///< 10 - BusFault, UNSTKERR, During exception unstacking.
    MCUX_REASON_BUS_FLT_STKERR,             ///< 11 - BusFault, STKERR, During exception stacking.
    MCUX_REASON_BUS_FLT_LSPERR,             ///< 12 - BusFault, LSPERR, During lazy floating-point state preservation.
    MCUX_REASON_USAGE_FLT_UNDEFINSTR,       ///< 13 - UsageFault, UNDEFINSTR, Undefined instruction.
    MCUX_REASON_USAGE_FLT_INVSTATE,         ///< 14 - UsageFault, INVSTATE, Attempt to enter an invalid instruction set state.
    MCUX_REASON_USAGE_FLT_INVPC,            ///< 15 - UsageFault, INVPC, Invalid exc_return value.
    MCUX_REASON_USAGE_FLT_NOCP,             ///< 16 - UsageFault, NOCP, Attempt to access a coprocessor.
    MCUX_REASON_USAGE_FLT_UNALIGNED,        ///< 17 - UsageFault, UNALIGNED, Illegal unaligned Load and Store.
    MCUX_REASON_USAGE_FLT_DIVBYZERO,        ///< 18 - UsageFault, DIVBYZERO, Divide by 0"
    MCUX_REASON_TYPE_LAST,                  ///< Last Mcu Exception Type
} MCUX_REASON;

typedef struct                              /// McuX Fault Table
{
    MCUX_TYPE           eMcuXType;          ///< Fault Type
    MCUX_REASON         eMcuXReason;        ///< Fault Reason
    volatile uint32_t  *pu32FaultStatusReg; ///< Fault Status Register
    uint32_t            u32FaultRegMask;    ///< Fault Register Mask
    uint8_t            *pFaultReasonString; ///< Fault Reason String
} MCUX_TABLE;

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
static const MCUX_TABLE mxMcuXFaultReasonsTable[] = ///< MCU Exceptions Master Table, for any new exceptions, add an entry here.
{
    { MCUX_TYPE_HARD_FAULT,    MCUX_REASON_HARD_FLT_FORCED,      &SCB_HFSR, SCB_HFSR_FORCED_MASK,      "HardFault, FORCED, Fault escalated to a hard fault"},
    { MCUX_TYPE_HARD_FAULT,    MCUX_REASON_HARD_FLT_VECTTBL,     &SCB_HFSR, SCB_HFSR_VECTTBL_MASK,     "HardFault, VECTTBL, Bus error on a vector read"    },
    { MCUX_TYPE_MEMMAN_FAULT,  MCUX_REASON_MEMMAN_FLT_IACCVIOL,  &SCB_CFSR, SCB_CFSR_IACCVIOL_MASK,    "MemManFault, IACCVIOL, On instruction access"      },
    { MCUX_TYPE_MEMMAN_FAULT,  MCUX_REASON_MEMMAN_FLT_DACCVIOL,  &SCB_CFSR, SCB_CFSR_DACCVIOL_MASK,    "MemManFault, DACCVIOL, On data access"             },
    { MCUX_TYPE_MEMMAN_FAULT,  MCUX_REASON_MEMMAN_FLT_MUNSTKERR, &SCB_CFSR, SCB_CFSR_MUNSTKERR_MASK,   "MemManFault, MUNSTKERR, During exception stacking" },
    { MCUX_TYPE_MEMMAN_FAULT,  MCUX_REASON_MEMMAN_FLT_MSTKERR,   &SCB_CFSR, SCB_CFSR_MSTKERR_MASK,     "MemManFault, MSTKERR, During exception unstacking" },
    { MCUX_TYPE_MEMMAN_FAULT,  MCUX_REASON_MEMMAN_FLT_MLSPERR,   &SCB_CFSR, SCB_CFSR_MLSPERR_MASK,     "MemManFault, MLSPERR, During lazy FP state pres"   },
    { MCUX_TYPE_BUS_FAULT,     MCUX_REASON_BUS_FLT_IBUSERR,      &SCB_CFSR, SCB_CFSR_IBUSERR_MASK,     "BusFault, IBUSERR, During instruction prefetch"    },
    { MCUX_TYPE_BUS_FAULT,     MCUX_REASON_BUS_FLT_PRECISERR,    &SCB_CFSR, SCB_CFSR_PRECISERR_MASK,   "BusFault, PRECISERR, Precise data bus error"       },
    { MCUX_TYPE_BUS_FAULT,     MCUX_REASON_BUS_FLT_IMPRECISERR,  &SCB_CFSR, SCB_CFSR_IMPRECISERR_MASK, "BusFault, IMPRECISERR, Imprecise data bus error"   },
    { MCUX_TYPE_BUS_FAULT,     MCUX_REASON_BUS_FLT_UNSTKERR,     &SCB_CFSR, SCB_CFSR_UNSTKERR_MASK,    "BusFault, UNSTKERR, During exception unstacking"   },
    { MCUX_TYPE_BUS_FAULT,     MCUX_REASON_BUS_FLT_STKERR,       &SCB_CFSR, SCB_CFSR_STKERR_MASK,      "BusFault, STKERR, During exception stacking"       },
    { MCUX_TYPE_BUS_FAULT,     MCUX_REASON_BUS_FLT_LSPERR,       &SCB_CFSR, SCB_CFSR_LSPERR_MASK,      "BusFault, LSPERR, During lazy FP state pres"       },
    { MCUX_TYPE_USAGE_FAULT,   MCUX_REASON_USAGE_FLT_UNDEFINSTR, &SCB_CFSR, SCB_CFSR_UNDEFINSTR_MASK,  "UsageFault, UNDEFINSTR, Undefined instruction"     },
    { MCUX_TYPE_USAGE_FAULT,   MCUX_REASON_USAGE_FLT_INVSTATE,   &SCB_CFSR, SCB_CFSR_INVSTATE_MASK,    "UsageFault, INVSTATE, Enter an inv instr set state"},
    { MCUX_TYPE_USAGE_FAULT,   MCUX_REASON_USAGE_FLT_INVPC,      &SCB_CFSR, SCB_CFSR_INVPC_MASK,       "UsageFault, INVPC, Invalid exc_return value"       },
    { MCUX_TYPE_USAGE_FAULT,   MCUX_REASON_USAGE_FLT_NOCP,       &SCB_CFSR, SCB_CFSR_NOCP_MASK,        "UsageFault, NOCP, Attempt to access a coprocessor" },
    { MCUX_TYPE_USAGE_FAULT,   MCUX_REASON_USAGE_FLT_UNALIGNED,  &SCB_CFSR, SCB_CFSR_UNALIGNED_MASK,   "UsageFault, UNALIGNED, Illegal unaligned L&S"      },
    { MCUX_TYPE_USAGE_FAULT,   MCUX_REASON_USAGE_FLT_DIVBYZERO,  &SCB_CFSR, SCB_CFSR_DIVBYZERO_MASK,   "UsageFault, DIVBYZERO, Divide by 0"                }
};

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void LogMcuXReason (uint32_t u32ReasonIn);
static void UpdateMcuXReason (MCUX_TYPE eMcuXTypeIn);
static void LogStackDump(void);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Logs the MCU Exception Reasons to the log.
 *
 * \details This function checks the Exception reason bit masks and logs the
 *          reason to the Log. Depending upon the type of exception there could
 *          be multiple reasons [bits] set for an exception.
 *
 * \param   u32ReasonIn - Bit masked value, where each bit corresponds to a
 *          reason.
 *
 * \return  None
 *
 * ========================================================================== */
static void LogMcuXReason (uint32_t u32ReasonIn)
{
    uint8_t u8ReasonCount = 0; /* McuX Reason Count */

    /* Iterate over all the Exception reasons */
    for (u8ReasonCount = 0; u8ReasonCount < MCUX_REASON_TYPE_LAST; u8ReasonCount++)
    {
        /* Check if the bit is set or not */
        if (u32ReasonIn & MCUX_TYPE_MASK(u8ReasonCount))
        {
            Log (REQ, "McuX Reason = %s",  mxMcuXFaultReasonsTable[u8ReasonCount].pFaultReasonString);
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Process the MCU exceptions
 *
 * \details This function compares the exception type with the master table and
 *          checks for the bits set in the exception status register. It creates
 *          the software dump based on the exception reason.
 *
 * \param   eMcuXTypeIn - MCU exception Type Input
 *
 * \return  None
 *
 * ========================================================================== */
static void UpdateMcuXReason (MCUX_TYPE eMcuXTypeIn)
{
    uint32_t u32McuXReason = 0; /* McuX Reason to SW Dump */
    uint8_t u8Count = 0;        /* Count Variable */
    MCUX_TABLE * pxMcuXItem;    /* Pointer to fault reason table */

    /* Iterate over the Reason Table and find the correct fault. */
    for (u8Count = 0; u8Count < MCUX_REASON_TYPE_LAST; u8Count++)
    {
        /* Get the entry from mxMcuXFaultReasonsTable table */
        pxMcuXItem = (MCUX_TABLE *)&mxMcuXFaultReasonsTable[u8Count];

        /* Is the fault type matching with the one passed? */
        if (eMcuXTypeIn == pxMcuXItem->eMcuXType)
        {
            /* Is the fault status register bit set for a specific fault reason? */
            if ((*(pxMcuXItem->pu32FaultStatusReg)) & pxMcuXItem->u32FaultRegMask)
            {
                /* Set the McuX Reason to SW Dump */
                u32McuXReason |= MCUX_TYPE_MASK(pxMcuXItem->eMcuXReason);

                /* Clear the status bit register */
                (*(pxMcuXItem->pu32FaultStatusReg)) |= pxMcuXItem->u32FaultRegMask;
            }
        }
    }

    /* Update the Reason Variable to SW Dump */
    noInitRam.xLastMcuXCoreDump.u32McuXReason = u32McuXReason;

    /* All set, update the u8DumpReady variable */
    noInitRam.xLastMcuXCoreDump.u8DumpReady = CORE_DUMP_READY_NUMBER;
}

/* ========================================================================== */
/**
 * \brief   Log Stack Memory Dump
 *
 * \details This function dumps noInitRam.xLastMcuXCoreDump.u8StackDump.
 *
 * \param   None
 *
 * \return  None
 *
 * ========================================================================== */
static void LogStackDump(void)
{
    uint8_t Count;      /* Count */
    uint8_t *pStack;    /* Pointer to the Stack Dump */

    /* Get the Pointer to the Stack Dump */
    pStack = noInitRam.xLastMcuXCoreDump.u8StackDump;

    Log (REQ, "Stack Dump:");

    /* Log DWord by DWord */
    for (Count = 0; Count < (STACK_DUMP_SIZE/BYTES_PER_DWORD); Count++)
    {
        Log (REQ, " 0x%02X 0x%02X 0x%02X 0x%02X  0x%02X 0x%02X 0x%02X 0x%02X",
             pStack[0], pStack[1], pStack[2], pStack[3],
             pStack[4], pStack[5], pStack[6], pStack[7]);
        pStack += BYTES_PER_DWORD;
    }
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Initializes the MCU exception handling.
 *
 * \details This function initializes the MCU exception handling.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void McuXInit(void)
{
    /* Enable separate fault handlers for MemFault, BusFault, and UsageFault */
    SCB_SHCSR |= SCB_SHCSR_USGFAULTENA_MASK |
                 SCB_SHCSR_BUSFAULTENA_MASK |
                 SCB_SHCSR_MEMFAULTENA_MASK;

    /* Enables following traps
    Divide by zero UsageFault - the processor has executed an SDIV or UDIV
    instruction with a divisor of 0.
    Unaligned access UsageFault - the processor has made an unaligned
    memory access */
    SCB_CCR |= SCB_CCR_DIV_0_TRP_MASK |
               SCB_CCR_UNALIGN_TRP_MASK;
}

/* ========================================================================== */
/**
 * \brief   Logs the Exception Software Dump to the Event log.
 *
 * \details This function reads the software dump from the noinitRam area and
 *          dumps to the event Log.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
#pragma optimize = none
void McuXLogSWDump(void)
{
    /*Log status flags*/
    Log (REQ, "McuX status Flags:");
    Log (REQ, "  wfiHardFault          = %d",  noInitRam.wfiHardFault);
    Log (REQ, "  BqChipWasReset        = %d",  noInitRam.BqChipWasReset);
    Log (REQ, "  deepSleepActivated    = %d",  noInitRam.deepSleepActivated);
    Log (REQ, "  batteryCheckFromSleep = %d",  noInitRam.batteryCheckFromSleep);
    Log (REQ, "  ProcedureHasFiredFlag = %d",  noInitRam.ProcedureHasFiredFlag);

    /*Clear old values*/
    noInitRam.wfiHardFault = false;
    noInitRam.BqChipWasReset = false;
    noInitRam.deepSleepActivated = false;
    noInitRam.batteryCheckFromSleep = false;
    noInitRam.TestModeActive = false;

    /* Log only if the memory is valid, and an Exception SW dump is Ready */
    if ((noInitRam.MagicNumber == NO_INIT_RAM_MAGIC_NUMBER) &&
        (noInitRam.xLastMcuXCoreDump.u8DumpReady == CORE_DUMP_READY_NUMBER))
    {
        /* Create the Last SW Dump */
        Log (REQ, "McuX Core Dump:");
        Log (REQ, "  McuX Num = 0x%08X",  noInitRam.xLastMcuXCoreDump.u32McuXReason);
        LogMcuXReason (noInitRam.xLastMcuXCoreDump.u32McuXReason);

        Log (REQ, " ARM Exception Stack Frame:");
        Log (REQ, "  R0   = 0x%08X",  noInitRam.xLastMcuXCoreDump.ArmStkFrame.R0);
        Log (REQ, "  R1   = 0x%08X",  noInitRam.xLastMcuXCoreDump.ArmStkFrame.R1);
        Log (REQ, "  R2   = 0x%08X",  noInitRam.xLastMcuXCoreDump.ArmStkFrame.R2);
        Log (REQ, "  R3   = 0x%08X",  noInitRam.xLastMcuXCoreDump.ArmStkFrame.R3);
        Log (REQ, "  R12  = 0x%08X",  noInitRam.xLastMcuXCoreDump.ArmStkFrame.R12);
        Log (REQ, "  LR   = 0x%08X",  noInitRam.xLastMcuXCoreDump.ArmStkFrame.LR);
        Log (REQ, "  PC   = 0x%08X",  noInitRam.xLastMcuXCoreDump.ArmStkFrame.PC);
        Log (REQ, "  xPSR = 0x%08X",  noInitRam.xLastMcuXCoreDump.ArmStkFrame.xPSR);

        Log (REQ, " Registers:");
        Log (REQ, "  PSP   = 0x%08X",  noInitRam.xLastMcuXCoreDump.PSP);
        Log (REQ, "  MSP   = 0x%08X",  noInitRam.xLastMcuXCoreDump.MSP);
        Log (REQ, "  CFSR  = 0x%08X",  noInitRam.xLastMcuXCoreDump.CFSR);
        Log (REQ, "  HFSR  = 0x%08X",  noInitRam.xLastMcuXCoreDump.HFSR);
        Log (REQ, "  DFSR  = 0x%08X",  noInitRam.xLastMcuXCoreDump.DFSR);
        Log (REQ, "  AFSR  = 0x%08X",  noInitRam.xLastMcuXCoreDump.AFSR);
        Log (REQ, "  MMFAR = 0x%08X",  noInitRam.xLastMcuXCoreDump.MMFAR);
        Log (REQ, "  BFAR  = 0x%08X",  noInitRam.xLastMcuXCoreDump.BFAR);

        /* Log the Stack Memory Dump */
        LogStackDump();

        Log (REQ, " Fault TCB:");
        Log (REQ, "  TCBPrio = 0x%08X",  noInitRam.xLastMcuXCoreDump.TaskPrio);
        if (noInitRam.xLastMcuXCoreDump.TaskPrio <= OS_LOWEST_PRIO)
        {
            Log (REQ, "  TCBName = Task_%d", noInitRam.xLastMcuXCoreDump.TaskPrio);
        }
        else
        {
            Log (REQ, "  TCBName = NOT AVAILABLE");
        }
        Log (REQ, "  TCBStkPtr    = 0x%08X",  noInitRam.xLastMcuXCoreDump.StkPtr);
        Log (REQ, "  TCBStkBottom = 0x%08X",  noInitRam.xLastMcuXCoreDump.StkBtm);
        Log (REQ, "  TCBStkBase   = 0x%08X",  noInitRam.xLastMcuXCoreDump.StkBase);
        Log (REQ, "  TCBStkSize   = 0x%08X",  noInitRam.xLastMcuXCoreDump.StkSize);
        Log (REQ, "  TCBStkUsed   = 0x%08X",  noInitRam.xLastMcuXCoreDump.StkUsed);
        Log (REQ, " McuX Core Dump Size [Bytes]: %d", sizeof(noInitRam.xLastMcuXCoreDump));
     }

    /* Done Logging, Re-Initialize the Dump, for next run. */
    memset ((uint8_t *)&noInitRam.xLastMcuXCoreDump, 0, sizeof(noInitRam.xLastMcuXCoreDump));
}

/* ========================================================================== */
/**
 * \brief   Non-Maskable Interrupt (NMI) Exception Service Routine
 *
 * \details Non-Maskable Interrupt (NMI) Exception Service Routine. A Non-Maskable
 *          Interrupt (NMI) can be signaled by a peripheral or triggered by
 *          software. This is the highest priority exception other than reset.
 *          It is permanently enabled and has a fixed priority of -2. NMIs cannot
 *          be masked or prevented from activation by any other exception or
 *          preempted by any exception other than Reset.
 *
 * ========================================================================== */
void McuXNMI_ESR(void)
{
    Log (REQ, "NMI: (NMI), NMI");

    // ToDo: What we need to do with NMI?
}

/* ========================================================================== */
/**
 * \brief   Hard Fault Exception Service Routine
 *
 * \details A Hard Fault is an exception that occurs because of an error during
 *          exception processing, or because an exception cannot be managed by
 *          any other exception mechanism. Hard Faults have a fixed priority of
 *          -1, meaning they have higher priority than any exception with
 *          configurable priority
 *
 *          The SCB_HFSR Register gives information about events that activate
 *          the HardFault handler.
 *
 *          The HFSR bits are sticky. This means as one or more fault occurs,
 *          the associated bits are set to 1. A bit that is set to 1 is cleared to 0
 *          only by writing 1 to that bit, or by a reset
 *
 * ========================================================================== */
void McuXHardFault_ESR(void)
{
    if (noInitRam.deepSleepActivated)
    {   // Log any hardfault related to WFI
        noInitRam.wfiHardFault = true;
    }

    /* Update the Core Dump */
    UPDATE_MCUX_CORE_DUMP();

    /* Process the HARD_FAULT */
    UpdateMcuXReason (MCUX_TYPE_HARD_FAULT);
    Log(REQ, "MCU Hard Fault");

    /* reset device */
    //SoftReset();

    //ToDo JA 17-Dec-2021: Enable soft reset later.
    //While (1) helps in debug.

    /* Go to Endless loop */
    while(1)
    {
        ;
    }
}

/* ========================================================================== */
/**
 * \brief   Mem Manage Exception Service Routine
 *
 * \details A Mem Manage fault is an exception that occurs because of a memory
 *          protection-related fault. The fixed memory protection constraints
 *          determine this fault, for both instruction and data memory
 *          transactions. This fault is always used to abort instruction
 *          accesses to Execute Never memory regions
 *
 * ========================================================================== */
void McuXMemManageFault_ESR(void)
{
    /* Update the Core Dump */
    UPDATE_MCUX_CORE_DUMP();

    /* Process the MEMMAN_FAULT */
    UpdateMcuXReason (MCUX_TYPE_MEMMAN_FAULT);

    /* reset device */
    //SoftReset();

    //ToDo JA 17-Dec-2021: Enable soft reset later.
    //While (1) helps in debug.
    while(1)
    {
        ;
    }
}

/* ========================================================================== */
/**
 * \brief   Bus Fault Exception Service Routine
 *
 * \details A Bus Fault is an exception that occurs because of a memory-related
 *          fault for an instruction or data memory transaction. This might be
 *          from an error detected on a bus in the memory system.
 *
 * ========================================================================== */
void McuXBusFault_ESR(void)
{
    /* Update the Core Dump */
    UPDATE_MCUX_CORE_DUMP();

    /* Process the BUS_FAULT */
    UpdateMcuXReason (MCUX_TYPE_BUS_FAULT);

    /* reset device */
    //SoftReset();

    //ToDo JA 17-Dec-2021: Enable soft reset later.
    //While (1) helps in debug.

    while(1)
    {
        ;
    }
}

/* ========================================================================== */
/**
 * \brief   Usage Fault Exception Service Routine
 *
 * \details A Usage Fault is an exception that occurs because of a fault related
 *          to instruction execution. This includes:
 *          - An undefined instruction
 *          - An illegal unaligned access
 *          - Invalid state on instruction execution
 *          - An error on exception return.
 *          The following can cause a Usage Fault when the core is configured to report them:
 *          - An unaligned address on word and halfword memory access
 *          - Division by zero.
 *
 * ========================================================================== */
void McuXUsageFault_ESR(void)
{
    /* Update the Core Dump */
    UPDATE_MCUX_CORE_DUMP();

    /* Process the USAGE_FAULT */
    UpdateMcuXReason (MCUX_TYPE_USAGE_FAULT);

    /* \ToDo Endless loop? Software Reset? or FATAL error? */

    //ToDo JA 17-Dec-2021: Enable soft reset later.
    //While (1) helps in debug.
    while(1)
    {
        ;
    }
}

/* ========================================================================== */
/**
 * \brief   Reads the RCM Registers and logs
 *
 * \details This function reads the RCM_SRS0 and RCM_SRS1 registers and logs
 *          the previous reset reason to the event log.
 *
 *          System Reset Status Register (RCM_SRS0 and RCM_SRS1) - This register
 *          includes read-only status flags to indicate the source of the most
 *          recent reset. The reset state of these bits depends on what caused
 *          the MCU to reset. There could be multiple bits set for a single reset.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void McuXGetPrevResetReason(void)
{
    /*  Power-On Reset(POR)- Bit 7 */
    if (RCM_SRS0 & RCM_SRS0_POR_MASK)
    {
        /* Reset has been caused by power-on detection logic. Because the
        internal supply voltage was ramping up at the time, the (LVD) status
        bit is also set to indicate that the reset occurred while the internal
        supply was below the LVD threshold. */
        Log (REQ, "Prev Reset Reason: (POR), Power-On");
    }

    /*  External Reset Pin(PIN)- Bit 6 */
    if (RCM_SRS0 & RCM_SRS0_PIN_MASK)
    {
        /* Reset has been caused by an active-low level on the external RESET pin. */
        Log (REQ, "Prev Reset Reason: (PIN), External Pin");
    }

    /*  Watchdog(WDOG)- Bit 5 */
    if (RCM_SRS0 & RCM_SRS0_WDOG_MASK)
    {
        /* Reset has been caused by the watchdog timer timing out. */
        Log (REQ, "Prev Reset Reason: (WDOG), Watchdog COP");
    }

    /*  Loss-of-Clock Reset(LOC)- Bit 2 */
    if (RCM_SRS0 & RCM_SRS0_LOC_MASK)
    {
        /*  Reset has been caused by a loss of external clock. The MCG clock
        monitor must be enabled for a loss of clock to be detected. */
        Log (REQ, "Prev Reset Reason: (LOC), Loss of Clock");
    }

    /*  Low-Voltage Detect Reset(LVD)- Bit 1 */
    if (RCM_SRS0 & RCM_SRS0_LVD_MASK)
    {
        /* If the supply drops below the LVD trip voltage, an LVD reset occurs.
        This field is also set by POR.  */
        Log (REQ, "Prev Reset Reason: (LVD), Low-Voltage Detect");
    }

    /*  Low Leakage Wakeup Reset(WAKEUP)- Bit 0 */
    if (RCM_SRS0 & RCM_SRS0_WAKEUP_MASK)
    {
        /*  Reset has been caused by an enabled LLWU module wakeup source while the
        chip was in a low leakage mode. */
        Log (REQ, "Prev Reset Reason: (WAKEUP), Low Leakage Wakeup");
    }

    /*  System Reset Status Register 1 (RCM_SRS1) */

    /*  Stop Mode Acknowledge Error Reset(SACKERR)- Bit 5 */
    if (RCM_SRS1 & RCM_SRS1_SACKERR_MASK)
    {
        /* Indicates that after an attempt to enter Stop mode, a reset has been caused
        by a failure of one or more peripherals to acknowledge within approximately one
        second to enter stop mode. */
        Log (REQ, "Prev Reset Reason: (SACKERR), Stop Mode Ack Error");
    }

    /*  EzPort Reset (EZPT)- Bit 4 */
    if (RCM_SRS1 & RCM_SRS1_EZPT_MASK)
    {
        /* Indicates a reset has been caused by EzPort receiving the RESET command while
        the device is in EzPort mode. */
        Log (REQ, "Prev Reset Reason: (EZPT), EzPort");
    }

    /*  MDM-AP System Reset Request (MDM_AP)- Bit 3 */
    if (RCM_SRS1 & RCM_SRS1_MDM_AP_MASK)
    {
        /* Indicates a reset has been caused by the host debugger system setting of the
        System Reset Request bit in the MDM-AP Control Register. */
        Log (REQ, "Prev Reset Reason: (MDM_AP), Watchdog COP");
    }

    /*  Software (SW)- Bit 2 */
    if (RCM_SRS1 & RCM_SRS1_SW_MASK)
    {
        /* Indicates a reset has been caused by software setting of SYSRESETREQ bit in
        Application Interrupt and Reset Control Register in the ARM core. */
        Log (REQ, "Prev Reset Reason: (SW), Software");
    }

    /*  Core Lockup(LOCKUP)- Bit 1 */
    if (RCM_SRS1 & RCM_SRS1_LOCKUP_MASK)
    {
        /* Indicates a reset has been caused by the ARM core indication of a LOCKUP event. */
        Log (REQ, "Prev Reset Reason: (LOCKUP), ARM Core Lockup");
    }

    /*  JTAG Generated Reset (JTAG)- Bit 0 */
    if (RCM_SRS1 & RCM_SRS1_JTAG_MASK)
    {
        /*  Indicates a reset has been caused by JTAG selection of certain IR codes:
        EZPORT, EXTEST, HIGHZ, and CLAMP. */
        Log (REQ, "Prev Reset Reason: (JTAG), JTAG");
    }
}

/* ========================================================================== */
/**
 * \brief   sets system status flag
 *
 * \details This function sets various system states in the noInit RAM. This status
 *          is used after a reboot to get the previous system state
 * \param   status: System status to be saved
 *
 * \return  None
 *
 * ========================================================================== */
void SetSystemStatus(SYSTEM_STATUS status)
{
    if ( status < SYSTEM_STATUS_LAST)
    {
        switch(status)
        {
            case SYSTEM_STATUS_BATTERY_SHUTDOWN:
                noInitRam.BqChipWasReset = true;
                break;

            case SYSTEM_STATUS_LLS_RESET:
                noInitRam.batteryCheckFromSleep = true;
                break;

            case SYSTEM_STATUS_DEEP_SLEEP_ACTIVATED:
                noInitRam.deepSleepActivated = true;
                break;

            case SYSTEM_STATUS_WFI_HARDFAULT:
                noInitRam.wfiHardFault = true;
                break;

            case SYSTEM_STATUS_TESTMODE:
                noInitRam.TestModeActive = true;
                
            case SYSTEM_STATUS_PROCEDURE_HAS_FIRED_FLAG:
                noInitRam.ProcedureHasFiredFlag = true;
                break;

            default:
                break;

        }
    }
}

/* ========================================================================== */
/**
 * \brief   gets system status
 *
 * \details This function gets various system states saved in the noInit RAM.
 *
 * \param   status - System status to be restored
 *
 * \return  bool - system status
 *
 * ========================================================================== */
bool GetSystemStatus(SYSTEM_STATUS status)
{
    bool Status;

    Status = false;

    if (status < SYSTEM_STATUS_LAST)
    {
        switch(status)
        {
            case SYSTEM_STATUS_BATTERY_SHUTDOWN:
                Status = noInitRam.BqChipWasReset;
                break;

            case SYSTEM_STATUS_LLS_RESET:
                Status = noInitRam.batteryCheckFromSleep;
                break;

            case SYSTEM_STATUS_DEEP_SLEEP_ACTIVATED:
                Status = noInitRam.deepSleepActivated;
                break;

            case SYSTEM_STATUS_WFI_HARDFAULT:
                Status = noInitRam.wfiHardFault;
                break;

            case SYSTEM_STATUS_TESTMODE:
                Status = noInitRam.TestModeActive;
                break;
            
            case SYSTEM_STATUS_PROCEDURE_HAS_FIRED_FLAG:
               Status = noInitRam.ProcedureHasFiredFlag;
               break;
            
            default:
                Status = false;
                break;
        }
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   clear system status
 *
 * \details This function clears system states saved in the noInit RAM.
 *
 * \param   < None >
 *
 * \retval  None
 *
 * ========================================================================== */
void ClearSystemStatus(void)
{
    noInitRam.batteryCheckFromSleep = false;
    noInitRam.BqChipWasReset = false;
    noInitRam.deepSleepActivated = false;
    noInitRam.wfiHardFault = false;
    noInitRam.TestModeActive = false;
}

/* ========================================================================== */
/**
 * \brief   clear system firing status status
 *
 * \details This function clears the firing flag status from noinit RAM
 *
 * \param   < None >
 *
 * \retval  None
 *
 * ========================================================================== */
void ClearNoinitProcedureHasFiredFlag(void)
{
    noInitRam.ProcedureHasFiredFlag = false;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
