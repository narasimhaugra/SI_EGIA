#ifndef L3_ONEWIRENETWORK_H
#define L3_ONEWIRENETWORK_H

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
 * \file    L3_OneWireNetwork.h
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
/*! \enum OW_NET_CMD
 *  1-Wire Network Command.
 */
typedef enum                               
{   
    OW_NET_CMD_MATCH        = 0x55,         ///< Command to select a slave
    OW_NET_CMD_SKIP         = 0xCC,         ///< Command to skip address check
    OW_NET_CMD_MATCH_OD     = 0x69,         ///< Command to select a slave with OD support
    OW_NET_CMD_SKIP_OD      = 0x3C,         ///< Command to skip address check for slave with OD support
    OW_NET_CMD_RESUME       = 0xA5,         ///< Command to select previous device
    OW_NET_CMD_READ         = 0x33,         ///< Command to read device address
    OW_NET_CMD_SEARCH_ALL   = 0xF0,         ///< Command to initiate normal search
    OW_NET_CMD_SEARCH_ALM   = 0xFC,         ///< Command to initiate search for slave with alarms
    OW_NET_CMD_LAST         = 0xFF,         ///< Invalid command    
}OW_NET_CMD;

/*! \enum OW_NET_SEARCH_TYPE
 *  1-Wire Network layer status.
 */
typedef enum                               
{   
    OW_NET_SEARCH_TYPE_CONTINUE,            ///< Initiate a progressive search
    OW_NET_SEARCH_TYPE_FULL,                ///< Initiate a new full search
    OW_NET_SEARCH_TYPE_ALARM,               ///< Initiate a new alarm/condition slave search
    OW_NET_SEARCH_TYPE_LAST,                ///< Search option range indicator
}OW_NET_SEARCH_TYPE;


/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern ONEWIRE_STATUS OwNetInit(void);
extern ONEWIRE_STATUS OwNetSetSpeed(ONEWIRE_SPEED Speed);
extern ONEWIRE_STATUS OwNetSend(uint8_t *pData, uint8_t Len);
extern ONEWIRE_STATUS OwNetRecv(uint8_t *pData, uint8_t Len);
extern ONEWIRE_STATUS OwNetDeviceCheck(ONEWIRE_DEVICE_ID *pAddress);
extern ONEWIRE_STATUS OwNetEnable(bool Enable);
extern ONEWIRE_STATUS OwNetSearch(OWSearchContext *pSearchCtx);

/* 1-Wire network/ROM commands */
extern ONEWIRE_STATUS OwNetCmdSelect(ONEWIRE_DEVICE_ID *pAddress);
extern ONEWIRE_STATUS OwNetCmdSelectOD(ONEWIRE_DEVICE_ID *pAddress);
extern ONEWIRE_STATUS OwNetCmdSkip(void);
extern ONEWIRE_STATUS OwNetCmdSkipOD(void);
extern ONEWIRE_STATUS OwNetCmdResume(void);
extern ONEWIRE_STATUS OwNetCmdRead(ONEWIRE_DEVICE_ID *Address);
extern ONEWIRE_STATUS OwNetReset(void);
extern ONEWIRE_STATUS OwNetCmdSearch(bool *pDevicePresent);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_ONEWIRENETWORK_H */

