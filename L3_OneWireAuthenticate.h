#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \brief   Public interface for One Wire Authentication module.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_OneWireAuthenticate.h
 *
 * ========================================================================== */

#ifndef L3_ONEWIREAUTHENTICATE_H
#define L3_ONEWIREAUTHENTICATE_H

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/*****************************************************************************/
#define ONEWIRE_CS_SUCCESS                      (0xAAu)         ///< CS value for Command Success Indicator.
#define ONEWIRE_SLV_ROM_CMD_RESUME              (0xA5u)         ///< OneWire command to resume Slave.
#define ONEWIRE_SLV_CMD_READ_WRITE_SCRATCH      (0x0Fu)         ///< OneWire command Read and Write to Scratchpad registers.
#define ONEWIRE_SLV_CMD_READ_PAGE_MAC           (0xA5u)         ///< OneWire command to read MAC from Slave
#define ONEWIRE_SLV_CMD_READ_STATUS             (0xAAu)         ///< OneWire command to read Status from Slave.
#define ONEWIRE_MST_CMD_COMPUTE_S_SECRET        (0x4Bu)         ///< OneWire command to compute Slave secret in Master
#define ONEWIRE_MST_CMD_COMPUTE_S_AUTHEN_MAC    (0x3Cu)         ///< OneWire command to compute Slave autentication MAC in Master.
#define ONEWIRE_MST_CONFIG_REG                  (0x67u)         ///< OneWire register to configure Master controller.
#define ONEWIRE_MST_COMMAND_REG                 (0x60u)         ///< OneWire register to command Master controller.
#define ONEWIRE_CMD_PACKET_SIZE                 (2u)            ///< OneWire EEPROM command payload size .
#define ONEWIRE_STATUS_CMD_PACKET_SIZE          (8u)            ///< OneWire Status command payload size .
#define ONEWIRE_CRC_BUF_SIZE                    (2u)            ///< CRC Size in bytes
#define ONEWIRE_EEPROM_NUM_PAGES                (2u)            ///< Number of Pages.
#define ONEWIRE_EEPROM_CMD_READ                 (0xF0u)         ///< 1 Wire EEPROM read command.
#define ONEWIRE_EEPROM_CRC_CONST_VAL            (0xB001u)       ///< CRC value of the Message transmitted and transmitted CRC bytes.
#define ONEWIRE_MID_CRC_CONST_VAL               (0x80D7u)       ///< CRC value of the Message transmitted and transmitted CRC bytes.
#define ONEWIRE_CMD_MEMORY_PAGE_SIZE            (32u)           ///< Memory Page Size.
#define ONEWIRE_MASTER_DS2465_ADDRESS           (0x18u)         ///< OneWire Master controller address.
#define ONEWIRE_EEPROM_TXFER_WAIT               (12u)           ///< Delay value used to wait after sending the Release command.
#define CMD_PARAM_SCRATCHPAD_WRITE              (0x00u)         ///< OneWire command paramenter to write to scratchpad. used along with ONEWIRE_SLV_CMD_READ_WRITE_SCRATCH.
#define CMD_PARAM_SCRATCHPAD_READ               (0x0Fu)         ///< OneWire command paramenter to read from scratchpad. used along with ONEWIRE_SLV_CMD_READ_WRITE_SCRATCH.
#define CMD_PARAM_STATUS_PBI                    (0xE0u)         ///< OneWire command paramenter to read personality byte from status command
#define CMD_PARAM_COMPUTE_S_AUTHEN_MAC          (0xBFu)         ///< COMPUTE_S_AUTHEN_MAC Parameter
#define CMD_PARAM_COMPUTE_S_SECRET              (0x43u)         ///< COMPUTE_S_SECRET  Parameter
#define ONEWIRE_TCSHA_DELAY                     (0x4u)          ///< CSHA delay is 4ms (2 * Tchsa)
#define PACKETINDEX_0                           (0x0)           ///< Packet index to be processed in the transfer handler
#define PACKETINDEX_1                           (0x1u)          ///< Packet index to be processed in the transfer handler
#define PACKETINDEX_2                           (0x2u)          ///< Packet index to be processed in the transfer handler
#define ONEWIRE_MANUFACURER_ID_OFFSET0          (72u)           ///< Manufacturer ID offset for the data to be sent to slave scratchpad
#define ONEWIRE_MANUFACURER_ID_OFFSET1          (73u)           ///< Manufacturer ID offset for the data to be sent to slave scratchpad
#define ONEWIRE_PAGENO_OFFSET                   (74u)           ///< Page offset to be used in the Slave
#define STATUS_CMD_MANUFACURER_ID_OFFSET0       (4u)            ///< Manufacturer ID offset for the slave Status command
#define STATUS_CMD_MANUFACURER_ID_OFFSET1       (5u)            ///< Manufacturer ID offset for the slave Status command

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/


/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/


#endif /* L3_ONEWIREAUTHENTICATE_H */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif


