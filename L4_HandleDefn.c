#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{
 *
 * \brief   Handle Definition functions
 *
 * \details The Handle Definition defines all the interfaces used for
            communication between Handle and Handle.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_HandleDefn.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_HandleDefn.h"
#include "Signia_AdapterManager.h"
#include "L3_GpioCtrl.h"
#include "FaultHandler.h"
#include "L2_Flash.h"
#include "L2_Adc.h"
#include "TestManager.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
AM_HANDLE_IF HandleInterface = {{0}, NULL, AM_STATUS_ERROR, {0}, NULL, NULL, HW_VER_NONE};;        ///< Handle Object Interface
HANDLE_DATA HandleData;

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_ADAPTER) ///< Log Group Identifier
#define HANDLE_USED             (1u)                ///< Handle used status
#define HANDLE_UNUSED           (0u)                ///< Handle unused status

#define HANDLE_FLASHDATA_START      (0x0007D000u)    ///< Handle Flash Data Section Start, Reserved for system data: usage counters...
#define HANDLE_FLASHDATA_END        (0x0007F000u)    ///< Handle Flash Data Section End, Reserved for system data: usage counters...

#define HANDLE_FLASHDATA_SECTORSIZE (HANDLE_FLASHDATA_END - HANDLE_FLASHDATA_START)     ///< Size of the sector available for Handle Flash data

#define OW_EEPROM_MEMORY_PAGE_SIZE                 (32u)           ///< Memory Page Size.
#define OW_EEPROM_MEMORY_SEGMENT_SIZE              (4u)            ///< Memory Segment Size.
#define OW_EEPROM_NUM_SEGMENTS_PER_PAGE            (8u)            ///< No of Segment Per Page.
#define OW_EEPROM_NUM_PAGES                        (2u)            ///< Number of Pages. *
#define OW_EEPROM_PAGE0                            (0u)
#define OW_EEPROM_PAGE1                            (1u)

