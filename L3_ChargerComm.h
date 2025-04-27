#ifndef L3_CHARGERCOMM_H
#define L3_CHARGERCOMM_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif
/* ========================================================================== */
/**
 * \addtogroup L3_ChargerComm
 * \{

 * \brief   Public interface for Clamshell Definitions.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_ChargerComm.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"           ///< Import common definitions such as types, etc

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define CHARGER_COMM_CMD_WAIT      (100u)
#define CTS_DATA_SIZE               (30u)     ///< CTS data size   

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum
{
    CHARGER_COMM_STATUS_OK,
    CHARGER_COMM_STATUS_INVALID_PARAM,
    CHARGER_COMM_STATUS_ERROR,
    CHARGER_COMM_STATUS_COM_ERROR,
    CHARGER_COMM_STATUS_LAST
} CHARGER_COMM_STATUS;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
CHARGER_COMM_STATUS L3_ChargerCommPing(uint8_t *pDeviceType);
CHARGER_COMM_STATUS L3_ChargerCommGetVersion(uint16_t *pChargerVersion);
CHARGER_COMM_STATUS L3_ChargerCommSleep(void);
CHARGER_COMM_STATUS L3_ChargerCommStartCharging(void);
CHARGER_COMM_STATUS L3_ChargerCommStopCharging(void);
CHARGER_COMM_STATUS L3_ChargerCommSetAuthResult(bool Authenticated);
CHARGER_COMM_STATUS L3_ChargerCommReboot(void);
CHARGER_COMM_STATUS L3_ChargerCommGetError(int16_t *pErrorCode);
CHARGER_COMM_STATUS L3_ChargerCommSetPowerPackMaster(void);
CHARGER_COMM_STATUS L3_ChargerCommRelPowerPackMaster(void);
CHARGER_COMM_STATUS L3_ChargerCommQuery(void);
CHARGER_COMM_STATUS L3_ChargerCommSendCTSResponse(uint8_t *pCtsData, uint8_t CtsDataSize);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_CHARGERCOMM_H */
