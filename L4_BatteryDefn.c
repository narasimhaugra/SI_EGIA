#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{
 *
 * \brief   Battery Definition functions
 *
 * \details The Battery Definition defines all the interfaces used for
            communication between Handle and Battery.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_BatteryDefn.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_BatteryDefn.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
  static AM_BATTERY_IF BatteryInterface = {{0},NULL,NULL,AM_STATUS_ERROR};       // Battery Object Interface

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER  (LOG_GROUP_ADAPTER) // Log Group Identifier
#define BATTERY_USED          (1u)                // Battery used status
#define BATTERY_UNUSED        (0)                 // Battery unused status

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static DEVICE_UNIQUE_ID   BatteryAddress;      // Battery identifier

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static AM_STATUS BatteryEepUpdate(void);
static AM_STATUS BatteryEepRead(void);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Set the device id
 *
 * \details Set the Battery unique 1-wire identifier
 *
 * \param   DeviceAddress - 1-Wire device address
 * \param   pData - pointer to data
 *
 * \return  None
 *
 * ========================================================================== */
void BatterySetDeviceId(DEVICE_UNIQUE_ID DeviceAddress, uint8_t *pData)
{
    // Set device unique address
    BatteryAddress = DeviceAddress;
    BatteryInterface.Update = &BatteryEepUpdate;
    BatteryInterface.Read = &BatteryEepRead;

    // Read and update local data repository. User is allowed update all attributes.
    memcpy((uint8_t*)&BatteryInterface.Data, &pData[0],ONEWIRE_MEMORY_TOTAL_SIZE);

    BatteryInterface.Status = AM_STATUS_OK;
}

/* ========================================================================== */
/**
 * \brief   Battery EEPROM read
 *
 * \details Read Battery 1-wire EEPROM memory
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in reading EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in reading EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS BatteryEepRead(void)
{
    AM_STATUS Status;         // Function status
    OW_EEP_STATUS OwEepStatus;
    uint8_t *pData;
    uint16_t CalcCRC;

    Status = AM_STATUS_OK;
    CalcCRC = 0;
    pData = (uint8_t*)&BatteryInterface.Data;

    OwEepStatus  = L3_OneWireEepromRead(BatteryAddress, 0, pData);
    OwEepStatus |= L3_OneWireEepromRead(BatteryAddress, 1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);

    do
    {
        if (OW_EEP_STATUS_OK != OwEepStatus)
        {
            Log(ERR, "Battery EEP Or'ed Read Error: %d", OwEepStatus);
            Status = AM_STATUS_ERROR;
            break;
        }

        // Check for Data integrity
        CalcCRC = CRC16(CalcCRC, pData,(size_t)(ONEWIRE_MEMORY_TOTAL_SIZE - sizeof( BatteryInterface.Data.Crc )));
        if (BatteryInterface.Data.Crc != CalcCRC)
        {
            Log(ERR, "Battery EEPRead: EEPROM CRC validation failed");
            Status = AM_STATUS_ERROR;
        }

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Battery EEPROM write
 *
 * \details Write Battery 1-wire EEPROM memory
 *
 * \param   pData  - Pointer to the data to write
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in writing EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in writing EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS BatteryEepWrite(uint8_t *pData)
{
    AM_STATUS Status;           // Function status
    OW_EEP_STATUS OwEepStatus;  // 1-Wire Eep function call status

    MemoryLayoutBattery_Ver2 *pOneWireMemFormat;  // Pointer to basic one wire memory format

    Status = AM_STATUS_OK;

    pOneWireMemFormat = (MemoryLayoutBattery_Ver2*)pData;

    // Update the calculated CRC in the WriteData buffer
    pOneWireMemFormat->Crc = CRC16(0, pData,(size_t)(ONEWIRE_MEMORY_TOTAL_SIZE - sizeof(pOneWireMemFormat->Crc)));

    OwEepStatus  = L3_OneWireEepromWrite(BatteryAddress, 0, pData);
    OwEepStatus |= L3_OneWireEepromWrite(BatteryAddress, 1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);

    if (OW_EEP_STATUS_OK != OwEepStatus)
    {
        Log(DBG, "Battery EEP Write Error");
        Status = AM_STATUS_ERROR;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Flush out RAM content to EEPROM
 *
 * \details Write Battery 1-wire EEPROM memory with the local data
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function return Status
 *
 * ========================================================================== */
static AM_STATUS BatteryEepUpdate( void )
{
    BatteryInterface.Status = BatteryEepWrite((uint8_t*)&BatteryInterface.Data);
    return BatteryInterface.Status;
}

/* ========================================================================== */
/**
 * \brief   API to get the Battery interface
 *
 * \param   < None >
 *
 * \return  AM_HANDLE_IF - Pointer to Battery interface
 *
 * ========================================================================== */
AM_BATTERY_IF* BatteryGetIF(void)
{
    return &BatteryInterface;
}


/**
 * \}
 **/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

