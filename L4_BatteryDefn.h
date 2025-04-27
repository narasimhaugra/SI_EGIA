#ifndef L4_BATTERYDEFN_H
#define L4_BATTERYDEFN_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{

 * \brief   Public interface for Battery Definitions.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_BatteryDefn.h
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
typedef struct                     ///< Battery Interface functions
{
    MEM_LAYOUT_BATTERY    Data;     ///< Attributes
    AM_DEFN_EEP_UPDATE    Update;   ///< Eeprom update interface function
    AM_DEFN_EEP_UPDATE    Read;     ///< Battery eEPROM Read function
    AM_STATUS             Status;   ///< General access status code

} AM_BATTERY_IF;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void BatterySetDeviceId(DEVICE_UNIQUE_ID DeviceAddress, uint8_t *pData);
extern AM_BATTERY_IF* BatteryGetIF(void);
/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_BATTERYDEFN_H */

