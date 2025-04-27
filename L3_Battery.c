#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_Battery
 * \{
 * \brief   Layer 3 support functions for the Battery BQ Chip (bq30z554-R1).
 *
 * \details  These functions allow layers L3 and above to do the following:
 *                    - Initialize the Battery and SMBus Interface
 *                    - Configure the Battery and SMBus Interface
 *                    - Reset the Battery IC
 *                    - Disable Discharge FETS
 *                    - Set Battery Modes
 *                    - Overwrite the full charge capacity with a
 *                      specified value
 *                    - Overwrite the remaining capacity of the battery
 *                      with a specified value
 *                    - Enable the SMBus for Battery communications
 *                    - Send a Battery Command via the SMBus.
 *                    - Obtain the response to a Battery Command
 *                    - Retrieve Battery data for a given Battery Command
 *                      via the SMBus interface.
 *
 * \sa      bq30z554-R1 Tech Ref.pdf
 * \sa      bq30z554 sbdat110.pdf
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_Battery.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include <Common.h>
#include "L3_GpioCtrl.h"
#include "L3_Battery.h"
#include "L3_SMBus.h"
#include "L3_I2c.h"
#include "L2_Spi.h"
#include "Logger.h"
#include "FaultHandler.h"
#include "Common.h"
#include "TestManager.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_BATTERY) ///< Identifier for log entries
#define BAT_SHUTDOWN_CMD_OPDATA     (0x0010u)           ///< Opdata for Battery shutdown mode cmd

#define BAT_DF_ROWDATA_SIZE         (32u)               ///< Size of Battery DataFlash Row in Bytes (refer bq30z554-R1 Tech Ref.pdf)
#define BAT_CMD_MFG_INFO            (0x2Fu)             ///< Manufacturing Info cmd
#define BAT_CMD_MFGACCESS_CHEMID    (0X0006u)           ///< Manufacturer Access Chemical ID data
#define BAT_CMD_MFGDATA             (0x23u)             ///< Manufacturer Data
#define BAT_OPDATA_OFFSET           (0X100u)            ///< Offset for Manufacturing info data
#define BQ_CHIP_DATA_WRITE_TIME     (200u)              ///< Time taken by bq chip to write data to its flash

#define SMBUS_SBSCONFIG_OFFSET          8u              ///< SBS configuration Offset
#define SMBUS_SBSCONFIG_SIZE            1u              ///< SBS configuration Size
#define SMBUS_SBSCONFIG_SUBCLSID        201u            ///< SBS configuration subclass id

#define SMBUS_HPE_MASK                  0X04u           ///< Bit Mask for PEC bit
#define BQ_CHEMID_BLOCKSIZE             3u              ///< BLock size to read chemical id
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
///< Defines the command and number bytes for SMBus transfers
typedef struct
{
    uint8_t Cmd;        ///< Cmd (byte 0-255)
    uint8_t OpSize;     ///< Operation data size
} BATT_XFER_STATIC;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
static const BATT_XFER_STATIC IdxToCmd[BAT_CMD_COUNT] =
{
    // Command, Length
    { 0x00, 2 },  // MANUFACTURING ACCESS
    { 0x03, 2 },  // MODE
    { 0x08, 2 },  // TEMPERATURE
    { 0x09, 2 },  // VOLTAGE
    { 0x0A, 2 },  // CURRENT
    { 0x0D, 2 },  // RELATIVE_STATE_OF_CHARGE
    { 0x0F, 2 },  // REMAINING_CAPACITY
    { 0x10, 2 },  // FULL_CHARGE_CAPACITY
    { 0x12, 2 },  // RESET_BQ_CHIP
    { 0x14, 2 },  // CHARGING_CURRENT
    { 0x16, 2 },  // BATTERY STATUS
    { 0x17, 2 },  // CHARGE_CYCLE_COUNT
    { 0x18, 2 },  // DESIGN_CHARGE_CAPACITY
    { 0x20, 21 }, // MANUF_NAME
    { 0x21, 21 }, // DEVICE_NAME
    { 0x22, 4 },  // DEVICE_CHEMISTRY
    { 0x3C, 2 },  // CELL3_VOLTAGE
    { 0x3D, 2 },  // CELL2_VOLTAGE
    { 0x3E, 2 },  // CELL1_VOLTAGE
    { 0x3F, 2 },  // CELL0_VOLTAGE
    { 0x51, 4 },  // SAFETY_STATUS
    { 0x53, 4 },  // PF_STATUS
    { 0x54, 4 },  // OPERATION_STATUS
    { 0x55, 3 },  // CHARGING_STATUS
    { 0x56, 2 },  // GAUGING_STATUS
    { 0x57, 2 },  // MANUF_STATUS
    { 0x58, 11 }, // AFE_REGISTERS
    { 0x60, 32 }, // LTIME_DATA_BLK_1
    { 0x61, 27 }, // LTIME_DATA_BLK_2
    { 0x62, 14 }, // LTIME_DATA_BLK_3
    { 0x71, 12 }, // VOLTAGES
    { 0x72, 10 }, // TEMPERATURES
    { 0x73, 30 }, // IT_STATUS_1
    { 0x74, 30 }, // IT_STATUS_1
};

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static bool battIsInitialized = false;            ///< Flag if battery is initialized

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Battery Initialization Status
 *
 * \details This function returns the status of battery initialization
 *
 * \param   < None >
 *
 * \return  bool - battery initialization status
 * \retval      true  - If the battery has been initialized
 * \retval      false - If the battery has not been initialized
 *
 * ========================================================================== */
bool IsBatteryInitialized(void)
{
    return battIsInitialized;
}

/* ========================================================================== */
/**
 * \brief   Battery command number by index
 *
 * \details This function returns the battery command by index
 *
 * \param   cmdItem - get SMBus command by item
 *
 * \return  uint8_t - command number
 * \retval      0 - 255
 *
 * ========================================================================== */
uint8_t L3_GetBattCmdByItem(uint8_t cmdItem)
{
    return (BAT_CMD_COUNT <= cmdItem ? BAT_CMD_COUNT : IdxToCmd[cmdItem].Cmd);
}

/* ========================================================================== */
/**
 * \brief   Battery command size by index
 *
 * \details This function returns the battery command size by index
 *
 * \param   cmdItem - get SMBus size by item
 *
 * \return  uint8_t - operation size
 * \retval      0 - 255
 *
 * ========================================================================== */
uint8_t L3_GetBattCmdByItemSize(uint8_t cmdItem)
{
    return (BAT_CMD_COUNT <= cmdItem ? BAT_CMD_COUNT : IdxToCmd[cmdItem].OpSize);
}

/* ========================================================================== */
/**
 * \brief   Battery Initialization
 *
 * \details This function initializes the battery by disabling the analog switch
 *          to the hardware (BQ chip) until needed to minimize capacitance.
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryInit(void)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusInit(BATTERY_SLAVE_ADDRESS, BATTERY_TXFR_TIMEOUT);

    if (SMBUS_NO_ERROR == opStatus)
    {
        //L3_BatteryPECEnable();
        battIsInitialized = true;
        Log(REQ, "L3 Battery has been initialized successfully.");
        eStatusReturn = BATTERY_STATUS_OK;
    }
    else
    {
        battIsInitialized = false;
        Log(ERR, "Error in L3 Battery during initialization.");
        eStatusReturn = BATTERY_STATUS_ERROR;
    }

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   Shutdown the BQ Chip
 *
 * \details This function sends a command to Shutdown the battery BQ chip. The
 *          device can be sent to SHUTDOWN mode before shipping to reduce power
 *          consumption to a minimum. The device will wake up when a voltage is
 *          applied to PACK.
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryShutdown(void)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */
    uint16_t       opData;         /* Operation data */

    opData = BAT_SHUTDOWN_CMD_OPDATA;
    opStatus = L3_SMBusWriteWord(BATTERY_SLAVE_ADDRESS,
                                    BAT_MANUFACTURING_ACCESS_BYTE,
                                    opData);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    OSTimeDly(SEC_5);   /* Allow time for the comms to occur */

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief   Reset the BQ Chip
 *
 * \details This function sends a command to Reset the battery BQ chip.
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryResetBQChip(void)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */
    uint16_t       opData;         /* Operation data */

    opData = IdxToCmd[BAT_MFG_CMD_RESET_BQ_CHIP].Cmd;
    opStatus = L3_SMBusWriteWord(BATTERY_SLAVE_ADDRESS,
                                    IdxToCmd[BAT_MANUFACTURING_ACCESS_BYTE].Cmd,
                                    opData);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    OSTimeDly(SEC_2);   /* Allow time for the comms to occur */

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function gets the battery’s capabilities, modes, and
 *           configured minor condition reporting which may require attention by
 *           the battery manager.
 *
 * \note     Value returned is a bit enumerated value as shown in table below.
 *   Bit     Field                         Value
 *   0       INTERNAL_CHARGE_CONTROLLER    0 - Not Supported
 *                                         1 - Supported
 *   1       PRIMARY_BATTERY_SUPPORT       0 - Not Supported
 *                                         1 - Primary or Secondary Supported
 *   2 - 6   Undefined
 *   7       CONDITION_FLAG                0 - Battery OK
 *                                         1 - Conditioning Cycle Requested
 *   8       CHARGE_CONTROLLER_ENABLED     0 - Internal Charge Control Disabled (default)
 *                                         1 - Internal Charge Control Enabled
 *   9       PRIMARY_BATTERY               0 - Battery operating in its secondary role (default)
 *                                         1 - Battery operating in its primary role
 *   10-12   Undefined
 *   13      ALARM_MODE                    0 - Enable AlarmWarning broadcasts to
 *                                             Host and Smart Battery Charger (default)
 *                                         1 - Disable broadcasts of ChargingVoltage
 *                                             and ChargingCurrent to Smart Battery
 *                                             Charger
 *   14      CHARGE_MODE                   0 - Enable ChargingVoltage and
 *                                             ChargingCurrent broadcasts to Smart
 *                                             Battery Charger (default)
 *                                         1 - Disable broadcasts of ChargingVoltage
 *                                             and ChargingCurrent to Smart Battery
 *                                             Charger
 *   15      CAPACITY_MODE                 0 - Report in mA or mAh (default)
 *                                         1 - Report in 10mW or 10mWh
 *
 * \param   pMode - pointer to battery operational mode
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetMode(uint16_t *pMode)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadWord(BATTERY_SLAVE_ADDRESS,
                                IdxToCmd[BAT_CMD_MODE].Cmd,
                               (uint8_t *)pMode);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns a character string that contains the battery's
 *           name. For example, a DeviceName() of "MBC101" would indicate that
 *           the battery is a model MBC101.
 *
 * \param   pNbytes - pointer to number of bytes in block data
 * \param   pData   - pointer to block data
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetDeviceName(uint8_t *pNbytes, uint8_t *pData)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadBlock(BATTERY_SLAVE_ADDRESS,
                                 IdxToCmd[BAT_CMD_DEVICE_NAME].Cmd,
                                 IdxToCmd[BAT_CMD_DEVICE_NAME].OpSize,
                                 (uint8_t *)pData);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    *pNbytes = ((BATTERY_STATUS_OK == eStatusReturn)? IdxToCmd[BAT_CMD_DEVICE_NAME].OpSize : 0);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns a character string that contains the battery's
 *           chemistry. For example, if the DeviceChemistry() function returns
 *           "NiMH," the battery pack would contain nickel metal hydride cells.
 *
 * \param   pDevChem - pointer to Design Charge Capacity
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetDeviceChemistry(uint32_t *pDevChem)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusRead32(BATTERY_SLAVE_ADDRESS,
                              IdxToCmd[BAT_CMD_DEVICE_CHEMISTRY].Cmd,
                              (uint8_t *)pDevChem);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns a character array containing the battery's
 *           manufacturer's name. For example, "MyBattCo" would identify the
 *           Smart Battery's manufacturer as MyBattCo.
 *
 * \param   pNbytes - pointer to number of bytes in block data
 * \param   pData   - pointer to block data
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetManufacturerName(uint8_t *pNbytes, uint8_t *pData)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadBlock(BATTERY_SLAVE_ADDRESS,
                                 IdxToCmd[BAT_CMD_MANUF_NAME].Cmd,
                                 IdxToCmd[BAT_CMD_MANUF_NAME].OpSize,
                                 pData);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    *pNbytes = ((BATTERY_STATUS_OK == eStatusReturn)? IdxToCmd[BAT_CMD_MANUF_NAME].OpSize : 0);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns the theoretical capacity of a new pack.
 *           The value is expressed in either current (mAh at a C/5 discharge rate)
 *           or power (10mWh at a P/5 discharge rate) depending on the setting
 *           of the BatteryMode()'s CAPACITY_MODE bit.
 *
 * \param   pDesChgCap - pointer to Design Charge Capacity
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetDesignChargeCapacity(uint16_t *pDesChgCap)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadWord(BATTERY_SLAVE_ADDRESS,
                                IdxToCmd[BAT_CMD_DESIGN_CHARGE_CAPACITY].Cmd,
                               (uint8_t *)pDesChgCap);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns the predicted pack capacity when it is fully
 *           charged. The FullChargeCapacity() value is expressed in either
 *           current (mAh at a C/5 discharge rate) or
 *           power (10mWh at a P/5 discharge rate) depending on the setting of
 *           the BatteryMode()'s CAPACITY_MODE bit.
 *
 * \param   pFullChgCap - pointer to Full Charge Capacity
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetFullChargeCapacity(uint16_t *pFullChgCap)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadWord(BATTERY_SLAVE_ADDRESS,
                                IdxToCmd[BAT_CMD_FULL_CHARGE_CAPACITY].Cmd,
                               (uint8_t *)pFullChgCap);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns the predicted pack capacity when it is fully
 *           charged.
 *
 * \param   FullChgCap - Full charge capacity
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatterySetFullChargeCapacity(uint16_t FullChgCap)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusWriteWord(BATTERY_SLAVE_ADDRESS,
                                 IdxToCmd[BAT_CMD_FULL_CHARGE_CAPACITY].Cmd,
                                 FullChgCap);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns the predicted remaining battery capacity.
 *           The RemainingCapacity() capacity value is expressed in either
 *           current (mAh at a C/5 discharge rate) or
 *           power (10mWh at a P/5 discharge rate) depending on the setting of
 *           the BatteryMode()'s CAPACITY_MODE bit.
 *
 * \param   pRemCap - pointer to remaining Charge Capacity
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetRemainCapacity(uint16_t *pRemCap)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadWord(BATTERY_SLAVE_ADDRESS,
                                IdxToCmd[BAT_CMD_REMAINING_CAPACITY].Cmd,
                               (uint8_t *)pRemCap);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function sets the predicted remaining battery capacity.
 *
 * \param   RemCap - Remaining charge capacity
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatterySetRemainCapacity(uint16_t RemCap)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusWriteWord(BATTERY_SLAVE_ADDRESS,
                                 IdxToCmd[BAT_CMD_REMAINING_CAPACITY].Cmd,
                                 RemCap);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns the cell-pack voltage (mV).
 *
 * \param   pVoltage - pointer to battery voltage
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetVoltage(uint16_t *pVoltage)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadWord(BATTERY_SLAVE_ADDRESS,
                                IdxToCmd[BAT_CMD_VOLTAGE].Cmd,
                                (uint8_t *)pVoltage);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns the cell voltage (mV).
 *
 * \param   CellNumber    - battery cell number (0 <= N <=3)
 * \param   pCellVoltage - pointer to battery cell voltage
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetCellVoltage(BATTERY_CELL_NUMBER CellNumber, uint16_t *pCellVoltage)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */
    uint8_t        cellNumCmd;     /* Command for indicated cell number */

    /* Initialize the variables */
    cellNumCmd = 0u;

    eStatusReturn = BATTERY_STATUS_LAST; /* Last error status */
    switch (CellNumber)
    {
        case CELL_0:
            cellNumCmd = IdxToCmd[BAT_CMD_CELL0_VOLTAGE].Cmd;
            break;

        case CELL_1:
            cellNumCmd = IdxToCmd[BAT_CMD_CELL1_VOLTAGE].Cmd;
            break;

        case CELL_2:
            cellNumCmd = IdxToCmd[BAT_CMD_CELL2_VOLTAGE].Cmd;
            break;

        case CELL_3:
            cellNumCmd = IdxToCmd[BAT_CMD_CELL3_VOLTAGE].Cmd;
            break;

        default:
            eStatusReturn = BATTERY_STATUS_INVALID_PARAM;
            break;
    }

    if (BATTERY_STATUS_INVALID_PARAM != eStatusReturn)
    {
        opStatus = L3_SMBusReadWord(BATTERY_SLAVE_ADDRESS,
                                    cellNumCmd,
                                    (uint8_t *)pCellVoltage);

        eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);
    }
    else
    {
        Log(DBG, "Invalid battery cell number voltage requested: %u", CellNumber);
    }

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns 12 bytes of voltage data values on in the
 *           following format:  aaAAbbBBccCCddDDeeEEffFF where:
 *               AAaa: Cell Voltage 0
 *               BBbb: Cell Voltage 1
 *               CCcc: Cell Voltage 2
 *               DDdd: Cell Voltage 3
 *               EEee: BAT Voltage
 *               FFff: PACK Voltage
 *
 * \param   pNbytes - pointer to number of bytes in block data
 * \param   pData   - pointer to block data
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetVoltages(uint8_t *pNbytes, uint8_t *pData)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadBlock(BATTERY_SLAVE_ADDRESS,
                                 IdxToCmd[BAT_CMD_VOLTAGES].Cmd,
                                 IdxToCmd[BAT_CMD_VOLTAGES].OpSize,
                                 pData);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    *pNbytes = ((BATTERY_STATUS_OK == eStatusReturn)? IdxToCmd[BAT_CMD_VOLTAGES].OpSize : 0);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns the cell-pack's internal temperature (°K).
 *           The actual operational temperature range will be defined at a pack
 *           level by a particular manufacturer. Typically it will be in the
 *           range of -20°C to +75°C.
 *
 * \param   pTemperature - pointer to battery temperature
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetTemperature(uint16_t *pTemperature)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadWord(BATTERY_SLAVE_ADDRESS,
                                IdxToCmd[BAT_CMD_TEMPERATURE].Cmd,
                                (uint8_t *)pTemperature);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns 14 bytes of temperature data values in the
 *           following format: aaAAbbBBccCCddDDeeEEffFF where:
 *               AAaa: Int Temperature
 *               BBbb: TS1 Temperature
 *               CCcc: TS2 Temperature
 *               DDdd: TS3 Temperature
 *               EEee: TS4 Temperature
 *               FFff: Cell Temperature
 *               GGgg: FET Temperature
 *
 * \param   pNbytes - pointer to number of bytes in block data
 * \param   pData   - pointer to block data
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetTemperatures(uint8_t *pNbytes, uint8_t *pData)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */
    uint8_t TempData[15];

    opStatus = L3_SMBusReadBlock(BATTERY_SLAVE_ADDRESS,
                                 IdxToCmd[BAT_CMD_TEMPERATURES].Cmd,
                                 IdxToCmd[BAT_CMD_TEMPERATURES].OpSize+1,
                                 TempData);
    memcpy(pData,&TempData[1],IdxToCmd[BAT_CMD_TEMPERATURES].OpSize);
    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    *pNbytes = ((BATTERY_STATUS_OK == eStatusReturn)? IdxToCmd[BAT_CMD_TEMPERATURES].OpSize : 0);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief   This function returns the current being supplied (or accepted)
 *          through the battery's terminals (mA).
 *
 * \note    The Current() function provides a snapshot for the power
 *          management system of the current flowing into or out of the
 *          battery. This information will be of particular use in power
 *          management systems because they can characterize individual
 *          devices and "tune" their operation to actual system power
 *          behavior.
 *          Output - signed int -- charge/discharge rate in mA increments
 *                 positive for charge, negative for discharge
 *          Units: mA
 *          Range: 0 to 32,767 mA for charge or
 *                 0 to -32,768 mA for discharge
 *          Granularity: 0.2% of the DesignCapacity() or better
 *          Accuracy: ±1.0% of the DesignCapacity()
 *
 * \param   pCurrent - pointer to battery current
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetCurrent(int16_t *pCurrent)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadWord(BATTERY_SLAVE_ADDRESS,
                                IdxToCmd[BAT_CMD_CURRENT].Cmd,
                                (uint8_t *)pCurrent);  /// \\todo 02-16-2022 KA : suspicious cast

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus) ? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function represents the maximum current which may be provided
 *           by the Smart Battery Charger to permit the Smart Battery to reach a
 *           Fully Charged state.
 *
 * \param   pChgrCurrent - pointer to battery charging current
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetChargingCurrent(uint16_t *pChgrCurrent)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadWord(BATTERY_SLAVE_ADDRESS,
                                IdxToCmd[BAT_CMD_CHARGING_CURRENT].Cmd,
                                (uint8_t *)pChgrCurrent);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns the predicted remaining battery capacity
 *           expressed as a percentage of FullChargeCapacity() (%).
 *
 * \param   pRsoc - pointer to battery relative state of change
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetRSOC(uint16_t *pRsoc)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadWord(BATTERY_SLAVE_ADDRESS,
                                IdxToCmd[BAT_CMD_RELATIVE_STATE_OF_CHARGE].Cmd,
                                (uint8_t *)pRsoc);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns the number of cycles the battery has
 *           experienced. A cycle is defined as: An amount of discharge
 *           approximately equal to the value of DesignCapacity.
 *
 * \param   pChgrCntCycle - pointer to Charger Count Cycle
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetChgrCntCycle(uint16_t *pChgrCntCycle)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadWord(BATTERY_SLAVE_ADDRESS,
                                IdxToCmd[BAT_CMD_CHARGE_CYCLE_COUNT].Cmd,
                                (uint8_t *)pChgrCntCycle);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function gets the battery life time data.
 *
 * \param   BlockNumber - battery LTDB number (1 <= N <=3)
 * \param   pNbytes    - pointer to number of bytes in block data
 * \param   pData      - pointer to block data
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetLifeTimeDataBlock(uint16_t BlockNumber, uint8_t *pNbytes, uint8_t *pData)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */
    uint8_t        blockNumCmd;    /* Command for indicated block number */
    uint8_t        blockSize;      /* Size of operand for indicated block number */

    /* Initialize the variables */
    eStatusReturn = BATTERY_STATUS_OK;
    blockSize = 0u;
    blockNumCmd = 0u;

    switch (BlockNumber)
    {
        case BLOCK_1:
            blockNumCmd = IdxToCmd[BAT_CMD_LTIME_DATA_BLK_1].Cmd;
            blockSize   = IdxToCmd[BAT_CMD_LTIME_DATA_BLK_1].OpSize;
            break;

        case BLOCK_2:
            blockNumCmd = IdxToCmd[BAT_CMD_LTIME_DATA_BLK_2].Cmd;
            blockSize   = IdxToCmd[BAT_CMD_LTIME_DATA_BLK_2].OpSize;
            break;

        case BLOCK_3:
            blockNumCmd = IdxToCmd[BAT_CMD_LTIME_DATA_BLK_3].Cmd;
            blockSize   = IdxToCmd[BAT_CMD_LTIME_DATA_BLK_3].OpSize;
            break;

        case BLOCK_0:
            eStatusReturn = BATTERY_STATUS_INVALID_PARAM;
            break;

        default:
            eStatusReturn = BATTERY_STATUS_INVALID_PARAM;
            break;
    }

    if (BATTERY_STATUS_INVALID_PARAM != eStatusReturn)
    {
        opStatus = L3_SMBusReadBlock(BATTERY_SLAVE_ADDRESS,
                                     blockNumCmd,
                                     blockSize,
                                     pData);

        eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

        *pNbytes = ((BATTERY_STATUS_OK == eStatusReturn)? blockSize : 0);
    }
    else
    {
        *pNbytes = 0;
        Log(DBG, "Invalid battery life time data block requested: %u", BlockNumber);
    }

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns the AFE register values in the following
 *           format: AABBCCDDEEFFGGHHIIJJKK where:
 *              AA: STATUS register
 *              BB: STATE_CONTROL register
 *              CC: OUTPUT_CONTROL register
 *              DD: OUTPUT_STATUS register
 *              EE: FUNCTION_CONTROL register
 *              FF: CELL_SEL register
 *              GG: OCDV register
 *              HH: OCDD register
 *              II: SCC register
 *              JJ: SCD1 register
 *              KK: SCD2 register
 *
 * \param   pNbytes - pointer to number of bytes in block data
 * \param   pData   - pointer to block data
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetAFERegisters(uint8_t *pNbytes, uint8_t *pData)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */

    opStatus = L3_SMBusReadBlock(BATTERY_SLAVE_ADDRESS,
                                 IdxToCmd[BAT_CMD_AFE_REGISTERS].Cmd,
                                 IdxToCmd[BAT_CMD_AFE_REGISTERS].OpSize,
                                 pData);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);

    *pNbytes = ((BATTERY_STATUS_OK == eStatusReturn)? IdxToCmd[BAT_CMD_AFE_REGISTERS].OpSize : 0);

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function returns 30 bytes of IT data in the following format:
 *           aaAAbbBBccCCddDDeeEEffFFGGggHHhhIIiiJJjjkkKKllLLmmMMnnNNooOO where:
 *               AAaa: DOD0 Cell 0
 *               BBbb: DOD0 Cell 1
 *               CCcc: DOD0 Cell 2
 *               DDdd: DOD0 Cell 3
 *               EEee: Passed Charge since last DOD0 Update
 *               FFff: QMAX Cell 0
 *               GGgg: QMAX Cell 1
 *               HHhh: QMAX Cell 2
 *               IIii: QMAX Cell 3
 *               JJjjKKkk: State Time
 *               LLll: DOD EOC Cell 0
 *               MMmm: DOD EOC Cell 1
 *               NNnn: DOD EOC Cell 2
 *               OOoo: DOD EOC Cell 3
 *
 *
 * \param   StatusNum - battery IT Status number (1 <= N <=2)
 * \param   pNbytes  - pointer to number of bytes in block data
 * \param   pData    - pointer to status data
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetITStatus(uint32_t StatusNum, uint8_t *pNbytes, uint8_t *pData)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */
    uint8_t        itNumCmd;       /* Command for indicated status number */
    uint8_t        itSize;         /* Size of operand for indicated status number */

    /* Initialize the variables */
    eStatusReturn = BATTERY_STATUS_OK;
    itNumCmd = 0u;
    itSize = 0u;

    switch (StatusNum)
    {
        case STATUS_1:
            itNumCmd = IdxToCmd[BAT_CMD_IT_STATUS_1].Cmd;
            itSize   = IdxToCmd[BAT_CMD_IT_STATUS_1].OpSize;
            break;

        case STATUS_2:
            itNumCmd = IdxToCmd[BAT_CMD_IT_STATUS_2].Cmd;
            itSize   = IdxToCmd[BAT_CMD_IT_STATUS_2].OpSize;
          break;

        case STATUS_0:
            eStatusReturn = BATTERY_STATUS_INVALID_PARAM;
            break;

        default:
            eStatusReturn = BATTERY_STATUS_INVALID_PARAM;
            break;
    }

    if (BATTERY_STATUS_INVALID_PARAM != eStatusReturn)
    {
        opStatus = L3_SMBusReadBlock(BATTERY_SLAVE_ADDRESS,
                                     itNumCmd,
                                     itSize,
                                     pData);

        eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);
        *pNbytes = ((BATTERY_STATUS_OK == eStatusReturn)? itSize : 0);
    }
    else
    {
        *pNbytes = 0;
        Log(DBG, "Invalid battery IT status requested: %u", StatusNum);
    }

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function gets the battery status.
 *           Battery Status - Returns the Smart Battery's status word which
 *             contains Alarm and Status bit flags. Some of the BatteryStatus()
 *             flags (REMAINING_CAPACITY_ALARM and REMAINING_TIME_ALARM) are
 *             calculated based on either current or power depending on the
 *             setting of the BatteryMode()'s CAPACITY_MODE bit.
 *             This is important because use of the wrong calculation mode may
 *             result in an inaccurate alarm.
 *           Battery Safety Status      - Returns the Safety status flags
 *           Battery PF Status          - Returns the PF satus flags
 *           Battery Operation Status   - Returns the operational status flags
 *           Battery Charging Status    - Returns the charging status flags
 *           Battery Gauging Status     - Returns the gauging status flags
 *           Battery Manufacture Status - Returns the manufacture status flags
 *
 * \param   StatusNum  - battery Status number
 * \param   pNbytes   - pointer to number of bytes in block data
 * \param   pData     - pointer to status data
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetStatus(uint32_t StatusNum, uint8_t *pNbytes, uint8_t *pData)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */
    uint8_t        statCmd;        /* Command for indicated status number */
    uint8_t        statCmdSize;    /* Size of operand for indicated status number */
    uint8_t        StatusData[5];  /* Local Buffer for Read Data*/
    bool           BlockRead;      /* Control variable to decide on Block read or word read*/
    /* Initialize the variables */
    eStatusReturn = BATTERY_STATUS_LAST; /* Initialize return status */
    statCmd = 0u;
    statCmdSize = 0u;
    BlockRead = true;
    switch (StatusNum)
    {
        case CMD_BATTERY_STATUS: // Word read
            statCmd     = IdxToCmd[BAT_CMD_STATUS].Cmd;
            statCmdSize = IdxToCmd[BAT_CMD_STATUS].OpSize;
            BlockRead = false;
            break;

        case CMD_SAFETY_STATUS: // Block read
            statCmd     = IdxToCmd[BAT_CMD_SAFETY_STATUS].Cmd;
            statCmdSize = IdxToCmd[BAT_CMD_SAFETY_STATUS].OpSize;
            break;

        case CMD_PF_STATUS: // Block read
            statCmd     = IdxToCmd[BAT_CMD_PF_STATUS].Cmd;
            statCmdSize = IdxToCmd[BAT_CMD_PF_STATUS].OpSize;
            break;

        case CMD_OPERATION_STATUS: // Block read
            statCmd     = IdxToCmd[BAT_CMD_OPERATION_STATUS].Cmd;
            statCmdSize = IdxToCmd[BAT_CMD_OPERATION_STATUS].OpSize;
            break;

        case CMD_CHARGING_STATUS: // Block read
            statCmd     = IdxToCmd[BAT_CMD_CHARGING_STATUS].Cmd;
            statCmdSize = IdxToCmd[BAT_CMD_CHARGING_STATUS].OpSize;
            break;

        case CMD_GAUGING_STATUS: // Block read
            statCmd     = IdxToCmd[BAT_CMD_GAUGING_STATUS].Cmd;
            statCmdSize = IdxToCmd[BAT_CMD_GAUGING_STATUS].OpSize;
            break;

        case CMD_MANUF_STATUS: // Block read
            statCmd     = IdxToCmd[BAT_CMD_MANUF_STATUS].Cmd;
            statCmdSize = IdxToCmd[BAT_CMD_MANUF_STATUS].OpSize;
            break;

        default:
            eStatusReturn = BATTERY_STATUS_INVALID_PARAM;
            break;
    }

    if (BATTERY_STATUS_INVALID_PARAM != eStatusReturn)
    {

        if ( BlockRead )
        {
            opStatus = L3_SMBusReadBlock(BATTERY_SLAVE_ADDRESS, statCmd, statCmdSize+1, StatusData);
            memcpy(pData,&StatusData[1],statCmdSize);
        }
        else
        {
            L3_SMBusReadWord(BATTERY_SLAVE_ADDRESS,
                                             statCmd,
                                             pData);
        }

       eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);
       *pNbytes = ((BATTERY_STATUS_OK == eStatusReturn)? statCmdSize : 0);
    }
    else
    {
        *pNbytes = 0;
        Log(DBG, "Invalid battery status requested: %u", StatusNum);
    }

    /* Return the status */
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function Reads Battery Chemical ID from Manufacturer Data
 *              Send Battery_Manufacturer_Access_Byte (0x00) command with data ChemicalID (0x0006)
 *              Read the Manufacturer_Data to recieve the Chemical ID
 *
 * \param   pData     - Pointer to Data buffer to hold the read parameter value
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval  BATTERY_STATUS_OK - if the write operation is sucessful
 * \retval  BATTERY_STATUS_ERROR - if the write operation fails
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetChemicalID(uint16_t *pData)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */
    uint16_t       opData;         /* Operation data */
    uint8_t data[3];               /* Local Buffer to read the Block data */

    opStatus = SMBUS_READ_ERROR;// Read the 32byte Row

    do
    {
        BREAK_IF (NULL == pData);
        *pData = 0;
        opData = BAT_CMD_MFGACCESS_CHEMID;
        opStatus = L3_SMBusWriteWord(BATTERY_SLAVE_ADDRESS,
                                     IdxToCmd[BAT_MANUFACTURING_ACCESS_BYTE].Cmd,
                                     opData);
        BREAK_IF(SMBUS_NO_ERROR != opStatus);

        opStatus = L3_SMBusReadBlock( BATTERY_SLAVE_ADDRESS, BAT_CMD_MFGDATA, BQ_CHEMID_BLOCKSIZE, data);

        BREAK_IF(SMBUS_NO_ERROR != opStatus);
        *pData = data[2];
        *pData <<= 8;
        *pData |= data[1];
    } while (false);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);
    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief    This function Reads DataFlash Parameter value
 *              - Reads the entire 32 byte Row from BQ chip
 *                Evaluate the index of the DF parameter in the 32byte row and update the value in local buffer
 *                updates the pData with the interseted parameter values
 *                Refer Techinical Reference manual of bq30z554-R1 - Appendix B (Literature Number: SLUUA79)
 *
 * \param   pDFinfo   - Pointer to the BATTERY_DF_PARAM (parameter details to be read)
 * \param   pData     - Pointer to Data buffer to hold the read parameter value
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval  BATTERY_STATUS_OK - if the write operation is sucessful
 * \retval  BATTERY_STATUS_ERROR - if the write operation fails
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatteryGetDataFlash(BATTERY_DF_PARAM *pDFinfo, uint8_t *pData)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    SMBUS_STATUS   opStatus;       /* SMBus communication status */
    uint16_t       opData;         /* Operation data */
    static uint16_t PhyAddr;
    static uint8_t RowNo;
    static uint8_t ByteIdx;
    uint8_t RowData[BAT_DF_ROWDATA_SIZE+1];
    uint8_t i;

    BATT_DF_TM_DATA TmDFinfo;

    opStatus = SMBUS_READ_ERROR;  // Read the 32byte Row
    do
    {
        BREAK_IF(NULL == pDFinfo);
        // calculate the physical address of the 32byte Row based on SubClass ID and Offset
        PhyAddr = pDFinfo->SubClsId + pDFinfo->Offset;
        // calculate the Row number and Byteindex
        RowNo = PhyAddr / BAT_DF_ROWDATA_SIZE;
        // Byteindex is incremented by one as the Read operation returns 33bytes (byte 0 being the size information)
        ByteIdx = (PhyAddr % BAT_DF_ROWDATA_SIZE)+1;// Read the 32byte Row

        opData = BAT_OPDATA_OFFSET+RowNo;
        opStatus = L3_SMBusWriteWord(BATTERY_SLAVE_ADDRESS,
                                      BAT_MANUFACTURING_ACCESS_BYTE,
                                      opData);
        BREAK_IF(SMBUS_NO_ERROR != opStatus);

        opStatus = L3_SMBusReadBlock(BATTERY_SLAVE_ADDRESS, BAT_CMD_MFG_INFO, BAT_DF_ROWDATA_SIZE+1, RowData);

        BREAK_IF(SMBUS_NO_ERROR != opStatus);

        // Retrieve the intrested DF value from 32byte Row data
        for (i=0; i < pDFinfo->Size; i++)
        {
            pData[i] = RowData[ByteIdx++];
        }

    } while(false);
    TmDFinfo.DFparameter = pDFinfo;
    TmDFinfo.pValue = pData;
    TM_Hook(HOOK_BATTERYDFFAIL, (void *)(&TmDFinfo));

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief    This function updates DataFlash Parameter value.
 *
 * \details  The BQ flash write is a 32 byte operation. Begins with reading 32 bytes
 *           from the BQ data flash by providing the row number of the BQ flash parameter
 *           to be updated. The row number is calculated form the subclass id and offset
 *           provided in:
 *           Technical Reference manual of bq30z554-R1 - Appendix B (Literature Number: SLUUA79)
 *
 *           Update the required number of bytes to be written in the read response array.
 *           Write back 33 bytes..!
 *
 *     Note: The read operation returns 33 bytes. The first byte is the size of the
 *           read flash data (32 in this case), this is provided by BQ chip.
 *           While writing back write 33 bytes.
 *
 *
 * \param   *pDFinfo   - Pointer to the BATTERY_DF_PARAM (parameter details to be updated)
 * \param   *pData     - Data value to be updated
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval  BATTERY_STATUS_OK - if the write operation is sucessful
 * \retval  BATTERY_STATUS_ERROR - if the write operation fails
 *
 * ========================================================================== */
