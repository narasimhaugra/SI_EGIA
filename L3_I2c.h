#ifndef L3_I2C_H
#define L3_I2C_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_I2c
 * \{
 * \brief   Public interface for I2C control routines
 *
 * \details This file has all I2C related symbolic constants and function prototypes
 *          This implementation uses Layer 2 I2C interface and implements additional
 *          logic around such as IPC protection, device specific clock switching.
 *
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_I2c.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"
#include "L2_I2c.h"    /* Import all L2 I2C declarations */

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define L3_I2C_Status() L2_I2cStatus()  /// I2C status check, redirected to L2 call

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

extern I2C_STATUS L3_I2cClaim(void);
extern I2C_STATUS L3_I2cRelease(void);

extern I2C_STATUS L3_I2cInit(void);
extern I2C_STATUS L3_I2cConfig (I2CControl *pControl);
extern I2C_STATUS L3_I2cWrite (I2CDataPacket *pPacket);
extern I2C_STATUS L3_I2cRead(I2CDataPacket *pPacket);
extern I2C_STATUS L3_I2cBurstRead(I2CDataPacket *pPacket);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_I2C_H */

/* End of file */
