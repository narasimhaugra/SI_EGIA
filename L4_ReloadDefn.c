#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{
 *
 * \brief   Reload Definition functions
 *
 * \details The Reload Definition defines all the interfaces used for
            communication between Handle and Reload.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_ReloadDefn.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_ReloadDefn.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
AM_RELOAD_IF ReloadInterface;              /*! Reload Object Interface */

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
static DEVICE_UNIQUE_ID   ReloadAddress;      /*! Reload identifier */

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static AM_STATUS ReloadEepRead(void);
static AM_STATUS ReloadEepUpdate(void);

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
 * \details Set the Reload unique 1-wire identifier
 *
 * \param   DeviceAddress - 1-Wire device address
 * \param   pData - pointer to data
 *
 * \return  None
 *
 * ========================================================================== */
void ReloadSetDeviceId(DEVICE_UNIQUE_ID DeviceAddress, uint8_t *pData)
{
    // Set device unique address
    ReloadAddress = DeviceAddress;
    ReloadInterface.Update = &ReloadEepUpdate;
    ReloadInterface.Read   = &ReloadEepRead;
    // Read and update local data repository. User is allowed update all attributes.
    memcpy((uint8_t*)&ReloadInterface.Data, &pData[0],ONEWIRE_MEMORY_TOTAL_SIZE);

    ReloadInterface.Status = AM_STATUS_OK;
}

/* ========================================================================== */
/**
 * \brief   Reload EEPROM read
 *
 * \details Read Reload 1-wire EEPROM memory
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in reading EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in reading EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS ReloadEepRead(void)
{
    AM_STATUS Status;         /* Function status */
    OW_EEP_STATUS OwEepStatus;
    uint8_t *pData;
    uint16_t CalcCRC;
    Status = AM_STATUS_OK;
    pData = (uint8_t*)&ReloadInterface.Data;

    OwEepStatus  = L3_OneWireEepromRead(ReloadAddress, 0, pData);
    OwEepStatus |= L3_OneWireEepromRead(ReloadAddress, 1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);

    do
    {
        if (OW_EEP_STATUS_OK != OwEepStatus)
        {
            Log(DBG, "Reload EEP Or'ed Read Error: %d", OwEepStatus);
            Status = AM_STATUS_ERROR;
            break;
        }
        /*Check for Data integrity*/
        CalcCRC = CRC16(0,pData,(size_t)(ONEWIRE_MEMORY_TOTAL_SIZE - sizeof(ReloadInterface.Data.Crc)));
        if (ReloadInterface.Data.Crc != CalcCRC)
        {
            Log(DBG, "Reload EEPRead: EEPROM CRC validation failed");
            Status = AM_STATUS_ERROR;
        }
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Reload EEPROM write
 *
 * \details Write Reload 1-wire EEPROM memory
 *
 * \param   pData  - Pointer to the data to write
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in writing EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in writing EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS ReloadEepWrite(uint8_t *pData)
{
    AM_STATUS Status;           /* Function status */
    OW_EEP_STATUS OwEepStatus;  /* 1-Wire Eep function call status */
    MemoryLayoutEgiaMulu_Ver2 *pOneWireMemFormat;

    Status = AM_STATUS_OK;
    pOneWireMemFormat = (MemoryLayoutEgiaMulu_Ver2*)pData;
    /*Update the calculated CRC in the WriteData buffer*/
    pOneWireMemFormat->Crc = CRC16(0,pData,(size_t)(ONEWIRE_MEMORY_TOTAL_SIZE - sizeof(pOneWireMemFormat->Crc)));
    OwEepStatus  = L3_OneWireEepromWrite(ReloadAddress, 0, pData);
    OwEepStatus |= L3_OneWireEepromWrite(ReloadAddress, 1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);

    if (OW_EEP_STATUS_OK != OwEepStatus)
    {
        Log(DBG, "Reload EEP Or'ed Write Error: %d", OwEepStatus);
        Status = AM_STATUS_ERROR;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Flush out RAM content to EEPROM
 *
 * \details Write Reload 1-wire EEPROM memory with the local data
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function return Status
 *
 * ========================================================================== */
static AM_STATUS ReloadEepUpdate(void)
{
    ReloadInterface.Status = ReloadEepWrite((uint8_t*)&ReloadInterface.Data);
    return ReloadInterface.Status;
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

