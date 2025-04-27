#ifndef L2_I2C_H
#define L2_I2C_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_I2c
 * \{
 * \brief   Public interface for I2C control routines
 *
 * \details This file has all I2C related symbolic constants and function prototypes
 *          This implementation uses only I2C0 interface, hence all the hardware
 *          memory/registers addresses are associated with I2C0 interface.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_I2c.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define MAX_I2C_SLAVE   (4u)    // Number of slave device supported by the design.

/// \todo 03/16/2021 DAZ - For reference:
#define IO_EXP_ADR  (0x21)
#define DS2465_ADR  (0x18)
#define BAT_ADR     (0x0B)
#define FPGA_ADR    (0x40)

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum I2C_CLOCK          // I2C Clock option enumerations
{
    I2C_CLOCK_78K,              ///< Clock  78 KHz
    I2C_CLOCK_144K,             ///< Clock 144 KHz
    I2C_CLOCK_312K,             ///< Clock 312 KHz
    I2C_CLOCK_1M,               ///< Clock   1 MHz
    I2C_CLOCK_LAST              ///< End of clock enum
} I2C_CLOCK;

typedef enum I2C_STATE          // I2C power states
{
    I2C_STATE_ENA,              ///< I2C Enabled
    I2C_STATE_DIS,              ///< I2C Disabled
    I2C_STATE_SLEEP,            ///< I2C in sleep slate
    I2C_STATE_LAST              ///< End of I2C state enum
} I2C_STATE;

typedef enum I2C_ADDR_MODE      // I2C Device address size
{
    I2C_ADDR_MODE_7BIT,         ///< 7 bit device addressing mode
    I2C_ADDR_MODE_10BIT,        ///< 10 bit device addressing mode
    I2C_ADDR_MODE_LAST          ///< End of device addressing mode enum
} I2C_ADDR_MODE;

typedef enum I2C_STATUS         // Function return status
{
    I2C_STATUS_SUCCESS,             ///< Status OK
    I2C_STATUS_IDLE,                ///< Bus is Idle
    I2C_STATUS_FAIL,                ///< Status Fail
    I2C_STATUS_BUSY,                ///< Bus is busy
    I2C_STATUS_FAIL_CONFIG,         ///< Config failed
    I2C_STATUS_FAIL_INVALID_PARAM,  ///< Failed due to invalid parameter
    I2C_STATUS_FAIL_NO_RESPONSE,    ///< No response from device
    I2C_STATUS_FAIL_TIMEOUT,        ///< Request timed out
    I2C_STATUS_LAST                 ///< End of status enum
} I2C_STATUS;

typedef  void (*I2C_EVT_HNDLR)(I2C_STATUS Event);   // Callback function prototype

typedef struct I2CControl       // I2C bus configuration parameters
{
  I2C_CLOCK     Clock;          ///< I2C bus Clock
  I2C_STATE     State;          ///< Enabled, Disabled, Sleep.
  I2C_ADDR_MODE AddrMode;       ///< Slave device address size
  uint16_t      Timeout;        ///< transaction timeout in ticks
  uint16_t      Device;         ///< Slave device address
} I2CControl;

typedef struct I2CDataPacket    // I2C device communication parameters
{
  uint16_t      Address;        ///< Slave device address
  uint8_t      *pReg;           ///< Slave Register/Memory address
  uint8_t      nRegSize;        ///< Slave Register/Memory address size
  uint8_t      *pData;          ///< Data to be transferred
  uint8_t      nDataSize;       ///< Size of data to be transferred
  I2C_EVT_HNDLR pHandler;       ///< Callback function to handle events
} I2CDataPacket;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void L2_I2cInit(void);
extern I2C_STATUS L2_I2cConfig (I2CControl *pControl);
extern I2C_STATUS L2_I2cWrite(I2CDataPacket *pPacket);
extern I2C_STATUS L2_I2cRead(I2CDataPacket *pPacket);
extern I2C_STATUS L2_I2cBurstRead(I2CDataPacket *pPacket);
extern I2C_STATUS L2_I2cStatus(void);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L2_I2C_H */

