#ifndef L3_ONEWIRECONTROLLER_H
#define L3_ONEWIRECONTROLLER_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
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
 * \file    L3_OneWireController.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "L3_OneWireCommon.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/*****************************************************************************/
#define ONEWIRE_MAX_DEVICE_FAMILY   (5u)     ///< Maximum family types support on each bus
#define ONEWIRE_MAX_PACKETS         (28u)     ///< Maximum number of packet supported in one transfer frame

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef  bool (*ONEWIRE_SEGMENT_HANDLER)(uint8_t Index, uint8_t *pData);            ///< callback function to process intermediate response 

/*! \enum ONEWIRE_EVENT
 *  1-Wire events.
 */
typedef enum                            
{
    ONEWIRE_EVENT_NEW_DEVICE,           ///< New device detected on the bus
    ONEWIRE_EVENT_SCAN_STARTED,         ///< Scan started
    ONEWIRE_EVENT_LOST_DEVICE,          ///< Connected device communication lost
    ONEWIRE_EVENT_SEARCH_COMPLETE,      ///< Search completed
    ONEWIRE_EVENT_UNKNOWN_DEVICE,       ///< Unknown device detected
    ONEWIRE_EVENT_BUS_SHORT,            ///< Short detected on the bus        
    ONEWIRE_EVENT_BUS_ERROR,            ///< Device communication resulted in bus error
    ONEWIRE_EVENT_LAST
} ONEWIRE_EVENT;

typedef void (*ONEWIRE_EVENT_HNDLR)(ONEWIRE_EVENT, ONEWIRE_DEVICE_ID);           ///< Event handler function pointer type

/*! \struct ONEWIREOPTIONS
 *  1-Wire bus configuration options.
 */
typedef struct                          
{           
    ONEWIRE_BUS             Bus;             ///< 1-Wire Bus    
    ONEWIRE_SPEED           Speed;           ///< Bus speed preference
    uint8_t                 DeviceCount;     ///< Device count threshold count to stop the scanning
    uint32_t                ScanInterval;    ///< Scan interval
    uint32_t                KeepAlive;       ///< Device handshake interval for detecting disconnect
    ONEWIRE_EVENT_HNDLR     pHandler;       ///< Notification to invoke during events
    ONEWIRE_DEVICE_FAMILY   Family[ONEWIRE_MAX_DEVICE_FAMILY]; ///< Support device family list
} ONEWIREOPTIONS;

/*! \struct ONEWIREPACKET
 *  1-Wire communication packet.
 */
typedef struct                          
{       
    uint8_t           *pTxData;        ///< Data to send out
    uint8_t           nTxSize;         ///< Data size to send out
    uint8_t           *pRxData;        ///< buffer to copy received data
    uint8_t           nRxSize;         ///< expected data receive size
} ONEWIREPACKET;        

/*! \struct ONEWIREFRAME
 *  1-Wire Packet Frame.
 */
typedef struct                          
{       
    ONEWIRE_DEVICE_ID Device;                   ///< 1-Wire device address
    ONEWIREPACKET Packets[ONEWIRE_MAX_PACKETS];  ///< 1-Wire packet segment list
    ONEWIRE_SEGMENT_HANDLER pHandler;           ///< Callback function to handle interim response
} ONEWIREFRAME;        

/*! \struct OWDEVICEINFO
 *  1-Wire Device information.
 */
typedef struct                          
{   
    ONEWIRE_DEVICE_ID       Device;             ///< 64 bit Device Address
    ONEWIRE_BUS             Bus;                ///< Bus ID on which device is found
    ONEWIRE_DEVICE_FAMILY   Family;             ///< Device family, duplicate from device address to simplify sorting
} OWDEVICEINFO;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern ONEWIRE_STATUS L3_OneWireInit(void);
extern ONEWIRE_STATUS L3_OneWireBusConfig(ONEWIREOPTIONS *pOptions);
extern ONEWIRE_STATUS L3_OneWireEnable(bool Enable);
extern ONEWIRE_STATUS L3_OneWireDeviceGetByFamily(ONEWIRE_DEVICE_FAMILY Family, ONEWIRE_DEVICE_ID *pList, uint8_t *pCount);
extern ONEWIRE_STATUS L3_OneWireDeviceGetByBus(ONEWIRE_BUS Bus, ONEWIRE_DEVICE_ID *pList, uint8_t *pCount);
extern ONEWIRE_STATUS L3_OneWireDeviceCheck(ONEWIRE_DEVICE_ID Device);
extern ONEWIRE_STATUS L3_OneWireTransfer(ONEWIREFRAME *pFrame);  
extern ONEWIRE_STATUS L3_OneWireAuthenticate(ONEWIRE_DEVICE_ID Device);
/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_ONEWIRECONTROLLER_H */
