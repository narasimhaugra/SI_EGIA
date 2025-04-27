#ifndef L4_DETACHABLECOMMON_H
#define L4_DETACHABLECOMMON_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{

 * \brief   Adapter Manager common symbols definitions.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_DetachableCommon.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"                      /*! Import common definitions such as types, etc */
#include "L3_OneWireCommon.h"
#include "L3_OneWireEeprom.h"
#include "L4_OwMemoryFormats.h"
#include "L3_OneWireController.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define DEVICE_UNIQUE_ID                ONEWIRE_DEVICE_ID               /*! Unique 1-Wire device identifier */
#define OW_EEP_MEM_SIZE                 (64u)                           /*! 1-Wire Eeprom memory size */
#define MEM_LAYOUT_HANDLE               MemoryLayoutHandle_Ver2
#define MEM_LAYOUT_ADAPTER              MemoryLayoutEgiaAdapter_Ver2
#define MEM_LAYOUT_CLAMSHELL            MemoryLayoutClamshell_Ver2
#define MEM_LAYOUT_RELOAD               MemoryLayoutEgiaMulu_Ver2     // \todo: How to detect type of reload?
#define MEM_LAYOUT_CARTRIDGE            MemoryLayoutEgiaCart_Ver2
#define MEM_LAYOUT_BATTERY              MemoryLayoutBattery_Ver2

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum                                /*! Adapter Manager Status */
{
    AM_STATUS_OK,                           /*! Success */
    AM_STATUS_INVALID_PARAM,                /*! Invalid parameter */
    AM_STATUS_DISCONNECTED,                 /*! Disconnected status */
    AM_STATUS_ERROR,                        /*! Error */
    AM_STATUS_ERROR_UPGRADE,
    AM_STATUS_DATA_CRC_FAIL,
    AM_STATUS_CRC_FAIL,
    AM_STATUS_TIMEOUT,
    AM_STATUS_WAIT,
    AM_STATUS_LAST                          /*! Last status  */
} AM_STATUS;

typedef enum                                /*! Adapter Manager state machine */
{
   AM_IDLE,                                 /*! Idle state */
   AM_INIT,                                 /*! Init state */
   AM_BOOT,                                 /*! Boot state */
   AM_MAIN,                                 /*! Main state */
   AM_WAIT,                                 /*! Wait state */
   AM_FAULT,                                /*! Fault state */
   AM_COUNT                                 /*! Last states*/
} AM_SM_STATE;

typedef enum                ///< Detatchable device status
{
    AM_DEVICE_DISCONNECTED, ///< Success
    AM_DEVICE_CONNECTED,    ///< Invalid parameter
    AM_DEVICE_AUTH_FAIL,    ///< Disconnected status
    AM_DEVICE_ACCESS_FAIL,  ///< Error
    AM_DEVICE_LAST          ///< Device status list end

} AM_DEVICE_STATUS;

typedef AM_STATUS (*AM_DEFN_IF)(void);                   /*! General interface function */
typedef AM_STATUS (*AM_DEFN_CMD_IF)(uint8_t*);           /*! General interface function */
typedef bool (*AM_DEFN_STATUS_IF)(void);
typedef AM_STATUS (*AM_DEFN_EEP_UPDATE)(void);          /*! EEPROM Read/Write interface function */
typedef AM_STATUS (*AM_DEFN_BATT_EEP_READ)(void);



/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_DETACHABLECOMMON_H */

