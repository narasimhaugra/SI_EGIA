#ifndef L4_OWMEMORYFORMATS_H
#define L4_OWMEMORYFORMATS_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif
/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{

 * \brief   One wire memory formats.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_OwMemoryFormats.h
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
#define ONEWIRE_MEMORY_TOTAL_SIZE        (64u)              /*! One wire memory size */
#define HANDLE_MEM_USED                  (38u)              /*! Handle memory used */
#define CLAMSHELL_MEM_USED               (62u)              /*! Clamshell memory used */
#define ONEWIRE_BASIC_LAYOUT_MEM_USED    (6u)               /*! One wire basic layout memory used */
#define EGIA_ADAPTER_GEN_MEM_USED        (14u)              /*! EGIA Generic adapter memory used */
#define EGIA_ADAPTER_MEM_USED            (54u)              /*! EGIA Adapter memory used */
#define ONEWIRE_LOT_NUMBER_LENGTH        (15u)              /*! One wire lot number length */
#define EGIA_ADAPTER_SULU_MEM_USED       (44u)              /*! EGIA Adapter SULU memory used */
#define EGIA_ADAPTER_MULU_MEM_USED       (52u)              /*! EGIA Adapter MULU memory used */
#define EGIA_CARTRIDGE_MEM_USED          (60u)              /*! EGIA Cartridge memory used */
#define EEA_ADAPTER_MEM_USED             (50u)              /*! EEA Adapter memory used */
#define EEA_RELOAD_MEM_USED              (54u)              /*! EEA Reload memory used */
#define DEVICE_UNIQUE_ID                 ONEWIRE_DEVICE_ID  /*! Unique 1-Wire device identifier */
#define CLAMSHELL_DATA_VERSION           (2u)
#define ONEWIRE_MEMORY_DATA_SIZE         (ONEWIRE_MEMORY_TOTAL_SIZE - 2)
#define ONEWIRE_MEMORY_DATA_CRC_SIZE     (ONEWIRE_MEMORY_TOTAL_SIZE - ONEWIRE_MEMORY_DATA_SIZE)
#define BATTERY_MEMORY_USED              (1+2+ONEWIRE_LOT_NUMBER_LENGTH+2+1+2) //battery 1 wire EEPROM memory used,
                                                                               //should be updated if new data is added to memory

#define OW_READ(x,y,z) {memcpy (&z, (uint8_t *)&x.Data.y, sizeof(x.Data.y));}  /*! Read the OW EEPROM Macro */
/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/*! Memory formats for the One wire devices */
typedef __packed struct
{
   uint8_t    dataVersion;
   uint16_t   oneWireID;
   uint8_t    unused[ONEWIRE_MEMORY_TOTAL_SIZE - ONEWIRE_BASIC_LAYOUT_MEM_USED];
   uint8_t    writeTest;
   uint16_t   crc;
} BasicOneWireMemoryLayout_Ver2;

static_assert(sizeof(BasicOneWireMemoryLayout_Ver2) == ONEWIRE_MEMORY_TOTAL_SIZE,  "HandlBasicOneWireMemoryLayout_Ver2 is not 64 bytes");

typedef __packed struct
{
   uint8_t      DataVersion;
   uint16_t     DeviceType;
   uint8_t      LotNumber[ONEWIRE_LOT_NUMBER_LENGTH];
   uint16_t     FireCount;
   uint16_t     FireLimit;
   uint16_t     ProcedureCount;
   uint16_t     ProcedureLimit;
   uint8_t      StatusFlags;
   DEVICE_UNIQUE_ID LastClamshellAddress;
   uint8_t      Unused[ONEWIRE_MEMORY_TOTAL_SIZE - HANDLE_MEM_USED];
   uint8_t      WriteTest;
   uint16_t     Crc;
} MemoryLayoutHandle_Ver2;

static_assert(sizeof(MemoryLayoutHandle_Ver2) == ONEWIRE_MEMORY_TOTAL_SIZE,  "MemoryLayoutHandle_Ver2 is not 64 bytes");

typedef __packed struct
{
   uint8_t      DataVersion;
   uint16_t     DeviceType;
   uint8_t      StatusFlags;
   uint8_t      LotNumber[ONEWIRE_LOT_NUMBER_LENGTH];
   uint16_t     FiringHandleDeviceType;
   DEVICE_UNIQUE_ID FiringHandleAddress;
   uint16_t     FiringAdapter1DeviceType;
   DEVICE_UNIQUE_ID FiringAdapter1Address;
   uint16_t     FiringAdapter2DeviceType;
   DEVICE_UNIQUE_ID FiringAdapter2Address;
   uint16_t     FiringAdapter3DeviceType;
   DEVICE_UNIQUE_ID FiringAdapter3Address;
   uint8_t      Unused[ONEWIRE_MEMORY_TOTAL_SIZE - CLAMSHELL_MEM_USED];
   uint8_t      WriteTest;
   uint16_t     Crc;
} MemoryLayoutClamshell_Ver2;

static_assert(sizeof(MemoryLayoutClamshell_Ver2) == ONEWIRE_MEMORY_TOTAL_SIZE,  "MemoryLayoutClamshell_Ver2 is not 64 bytes");

typedef __packed struct
{
   uint8_t    DataVersion;
   uint16_t   DeviceType;
   uint16_t   FireCount;
   uint16_t   FireLimit;
   uint16_t   ProcedureCount;
   uint16_t   ProcedureLimit;
   uint8_t    Unused[ONEWIRE_MEMORY_TOTAL_SIZE - EGIA_ADAPTER_GEN_MEM_USED];
   uint8_t    WriteTest;
   uint16_t   Crc;
} MemoryLayoutGenericAdapter_Ver2;

static_assert(sizeof(MemoryLayoutGenericAdapter_Ver2) == ONEWIRE_MEMORY_TOTAL_SIZE,  "MemoryLayoutGenericAdapter_Ver2 is not 64 bytes");

typedef __packed struct
{
   uint8_t      DataVersion;
   uint16_t     DeviceType;
   uint16_t     FireCount;
   uint16_t     FireLimit;
   uint16_t     ProcedureCount;
   uint16_t     ProcedureLimit;
   uint16_t     FiringClamshellDeviceType;
   DEVICE_UNIQUE_ID FiringClamshellAddress;
   uint16_t     FiringHandleDeviceType;
   DEVICE_UNIQUE_ID FiringHandleAddress;
   uint16_t     FiringReloadDeviceType;
   DEVICE_UNIQUE_ID FiringReloadAddress;
   uint16_t     FiringCartridgeDeviceType;
   DEVICE_UNIQUE_ID FiringCartridgeAddress;
   uint8_t      Unused[ONEWIRE_MEMORY_TOTAL_SIZE - EGIA_ADAPTER_MEM_USED];
   uint8_t      WriteTest;
   uint16_t     Crc;
} MemoryLayoutEgiaAdapter_Ver2;

static_assert(sizeof(MemoryLayoutEgiaAdapter_Ver2) == ONEWIRE_MEMORY_TOTAL_SIZE,  "MemoryLayoutEgiaAdapter_Ver2 is not 64 bytes");

typedef __packed struct
{
   uint8_t    DataVersion;
   uint16_t   DeviceType;
   uint8_t    LotNumber[ONEWIRE_LOT_NUMBER_LENGTH];
   uint8_t    FireCount;
   uint8_t    InterlockZone;
   uint8_t    EndstopZone;
   uint8_t    StopPosition;
   uint8_t    ConfigFlags;
   uint8_t    asaLow;
   uint8_t    asaHigh;
   uint8_t    asaMax;
   uint8_t    ClampLow;
   uint8_t    ClampHigh;
   uint8_t    ClampMax;
   uint8_t    ClampForceMax;
   uint8_t    FireForceMax;
   uint8_t    StatusFlags;
   uint8_t    FireSlowPos;
   uint8_t    ArticFlagStrokeAtFire;
   uint8_t    ReloadColor;
   uint16_t   FiringHandleDeviceType;
   uint8_t    FiringHandleAddress;
   uint16_t   FiringAdapterDeviceType;
   uint8_t    FiringAdapterAddress;
   uint8_t    Unused[ONEWIRE_MEMORY_TOTAL_SIZE - EGIA_ADAPTER_SULU_MEM_USED];
   uint8_t    WriteTest;
   uint16_t   Crc;
} MemoryLayoutEgiaSulu_Ver2;

static_assert(sizeof(MemoryLayoutEgiaSulu_Ver2) == ONEWIRE_MEMORY_TOTAL_SIZE,  "MemoryLayoutEgiaSulu_Ver2 is not 64 bytes");

typedef __packed struct
{
   uint8_t      DataVersion;
   uint16_t     DeviceType;
   uint8_t      LotNumber[ONEWIRE_LOT_NUMBER_LENGTH];
   uint8_t      FireCount;
   uint8_t      FireLimit;
   uint8_t      InterlockZone;
   uint8_t      EndstopZone;
   uint8_t      StopPosition;
   uint8_t      ConfigFlags;
   uint8_t      ClampForceMax;
   uint8_t      FireForceMax;
   uint8_t      StatusFlags;
   uint8_t      FireSlowPos;
   uint8_t      ArticFlagStrokeAtFire;
   uint16_t     FiringHandleDeviceType;
   DEVICE_UNIQUE_ID FiringHandleAddress;
   uint16_t     FiringAdapterDeviceType;
   DEVICE_UNIQUE_ID FiringAdapterAddress;
   uint8_t      Unused[ONEWIRE_MEMORY_TOTAL_SIZE - EGIA_ADAPTER_MULU_MEM_USED];
   uint8_t      WriteTest;
   uint16_t     Crc;
} MemoryLayoutEgiaMulu_Ver2;

static_assert(sizeof(MemoryLayoutEgiaMulu_Ver2) == ONEWIRE_MEMORY_TOTAL_SIZE,  "MemoryLayoutEgiaMulu_Ver2 is not 64 bytes");

typedef __packed struct
{
   uint8_t      DataVersion;
   uint16_t     DeviceType;
   uint8_t      LotNumber[ONEWIRE_LOT_NUMBER_LENGTH];
   uint8_t      FireCount;
   uint8_t      configFlags;
   uint8_t      asaLow;
   uint8_t      asaHigh;
   uint8_t      asaMax;
   uint8_t      ClampLow;
   uint8_t      ClampHigh;
   uint8_t      ClampMax;
   uint8_t      ReloadColor;
   uint16_t     FiringHandleDeviceType;
   DEVICE_UNIQUE_ID FiringHandleAddress;
   uint16_t     FiringAdapterDeviceType;
   DEVICE_UNIQUE_ID FiringAdapterAddress;
   uint16_t     FiringReloadDeviceType;
   DEVICE_UNIQUE_ID FiringReloadAddress;
   uint8_t      Unused[ONEWIRE_MEMORY_TOTAL_SIZE - EGIA_CARTRIDGE_MEM_USED];
   uint8_t      WriteTest;
   uint16_t     Crc;
} MemoryLayoutEgiaCart_Ver2;

static_assert(sizeof(MemoryLayoutEgiaCart_Ver2) == ONEWIRE_MEMORY_TOTAL_SIZE,  "MemoryLayoutEgiaCart_Ver2 is not 64 bytes");

typedef __packed struct
{
   uint8_t      DataVersion;
   uint16_t     DeviceType;
   uint16_t     FireCount;
   uint16_t     FireLimit;
   uint16_t     ProcedureCount;
   uint16_t     ProcedureLimit;
   uint16_t     FiringClamshellDeviceType;
   DEVICE_UNIQUE_ID FiringClamshellAddress;
   uint16_t     FiringHandleDeviceType;
   DEVICE_UNIQUE_ID FiringHandleAddress;
   uint16_t     FiringReloadDeviceType;
   DEVICE_UNIQUE_ID FiringReloadAddress;
   uint8_t      Unused[ONEWIRE_MEMORY_TOTAL_SIZE - EEA_ADAPTER_MEM_USED];
   uint16_t     RecoveryItemError;
   uint16_t     RecoveryDataVer;
   uint16_t     RecoveryIDBackup;
   uint8_t      WriteTest;
   uint16_t     Crc;
} MemoryLayoutEeaAdapter_Ver2;

static_assert(sizeof(MemoryLayoutEeaAdapter_Ver2) == ONEWIRE_MEMORY_TOTAL_SIZE,  "MemoryLayoutEeaAdapter_Ver2 is not 64 bytes");

typedef __packed struct
{
   uint8_t      DataVersion;
   uint16_t     DeviceType;
   uint8_t      LotNumber[ONEWIRE_LOT_NUMBER_LENGTH];
   uint8_t      configFlags;
   uint8_t      ReloadColor;
   uint16_t     ClampForceMax;
   uint16_t     stapleForceMax;
   uint16_t     CutForceMax;
   uint16_t     FiringHandleDeviceType;
   DEVICE_UNIQUE_ID FiringHandleAddress;
   uint16_t     FiringAdapterDeviceType;
   DEVICE_UNIQUE_ID FiringAdapterAddress;
   uint16_t     stapleOffset;
   int16_t      kFactor;
   uint8_t      maxClampForce;
   uint8_t      Unused[ONEWIRE_MEMORY_TOTAL_SIZE - EEA_RELOAD_MEM_USED];
   uint8_t      WriteTest;
   uint16_t     Crc;
} MemoryLayoutEeaReload_Ver2;

static_assert(sizeof(MemoryLayoutEeaReload_Ver2) == ONEWIRE_MEMORY_TOTAL_SIZE,  "MemoryLayoutEeaReload_Ver2 is not 64 bytes");

typedef __packed struct
{
    uint8_t    DataVersion;
    uint16_t   OneWireID;
    uint8_t    LotNumber[ONEWIRE_LOT_NUMBER_LENGTH];
    uint16_t   ChargeCycleCount;
    uint8_t    Unused[ONEWIRE_MEMORY_TOTAL_SIZE - BATTERY_MEMORY_USED];
    uint8_t    WriteTest;
    uint16_t   Crc;
} MemoryLayoutBattery_Ver2;


static_assert(sizeof(MemoryLayoutBattery_Ver2) == ONEWIRE_MEMORY_TOTAL_SIZE,  "MemoryLayoutBattery_Ver2 is not 64 bytes");


/* Clamshell 1-Wire EEPROM Status Flag Enum */
typedef enum
{
   CLAMSHELL_STATUS_FLAG_REMOVED             = 0x01,
   CLAMSHELL_STATUS_FLAG_ONEWIRE_CONNECTED   = 0x02,
   CLAMSHELL_STATUS_FLAG_ONEWIRE_AUTHEN      = 0x04,
   CLAMSHELL_STATUS_FLAG_ONEWIRE_WRITEABLE   = 0x08,
   CLAMSHELL_STATUS_FLAG_ONEWIRE_DATA_GOOD   = 0x10,
   CLAMSHELL_STATUS_FLAG_ONEWIRE_PROCESSED   = 0x20,
   CLAMSHELL_STATUS_FLAG_USED                = 0x40,
   CLAMSHELL_STATUS_FLAG_DIRTY               = 0x80
} CLAMSHELL_STATUS_FLAG;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/**
 * \}
 */

#endif /* L4_OWMEMORYFORMATS_H */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
