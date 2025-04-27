#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_OneWire
 * \{
 * \brief   One Wire Bus primitives
 *
 * \details This module supplies functions necessary for manipulating devices on
 *          the One Wire bus(es). Functions are provided to achieve the following
 *          necessary One Wire bus operations:
 *              - Initialize interface hardware
 *              - Select One Wire Bus to operate on
 *              - Scan selected bus to detect all connected devices
 *              - Select a specified device on a specified bus
 *              - Authenticate a selected device
 *              - Transfer data to/from selected device
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_OneWireTransport.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_OneWireNetwork.h"
#include "L3_OneWireTransport.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static OWSearchContext BusContext[ONEWIRE_BUS_COUNT];   ///< Saves bus context 

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static OWSearchContext *OwTransportGetContext(ONEWIRE_BUS Bus);

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Get bus specific context
 *
 * \details 1-Wire controller may need to switch bus during peripheral communication. 
 *          Hence storing context for next time use.
 *                              
 * \param   Bus - 1-Wire bus
 *                              
 * \return  OWSearchContext - pointer to 1-Wire bus context, NULL if invalid bus ID mentioned
 *
 * ========================================================================== */
static OWSearchContext *OwTransportGetContext(ONEWIRE_BUS Bus)
{
    uint8_t BusIdx;
    OWSearchContext *pCtx;

    pCtx = NULL;
    
    for (BusIdx = 0; BusIdx < ONEWIRE_BUS_COUNT; BusIdx++)
    {
        if (BusContext[BusIdx].Bus == Bus)
        {
            pCtx = &BusContext[BusIdx];
            break;
        }
    }
    return pCtx;
}

/* ========================================================================== */
/**
 * \brief   Reset stored context of all 1-Wire bus
 *
 * \details Sets all context parameters to default values
 *                              
 * \param   < None >
 *                              
 * \return  None
 *
 * ========================================================================== */
static void OwTransportResetContex(void)
{
    uint8_t BusIdx;
    OWSearchContext *pCtx;
    
    for (BusIdx = 0; BusIdx < ONEWIRE_BUS_COUNT; BusIdx++)
    {
        pCtx = &BusContext[BusIdx];
        pCtx->Bus = (ONEWIRE_BUS)BusIdx;
        pCtx->LastConflict = 0;
        pCtx->RomId = ONEWIRE_DEVICE_ID_INVALID; /* 64 bit invalid bus ID */
        pCtx->LastDevice = false;
        pCtx->ScanType = OW_SCAN_TYPE_FULL;
    }
    
    return;
}

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Initialize 1-Wire transport layer
 *
 * \details Call link layer initialize function which inturn initializes 
 *          I2C communication channel
 *                              
 * \param   < None >
 *                              
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwTransportInit(void)
{   
    ONEWIRE_STATUS Status;

    /* Initialize the Search context to defaults */
    OwTransportResetContex();
    
    Status = OwNetInit();    
    
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Scan the selected bus
 *
 * \details Using ROM SEARCH command find 1-Wire slaves on the selected bus.
 *          The caller allocates required memory for device list and specify the 
 *          list capacity in the function call. 
 *
 * \note    See Maxim Application Note 187 for search algorithm details.
 *
 * \param   Bus - Onewire Bus
 * \param   ScanType - Scan type, All or only devices with alarm .                      
 * \param   pDeviceList - pointer to Placeholder to copy new device found.                         
 * \param   pCount - pointer to Device count requested. Will be updated with actual devices found
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwTransportScan(ONEWIRE_BUS Bus, OW_SCAN_TYPE ScanType, ONEWIRE_DEVICE_ID *pDeviceList, uint8_t *pCount)
{
    ONEWIRE_STATUS  Status;
    OWSearchContext *Context;
    uint8_t DeviceFound;
    
    Status = ONEWIRE_STATUS_ERROR;  /* Default error */
    DeviceFound = 0;                /* Init device count */

    do
    {        
        if ((NULL == pDeviceList) || (0 == pCount) || (Bus >= ONEWIRE_BUS_COUNT))
        {
            Status = ONEWIRE_STATUS_PARAM_ERROR;
            break;
        }

        /* the pCount is now validated, so check the count value now */
        if (0 == *pCount)
        {
            Status = ONEWIRE_STATUS_PARAM_ERROR;
            break;
        }

        /* Clear the list */
        memset(pDeviceList, 0, sizeof(ONEWIRE_DEVICE_ID)* *pCount);
        
        /* Restored stored context for progressive scan */
        Context = OwTransportGetContext(Bus);
        if (NULL == Context)
        {   
            /* Couldn't get the context */
            Status = ONEWIRE_STATUS_ERROR;
            break; 
        }
        
        do
        {
            Status = OwNetSearch(Context);
            
            /* Bus Short - No Device on found */
            if ((ONEWIRE_STATUS_BUS_ERROR == Status) && (0 == Context->RomId))
            {
                Status = ONEWIRE_STATUS_BUS_ERROR;      
                break;
            }
            
            if (ONEWIRE_STATUS_OK != Status)
            {
                Status = ONEWIRE_STATUS_ERROR;      // Error in search
                break;
            }
            
            if (0 == Context->RomId)
            {
                /* No device found*/
                Status = ONEWIRE_STATUS_OK;  
                break;
            }
            
            /* Copy new device to the list */
            memcpy(pDeviceList, (uint8_t*)&Context->RomId, sizeof(Context->RomId)); 
            DeviceFound++;

            if (DeviceFound >= *pCount)
            {
                /* Let's copy only requested number of devices found to avoid memory overflow 
                   ignore additional device if any */
                   
                Status = ONEWIRE_STATUS_OK;  
                break;
            }
            
            pDeviceList++;  /* 1 Device update, Update pointer to store next find */
            
        } while ( (DeviceFound <*pCount) && (!Context->LastDevice) );

        *pCount = DeviceFound;  /* return device found count */

    } while (false);

    return Status;

}

/* ========================================================================== */
/**
 * \brief   Check if device still connected
 *
 * \details Checks device presence by issues select command and check if device
 *          responds.
 *
 * \param   pAddr - pointer to Device address 
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwTransportCheck(ONEWIRE_DEVICE_ID *pAddr)
{
    /* This function just cascades into Network layer function */
    return OwNetDeviceCheck(pAddr);
}

/* ========================================================================== */
/**
 * \brief   Send data over 1-Wire bus
 *
 * \details Sends data over 1-Wire bus.
 *          Invokes ROM SELECT command if device address is specified, otherwise 
 *          assumes device is already selected and just data is transferred.
 *
 * \param   pDevice - pointer to Device address
 * \param   pData - Pointer to data to send
 * \param   Len - Data size 
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwTransportSend(ONEWIRE_DEVICE_ID *pDevice, uint8_t *pData, uint16_t Len)
{
    ONEWIRE_STATUS Status;

    Status = ONEWIRE_STATUS_OK; // Initialize Status

    if (NULL != pDevice)
    {
        Status = OwNetCmdSelect(pDevice);        
        OSTimeDly(1);
    }

    if (ONEWIRE_STATUS_OK == Status)
    {
        Status = OwNetSend(pData, Len);
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Receive data from the device over 1-Wire bus
 *
 * \details Receive data from the device if the device is still connected
 *
 * \note    If either data pointer is NULL or the size is 0, then reset is 
 *          is issued to terminate the transfer.
 *
 * \param   pData - Pointer to data to be send 
 * \param   Len - Data size 
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwTransportReceive(uint8_t *pData, uint16_t Len)
{
    ONEWIRE_STATUS Status;

    Status = ONEWIRE_STATUS_PARAM_ERROR;

    /* Check if the caller passed a NULL packet */
    if ((NULL != pData) && (Len > 0))
    {
        Status = OwNetRecv(pData, Len);    
    }
    else
    {
        /* Caller specified NULL packet, terminate the transfer */
        Status = OwNetReset();
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Set 1-Wire bus speed
 *
 * \details Sets bus speed for 1-Wire master to communicate
 *
 * \param   Speed - 1-Wire bus speed
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwTransportSpeed(ONEWIRE_SPEED Speed)
{
    return OwNetSetSpeed(Speed);
}

/* ========================================================================== */
/**
 * \brief   Enable/Disable 1-Wire master
 *
 * \details Uses 1-Wire master power down feature to control 1-Wire enable/disable
 *
 * \param   Enable - 1-Wire master power state, enabled if 'true' otherwise disabled
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwTransportEnable(bool Enable)
{
   return OwNetEnable(Enable);
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

