#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/** \addtogroup Charger
 * \{
 * \brief
 *
 * \details
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file Signia_BatteryHealthCheck.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Signia_BatteryHealthCheck.h"
#include "Signia_PowerControl.h"
#include "Signia_CommManager.h"
#include "L3_Battery.h"
#include "L3_GpioCtrl.h"
#include "L3_ChargerComm.h"
#include "L3_SMBus.h"
#include "L2_Lptmr.h"
#include "McuX.h"                   // Import McuX interfaces

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
//#define DISABLE_THE_BATTERY
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_CHARGER)           ///< Log Group Identifier

#define TIME_TO_LOG_COUNT               (6u)      // Approx 5 minutes based on fast sleep time and wake time

#define MINUTES_IN_HOUR                 (60u)
#define NUM_SAMPLE_POINTS               (3u)    ///< Number of Sampe Points
#define MAX_MAINTENANCE_COUNT           (5u)    ///< Maximum Maintaince charge counts

// The following parameters are used to set the processor sleep time
#define LPTMR_TICKS_PER_SECOND  (500u)     ///< LPTMR ticks per second
#define LPTMR_PRESCALE_SETTING  (0)       ///< Default LPTMR prescaler settings
#define SLEEP_TIME_20MIN        (18750u)   ///< Prescaler * secondsIn20Min = 15.625 * 20 * 60 = 18750
#define SLEEP_TIME_15MIN        (13784u)   ///< Prescaler * secondsIn15Min = 15.625 * 15 * 60 = 14062
                                          // Parameter adjusted to 13784 for the timing test
#define SLEEP_TIME_5MIN         (37500u)   ///< Prescaler * secondsIn15Min = 125 * 5 * 60

//#define LPTMR_CLOCK1            (1u)
//#define LPTMR_PRESCALER_DIV64   (5u)
//#define LPTMR_PRESCALER_DIV8    (2u)

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                    ///< Sleep time enumeration
{
    SLEEP_ZONE_CF,          ///< 20Minutes sleep time
    SLEEP_ZONE_BC,          ///< 15Minutes sleep time
    SLEEP_ZONE_FG,           ///<  5Minutes sleep time
    SLEEP_ZONE_AB_GJ,          ///< 15Seconds sleep time
    SLEEP_ZONE_MAX
} NEXTSLEEPTIME;

typedef enum                                    ///< Battery Discharge States used to Dissable battery
{
   DISCHARGE_STATE_CHECK_MFG_FETS,              ///< State to check and dissable Charging FETS
   DISCHARGE_STATE_CHECK_DISCHARGE_FET,         ///< State to check and dissable Discharging FETS
   DISCHARGE_STATE_SLEEP_FOREVER,               ///< State to Dissable battery and sleep
} BATTERYDISCHARGE_STATES;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
static const uint32_t HandleSleepTime[SLEEP_ZONE_MAX] =
{
    1200000u,
    900000u,
    300000u,
    15000u
};
/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static BATTERYHEALTHPARAM BatteryHealthParam;
static bool BatteryHealthParmsWereLogged;
static NEXTSLEEPTIME CurrentSleepTime;
/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static NEXTSLEEPTIME DetermineNextSleepTime(CHARGER_INFO *pInfo);
static void SetLPtmrWithSleepTime(NEXTSLEEPTIME sleepTime);
static void LogBatteryParmsOnCharger(CHARGER_INFO *pChgrInfo, BATTERYHEALTHPARAM *bhp);
static void DisableTheBattery(void);
static void LogBatteryHealthMeasureParms(uint8_t *pTimeStr, BATTERYHEALTHPARAM *bhp, CHARGER_INFO *pInfo);
static void LogBatteryHealthResults(BATTERYHEALTHPARAM *bhp, CHARGER_INFO *pInfo);
static bool CheckBatteryTemperatureError(CHARGER_INFO* pInfo, BATTERYHEALTHPARAM *bhp);
static void CalcImpliedCurrent(CHARGER_INFO *pInfo, BATTERYHEALTHPARAM *bhp);
static BATTERY_HEALTHCHECK_STATES CompleteBatteryHealthCycle(BATTERYHEALTHPARAM *bhp, CHARGER_INFO* pInfo);
static bool CheckHasMaintenancePeriod(CHARGER_INFO *pInfo, BATTERYHEALTHPARAM *bhp);
static void SetNextSleepTime(NEXTSLEEPTIME SleepTime);
/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   This functions determines the next sleep time based on the battery voltage
 *
 * \details If TCA bit is not set       : Sleep time = 15sec
 *             Battery Voltage > 8280mV : Sleep time = 20Min
 *    8260mv < Battery Voltage < 8280mV : Sleep time = 5min
 *
 * \param   pInfo - pointer to the battery parameters
 *
 * \return  NEXTSLEEPTIME - time to sleep
 *
 * ========================================================================== */
