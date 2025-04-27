#ifndef L4_CLAMSHELLDEFN_H
#define L4_CLAMSHELLDEFN_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{

 * \brief   Public interface for Clamshell Definitions.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_ClamshellDefn.h
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
/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef struct                                /*! Clamshell Interface functions */
{
    MEM_LAYOUT_CLAMSHELL    Data;       ///< Attributes
    AM_DEFN_EEP_UPDATE      Update;     ///< Eeprom update interface function
    AM_STATUS               Status;     ///< General access status code
    bool                    ClamShellEOL;     ///< Clamshell EOL
} AM_CLAMSHELL_IF;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void ClamshellSetDeviceId(DEVICE_UNIQUE_ID DeviceAddress, uint8_t *pData);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_CLAMSHELLDEFN_H */