#define ADC_TIMEOUT     MSEC_2      ///< Timeout set to 2ms for ADC1 conversion
#define ADC_OFFSET      0u          ///< Offset value for the ADC1
#define HANDLE_ADCNUM   ADC1        ///< ADC used to determine HW version number, Voltage and BoardTemperature
#define ZERO_VALUE      (0u)        ///< Zero Decimal Value
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef struct                      ///< Table to determine Handle Hw version from ADC
{
    HANDLE_HW_VERSIONS version;     ///< Handle HW Version
    uint16_t AdcCountLoThreshold;   ///< Lo Threashold = AdcCount - ADC_HW_VOLT_TOLERANCE*AdcCount
    uint16_t AdcCountHiThreshold;   ///< Hi Threashold = AdcCount + ADC_HW_VOLT_TOLERANCE*AdcCount
} HW_VERSION_TABLE;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
static const HW_VERSION_TABLE Handle_HWVersionTable[HW_VER_COUNT] =
{
    { HW_VER_NONE,  7680, 7680},
    { HW_VER_5,    23728, 27575},   //ADCREF (excitation) 1.195VR1 = 365K, R2 = 1650000,  Vref 2.5 => Calculated value = 25652, Tolerance = 7.5%
    { HW_VER_4,  27956, 32489},   //ADCREF (excitation) 1.195VR1 = 365K, R2 = 10000000, Vref 2.5 => Calculated value = 30223, Tolerance = 7.5%
};

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static DEVICE_UNIQUE_ID   HandleAddress = NULL;      ///< Handle identifier

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static AM_STATUS HandleEepUpdate(void);
static AM_STATUS HandleEepRead(void);
static AM_STATUS HandleUpdateFlashData(void);
static AM_STATUS HandleReadFlashData(void);
static AM_STATUS Handle_UpdateHWVersion(void);
static HANDLE_HW_VERSIONS CalculateHWVersion(uint16_t AdcCount);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Handle EEPROM read
 *
 * \details Read Handle 1-wire EEPROM memory and stores the data in the Handle Interface
 *
 * \param   < None >
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in reading EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in reading EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS HandleEepRead(void)
{
    AM_STATUS Status;
    uint8_t *pData;
    uint16_t calccrc;
    OW_EEP_STATUS OwEepStatus;

    calccrc = 0;

    do
    {
        Status = AM_STATUS_OK;
        pData = (uint8_t*)&HandleInterface.Data;

     /*Perform a pagewire read operation to read entier 64Byte data*/
        OwEepStatus  = L3_OneWireEepromRead(HandleAddress, OW_EEPROM_PAGE0, pData);
        OwEepStatus |= L3_OneWireEepromRead(HandleAddress, OW_EEPROM_PAGE1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);

        if (OW_EEP_STATUS_OK != OwEepStatus)
        {
            Log(DBG, "Handle EEP Or'ed Read Error: %d", OwEepStatus);
            Status = AM_STATUS_ERROR;
            break;
        }

        /*Check for Data integrity*/
        calccrc = CRC16(calccrc,pData,(size_t)(ONEWIRE_MEMORY_TOTAL_SIZE - sizeof(HandleInterface.Data.Crc)));
        if (HandleInterface.Data.Crc != calccrc)
        {
            Log(DBG, "HandleEEPRead: EEPROM CRC validation failed");
            Status = AM_STATUS_ERROR;
        }
    } while (false);
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Handle EEPROM write
 *
 * \details Write Handle 1-wire EEPROM memory
 *
 * \param   pData  - Pointer to the data to write
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK     - No error in writing EEPROM memory
 * \retval  AM_STATUS_ERROR  - Error in writing EEPROM memory
 *
 * ========================================================================== */
static AM_STATUS HandleEepWrite(uint8_t *pData)
{
    OW_EEP_STATUS OwEepStatus;
    AM_STATUS Status;
    MemoryLayoutHandle_Ver2 *pOneWireMemFormat;  /* Pointer to basic one wire memory format */

    Status = AM_STATUS_OK;

    pOneWireMemFormat = (MemoryLayoutHandle_Ver2*)pData;

    /*Update the calculated CRC in the WriteData buffer*/
    pOneWireMemFormat->Crc = CRC16(0,pData,(size_t)(ONEWIRE_MEMORY_TOTAL_SIZE - sizeof(pOneWireMemFormat->Crc)));

    /*Perform a pagewise write operation*/
    OwEepStatus  = L3_OneWireEepromWrite(HandleAddress, OW_EEPROM_PAGE0, pData);
    OwEepStatus |= L3_OneWireEepromWrite(HandleAddress, OW_EEPROM_PAGE1, pData + OW_EEPROM_MEMORY_PAGE_SIZE);

    if (OW_EEP_STATUS_OK != OwEepStatus)
    {
        Log(DBG, "Handle EEP Or'ed Write Error: %d");
        Status = AM_STATUS_ERROR;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Flush out RAM content to EEPROM
 *
 * \details Write Handle 1-wire EEPROM memory with the local data
 *
 * \param   <none>
 *
 * \return  AM_STATUS - Function return Status
 *
 * ========================================================================== */
static AM_STATUS HandleEepUpdate(void)
{
    HandleInterface.Status = HandleEepWrite((uint8_t*)&HandleInterface.Data);
    return HandleInterface.Status;
}

/* ========================================================================== */
/**
 * \brief Update Handle Hardware version
 *
 * \details The function runs only once during starting of the handle. It determines
 *          the Handle Hardware version by reading the voltage at ADC1 channel2 (DP1).
 *          During L2 init ADC1 is configured to read HW version.
 *
 * \note This function should be called only when all the Allegro chips are enabled to avoid erroneous results.
 *
 *
 * \param   < None >
 *
 * \return AM_STATUS
 *
 * ========================================================================== */
static AM_STATUS Handle_UpdateHWVersion(void)
{
    AM_STATUS Status;
    uint16_t HwVersionADCcount;
    ADC_STATUS AdcStatus;
    uint32_t timeout;

    Status = AM_STATUS_ERROR;
    // Enable reference 2.5V
    L3_GpioCtrlClearSignal(GPIO_EN_2P5V);
    do
    {
        BREAK_IF (L2_AdcSetOffset(HANDLE_ADCNUM, ADC_OFFSET));   /* Clear offset in ADC structure */
        // Start ADC measurement
        BREAK_IF (L2_AdcStart(HANDLE_ADCNUM, false));
        // Wait till the ADC measurement is done or timeout
        timeout = SigTime() + ADC_TIMEOUT;
        while ((L2_AdcGetStatus(HANDLE_ADCNUM) == ADC_STATUS_BUSY) && (SigTime() < timeout));
        // Read the ADC converstion status and ADC count
        AdcStatus = L2_AdcRead(HANDLE_ADCNUM, &HwVersionADCcount);
        BREAK_IF (ADC_STATUS_DATA_NEW != AdcStatus);
        // convert the ADC count to HW version number
        HandleInterface.VersionNumber = CalculateHWVersion(HwVersionADCcount);
        Status = AM_STATUS_OK;
    } while(false);

    if (Status)
    {
      Log(DBG, "Handle: Error Reading HW Version");
    }
    else
    {
      Log(DBG, "Handle: HW Version = %s", ((HandleInterface.VersionNumber == HW_VER_4)? "V4" : \
                                           (HandleInterface.VersionNumber == HW_VER_5)? "V5" : "Unknown"));
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief calculates the Hardware version from the ADC voltage
 *
 * \details This is a support function to calculate the Hardware version from the ADC count
 *
 * \param   AdcCount
 *
 * \return  AM_STATUS - Function return Status
 *
 * ========================================================================== */
static HANDLE_HW_VERSIONS CalculateHWVersion(uint16_t AdcCount)
{
    uint8_t temp;
    HANDLE_HW_VERSIONS Version;

    Version= HW_VER_NONE;
    // Compare the Measured ADC count with range specified in the HW version table
    for(temp = 0; temp < HW_VER_COUNT; temp++)
    {
        if((AdcCount >= (Handle_HWVersionTable[temp].AdcCountLoThreshold)) &&
           (AdcCount <= (Handle_HWVersionTable[temp].AdcCountHiThreshold)))
        {
          Version = Handle_HWVersionTable[temp].version;
          break;
        }
    }
    return Version;
}

/* ========================================================================== */
/**
 * \brief   Update the parameters in Handle Flash
 *
 * \details Write Handle Flash memory with the local data
 *
 * \param   < None >
 *
 * \return  AM_STATUS - Function return Status
 *
 * ========================================================================== */
static AM_STATUS HandleUpdateFlashData(void)
{
    AM_STATUS Status;
    uint16_t DataSize;
    Status = AM_STATUS_OK;

    do
    {
        // Erase the ACTIVE_METADATA_START area
        if ((L2_FlashEraseSector(HANDLE_FLASHDATA_START, HANDLE_FLASHDATA_SECTORSIZE)))
        {
            Log (DBG,"HandleFlashData FlashEraseSector failed");
            Status = AM_STATUS_ERROR;
            break;
        }

        // care is taken at Declaration that the FlashData size is in Multiple of 8
        DataSize = sizeof (HandleInterface.FlashData);

        if ((L2_FlashWrite ((uint32_t)HANDLE_FLASHDATA_START, DataSize, (uint32_t)&HandleInterface.FlashData)))
        {
            Log (DBG,"HandleFlashData FlashProgramPhrase failed");
            Status = AM_STATUS_ERROR;
            break;
        }
    } while (false);
   return Status;
}

/* ========================================================================== */
/**
 * \brief   Read the parameters in Handle Flash
 *
 * \details Read Handle data from Flash memory location to the Ram structure
 *
 * \param   <none>
 *
 * \return  AM_STATUS - Function return Status
 *
 * ========================================================================== */
static AM_STATUS HandleReadFlashData(void)
{
    AM_STATUS Status;
    Status = AM_STATUS_OK;

    HandleFlashParameters *pFlashData = (HandleFlashParameters *)HANDLE_FLASHDATA_START;
    memcpy(&HandleInterface.FlashData, pFlashData, sizeof (HandleFlashParameters));

    return Status;
}
/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Set the device id
 *
 * \details Set the Handle unique 1-wire identifier. The function also reads the
 *          Device EEPROM and stores the data in Handle Interface structure
 *
 * \param   DeviceAddress - 1-Wire device address
 * \param   *pData - Data pointer to 1-Wire Eep Data
 *
 * \return  None
 *
 * ========================================================================== */
void HandleSetDeviceId(DEVICE_UNIQUE_ID DeviceAddress, uint8_t *pData)
{
    /*Set device unique address*/
    HandleAddress = DeviceAddress;
    HandleInterface.Update = &HandleEepUpdate;
    HandleInterface.SaveFlashData = &HandleUpdateFlashData;
    HandleInterface.ReadFlashData = &HandleReadFlashData;

    HandleInterface.ReadFlashData();

    /* Update local data repository. User is allowed update all attributes.*/
    memcpy((uint8_t*)&HandleInterface.Data, &pData[0], ONEWIRE_MEMORY_TOTAL_SIZE);
    HandleInterface.Status = AM_STATUS_OK;
    Handle_UpdateHWVersion();
}

/* ========================================================================== */
/**
 * \brief   Check for startup errors in handle
 *
 * \details Checks handle fire and procedure count. Any errors found are logged
 *          into Fault Handler structure
 *
 * \param   < None >
 *
 * \return  none
 *
 * ========================================================================== */
void CheckHandleStartupErrors(void)
{
    if (HandleInterface.Data.ProcedureLimit >= HandleInterface.Data.ProcedureCount)
    {
        HandleData.handleRemainingProceduresCount = HandleInterface.Data.ProcedureLimit - HandleInterface.Data.ProcedureCount;
    }
    else
    {
        HandleData.handleRemainingProceduresCount = 0;
    }

    if (HandleData.handleRemainingProceduresCount == 0)
    {
        FaultHandlerSetFault(HANDLE_EOL_ZEROPROCEDURECOUNT, SET_ERROR);
    }

    if (HandleInterface.Data.FireLimit >= HandleInterface.Data.FireCount)
    {
        HandleData.handleRemainingFireCount = HandleInterface.Data.FireLimit - HandleInterface.Data.FireCount;
    }
    else
    {
        HandleData.handleRemainingFireCount = 0;
    }

    if (HandleData.handleRemainingFireCount == 0)
    {
        FaultHandlerSetFault(HANDLE_EOL_ZEROFIRINGCOUNTER, SET_ERROR);
    }
    /* Perform Handle Procedure, Fire Count Increment test only when Having greater than zero Handle Procedure, Fire Count */
    if ( (ZERO_VALUE != HandleData.handleRemainingProceduresCount) || (ZERO_VALUE != HandleData.handleRemainingFireCount) )
    {
        HandleProcedureFireCountTest();
    }
}

/* ========================================================================== */
/**
 * \brief   API to get the Handle interface
 *
 * \param   < None >
 *
 * \return  AM_HANDLE_IF - Pointer to Handle interface
 *
 * ========================================================================== */
AM_HANDLE_IF* HandleGetIF(void)
{
    return &HandleInterface;
}

/* ========================================================================== */
/**
 * \brief   API to get the Handle Onewire ID
 *
 * \param   < None >
 *
 * \return  AM_HANDLE_IF - Pointer to Handle interface
 *
 * ========================================================================== */
DEVICE_UNIQUE_ID HandleGetAddress(void)
{    
    return HandleAddress;
}

/* ========================================================================== */
/**
 * \brief   Performs Handle Procedure Count Test durin Startup
 *
 * \details This function performs Handle Procedure and Fire Count test if it is with in the limits
 *          Increments Procedure and Fire Counts and write's to Handle Memory and Read's back the latest value.
 *          Checks whether Procedure and Fire Counts are incremented or not.
 *          On successful test, provides status and writes back the original Procedure and Fire Counts to Handle
 *          On Failure provides status, play fault tone and display Handle Error Screen
 *          Req: 349976, 349980, 349986, 349988, 349974, 349978, 349982, 349984
 *
 * \param   <None>
 *
 * \return  <None>
 *
 * ========================================================================== */
void HandleProcedureFireCountTest(void)
{
    bool Success;
    uint16_t HandleProcCount;            /* Handle Procedure Count */
    uint16_t HandleFireCount;            /* Handle Fire Count */
    uint8_t ReadData[OW_MEMORY_TOTAL_SIZE];   /* EEPROM read data */
    OW_EEP_STATUS OneWireEepStatus;
    MemoryLayoutHandle_Ver2 *pHandleOneWireMemFormat;  /* Pointer to Handle one wire memory format */

    Success = false;
    HandleProcCount = 0;
    HandleFireCount = 0;
    
    do
    {
        OneWireEepStatus = L3_OneWireEepromRead(HandleAddress, OW_EEPROM_PAGE_NUM, ReadData);
        OneWireEepStatus = L3_OneWireEepromRead(HandleAddress, OW_EEPROM_PAGE_NUM2, &ReadData[OW_EEPROM_PAGE_OFFSET]);

        if (OW_EEP_STATUS_OK != OneWireEepStatus)
        {
            Log(DBG, "Error in EEPROM Read during Procedure Fire Count Test");
            break;
        }

        pHandleOneWireMemFormat = (MemoryLayoutHandle_Ver2*)ReadData;

        HandleProcCount = pHandleOneWireMemFormat->ProcedureCount;
        HandleFireCount = pHandleOneWireMemFormat->FireCount;

        /* Increment Procedure and Fire Count */
        pHandleOneWireMemFormat->ProcedureCount++;
        pHandleOneWireMemFormat->FireCount++;

        /* Calculate CRC and write back */
        pHandleOneWireMemFormat->Crc = CRC16(0,ReadData,(size_t)(OW_MEMORY_TOTAL_SIZE-2));
        OneWireEepStatus = L3_OneWireEepromWrite(HandleAddress, OW_EEPROM_PAGE_NUM, ReadData);
        OneWireEepStatus = L3_OneWireEepromWrite(HandleAddress, OW_EEPROM_PAGE_NUM2, &ReadData[OW_EEPROM_PAGE_OFFSET]);
        if (OW_EEP_STATUS_OK != OneWireEepStatus)
        {
            Log(DBG, "Error in EEPROM Write during Procedure Fire Count Test");
            break;
        }

        OneWireEepStatus = L3_OneWireEepromRead(HandleAddress, OW_EEPROM_PAGE_NUM, ReadData);
        if (OW_EEP_STATUS_OK != OneWireEepStatus)
        {
            Log(DBG, "Error in EEPROM Read during Procedure Fire Count Test");
            break;
        }

        pHandleOneWireMemFormat = (MemoryLayoutHandle_Ver2*)ReadData;
		
        TM_Hook (HOOK_PROCEDURE_FIRE_CNT_SIMULATE, pHandleOneWireMemFormat);
        
        /* Verify with Incremented Procedure/fire Count Value */
        if ( ++HandleProcCount != pHandleOneWireMemFormat->ProcedureCount || 
            ++HandleFireCount != pHandleOneWireMemFormat->FireCount)
        {
            Log(DBG, "Handle Procedure/Fire Count Test Failed");
            break;
        }

        /* Decrement and write back the original Procedure and Fire Counts */
        pHandleOneWireMemFormat->ProcedureCount--;
        pHandleOneWireMemFormat->FireCount--;
        pHandleOneWireMemFormat->Crc = CRC16(0,ReadData,(size_t)(OW_MEMORY_TOTAL_SIZE-2));
        OneWireEepStatus = L3_OneWireEepromWrite(HandleAddress, OW_EEPROM_PAGE_NUM, ReadData);
        OneWireEepStatus = L3_OneWireEepromWrite(HandleAddress, OW_EEPROM_PAGE_NUM2, &ReadData[OW_EEPROM_PAGE_OFFSET]);
        if (OW_EEP_STATUS_OK != OneWireEepStatus)
        {
            Log(DBG, "Error in EEPROM Write during Procedure Fire Count Test");
            break;
        }

        OneWireEepStatus = L3_OneWireEepromRead(HandleAddress, OW_EEPROM_PAGE_NUM, ReadData);
        if (OW_EEP_STATUS_OK != OneWireEepStatus)
        {
            Log(DBG, "Error in EEPROM Read during Procedure Fire Count Test");
            break;
        }

        pHandleOneWireMemFormat = (MemoryLayoutHandle_Ver2*)ReadData;

        if ( (--HandleProcCount != pHandleOneWireMemFormat->ProcedureCount) || 
            (--HandleFireCount != pHandleOneWireMemFormat->FireCount) )
        {
            Log(DBG, "Handle Procedure/Fire Count Test Failed");
            break;
        }
        Success = true;
        Log(REQ, "Handle Procedure/Fire Count Test Passed");
    } while (false);
    /* Check for Procedure or Fire Count Test Status */
    if (!Success)
    {
        /* Handle Procedure Count or fire count test failed, Log Error */
        FaultHandlerSetFault(HANDLE_PROCEDURE_FIRE_COUNT_TEST_FAIL,SET_ERROR);
    }
}
/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

