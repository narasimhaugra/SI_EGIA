#ifndef CRC_H
#define CRC_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup CRC
 * \{
 * \brief Public interface for CRC routines
 *
 * \details This header file provides the public interface for the CRC module,
 *          including function prototypes.
 *
 * \copyright  Copyright 2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * \file  Crc.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define DoSMBusCRC8(crc8, value)   SMBusCrcTable[crc8 ^ value]
/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/
extern const uint8_t SMBusCrcTable[256];
/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern uint32_t CRC32(uint32_t crc, const uint8_t *pBuf, size_t size);
extern uint16_t CRC16(uint16_t crc16In, uint8_t *pBuf, size_t size);
extern uint8_t CRC8(uint8_t crc, const uint8_t *pBuf, size_t size);
extern uint16_t DoCRC16(uint16_t crc16In, uint16_t data);
extern uint8_t  DoCRC8(uint8_t crc8, uint8_t value);
extern uint16_t SlowCRC16(uint16_t sum, uint8_t *pBuf, uint16_t len);
//extern uint8_t DoSMBusCRC8(uint8_t crc8, uint8_t value);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* CRC_H */

/* End of file */


