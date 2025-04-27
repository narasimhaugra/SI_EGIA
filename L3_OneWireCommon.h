#ifndef L3_ONEWIRECOMMON_H
#define L3_ONEWIRECOMMON_H

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
 * \file    L3_OneWireCommon.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"               ///< Import common definitions such as types, etc

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/*****************************************************************************/
#define ONEWIRE_MAX_DEVICES         (10u)    ///< Maximum 1-Wire slave devices used in the system.
#define ONEWIRE_MAX_DEVICE_FAMILY   (5u)     ///< Maximum family types support on each bus
#define ONEWIRE_DEVICE_ID_INVALID   (0xFFFFFFFFFFFFFFFFu)     ///< Invalid Bus ID
#define ONEWIRE_ADDR_LENGTH        (sizeof(ONEWIRE_DEVICE_ID))  ///< Size of 1-Wire address in bytes

#define ONEWIRE_MEMORY_BANK_SIZE     (32u)                                                      ///< Memory bank size to store 1-wire data
#define ONEWIRE_MEMORY_TEMPDATA_SIZE (76u)                                                      ///< Memory bank maximum size to store 1-wire data
/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef uint64_t ONEWIRE_DEVICE_ID;          ///< 64 bit 1-Wire device ID

/*! \enum OW_SCAN_TYPE
 *  1-Wire Device scan type.
 */
typedef enum
{
    OW_SCAN_TYPE_FULL,                      ///< New scan, current device list is deleted
    OW_SCAN_TYPE_ALARMS,                    ///< Scan devices with alarm conditions
    OW_TRANSPORT_SCAN_LAST                  ///< Scan type range indicator
}OW_SCAN_TYPE;

/*! \enum ONEWIRE_DEVICE_FAMILY
 *  1-Wire Device family.
 */
typedef enum
{
    ONEWIRE_DEVICE_FAMILY_ALL       = 0x00, ///< 1-Wire device family all
    ONEWIRE_DEVICE_FAMILY_RTC       = 0x27, ///< 1-Wire device family RTC
    ONEWIRE_DEVICE_FAMILY_EEPROM    = 0x17, ///< 1-Wire device family EEPROM
    ONEWIRE_DEVICE_FAMILY_LAST      = 0xFF  ///< 1-Wire device family range indicator
}ONEWIRE_DEVICE_FAMILY;

/*! \enum ONEWIRE_STATE
 *  1-Wire State.
 */
typedef enum
{
   ONEWIRE_STATE_ENABLE,                ///< Enabled
   ONEWIRE_STATE_DISABLE,               ///< Disabled
   ONEWIRE_LAST                         ///< Range indicator
} ONEWIRE_STATE;

/*! \enum ONEWIRE_SPEED
 *  1-Wire bus speed.
 */
typedef enum
{
    ONEWIRE_SPEED_STD,                  ///< 1-Wire bus speed standard
    ONEWIRE_SPEED_OD,                  ///< 1-Wire bus speed standard
    ONEWIRE_SPEED_COUNT                 ///< 1-Wire bus speed range indicator
} ONEWIRE_SPEED;

/*! \enum ONEWIRE_BUS
 *  1-Wire bus available in the system.
 */
typedef enum
{
    ONEWIRE_BUS_CLAMSHELL,              ///< 1-Wire bus ports clamshell
    ONEWIRE_BUS_EXP,                    ///< 1-Wire expansion bus (RTC)
    ONEWIRE_BUS_LOCAL,                  ///< 1-Wire local bus (battery, charger, handle)
    ONEWIRE_BUS_CONNECTORS,             ///< 1-Wire connector bus(adapter, reload, cartridge)
    ONEWIRE_BUS_COUNT                  ///< **** CAUTION : MUST NOT ASSIGN NUMBERS TO ANY ENUMS ****
} ONEWIRE_BUS;

/*! \enum ONEWIRE_STATUS
 *  1-Wire status.
 */
typedef enum
{
    ONEWIRE_STATUS_OK,                  ///< All good
    ONEWIRE_STATUS_WAIT,                ///< Any use?
    ONEWIRE_STATUS_BUSY,                ///< 1-Wire bus is busy
    ONEWIRE_STATUS_ERROR,               ///< General error
    ONEWIRE_STATUS_READ_ERROR,          ///< 1-wire bus Read error
    ONEWIRE_STATUS_WRITE_ERROR,         ///< 1-wire bus Write error
    ONEWIRE_STATUS_BUS_ERROR,           ///< 1-Wire bus error, probably a short?
    ONEWIRE_STATUS_NO_DEVICE,           ///< Specified Device not found
    ONEWIRE_STATUS_TIMEOUT,             ///< Request timed out
    ONEWIRE_STATUS_Q_FULL,              ///< Q full, can't take any more request
    ONEWIRE_STATUS_PARAM_ERROR,         ///< One or more invalid parameter specified
    ONEWIRE_STATUS_DISABLED,            ///< 1-Wire bus is disabled
    ONEWIRE_STATUS_NVMTEST_ERROR,        ///< 1-Wire NVM Test fail
    ONEWIRE_STATUS_LAST                 ///< Status range indicator
} ONEWIRE_STATUS;

/*! \struct OWSearchContext
 *  1-Wire search Context.
 */
typedef struct
{
    ONEWIRE_BUS Bus;                    ///< Bus Id
    ONEWIRE_DEVICE_ID RomId;            ///< Device Address
    uint8_t LastConflict;               ///< Last 0,1 conflict
    bool    LastDevice;                 ///< End of device on the bus
    OW_SCAN_TYPE ScanType;              ///< Scan type
}OWSearchContext;

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_ONEWIRECOMMON_H */
