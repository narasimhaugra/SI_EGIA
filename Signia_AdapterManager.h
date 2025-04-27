#ifndef L4_ADAPTERMANAGER_H
#define L4_ADAPTERMANAGER_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{

 * \brief   Public interface for Adapter Manager.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_AdapterManager.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"                      /*! Import common definitions such as types, etc */
#include "L3_GpioCtrl.h"
#include "L4_BatteryDefn.h"
#include "L4_HandleDefn.h"
#include "L4_AdapterDefn.h"
#include "L4_ClamshellDefn.h"
#include "L4_ReloadDefn.h"
#include "L4_CartridgeDefn.h"
#include "L4_DetachableCommon.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define AM_DEVICE_INFO_SIZE     (64u)                  ///< 1-Wire EEPROM memory size */
#define OW_ID_TYPE_MASK         (0xFC00)               ///< Mask to get the one wire type */
#define OW_ID_TYPE_SHIFT        (10u)                  ///< Bits to shift */
#define OW_ID_INSTANCE_MASK     (0x03FF)               ///< Instance Mask */
#define OW_ID_INSTANCE_SHIFT    (0)                    ///< Bits to shift for Instance*/

#define OW_MEMORY_TOTAL_SIZE             (64u)         /*! One wire memory size */
#define OW_EEPROM_PAGE_NUM               (0)           /*! EEPROM page number */
#define OW_EEPROM_PAGE_NUM2              (1u)          /*! EEPROM page 2 */
#define OW_EEPROM_PAGE_OFFSET            (32u)         /*! OFFSET for second EEPROM page */

#define ONEWIRE_ID(type, instance)     ((((INT16U)(((INT16U)(type)) << OW_ID_TYPE_SHIFT)) & OW_ID_TYPE_MASK) | \
                                        (((INT16U)(((INT16U)(instance)) << OW_ID_INSTANCE_SHIFT)) & OW_ID_INSTANCE_MASK)) ///< Form Device ID from type & instance
#define ONEWIRE_ID_TYPE(OneWireId)  ((((uint16_t)(OneWireId)) & OW_ID_TYPE_MASK) >> OW_ID_TYPE_SHIFT)            ///< Device type from Device ID
#define ONEWIRE_INSTANCE(OneWireId) ((((uint16_t)(OneWireId)) & OW_ID_INSTANCE_MASK) >> OW_ID_INSTANCE_SHIFT)    ///< Device instance from Device ID


/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

/// Device types ("Families") in Device ID
typedef enum
{
    DEVICE_TYPE_UNKNOWN1,       ///< Unknown ID
    DEVICE_TYPE_MISC,           ///< Handle, Clamshell,battery, charger
    DEVICE_TYPE_ADAPTER,        ///< Adapter
    DEVICE_TYPE_EGIA_SULU,      ///< EGIA SULU
    DEVICE_TYPE_EGIA_MULU,      ///< EGIA MULU
    DEVICE_TYPE_EGIA_RADIAL,    ///< EGIA RADIAL
    DEVICE_TYPE_EGIA_CART,      ///< EGIA Cartridge
    DEVICE_TYPE_EEA_RELOAD,     ///< EEA Reload
    DEVICE_TYPE_CHARGER,        ///< Charger
    DEVICE_TYPE_COUNT,          ///< Number of ID types
    DEVICE_TYPE_UNKNOWN2 = 0x3F,
} DEVICE_TYPE;

 typedef enum                       /// Misc Device types in Adapter Manager
 {
    DEVICE_UNKNOWN1,                ///< Unknown device type
    DEVICE_HANDLE,                  ///< Handle
    DEVICE_BATTERY,                 ///< Battery
    DEVICE_CLAMSHELL,               ///< Clamshell
    DEVICE_CHARGER,                 ///< Charger
    DEVICE_CV1,                     ///< Obsolete. Placeholder only
    DEVICE_UNKNOWN2 = 0x3FF,        ///< Unknown device type
 } DEVICE_INSTANCE_MISC;

