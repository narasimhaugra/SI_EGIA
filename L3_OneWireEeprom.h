#ifndef L3_ONEWIREEEPROM_H
#define L3_ONEWIREEEPROM_H

#ifdef __cplusplus      /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_OneWire
 * \{
 * \brief       Public interface for One Wire Eeprom module.
 *
 * \details     Symbolic types and constants are included with the interface prototypes
 *
 * \copyright   2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file        L3_OneWireEeprom.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/*****************************************************************************/
#define OW_EEPROM_RDWR_MAX_RETRY (3u)   ///< Max retry count value.
#define OW_EEPROM_MEMORY_PAGE_SIZE      (32u)           ///< Memory Page Size.

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

typedef enum                        /// 1-Wire EEPROM status
{
    OW_EEP_STATUS_OK,               ///< 1 Wire EEPROM function status OK.
    OW_EEP_STATUS_DEVICE_NOT_FOUND, ///< 1 Wire EEPROM Device Not Found.
    OW_EEP_STATUS_PARAM_ERROR,      ///< Invalid Parameter.
    OW_EEP_STATUS_ACCESS_DENIED,    ///< Access to 1 Wire device denied.
    OW_EEP_STATUS_COMM_ERROR,       ///< 1 Wire EEPROM device communication error.
    OW_EEP_STATUS_TIMEOUT,          ///< 1 Wire EEPROM device communication timeout.
    OW_EEP_STATUS_ERROR,            ///< 1 Wire EEPROM Generic Error.
    OW_EEP_STATUS_LAST
} OW_EEP_STATUS;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern OW_EEP_STATUS L3_OneWireEepromWrite(ONEWIRE_DEVICE_ID Device, uint8_t Page, uint8_t *pData);
extern OW_EEP_STATUS L3_OneWireEepromRead(ONEWIRE_DEVICE_ID Device, uint8_t Page, uint8_t *pBuffer);
extern OW_EEP_STATUS L3_OneWireEepromErase(ONEWIRE_DEVICE_ID Device);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_ONEWIREEEPROM_H */
