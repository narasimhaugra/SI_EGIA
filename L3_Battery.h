#ifndef L3_BATTERY_H
#define L3_BATTERY_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_Battery
 * \{
 * \brief   Public interface for the Battery control and reporting routines.
 *
 * \details This module defines symbolic constants as well as function prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_Battery.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define BATTERY_SLAVE_ADDRESS            (0x0Bu)    ///< I2C slave address
#define BATTERY_MAX_BLOCK_DATA_SIZE      (64u)      ///< Max size of a block data
#define BATTERY_TXFR_TIMEOUT             (200u)     ///< I2C busy retry timeout
#define MFGACCESS_FETCNTRL               (0x0022u)  ///< Manufacuturer Access FET Control
#define MFGACCESS_DSGFET                 (0x0020u)  ///< Manufacuturer Access Discharging FET Control

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
///< BATTERY STATUS
typedef enum
{
    BATTERY_STATUS_OK,                 ///< No Error
    BATTERY_STATUS_TIMEOUT,            ///< Timeout
    BATTERY_STATUS_COMM_FAILURE,       ///< Communication Failure
    BATTERY_STATUS_BUSY,               ///< Busy
    BATTERY_STATUS_INVALID_PARAM,      ///< Invalid Parameter
    BATTERY_STATUS_ERROR,              ///< Error
    BATTERY_STATUS_LAST                ///< Last Enum
} BATTERY_STATUS;

/* Battery Command Index List */
///< Enumeration
typedef enum {
    BAT_MANUFACTURING_ACCESS_BYTE,     ///< Access byte
    BAT_CMD_MODE,                      ///< Mode
    BAT_CMD_TEMPERATURE,               ///< Temperature
    BAT_CMD_VOLTAGE,                   ///< Voltage
    BAT_CMD_CURRENT,                   ///< Current
    BAT_CMD_RELATIVE_STATE_OF_CHARGE,  ///< RSOC
    BAT_CMD_REMAINING_CAPACITY,        ///< Remaining Capacity
    BAT_CMD_FULL_CHARGE_CAPACITY,      ///< Full Charge Capacity
    BAT_MFG_CMD_RESET_BQ_CHIP,         ///< RESET BQ Chip
    BAT_CMD_CHARGING_CURRENT,          ///< Charging Current
    BAT_CMD_STATUS,                    ///< Battery Status
    BAT_CMD_CHARGE_CYCLE_COUNT,        ///< Charge Cycle Count
    BAT_CMD_DESIGN_CHARGE_CAPACITY,    ///< Design Charge Capacity
    BAT_CMD_MANUF_NAME,                ///< Manufacturer Name
    BAT_CMD_DEVICE_NAME,               ///< Device Name
    BAT_CMD_DEVICE_CHEMISTRY,          ///< Device Chemistry
    BAT_CMD_CELL3_VOLTAGE,             ///< Cell 3 Voltage
    BAT_CMD_CELL2_VOLTAGE,             ///< Cell 2 Voltage
    BAT_CMD_CELL1_VOLTAGE,             ///< Cell 1 Voltage
    BAT_CMD_CELL0_VOLTAGE,             ///< Cell 0 Voltage
    BAT_CMD_SAFETY_STATUS,             ///< Safety Status
    BAT_CMD_PF_STATUS,                 ///< Permanent fail status
    BAT_CMD_OPERATION_STATUS,          ///< Operation Status
    BAT_CMD_CHARGING_STATUS,           ///< Charging Status
    BAT_CMD_GAUGING_STATUS,            ///< Gauging Status
    BAT_CMD_MANUF_STATUS,              ///< Manufacturing Status
    BAT_CMD_AFE_REGISTERS,             ///< AFE Registers
    BAT_CMD_LTIME_DATA_BLK_1,          ///< Life Time data block 1
    BAT_CMD_LTIME_DATA_BLK_2,          ///< Life Time data block 2
    BAT_CMD_LTIME_DATA_BLK_3,          ///< Life Time data block 3
    BAT_CMD_VOLTAGES,                  ///< Voltages
    BAT_CMD_TEMPERATURES,              ///< Temperatures
    BAT_CMD_IT_STATUS_1,               ///< IT Status 1
    BAT_CMD_IT_STATUS_2,               ///< IT Status 2
    BAT_CMD_COUNT                      ///< Number of Battery Commands
} BATTERY_COMMANDS;

/* Battery DataFeild Parameter Index List */
///< Enumeration
typedef enum
{
    DSG_CURRENT_THD,                   ///< DSG Current Threshold - Class: 249, Offset: 0
    CHG_CURRENT_THD,                   ///< CHG Current Threshold - Class: 249, Offset: 2
    QUIT_CURRENT,                      ///< Quit Current Threshold - Class: 249, Offset: 4
    CYCLE_COUNT_PERC,                  ///< Cycle Count Percentage - Class: 489, Offset: 18
    ENABLED_PF_0_15,                   ///< Enabaled PF 0 to 15 bits - Class: 197, Offset: 0
    SHUTDOWN_TIME,                     ///< Shutdown Time - Class: 230, Offset: 2
    PRECHARGING_CURRENT,               ///< Pre-Charging current - Class: 148, Offset: 0
    MIN_START_BALANCE_DELTA,           ///< Cell Balancing congi, Min Start Balance Delta - Class: 168, Offset: 4
    CURRENT_DEADBAND,                  ///< Current Deadband - Class: 103, Offset: 0
    VALID_VOLTAGE_UPDATE,              ///< Valid Update Voltage - Class: 228, Offset: 0
    SBS_DATA_CONFIG_0_15,              ///< Setting Configuration - Class: 201, Offset: 9
    CHARGING_CONFIG,                   ///< Setting Configuration - Class: 201, Offset: 3
    CLEAR_VOLTAGE_THD,                 ///< Clear voltage threshold - Class: 578, Offset: 2
    DF_COUNT                           ///< Number of DF Parameters
} BATTERY_DF_NAMES;

/* Battery Cell Number Index */
///< Enumeration
typedef enum {
  CELL_0,                              ///< Cell 0
  CELL_1,                              ///< Cell 1
  CELL_2,                              ///< Cell 2
  CELL_3,                              ///< Cell 3
  CELL_COUNT                           ///< Number of cells defined
} BATTERY_CELL_NUMBER;

/* Battery Block Number Index */
///< Enumeration
typedef enum {
  BLOCK_0,                             ///< Block 0
  BLOCK_1,                             ///< Block 1
  BLOCK_2,                             ///< Block 2
  BLOCK_3,                             ///< Block 3
  BLOCK_COUNT                          ///< Number of blocks defined
} BATTERY_BLOCK_NUMBER;

/* IT Status Number Index */
///< Enumeration
typedef enum {
  STATUS_0,                            ///< Status 0
  STATUS_1,                            ///< Status 1
  STATUS_2,                            ///< Status 2
  STATUS_COUNT                         ///< Number of status blocks defined
} IT_STATUS_NUMBER;

/* Status Number Index */
///< Enumeration
typedef enum {
  CMD_BATTERY_STATUS,                  ///< Battery Status Register
  CMD_SAFETY_STATUS,                   ///< Safety Status Register
  CMD_PF_STATUS,                       ///< Permanent Fail Status Register
  CMD_OPERATION_STATUS,                ///< Operation Status Register
  CMD_CHARGING_STATUS,                 ///< Charging Status Register
  CMD_GAUGING_STATUS,                  ///< Gauging Status Register
  CMD_MANUF_STATUS,                    ///< Manufacture Status Register
  CMD_STATUS_COUNT                     ///< Number of status commands defined
} STATUS_NUMBER;

/* Structure for Battery DF parameters*/
typedef struct
{
    uint16_t SubClsId;                  ///< Sub class ID
    uint8_t Offset;                     ///< Offset
    uint8_t Size;                       ///< Size of the Parameter in bytes
} BATTERY_DF_PARAM;

typedef struct{                 /// Test Hook Structure
  BATTERY_DF_PARAM *DFparameter;  ///<  Data Flash parameters
  uint8_t *pValue;                ///< pointer to Flash Data Value
}BATT_DF_TM_DATA;


/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern bool IsBatteryInitialized(void);

extern uint8_t L3_GetBattCmdByItem(uint8_t cmdItem);
extern uint8_t L3_GetBattCmdByItemSize(uint8_t cmdItem);

extern BATTERY_STATUS L3_BatteryInit(void);
extern BATTERY_STATUS L3_BatteryShutdown(void);
extern BATTERY_STATUS L3_BatteryResetBQChip(void);
extern BATTERY_STATUS L3_BatteryGetMode(uint16_t *pMode);
extern BATTERY_STATUS L3_BatteryGetDeviceName(uint8_t *pNbytes, uint8_t *pData);
extern BATTERY_STATUS L3_BatteryGetDeviceChemistry(uint32_t *pDevChem);
extern BATTERY_STATUS L3_BatteryGetManufacturerName(uint8_t *pNbytes, uint8_t *pData);
extern BATTERY_STATUS L3_BatteryGetDesignChargeCapacity(uint16_t *pDesChgCap);
extern BATTERY_STATUS L3_BatteryGetFullChargeCapacity(uint16_t *pFullChgCap);
extern BATTERY_STATUS L3_BatterySetFullChargeCapacity(uint16_t FullChgCap);
extern BATTERY_STATUS L3_BatteryGetRemainCapacity(uint16_t *pRemCap);
extern BATTERY_STATUS L3_BatterySetRemainCapacity(uint16_t RemCap);
extern BATTERY_STATUS L3_BatteryGetVoltage(uint16_t *pVoltage);
extern BATTERY_STATUS L3_BatteryGetCellVoltage(BATTERY_CELL_NUMBER CellNumber, uint16_t *pCellVoltage);
extern BATTERY_STATUS L3_BatteryGetVoltages(uint8_t *pNbytes, uint8_t *pData);
extern BATTERY_STATUS L3_BatteryGetTemperature(uint16_t *pTemperature);
extern BATTERY_STATUS L3_BatteryGetTemperatures(uint8_t *pNbytes, uint8_t *pData);
extern BATTERY_STATUS L3_BatteryGetCurrent(int16_t *pCurrent);
extern BATTERY_STATUS L3_BatteryGetChargingCurrent(uint16_t *pChgrCurrent);
extern BATTERY_STATUS L3_BatteryGetRSOC(uint16_t *pRsoc);
extern BATTERY_STATUS L3_BatteryGetChgrCntCycle(uint16_t *pChgrCntCycle);
extern BATTERY_STATUS L3_BatteryGetLifeTimeDataBlock(uint16_t BlockNumber, uint8_t *pNbytes, uint8_t *pData);
extern BATTERY_STATUS L3_BatteryGetAFERegisters(uint8_t *pNbytes, uint8_t *pData);
extern BATTERY_STATUS L3_BatteryGetITStatus(uint32_t StatusNum, uint8_t *pNbytes, uint8_t *pData);
extern BATTERY_STATUS L3_BatteryGetStatus(uint32_t StatusNum, uint8_t *pNbytes, uint8_t *pData);
extern BATTERY_STATUS L3_BatteryGetChemicalID(uint16_t *pData);
extern BATTERY_STATUS L3_BatteryGetDataFlash(BATTERY_DF_PARAM *pDFinfo, uint8_t *pData);
extern BATTERY_STATUS L3_BatterySetDataFlash(BATTERY_DF_PARAM *pDFinfo, uint8_t *pData);
extern BATTERY_STATUS L3_BatteryPECEnable(void);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_BATTERY_H */