/// Adapter instances. These are known as of v. 40. Some are obsolete, but kept as placeholders.
typedef enum
{
   DEVICE_ADAPTER_UNKNOWN1,
   DEVICE_ADAPTER_EGIA,
   DEVICE_ADAPTER_EGIA_RELIABILITY,
   DEVICE_ADAPTER_EEA,
   DEVICE_ADAPTER_FLEX_EEA,            ///< Obsolete, but kept as placeholder for following devices
   DEVICE_ADAPTER_EEA_RELIABILITY,
   DEVICE_ADAPTER_FLS,
   DEVICE_ADAPTER_ASAP,
   DEVICE_ADAPTER_FLEX_ASAP,
   DEVICE_ADAPTER_DEBUG_RELIABILITY,
   DEVICE_ADAPTER_ABS,
   DEVICE_ADAPTER_ABSA,
   DEVICE_ADAPTER_HAND_CAL,
   DEVICE_ADAPTER_ASAP_360,
   DEVICE_ADAPTER_COUNT,
   DEVICE_ADAPTER_UNKNOWN2 = 0x3FF,  // binary 11 1111 1111
} DEVICE_INSTANCE_ADAPTER;

/// SULU instances. These are known as of v. 40.
typedef enum
{
   DEVICE_EGIA_SULU_UNKNOWN1,
   DEVICE_EGIA_SULU_30,
   DEVICE_EGIA_SULU_45,
   DEVICE_EGIA_SULU_60,
   DEVICE_EGIA_SULU_COUNT,
   DEVICE_EGIA_SULU_UNKNOWN2 = 0x3FF,  // binary 11 1111 1111
} DEVICE_INSTANCE_EGIA_SULU;

typedef enum
{
   DEVICE_EGIA_MULU_UNKNOWN1,
   DEVICE_EGIA_MULU_30,
   DEVICE_EGIA_MULU_45,
   DEVICE_EGIA_MULU_60,
   DEVICE_EGIA_MULU_COUNT,
   DEVICE_EGIA_MULU_UNKNOWN2 = 0x3FF,  // binary 11 1111 1111
} DEVICE_INSTANCE_EGIA_MULU;

typedef enum
{
   DEVICE_EGIA_CART_UNKNOWN1,
   DEVICE_EGIA_CART_30,
   DEVICE_EGIA_CART_45,
   DEVICE_EGIA_CART_60,
   DEVICE_EGIA_CART_COUNT,
   DEVICE_EGIA_CART_UNKNOWN2 = 0x3FF,  // binary 11 1111 1111
} DEVICE_INSTANCE_EGIA_CART;

typedef enum
{
   DEVICE_EGIA_RADIAL_UNKNOWN1,
   DEVICE_EGIA_RADIAL_30,
   DEVICE_EGIA_RADIAL_45,
   DEVICE_EGIA_RADIAL_60,
   DEVICE_EGIA_RADIAL_COUNT,
   DEVICE_EGIA_RADIAL_UNKNOWN2 = 0x3FF,  // binary 11 1111 1111
} DEVICE_INSTANCE_EGIA_RADIAL;

typedef enum
{
   DEVICE_EEA_RELOAD_UNKNOWN1,
   DEVICE_EEA_RELOAD_21,
   DEVICE_EEA_RELOAD_25,
   DEVICE_EEA_RELOAD_28,
   DEVICE_EEA_RELOAD_31,
   DEVICE_EEA_RELOAD_33,
   DEVICE_EEA_RELOAD_COUNT,
   DEVICE_EEA_RELOAD_UNKNOWN2 = 0x3FF,  // binary 11 1111 1111
} DEVICE_INSTANCE_EEA_RELOAD;

/// Currently defined Device IDs
typedef enum
{
   DEVICE_ID_HANDLE                      = ONEWIRE_ID(DEVICE_TYPE_MISC, DEVICE_HANDLE),
   DEVICE_ID_BATTERY                     = ONEWIRE_ID(DEVICE_TYPE_MISC, DEVICE_BATTERY),
   DEVICE_ID_CLAMSHELL                   = ONEWIRE_ID(DEVICE_TYPE_MISC, DEVICE_CLAMSHELL),
   DEVICE_ID_CHARGER                     = ONEWIRE_ID(DEVICE_TYPE_MISC, DEVICE_CHARGER),
   DEVICE_ID_CV1                         = ONEWIRE_ID(DEVICE_TYPE_MISC, DEVICE_CV1),    ///< Obsolete, historical reference

   DEVICE_ID_ADAPTER_EGIA                = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_EGIA),
   DEVICE_ID_ADAPTER_EGIA_RELIABILITY    = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_EGIA_RELIABILITY),
   DEVICE_ID_ADAPTER_EEA                 = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_EEA),
   DEVICE_ID_ADAPTER_FLEX_EEA            = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_FLEX_EEA),
   DEVICE_ID_ADAPTER_EEA_RELIABILITY     = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_EEA_RELIABILITY),
   DEVICE_ID_ADAPTER_FLS                 = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_FLS),
   DEVICE_ID_ADAPTER_ASAP                = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_ASAP),
   DEVICE_ID_ADAPTER_FLEX_ASAP           = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_FLEX_ASAP),
   DEVICE_ID_ADAPTER_DEBUG_RELIABILITY   = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_DEBUG_RELIABILITY),
   DEVICE_ID_ADAPTER_ABS                 = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_ABS),
   DEVICE_ID_ADAPTER_ABSA                = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_ABSA),
   DEVICE_ID_ADAPTER_HAND_CAL            = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_HAND_CAL),
   DEVICE_ID_ADAPTER_ASAP_360            = ONEWIRE_ID(DEVICE_TYPE_ADAPTER, DEVICE_ADAPTER_ASAP_360),

   DEVICE_ID_EGIA_SULU_30                = ONEWIRE_ID(DEVICE_TYPE_EGIA_SULU, DEVICE_EGIA_SULU_30),
   DEVICE_ID_EGIA_SULU_45                = ONEWIRE_ID(DEVICE_TYPE_EGIA_SULU, DEVICE_EGIA_SULU_45),
   DEVICE_ID_EGIA_SULU_60                = ONEWIRE_ID(DEVICE_TYPE_EGIA_SULU, DEVICE_EGIA_SULU_60),

   DEVICE_ID_EGIA_RADIAL_30              = ONEWIRE_ID(DEVICE_TYPE_EGIA_RADIAL, DEVICE_EGIA_RADIAL_30),
   DEVICE_ID_EGIA_RADIAL_45              = ONEWIRE_ID(DEVICE_TYPE_EGIA_RADIAL, DEVICE_EGIA_RADIAL_45),
   DEVICE_ID_EGIA_RADIAL_60              = ONEWIRE_ID(DEVICE_TYPE_EGIA_RADIAL, DEVICE_EGIA_RADIAL_60),

   DEVICE_ID_EGIA_MULU_30                = ONEWIRE_ID(DEVICE_TYPE_EGIA_MULU, DEVICE_EGIA_MULU_30),
   DEVICE_ID_EGIA_MULU_45                = ONEWIRE_ID(DEVICE_TYPE_EGIA_MULU, DEVICE_EGIA_MULU_45),
   DEVICE_ID_EGIA_MULU_60                = ONEWIRE_ID(DEVICE_TYPE_EGIA_MULU, DEVICE_EGIA_MULU_60),

   DEVICE_ID_EGIA_CART_30                = ONEWIRE_ID(DEVICE_TYPE_EGIA_CART, DEVICE_EGIA_CART_30),
   DEVICE_ID_EGIA_CART_45                = ONEWIRE_ID(DEVICE_TYPE_EGIA_CART, DEVICE_EGIA_CART_45),
   DEVICE_ID_EGIA_CART_60                = ONEWIRE_ID(DEVICE_TYPE_EGIA_CART, DEVICE_EGIA_CART_60),

   DEVICE_ID_EEA_RELOAD_21               = ONEWIRE_ID(DEVICE_TYPE_EEA_RELOAD, DEVICE_EEA_RELOAD_21),
   DEVICE_ID_EEA_RELOAD_25               = ONEWIRE_ID(DEVICE_TYPE_EEA_RELOAD, DEVICE_EEA_RELOAD_25),
   DEVICE_ID_EEA_RELOAD_28               = ONEWIRE_ID(DEVICE_TYPE_EEA_RELOAD, DEVICE_EEA_RELOAD_28),
   DEVICE_ID_EEA_RELOAD_31               = ONEWIRE_ID(DEVICE_TYPE_EEA_RELOAD, DEVICE_EEA_RELOAD_31),
   DEVICE_ID_EEA_RELOAD_33               = ONEWIRE_ID(DEVICE_TYPE_EEA_RELOAD, DEVICE_EEA_RELOAD_33),
   DEVICE_ID_UNKNOWN                     = 0xFFFF,
} DEVICE_ID_ENUM;

