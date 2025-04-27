#ifndef L3_ONEWIRETRANSPORT_H
#define L3_ONEWIRETRANSPORT_H

#ifdef __cplusplus      /* header compatible with C++ project */
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
 * \file    L3_OneWireTransport.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "L3_OneWireCommon.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/


/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/


/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void OwTransportResetContex(void);
extern ONEWIRE_STATUS OwTransportInit(void);
extern ONEWIRE_STATUS OwTransportScan(ONEWIRE_BUS Bus, OW_SCAN_TYPE ScanType, ONEWIRE_DEVICE_ID *pDeviceList, uint8_t *pCount);
extern OWSearchContext *OwTransportGetContex(ONEWIRE_BUS Bus);
extern ONEWIRE_STATUS L3_OwNetSearch(OWSearchContext *pSearch);

/* Passthorugh functions */
extern ONEWIRE_STATUS OwTransportSetSpeed(ONEWIRE_SPEED Speed);
extern ONEWIRE_STATUS OwTransportCheck(ONEWIRE_DEVICE_ID *pAddr);
extern ONEWIRE_STATUS OwTransportSend(ONEWIRE_DEVICE_ID *pDevice, uint8_t *pData, uint16_t Len);
extern ONEWIRE_STATUS OwTransportReceive(uint8_t *pData, uint16_t Len);
extern ONEWIRE_STATUS OwTransportSpeed(ONEWIRE_SPEED Speed);
extern ONEWIRE_STATUS OwTransportEnable(bool Enable);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_ONEWIRETRANSPORT_H */