static NEXTSLEEPTIME DetermineNextSleepTime(CHARGER_INFO *pInfo)
{
    NEXTSLEEPTIME Retval;
    // if charge current is positive, charging is active. Sleep the shortest time
    if (GET_BIT(pInfo->BatteryGaugSts,BATTERY_TCABIT) == 0)
    {
        Retval = SLEEP_ZONE_AB_GJ;
    }
    else if (pInfo->BatteryVoltage > HIGH_VOLTAGE_THRESH_mv)
    {
    Retval = SLEEP_ZONE_CF;
    }
    else if (pInfo->BatteryVoltage  > MED_VOLTAGE_THRESH_mv)
    {
    Retval = SLEEP_ZONE_FG;
    }
    else
    {
    Retval = SLEEP_ZONE_AB_GJ;
    }
    return Retval;
}

/* ========================================================================== */
/**
 * \brief   This function sets when to awake next in the lower level hardware
 *
 * \details Based on the sleep time the LPTMR regeristers are configured.
 *
 * \param   sleepTime - next sleep time
 *
 * \return  None
 *
 * Notes - This function should be moved to the low level hardware interface module
 *         See section 42.3.2 of the K20 reference manual
 * ========================================================================== */
static void SetNextSleepTime(NEXTSLEEPTIME SleepTime)
{
    do
    {
      CurrentSleepTime = SleepTime;

      if (!L4_CommStatusActive())
      {
         SetLPtmrWithSleepTime(SleepTime);
      }

    } while (false);
}

/* ========================================================================== */
/**!
 * \brief   This function sets when to awake next in the lower level hardware
 *
 * \details Based on the sleep time the LPTMR registers are configured.
 *
 * \param   sleepTime - next sleep time
 *
 * \return  None
 *
 * Notes - This function should be moved to the low level hardware interface module
 *         See section 42.3.2 of the K20 reference manual
 * ========================================================================== */
static void SetLPtmrWithSleepTime(NEXTSLEEPTIME sleepTime)
{
    LptmrControl   LptmrConfig;

    // Select prescaler clock to LPO - 1kHz clock
    LptmrConfig.ClkSource = LPTMR_LPO1KHZ;
    LptmrConfig.Mode = LPTMR_MODE_TIME;
    LptmrConfig.pHandler = NULL;

    switch (sleepTime)
    {
        case SLEEP_ZONE_CF:
            LptmrConfig.Prescalar = LPTMR_PRESCALAR_DIV64;
            LptmrConfig.Value = SLEEP_TIME_20MIN;
            Log(DBG, "Next sleep time 20min");
            break;

        case SLEEP_ZONE_BC:
            LptmrConfig.Prescalar = LPTMR_PRESCALAR_DIV64;
            LptmrConfig.Value = SLEEP_TIME_15MIN;
            Log(DBG, "Next sleep time 15min");
            break;

        case SLEEP_ZONE_FG:
            LptmrConfig.Prescalar = LPTMR_PRESCALAR_DIV8;
            LptmrConfig.Value = SLEEP_TIME_5MIN;
            Log(DBG, "Next sleep time 5min");
            break;

        case SLEEP_ZONE_AB_GJ:
        default:
            LptmrConfig.Prescalar = LPTMR_PRESCALAR_DIV2;
            LptmrConfig.Value = (LPTMR_TICKS_PER_SECOND * 15);
            Log(DBG, "Next sleep time 15Sec");
            break;
    }
    L2_LptmrConfig(&LptmrConfig);
}

/* ========================================================================== */
/**
 * \brief   This function logs the battery parameters when on the charger
 *
 * \details Paramerters logged in this fucntion are
 *            Battery Voltage,          Battery Cell0 Voltage,          Battery Cell1 Voltage,
 *            Charger current,          BQ RSOC,                        Calculated RSOC,
 *            Battery Safety Status,    Battery Opertion Status,        Battery Charging Status,
 *            Battery Gauging Status,   Battery PermenantFail Status,   HealthCheck State,
 *            Internal Temperature,     TS1 Temperature,                TS2 Temperature,
 *            Delta Temperature,        CA Status
 *
 * \param   pChgrInfo - Pointer to Charger information
 * \param   bhp - Pointer to battery health parameters
 *
 * \return  None
 *
 * ========================================================================== */
static void LogBatteryParmsOnCharger(CHARGER_INFO *pChgrInfo, BATTERYHEALTHPARAM *bhp)
{
    do
    {
        BREAK_IF (BatteryHealthParmsWereLogged);

        bhp->TimeToLogCount = 0;
        BatteryHealthParmsWereLogged = true;  // Do not log again during this sleep period
        //LogTimeDate();
        Log(DBG, "BQ1: mV=%d, C1mV=%d, C2mV=%d, mA=%d, CalcRSOC=%.0f%, bqRSOC=%d%%",
            pChgrInfo->BatteryVoltage,
            pChgrInfo->BatteryCell0Voltage,
            pChgrInfo->BatteryCell1Voltage,
            bhp->Chargecurrent,
            pChgrInfo->BatteryLevelBQ,
            pChgrInfo->BatteryLevel);

        Log(DBG, "BQ2: SSt=0x%X, OpSt=0x%X, ChrSt=0x%X, GSt=0x%X, PFSt=0x%X, BHState=%d",
            pChgrInfo->BatterySafetySts,
            pChgrInfo->BatteryOperationSts,
            pChgrInfo->BatteryChargeSts,
            pChgrInfo->BatteryGaugSts,
            pChgrInfo->BatteryPFSts,
            bhp->HealthCheckState);

        Log(DBG, "BQ3: IntTmp=%.1f, TS1Tmp=%.1f, TS2Tmp=%.1f, TmpDelta=%.1f, TCA=%d",
            pChgrInfo->InternalTemperature,
            pChgrInfo->TS1Temperature,
            pChgrInfo->TS2Temperature,
            bhp->TemperatureDelta,
            GET_BIT(pChgrInfo->BatteryGaugSts,BATTERY_TCABIT));
    } while(false);
}