/// \todo 01/20/2022 DAZ - NOTE: THESE ARE DEVICE **CLASSES**, NOT TYPE IN THE TYPE/INSTANCE SENSE.
/// \todo 01/20/2022 DAZ - NOTE: HANDLE **MUST** COME FIRST. SCAN THIS **BEFORE** ALL OTHERS!
/// \todo 01/25/2022 DAZ - typedef probably should be called AM_DEVICE_CLASS to avoid confusion
/// Device types in Adapter Manager
/// Don't Change the ENUM Order
 typedef enum
 {
    AM_DEVICE_HANDLE,              ///< Handle
    AM_DEVICE_CLAMSHELL,           ///< Clamshell
    AM_DEVICE_ADAPTER,             ///< Adapter
    AM_DEVICE_RELOAD,              ///< Reload
    AM_DEVICE_CARTRIDGE,           ///< Cartridge
    AM_DEVICE_BATTERY,             ///< Battery
    AM_DEVICE_COUNT                ///< Last Device type
 } AM_DEVICE;

typedef enum                         /// Adapter Manager event types
{
    AM_EVENT_NONE,
    AM_EVENT_NEW_DEVICE,             ///< New Device
    AM_EVENT_LOST_DEVICE,            ///< Lost
    AM_EVENT_FAULT,                  ///< Fault
    AM_EVENT_INVALID,                ///< Invalid or Unrecognized device
    AM_EVENT_ADAPTER_DATA,           ///< Adapter data
    AM_EVENT_COUNT,                  ///< Number of events
} AM_EVENT;

/// Individual detachable device states (including handle)
typedef enum
{
    AM_DEVICE_STATE_NO_DEVICE,      ///< No device
    AM_DEVICE_STATE_AUTHENTICATE,   ///< Authenticate
    AM_DEVICE_STATE_EEPROM_TEST,    ///< Test the EEPROM
    AM_DEVICE_STATE_ACTIVE,         ///< Active
    AM_DEVICE_STATE_INVALID,        ///< Invalid
    AM_DEVICE_STATE_CRCFAIL,        ///< CRC Fail
    AM_DEVICE_STATE_SHORT,          ///< Short
    AM_DEVICE_STATE_COUNT           ///< Number of states
} AM_DEVICE_STATE;

/// Adapter Manager states based on the device connection status
typedef enum
{
    AM_STATE_DISARMED,               ///< Disarmed
    AM_STATE_CLAMSHELL_ARMED,        ///< Clamshell armed
    AM_STATE_ADAPTER_ARMED,          ///< Adapter armed
    AM_STATE_RELOAD_ARMED,           ///< Reload armed
    AM_STATE_CARTRIDGE_ARMED,        ///< Cartridge armed
    AM_STATE_COUNT                   ///< Number of states
} AM_STATE;

///< Device Information to store based  on the connection status
typedef struct                          ///< Device information
{
    bool               Present;        ///< Device Detected
    AM_DEVICE          Device;         ///< Device - Adapter, Clamshell, Reload, Cartridge
    DEVICE_TYPE        DeviceType;     ///< 2 byte device type
    DEVICE_UNIQUE_ID   DeviceUID;      ///< Device unique identifier
    AM_DEVICE_STATE    State;          ///< Device state
    bool               Writable;       ///< True if device is writable
    bool               Authentic;      ///< True/False if device is Authentic or Not
    bool               DeviceCrcFail;      ///< True/False if device is Authentic or Not
    bool               DeviceWriteTest;///< 1-Wire Device Read/Write status
    bool               DeviceUnsupported;
    void               *pDevHandle;    ///< Device interface access handle
} AM_DEVICE_INFO;

typedef void (*AM_HANDLER)(AM_EVENT Event, uint64_t *pData);              ///< Event handler function

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern AM_STATUS L4_AdapterManagerInit(void);
extern AM_STATUS Signia_AdapterManagerRegisterHandler(AM_HANDLER pHandler);
extern AM_STATE Signia_AdapterManagerGetState(void);
extern void* Signia_AdapterManagerDeviceHandle(AM_DEVICE Device);
extern AM_STATUS Signia_AdapterManagerGetInfo(AM_DEVICE Device, AM_DEVICE_INFO *pInfo);
extern AM_STATUS Signia_AdapterMgrEventPublish(AM_EVENT AmEvent, AM_DEVICE_INFO *pDeviceData);
extern bool Signia_IsReloadConnected(void);
extern void L4_AdapterUartComms(bool state);
extern bool Signia_GetAdapterStatus(void);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_ADAPTERMANAGER_H */