BATTERY_STATUS L3_BatterySetDataFlash(BATTERY_DF_PARAM *pDFinfo, uint8_t *pData)
{
    BATTERY_STATUS eStatusReturn;  // Battery Status Return
    SMBUS_STATUS   opStatus;       // SMBus communication status
    uint16_t       opData;         // Operation data
    uint16_t PhyAddr;
    uint8_t RowNo;
    uint8_t ByteIdx;
    uint8_t RowData[BAT_DF_ROWDATA_SIZE+2];
    uint8_t DataIndex;

    opStatus = SMBUS_READ_ERROR;// Read the 32byte Row
    do
    {
        BREAK_IF(NULL == pDFinfo);

        // calculate the physical address of the 32byte Row based on SubClass ID and Offset
        PhyAddr = pDFinfo->SubClsId + pDFinfo->Offset;

        // calculate the Row number and Byteindex
        RowNo = PhyAddr / BAT_DF_ROWDATA_SIZE;

        // Byteindex is incremented by one as the Read operation returns 33bytes (byte 0 being the size information)
        ByteIdx = (PhyAddr % BAT_DF_ROWDATA_SIZE)+1;

        // opData includes ManufacturerAccess code and row number
        opData = BAT_OPDATA_OFFSET + RowNo;

        // Send the data flash row number to bq30z554-R1 using MAC command 0x1yy.
        opStatus = L3_SMBusWriteWord(BATTERY_SLAVE_ADDRESS,BAT_MANUFACTURING_ACCESS_BYTE, opData);

        //break if error in sending Bq chip flash command
        BREAK_IF(SMBUS_NO_ERROR != opStatus);


        // Read flash block of 32 bytes
        opStatus = L3_SMBusReadBlock(BATTERY_SLAVE_ADDRESS, BAT_CMD_MFG_INFO, BAT_DF_ROWDATA_SIZE+1, RowData);

        //break if error in reading Bq chip flash data
        BREAK_IF(SMBUS_NO_ERROR != opStatus);

        for (DataIndex=0; DataIndex< pDFinfo->Size; DataIndex++)
        {// Update the interested DF parameter value in the 32byte RowData
            RowData[ByteIdx++] = pData[DataIndex];
        }

        // Send the data flash row number to bq30z554-R1 using MAC command 0x1yy.
        opStatus = L3_SMBusWriteWord(BATTERY_SLAVE_ADDRESS, BAT_MANUFACTURING_ACCESS_BYTE, opData);

        //break if error in sending Bq chip flash command
        BREAK_IF(SMBUS_NO_ERROR != opStatus);

        // Write 33 byte data, the first index in array is not written to BQ flash its the size info of the received data
        opStatus = L3_SMBusWriteBlock(BATTERY_SLAVE_ADDRESS, BAT_CMD_MFG_INFO, BAT_DF_ROWDATA_SIZE + 1, RowData);

        //break if write operation failed
        BREAK_IF(SMBUS_NO_ERROR != opStatus);

        // Give the BQ chip time to program its internal flash
        OSTimeDly(BQ_CHIP_DATA_WRITE_TIME);

         // Send the data flash row number to bq30z554-R1 using MAC command 0x1yy.
        opStatus = L3_SMBusWriteWord(BATTERY_SLAVE_ADDRESS, BAT_MANUFACTURING_ACCESS_BYTE, opData);

        //break if error in writing Bq chip flash data
        BREAK_IF(SMBUS_NO_ERROR != opStatus);

        // Read flash block of 32 bytes
        opStatus = L3_SMBusReadBlock(BATTERY_SLAVE_ADDRESS, BAT_CMD_MFG_INFO, BAT_DF_ROWDATA_SIZE+1, RowData);

        //break if error in reading Bq chip flash data
        BREAK_IF(SMBUS_NO_ERROR != opStatus);

        ByteIdx = (PhyAddr % BAT_DF_ROWDATA_SIZE)+1;

        for (DataIndex=0; DataIndex< pDFinfo->Size; DataIndex++)
        { //Compare the read data and data written
            if(RowData[ByteIdx++] != pData[DataIndex])
            {
                Log(DBG,"DF paramter Read Write mismatch");
            }
        }

    } while(false);

    eStatusReturn = ((SMBUS_NO_ERROR == opStatus)? BATTERY_STATUS_OK : BATTERY_STATUS_ERROR);
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   Enable PEC mode
 *
 * \details This function enables the PEC (Parity Error Check) for the BQ chip
 *
 * \param   < None >
 *
 * \return  BATTERY_STATUS - Battery Status
 * \retval      See L3_Battery.h
 *
 * ========================================================================== */
 BATTERY_STATUS L3_BatteryPECEnable(void)
{
    BATTERY_STATUS eStatusReturn;  /* Battery Status Return */
    static BATTERY_DF_PARAM DF_Param;
    static uint8_t Val;

    eStatusReturn = BATTERY_STATUS_ERROR;
    do
    {
        DF_Param.Offset = SMBUS_SBSCONFIG_OFFSET;
        DF_Param.Size = SMBUS_SBSCONFIG_SIZE;
        DF_Param.SubClsId = SMBUS_SBSCONFIG_SUBCLSID ;
        // Read sbs settings from Battery Datafield
        if (BATTERY_STATUS_OK != L3_BatteryGetDataFlash(&DF_Param,&Val))
        {
            L3_SMBus_UpdatePECFlag(false);
            break;
        }
        // check for bit 2 (HPE - PCE on Host Communication) if ENABLED then enable PEC_mode by setting the PECFlag to true
        if (Val & SMBUS_HPE_MASK)
        {
            L3_SMBus_UpdatePECFlag(true);
            break;
        }
        // if not Enable the bit
        Val |= SMBUS_HPE_MASK;
        // Write the Datafield with new value
        if (BATTERY_STATUS_OK !=L3_BatterySetDataFlash(&DF_Param,&Val))
        {
            // if write fails make the PEC_mode to false
            L3_SMBus_UpdatePECFlag(false);
            break;
        }
        // if write is successful then enable PEC_mode by setting the PECFlag to true
        L3_SMBus_UpdatePECFlag(true);
        eStatusReturn = BATTERY_STATUS_OK;
    } while (false);
    return eStatusReturn;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

