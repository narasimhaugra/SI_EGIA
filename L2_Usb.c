#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Usb
 * \{
 * \brief   Functions related to USB which are used for communication with MCP.
 *
 * \sa      μC/ USB Device User’s Manual V4.01.01
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_Usb.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L2_Usb.h"
#include "uC-USB/Source/usbd_core.h"
#include "uC-USB/Cfg/usbd_dev_cfg.h"
#include "uC-USB/Drivers/usbd_bsp_kinetis_kxx.h"
#include "uC-USB/Class/CDC/ACM/usbd_acm_serial.h"
#include "TestManager.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
extern USBD_DRV  *USBD_DrvPtr;
const char USBEvent[5][11] = {"RESET     ",  ///< Strings for Log messages
                              "CONNECT   ",  // 10 characters plus NULL
                              "DISCONNECT",
                              "SUSPEND   ",
                              "RESUME    "};

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER           (LOG_GROUP_USB)       ///< Log Group Identifier
#define SIGNIA_VENDOR_ID               (0x1264u)            /*! Vendor  ID */
#define SIGNIA_PROD_ID                 (0x0500u)            /*! Product ID (CDC) */
#define SIGNIA_DEV_REL_NUM             (0x0100u)            /*! Device release number */
#define SIGNIA_MAX_POWER               (100u)               /*! Maximum power */

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static uint8_t DevNbr = 0;                              ///< USB device number.
static uint8_t SubClassNbr = 0;                         ///< USB sub class number.
static bool UsbInitialized = false;                     ///< Boolean to indicate the Usb initialization.
static USB_EVENT_HNDLR pUsbEventCB = NULL;              ///< Callback function used to notify the upper layers on the event occured.

/******************************************************************************/
/*                             Global Function Prototype(s)                    */
/******************************************************************************/
extern void USBD_DrvISR_Handler(USBD_DRV *p_drv);

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   This is the Bus-reset event callback function.
 *
 * \param   DevNmbr - Device number.
 *
 * \return  None
 *
 * ==========================================================================*/
static void EventReset_Callback(uint8_t DevNmbr)
{
    Log(DEV, "USB reset callback");

    if ((NULL != pUsbEventCB))
    {
        pUsbEventCB(USB_EVENT_RESET);
    }
}

/* ========================================================================== */
/**
 * \brief   This is the Bus-suspend event callback function.
 *
 * \param   DevNmbr - Device number.
 *
 * \return  None
 *
 * ==========================================================================*/
static void EventSuspend_Callback(uint8_t DevNmbr)
{
    Log(DEV, "USB suspend callback");

    if ((NULL != pUsbEventCB))
    {
        pUsbEventCB(USB_EVENT_SUSPEND);
    }
}

/* ========================================================================== */
/**
 * \brief   This is the Bus-resume event callback function.
 *
 * \param   DevNmbr - Device number.
 *
 * \return  None
 *
 * ==========================================================================*/
static void EventResume_Callback(uint8_t DevNmbr)
{
    Log(DEV, "USB resume callback");

    if ((NULL != pUsbEventCB))
    {
        pUsbEventCB(USB_EVENT_RESUME);
    }
}

/* ========================================================================== */
/**
 * \brief   This is the device connection event callback function.
 * \note    This is supposed to be called when USB cable is attached, but the ATTACH
 *          bit in USBx_ISTAT which is used to detect an attach of a USB device is
 *          valid only when HOSTMODEEN is '1'. In this case the HOSTMODDEEN is '0'
 *          as we are acting as Device mode.
 *
 * \param   DevNmbr - Device number.
 *
 * \return  None
 *
 * ==========================================================================*/
static void EventConnect_Callback(uint8_t DevNmbr)
{
    Log(DEV, "USB connect callback");

    if ((NULL != pUsbEventCB))
    {
        pUsbEventCB(USB_EVENT_CONNECT);
    }
}

/* ========================================================================== */
/**
 * \brief   This is the device disconnect event callback function.
 * \note    This is supposed to be called when USB cable is detached, but the ATTACH
 *          bit in USBx_ISTAT which is used to detect an attach of a USB device is
 *          valid only when HOSTMODEEN is '1'. In this case the HOSTMODDEEN is '0'
 *          as we are acting as Device mode.

 * \param   DevNmbr - Device number.
 *
 * \return  None
 *
 * ==========================================================================*/
static void EventDisconnect_Callback(uint8_t DevNmbr)
{
    Log(DEV, "USB disconnect callback");

    if ((NULL != pUsbEventCB))
    {
        pUsbEventCB(USB_EVENT_DISCCONNECT);
    }
}

/* ========================================================================== */
/**
 * \brief   This is the configuration set event callback function.
 *
 * \param   DevNmbr - Device number.
 * \param   Cfgval - Active device configuration.
 *
 * \note    This callback is used to notify the event to upper layer.
 *
 * \return  None
 *
 * ==========================================================================*/
static void EventCfgset_Callback(uint8_t DevNmbr, uint8_t Cfgval)
{
    Log(DEV, "USB config: set callback");
    if ((NULL != pUsbEventCB))
    {
        pUsbEventCB(USB_EVENT_CONNECT);
    }
}

/* ========================================================================== */
/**
 * \brief   This is the configuration clear event callback function.
 *
 * \param   DevNmbr - Device number.
 * \param   Cfgval - Active device configuration.
 *
 * \note    This callback is used to notify the event to upper layer.
 *
 * \return  None
 *
 * ==========================================================================*/
static void EventCfgclear_Callback(uint8_t DevNmbr, uint8_t Cfgval)
{
    Log(DEV, "USB config: clear callback");
    if ((NULL != pUsbEventCB))
    {
        pUsbEventCB(USB_EVENT_DISCCONNECT);
    }
}

/* ========================================================================== */
/**
 * \brief   This function initializes the USB Device and registers the callbacks for
 *          event notification.
 *
 * \param   pUsbDevCfg - Pointer to USB Device configuration supplied by the application..
 * \param   CfgFSNbr - Index of Full Speed configuration.
 *
 * \return  USB_STATUS - status of configuration update.
 *
 * ========================================================================== */
static USB_STATUS Usb_DevInit(USB_DEV_CFG *pUsbDevCfg, uint8_t *pCfgFSNbr)
{
    USB_STATUS Status = USB_STATUS_OK;       // Variable used to hold the return status.
    USBD_ERR     Error;
    USBD_DEV_CFG  USBDevCfg =
    {
        SIGNIA_VENDOR_ID,             /* Vendor  ID.               */
        SIGNIA_PROD_ID,               /* Product ID (CDC).         */
        SIGNIA_DEV_REL_NUM,           /* Device release number     */
        "Covidien",                   /* Manufacturer  string.     */
        "Covidien Gen2",              /* Product       string.     */
        0,                            /* Serial number string.     */
        USBD_LANG_ID_ENGLISH_US       /* String language ID.       */
    };

    USBD_BUS_FNCTS  App_USBD_BusFncts =
    {
        &EventReset_Callback,
        &EventSuspend_Callback,
        &EventResume_Callback,
        &EventCfgset_Callback,
        &EventCfgclear_Callback,
        &EventConnect_Callback,
        &EventDisconnect_Callback
    };

    do
    {
        USBD_Init(&Error);

        if (USBD_ERR_NONE != Error)
        {
            Log(ERR, "USB Device Init: Failed with Error = %d", Error);
            Status = USB_STATUS_FAIL;
            break;
        }

        /* Add USB device instance.                           */
        DevNbr = USBD_DevAdd(&USBDevCfg,
                             &App_USBD_BusFncts,
                             &USBD_DrvAPI_KINETIS,
                             &USBD_DrvCfg_kinetis,
                             &USBD_DrvBSP_kinetis,
                             &Error);

        if (USBD_ERR_NONE != Error)
        {
            Log(ERR, "USB Device Init: Add device Failed with Error = %d", Error);
            Status = USB_STATUS_FAIL;
            break;
        }

        USBD_DevSetSelfPwr(DevNbr, DEF_TRUE, &Error);              /* Device is self-powered.                          */

        *pCfgFSNbr = USBD_CfgAdd(DevNbr,                           /* Add Full Speed configuration.                                */
                                 USBD_DEV_ATTRIB_SELF_POWERED,
                                 SIGNIA_MAX_POWER,   
                                 USBD_DEV_SPD_FULL,
                                 "FullSpeed Config",
                                 &Error);

        if (USBD_ERR_NONE != Error)
        {
            Log(ERR, "USB Device Init: Add config Failed with Error = %d", Error);
            Status = USB_STATUS_FAIL;
            break;
        }

        Log(DEV, "USB Device: Initialized");

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   This function initializes the CDC class, which is a prerequisite for
 *          establishing USB connectivity.
 *
 * \param   pUsbClassCfg - Pointer to class configuration supplied by the application..
 * \param   CfgFSNbr - Index of Full Speed configuration.
 *
 * \return  USB_STATUS - status of configuration update.
 *
 * ========================================================================== */
static USB_STATUS Usb_ClassInit(USB_CLASS_CFG *pUsbClassCfg, uint8_t CfgFSNbr)
{
    USB_STATUS Status = USB_STATUS_OK;       ///< Variable used to hold the return status.
    USBD_ERR     Error;
    USBD_ACM_SERIAL_LINE_CODING SerialPortSettings;

    do
    {
        USBD_CDC_Init(&Error);

        if (USBD_ERR_NONE != Error)
        {
            Log(ERR, "USB CDC Init Failed with Error = %d", Error);
            Status = USB_STATUS_FAIL;
            break;
        }

        USBD_ACM_SerialInit(&Error);

        if (USBD_ERR_NONE != Error)
        {
            Log(ERR, "USB ACM Serial Init Failed with Error = %d", Error);
            Status = USB_STATUS_FAIL;
            break;
        }

        SubClassNbr = USBD_ACM_SerialAdd(pUsbClassCfg->LineStateInterval, &Error);

        if (USBD_ERR_NONE != Error)
        {
            Log(ERR, "USB ACM Serial Add Failed with Error = %d",Error);
            Status = USB_STATUS_FAIL;
            break;
        }

        USBD_ACM_SerialCfgAdd(SubClassNbr, DevNbr, CfgFSNbr, &Error);

        if (USBD_ERR_NONE != Error)
        {
            Log(ERR, "USB ACM Serial CfgAdd Failed with Error = %d",Error);
            Status = USB_STATUS_FAIL;
            break;
        }
        
        SerialPortSettings.BaudRate = pUsbClassCfg->BaudRate;
        SerialPortSettings.StopBits = pUsbClassCfg->StopBits;
        SerialPortSettings.DataBits = pUsbClassCfg->DataBits;
        SerialPortSettings.Parity   = USBD_ACM_SERIAL_PARITY_NONE;

        USBD_ACM_SerialLineCodingSet (SubClassNbr,&SerialPortSettings,&Error);

        if (USBD_ERR_NONE != Error)
        {
            Log(ERR, "USB ACM SerialLineCodingSet Failed with Error = %d",Error);
            Status = USB_STATUS_FAIL;
            break;
        }

        Log(DEV, "USB Class: Initialized");

    } while (false);

    return Status;
}

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Function to to initializes and configures the USB port.
 *
 * \details This function configures the USB port with device configuraton and
 *          class configuration.
 *
 * \param   pUsbDevCfg - Pointer to USB device configuration supplied by the upper layer.
 * \param   pUsbClassCfg - Pointer to class configuration supplied by the application.
 *
 * \return  USB_STATUS - status
 *
 * ========================================================================== */
USB_STATUS L2_UsbInit(USB_DEV_CFG *pUsbDevCfg, USB_CLASS_CFG *pUsbClassCfg)
{
    USB_STATUS Status;
    uint8_t   CfgFSNbr;
    USBD_ERR     Error;

    Status = USB_STATUS_OK;

    do
    {
        if ((NULL == pUsbDevCfg) || (NULL == pUsbClassCfg))
        {
            /* Invalid configuration, return error */
            Log(ERR, "USB Init: Invalid Config");
            Status = USB_STATUS_INVALID_PARAM;
            break;
        }

        if ((NULL == pUsbDevCfg->pManufacturerStr) || (NULL == pUsbDevCfg->pProductStr) || (NULL == pUsbDevCfg->pSerialNbrStr))
        {
            /* Invalid configuration, return error */
            Log(ERR, "USB Init: Invalid Config");
            Status = USB_STATUS_INVALID_PARAM;
            break;
        }

        if (USB_STATUS_OK != Usb_DevInit(pUsbDevCfg,&CfgFSNbr))
        {
            Log(ERR, "USB Init: Class Init Failed");
            Status = USB_STATUS_FAIL;
            break;
        }

        if (USB_STATUS_OK != Usb_ClassInit(pUsbClassCfg,CfgFSNbr))
        {
            Log(ERR, "USB Init: Class Init Failed");
            Status = USB_STATUS_FAIL;
            break;
        }

        USBD_DevStart(DevNbr, &Error);

        if (USBD_ERR_NONE != Error)
        {
            Log(ERR, "Usb Init: Device Start Failed with Error = %d", Error);
            Status = USB_STATUS_FAIL;
            break;
        }

        pUsbEventCB = pUsbDevCfg->pHandler;

        UsbInitialized = true;

        Log(REQ, "L2_USB: Initialized");

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Function to transmit data via USB port.
 *
 * \param   pDataOut  - Pointer to the data to be transmitted.
 * \param   DataCount - Size of the data buffer in bytes.
 * \param   Timeout   - Timeout value in milliseconds. If the Timeout value is 0 then
 *                      this becomes indefinite blocking call.
 * \param   pSentCount - Pointer to Variable which holds the value of number of bytes
 *                       actually transmitted..
 *
 * \note    \todo Based on the upper layers usage if required we can use a single argument
 *                instead of two arguments DataCount and pSentCount.
 *
 * \return  USB_STATUS - status
 *
 * ========================================================================== */
USB_STATUS L2_UsbSend(uint8_t *pDataOut, uint16_t DataCount, uint16_t Timeout, uint16_t *pSentCount)
{
    USB_STATUS  Status = USB_STATUS_OK;
    USBD_ERR    Error;

    do
    {
        if ((NULL == pDataOut) || (0 == DataCount) || (NULL == pSentCount))
        {
            Log(ERR, "L2_USB Send: Invalid Param");
            Status = USB_STATUS_INVALID_PARAM;
            break;
        }

        if (!UsbInitialized)
        {
            Log(ERR, "L2_USB Send: USB not initialized");
            Status = USB_STATUS_INVALID_STATE;
            break;
        }

        TM_Hook(HOOK_USBTXSTART, NULL);
        *pSentCount = USBD_ACM_SerialTx(SubClassNbr,(CPU_INT08U *)pDataOut,DataCount,Timeout,&Error);
        (USBD_ERR_NONE == Error)? TM_Hook(HOOK_USBTXEND, &DataCount) : TM_Hook(HOOK_USBTXSTART, NULL);

        if ((USBD_ERR_INVALID_CLASS_STATE == Error) || (USBD_ERR_EP_NONE_AVAIL == Error) || (USBD_ERR_DEV_INVALID_STATE == Error))
        {
           Log(ERR, "L2_USB Send: Failed with Error = %d", Error);
           Status = USB_STATUS_INVALID_STATE;
           break;
        }

        if (USBD_ERR_OS_TIMEOUT == Error)
        {
            /// \todo 02/17/2022 SE commenting out to avoid load and smother execution of MCP. Has to revisit later
            //Log(ERR, "L2_USB Send: Timeout");
            Status = USB_STATUS_TIMEOUT;
            break;
        }

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Function to receive data via USB port.
 *
 * \param   pDataIn   - Pointer to the data buffer to store the received data.
 * \param   DataCount - Size of the data buffer to receive.
 * \param   Timeout   - Timeout value in milliseconds. If the Timeout value is 0 then
 *                      this becomes indefinite blocking call.
 * \param   pReceivedCount - Pointer to Variable which holds the value of number of bytes
 *                           actually received.
 *
 * \return  USB_STATUS - status
 *
 * ========================================================================== */
USB_STATUS L2_UsbReceive(uint8_t *pDataIn,  uint16_t DataCount, uint16_t Timeout, uint16_t *pReceivedCount)
{
    USB_STATUS Status = USB_STATUS_OK;
    USBD_ERR     Error;

    do
    {
        if ((NULL == pDataIn) || (0 == DataCount) || ((NULL == pReceivedCount)))
        {
            Log(ERR, "L2_USB Receive: Invalid Param");
            Status = USB_STATUS_INVALID_PARAM;
            break;
        }

        if (!UsbInitialized)
        {
            Log(ERR, "L2_USB Receive: USB not initialized");
            Status = USB_STATUS_INVALID_STATE;
            break;
        }

        TM_Hook(HOOK_USBRXSTART, NULL);
        *pReceivedCount = (uint16_t)USBD_ACM_SerialRx(SubClassNbr,(CPU_INT08U *)pDataIn,DataCount,Timeout,&Error);
        (USBD_ERR_NONE == Error) ? TM_Hook(HOOK_USBRXEND, pReceivedCount) : TM_Hook(HOOK_USBRXSTART, NULL);

        if ((USBD_ERR_INVALID_CLASS_STATE == Error) || (USBD_ERR_EP_NONE_AVAIL == Error) || (USBD_ERR_DEV_INVALID_STATE == Error))
        {
           Log(ERR, "L2_USB Receive: Failed with Error = %d", Error);
           Status = USB_STATUS_INVALID_STATE;
           break;
        }

        if (USBD_ERR_OS_TIMEOUT == Error)
        {
           Status = USB_STATUS_TIMEOUT;
           break;
        }

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   This function provides the p_drv pointer to the driver ISR Handler..
 *
 * \details This handler in turn calls the .USBD_DrvISR_Handler which is part of Micrium
 *          USB driver files. Function USBD_DrvISR_Handler will
 *
 * \return  none
 *
 * ========================================================================== */
void USB_ISR(void)
{
    OS_CPU_SR  cpu_sr;

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    USBD_DrvISR_Handler(USBD_DrvPtr);

    OSIntExit();
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
