#ifndef NOINITRAM_H
#define NOINITRAM_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup NOINITRAM
 * \{
 * \brief   NoInit RAM
 *
 * \details This module defines the NoInit RAM structure.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    NoInitRam.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "McuX.h"
#include "L4_BlobHandler.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
/* As per .icf file we have 0x20000000 - 0x2000FFFF as RAM2 (internal) 64K. In 
that 0x2000FF00 to 0x2000FFFF - 0xFF bytes available for NO_INIT_RAM */
#define NO_INIT_RAM_LOCATION      (0x2000FF00u)  ///< Starting location of RAM where it is not initialized at reset
#define NO_INIT_RAM_SIZE          (0xFFu)        ///< Size of NO_INIT_RAM
#define NO_INIT_RAM_MAGIC_NUMBER  (0x9ABCDEF0u)  ///< A Unique number to check the NO_INIT_RAM area validity

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/*  Struct object to hold a small area in RAM where it is not initialized at reset.
    The data in this structure (stored in RAM) will survive resets, but not power downs. */
typedef struct                                    ///  No Init RAM Structure
{
    BOOT_FLAGS           BootStatus;              ///< Bootloader status (used by Bootloader, keep as 1st struct member)
    uint32_t             MagicNumber;             ///< A magic number to check the memory validity (keep as 2nd struct member)
    McuXCoreDump         xLastMcuXCoreDump;       ///< MCU Exception Software Dump
    bool                 deepSleepActivated;      ///< Set to TRUE before calling the WFI command to sleep
    bool                 wfiHardFault;            ///< The WFI instruction caused a hard fault
    bool                 batteryCheckFromSleep;   ///< Flag to identify the last sleep event was from a battery health check
    bool                 BqChipWasReset;          ///< Flag to signify the BQ chip was reset. Used later to log the event
    bool                 TestModeActive;          ///< Test Mode status flag
    bool                 ProcedureHasFiredFlag;
    bool                 NoInitRamWasReset;       ///< noinit ram magic number mismatched, noinit ram is cleared
    uint16_t             TM_KEY;
    uint16_t             TM_TestData;
    uint32_t             TM_TSTMODEONSTARTUP;
    uint8_t              TM_TestID;
    BLOB_HANDLER_STATUS  BlobValidationStatus;    ///< Blob validation status
} NoInitRamStruct;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/
/* Do a compile time check, if expression is false, the message string will be issued. */
static_assert (sizeof(NoInitRamStruct) <= NO_INIT_RAM_SIZE, "NoInitRamStruct Over Size");
/* The data in this structure (stored in MCU RAM) will survive resets, but not power downs. */
__no_init NoInitRamStruct noInitRam @ NO_INIT_RAM_LOCATION;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* NOINITRAM_H */
