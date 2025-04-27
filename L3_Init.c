#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_Init
 * \{
 * \brief   Layer 3 Initialization function
 *
 * \details This file contains layer 3 initialization function which invokes
 *          all relevant module initializaiton functions.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_Init.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_Init.h"
#include "L3_DispPort.h"
#include "L3_I2c.h"
#include "L3_GpioCtrl.h"
#include "L3_Spi.h"
#include "L3_Fpga.h"
#include "L3_OneWireController.h"
#include "L3_OneWireRtc.h"
#include "L3_Battery.h"
#include "L3_Motor.h"
//#include "L3_Wlan.h"  

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
  
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Layer 3 initialization function
 *
 * \details Initializes Layer 3 functions and module
 *
 * \note    Initialize All L3 components listed below
 *
 *          One Wire Controller,
 *          FPGA Controller,
 *          GPIO Controller,
 *          GUI Controller,
 *          File System Controller,
 *          Serial Controller,
 *          SMBus Controller,
 *          IR Receiver, etc.
 *
 * \param   < None >
 *
 * \return  bool - Status - L3 Initialization Status, false if no errors.
 *
 * ========================================================================== */
bool L3_Init(void)
{
    bool Status;    // Function return status
    /* Initialize L3 Module in desired order */
    Status = true;  // Default to error

    Status =  (I2C_STATUS_SUCCESS != L3_I2cInit());
    Status = (SPI_STATUS_OK != L3_SpiInit())          || Status;
    Status = (GPIO_STATUS_OK != L3_GpioCtrlInit())    || Status;
    Status = (DISP_PORT_STATUS_OK != L3_DispInit())   || Status;
    Status = (BATTERY_STATUS_OK != L3_BatteryInit())  || Status;

    /// \\todo 02-03-2022 KIA : WiFi disabled for existing WiFi module. Revisit when new module is available.
    // Status = (WLAN_STATUS_OK!= L3_WlanInit()) || Status;

    Status =  (ONEWIRE_STATUS_OK != L3_OneWireInit()) || Status;

    L3_OneWireEnable(true); // Initialize for RTC    

    Status = (L3_FpgaInit())                     || Status;
    Status = (MOTOR_STATUS_OK != L3_MotorInit()) || Status;
    Status = (BATT_RTC_STATUS_OK != L3_BatteryRtcInit()) || Status;
    return Status;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
 }
#endif