/* ========================================================================== */
/**
 * \brief   This function logs the battery health parameters
 *
 * \details Paramerters logged in this fucntion are
 *            All the parameters logged in LogBatteryParmsOnCharger() and
 *            TimeStamp, Battery Voltage, Calculated RSOC, Implired Current(latest), Charger current
 *
 * \param   pTimeStr - Pointer to timestamp stirng
 * \param   bhp - Pointer to Battery health parameters
 * \param   pInfo - Pointer to Charger information
 *
 * \return  None
 *
 * ========================================================================== */
static void LogBatteryHealthMeasureParms(uint8_t *pTimeStr, BATTERYHEALTHPARAM *bhp, CHARGER_INFO *pInfo)
{
    LogBatteryParmsOnCharger(pInfo,bhp);
    Log(DBG, "BatHM: %s, BatV=%d, CalcRSOC=%.2f, ImpCurr=%d, Curr=%d",
            pTimeStr, pInfo->BatteryVoltage,
            pInfo->BatteryLevel,
            bhp->CurrImpliedCurr,
            bhp->Chargecurrent);
}

/* ========================================================================== */
/**
 * \brief   This function logs the battery health calculation
 *
 * \details Paramerters logged in this fucntion are
 *            All the parameters logged in LogBatteryParmsOnCharger() and
 *            TimeStamp, Battery Voltage, Calculated RSOC, Implired Current(latest), Charger current
 *
 *
 * \param   bhp - Pointer to Battery health parameters
 * \param   pInfo - Pointer to Charger information
 *
 * \return  None
 *
 * ========================================================================== */
static void LogBatteryHealthResults(BATTERYHEALTHPARAM *bhp, CHARGER_INFO *pInfo)
{
    const CHGR_MNGR_BATTERYDESIGNPARAMS *pBatteryParm;
    pBatteryParm = pInfo->pBattParam;
    LogBatteryParmsOnCharger(pInfo,bhp);

    Log(DBG, "BatHR1: Vstart=%d, Vend=%d, RSOCstart=%.2f, RSOCend=%.2f, RSOCdelta=%.2f",
        bhp->StartVoltage,
        bhp->EndVoltage,
        (float32_t)bhp->RSOCStart / 100.0f,
        (float32_t)bhp->RSOCEnd / 100.0f,
        (float32_t)bhp->RSOCDelta / 100.0f);

    Log(DBG, "BatHR2: IntTmp=%.1f, TmpDelta=%.1f, CyclCnt=%d, ChemId=%04X, ChargeCap=%d",
        pInfo->InternalTemperature,
        bhp->TemperatureDelta,
        pInfo->BatChgrCntCycle,
        pBatteryParm->ChemID,
        pBatteryParm->ChargeCapacity);

    Log(DBG, "BatHR3: HasMaintPeriod=%d, Limit=%d, LastImpCur=%d, CurrImpCur=%d, IsHealthy=%d",
        bhp->HasMaintainancePeriod,
        bhp->ImpliedCurrLimit,
        bhp->PrevImpliedCurr,
        bhp->CurrImpliedCurr,
        bhp->BatteryIsHealthy);
}

/* ========================================================================== */
/**
 * \brief   This function checks for a over temperature error
 *
 * \details The temperature Delta value is the different b/w Internal Temperature and the Max( TS1, TS2 Temperatures)
 *          Over temperature error is reported if the TempDelta and cell temp > limt for 2 calculation cycles
 *
 * \param   pInfo - Pointer to Charger information
 * \param   bhp - Pointer to Battery health parameters
 *
 * \return  bool -  True: Temperature is in range
 *
 * ========================================================================== */
