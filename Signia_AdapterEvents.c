#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/// \todo 05/16/2022 David Zeichner: Many things in here aren't adapter events. In fact, functions
/// \todo 05/16/2022 David Zeichner: that have to do with events shouldn't be here as they are NOT called from the application.
/// \todo 05/16/2022 David Zeichner: File should be named Signia_Adapter.c (.h)


/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterEvents
 * \{
 *
 * \brief   Signia functions to publish Adapterm manager Events
 *
 * \details AdapterEvents is responsible for handling all Event published between
             the Signia Handle and the Adapter.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_AdapterEvents.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Signia_AdapterEvents.h"
#include "ActiveObject.h"
#include "FaultHandler.h"
#include "Screen_AdapterError.h"
#include "L4_DisplayManager.h"
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
extern AM_HANDLE_IF HandleInterface;

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER             (LOG_GROUP_ADAPTER)   /*! Log Group Identifier */
/*! One wire ID type from One wire device ID */
#define MAX_INDEX             (5u)
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static AM_ADAPTER_IF *AdapterHandle;

typedef enum                      /// PUBLISH DEVICE CONNECT/DISCONNECT SIGNAL
{
   PUBLISH_NONE,                 /// Publish State None
   PUBLISH_NEW_DEVICE,           /// Publish New Device
   PUBLISH_LOST_DEVICE,          /// Publish Lost Device
} PUBLISH_STATE;

/* Struct to handle physical device connect and disconnect signal */
typedef struct
{
    AM_DEVICE_INFO      *pData;         ///< Pointer to Device Data
    AM_EVENT            AmEvent;        ///< To hold the Event Occured/Received
    PUBLISH_STATE       Publish;      ///< Set after Publishing the event while all Connected Signal
    bool                Present;        ///< Set when device connect signal/event is received
    AM_DEVICE           Device;         ///< Device Enum for Reference
}PHY_DEVICE;                             ///< Device Handling Structure

/* Physical Device Structure Allocation */
static PHY_DEVICE PhyDeviceList[MAX_INDEX] = {
    {NULL, AM_EVENT_NONE, PUBLISH_NONE, false, AM_DEVICE_HANDLE},
    {NULL, AM_EVENT_NONE, PUBLISH_NONE, false, AM_DEVICE_CLAMSHELL},
    {NULL, AM_EVENT_NONE, PUBLISH_NONE, false, AM_DEVICE_ADAPTER},
    {NULL, AM_EVENT_NONE, PUBLISH_NONE, false, AM_DEVICE_RELOAD},
    {NULL, AM_EVENT_NONE, PUBLISH_NONE, false, AM_DEVICE_CARTRIDGE}
};

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static bool CheckDevicePresent(uint8_t DeviceIndex);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Get Device Present Status
 *
 * \details This function returns Device Present status.
 *          This function is used in case of 1-wire authentication failure
 *
 * \param   DeviceIndex - Index for the PhyDevList Struct check
 * \return  uint8_t   True-Device Present/ False -NO Device Present
 * \todo 1/24/2023 BS: check and update the function
 * ========================================================================== */
static bool CheckDevicePresent(uint8_t DeviceIndex)
{
    uint8_t TempDeviceIndex;
    bool IsPresent = false;

    for ( TempDeviceIndex = DeviceIndex+1; TempDeviceIndex< MAX_INDEX; TempDeviceIndex++)
    {
        if (PhyDeviceList[TempDeviceIndex].Present)
        {
            /* Device Present return true */
            IsPresent = true;
            break;
        }
    }
    return IsPresent;
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Public function interfacing the Adapter manager.
 *
 * \details This function receives the Event and Device details and publishes the event to the
 *          upper layers as per physical device connect/disconnect. if signal received not in physical device
 *          connect/disconnect order, this function makes sure the right order to publish them to Application.
 *          Uses the static helper functions in this file to publish
 *          Order - Clamshell/Adapter/Reload/Cartridge Connect events
 *          Reverse order for Disconnect events
 *
 * \param   AmEvent         - Connect / Disconnect event
 * \param   pDeviceData     - Pointer to Device specific data used to identify device id, device type
 *
 * \return  AM_STATUS       - Adapter Manager Status
 * \retval  AM_STATUS_OK    - No error in initilalizing
 * \retval  AM_STATUS_ERROR - Error in initializing
 *
 * ========================================================================== */
/// \todo 2022-05-13 David Zeichner: Why is this Signia_xxxx? This is never called from the application.
/// \todo 2022-05-13 David Zeichner: I would call this AdapterMgrPublish() & make it a static in the file in which it is invoked. (Adapter?)
AM_STATUS Signia_AdapterMgrEventPublish(AM_EVENT AmEvent, AM_DEVICE_INFO *pDeviceData)
{
    AM_STATUS Status;
    SIGNAL SignEventId;
    QEVENT_ADAPTER_MANAGER *pEvent;
    uint8_t List;
    uint8_t DeviceIndex;

    /** \todo 01/21/2022 DAZ - Using an ENUM to index into an array as done below is dangerous, as it makes the contents of the array
     *                         dependent on the ordering of the values in the ENUM. This should be implemented differently.
     *                         (Perhaps specify signal as an argument?)
     *                         The Device ENUM has been reordered to insure that the handle data is checked for before
     *                         issuing any device connection events that may require handle data. (ie Clamshell) This
     *                         required the reordering of the array below.
     */
    // Event lookup. New Device = 0, Lost Device = 1. \todo: Support additional events such as invalid device connected?
    // R_EMPTY_SIG (0) entries in array for devices that have not yet had signals assigned to them
    const SIGNAL DeviceEventLup[2][AM_DEVICE_COUNT] =
    {
        R_EMPTY_SIG, P_CLAMSHELL_REMOVED_SIG, P_ADAPTER_REMOVED_SIG, P_RELOAD_REMOVED_SIG, P_CARTRIDGE_REMOVED_SIG, R_EMPTY_SIG, \
        R_EMPTY_SIG, P_CLAMSHELL_CONNECTED_SIG, P_ADAPTER_CONNECTED_SIG, P_RELOAD_CONNECTED_SIG, P_CARTRIDGE_CONNECTED_SIG, R_EMPTY_SIG
    };

    // Start with a default status
    Status = AM_STATUS_ERROR;

    // If a valid device found then notify application, Check only Handle/Clamshell/Adpater/Reload/Cartridge
    // Device handlers are populated for Handle/Clamshell/Adpater/Reload/Cartridge only
    if (pDeviceData->Device < AM_DEVICE_BATTERY)
    {
        // Index order for the Clamshell and Adapter reversed for the Signal processing
        if (AM_EVENT_NEW_DEVICE == AmEvent)
        {
            PhyDeviceList[pDeviceData->Device].pData = pDeviceData;
            PhyDeviceList[pDeviceData->Device].Present = true;
            // To get the Connected Signal from DeviceEventLup[] when device connected
            List = true;
            // Skip publishing signal if no signal assigned for device type
            for (DeviceIndex=0; DeviceIndex < MAX_INDEX; DeviceIndex++)
            {
                if (!PhyDeviceList[DeviceIndex].Present)
                {
                    break;
                }
                if (PUBLISH_NONE == PhyDeviceList[DeviceIndex].Publish)
                {
                    PhyDeviceList[DeviceIndex].Publish = PUBLISH_NEW_DEVICE;
                    PhyDeviceList[DeviceIndex].AmEvent = AM_EVENT_NEW_DEVICE;

                    SignEventId = DeviceEventLup[List][DeviceIndex];
                    if ((SignEventId != R_EMPTY_SIG) && (PhyDeviceList[DeviceIndex].AmEvent == AM_EVENT_NEW_DEVICE))
                    {
                        pEvent = AO_EvtNew(SignEventId, sizeof(QEVENT_ADAPTER_MANAGER));
                        if(pEvent == NULL)
                        {
                            Log(DBG, "Signia Event allocation error - Connect");
                        }
                        else
                        {
                            /* Send Signal only Clamshell/Adapter/Reload/Cartridge */
                            pEvent->Device = PhyDeviceList[DeviceIndex].pData->Device;               // Device class
                            pEvent->DevAddr = PhyDeviceList[DeviceIndex].pData->DeviceUID;           // Device Unique ID
                            pEvent->pDeviceHandle = PhyDeviceList[DeviceIndex].pData->pDevHandle;
                            pEvent->Authentic = PhyDeviceList[DeviceIndex].pData->Authentic;         // device authentic boolean flag
                            pEvent->DeviceWriteTest = PhyDeviceList[DeviceIndex].pData->DeviceWriteTest; //1-Wire Device Write test status
                            AO_Publish(pEvent, NULL);
                            PhyDeviceList[DeviceIndex].AmEvent = AM_EVENT_NONE;
                            Log(DBG, "Signia Adapter Event: %d,  0x%X", SignEventId, PhyDeviceList[DeviceIndex].pData->DeviceUID);
                            Status = AM_STATUS_OK;
                        }
                    }
                }
            }
        }
        else    /* Device Removed */
        {

            PhyDeviceList[pDeviceData->Device].pData = pDeviceData;
            PhyDeviceList[pDeviceData->Device].Publish = PUBLISH_LOST_DEVICE;

            // To get the Disconnected Signal from DeviceEventLup[] when device removed
            List = false;
            /* Update the Device Data */
            // Skip publishing signal if no signal assigned for device type
            for (DeviceIndex = MAX_INDEX-1; DeviceIndex > 0; DeviceIndex--)
            {
                /* Check if Device Present as per physical device disconnection */
                if ((PhyDeviceList[DeviceIndex].Present) && (PUBLISH_LOST_DEVICE == PhyDeviceList[DeviceIndex].Publish)
                    && (!CheckDevicePresent(DeviceIndex)))
                {
                    PhyDeviceList[DeviceIndex].AmEvent = AM_EVENT_LOST_DEVICE;
                    SignEventId = DeviceEventLup[List][DeviceIndex];
                    if ((SignEventId != R_EMPTY_SIG) && (PhyDeviceList[DeviceIndex].AmEvent == AM_EVENT_LOST_DEVICE))
                    {
                        pEvent = AO_EvtNew(SignEventId, sizeof(QEVENT_ADAPTER_MANAGER));

                        if(pEvent == NULL)
                        {
                            Log(DBG, "Signia Event allocation error - Disconnect");
                        }
                        else
                        {
                            pEvent->Device = PhyDeviceList[DeviceIndex].pData->Device;               // Device class
                            pEvent->DevAddr = PhyDeviceList[DeviceIndex].pData->DeviceUID;           // Device Unique ID
                            pEvent->pDeviceHandle = PhyDeviceList[DeviceIndex].pData->pDevHandle;
                            pEvent->Authentic = PhyDeviceList[DeviceIndex].pData->Authentic;         // device authentic boolean flag
                            pEvent->DeviceWriteTest = PhyDeviceList[DeviceIndex].pData->DeviceWriteTest; //1-Wire Device Write test status
                            AO_Publish(pEvent, NULL);
                            PhyDeviceList[DeviceIndex].AmEvent = AM_EVENT_NONE;
                            PhyDeviceList[DeviceIndex].Publish = PUBLISH_NONE;
                            PhyDeviceList[DeviceIndex].Present = false;
                            Log(DBG, "Signia Adapter Event: %d,  0x%X", SignEventId, PhyDeviceList[DeviceIndex].pData->DeviceUID);
                            Status = AM_STATUS_OK;
                        }
                    }
                }
            }
        }
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief Request Adapter information via uart
 *
 * \details the Api posts the Command to the AdapterQ.
 *
 * \param   AdapCmd - Requested Adapter Command
 *
 * \param   Delayms - Delay to wait after receivng the response
 *
 * \return  SIGNIA_AM_EVENT_STATUS  - Adapter Manager Status
 * \retval  SIGNIA_AM_STATUS_OK     - No error in initilalizing
 * \retval  SIGNIA_AM_STATUS_ERROR  - Error in initializing
 *
 * ========================================================================== */
AM_STATUS Signia_AdapterRequestCmd(ADAPTER_COMMANDS AdapCmd, uint32_t Delayms)
{
    AM_STATUS Status;
    ADAPTER_COM_MSG AdapterComMsg;

    Status = AM_STATUS_ERROR;
    do
    {
        AdapterComMsg.Cmd = AdapCmd;
        AdapterComMsg.DelayInMsec = Delayms;
        if( AM_STATUS_OK != L4_AdapterComPostReq(AdapterComMsg) )
        {
            Log(DEV, "AdapterEvents: AdapterComQ full");
            break;
        }
        Status = AM_STATUS_OK;
    } while (false);

    return Status;

}

/* ========================================================================== */
/**
 * \brief   Function Provides API for checking Handle status.
 *
 * \details This funciton provides lets the upperlayer to check on the handle onwire status.
 *
 * \param   < None >
 *
 * \return  AM_DEVICE_STATUS - Device status
 * \retval  AM_DEVICE_CONNECTED     - Handle One Wire connected without error
 * \retval  AM_DEVICE_DISCONNECTED  - Handle One Wire not connected
 *
 * ========================================================================== */

/// \todo 2022-05-13 David Zeichner: Functions named _GetHandle... have nothing to do with adapter. Shouldn't be here.
AM_DEVICE_STATUS Signia_GetHandleStatus(void)
{
    AM_DEVICE_STATUS Status;
    AM_DEVICE_INFO DeviceInfo;

    Status = AM_DEVICE_DISCONNECTED;
    Signia_AdapterManagerGetInfo( AM_DEVICE_HANDLE , &DeviceInfo);
    if ( AM_DEVICE_STATE_ACTIVE == DeviceInfo.State)
    {
        Status = AM_DEVICE_CONNECTED;
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Function provides API to get the Handle Interface function.
 *
 * \details This function returns the Handle interface and can be used by upper layers to access Handle EEPROM Data.
 *
 * \param   < None >
 *
 * \return  AM_HANDLE_IF* - Pointer to the Handle interface
 * \retval  NULL if the Handle is invalid
 * \retval  pointer to Handle interface if Handle onewaire is valid
 *
 * ========================================================================== */
AM_HANDLE_IF* Signia_GetHandleIF(void)
{
    AM_DEVICE_INFO DeviceInfo;
    AM_HANDLE_IF *pHandle;

    pHandle = NULL;

    Signia_AdapterManagerGetInfo( AM_DEVICE_HANDLE , &DeviceInfo);
    if ( AM_DEVICE_STATE_ACTIVE == DeviceInfo.State)
    {
        pHandle = &HandleInterface;
    }
    return pHandle;
}

/* ========================================================================== */
/**
 * \brief   function to read strain gauge values.
 *
 * \details Reads latest strain gauge values
 *
 * \param   pSGData - Pointer to the Strain gauge buffer
 *
 * \return  AM_STATUS - status
 * \retval  AM_STATUS_OK     - operation successful
 * \retval  AM_STATUS_ERROR  - Null parameter passed
 *
 * ========================================================================== */
AM_STATUS Signia_GetStrainGauge(SG_FORCE *pSGData)
{
    AM_STATUS Status;
    Status = AM_STATUS_ERROR;

    do
    {
        if (NULL == pSGData)
        {
            break;
        }

        if (NULL == AdapterHandle)
        {
            break;
        }

        BREAK_IF (AdapterHandle->pGetStrainGaugeData(pSGData));
        Log(DBG, "Strain Gauge Value =  %x", pSGData->Current);

        Status = AM_STATUS_OK;

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Function provides API to get the Handle onewire address.
 *
 * \details This function returns the Handle device address and can be used by upper layers to access Handle onewire address.
 *
 * \param   < None >
 * \return  DEVICE_UNIQUE_ID - Device Address of the Handle interface
 * \retval  NULL if the Handle is not in active state
 * \retval  Handle onewire address if Handle is in active state
 *
 * ========================================================================== */
DEVICE_UNIQUE_ID Signia_GetHandleAddr(void)
{
    AM_DEVICE_INFO DeviceInfo;
    DEVICE_UNIQUE_ID HandleAddress;

    Signia_AdapterManagerGetInfo( AM_DEVICE_HANDLE , &DeviceInfo);
    if ( AM_DEVICE_STATE_ACTIVE == DeviceInfo.State)
    {
        HandleAddress = DeviceInfo.DeviceUID;
    }
    return HandleAddress;
}

/* ========================================================================== */
/**
 * \brief   Get Uart adapter type
 *
 * \details This function returns Adapter type connected to handle.
 *          This function is used in case of 1-wire authentication failure
 *
 * \param   < None >
 * \return  uint16_t  UartAdapterType
 * ========================================================================== */
uint16_t Signia_GetUartAdapterType(void)
{
    uint16_t AdapType;
    AM_STATUS Status;
    Status = AdapterGetType(&AdapType);
    if(AM_STATUS_OK != Status)
    {
        AdapType = DEVICE_ID_UNKNOWN;
    }
    return AdapType;
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

