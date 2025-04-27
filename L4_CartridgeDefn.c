#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{
 *
 * \brief   Cartridge Definition functions
 *
 * \details The Cartridge Definition defines all the interfaces used for
            communication between Handle and Cartridge.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_CartridgeDefn.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_CartridgeDefn.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER       (LOG_GROUP_ADAPTER)   /*! Log Group Identifier */

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static DEVICE_UNIQUE_ID   CartridgeAddress;      /*! Cartridge identifier */

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static AM_STATUS CartridgeEepRead(void);

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
AM_CARTRIDGE_IF CartridgeInterface; /*! Cartridge Object Interface */

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
static AM_STATUS CartridgeEepUpdate(void);

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Set the device id
 *
 * \details Set the Cartridge unique 1-wire identifier
 *
 * \param   DeviceAddress - 1-Wire device address
 * \param   pData - pointer to data
 *
 * \return  None
 *
 * ========================================================================== */
void CartridgeSetDeviceId(DEVICE_UNIQUE_ID DeviceAddress, uint8_t *pData)
{
    // Set device unique address, map update function
    CartridgeAddress = DeviceAddress;
    CartridgeInterface.Update = &CartridgeEepUpdate;
    CartridgeInterface.Read   = &CartridgeEepRead;

    // Read and update local data repository. User is allowed update all attributes
    memcpy((uint8_t *)&CartridgeInterface.Data, &pData[0], ONEWIRE_MEMORY_TOTAL_SIZE);
    CartridgeInterface.Status = AM_STATUS_OK;
}

/* ========================================================================== */
/**
 * \brief   Cartridge EEPROM read
 *
 * \details Read Cartridge 1-wire EEPROM memory
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in reading EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in reading EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS CartridgeEepRead(void)
{
    AM_STATUS Status;         /* Function status */
    OW_EEP_STATUS OwEepStatus;
    uint8_t *pData;
    uint16_t CalcCRC;        /*Calculated CRC */

    CalcCRC = 0x0;
    Status = AM_STATUS_OK;
    pData = (uint8_t*)&CartridgeInterface.Data;

    OwEepStatus  = L3_OneWireEepromRead(CartridgeAddress, 0, pData);
    OwEepStatus |= L3_OneWireEepromRead(CartridgeAddress, 1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);

    do
    {
        if (OW_EEP_STATUS_OK != OwEepStatus)
        {
            Log(DBG, "Cartridge EEP Or'ed Read Error: %d", OwEepStatus);
            Status = AM_STATUS_ERROR;
            break;
        }
        /*Calculate the CRC*/
        CalcCRC = CRC16(0, pData,(size_t)(ONEWIRE_MEMORY_TOTAL_SIZE - sizeof( CartridgeInterface.Data.Crc )));

        /*Check for Data integrity*/
        if (CartridgeInterface.Data.Crc != CalcCRC)
        {
            Log(ERR, "cartridge  EEPRead: EEPROM CRC validation failed");
            Status = AM_STATUS_ERROR;
        }
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Cartridge EEPROM write
 *
 * \details Write Cartridge 1-wire EEPROM memory
 *
 * \param   pData  - Pointer to the data to write
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in writing EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in writing EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS CartridgeEepWrite(uint8_t *pData)
{
    AM_STATUS Status;           /* Function status */
    OW_EEP_STATUS OwEepStatus;  /* 1-Wire Eep function call status */
    MemoryLayoutEgiaCart_Ver2 *pOneWireMemFormat;  /* Pointer to basic one wire memory format */

    Status = AM_STATUS_OK;

    pOneWireMemFormat = (MemoryLayoutEgiaCart_Ver2*)pData;
    /* Update the calculated CRC in the WriteData buffer */
    pOneWireMemFormat->Crc = CRC16(0, pData,(size_t)(ONEWIRE_MEMORY_TOTAL_SIZE - sizeof(pOneWireMemFormat->Crc)));

    OwEepStatus  = L3_OneWireEepromWrite(CartridgeAddress, 0, pData);
    OwEepStatus |= L3_OneWireEepromWrite(CartridgeAddress, 1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);

    if (OW_EEP_STATUS_OK != OwEepStatus)
    {
        Log(DBG, "Cartridge EEP Or'ed Write Error: %d");
        Status = AM_STATUS_ERROR;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Flush out RAM content to EEPROM
 *
 * \details Write Cartridge 1-wire EEPROM memory with the local data
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function return Status
 *
 * ========================================================================== */
static AM_STATUS CartridgeEepUpdate(void)
{
    CartridgeInterface.Status = CartridgeEepWrite((uint8_t*)&CartridgeInterface.Data);
    return CartridgeInterface.Status;
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