static bool CheckBatteryTemperatureError(CHARGER_INFO* pInfo, BATTERYHEALTHPARAM *bhp)
{
    bool RetValue;
    RetValue = true;
    do
    {
        // Only measure the temperature when the battery is discharging
        BREAK_IF (pInfo->BatteryCurrent > 10);


        bhp->CellTemp = (pInfo->TS1Temperature > pInfo->TS2Temperature)? pInfo->TS1Temperature : pInfo->TS2Temperature ;
        bhp->TemperatureDelta = bhp->CellTemp - pInfo->InternalTemperature;

        if ((bhp->TemperatureDelta > MAX_HEALTH_TEMP_DELTA_ALLOWED) &&
            (bhp->CellTemp > HEALTH_ABSOLUTE_CELL_TEMP_LIMIT) &&
            (bhp->PrevTemperatureDelta > MAX_HEALTH_TEMP_DELTA_ALLOWED) &&
            (bhp->PrevCellTemp > HEALTH_ABSOLUTE_CELL_TEMP_LIMIT)
           )
        {
            LogBatteryParmsOnCharger(pInfo, bhp);
            Log(DBG, "BatErr: Temp Error. CellTemp=%.1f, DeltaTemp=%.1f, LastCellTemp=%.1f, LastDeltaTemp=%.1f",
                      bhp->CellTemp,
                      bhp->TemperatureDelta,
                      bhp->PrevCellTemp,
                      bhp->PrevTemperatureDelta);
            RetValue = true;
            DisableTheBattery();     // This function does not return
        }

        else
        {
            // Save the current values for the next sleep cycle
            bhp->PrevTemperatureDelta = bhp->TemperatureDelta;
            bhp->PrevCellTemp = bhp->PrevCellTemp;
            RetValue = false;
        }
    } while (false);
    return RetValue;  // The return value is used only for unit testing
}

/* ========================================================================== */
/**
 * \brief   This function calculates the implied current used in the health algorithm
 *
 * \details ImpliedCurr_ma = (RSOC(StartVoltage) - RSOC(endVoltage)) * designChargeCapacity_maHr * 60 / numMinutes
 *
 * \param   pInfo - Pointer to Charger information
 * \param   bhp - Pointer to Battery health parameters
 *
 * \return  None
 *
 * ========================================================================== */
static void CalcImpliedCurrent(CHARGER_INFO *pInfo, BATTERYHEALTHPARAM *bhp)
{
    int32_t  Temps32;

    const CHGR_MNGR_BATTERYDESIGNPARAMS *pBattInfo;

    pBattInfo = pInfo->pBattParam;

    if ((bhp->StartVoltage - bhp->EndVoltage) <= 0)
    {
        bhp->RSOCDelta = 0;
        bhp->CurrImpliedCurr = 0;
    }
    else
    {
        bhp->RSOCDelta = bhp->RSOCStart - bhp->RSOCEnd;

        // Implied current = (RSOCDelta * designChargeCapacity_maHr * 60) / NumMinutes.
        Temps32 = (uint32_t)bhp->RSOCDelta * pBattInfo->ChargeCapacity  * 60;
        Temps32 /= bhp->TotalSleepTime * 10000; // Divide by sample time, correct scaling
        bhp->CurrImpliedCurr = (uint16_t)Temps32;

        Log(DBG, "ImpCurrCalc: SVolt=%d, SRSOC=%.2f, EVolt=%d, ERSOC=%.2f, CalcImpCurr=%d",
                 bhp->StartVoltage, (float32_t)bhp->RSOCStart/100.0f,
                 bhp->EndVoltage, (float32_t)bhp->RSOCEnd/100.0f,
                 bhp->CurrImpliedCurr);
    }
}

/* ========================================================================== */
/**
 * \brief   This function compares the implied current calculation
 *               from the this health check and the last health check
 *               to determine the battery healthy .
 *
 * \details  Battery Health is set to false if the Implied current is out of range for two cycles
 *
 * \param   bhp - Pointer to Battery health parameters
 * \param   pInfo - Pointer to Charger information
 *
 * \return  None
 *
 * ========================================================================== */
static void CheckBatteryHealthResults(BATTERYHEALTHPARAM *bhp, CHARGER_INFO *pInfo)
{
    if (bhp->HasMaintainancePeriod)
    {
        bhp->ImpliedCurrLimit = MAX_IMPLIED_CURR_LIMIT_WITH_MAINT_PERIOD_ma;
    }
    else
    {
        bhp->ImpliedCurrLimit = MAX_IMPLIED_CURR_LIMIT_NO_MAINT_PERIOD_ma;
    }

    bhp->BatteryIsHealthy = true;
    if (bhp->CurrImpliedCurr > bhp->ImpliedCurrLimit)
    {
        if (bhp->PrevImpliedCurr > bhp->ImpliedCurrLimit)
        {
            LogBatteryParmsOnCharger(pInfo, bhp);
            Log(DBG, "BatErr: LastImpliedCur=%d, ThisImpCurr=%d, Limit=%d",
                   bhp->PrevImpliedCurr,
                   bhp->CurrImpliedCurr,
                   bhp->ImpliedCurrLimit);
            Log(DBG, "BatErr: Implied Current. Disabling Battery.");
            bhp->BatteryIsHealthy = false;
            DisableTheBattery(); // This function does not return
        }
    }
}

/* ========================================================================== */
/**
 * \brief   This function is called when a battery health cycle is complete
 *
 * \details Determines the next state based on the charger version determined by Maintaince period
 *
 * \param   bhp - Pointer to Battery health parameters
 * \param   pInfo - Pointer to Charger information
 *
 * \return  BATTERY_HEALTHCHECK_STATES
 *
 * ========================================================================== */
