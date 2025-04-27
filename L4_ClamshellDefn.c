#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{
 *
 * \brief   Clamshell Definition functions
 *
 * \details The Clamshell Definition defines all the interfaces used for
            communication between Handle and Clamshell.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_ClamshellDefn.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_ClamshellDefn.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
AM_CLAMSHELL_IF ClamshellInterface;       /*! Clamshell Object Interface */

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_ADAPTER) /*! Log Group Identifier */
#define CLAMSHELL_USED          (1u)                /*! Clamshell used status */
#define CLAMSHELL_UNUSED        (0)                 /*! Clamshell unused status */

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static DEVICE_UNIQUE_ID   ClamshellAddress;      /*! Clamshell identifier */

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static AM_STATUS ClamshellEepUpdate(void);
static AM_STATUS ClamshellEepRead(void);

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
 * \details Set the Clamshell unique 1-wire identifier
 *
 * \param   DeviceAddress - 1-Wire device address
 * \param   pData - pointer to data
 *
 * \return  None
 *
 * ========================================================================== */
void ClamshellSetDeviceId(DEVICE_UNIQUE_ID DeviceAddress, uint8_t *pData)
{
    // Set device unique address
    ClamshellAddress = DeviceAddress;
    ClamshellInterface.Update = &ClamshellEepUpdate;

    // Read and update local data repository. User is allowed update all attributes.
    memcpy((uint8_t*)&ClamshellInterface.Data, &pData[0],ONEWIRE_MEMORY_TOTAL_SIZE);

    ClamshellInterface.Status = AM_STATUS_OK;
}

/* ========================================================================== */
/**
 * \brief   Clamshell EEPROM read
 *
 * \details Read Clamshell 1-wire EEPROM memory
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in reading EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in reading EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS ClamshellEepRead(void)
{
    AM_STATUS Status;         /* Function status */
    OW_EEP_STATUS OwEepStatus;
    uint8_t *pData;
    uint16_t CalcCRC;

    Status = AM_STATUS_OK;
    CalcCRC = 0;
    pData = (uint8_t*)&ClamshellInterface.Data;

    OwEepStatus  = L3_OneWireEepromRead(ClamshellAddress, 0, pData);
    OwEepStatus |= L3_OneWireEepromRead(ClamshellAddress, 1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);

    do
    {
        if (OW_EEP_STATUS_OK != OwEepStatus)
        {
            Log(ERR, "Clamshell EEP Or'ed Read Error: %d", OwEepStatus);
            Status = AM_STATUS_ERROR;
            break;
        }

        /* Check for Data integrity */
        CalcCRC = CRC16(CalcCRC, pData,(size_t)(ONEWIRE_MEMORY_TOTAL_SIZE - sizeof( ClamshellInterface.Data.Crc )));
        if (ClamshellInterface.Data.Crc != CalcCRC)
        {
            Log(ERR, "Clamshell EEPRead: EEPROM CRC validation failed");
            Status = AM_STATUS_ERROR;
        }

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Clamshell EEPROM write
 *
 * \details Write Clamshell 1-wire EEPROM memory
 *
 * \param   pData  - Pointer to the data to write
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in writing EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in writing EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS ClamshellEepWrite(uint8_t *pData)
{
    AM_STATUS Status;           /* Function status */
    OW_EEP_STATUS OwEepStatus;  /* 1-Wire Eep function call status */

    MemoryLayoutClamshell_Ver2 *pOneWireMemFormat;  /* Pointer to basic one wire memory format */

    Status = AM_STATUS_OK;

    pOneWireMemFormat = (MemoryLayoutClamshell_Ver2*)pData;

    /* Update the calculated CRC in the WriteData buffer */
    pOneWireMemFormat->Crc = CRC16(0, pData,(size_t)(ONEWIRE_MEMORY_TOTAL_SIZE - sizeof(pOneWireMemFormat->Crc)));

    OwEepStatus  = L3_OneWireEepromWrite(ClamshellAddress, 0, pData);
    OwEepStatus |= L3_OneWireEepromWrite(ClamshellAddress, 1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);

    if (OW_EEP_STATUS_OK != OwEepStatus)
    {
        Log(DBG, "Clamshell EEP Write Error");
        Status = AM_STATUS_ERROR;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Flush out RAM content to EEPROM
 *
 * \details Write Clamshell 1-wire EEPROM memory with the local data
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function return Status
 *
 * ========================================================================== */
static AM_STATUS ClamshellEepUpdate(void)
{
    ClamshellInterface.Status = ClamshellEepWrite((uint8_t*)&ClamshellInterface.Data);
    return ClamshellInterface.Status;
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

