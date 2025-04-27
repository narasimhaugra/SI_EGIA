#ifndef L2_SPICOMMON_H
#define L2_SPICOMMON_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Spi
 * \{
 * \brief   Public interface for SPI symbolic constants/enums.
 *
 * \details This file has Common SPI related symbolic constants for both L2 and L3 layer of SPI.
 *
 * \copyright  Copyright 2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * \file  L2_SpiCommon.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         ///< Import common definitions such as types, etc.

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

/// List all SPI function return values
typedef enum  {
    SPI_STATUS_OK,                     ///< Success
    SPI_STATUS_CONFIG_FAILED,          ///< Config failed
    SPI_STATUS_PARAM_INVALID,          ///< Invalid Parameter
    SPI_STATUS_UNINITIALIZED,          ///< Interface not initialized
    SPI_STATUS_TIMEOUT,                ///< Timeout
    SPI_STATUS_ERROR,                  ///< Fail
    SPI_STATUS_COUNT
}SPI_STATUS;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/**
* \}  <If using addtogroup above>
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L2_SPICOMMON_H */

/* End of file */