static BATTERY_HEALTHCHECK_STATES CompleteBatteryHealthCycle(BATTERYHEALTHPARAM *bhp,
                                       CHARGER_INFO* pInfo)
{
    BATTERY_HEALTHCHECK_STATES NextBattHealthState;

    LogBatteryParmsOnCharger(pInfo, bhp);
    Log(DBG, "BatStat: Health Check Complete");
    CheckBatteryHealthResults(bhp, pInfo);
    LogBatteryHealthResults(bhp, pInfo);

    // Save the last implied current to be used the next time
    bhp->PrevImpliedCurr = bhp->CurrImpliedCurr;

    // Do the following for a V12 charger (No Maintenance Peroid)
    if (!bhp->HasMaintainancePeriod)
    {
        // Do the following if TCA is still on
        if (GET_BIT(pInfo->BatteryGaugSts,BATTERY_TCABIT))
        {
            NextBattHealthState = BHC_STATE_NO_MAINT_PERIOD_WAIT_UNTIL_TCA_IS_OFF;
        }
        else // Else, Go to state where TCA is off
        {
            NextBattHealthState = BHC_STATE_TCA_IS_OFF;
        }
    }
    else  // Has Maintenance Charge, 1 Bay V15/16, 4 Bay
    {
        // Do the following If the battery is discharging and TCA is Off
        if ((bhp->Chargecurrent < BATTERY_DISCHARGE_CURRENT_THRESHOLD_ma) &&
             (!GET_BIT(pInfo->BatteryGaugSts,BATTERY_TCABIT) ) )
        {
            NextBattHealthState = BHC_STATE_TCA_IS_OFF;
        }
        else // else wait for Maintenance Period to finish
        {
            NextBattHealthState = BHC_STATE_HAS_MAINT_PERIOD_WAIT_TO_FINISH;
        }
    }
    return NextBattHealthState;
}

/* ========================================================================== */
/**
 * \brief   This function checks if the charger has the 3 1/2 hr maintenance period.
 *               The V16 charger has a maintenance period
 *               The V12 charger does not have a maintenance period
 *
 * \details Switches off the Charging FETs, Discharging FETS, and Dissbales the Battery
 *
 * \param   pInfo - Pointer to Charger information
 * \param   bhp - Pointer to Battery health parameters
 *
 * \return  bool: True - if Charger has Maintenance Period, False - otherwise
 * Notes - The up/down counter is used to add some hysterisis to this check.
 * ========================================================================== */
static bool CheckHasMaintenancePeriod(CHARGER_INFO *pInfo, BATTERYHEALTHPARAM *bhp)
{
    bool HasMaintancePeriod;

    // Only check for Maintenance Period if TCA is on.
    // The battery is not charging if TCA is on.
    if (GET_BIT(pInfo->BatteryGaugSts,BATTERY_TCABIT))
    {
        // Charge current is above the battery discharge threshold
        // Charger is powering the handle. Handle is in maintenance period.
        if (bhp->Chargecurrent > BATTERY_DISCHARGE_CURRENT_THRESHOLD_ma)
        {
            if (bhp->MaintenanceCount < MAX_MAINTENANCE_COUNT)
            {
                bhp->MaintenanceCount ++;
            }
        }
        else
        {
            // Charger is powering the handle. No Maintenance period - V12 charger.
            if (bhp->MaintenanceCount > -MAX_MAINTENANCE_COUNT)
            {
                bhp->MaintenanceCount --;
            }
        }
    }

    if (0 == bhp->MaintenanceCount)
    {
        if (bhp->Chargecurrent > BATTERY_DISCHARGE_CURRENT_THRESHOLD_ma)
        {
            HasMaintancePeriod = true;
        }
        else
        {
            HasMaintancePeriod = false;
        }
    }
    else if (bhp->MaintenanceCount > 0)
    {
        HasMaintancePeriod =  true;
    }
    else
    {
        HasMaintancePeriod =  false;
    }

    return HasMaintancePeriod;
}

