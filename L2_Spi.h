#ifndef L2_SPI_H
#define L2_SPI_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif
/* ========================================================================== */
/**
 * \addtogroup L2_Spi
 * \{
 * \brief Interface for SPI functions.
 *
 * \details This module defines symbolic constants as well as function prototypes.
 *
 * \copyright  Copyright 2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * \file  L2_Spi.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "L2_SpiCommon.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/*****************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/// List of all available SPI channels here
typedef enum
{
    SPI_PORT_ZERO,                  ///< Used for FPGA
    SPI_PORT_ONE,                   ///< Not currently used
    SPI_PORT_TWO,                   ///< Used for Accelerometer, Charger
    SPI_PORT_COUNT                  ///< Identifies channel count (last channel)
} SPI_PORT;

/// List of all available SPI channels here
typedef enum
{
    SPI_CHANNEL0,      ///< Used Charger
    SPI_CHANNEL1,      ///< Used for Accelerometer, Charger
    SPI_CHANNEL2,      ///< Unused
    SPI_CHANNEL3,      ///< Unused
    SPI_CHANNEL4,      ///< Unused
    SPI_CHANNEL5,      ///< Unused
    SPI_CHANNEL_COUNT  ///< Identifies channel count
} SPI_CHANNEL;

/// List SPI peripheral power states. Refer to K20 data sheet for more details
typedef enum
{
    SPI_POWER_STATE_DISABLE,
    SPI_POWER_STATE_STOP,
    SPI_POWER_STATE_DOZE,
    SPI_POWER_STATE_ON,
    SPI_POWER_STATE_COUNT
} SPI_POWER_STATE;

/// SPI Frame size
typedef enum
{
    SPI_FRAME_SIZE8 = 1,
    SPI_FRAME_SIZE16 = 2,
    SPI_FRAME_SIZE_COUNT
} SPI_FRAME_SIZE;

typedef void (*SPI_CALLBACK_HNDLR)( void );  ///< SPI callback handler function pointer type

///  SPI Data Input Output Structure.
typedef struct
{
    SPI_PORT  SpiPort;         ///< SPI Channel (0 for FPGA, 2 for Accelerometer or Charger)
    uint8_t     *pSpiTxData;         ///< Pointer to transmit data array
    uint8_t     *pSpiRxData;         ///< Pointer to receive data array
    uint16_t     Nbytes;             ///< Number of bytes to be received as response (for TX_RX)
    SPI_CALLBACK_HNDLR  pCallback;   ///< The callback function to handle the response
} SPIIO;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern SPI_STATUS L2_SpiInit(void);
extern uint16_t L2_SpiTransfer(SPI_PORT Port, SPI_CHANNEL Channel, uint16_t Data, bool LastTxfer);
extern SPI_STATUS L2_SpiEnable(SPI_PORT Port, bool Enable);
extern SPI_STATUS L2_Spi0TxPacket(bool firstPkt, uint8_t numBytes, uint8_t *pPktBytes);
extern SPI_STATUS L2_SpiDataIO(SPIIO *pDataIO);
extern SPI_STATUS L2_SpiSetup(SPI_PORT Port, SPI_CHANNEL Channel);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L2_SPI_H */
