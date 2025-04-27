#ifndef L3_ONEWIRELINK_H
#define L3_ONEWIRELINK_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif
  
/* ========================================================================== */
/**
 * \addtogroup L3_OneWire
 * \{
 * \brief   Public interface for One Wire module.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_OneWireLink.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "L3_OneWireCommon.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
/* NONE */
/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/*! \enum OW_NET_BUS_PULLUP
 *  1-Wire bus pullup types.
 */
typedef enum                              
{                                       
    OW_NET_BUS_PULLUP_ACTIVE,              ///< Active pullup
    OW_NET_BUS_PULLUP_PASSIVE,             ///< Passive 
    OW_NET_BUS_PULLUP_STRONG,              ///< Strong
}OW_NET_BUS_PULLUP;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern ONEWIRE_STATUS OwLinkInit(void);
extern ONEWIRE_STATUS OwLinkReset(bool *pDevicePresent);
extern ONEWIRE_STATUS OwLinkWriteBit(bool BitValue, bool *pReturnValue);
extern ONEWIRE_STATUS OwLinkWriteByte(uint8_t Data);
extern ONEWIRE_STATUS OwLinkReadByte(uint8_t *pData);

/* 1-Wire master control interfaces */
extern ONEWIRE_STATUS OwLinkSleep(bool Sleep);
extern ONEWIRE_STATUS OwLinkSetPullup(OW_NET_BUS_PULLUP Pullup);
extern ONEWIRE_STATUS OwLinkSetSpeed(ONEWIRE_SPEED Speed);
extern ONEWIRE_STATUS OwLinkUpdateConfig(void);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_ONEWIRELINK_H */
