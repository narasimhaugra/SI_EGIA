#ifndef SIGNIA_ADAPTEREVENTS_H
#define SIGNIA_ADAPTEREVENTS_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterEvents
 * \{

 * \brief   Public interface for Signia Adapter Publish events.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_AdapterEvents.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"                      /*! Import common definitions such as types, etc */
#include "Signia_AdapterManager.h"
#include "L4_AdapterDefn.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define AM_DEVICE_DATA_SIZE     (64u)                  /*! 1-Wire EEPROM memory size */
#define OW_ID_TYPE_MASK         (0xFC00)               /*! Mask to get the one wire type */
#define OW_ID_TYPE_SHIFT        (10u)                  /*! Bits to shift */
#define OW_ID_INSTANCE_MASK     (0x03FF)               /*! Instance Mask */
#define OW_ID_INSTANCE_SHIFT    (0)                    /*! Bits to shift for Instance*/
#define EGIA_ADAPTER_TYPE       (0x0801u)             /*!  0x801 is the EGIA, 1-wire device type*/
#define MAX_ITERATIONS          (3u)
/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef struct                      // Reload connected signal
{
    QEvt                Event;          ///< QPC event header
    AM_DEVICE           Device;         ///< Device type/variant
    DEVICE_UNIQUE_ID    DevAddr;        ///< Device device address
    bool                Authentic;      ///< True/False if device is Authentic or Not
    bool                DeviceWriteTest;///< 1-Wire Device Read/Write test status
    void                *pDeviceHandle;
} QEVENT_ADAPTER_MANAGER;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern AM_STATUS Signia_AdapterMgrEventPublish(AM_EVENT AmEvent, AM_DEVICE_INFO *pDeviceData);
extern AM_STATUS Signia_InitAdapterComm(void);
extern AM_STATUS Signia_GetStrainGauge(SG_FORCE *pSGData);
extern AM_DEVICE_STATUS Signia_GetHandleStatus(void);
extern AM_HANDLE_IF* Signia_GetHandleIF(void);
extern DEVICE_UNIQUE_ID Signia_GetHandleAddr(void);
extern uint16_t Signia_GetUartAdapterType(void);
extern AM_STATUS Signia_AdapterRequestCmd(ADAPTER_COMMANDS AdapCmd, uint32_t Delayms);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SIGNIA_ADAPTEREVENTS_H */

