#ifndef L4BATTERYHEALTHCHECK_H
#define L4BATTERYHEALTHCHECK_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/** * \addtogroup Charger
 * \{
 * \brief Brief Description.
 *
 * \details <if necessary>
 *
 * \bug  <if necessary>
 *
 * \copyright  2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * * \file  Signia_BatteryHealthCheck.h
 *
* ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"           ///< Import common definitions such as types, etc
#include "Signia_ChargerManager.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define HIGH_VOLTAGE_THRESH_mv              (8280u)    ///< Above this voltage sleep for 60 min
#define MED_VOLTAGE_THRESH_mv               (8260u)    ///< Above this voltage sleep for 15 min
#define LOW_VOLTAGE_THRESH_mv               (8250u)    ///< Below this voltage sleep for 5 minutes
#define MAX_BATTERY_VOLTAGE_mv              (9000u)    ///< max voltage reported by battery, in millivolts (for sanity check)
#define MAX_CHARGE_CURRENT_ma               (2000u)
#define MIN_CHARGE_CURRENT_ma               (-1000)

#define BATTERY_HEALTH_RELAX_INTERVAL_min   (15u)      ///< After charging, this time to allow the battery to relax. (Settle). The term Relax is used by batteries manufacturers.
#define BATTERY_HEALTH_SLEEP_INTERVAL_min   (20u)      ///< 20min sleep interval
#define BATTERY_HEALTH_MAX_TIME_min         (180u)     ///< 180min max time for Maintainance period

#define BATTERY_FULL_CHARGE_maHr            (2050u)    ///< Battery Full charge in mAh

#define MAX_HEALTH_TEMP_DELTA_ALLOWED       (10u)     ///< Max Limit for Delta Temperuate
#define HEALTH_ABSOLUTE_CELL_TEMP_LIMIT     (30u)     ///< Max Limit for Cell Temperature
#define MAX_IMPLIED_CURR_LIMIT_WITH_MAINT_PERIOD_ma (40u)        ///< Max limit for Implied current in mA if there is Maintainance period
#define MAX_IMPLIED_CURR_LIMIT_NO_MAINT_PERIOD_ma   (60u)        ///< Max limit for Implied current in mA if there is NO Maintainance period

#define TCA_BIT_MASK                                (0x4000u)
#define TCA_IS_ON                           (1u)
#define TCA_IS_OFF                          (0u)
#define MFG_STATUS_FET_ENABLED_BIT                  (0x10u)
#define OP_STATUS_DISCHARGE_FET_ENABLED_BIT         (0x02u)
#define OP_STATUS_PERM_FAIL_BIT                     (0x1000u)

#define BATTERY_DISCHARGE_CURRENT_THRESHOLD_ma      (-15)     ///< The Battery is discharging if curr < this theshold

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum                                ///< Battery Health check states
{
   BHC_INIT,
   BHC_STATE_TCA_IS_OFF,                    ///< TCA off state
   BHC_STATE_DISCHARGE_SETTLE_TIME_15MIN,   ///< Allow the battery voltage to settle after charge is complete
   BHC_STATE_DISCHARGE_CHECK_IMPLIED_CURR,  ///< Maintainance Period
   BHC_STATE_HAS_MAINT_PERIOD_WAIT_TO_FINISH, ///< Wait for Finish
   BHC_STATE_NO_MAINT_PERIOD_WAIT_UNTIL_TCA_IS_OFF, ///< Wait for TCA off if no Maintainance period
   BHC_CHRGCYCLE_LAST,
} BATTERY_HEALTHCHECK_STATES;

typedef struct
{
    uint16_t CurrImpliedCurr;   ///< Implied current for current cycle in mA
    uint16_t PrevImpliedCurr;   ///< Implied current from previous cycle in mA
    uint16_t ImpliedCurrLimit;  ///< Limit for implied current
    float32_t TemperatureDelta; ///< Temperature Delta
    float32_t PrevTemperatureDelta; ///< Previous Temperature Delat
    float32_t CellTemp;                 ///< Cell Temperature
    float32_t PrevCellTemp;             ///< Previous Cell Temperature

    int16_t Chargecurrent;              ///< Charge current
    uint32_t BatteryOperationSts;            ///< Battery Operation Status
    uint16_t StartVoltage;              ///< Start Voltage at Maintaince period start
    uint16_t EndVoltage;                ///< End Voltage

    uint16_t RSOCStart;                 ///< Start RSOC
    uint16_t RSOCEnd;                   ///< End RSOC
    uint16_t RSOCDelta;                 ///< Delta RSOC
    uint16_t numMinutes;

    uint16_t WakeupOnChargerCount;      ///< On Charger Wake count
    uint16_t TimeToLogCount;            ///< Time to log
    uint16_t TotalSleepTime;            ///< Total Sleep time
    int8_t MaintenanceCount;            ///< Maintainance period wake count
    BATTERY_HEALTHCHECK_STATES HealthCheckState;  ///< Battery Health check state

    bool HasMaintainancePeriod;         ///< Has Maintainance period
    bool BatteryIsHealthy;              ///< Is battery Healthy

} BATTERYHEALTHPARAM;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
void Signia_BatteryHealthCheck(CHARGER_INFO *pInfo);
void Signia_BatteryHealthCheckReset(void);
uint32_t Signia_BatteryHealthGetNextSleepTime(void);

/**
 * \}  <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4BATTERYHEALTHCHECK_H */
