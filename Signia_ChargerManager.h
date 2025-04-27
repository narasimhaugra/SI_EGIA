#ifndef L4_CHARGERMANAGER_H
#define L4_CHARGERMANAGER_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_ChargerManager
 * \{

 * \brief   Public interface for Clamshell Definitions.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_ChargerManager.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"           ///< Import common definitions such as types, etc

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define BATT_MAX_CHARGECYCLECOUNT   (300u)      ///< Battery Maximum Charge Cycle Count
#define BATTERY_TCABIT              (14u)      ///< Battery TCA bit position
#define BATTERY_LIMIT_LOW           (25.0f)      ///< Low battery level limit
#define BATTERY_LIMIT_LOW_MIN       (10.0f)      ///< Low battery level limit minimum
#define BATTERY_LIMIT_INSUFFICIENT  (9.0f)       ///< Insufficient battery level limit
#define GET_BIT(val,pos)            ((val & (1<<pos))? true: false)
  
#define BATT_RSOCSHUTDOWN          (7.0f)      ///< Battery RSOC shutdown level

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
///< Function call status
typedef enum
{
   CHRG_MNGR_STATUS_OK,                 ///< All good
   CHRG_MNGR_STATUS_INVALID_PARAM,      ///< Invalid parameter supplied
   CHRG_MNGR_STATUS_ERROR,              ///< Error
   CHRG_MNGR_STATUS_COM_ERROR,          ///< Charger spi communication error
   CHRG_MNGR_STATUS_LAST                ///< End of list mark
} CHRG_MNGR_STATUS;

///< Charger Manager Status Flags
typedef enum
{
   CHRG_MNGR_STATE_DISCONNECTED,        ///< Handle not on the charger
   CHRG_MNGR_STATE_CONNECTED,           ///< Handle on the charger
   CHRG_MNGR_STATE_CHARGING,            ///< Handle on the charger, charging in progress
   CHRG_MNGR_STATE_CHARGED,             ///< Handle on the charger, charge level reached desired set level
   CHRG_MNGR_STATE_FAULT,               ///< Handle on the charger, encountered fault
   CHRG_MNGR_STATE_SLEEP,               ///< Handle on the charger, charging disabled/aborted
   CHRG_MNGR_STATE_LAST                ///< List end mark
} CHRG_MNGR_STATE;

///< Charger Event Enumeration
typedef enum
{
   CHARGER_EVENT_DISCONNECTED,      ///< Charger disconnected
   CHARGER_EVENT_CONNECTED,         ///< Charger connected
   CHARGER_EVENT_FAULT,             ///< Charger fault
   CHARGER_EVENT_DATA,              ///< Charger/Battery info
   CHARGER_EVENT_WAKEUPONCHARGER,   ///< Wakeup on charger to check battery health
   CHARGER_EVENT_LAST               ///< Last enumeration
} CHARGER_EVENT;

///< Battery type
typedef enum
{
    BATTERY_DEFAULT       = 0,
    BATTERY_PANASONIC     = 0,          ///< Panasonic Battery
    BATTERY_MOLICELL      = 1,          ///< Moli Cell type
    BATTERY_TYPE_LAST     = 2
} CHGR_MNGR_BATTERYTYPES;

typedef struct
{
    uint16_t ChemID;
    uint16_t ChargeCapacity;
    const uint16_t *pRSOCLUT;
    uint32_t MinWakeTimeinms;
} CHGR_MNGR_BATTERYDESIGNPARAMS;

///< Charger Manager information
typedef struct
{
    CHARGER_EVENT Event;                ///< Event notified
    float32_t BatteryLevel;             ///< Battery (Rsoc)level
    uint16_t BatteryVoltage;            ///< battery Voltage
    int16_t BatteryCurrent;             ///< Battery current
    float32_t  BatteryTemperature;      ///< Battery temperature
    uint16_t BatChgrCntCycle;           ///< Battery Charger Count Cycle
    uint16_t BatteryCell0Voltage;       ///< Battery Cell0 Voltage in mV
    uint16_t BatteryCell1Voltage;       ///< Battery Cell1 Voltage in mV
    uint16_t BatteryLevelBQ;            ///< Battery (RSOC) level from BQ chip in %
    uint16_t BatteryChargeSts;          ///< Battery Charging Status
    uint16_t BatteryGaugSts;            ///< Battery Gauging Satus
    uint32_t BatterySafetySts;          ///< Battery Safety Status
    uint32_t BatteryOperationSts;       ///< Battery Operation Status
    uint32_t BatteryPFSts;              ///< Battery Permanent Fail Status
    CHGR_MNGR_BATTERYTYPES BatteryType; ///< Battery Type
    const CHGR_MNGR_BATTERYDESIGNPARAMS *pBattParam;     /// pointer to the Battery design parameter table
    float32_t InternalTemperature;
    float32_t TS1Temperature;
    float32_t TS2Temperature;
    bool IsValid;
    CHRG_MNGR_STATE BatCommState;       ///< Battery Communication Status
    uint16_t RemainingChargeCycleCount;
} CHARGER_INFO;

typedef void (*CHARGER_HANDLER)(CHARGER_INFO *pChargerInfo); ///< Event handler function pointer type

extern const char BatteryManufacturer[2][10];  ///< Battery manufacturer

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
CHRG_MNGR_STATUS L4_ChargerManagerInit(void);
CHRG_MNGR_STATE  Signia_ChargerManagerGetState(void);
CHRG_MNGR_STATUS Signia_ChargerManagerGetBattRsoc(uint8_t *pBatteryRsoc);
CHRG_MNGR_STATUS Signia_ChargerManagerGetBattCur(uint16_t *pBatteryCurrent);
CHRG_MNGR_STATUS Signia_ChargerManagerSetChargeLimits(uint8_t LimitHigh, uint8_t LimitLow);
CHRG_MNGR_STATUS Signia_ChargerManagerSleep(void);
CHRG_MNGR_STATUS Signia_ChargerManagerRegEventHandler(CHARGER_HANDLER pChargerHandler, uint16_t Period);
CHARGER_INFO* Signia_ChargerManagerGetChargerInfo(void);
uint16_t Signia_ChargerManagerGetChgrCntCycle(void);
void Signia_ChargerManagerRSOCCalAllowed(bool state);
float32_t BatteryCalculateRSOC(uint16_t voltage);
void Signia_ChargerManagerSetWakupstate(bool state);
/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_CHARGERMANAGER_H */