/* ========================================================================== */
/**
 * \brief   This function disables the battery. It continues to retry until sucessful
 *
 * \details Switches off the Charging FETs, keeps Discharging FETS ON, This dissbales furhter charging of the Battery
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void DisableTheBattery(void)
{
    uint16_t manufacturingStatus;
    uint32_t operationStatus;
    BATTERY_STATUS opStatus;       /* SMBus communication status */
    uint16_t       opData;         /* Operation data */
    uint8_t Size;
    BATTERYDISCHARGE_STATES dischargeState;

    dischargeState = DISCHARGE_STATE_CHECK_MFG_FETS;
    Log(DBG, "BatErr: Disabling the battery.");

    OSTimeDly(250); // Allow time for the Log to be saved

    // This will disable the handle from powering up again

    for (;;)
    {

        switch (dischargeState)
        {
            case DISCHARGE_STATE_CHECK_DISCHARGE_FET:
                opStatus = L3_BatteryGetStatus(CMD_OPERATION_STATUS, &Size, (uint8_t *)&operationStatus);
                if (BATTERY_STATUS_OK == opStatus )
                {

                    // Check if discharge FET is disabled. Enable if disabled
                    if ((operationStatus & OP_STATUS_DISCHARGE_FET_ENABLED_BIT) == 0)
                    {
                        // This command enables the the Discharge FET
                        opData = MFGACCESS_DSGFET;
                        L3_SMBusWriteWord(BATTERY_SLAVE_ADDRESS,
                                      BAT_MANUFACTURING_ACCESS_BYTE,
                                      opData);
                    }
                    else
                    {
                        Log(DBG, "BatErr: Charge Fets Disabled, Discharge Fets Enabled");
                        Log(DBG, "BHC: Show BHC Battery Comm Error Screen");
                        dischargeState = DISCHARGE_STATE_SLEEP_FOREVER;
                    }
                }
                break;

            case DISCHARGE_STATE_SLEEP_FOREVER:
                ///\todo 02/04/2022 BS - Need Batt com Error screen? the Display is dissbaled at this point should we display the screen?
                //GUI_BHC_DrawBattCommErrorScreen();
                break;

            case DISCHARGE_STATE_CHECK_MFG_FETS:
            default:
                opStatus = L3_BatteryGetStatus(CMD_MANUF_STATUS, &Size, (uint8_t *)&manufacturingStatus);
                if (BATTERY_STATUS_OK == opStatus)
                {
                    // Check if FETs are enabled. FETs are enabled if status bit = 1
                    if ((manufacturingStatus & MFG_STATUS_FET_ENABLED_BIT) != 0)
                    {
                        // This command disables the Charge FET, PreCharge FET
                        opData = MFGACCESS_FETCNTRL;
                        ///\todo 3/6/2022 - BS : Battery dissbaling is not allowed for now to avoid accidental battary locking. this should be included in final code.
                        #ifdef DISABLE_THE_BATTERY
                            L3_SMBusWriteWord(BATTERY_SLAVE_ADDRESS,
                                      BAT_MANUFACTURING_ACCESS_BYTE,
                                      opData);
                        #endif
                        Log(REQ, "Battery Disabled: OLED, Piezo, Heartbeat LED and Safety key LED Disabled");        
                        L3_GpioCtrlClearSignal(GPIO_GN_LED);
                        L3_GpioCtrlClearSignal(GPIO_IM_GOOD);
                    }
                    else
                    {
                        dischargeState = DISCHARGE_STATE_CHECK_DISCHARGE_FET;
                    }
                }
                break;
        }
        OSTimeDly(500);
    }
}


