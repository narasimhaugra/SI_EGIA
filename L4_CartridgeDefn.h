#ifndef L4_CARTRIDGEDEFN_H
#define L4_CARTRIDGEDEFN_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif
/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{

 * \brief   Public interface for Cartridge Definitions.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_CartridgeDefn.h
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
/*! Cartridge types */
typedef enum                                /*! Adapter Manager States */
{
   CARTRIDGE_TYPE_DUMB,                       /*! EGIA Adapter */
   CARTRIDGE_TYPE_SMART,                        /*! EEA Adapter */
   CARTRIDGE_TYPE_NGL,                        /*! NGSL Adapter */
   CARTRIDGE_TYPE_COUNT                           /*! Adapter type count */
} CARTRIDGE_TYPE;

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
// Cartridge acces interface strucutre
typedef struct
{
    MEM_LAYOUT_CARTRIDGE    Data;         // Attributes
    AM_DEFN_EEP_UPDATE      Update;       // Function to flush data to 1-Wire EEPROM
    AM_DEFN_EEP_UPDATE      Read;         // Function to Read data from 1-Wire EEPROM
    AM_STATUS               Status;       // General access status code
} AM_CARTRIDGE_IF;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void CartridgeSetDeviceId(DEVICE_UNIQUE_ID DeviceAddress, uint8_t *pData);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_CARTRIDGEDEFN_H */

