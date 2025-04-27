#ifndef L3_SMBUS_H
#define L3_SMBUS_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_SMBus
 * \{
 * \brief   Public interface for the L3 SMBus Communications.
 *
 * \details This file contains the enumerations as well as the function
 *          prototypes for the L3 SMBus communications.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_SMBus.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define MAX_SMBUS_COMMAND_SIZE (64u)  ///< Number of bytes in byte cmd

#define SMBUS_BYTE_NUMBYTES    (1u)  ///< Number of bytes in byte cmd
#define SMBUS_WORD_NUMBYTES    (2u)  ///< Number of bytes in word cmd
#define SMBUS_RDWR32_NUMBYTES  (4u)  ///< Number of bytes in long cmd

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
///< SMBUS STATUS
typedef enum {                        ///< Return Status for SMBus functions
  SMBUS_NO_ERROR,
  SMBUS_READ_ERROR,                   ///< Error reading from Smart Device
  SMBUS_WRITE_ERROR,                  ///< Error writing to Smart Device
  SMBUS_UPDATE_ERROR,                 ///< Error reading after write
  SMBUS_CONFIG_ERROR,                 ///< Error configuring I2C interface
  SMBUS_BUSY_ERROR,                   ///< Error initializing I2C interface
  SMBUS_LAST                          ///< Last enumeration
}SMBUS_STATUS;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern SMBUS_STATUS L3_SMBusInit(uint16_t devAddr, uint16_t timeOut);
extern SMBUS_STATUS L3_SMBusQuickCommand(uint16_t devAddr, uint8_t devCmd, uint8_t devCmdSize, uint8_t *pOpData);

extern SMBUS_STATUS L3_SMBusReadByte(uint16_t devAddr, uint8_t devCmd, uint8_t *pOpData);
extern SMBUS_STATUS L3_SMBusWriteByte(uint16_t devAddr, uint8_t devCmd, uint8_t opData);

extern SMBUS_STATUS L3_SMBusReadWord(uint16_t devAddr, uint8_t devCmd, uint8_t *pOpData);
extern SMBUS_STATUS L3_SMBusWriteWord(uint16_t devAddr, uint8_t devCmd, uint16_t opData);

extern SMBUS_STATUS L3_SMBusRead32(uint16_t devAddr, uint8_t devCmd, uint8_t *pOpData);
extern SMBUS_STATUS L3_SMBusWrite32(uint16_t devAddr, uint8_t devCmd, uint32_t opData);

extern SMBUS_STATUS L3_SMBusReadBlock(uint16_t devAddr, uint8_t devCmd, uint8_t devCmdSize, uint8_t *pOpData);
extern SMBUS_STATUS L3_SMBusWriteBlock(uint16_t devAddr, uint8_t devCmd, uint8_t devCmdSize, uint8_t *pOpData);

extern void L3_SMBus_UpdatePECFlag(bool Status);
extern bool L3_SMBus_GetPECFlag(void);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_SMBUS_H */

/* End of file */