/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   BatteryHealthcheck statemachine
 *
 * \details This function implements the Battery health check statemachine
 *          Battery Health parameters are logged at Min 5min interval
 *
 * \param   pInfo - pointer to charger information
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_BatteryHealthCheck(CHARGER_INFO *pInfo)
{
    NEXTSLEEPTIME NextSleepTime;
    uint8_t tmpStr[10];
    int16_t totalTime_min;
    int16_t Temp16s;
    int16_t TempCurrent;
    uint32_t Temp32;
    uint8_t i;
    uint8_t Size;

    do
    {
        BatteryHealthParmsWereLogged = false;

        NextSleepTime = DetermineNextSleepTime(pInfo);
        BatteryHealthParam.WakeupOnChargerCount++;
        if (BatteryHealthParam.TimeToLogCount >= TIME_TO_LOG_COUNT)
        {
            // Log the battery health parameters at most once every 5 minutes
            // When the sleep time is 15 seconds, this will limit the logging to every 5 minutes
            LogBatteryParmsOnCharger(pInfo, &BatteryHealthParam);
        }
        // Do not run the battery health algorithm if there was an error reading the battery parameters
        if (!pInfo->IsValid)
        {
             Log(DBG, "Invalid Charger Info");
            // If there was an error reading the battery parms, set the sleep time to the
            // shortest time. Do not change state and exit.
            NextSleepTime = SLEEP_ZONE_AB_GJ;
            BatteryHealthParam.TimeToLogCount++;
            break;
        }
        else
        {
            BatteryHealthParam.Chargecurrent = pInfo->BatteryCurrent;
            L3_ChargerCommSetPowerPackMaster();
            for( i = 0; i< 2; i ++)
            {
                OSTimeDly(SEC_3);
                if (BATTERY_STATUS_OK == L3_BatteryGetCurrent(&Temp16s) && BATTERY_STATUS_OK == L3_BatteryGetStatus(CMD_OPERATION_STATUS,&Size,(uint8_t*)&Temp32))
                {
                    TempCurrent = Temp16s;
                    if(TempCurrent < BatteryHealthParam.Chargecurrent)
                    {
                        BatteryHealthParam.Chargecurrent = TempCurrent;
                        BatteryHealthParam.BatteryOperationSts = Temp32;
                    }
                }
                else
                {
                    Log(DBG, "BatteryCurrent Read Error");
                    break;
                }
            }
            L3_ChargerCommRelPowerPackMaster();
            CheckBatteryTemperatureError(pInfo, &BatteryHealthParam);
            Log(DBG, "WakeupCount: %d, State: %d ",BatteryHealthParam.WakeupOnChargerCount, BatteryHealthParam.HealthCheckState);
            switch (BatteryHealthParam.HealthCheckState)
            {

                // In this state, TCA = 0, the Charge FETs are active, so current applied to the
                // handle will charge the battery.
                case BHC_STATE_TCA_IS_OFF:
                    if (GET_BIT(pInfo->BatteryGaugSts,BATTERY_TCABIT))
                    {
                        // Transition to the 15 minute sleep at the start of maintenance period
                        BatteryHealthParam.HealthCheckState = BHC_STATE_DISCHARGE_SETTLE_TIME_15MIN;
                        LogBatteryParmsOnCharger(pInfo, &BatteryHealthParam);  // Log this transition
                        LogBatteryHealthMeasureParms("0:00", &BatteryHealthParam, pInfo);
                        NextSleepTime = SLEEP_ZONE_BC;
                    }
                    else
                    {
                        // Set the sleep time to wake up in the shortest interval when charging the battery
                        NextSleepTime = SLEEP_ZONE_AB_GJ;
                    }
                    break;

                // Allow the battery voltage to settle after charging
                // Read Battery Parameters after sleeping 0:15 min
                case BHC_STATE_DISCHARGE_SETTLE_TIME_15MIN:
                    LogBatteryParmsOnCharger(pInfo, &BatteryHealthParam);

                    BatteryHealthParam.HasMaintainancePeriod = CheckHasMaintenancePeriod(pInfo, &BatteryHealthParam);
                    BatteryHealthParam.StartVoltage = pInfo->BatteryVoltage;
                    BatteryHealthParam.RSOCStart = (uint16_t)((float32_t)BatteryCalculateRSOC(BatteryHealthParam.StartVoltage)*100.0f);
                    BatteryHealthParam.CurrImpliedCurr = 0;

                    LogBatteryHealthMeasureParms("0:15", &BatteryHealthParam, 0);

                    if (SLEEP_ZONE_CF == NextSleepTime)
                    {
                        BatteryHealthParam.HealthCheckState = BHC_STATE_DISCHARGE_CHECK_IMPLIED_CURR;
                        BatteryHealthParam.TotalSleepTime = 0;
                    }
                    // Battery voltage has dropped too fast. Exit health algo.
                    else if (!BatteryHealthParam.HasMaintainancePeriod)
                    {
                        BatteryHealthParam.HealthCheckState = BHC_STATE_NO_MAINT_PERIOD_WAIT_UNTIL_TCA_IS_OFF;
                    }
                    else
                    {
                        BatteryHealthParam.HealthCheckState = BHC_STATE_HAS_MAINT_PERIOD_WAIT_TO_FINISH;
                    }
                    break;

                // After the battery settling time, check the implied current every 20 minutes for the next 3 hours
                // For a V15/V16 1 Bay and 4 Bay charger, This state is the Maintenence Charge period.
                // For all changers, calculate implied current only if the battery voltage is > HIGH_VOLTAGE_THRESH_mv
                case BHC_STATE_DISCHARGE_CHECK_IMPLIED_CURR:
                    LogBatteryParmsOnCharger(pInfo, &BatteryHealthParam);
                    //CheckBatteryTemperatureError(bp, noInitBatHealth);

                    // For a V15/V16 1 Bay and 4 Bay charger, if the battery is discharging (Charger ended Maintenence period)
                    // Then stop calculating implied current
                    if ( (BatteryHealthParam.HasMaintainancePeriod) &&
                        (BatteryHealthParam.Chargecurrent < BATTERY_DISCHARGE_CURRENT_THRESHOLD_ma) )
                    {
                        BatteryHealthParam.HealthCheckState = CompleteBatteryHealthCycle(&BatteryHealthParam, pInfo);
                    }

                    // For all chargers, if the next sleep period is not 20 minutes (Battery voltage < HIGH_VOLTAGE_THRESH_mv)
                    // Then stop calculating implied current
                    else if (SLEEP_ZONE_CF != NextSleepTime)
                    {
                        BatteryHealthParam.HealthCheckState = CompleteBatteryHealthCycle(&BatteryHealthParam, pInfo);
                    }

                    // If the maximum Battery Health Time been exceeded, then stop calculating implied current
                    else if (BatteryHealthParam.TotalSleepTime > BATTERY_HEALTH_MAX_TIME_min)
                    {
                        BatteryHealthParam.HealthCheckState = CompleteBatteryHealthCycle(&BatteryHealthParam, pInfo);
                    }
                    else
                    {   // Calculate implied current. Implied current is calculated each time through
                        // this branch, but only the last calculation will be used.
                        BatteryHealthParam.HasMaintainancePeriod = CheckHasMaintenancePeriod(pInfo, &BatteryHealthParam);

                        BatteryHealthParam.TotalSleepTime += BATTERY_HEALTH_SLEEP_INTERVAL_min;
                        BatteryHealthParam.EndVoltage = pInfo->BatteryVoltage;
                        BatteryHealthParam.RSOCEnd = (uint16_t)((float32_t)BatteryCalculateRSOC(BatteryHealthParam.EndVoltage)*100.0f);
                        CalcImpliedCurrent(pInfo, &BatteryHealthParam);

                        totalTime_min = BatteryHealthParam.TotalSleepTime + BATTERY_HEALTH_RELAX_INTERVAL_min;
                        sprintf((char *)tmpStr, "%d:%02d",  totalTime_min / MINUTES_IN_HOUR, totalTime_min % MINUTES_IN_HOUR);
                        LogBatteryHealthMeasureParms(tmpStr, &BatteryHealthParam, pInfo);
                    }
                    break;

                // V15/V16 1 Bay, 4 Bay Charger - Wait for TCA to turn Off
                case BHC_STATE_HAS_MAINT_PERIOD_WAIT_TO_FINISH:


                    // The maintenance period is complete when the battery is charging the handle
                    if ((BatteryHealthParam.Chargecurrent < BATTERY_DISCHARGE_CURRENT_THRESHOLD_ma) &&
                        (!GET_BIT(pInfo->BatteryGaugSts, BATTERY_TCABIT)) )
                    {
                        LogBatteryParmsOnCharger(pInfo, &BatteryHealthParam);  // Log this transition
                        BatteryHealthParam.HealthCheckState = BHC_STATE_TCA_IS_OFF;
                    }
                    break;

                // V12 1 Bay - No Maintenance Change. Wait for TCA to turn off
                case BHC_STATE_NO_MAINT_PERIOD_WAIT_UNTIL_TCA_IS_OFF:


                    if (!GET_BIT(pInfo->BatteryGaugSts, BATTERY_TCABIT))
                    {
                        LogBatteryParmsOnCharger(pInfo, &BatteryHealthParam);  // Log this transition
                        BatteryHealthParam.HealthCheckState = BHC_STATE_TCA_IS_OFF;
                    }
                    break;

                // Reset if the state is not one of the defined states.
                default:
                    Log(DBG, "BatStat: Reset From Default Case. State=%d", BatteryHealthParam.HealthCheckState);
                    Signia_BatteryHealthCheckReset();
                    NextSleepTime = SLEEP_ZONE_AB_GJ;
                    break;
            }
        }

        // Determine next time to log the parameters. Would like to log at least 5 minutes between logs
        switch (NextSleepTime)
        {
            // If the sleep time is not 15 seconds, set the log time to log the next awake time
            case SLEEP_ZONE_CF:
            case SLEEP_ZONE_BC:
            case SLEEP_ZONE_FG:
                // With longer sleep times, this will cause logging for every wake cycle
                LogBatteryParmsOnCharger(pInfo, &BatteryHealthParam);
                break;
            default:
                BatteryHealthParam.TimeToLogCount++;
                break;
        }

    } while (false);
    SetNextSleepTime(NextSleepTime);
}

