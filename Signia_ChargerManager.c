#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Charger
 * \{
 *
 * \brief   Charger Manager Definition functions
 *
 * \details The Charger Manager Definition defines all the interfaces used for
            communication between Handle, Charger and battery.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_ChargerManager.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Signia_ChargerManager.h"
#include "Signia_AdapterManager.h"
#include "Signia_Motor.h"
#include "L3_ChargerComm.h"
#include "L3_Battery.h"
#include "L3_GpioCtrl.h"
#include "FaultHandler.h"
#include "McuX.h"
#include "TestManager.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_CHARGER)           ///< Log Group Identifier
#define CHARGER_TASK_PERIOD         (5000u)     ///< Charger Task period
#define WAIT_FOR_BATTERY_INIT       (500u)      ///< wait for battery interface to initialize
#define CHGMGR_TASK_STACK           (512u)     ///< Task stack size
#define BATTERY_RSOC_MAX            (100u)      ///< Maximum RSOC
#define BATTERY_DEFAULT_LIMIT_HI    (100u)      ///< Default limit hi
#define BATTERY_DEFAULT_LIMIT_LO    (100u)      ///< Default limit low
#define CHARGER_MAX_NOTIFY_PERIOD   (10000u)    ///< Maximum notification period
#define FULL_CHARGE_TIMEOUT         (120u)      ///< Full charge timeout in minutes
#define FULL_CHARGE_TIMEOUT_TICKS   ((FULL_CHARGE_TIMEOUT * 1000 * 60) / CHARGER_TASK_PERIOD) ///< Full charge timeout
#define CHARGER_PRECHARGE_DURATION  (20000)     ///< Precharge stage duration
#define BATTERY_DF_SIZE_ONEBYTE     (1u)       ///< DataFlash (DF) parameter of size 1byte
#define BATT_WARN_CHARGECYCLECOUNT  (295u)      ///< Battery Warning Charging Cycle Count
#define BATT_TEMP_LOWVALUE          (0.0f)      ///< Battery  Temperature Low Value, Unit: Celsius
#define BATT_TEMP_HIGHVALUE         (75.0f)     ///< Battery Temperature High Value, Unit: Celsius
#define KELVIN_CONVFACTOR           (10u)       ///< 0.1K to 1K conversion factor
#define KELVIN_CONSTANT             (273.15f)   ///< Kelvin Constant
#define KELVIN_TO_CELSIUS(BattTemperature)        ( (BattTemperature)/KELVIN_CONVFACTOR  - KELVIN_CONSTANT)  /*BatteryTemperature Unit: 0.1K, Formulae: Celsius = K - 273.15 */
#define BATTERY_LOGPERIOD           (MIN_5)    ///< Battery Log Period in minutes

#define BATTSHUTDN_VOLTAGE_LIMIT    (6626u)     ///< Battery shutdown voltage limit in mV

#define BATT_IIR_COEFF              (0.03f)     ///< IIR filter coefficient for Battery voltage
#define MAX_VOLTAGE_DELTA           (5u)        ///< max delta allowed between voltage samples, in millivolts

#define MAX_RSOC_VALUE             (100u)       ///< Max RSOC Value (RSOC 100%)
#define NUM_RSOC_VALUES            (101u)       ///< max size of RSOC table

#define BATTPARM_RETRYCOUNT        (3u)        ///< Max Retrys allowed per Battery Parameter read

#define INTTEMP_OFFSET             (0u)        ///< Offset of Internal Temperature
#define TS1TEMP_OFFSET             (2u)        ///< Offset of TS1 Temperature
#define TS2TEMP_OFFSET             (4u)        ///< Offset of TS2 Temperature
#define TEMPERATURE_SIZE           (2u)        ///< Temperature data size in bytes
#define TEMPERATUREBUFF_SIZE       (14u)       ///< Size of the buffer used to read Temperatures (command 0x72)
#define CHRGRCMD_RETRYCOUNT        (5u)        ///< Retry count for the charger commands

#define CELLTEMP_OUTOFRANGE_LOLIMIT           0.0f      ///< CELL TEMPERATURE IN DEGREE CELSIUS OUT OF RANGE LOW LIMIT
#define CELLTEMP_OUTOFRANGE_HILIMIT           (55.0f)   ///< CELL TEMPERATURE IN DEGREE CELSIUS OUT OF RANGE HIGH LIMIT

#define CELLTEMP_VALIDRANGE_LOLIMIT           (2.0f)    ///< CELL TEMPERATURE IN DEGREE CELSUIS VALID RANGE LOW LIMIT
#define CELLTEMP_VALIDRANGE_HILIMIT           (53.0f)   ///< CELL TEMPERTURE IN DEGREE CELSIUS VALID RANGE HIGH LIMIT

#define ERROR_PUBLISHED     (true)          ///< Lock the Error upon publishing
#define ERROR_NOTPUBLISHED  (false)         ///< UnLock the Error upon Error conditions are cleared

#define BQ_CYCLE_COUNT_SET_LIMIT        (51u)     ///< number of charge cycle count BQ chip  holds before getting cleared by handle FW
#define CYCLE_COUNT_RESET_VALUE         (1u)      ///< battery charge cycle count reset value to 1
#define INITIAL_CHARGER_TASK_DELAY      (SEC_3)
#define MAX_CHARGE_CYCLEC_COUNT         (300u)  
#define MSEC_BQ_50                      (MSEC_50)  
#define SM_BUS_DELAY                    (MSEC_10)  
#define BATTERY_OW_CHECK_MAX_RETRY		(5u)
  
/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
OS_STK   ChgMgrTaskStack[CHGMGR_TASK_STACK + MEMORY_FENCE_SIZE_DWORDS];  /// Charger Manager task stack
const char BatteryManufacturer[2][10] = {"PANASONIC","MOLICELL "};           /// Log string (9 characters + NULL)

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
/// Charger Database
typedef struct
{
    CHARGER_INFO Info;              	///< Contain information about charger state, battery info
    uint16_t     	Version;           ///< Charger version
    uint8_t      	RangeHi;           ///< Desired battery limit high. Charger stops charging
    uint8_t      	RangeLo;           ///< Desired battery limit Low. Charger start charging again
    uint8_t      	StopRequest;       ///< Charge stop request
    CHARGER_HANDLER Handler;        	///< Event handler function
    uint32_t    	NotifyPeriod;       ///< Notification period
    bool        	IsPPMaster;         ///< Flag to store the SM bus master status
    bool        	IsRSOCCalcAllowed;  ///< Flag to control the RSOC calculation
    bool        	ChargeCycleUpdated; ///< Flag to check charge cycle update after restart
} CHARGER_DATA;

/// Battery DataFlash information
typedef struct
{
    BATTERY_DF_PARAM DFinfo;        ///< Parameter information in DataFlash
    uint16_t DefaultValue;          ///< Default Value of the parameter in DataFlash
} BATTERY_DF_INFO;

/*! \enum ERRORLIST
 *   Battery Error List
 */
typedef enum
{
    BATT_COMMFAILURE,              ///< Battery Communcation Failure
    BATT_TEMPERATURE,              ///< Battery Temperature Out of Range
    BATT_WARNING,                  ///< Battery Warning
    BATT_EOL,                      ///< Battery EOL
    LAST_ERRORLIST
} ERRORLIST;

/*! \struct ERROR_INFO
 *   Holds Error Cause Information
 */
typedef struct
{
    ERROR_CAUSE Error;    ///< Error Cause
    bool ErrorStatus;     ///< True: Error is Set, False: Error is Cleared
    bool ErrorPublished;  ///< True: Upon publishing the Error, ErrorPublished is set to true to not publish continuosly. False: Once error is cleared ErrorPublished is cleared (false)
} ERROR_INFO;

typedef enum    ///Request Data Flash Read/Write access
{
    DF_READ,    ///< Read  from Data Flash
    DF_WRITE    ///< Write  to Data Flash
} DATAFLASHRW;


///< Battery cycle count update states
typedef enum
{
    STATE_BQ_1W_READ_ANALYZE,       ///< read cycle count from BQ Chip and 1-W
    STATE_BQ_1W_UPDATE,             ///< cycle count update in 1-W and Reset BQ chip
    STATE_BQ_CHECK_EOL,             ///< Check EOL
    STATE_BQ_DEFAULT,               ///< Error, end state machine
    STATE_BQ_END,                   ///< Successful, end state machine
} STATE_CYCLE_COUNT_UPDATE;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
// RSOC table for Panasonic Battery: Panasonic_UR18650RX
static const uint16_t PanasonicBatteryRsocLookupTable[NUM_RSOC_VALUES] =
{
    5840u, 5958u, 6075u, 6193u, 6310u, 6427u, 6545u, 6626u, 6780u, 6892u, //  0-9 % RSOC
    6928u, 6939u, 6949u, 6960u, 6971u, 6981u, 6992u, 7003u, 7014u, 7025u, // 10-19%
    7036u, 7047u, 7058u, 7069u, 7081u, 7092u, 7103u, 7115u, 7127u, 7138u, // 20-29%
    7150u, 7162u, 7174u, 7186u, 7198u, 7210u, 7223u, 7235u, 7248u, 7260u, // 30-39%
    7273u, 7286u, 7298u, 7312u, 7325u, 7338u, 7352u, 7365u, 7379u, 7392u, // 40-49%
    7406u, 7420u, 7434u, 7449u, 7463u, 7478u, 7493u, 7508u, 7523u, 7538u, // 50-59%
    7554u, 7570u, 7585u, 7602u, 7618u, 7635u, 7651u, 7668u, 7686u, 7703u, // 60-69%
    7721u, 7739u, 7758u, 7776u, 7796u, 7815u, 7835u, 7855u, 7876u, 7897u, // 70-79%
    7919u, 7941u, 7964u, 7988u, 8012u, 8037u, 8055u, 8071u, 8095u, 8119u, // 80-89%
    8144u, 8168u, 8192u, 8217u, 8240u, 8265u, 8289u, 8313u, 8337u, 8361u, // 90-99%
    8386u                                                                 // 100%
};

// RSOC table for Moli Cell Battery: Moli_INR18650A
static const uint16_t MoliBatteryRsocLookupTable[NUM_RSOC_VALUES] =
{
    5095u, 5404u, 5767u, 6012u, 6186u, 6324u, 6438u, 6537u, 6622u, 6683u, //  0-9 % RSOC
    6718u, 6744u, 6772u, 6794u, 6813u, 6830u, 6847u, 6864u, 6884u, 6905u, // 10-19%
    6926u, 6948u, 6970u, 6990u, 7009u, 7027u, 7044u, 7061u, 7077u, 7092u, // 20-29%
    7107u, 7122u, 7135u, 7149u, 7162u, 7175u, 7191u, 7206u, 7220u, 7232u, // 30-39%
    7245u, 7258u, 7271u, 7284u, 7298u, 7311u, 7326u, 7340u, 7356u, 7371u, // 40-49%
    7388u, 7405u, 7422u, 7440u, 7458u, 7478u, 7498u, 7520u, 7544u, 7571u, // 50-59%
    7597u, 7621u, 7646u, 7670u, 7693u, 7714u, 7733u, 7750u, 7767u, 7784u, // 60-69%
    7801u, 7818u, 7835u, 7852u, 7870u, 7888u, 7908u, 7928u, 7949u, 7971u, // 70-79%
    7993u, 8017u, 8040u, 8064u, 8087u, 8110u, 8132u, 8154u, 8173u, 8189u, // 80-89%
    8206u, 8222u, 8239u, 8258u, 8279u, 8302u, 8329u, 8357u, 8390u, 8429u, // 90-99%
    8462                                                                  // 100%
};

static const CHGR_MNGR_BATTERYDESIGNPARAMS BatteryDesignParams[BATTERY_TYPE_LAST] =
{
    { 0x2060u, 2050u, PanasonicBatteryRsocLookupTable, 32000u },
    { 0x0528u, 2500u, MoliBatteryRsocLookupTable, 40000u }
};

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/// Charger Database
static CHARGER_DATA ChargerData;
static ERROR_INFO ErrorInfo[LAST_ERRORLIST];
///\todo 06/27/2022 - CPK: update to static const. underlying L3 functions needs to be updated and tested
static BATTERY_DF_INFO BatteryDF_Default[DF_COUNT] =
{
    //{249u, 0u, 2u, 10u},
    { 249u, 0u, 2u, 50u },
    //{249u, 2u, 2u, 70u},
    { 249u, 2u, 2u, 50u },
    { 249u, 4u, 2u, 35u },
    { 489u, 18u, 1u, 100u },
    { 197u, 0u, 2u, 0xEEFBu },
    { 230u, 2u, 1u, 1u },
    { 148u, 0u, 2u, 320u },
    { 168u, 4u, 1u, 10u },
    { 103u, 0u, 1u, 2u },
    { 228u, 0u, 2u, 4500u },
    { 201u, 9u, 2u, 0x0C2Fu },
    { 201u, 3u, 1u, 0x21u },
    { 578u, 2u, 2u, 4125u }
};

static BATTERY_DF_PARAM BatteryDFCycleCount = { 489u, 16u, 2u }; // Cycle Count - Class: 489, Offset: 16, size: 1, Default: 0

static bool PublishWakefromsleep;

static CHRG_MNGR_STATE ChargerManagerState = CHRG_MNGR_STATE_DISCONNECTED;
static AM_BATTERY_IF *pBatteryInterface;
static AM_HANDLE_IF *pHandleIF = NULL;
/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void LogBatteryErrors(void);
static void ErrorInfoInit(void);
static CHRG_MNGR_STATUS ChargerMgrChkBatteryDF(void);

static void BatteryLogInfo(CHARGER_INFO *pInfo);
static CHRG_MNGR_STATUS ReadBatteryParameters(CHARGER_INFO *pInfo, bool OnCharger);
static CHGR_MNGR_BATTERYTYPES BatteryGetTypefromChemID(uint16_t ChemicalID);
static uint16_t Interpolate(uint16_t Input, const uint16_t *pTbl);
static void SetErrorInfo(ERRORLIST Error, ERROR_CAUSE Cause);
static void ClearErrorInfo(ERRORLIST Error, ERROR_CAUSE Cause);
static void CheckBatteryTemperatureValidRange(void);
static BATTERY_STATUS ReadBatteryCurrent(void);
static BATTERY_STATUS ReadBatteryRSOC(void);
static BATTERY_STATUS ReadBatteryVoltage(void);
static BATTERY_STATUS ReadBatteryTemperature(void);
static BATTERY_STATUS ReadBatteryCellVoltage(BATTERY_CELL_NUMBER BatteryCell);
static BATTERY_STATUS ReadBatteryChargingStatus(void);
static BATTERY_STATUS ReadBatteryGaugingStatus(void);
static BATTERY_STATUS ReadBatterySafetyStatus(void);
static BATTERY_STATUS ReadBatteryOperationStatus(void);
static BATTERY_STATUS ReadBatteryPermanentFailFStatus(void);
static BATTERY_STATUS ReadUpdateBatteryChgrCycleCnt(void);
static BATTERY_STATUS ReadBatteryTemperatures(void);
static BATTERY_STATUS ReadWriteBatteryDF(uint8_t Index,  uint8_t *Data, DATAFLASHRW DataAccess);
static void ChargerMgrStateMachine(bool OnCharger);
static void SetPowerPackAsMaster(bool OnCharger);
static void SetBatteryChargingCycleFaults(void);
static void NotifyChargerMgrUserEvents(void);
static void ProcessChargingState(void);
static void ProcessChargedState(bool OnCharger);
static void ProcessConnectedState(void);
static BATTERY_STATUS UpdateBQChipChgrCntCycle(uint16_t CycleCountValue);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Log Battery Parameter Information
 *
 * \details Battery information is logged every time the function is called
 *              - Battery Voltage,
 *              - Battery Cell0 Voltage
 *              - Battery Cell1 Voltage
 *              - Battery Current
 *              - Battery Temperature
 *              - Battery Calculated RSOC level
 *              - Battery RSOC level from BQ
 *              - Battery Safety Status
 *              - Battery Operating Status
 *              - Battery Charging Status
 *              - Battery Permanent Fault Status
 *              - Battery Gauging Status
 *              - Battery Terminate Charging Alert (TCA) Status
 *
 * \param   pInfo - Pointer to Battery information in charger structure
 *
 * \return  None
 *
 * ========================================================================== */
static void BatteryLogInfo(CHARGER_INFO *pInfo)
{
    Log(REQ, "Bat1: mV=%d,C1mV=%d,C2mV=%d,mA=%d,CTemp=%.1f,CalcRSOC=%.0f%,BqRSOC=%d%%",
        pInfo->BatteryVoltage,                // 0x09
        pInfo->BatteryCell0Voltage,           // 0x3F
        pInfo->BatteryCell1Voltage,           // 0x3E
        pInfo->BatteryCurrent,
        pInfo->BatteryTemperature,
        pInfo->BatteryLevel,                  // Calculated RSOC
        pInfo->BatteryLevelBQ);               // RSOC from BQ

    Log(REQ, "Bat2: SSt=0x%X,OpSt=0x%X,ChrSt=0x%X,PFSt=0x%X,GSt=0x%X,TCA=%d",
        pInfo->BatterySafetySts,
        pInfo->BatteryOperationSts,
        pInfo->BatteryChargeSts,
        pInfo->BatteryPFSts,
        pInfo->BatteryGaugSts,
        GET_BIT(pInfo->BatteryGaugSts, BATTERY_TCABIT));
}

/* ========================================================================== */
/**
 * \brief   Interpolation function
 *
 * \details implements linear interpolation function
 *              y = y1 + (y2-y1)((input - x1)/(x2-x1))
              in case of RSOC look up tables y2-y1 is always 1,
 *
 * \param   Input - Filtered voltage
 * \param   pTbl - pointer to the LUT
 *
 * \return  uint16_t - Calculated RSOC level
 *
 * ========================================================================== */
/// \todo 07/27/2022 DAZ - We have a generalized interpolation function already implemented.
static uint16_t Interpolate(uint16_t Input, const uint16_t *pTbl)
{
    uint16_t index;
    float Fraction;
    uint16_t output;
    output = 0;

    do
    {
        // If the voltage is greater than the max voltage, then RSOC = 100
        if (Input > pTbl[NUM_RSOC_VALUES - 1])
        {
            output =  MAX_RSOC_VALUE; // 100 @ x100scaling
            break;
        }

        // If the voltage is less than the minimum voltage, then RSOC = 0
        if (Input < pTbl[0])
        {
            output = 0;
            break;
        }

        // Find the voltage in the table
        for (index = NUM_RSOC_VALUES - 2; index > 0; index--)
        {
            BREAK_IF(Input > pTbl[index]);
        }

        // Interpolate between the last two values
        Fraction = ((Input - pTbl[index])) / (pTbl[index + 1] - pTbl[index]);

        output = (uint16_t)(index + Fraction);
    } while (false);

    return (output);
}

/* ========================================================================== */
/**
 * \brief   Read Battery Parameters from BQ chip and calculates the Battery RSOC
 *
 * \details The following Battery parameters are read from the BQ chip
 *           Battery Current in mA
 *           Battery RSOC Level from BQ in %
 *           Battery Voltage in mV
 *           Battery RSOC Level calucated from Battery Voltage
 *           Battery Temperature in DegC
 *           Battery Cell0 Voltage in mV
 *           Battery Cell1 Voltage in mV
 *           Battery Status - ( Charging, Gauging, Safety, Operational, Permanent Fault (PF), TCA )
 *           Battery Charge Cycle Count
 *    if on charger
 *           Battery Internal temperature
 *           Battery TS1 temperature
 *           Battery TS2 temperature
 *
 * \param   pInfo - Pointer to the Battery information
 *          OnCharger - handle on/off charger state
 *
 * \return  CHRG_MNGR_STATUS - CHRG_MNGR_STATUS_OK if all the Battery parameters are read
 *                             CHRG_MNGR_STATUS_ERROR if any of the Battery parameter read failed
 *
 * \note BATTERY_STATUS_OK should be 0 in the enum
 * ========================================================================== */
static CHRG_MNGR_STATUS ReadBatteryParameters(CHARGER_INFO *pInfo, bool OnCharger)
{
    BATTERY_STATUS BattStatus;  ///< Indicates if the L3 battery functions return status. It dosen't identify the exact error.
    CHRG_MNGR_STATUS Status;

    BattStatus = BATTERY_STATUS_OK;

    BattStatus |= ReadBatteryCurrent();
    BattStatus |= ReadBatteryRSOC();
    BattStatus |= ReadBatteryVoltage();
    BattStatus |= ReadBatteryTemperature();
    BattStatus |= ReadBatteryCellVoltage(CELL_0);
    BattStatus |= ReadBatteryCellVoltage(CELL_1);
    BattStatus |= ReadBatteryChargingStatus();
    BattStatus |= ReadBatteryGaugingStatus();
    BattStatus |= ReadBatterySafetyStatus();
    BattStatus |= ReadBatteryOperationStatus();
    BattStatus |= ReadBatteryPermanentFailFStatus();
    BattStatus |= ReadBatteryTemperatures();

    Status = (BATTERY_STATUS_OK == BattStatus) ? CHRG_MNGR_STATUS_OK : CHRG_MNGR_STATUS_ERROR; // In case of any Battery parameter read failure return Error else return OK
    ChargerData.Info.IsValid = (CHRG_MNGR_STATUS_OK == Status) ? true : false;

    TM_Hook(HOOK_BATTERYPARAMETER, (void *)(&ChargerData.Info));
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Read Battery Current from BQ chip
 *
 * \details The following Battery paramerters are read from the BQ chip
 *           Battery Current in mA
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Read is succesful
 *                           BATTERY_STATUS_ERROR if  read failed
 *
 * ========================================================================== */
static BATTERY_STATUS ReadBatteryCurrent(void)
{
    BATTERY_STATUS BattStatus;  ///< Indicates if the L3 battery functions return status. It doesn't identify the exact error.
    uint8_t        RetryCount;
    int16_t        Temp16s;

    RetryCount = 0x0;
    do
    {
        OSTimeDly(SM_BUS_DELAY);//SM bus settling time
        BattStatus = L3_BatteryGetCurrent(&Temp16s); // Read Battery Current
        if (BATTERY_STATUS_OK == BattStatus)
        {
            ChargerData.Info.BatteryCurrent = Temp16s;
            RetryCount = 0;
            break;
        }
        else  // In case of read fail, retry reading the Battery parameter
        {
            RetryCount++;
            BattStatus = (RetryCount < BATTPARM_RETRYCOUNT) ? BATTERY_STATUS_OK : BattStatus;
        }
    } while (RetryCount < BATTPARM_RETRYCOUNT);

    if (BattStatus != BATTERY_STATUS_OK)
    {
        Log(DBG, "BatteryCurrent Read Error");
    }

    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Read Battery RSOC from BQ chip
 *
 * \details The following Battery paramerters are read from the BQ chip
 *           Battery RSOC Level from BQ in %
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Read is succesful
 *                           BATTERY_STATUS_ERROR if  read failed
 *
 * ========================================================================== */
static BATTERY_STATUS ReadBatteryRSOC(void)
{
    BATTERY_STATUS BattStatus;  ///< Indicates if the L3 battery functions return status. It dosen't identify the exact error.
    uint8_t        RetryCount;
    uint16_t        Temp16;

    RetryCount = 0x0;
    do
    {
        OSTimeDly(SM_BUS_DELAY);//SM bus settling time
        BattStatus |= L3_BatteryGetRSOC(&Temp16); // Read Battery RSOC from BQ
        if (BATTERY_STATUS_OK == BattStatus)
        {
            ChargerData.Info.BatteryLevelBQ = Temp16;
            RetryCount = 0;
            break;
        }
        else  // In case of read fail, retry reading the Battery parameter
        {
            RetryCount++;
            BattStatus = (RetryCount < BATTPARM_RETRYCOUNT) ? BATTERY_STATUS_OK : BattStatus;
        }
    } while (RetryCount < BATTPARM_RETRYCOUNT);

    if (BattStatus != BATTERY_STATUS_OK)
    {
        Log(DBG, "BatteryLevelBQ Read Error");
    }

    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Read Battery Voltage from BQ chip
 *
 * \details The following Battery paramerters are read from the BQ chip
 *           Battery Voltage in mV
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Read is succesful
 *                           BATTERY_STATUS_ERROR if  read failed
 *
 * ========================================================================== */
static BATTERY_STATUS ReadBatteryVoltage(void)
{
    BATTERY_STATUS BattStatus;  ///< Indicates if the L3 battery functions return status. It dosen't identify the exact error.
    uint8_t        RetryCount;
    uint16_t        Temp16;

    RetryCount = 0x0;
    do
    {
        
        OSTimeDly(SM_BUS_DELAY);//SM bus settling time
        BattStatus |= L3_BatteryGetVoltage(&Temp16); // Read Battery Voltage

        if (BATTERY_STATUS_OK == BattStatus)
        {
            ChargerData.Info.BatteryVoltage = Temp16;
            ChargerData.Info.BatteryLevel = BatteryCalculateRSOC(ChargerData.Info.BatteryVoltage); // Calculate Battery RSOC based on Battery voltage
            RetryCount = 0;
            break;
        }
        else // In case of read fail, retry reading the Battery parameter
        {
            RetryCount++;
            BattStatus = (RetryCount < BATTPARM_RETRYCOUNT) ? BATTERY_STATUS_OK : BattStatus;
        }
    } while (RetryCount < BATTPARM_RETRYCOUNT);

    if (BattStatus != BATTERY_STATUS_OK)
    {
        Log(DBG, "BatteryVoltage Read Error");
    }

    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Read Battery Temperature from BQ chip
 *
 * \details The following Battery parameters are read from the BQ chip:
 *              - The Battery Temperature in DegC
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Read is succesful
 *                           BATTERY_STATUS_ERROR if  read failed
 *
 * ========================================================================== */
static BATTERY_STATUS ReadBatteryTemperature(void)
{
    BATTERY_STATUS BattStatus;  ///< Indicates if the L3 battery functions return status. It dosen't identify the exact error.
    uint8_t        RetryCount;
    uint16_t       Temp16;

    RetryCount = 0x0;
    do
    {
        OSTimeDly(SM_BUS_DELAY);//SM bus settling time
        BattStatus |= L3_BatteryGetTemperature(&Temp16); // Read Battery Temperature
        if (BATTERY_STATUS_OK == BattStatus)
        {

            ChargerData.Info.BatteryTemperature = KELVIN_TO_CELSIUS((float32_t)Temp16); // convert temperature to Celsius
            RetryCount = 0;
            break;
        }
        else  // In case of read fail, retry reading the Battery parameter
        {
            RetryCount++;
            BattStatus = (RetryCount < BATTPARM_RETRYCOUNT) ? BATTERY_STATUS_OK : BattStatus;
        }
    } while (RetryCount < BATTPARM_RETRYCOUNT);

    if (BattStatus != BATTERY_STATUS_OK)
    {
        Log(DBG, "BatteryTemperature Read Error");
    }

    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Read Battery Cell Voltage from BQ chip
 *
 * \details The following Battery paramerters are read from the BQ chip
 *           Battery Cell0 Voltage in mV
 *           Battery Cell1 Voltage in mV
 *
 * \param   BatteryCell - Battery Cell number (Cell0, Cell1)
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Read is succesful
 *                           BATTERY_STATUS_ERROR if  read failed
 *
 * ========================================================================== */
static BATTERY_STATUS ReadBatteryCellVoltage(BATTERY_CELL_NUMBER BatteryCell)
{
    BATTERY_STATUS BattStatus;  ///< Indicates if the L3 battery functions return status. It doesn't identify the exact error.
    uint8_t        RetryCount;
    uint16_t       Temp16;
    RetryCount = 0u;

    do
    {
        OSTimeDly(SM_BUS_DELAY);//SM bus settling time
        BattStatus |= L3_BatteryGetCellVoltage(BatteryCell, &Temp16); // Read Battery Cell0 Voltage
        if (BATTERY_STATUS_OK == BattStatus)
        {
            if (CELL_0 == BatteryCell)
            {
                ChargerData.Info.BatteryCell0Voltage = Temp16;
            }
            else if (CELL_1 == BatteryCell)
            {
                ChargerData.Info.BatteryCell1Voltage = Temp16;
            }
            RetryCount = 0;
            break;
        }
        else  // In case of read fail, retry reading the Battery parameter
        {
            RetryCount++;
            BattStatus = (RetryCount < BATTPARM_RETRYCOUNT) ? BATTERY_STATUS_OK : BattStatus;
        }
    } while (RetryCount < BATTPARM_RETRYCOUNT);

    if (BattStatus != BATTERY_STATUS_OK)
    {
        Log(DBG, "BatteryCellVoltage Read Error, cell %d", BatteryCell);
    }

    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Read Battery Status from BQ chip
 *
 * \details The following Battery paramerters are read from the BQ chip
 *           Battery Status - Charging
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Read is succesful
 *                           BATTERY_STATUS_ERROR if  read failed
 * ========================================================================== */
static BATTERY_STATUS ReadBatteryChargingStatus(void)
{
    BATTERY_STATUS BattStatus;  ///< Indicates if the L3 battery functions return status. It dosen't identify the exact error.
    uint8_t        RetryCount;
    uint16_t        Temp16;
    uint8_t         Size;

    RetryCount = 0x0;
    do
    {
        OSTimeDly(SM_BUS_DELAY);//SM bus settling time
        BattStatus |= L3_BatteryGetStatus(CMD_CHARGING_STATUS, &Size, (uint8_t *)&Temp16); // Read Battery Charging Status
        if (BATTERY_STATUS_OK == BattStatus)
        {
            ChargerData.Info.BatteryChargeSts = Temp16;
            RetryCount = 0;
            break;
        }
        else  // In case of read fail, retry reading the Battery parameter
        {
            RetryCount++;
            BattStatus = (RetryCount < BATTPARM_RETRYCOUNT) ? BATTERY_STATUS_OK : BattStatus;
        }
    } while (RetryCount < BATTPARM_RETRYCOUNT);

    if (BattStatus != BATTERY_STATUS_OK)
    {
        Log(DBG, "BatteryChargeSts Read Error");
    }

    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Read Battery Status from BQ chip
 *
 * \details The following Battery paramerters are read from the BQ chip
 *           Battery Status - Gauging
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Read is succesful
 *                           BATTERY_STATUS_ERROR if  read failed
 *
 * ========================================================================== */
static BATTERY_STATUS ReadBatteryGaugingStatus(void)
{
    BATTERY_STATUS BattStatus;  ///< Indicates if the L3 battery functions return status. It dosen't identify the exact error.
    uint8_t        RetryCount;
    uint16_t        Temp16;
    uint8_t         Size;

    RetryCount = 0x0;
    do
    {
        OSTimeDly(SM_BUS_DELAY);//SM bus settling time
        BattStatus |= L3_BatteryGetStatus(CMD_GAUGING_STATUS, &Size, (uint8_t *)&Temp16); // Read Battery Gaug Status
        if (BATTERY_STATUS_OK == BattStatus)
        {
            ChargerData.Info.BatteryGaugSts = Temp16;
            RetryCount = 0;
            break;
        }
        else  // In case of read fail, retry reading the Battery parameter
        {
            RetryCount++;
            BattStatus = (RetryCount < BATTPARM_RETRYCOUNT) ? BATTERY_STATUS_OK : BattStatus;
        }
    } while (RetryCount < BATTPARM_RETRYCOUNT);

    if (BattStatus != BATTERY_STATUS_OK)
    {
        Log(DBG, "BatteryGaugeSts Read Error");
    }

    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Read Battery Status from BQ chip
 *
 * \details The following Battery paramerters are read from the BQ chip
 *           Battery Status - Safety
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Read is succesful
 *                           BATTERY_STATUS_ERROR if  read failed
 *
 * ========================================================================== */
static BATTERY_STATUS ReadBatterySafetyStatus(void)
{
    BATTERY_STATUS BattStatus;  // Indicates if the L3 battery functions return status. It dosen't identify the exact error.
    uint8_t        RetryCount;
    uint32_t       Temp32;
    uint8_t        Size;

    RetryCount = 0x0;
    do
    {
        OSTimeDly(SM_BUS_DELAY);//SM bus settling time
        BattStatus |= L3_BatteryGetStatus(CMD_SAFETY_STATUS, &Size, (uint8_t *)&Temp32); // Read Battery Safety Status
        if (BATTERY_STATUS_OK == BattStatus)
        {
            ChargerData.Info.BatterySafetySts = Temp32;
            RetryCount = 0;
            break;
        }
        else  // In case of read fail, retry reading the Battery parameter
        {
            RetryCount++;
            BattStatus = (RetryCount < BATTPARM_RETRYCOUNT) ? BATTERY_STATUS_OK : BattStatus;
        }
    } while (RetryCount < BATTPARM_RETRYCOUNT);

    if (BattStatus != BATTERY_STATUS_OK)
    {
        Log(DBG, "BatterySafetySts Read Error");
    }

    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Read Battery Status from BQ chip
 *
 * \details The following Battery paramerters are read from the BQ chip
 *           Battery Status - Battery Operation
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Read is succesful
 *                           BATTERY_STATUS_ERROR if  read failed
 *
 * \note BATTERY_STATUS_OK should be 0 in the enum
 * ========================================================================== */
static BATTERY_STATUS ReadBatteryOperationStatus(void)
{
    BATTERY_STATUS BattStatus;  ///< Indicates if the L3 battery functions return status. It dosen't identify the exact error.
    uint8_t        RetryCount;
    uint32_t        Temp32;
    uint8_t         Size;

    RetryCount = 0x0;
    do
    {
        OSTimeDly(SM_BUS_DELAY);//SM bus settling time
        BattStatus |= L3_BatteryGetStatus(CMD_OPERATION_STATUS, &Size, (uint8_t *)&Temp32); // Read Battery Operation Status
        if (BATTERY_STATUS_OK == BattStatus)
        {
            ChargerData.Info.BatteryOperationSts = Temp32;
            RetryCount = 0;
            break;
        }
        else  // In case of read fail, retry reading the Battery parameter
        {
            RetryCount++;
            BattStatus = (RetryCount < BATTPARM_RETRYCOUNT) ? BATTERY_STATUS_OK : BattStatus;
        }
    } while (RetryCount < BATTPARM_RETRYCOUNT);

    if (BattStatus != BATTERY_STATUS_OK)
    {
        Log(DBG, "BatteryOpertionStatus Read Error");
    }

    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Read Battery Status from BQ chip
 *
 * \details The following Battery paramerters are read from the BQ chip
 *           Battery Status - Permanent Fault (PF)
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Read is succesful
 *                           BATTERY_STATUS_ERROR if  read failed
 *
 * ========================================================================== */
static BATTERY_STATUS ReadBatteryPermanentFailFStatus(void)
{
    BATTERY_STATUS BattStatus;  ///< Indicates if the L3 battery functions return status. It dosen't identify the exact error.
    uint8_t        RetryCount;
    uint32_t       Temp32;
    uint8_t        Size;

    RetryCount = 0x0;
    do
    {
        OSTimeDly(SM_BUS_DELAY);//SM bus settling time
        BattStatus |= L3_BatteryGetStatus(CMD_PF_STATUS, &Size, (uint8_t *)&Temp32); // Read Battery Permanent Fail Status
        if (BATTERY_STATUS_OK == BattStatus)
        {
            ChargerData.Info.BatteryPFSts = Temp32;
            RetryCount = 0;
            break;
        }
        else  // In case of read fail, retry reading the Battery parameter
        {
            RetryCount++;
            BattStatus = (RetryCount < BATTPARM_RETRYCOUNT) ? BATTERY_STATUS_OK : BattStatus;
        }
    } while (RetryCount < BATTPARM_RETRYCOUNT);

    if (BattStatus != BATTERY_STATUS_OK)
    {
        Log(DBG, "BatteryPermanentFailStatus Read Error");
    }

    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Read update Battery Charger count cycle
 *
 * \details This function implements the charge cycle count read logic. This function
 *          cycles through various states every CHARGER_TASK_PERIOD_2MIN to read/write charge
 *          cycle count value battery 1-wire EEPROM and Battery BQ chip.
 *
 *          For legacy on new firmware upload, The BQ chip charge cycle count is read and store directly into OW memory
 *          for the backward compatibility,
 *          noInitRam flag i.e. if not "true" is used to find first time entry for new upload
 *          Then BQ value is set to "1" to start fresh, noInitRam flag is set to true
 *
 *          The BQ chip charge cycle count is read and set to "1" once its >= BQ_CYCLE_COUNT_SET_LIMIT
 *          The read value is added to the current cycle count value saved in 1-wire
 *          battery EEPROM.
 *
 * \param  < None >
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Bq chip/eeprom Read/write is succesful
 *                           BATTERY_STATUS_ERROR if read failed
 *
 * \note    BATTERY_STATUS_OK should be 0 in the enum
 *
 * ========================================================================== */
static BATTERY_STATUS ReadUpdateBatteryChgrCycleCnt(void)
{
    BATTERY_STATUS BattStatus; ///< Indicates if the L3 battery functions return status. It dosen't identify the exact error.
    AM_STATUS      AMStatus;
    static uint16_t       	BqChipCycleCount = 0;
    static uint8_t 			RetryCount;
    static STATE_CYCLE_COUNT_UPDATE  StateNext = STATE_BQ_1W_READ_ANALYZE;
    static bool FirstTimeEntry = false;
    BattStatus = BATTERY_STATUS_ERROR; ///< Indicates if the L3 battery functions return status. It dosen't identify the exact error.
    AMStatus = AM_STATUS_ERROR;

    switch (StateNext)
    {
        case STATE_BQ_1W_READ_ANALYZE:
         do
          {
              //Read Battery Charge Cycle count from BQ
              BattStatus = L3_BatteryGetChgrCntCycle(&BqChipCycleCount);

              //Read battery 1-wire EEPROM to get the saved cycle count value
              AMStatus = pBatteryInterface->Read();

              //Check 1-W data and BQ data is greater than MAX_CHARGE_CYCLEC_COUNT
              if ((MAX_CHARGE_CYCLEC_COUNT < BqChipCycleCount) || ((BATTERY_STATUS_OK != BattStatus) || (AM_STATUS_OK != AMStatus)))
              {
                  RetryCount++;
                  if (BATTPARM_RETRYCOUNT == RetryCount)
                  {
                      StateNext = STATE_BQ_DEFAULT;
                  }
                  break;
              }

              /* First time */
              if ((0 == pBatteryInterface->Data.ChargeCycleCount) || (BATT_MAX_CHARGECYCLECOUNT < pBatteryInterface->Data.ChargeCycleCount))
              {
                  //Clear the 1-W data
                  pBatteryInterface->Data.ChargeCycleCount = 0;
                  pBatteryInterface->Data.ChargeCycleCount += BqChipCycleCount;
                  ChargerData.Info.BatChgrCntCycle = pBatteryInterface->Data.ChargeCycleCount;
                  // write the updated cycle count to eeprom in next state
                  FirstTimeEntry = true;
                  StateNext = STATE_BQ_1W_UPDATE;
                  Log(DBG, "First Time Check for 1-W");
                  break;
              }
              else
              {
                  /* Not First time & Charge Cycle count is equal to BQ_CYCLE_COUNT_SET_LIMIT */
                  if (BQ_CYCLE_COUNT_SET_LIMIT == BqChipCycleCount)
                  {
                      pBatteryInterface->Data.ChargeCycleCount += BqChipCycleCount - 1;
                      ChargerData.Info.BatChgrCntCycle = pBatteryInterface->Data.ChargeCycleCount;
                      // write the updated cycle count to eeprom in next state
                      StateNext = STATE_BQ_1W_UPDATE;
                      break;
                  }
                  /* Not First time & Charge Cycle count is less than BQ_CYCLE_COUNT_SET_LIMIT */
                  if (BQ_CYCLE_COUNT_SET_LIMIT > BqChipCycleCount)
                  {
                      pBatteryInterface->Data.ChargeCycleCount += BqChipCycleCount - 1;
                      ChargerData.Info.BatChgrCntCycle = pBatteryInterface->Data.ChargeCycleCount;
                      // write the updated cycle count to eeprom in next state
                      if (BATT_MAX_CHARGECYCLECOUNT == pBatteryInterface->Data.ChargeCycleCount)
                      {
                          // update 1W/BQ and exit
                          StateNext = STATE_BQ_1W_UPDATE;
                      }
                      else
                      {
                          StateNext = STATE_BQ_CHECK_EOL;
                      }
                      break;
                  }
              }
          }while(false);
          break;

          // Update the 1-W and REset BQ Chip
          case STATE_BQ_1W_UPDATE:
          do
          {
              OSTimeDly(MSEC_BQ_50);//SM bus settling time
              //reset the BQ cycle count to 1 on First time entry
              BqChipCycleCount = CYCLE_COUNT_RESET_VALUE;
              BattStatus = UpdateBQChipChgrCntCycle(BqChipCycleCount);

		      if (BATTERY_STATUS_OK != BattStatus)
              {
                  RetryCount++;
                  if (BATTPARM_RETRYCOUNT == RetryCount)
                  {
                      StateNext = STATE_BQ_END;
                      break;
                  }
                  else
                  {
                      Log(DBG, "Update BQ Chip failed");
                      break;
                  }
              }
              AMStatus = pBatteryInterface->Update();
              BattStatus = (AMStatus == AM_STATUS_OK) ? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR;

              if (BATTERY_STATUS_OK != BattStatus)
              {
                  Log(DBG, "Update 1-W failed - STATE_UPDATE_FIRSTENTRY");
                  StateNext = STATE_BQ_END;
                  break;
              }

		      // Stops the Battery Charge Cycle State machine if First time
              if (FirstTimeEntry)
              {
                  StateNext = STATE_BQ_END;
              }
              else
              {
                  StateNext = STATE_BQ_CHECK_EOL;
              }
              Log(DBG, "Successful-Updated 1-w and BQ Chip");
          }while(false);
          break;

        //Not First Time and Check EOL
        case STATE_BQ_CHECK_EOL:
          if (BATT_MAX_CHARGECYCLECOUNT > ChargerData.Info.BatChgrCntCycle)
          {
              ;// Stops the Battery Charge Cycle State machine
          }
          else
          {
              // Charge Cycle Remaining is zero, raise EOL fault
              FaultHandlerSetFault(HANDLE_EOL_ZEROBATTCHARGECYCLE, SET_ERROR);
          }
          StateNext = STATE_BQ_END;
          Log(DBG, "Successful-Checked Handle EOL");
          break;

        case STATE_BQ_DEFAULT: //STATE_DEFAULT is handling only - STATE_READ_BQ_CHIP_CYCLE_COUNT state
          Log(DBG, "Battery charge cycle count Read Failed BQ value =%d, BQ Status BQ= %d, 1-W Status 1-W =%d", BqChipCycleCount,BattStatus,AMStatus);

          // Stops the Battery Charge Cycle State machine
          ChargerData.ChargeCycleUpdated = true;
          break;

       case STATE_BQ_END:
          // Stops the Battery Charge Cycle State machine
          ChargerData.Info.RemainingChargeCycleCount = MAX_CHARGE_CYCLEC_COUNT - ChargerData.Info.BatChgrCntCycle;
          Log(DBG, "Remaining Charge Cycle Count = %d", ChargerData.Info.RemainingChargeCycleCount);
          ChargerData.ChargeCycleUpdated = true;
          break;
    }
    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Read Battery Current from BQ Temperatures
 *
 * \details The following Battery paramerters are read from the BQ chip
 *           Battery TS1 temperature
 *           Battery TS2 temperature
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Read is succesful
 *                           BATTERY_STATUS_ERROR if  read failed
 * ========================================================================== */
static BATTERY_STATUS ReadBatteryTemperatures(void)
{
    BATTERY_STATUS BattStatus;  ///< Indicates if the L3 battery functions return status. It dosen't identify the exact error.
    uint8_t        RetryCount;
    uint16_t        Temp16;
    uint8_t         Size;
    static uint8_t  data[TEMPERATUREBUFF_SIZE]; ///< buffer to hold the Temperature values

    RetryCount = 0x0;
    do
    {
        OSTimeDly(SM_BUS_DELAY);//SM bus settling time
        ///\todo 3/3/2022 - BS: Battery Temeratures are not reading correct values further investigation needed.
        BattStatus |= L3_BatteryGetTemperatures(&Size, data); // Read Battery Temperatures
        if (BATTERY_STATUS_OK == BattStatus)
        {
            memcpy(&Temp16, &data[INTTEMP_OFFSET], TEMPERATURE_SIZE);                    // Get Battery Internal Temperature from buffer
            ChargerData.Info.InternalTemperature = KELVIN_TO_CELSIUS((float32_t)Temp16); // convert the Temperature to deg Celsius
            memcpy(&Temp16, &data[TS1TEMP_OFFSET], TEMPERATURE_SIZE);                    // Get Battery Cell0 Temperature from buffer
            ChargerData.Info.TS1Temperature = KELVIN_TO_CELSIUS((float32_t)Temp16);      // convert the Temperature to deg Celsius
            memcpy(&Temp16, &data[TS2TEMP_OFFSET], TEMPERATURE_SIZE);                    // Get Battery Cell1 Temperature from buffer
            ChargerData.Info.TS2Temperature = KELVIN_TO_CELSIUS((float32_t)Temp16);      // convert the Temperature to deg Celsius
            RetryCount = 0;
            break;
        }
        else  // In case of read fail, retry reading the Battery parameter
        {
            RetryCount++;
            BattStatus = (RetryCount < BATTPARM_RETRYCOUNT) ? BATTERY_STATUS_OK : BattStatus;
        }
    } while (RetryCount < BATTPARM_RETRYCOUNT);

    if (BattStatus != BATTERY_STATUS_OK)
    {
        Log(DBG, "Battery Temperatures Read Error");
    }

    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Read / Write to Battery Data Flash
 *
 * \details The following Battery paramerters are read from the BQ chip
 *           Battery Voltage in mV
 *
 * \param   Index - Array Index of the BatteryDF_Default Structure
 *          Data - Data pointer for data to be read by the API
 *          DataAccess - DF_READ  - Read from Battery Data Flash
 *                       DF_WRITE - Write to the Battery Data Flash
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Read is succesful
 *                           BATTERY_STATUS_ERROR if  read failed
 *
 * ========================================================================== */
static BATTERY_STATUS ReadWriteBatteryDF(uint8_t Index, uint8_t *Data, DATAFLASHRW DataAccess)
{
    BATTERY_STATUS BattStatus;  ///< Indicates if the L3 battery functions return status. It dosen't identify the exact error.

    uint8_t  RetryCount;

    RetryCount = 0x0;
    do
    {
        /* Read / Write based on the Request received */
        if (DF_READ == DataAccess)
        {
            BattStatus = L3_BatteryGetDataFlash(&(BatteryDF_Default[Index].DFinfo), Data);
        }
        else if (DF_WRITE == DataAccess)
        {
            BattStatus = L3_BatterySetDataFlash(&(BatteryDF_Default[Index].DFinfo), Data);
        }
        else
        {
            /* Invalid option */
            BattStatus = BATTERY_STATUS_ERROR;
            break;
        }

        if (BATTERY_STATUS_OK == BattStatus)
        {
            RetryCount = 0;
            break;
        }
        else // In case of read or write fail, retry
        {
            RetryCount++;
            /* It is observed that I2C writes fail sometimes. Delay before next retry */
            OSTimeDly(MSEC_1);
            BattStatus = (RetryCount < BATTPARM_RETRYCOUNT) ? BATTERY_STATUS_OK : BattStatus;
        }
    } while (RetryCount < BATTPARM_RETRYCOUNT);

    if (BattStatus != BATTERY_STATUS_OK)
    {
        if (DF_READ == DataAccess)
        {
            Log(DBG, "BatteryDF Read Error");
        }
        else
        {
            Log(DBG, "BatteryDF write Error");
        }
    }

    return BattStatus;
}

/* ========================================================================== */
/**
 * \brief   Update Battery Charger count cycle in BQ chip
 *
 * \details This function updates the battery charge cycle count value in BQ chip.
 *
 * \param   CycleCountValue- New cycle count value to be updated in BQ chip
 *
 *
 * \return  BATTERY_STATUS - BATTERY_STATUS_OK if Reset is succesful
 *                           BATTERY_STATUS_ERROR if operation failed
 *
 * \note BATTERY_STATUS_OK should be 0 in the enum
 * ========================================================================== */
static BATTERY_STATUS UpdateBQChipChgrCntCycle(uint16_t CycleCountValue)
{
    uint8_t Data[2];

    Data[0] = CycleCountValue >> 8;  //Big endian comms
    Data[1] = CycleCountValue;

    return L3_BatterySetDataFlash(&BatteryDFCycleCount, (uint8_t *)Data);
}

/* ========================================================================== */
/**
 * \brief     Check for Battery DataFlash (DF) parameter values
 *
 * \details   The check, for parameter values in Battery DataFlash, happens once during startup
 *            The routine checks the list of Data Flash parameters with the expected Default values
 *            if the values differ from the Default value the parameter is updated with Default value
 *                     DSG_CURRENT_THD                          - DSG Current Threshold - Class: 249, Offset: 0
 *                     CHG_CURRENT_THD                          - CHG Current Threshold - Class: 249, Offset: 2
 *                     QUIT_CURRENT                             - Quit Current Threshold - Class: 249, Offset: 4
 *                     CYCLE_COUNT_PERC                         - Cycle Count Percentage - Class: 489, Offset: 18
 *                     ENABLED_PF_0_15                          - Enabaled PF 0 to 15 bits - Class: 197, Offset: 0
 *                     SHUTDOWN_TIME                            - Shutdown Time - Class: 230, Offset: 2
 *                     PRECHARGING_CURRENT                      - Pre-Charging current - Class: 148, Offset: 0
 *                     MIN_START_BALANCE_DELTA                  - Cell Balancing congi, Min Start Balance Delta - Class: 168, Offset: 4
 *                     CURRENT_DEADBAND                         - Current Deadband - Class: 103, Offset: 0
 *                     VALID_VOLTAGE_UPDATE                     - Valid Update Voltage - Class: 228, Offset: 0
 *                     SBS_DATA_CONFIG_0_15                     - Setting Configuration - Class: 201, Offset: 9
 *                     CHARGING_CONFIG- Setting Configuration   - Class: 201, Offset: 3
 *                     CLEAR_VOLTAGE_THD                        - Clear voltage threshold - Class: 578, Offset: 2
 *           The parameters in Battery DataFlash are used by bq30z554 chip working(like the internal working mode, Impedance Track gauging algorithm etc...)
 *
 * \param   < None >
 *
 * \return  CHRG_MNGR_STATUS
 * \retval  CHRG_MNGR_STATUS_OK - if the check is successful
 * \retval  CHRG_MNGR_STATUS_ERROR - if the check is unsuccessful
 *
 * ========================================================================== */
static CHRG_MNGR_STATUS ChargerMgrChkBatteryDF(void)
{
    uint8_t i;
    uint8_t sz;
    uint8_t Data[2];
    uint16_t value;

    BATTERY_STATUS BattStatus;
    CHRG_MNGR_STATUS Status;

    for (i = 0; i < DF_COUNT; i++)
    {
        // Read the DataFlash Parameter
        BattStatus = ReadWriteBatteryDF(i,  Data, DF_READ);

        BREAK_IF(BATTERY_STATUS_ERROR == BattStatus);
        sz = BatteryDF_Default[i].DFinfo.Size;
        value = 0;
        for (sz = 0; sz < BatteryDF_Default[i].DFinfo.Size; sz++)
        {
            value <<= 8;
            value |= Data[sz];
        }

        // check if the DataFlash Parameter is same as the exptected Default value. If not update the Data flash value with Default value
        if (value != BatteryDF_Default[i].DefaultValue)
        {
            Log(REQ, "Updated Battery DataFlash SubClass id = %d, Read value = %d, Update value = %d", BatteryDF_Default[i].DFinfo.SubClsId, value, BatteryDF_Default[i].DefaultValue);
            sz = 0;
            value = BatteryDF_Default[i].DefaultValue;
            if (BATTERY_DF_SIZE_ONEBYTE == BatteryDF_Default[i].DFinfo.Size)
            {
                Data[0] = (value & 0xff);
            }
            else
            {
                Data[0] = ((value >> 8) & 0xff);
                Data[1] = (value & 0xff);
            }
            BattStatus = ReadWriteBatteryDF(i,  Data, DF_WRITE);
        }
        else
        {
            //Log(DBG, "Battery DataField SubClass id = %d OK", BatteryDF_Default[i].DFinfo.SubClsId);
        }
        OSTimeDly(SM_BUS_DELAY);
    }
    Status = ((BATTERY_STATUS_OK == BattStatus) ? CHRG_MNGR_STATUS_OK : CHRG_MNGR_STATUS_ERROR);

    TM_Hook(HOOK_BATTCOMMSIMULATE, &Status);
    return Status;
}

/* ========================================================================== */
/**
 * \brief      Get Battery type from Battery Chemical ID
 *
 * \todo 07/27/2022 DAZ - NEED DETAILS FOR BatteryGetTypefromChemID
 * \details
 * \note    This actually returns the manufacturer, not the battery type
 *
 * \param   ChemicalID - battery chemistry
 *
 * \return  CHGR_MNGR_BATTERYTYPES
 *
 * ========================================================================== */
static CHGR_MNGR_BATTERYTYPES BatteryGetTypefromChemID(uint16_t ChemicalID)
{
    CHGR_MNGR_BATTERYTYPES i;

    for (i = BATTERY_PANASONIC; i < BATTERY_TYPE_LAST; i++)
    {
        BREAK_IF(ChemicalID == BatteryDesignParams[i].ChemID);
    }
    if (BATTERY_TYPE_LAST == i)
    {
        i = BATTERY_PANASONIC;
    }
    return i;
}

/* ========================================================================== */
/**
 * \brief      ChargerMgrTask
 *
 * \details   Charger Manager task periodically checks for charger connection
 *            Responsible for charging, monitoring of the battery
 *
 * \param   pArg - Pointer to task specific initialization data (unused)
 *
 * \return  None
 *
 * ========================================================================== */
static void ChargerMgrTask(void *pArg)
{
    static bool     PreviousOnChargerStatus = false;
    bool            NewOnChargerStatus;
    static bool     OnCharger = false;
    uint8_t         ChargerDeviceType;
    uint16_t        Temp16;
    uint8_t         RetryCount;
    static uint32_t BatteryLogTimer = BATTERY_LOGPERIOD;  // force log message at task start
    CHARGER_COMM_STATUS ChargerCommStatus;
    BATTERY_STATUS  BatteryStatus;
    static uint8_t  BatteryOWCheckCount;


    ErrorInfoInit();
    L3_BatteryPECEnable();

    ///\todo 3/3/2022 - BS: Chemical ID is not reading correct values further investigation needed. For now by default Panasonic battery is selected
    // Ready battery Chemical ID once during startup, if read failed update Chemical ID as PANASONIC
    BatteryStatus = L3_BatteryGetChemicalID(&Temp16);
    if ((BATTERY_STATUS_OK == BatteryStatus) && (BatteryGetTypefromChemID(Temp16) < BATTERY_TYPE_LAST))
    {
        ChargerData.Info.BatteryType = BatteryGetTypefromChemID(Temp16);
        ChargerData.Info.pBattParam = &BatteryDesignParams[ChargerData.Info.BatteryType];
        Log(DBG, "Chemical ID = %s", &BatteryManufacturer[ChargerData.Info.BatteryType][0]);
    }
    else
    {
        ChargerData.Info.BatteryType = BATTERY_DEFAULT;
        Log(DBG, "Chemical ID error, default assigned = %s", &BatteryManufacturer[ChargerData.Info.BatteryType][0]);
    }

    // Read the battery DF parameters and compare them to the default. If different update them with default
    if (CHRG_MNGR_STATUS_OK != ChargerMgrChkBatteryDF())
    {
        FaultHandlerSetFault(BATTERY_COMMFAIL, SET_ERROR);
        Log(DBG, "Battery Comm Error");
    }
    else
    {
        Log(DBG, "Battery Data Flash Check Successful");
    }

    while (true)
    {
        do
        {
            // Charger connect status read with debounce logic, loop delay used as bebounce delay
            (void)L3_GpioCtrlGetSignal((GPIO_SIGNAL)GPIO_PERIPHERAL_WUn, (bool *)&NewOnChargerStatus);

            // Charger status is active low, hence compliment it
            NewOnChargerStatus = !NewOnChargerStatus;
            // Confirm charger connection by communicating with it
            if (NewOnChargerStatus)
            {
                RetryCount = 0;
                do
                {   ///\todo 3/14/2022 BS - Ping command failing occasionally. This leads to handle rebooting while on charger.
                    ChargerCommStatus = L3_ChargerCommPing(&ChargerDeviceType);
                    if ((CHARGER_COMM_STATUS_OK != ChargerCommStatus))
                    {
                        RetryCount++;
                        NewOnChargerStatus = false;
                    }
                    else
                    {
                        NewOnChargerStatus = true;
                        break;
                    }
                } while (RetryCount < CHRGRCMD_RETRYCOUNT);
            }

            // A change in Charger state is detected
            if (NewOnChargerStatus ^ OnCharger)
            {
                // For OnCharger state to be detected as true, Handle should be on charger for at least one looptime. (5sec)
                OnCharger = (NewOnChargerStatus && PreviousOnChargerStatus);

                if (NewOnChargerStatus && !PreviousOnChargerStatus)
                {
                    Log(DBG, "ChargerManager Charger: Detected");
                }
                else
                {
                    Log(DBG, "ChargerManager Charger: %s", (OnCharger) ? "Connected" : "Disconnected");
                }
            }
            SetPowerPackAsMaster(OnCharger);
            PreviousOnChargerStatus = NewOnChargerStatus;

            
            if ((AM_STATUS_OK == pBatteryInterface->Status) && (AM_STATUS_OK == pHandleIF->Status))
            {
                 if(!ChargerData.ChargeCycleUpdated)
                 { 
                    // Read the BQ Charge Cycle Count and update the battery 1-wire 
                    ReadUpdateBatteryChgrCycleCnt();
                 }
            }
            else
            {
                BatteryOWCheckCount++;
                if (BATTERY_OW_CHECK_MAX_RETRY == BatteryOWCheckCount)
                {
                    FaultHandlerSetFault(REQRST_MOO_SYSTEM_FAULT,SET_ERROR);
                }
            }


            // Read battery parameters irrespective of current state.
            if (CHRG_MNGR_STATUS_OK != ReadBatteryParameters(&ChargerData.Info, OnCharger))
            {
                SetErrorInfo(BATT_COMMFAILURE, BATTERY_COMMFAIL);
            }
            else
            {
                /* Clear Error Cause, ErrorPublished: Used to set the error */
                ClearErrorInfo(BATT_COMMFAILURE, LAST_ERROR_CAUSE);
            }

            L3_ChargerCommRelPowerPackMaster();

            /* Check Battery Temperature Out of Range */
            CheckBatteryTemperatureValidRange();
            BatteryLogTimer += CHARGER_TASK_PERIOD;

            if (OnCharger)
            {
                BatteryLogTimer = 0;
            }
            else
            {
                if (BatteryLogTimer > BATTERY_LOGPERIOD)
                {
                    BatteryLogTimer = 0;
                    BatteryLogInfo(&(ChargerData.Info));
                }
            }

            /* Process Charger manager State Machine */
            ChargerMgrStateMachine(OnCharger);

            /* Notify User Events to the registered users */
            NotifyChargerMgrUserEvents();

            ChargerData.Info.BatCommState = ChargerManagerState;

            /* Log Battery Errors */
            LogBatteryErrors();

          //  Log(DBG, "ChargerMgrTask..END..");

        } while( false );

        OSTimeDly(CHARGER_TASK_PERIOD);
    }
}

/* ========================================================================== */
/**
 * \brief      This function sets the power pack as master
 *
 * \details    This function sets the power pack as master and sets the corresponding
 *             flag to the Charger Info structure
 *
 * \param   OnCharger - True  - Handle currently On  Charger
 *                      False - Handle currently Off Charger
 *
 * \return  None
 *
 * ========================================================================== */
static void SetPowerPackAsMaster(bool OnCharger)
{
    uint8_t RetryCount;

    ChargerData.IsPPMaster = true;
    if (OnCharger)
    {
        // Set Power pack as SMBus master, if the request fails retry 2 times every 250ms
        ///\ todo 12/03/2021 BS - Expectation is, SMBus transfers from PP should happen only if pp is master. check for flag before transfer to be updated.
        RetryCount = 0;
        while (RetryCount < BATTPARM_RETRYCOUNT)
        {
            if (CHARGER_COMM_STATUS_OK != L3_ChargerCommSetPowerPackMaster())
            {
                ChargerData.IsPPMaster = false;
                RetryCount++;
                OSTimeDly(MSEC_250);
            }
            else
            {
                ChargerData.IsPPMaster = true;
                break;
            }
        }
    }
}

/* ========================================================================== */
/**
 * \brief      ChargerMgrTask State Machine
 *
 * \details   This State machine is part of Charger Manager task
 *            This State Machine is Responsible for Handle State changes such as
 *                 -Handle Connected to Charger
 *                 -Handle Disconnected to Charger
 *                 -Handle Charging
 *                 -Handle Charged and
 *                 -Fault handling while Charging
 *
 *
 * \param   OnCharger - True  - Handle currently On  Charger
 *                      False - Handle currently Off Charger
 *
 * \return  None
 *
 * ========================================================================== */
static void ChargerMgrStateMachine(bool OnCharger)
{
    static uint32_t Timeout;
    uint16_t        Temp16;
    uint8_t         ChargerDeviceType;

    // Main charger state-machine
    switch (ChargerManagerState)
    {
        case CHRG_MNGR_STATE_DISCONNECTED:

            if (OnCharger)
            {

                BREAK_IF(CHARGER_COMM_STATUS_OK != L3_ChargerCommPing(&ChargerDeviceType));
                BREAK_IF(CHARGER_COMM_STATUS_OK != L3_ChargerCommGetVersion(&Temp16));
                ChargerData.Version = Temp16;

                /* Made firm contact with Charger, transition to connected state */
                ChargerManagerState = CHRG_MNGR_STATE_CONNECTED;
                Log(DBG, "ChargerManager: Communicating. Id: %d, SW Ver: %x", ChargerDeviceType, ChargerData.Version);

                // Load a timer to wait for pre-charge stage to get over
                Timeout = CHARGER_PRECHARGE_DURATION;
            }
            break;

        case CHRG_MNGR_STATE_CONNECTED:
            do
            {
                if (!OnCharger)
                {
                    ChargerManagerState = CHRG_MNGR_STATE_DISCONNECTED;
                    Log(DBG, "ChargerManager: Disconnected, Battery level: %.1f",  ChargerData.Info.BatteryLevel);
                    break;
                }

                // Inform charger that it is authenticated by Handle
                BREAK_IF(CHARGER_COMM_STATUS_OK != L3_ChargerCommSetAuthResult(true));
                Timeout = FULL_CHARGE_TIMEOUT_TICKS;     // Reset timeout
                ProcessConnectedState();
            } while (false);
            break;

        case CHRG_MNGR_STATE_CHARGING:
            do
            {

                SetBatteryChargingCycleFaults();

                // Check if charging is taking too much time
                if (Timeout == 0)
                {
                    Log(DBG, "Charger timed out(%d mins), battery level: %.1f", FULL_CHARGE_TIMEOUT,  ChargerData.Info.BatteryLevel);
                    ChargerManagerState = CHRG_MNGR_STATE_FAULT;
                    break;
                }

                // Check if Handle is removed
                if (!OnCharger)
                {
                    ChargerManagerState = CHRG_MNGR_STATE_DISCONNECTED;
                    Log(DBG, "Charger Disconnected, battery level: %.1f",  ChargerData.Info.BatteryLevel);
                    break;
                }

                Timeout--;       // Update timeout counter
                ProcessChargingState();
            } while (false);
            break;

        case CHRG_MNGR_STATE_CHARGED:
            ProcessChargedState(OnCharger);
            break;

        case CHRG_MNGR_STATE_FAULT:
        case CHRG_MNGR_STATE_SLEEP:
            if (OnCharger)
            {
                // Check if the charger is really connected.
                if (CHARGER_COMM_STATUS_OK == L3_ChargerCommPing(&ChargerDeviceType))
                {
                    ChargerManagerState = CHRG_MNGR_STATE_CONNECTED;
                }
            }
            break;

        default:
            ChargerManagerState = CHRG_MNGR_STATE_DISCONNECTED;
            break;
    }
}

/* ========================================================================== */
/**
 * \brief      State handling of ChargerMgrTask State Machine
 *
 * \details   This function Processes the Handle State while on Charger
 *
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void ProcessChargingState(void)
{
    do
    {
        if ((ErrorInfo[BATT_TEMPERATURE].Error == BATTERY_TEMP_OUTOFRANGE) ||\
                (ErrorInfo[BATT_EOL].Error == BATT_CHARGECYCLE_EOL))
        {
            /* Battery Temperature is Out of Range, Disable Charging */
            ChargerData.StopRequest = true;
        }

        if ((ChargerData.Info.BatteryLevel >= ChargerData.RangeHi) ||  (ChargerData.StopRequest))
        {
            // Attempt stopping charger
            BREAK_IF(CHARGER_COMM_STATUS_OK != L3_ChargerCommStopCharging());

            if (ChargerData.StopRequest)
            {
                ChargerManagerState = CHRG_MNGR_STATE_SLEEP;
                Log(DBG, "Charging aborted, battery level: %.1f", ChargerData.Info.BatteryLevel);
            }
            else
            {
                ChargerManagerState = CHRG_MNGR_STATE_CHARGED;
                Log(DBG, "Charging stopped, battery reached sufficient level: %.1f", ChargerData.Info.BatteryLevel);
            }
        }

    } while (false);
}

/* ========================================================================== */
/**
 * \brief      State handling of ChargerMgrTask State Machine
 *
 * \details   This function Processes the Handle State while Handle is Charged
 *
 *
 * \param   OnCharger - True  - Handle currently On  Charger
 *                      False - Handle currently Off Charger
 *
 * \return  None
 *
 * ========================================================================== */
static void ProcessChargedState(bool OnCharger)
{
    do
    {
        if (!OnCharger)
        {
            ChargerManagerState = CHRG_MNGR_STATE_DISCONNECTED;
            Log(DBG, "Charger Disconnected, battery level: %f",  ChargerData.Info.BatteryLevel);
            break;

        }
        else if (ChargerData.Info.BatteryLevel < ChargerData.RangeLo)
        {
            /* Swtiching to CONNECTED state will again transition to CHARGING */
            ChargerManagerState = CHRG_MNGR_STATE_CONNECTED;
        }
        else
        {
            /* Dummy statement below to prevent static code check warning */
            ChargerManagerState = CHRG_MNGR_STATE_CHARGED;
        }
    } while (false);
}

/* ========================================================================== */
/**
 * \brief      State handling of ChargerMgrTask State Machine
 *
 * \details   This function Processes the Handle State while Handle is Connected to Charger
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void ProcessConnectedState(void)
{
    do
    {
        // Check if Handle already sufficient charge level
        if (ChargerData.Info.BatteryLevel >= ChargerData.RangeHi)
        {
            BREAK_IF(CHARGER_COMM_STATUS_OK != L3_ChargerCommStopCharging());
            ChargerManagerState = CHRG_MNGR_STATE_CHARGED;
            Log(DBG, "Charging stopped, battery is charged: %.1f, ",  ChargerData.Info.BatteryLevel);
        }
        else
        {
            if ( pHandleIF->Data.ProcedureCount < pHandleIF->Data.ProcedureLimit )
            {
                BREAK_IF(CHARGER_COMM_STATUS_OK != L3_ChargerCommStartCharging());
                ChargerManagerState = CHRG_MNGR_STATE_CHARGING;
                Log(DBG, "Charging started, battery level: %.1f",  ChargerData.Info.BatteryLevel);
            }
        }

    } while (false);
}

/* ========================================================================== */
/**
 * \brief      This function notifies User events to upper layer
 *
 * \details    This function publishes various user events to the upper layer.
 *             Registered Users are notified of the Charger events.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void NotifyChargerMgrUserEvents(void)
{
    CHARGER_INFO    InfoCopy;
    static uint32_t NotifyCounter = CHARGER_TASK_PERIOD;
    static CHARGER_EVENT PrevEvent;

    // Check if user to notified. Only events generated - Connected, Disconnected, Periodic data.
    if (ChargerData.Handler)
    {
        if (ChargerManagerState == CHRG_MNGR_STATE_DISCONNECTED)
        {
            ChargerData.Info.Event = CHARGER_EVENT_DISCONNECTED;

        }
        else
        {
            // check if Sleep is exited for battery health check
            if (GetSystemStatus(SYSTEM_STATUS_LLS_RESET) && PublishWakefromsleep)
            {
                PublishWakefromsleep = false;
                ChargerData.Info.Event = CHARGER_EVENT_WAKEUPONCHARGER;
            }
            else
            {
                ChargerData.Info.Event = CHARGER_EVENT_CONNECTED;
            }
        }

        // Create a copy of the info to avoid use modifiying the info.
        memcpy(&InfoCopy, &ChargerData.Info, sizeof(CHARGER_INFO));

        if (PrevEvent != ChargerData.Info.Event)
        {
            if (CHARGER_EVENT_WAKEUPONCHARGER != ChargerData.Info.Event)
            {
                PrevEvent = ChargerData.Info.Event;
            }
            NotifyCounter = 0;      // Mark notification pending
        }
        else if (NotifyCounter >= ChargerData.NotifyPeriod)
        {
            InfoCopy.Event = CHARGER_EVENT_DATA;
            NotifyCounter = 0;      // Mark notification pending
        }
        else
        {
            // Do nothing
        }

        if (!NotifyCounter)
        {
            ChargerData.Handler(&InfoCopy);
        }

        if (ChargerData.NotifyPeriod)
        {
            NotifyCounter += CHARGER_TASK_PERIOD;
        }
    }
}

/* ========================================================================== */
/**
 * \brief      Sets and clears the Battery charging faults
 *
 * \details   Sets and clears the Battery charging faults such as Battery warning,
 *            Battery EOL. This information is passed on to the Fault Handler.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void SetBatteryChargingCycleFaults(void)
{
    ///\todo Battery Warning needs update
    if (ChargerData.Info.BatChgrCntCycle >= BATT_WARN_CHARGECYCLECOUNT && ChargerData.Info.BatChgrCntCycle < BATT_MAX_CHARGECYCLECOUNT)
    {
        /* BATT_WARN Error */
        SetErrorInfo(BATT_WARNING, BATTWARN_CHARGECYCLE_INCREMENT);
    }
    else
    {
        ClearErrorInfo(BATT_WARNING, LAST_ERROR_CAUSE);
    }

    if ((ChargerData.Info.BatChgrCntCycle >= BATT_MAX_CHARGECYCLECOUNT) ||\
            ((AM_STATUS_OK == pHandleIF->Status) && (pHandleIF->Data.ProcedureCount >= MAX_HANDLE_PROCEDURE_COUNT)))
    ///\todo 7/11/2022 - JA: replace max procedure count with procedure limit value?)
    {
        // BATT_EOL Error
        SetErrorInfo(BATT_EOL, BATT_CHARGECYCLE_EOL);
    }
    else
    {
        ClearErrorInfo(BATT_EOL, LAST_ERROR_CAUSE);
    }
}

/* ========================================================================== */
/**
 * \brief   Initialize the Error Info
 *
 * \details Initializes the Error Info to no Error for Error Instances of Charger Manager
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void ErrorInfoInit(void)
{
    ERRORLIST  List;

    for (List = BATT_COMMFAILURE; List < LAST_ERRORLIST; List++)
    {
        ErrorInfo[List].Error = LAST_ERROR_CAUSE;
        ErrorInfo[List].ErrorPublished = ERROR_NOTPUBLISHED;
    }
}

/* ========================================================================== */
/**
 * \brief   Log Battery Errors
 *
 * \details Log Battery Error and locks the Error Instance to not log Error on each call.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void LogBatteryErrors(void)
{
    ERRORLIST  List;

    for (List = BATT_COMMFAILURE; List < LAST_ERRORLIST; List++)
    {
        /* Is Error Active and Is not Logged */
        if (ErrorInfo[List].Error != LAST_ERROR_CAUSE && ErrorInfo[List].ErrorPublished == ERROR_NOTPUBLISHED)
        {
            /* Log the Error and lock the instance to stop logging on each call */
            ErrorInfo[List].ErrorPublished = FaultHandlerSetFault(ErrorInfo[List].Error, ErrorInfo[List].ErrorStatus);
        }
    }
}
/* ========================================================================== */
/**
 * \brief   Check Battery Temperature and Cell Temperatures are within valid range
 *
 * \details This API checks Battery Temperature and Cell Temperatures are within valid range.
 * \details If not in valid range Battery Temperature Error is Set
 * \details Upon battery temperature going back to Valid Temperature Error is Cleared
 *
 * \param   None
 *
 * \return  None
 *
 * ========================================================================== */
static void CheckBatteryTemperatureValidRange(void)
{
    bool BattTempOutOfRange;
    bool CellTempOutOfRange;
    float32_t CellTemp;

    do
    {
        ///\todo Battery out of range needs update
        BattTempOutOfRange = (ChargerData.Info.BatteryTemperature < BATT_TEMP_LOWVALUE || ChargerData.Info.BatteryTemperature > BATT_TEMP_HIGHVALUE);
        /* Get maximum of TS1 Cell, TS2 Cell temprature*/
        CellTemp = (ChargerData.Info.TS1Temperature > ChargerData.Info.TS2Temperature) ? ChargerData.Info.TS1Temperature : ChargerData.Info.TS2Temperature;
        /* Cell Temperature is out of range */
        CellTempOutOfRange = (CellTemp < CELLTEMP_OUTOFRANGE_LOLIMIT || CellTemp > CELLTEMP_OUTOFRANGE_HILIMIT);
        /* Check Battery Temperature Out of range or Cell Temperature Out of range */
        if (CellTempOutOfRange || BattTempOutOfRange)
        {
            SetErrorInfo(BATT_TEMPERATURE, BATTERY_TEMP_OUTOFRANGE);
            break;
        }
        /* Cell temperature valid range */
        CellTempOutOfRange = (CellTemp < CELLTEMP_VALIDRANGE_LOLIMIT || CellTemp > CELLTEMP_VALIDRANGE_HILIMIT);
        /* Cell temperature, Battery temperature are within valid range and Previously BATT TEMPERATURE Error is Set */
        if (!BattTempOutOfRange && !CellTempOutOfRange && ErrorInfo[BATT_TEMPERATURE].ErrorStatus)
        {
            /* Clear Error Info */
            ClearErrorInfo(BATT_TEMPERATURE, BATTERY_TEMP_OUTOFRANGE);
            break;
        }
        /* Once BATT Temperature Clear Signal is published clear the Error Info */
        if (!ErrorInfo[BATT_TEMPERATURE].ErrorStatus && ErrorInfo[BATT_TEMPERATURE].ErrorPublished)
        {
            ClearErrorInfo(BATT_TEMPERATURE, LAST_ERROR_CAUSE);
        }

    } while (false);
}

/* ========================================================================== */
/**
 * \brief   This API will set an Battery Error with Cause
 *
 * \details Once Battery Error with Cause is updated, LogBatteryErrors api will update the Error Status to Fault Handler
 *
 * \param   Error - Battery Error
 * \param   Cause - Error Cause
 *
 * \return  None
 *
 * ========================================================================== */
static void SetErrorInfo(ERRORLIST Error, ERROR_CAUSE Cause)
{
    ErrorInfo[Error].Error       = Cause;
    ErrorInfo[Error].ErrorStatus = SET_ERROR;
}

/* ========================================================================== */
/**
 * \brief   This API will clear an Battery Error with Cause
 *
 * \details Once Battery Error with Cause is updated, LogBatteryErrors api will update the Error Status to Fault Handler. If no cause(LAST_ERROR_CAUSE) then Fault Handler isn't called from LogBatteryErrors
 *
 * \param   Error - Battery Error
 * \param   Cause - Error Cause
 *
 * \return  None
 *
 * ========================================================================== */
static void ClearErrorInfo(ERRORLIST Error, ERROR_CAUSE Cause)
{
    ErrorInfo[Error].Error            = Cause;
    ErrorInfo[Error].ErrorStatus      = CLEAR_ERROR;
    ErrorInfo[Error].ErrorPublished   = ERROR_NOTPUBLISHED;
}


/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Initialize the Charger Manager
 *
 * \details creates charger manager task
 *
 * \param   < None >
 *
 * \return CHRG_MNGR_STATUS - charger manager init status
 * \retval CHGMGR_STATUS_OK - init done
 * \retval CHGMGR_STATUS_ERROR - error in init
 *
 * ========================================================================== */
CHRG_MNGR_STATUS L4_ChargerManagerInit(void)
{
    CHRG_MNGR_STATUS Status;    /// Operation status
    uint8_t Error;              /// OS error return temporary
    static bool    ChargerManagerInitDone = false;              /// to mark initialization, avoid multiple inits

    do
    {
        Status = CHRG_MNGR_STATUS_OK;       /// Default Status
        BREAK_IF(ChargerManagerInitDone);    /// Repeated initialization attempt, do nothing

        //BatterySetIF();

        // Initialize charger data
        ChargerData.Handler = NULL;
        ChargerData.RangeHi = BATTERY_DEFAULT_LIMIT_HI;
        ChargerData.RangeLo = BATTERY_DEFAULT_LIMIT_LO;
        ChargerData.StopRequest = false;
        ChargerData.Version = 0;
        ChargerData.Info.BatteryCurrent = 0;
        ChargerData.Info.Event = CHARGER_EVENT_LAST;
        ChargerData.NotifyPeriod = SEC_5;
        ChargerData.IsPPMaster = true;
        ChargerData.Info.BatChgrCntCycle = 0;

        /// Create Chrg Mgr Handler task
        Error = SigTaskCreate(&ChargerMgrTask,
                              NULL,
                              &ChgMgrTaskStack[0],
                              TASK_PRIORITY_L4_CHARGER_MANAGER,
                              CHGMGR_TASK_STACK,
                              "ChargerMgr");

        if (Error != OS_ERR_NONE)
        {
            // Couldn't create task, exit with error
            Status = CHRG_MNGR_STATUS_ERROR;
            Log(ERR, "L4_ChargerManagerInit: Init failed - %d", Error);
            break;
        }

        ChargerData.IsRSOCCalcAllowed = true;
        ChargerManagerInitDone = true;   /// Mark init done to avoid repeated initialization

        pBatteryInterface = BatteryGetIF(); // get the battery interface handle
        pHandleIF = HandleGetIF();

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Get current state of charger manager module
 *
 * \details Get current state of charger manager module.
 *
 * \param   < None >
 *
 * \return  CHRG_MNGR_STATE - Charger manager state
 *
 * ========================================================================== */
CHRG_MNGR_STATE  Signia_ChargerManagerGetState(void)
{
    return ChargerManagerState;
}

/* ========================================================================== */
/**
 * \brief   Get battery RSOC
 *
 * \details Get last read RSOC
 *
 * \param   pBatteryRsoc - pointer to Buffer to store read RSOC
 *
 * \return  CHRG_MNGR_STATUS - Function call status
 *
 * ========================================================================== */
CHRG_MNGR_STATUS Signia_ChargerManagerGetBattRsoc(uint8_t *pBatteryRsoc)
{
    CHRG_MNGR_STATUS Status;

    if (CHRG_MNGR_STATE_FAULT == ChargerManagerState)
    {
        Status = CHRG_MNGR_STATUS_ERROR;
    }
    else
    {
        *pBatteryRsoc = (uint8_t)ChargerData.Info.BatteryLevel;
        Status = CHRG_MNGR_STATUS_OK;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Get battery current
 *
 * \details Get last read battery current
 *
 * \param   pBatteryCurrent - pointer to Buffer to store read battery current
 *
 * \return  CHRG_MNGR_STATUS - Function call status
 *
 * ========================================================================== */
CHRG_MNGR_STATUS Signia_ChargerManagerGetBattCur(uint16_t *pBatteryCurrent)
{
    CHRG_MNGR_STATUS Status;

    if (CHRG_MNGR_STATE_FAULT == ChargerManagerState)
    {
        Status = CHRG_MNGR_STATUS_ERROR;
    }
    else
    {
        *pBatteryCurrent = ChargerData.Info.BatteryCurrent;
        Status = CHRG_MNGR_STATUS_OK;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Set desired battery level
 *
 * \details Allows user to set desired level of battery charge. The charging control
 *          logic in the module uses this to control charing operation.
 *
 * \param   LimitHigh - Desired high battery charge limit
 * \param   LimitLow - Desired low battery charge limit
 *
 * \return  CHRG_MNGR_STATUS - Function call status
 *
 * ========================================================================== */
CHRG_MNGR_STATUS Signia_ChargerManagerSetChargeLimits(uint8_t LimitHigh, uint8_t LimitLow)
{
    CHRG_MNGR_STATUS Status;

    Status = CHRG_MNGR_STATUS_INVALID_PARAM;


    if ((LimitHigh <= BATTERY_RSOC_MAX) && (LimitLow <= BATTERY_RSOC_MAX) && (LimitLow <= LimitHigh))
    {
        ChargerData.RangeHi = LimitHigh;
        ChargerData.RangeLo = LimitLow;
        Status = CHRG_MNGR_STATUS_OK;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Stop charging
 *
 * \details Allows user to stop on going charging operation
 *
 * \param   < None >
 *
 * \return  CHRG_MNGR_STATUS - Function call status
 *
 * ========================================================================== */
CHRG_MNGR_STATUS Signia_ChargerManagerSleep(void)
{
    ChargerData.StopRequest = true;
    return CHRG_MNGR_STATUS_OK;
}

/* ========================================================================== */
/**
 * \brief   Get the Battery Charging Count Cycle
 *
 * \details Returns the battery Charge Count Cycle from the local ChargerData structure.
 *
 * \param   < None >
 *
 * \return  uint16_t  - return ChargerData.Info.BatChgrCntCycle value
 *
 * ========================================================================== */
uint16_t Signia_ChargerManagerGetChgrCntCycle(void)
{
    return ChargerData.Info.BatChgrCntCycle;
}

/* ========================================================================== */
/**
 * \brief   Register charger event handler
 *
 * \details Allows user to register user function to handle charger events upon
 *          change in charger/battery information or periodically receive notifications
 *
 * \param   pChargerHandler - Charger events handler function. NULL disables notifications
 * \param   Period - Event notification period. Value is clipped if out of range.
 *
 * \return  CHRG_MNGR_STATUS - Function call status
 *
 * ========================================================================== */
CHRG_MNGR_STATUS Signia_ChargerManagerRegEventHandler(CHARGER_HANDLER pChargerHandler, uint16_t Period)
{
    ChargerData.Handler = pChargerHandler;

    // Limit user input value to min-max limits
    ChargerData.NotifyPeriod = (Period > CHARGER_MAX_NOTIFY_PERIOD) ? CHARGER_MAX_NOTIFY_PERIOD : (Period < CHARGER_TASK_PERIOD) ? CHARGER_TASK_PERIOD : Period;

    return CHRG_MNGR_STATUS_OK;
}

/* ========================================================================== */
/**
 * \brief   set or clear the flag IsRSOCCalcAllowed
 *
 * \details Allows user to set or clear the flag IsRSOCCalcAllowed. if the
 *          flag set allows the calculated RSOC to be updated else the previous value is maintained.
 *
 * \param   state - boolean value indicating to set or clear the flag
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_ChargerManagerRSOCCalAllowed(bool state)
{
    ChargerData.IsRSOCCalcAllowed = state;
}

/* ========================================================================== */
/**
 * \brief   Calculate RSOC level from Battery voltage
 *
 * \details Battery RSOC level needs to be calculated based on the Measured
 *          Battery Voltage
 *          For now function retuns the measured Battery level from BQ chip
 *
 * \param   voltage - Battery Voltage
 *
 * \return  float32_t - Calculated RSOC level
 *
 * ========================================================================== */
float32_t BatteryCalculateRSOC(uint16_t voltage)
{
    static uint16_t	PrevBattV;
    static uint16_t FilteredBattV;
    float32_t 		CalcRSOC;
    float32_t 		IIRFilterCoeff;

    // during handle startup update PrevbattV with current voltage.
    if (0u == PrevBattV)
    {
        PrevBattV = voltage;
        FilteredBattV = voltage;
    }

    do
    {
        // flag IsRSOCCalcAllowed should be set to false to stop the RSOC calculation and Batterylevel maintained at previous level.
        if (!ChargerData.IsRSOCCalcAllowed)
        {
            CalcRSOC = ChargerData.Info.BatteryLevel;
            break;
        }
        // Set IIR filter coefficient to 1 - to disssable filter giving filteroutput = Input
        /// \todo 02/01/2022 BS - Need api to check motor running or stopped, for now added dummy api
        // Filter should start only if Voltage difference > 5mv or motor's are running
        IIRFilterCoeff = ((abs(PrevBattV - voltage) > MAX_VOLTAGE_DELTA) || Signia_AnyMotorRunning()) ? BATT_IIR_COEFF : 1u;

        FilteredBattV = (uint16_t)((voltage * IIRFilterCoeff) + (FilteredBattV * (1 - IIRFilterCoeff)));

        CalcRSOC = Interpolate(FilteredBattV, BatteryDesignParams[ChargerData.Info.BatteryType].pRSOCLUT);

        // allow only decreasing RSOC.
        if ((CalcRSOC > ChargerData.Info.BatteryLevel) && (ChargerData.Info.BatteryLevel > 0) &&
            (CHRG_MNGR_STATE_CONNECTED != ChargerManagerState) && (CHRG_MNGR_STATE_CHARGING != ChargerManagerState))
        {
            CalcRSOC = ChargerData.Info.BatteryLevel;
        }
    } while (false);

    PrevBattV = voltage;
    return CalcRSOC;
}

/* ========================================================================== */
/**
 * \brief   Access to ChargerData Info
 *
 * \details Returs the pointer to the Battery parameters
 *
 * \param   < None >
 *
 * \return  CHARGER_INFO
 *
 * ========================================================================== */
CHARGER_INFO* Signia_ChargerManagerGetChargerInfo(void)
{
    return &ChargerData.Info;
}

/* ========================================================================== */
/**
 * \brief   Set flag to inform ChargerManager to publish the Wake from sleep signal
 *          This flag is to allow the publish only once after wakefromsleep
 *
 * \param   state - true: Request to publish the WakefromSleep signal
 *                - false: Do not publish the WakefromSleep signal
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_ChargerManagerSetWakupstate(bool state)
{
    PublishWakefromsleep = state;
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

