#ifndef L4_HANDLEDEFN_H
#define L4_HANDLEDEFN_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{
 *
 * \brief   Public interface for Clamshell Definitions.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_HandleDefn.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"                      /*! Import common definitions such as types, etc */
#include "L4_DetachableCommon.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define HANDLE_FLASHPARAM_UNUSED        (4u)
#define MAX_HANDLE_PROCEDURE_COUNT      (300u)   ///< Max handle procedure count

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum
{
    HW_VER_NONE,
    HW_VER_4,
    HW_VER_5,
    HW_VER_COUNT
} HANDLE_HW_VERSIONS;

typedef struct
{
    uint8_t  padding[178];                      ///< align Country Code to be compatible with Legacy
    uint16_t CountryCode;                       ///< Country Code
    uint8_t unused[HANDLE_FLASHPARAM_UNUSED];   ///< Unused bytes to ensure size is multiple of 8
} HandleFlashParameters;

static_assert(!((sizeof(HandleFlashParameters)) % 8)  , "Error: Handle Flash Paramerts size should be multiple of 8");

typedef struct                                /*! Handle Interface functions */
{
    MEM_LAYOUT_HANDLE       Data;             ///< Attributes
    AM_DEFN_EEP_UPDATE      Update;           ///< Eeprom update interface function
    AM_STATUS               Status;           ///< General access status code
    HandleFlashParameters   FlashData;        ///< Datastored in Flashmemory
    AM_DEFN_IF              SaveFlashData;    ///< Update the FlashData to Flash location
    AM_DEFN_IF              ReadFlashData;    ///< Read the FlashData from Flash location and update the FlashData
    HANDLE_HW_VERSIONS      VersionNumber;    ///< Handle Version Number
} AM_HANDLE_IF;

typedef struct
{
    uint16_t handleRemainingProceduresCount;
    uint16_t handleRemainingFireCount;
} HANDLE_DATA;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void HandleSetDeviceId(DEVICE_UNIQUE_ID DeviceAddress, uint8_t *pData);
extern void CheckHandleStartupErrors(void);
extern AM_HANDLE_IF* HandleGetIF(void);
extern DEVICE_UNIQUE_ID HandleGetAddress(void);
extern void HandleProcedureFireCountTest(void);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_HANDLEDEFN_H */