/* ========================================================================== */
/**
 * \brief   The function returns the next configured handle sleep time in ms
 *
 * \details  Depending on the the Battery charger state, Sleep times are configred
 *           The function returns the next configured handle sleep time
 *
 * \param   < None >
 *
 * \return  uint32_t - Next Sleep time
 *
 * ========================================================================== */
uint32_t Signia_BatteryHealthGetNextSleepTime(void)
{
    uint32_t Retval;
    Retval = HandleSleepTime[SLEEP_ZONE_AB_GJ];
    do
    {
        BREAK_IF( CurrentSleepTime > SLEEP_ZONE_MAX);
        Retval = HandleSleepTime[CurrentSleepTime];
    } while (false);

    return Retval;
}

/* ========================================================================== */
/**
 * \brief   Reset Battery Health parameters
 *
 * \details
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_BatteryHealthCheckReset(void)
{
    BatteryHealthParam.HealthCheckState = BHC_STATE_TCA_IS_OFF;
    BatteryHealthParam.CurrImpliedCurr = 0;
    BatteryHealthParam.PrevImpliedCurr = 0;
    BatteryHealthParam.TemperatureDelta = 0;
    BatteryHealthParam.MaintenanceCount = 0;
    BatteryHealthParam.HasMaintainancePeriod = false;
    BatteryHealthParam.TimeToLogCount = TIME_TO_LOG_COUNT;

    BatteryHealthParam.BatteryOperationSts = 0;
    BatteryHealthParam.CellTemp = 0;
    BatteryHealthParam.Chargecurrent = 0;
    BatteryHealthParam.WakeupOnChargerCount = 0;
    BatteryHealthParam.StartVoltage = 0;
    BatteryHealthParam.EndVoltage = 0;
    BatteryHealthParam.RSOCDelta = 0;
    BatteryHealthParam.RSOCEnd = 0;
    BatteryHealthParam.RSOCStart = 0;

    BatteryHealthParmsWereLogged = false;

    CurrentSleepTime = SLEEP_ZONE_AB_GJ;
}

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
