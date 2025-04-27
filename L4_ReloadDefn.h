#ifndef L4_RELOADDEFN_H
#define L4_RELOADDEFN_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif
/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{

 * \brief   Public interface for Reload Definitions.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_ReloadDefn.h
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
/*! Reload types */
typedef enum
{
   RELOAD_TYPE_SULU,    /*! Single reload */
   RELOAD_TYPE_MULU,    /*! Multi user reload */
   RELOAD_TYPE_COUNT    /*! Reload type count */
} RELOAD_TYPE;

typedef struct                                /*! Reload Interface functions */
{
    MEM_LAYOUT_RELOAD   Data;         // Attributes
    AM_DEFN_EEP_UPDATE  Update;       // Funciton to flush data to 1-Wire EEPROM
    AM_DEFN_EEP_UPDATE  Read;         // Funciton to read data from 1-Wire EEPROM
    AM_STATUS           Status;       // General access status code

} AM_RELOAD_IF;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void ReloadSetDeviceId(DEVICE_UNIQUE_ID DeviceAddress, uint8_t *pData);
static AM_STATUS ReloadEEPRead(uint8_t *pData);
static AM_STATUS ReloadEEPWrite(uint8_t *pData);
static AM_STATUS ReloadLotNumber(uint8_t *pLotNumber);
static AM_STATUS ReloadLotGetUsageCount(uint8_t *pUseCount);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_RELOADDEFN_H */

