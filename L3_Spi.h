#ifndef L3_SPI_H
#define L3_SPI_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_SPI
 * \{
 * \brief   Public interface for SPI control routines
 *
 * \details This file has all SPI related symbolic constants and function prototypes
 *          This implementation uses Layer 2 SPI interface and implements additional
 *          logic around such as IPC protection, device specific clock switching
 *
 * \copyright  Copyright 2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * \file  L3_Spi.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         ///< Import common definitions such as types, etc.
#include "L2_Spi.h"         ///< Import all L2 SPI declarations
#include "L2_SpiCommon.h"   ///< Import Common definitions for SPI

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/// Available SPI Bus States list
typedef enum
{
    SPI_STATE_DISABLED,                  ///< Disabled
    SPI_STATE_ENABLED,                   ///< Enabled
    SPI_STATE_SLEEP                      ///< Sleep
}SPI_STATE;

/// Available SPI Devices IDs list
typedef enum
{
    SPI_DEVICE_CHARGER,                   ///< Charger
    SPI_DEVICE_ACCELEROMETER,             ///< Accelerometer
    SPI_DEVICE_COUNT                      ///< Last
}SPI_DEVICE; 

/// SPI Config Structure
typedef struct
{
    SPI_DEVICE Device;                   ///< Device ID
    SPI_PORT PortId;
    SPI_CHANNEL ChannelId;
    SPI_STATE  State;                    ///< Current Device State
}SPIConfig;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/


/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/


/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern SPI_STATUS L3_SpiInit(void);
extern SPI_STATUS L3_SpiTransfer(SPI_DEVICE Device, uint8_t *pDataOut, uint8_t OutCount, uint8_t *pDataIn, uint8_t InCount);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_SPI_H */
