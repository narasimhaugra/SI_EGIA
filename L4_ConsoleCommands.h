#ifndef L4_CONSOLECOMMANDS_H
#define L4_CONSOLECOMMANDS_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif
/* ========================================================================== */
/**
 * \addtogroup L4_ConsoleManager
 * \{

 * \brief   Public interface for Console Manager.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_ConsoleCommands.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"                      ///< Import common definitions such as types, etc 
#include "Signia_CommManager.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
// Offsets for various fiels in the command frame
#define PCKT_SIZE_OFFSET          (1u)
#define COMMAND_OFFSET_8BIT       (2u)
#define COMMAND_OFFSET_16BIT      (3u)
#define DATA_OFFSET_8BIT          (3u)
#define DATA_OFFSET_16BIT         (4u)
#define DEV_ADDR_LENGTH           (8u)            ///< 1-Wire Device Address Length (bytes)

#define MAX_DATA_TRANSMIT_SIZE              (1010u)   /*! Max data size transmitted by console mgr*/
#define MAX_TIME_TO_WAIT_FOR_PACKET         (SEC_2)   /*! timeout for partial data */
#define LARGEST_PACKET_SIZE_16BIT           (1010u)   /*! max data buffer*/
#define MIN_PCKT_SIZE                       (3u)      /*! min packet size needed for processing*/
#define PCKT_START                          (0xAA)    /*! start of frame */
#define PCKT_OVERHEAD_16BIT                 (6u)      /*! Packet size not including the data */
#define PCKT_OVERHEAD_8BIT                  (4u)      /*! Packet size not including the data */
#define CMD_INDEX_OFFSET                    (2u)      /*! Command index offset in the received packet */
#define CMD_DATA_OFFSET                     (1u)      /*! Data index offset of the command */
#define CMD_PASSWORD_LEN                    (16u)     /*! Data array for the password command */
#define MAX_VALID_COMMANDS                  (0x4u)    /*! maximum number of valid commands in a data frame */

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/// An enumerated list of all the serial commands:
///    The command byte in the communications packet.Created from the file SerialCommands.def */
typedef enum
{
   SERIALCMD_UNKNOWN,
#define X(a) a,
#include "SerialCommands.def"
#undef X
   SERIALCMD_COUNT
} SERIAL_CMD;

typedef enum                           /// Enum used for various status in MCP authentication
{
    AUTHENTICATION_STATUS_UNKNOWN=0,    ///< default authentication status
    AUTHENTICATION_STATUS_STARTED,      ///< first time authentication command received
    AUTHENTICATION_STATUS_IN_PROGRESS,  ///< authentication underway
    AUTHENTICATION_STATUS_SUCCESS,      ///< MCP aurthentication success 
    AUTHENTICATION_STATUS_FAILED,       ///< MCP aurthentication failed 
    AUTHENTICATION_STATUS_COUNT         ///< MCP aurthentication status count 

}AUTHENTICATION_STATUS;

typedef  enum
{
    MSG_TYPE_8BIT = 0,
    MSG_TYPE_16BIT,
    MSG_TYPE_COUNT

} MSG_TYPE;

typedef enum               /// Enum used streaming data for type of variable 
{
    VAR_TYPE_UNKNOWN,       ///< Unknown 
    VAR_TYPE_BOOL,          ///< Bool Type 
    VAR_TYPE_INT8U,         ///< Unsigned Char 
    VAR_TYPE_INT8S,         ///< Signed Char 
    VAR_TYPE_INT16U,        ///< Unsigned integer 
    VAR_TYPE_INT16S,        ///< Signed integer 
    VAR_TYPE_INT32U,        ///< Unsigned long 
    VAR_TYPE_INT32S,        ///< Signed Long 
    VAR_TYPE_INT64U,        ///< Unsigned Long Long 
    VAR_TYPE_INT64S,        ///< Signed Long Long 
    VAR_TYPE_FP32,          ///< Float 32 
    VAR_TYPE_FP64,          ///< Float 64 
    VAR_TYPE_STRING,        ///< String 
    VAR_TYPE_ARRAY,         ///< Array 
    VAR_TYPE_ENUM,          ///< Enum 
    VAR_TYPE_TIMESTAMP,     ///< Time Stamp 
    VAR_TYPE_COUNT          ///< Count 
} VAR_TYPE;

// Keep this enum synchronized with the one in the PC code's Device.h.
enum DeviceTypes
{
  DEVICETYPE_UNKNOWN,
  DEVICETYPE_AUTOCLAVE,
  DEVICETYPE_ULTRA,
  DEVICETYPE_IDRIVE,
  DEVICETYPE_INVISIBLE_HAND,
  DEVICETYPE_BATTERY_SIM,
  DEVICETYPE_HALL_FIXTURE,
  DEVICETYPE_INVISIBLE_HAND_TARGET,
  DEVICETYPE_ACS_DEMO,
  DEVICETYPE_GEN2,
  DEVICETYPE_CV1,
  DEVICETYPE_MPV100,
  DEVICETYPE_GEN2_FIXTURE,

  DEVICETYPE_ANY_PIC18,
  DEVICETYPE_ANY_PIC24,
  DEVICETYPE_1_BAY,
  DEVICETYPE_4_BAY,
  DEVICETYPE_COUNT
};

typedef enum
{
  CONSOLE_PARAM_INVALID = 0,
  CONSOLE_PARAM_ACTIVE_PP_TIMESTAMP = 1,
  CONSOLE_PARAM_ACTIVE_PP_BL_TIMESTAMP,
  CONSOLE_PARAM_ACTIVE_FPGA_TIMESTAMP,
  CONSOLE_PARAM_BLOB_AGILE_NUMBER,
  CONSOLE_PARAM_BLOB_TIMESTAMP,
  CONSOLE_PARAM_BLOB_FLAGS,
  CONSOLE_PARAM_BLOB_PP_REVISION,
  CONSOLE_PARAM_BLOB_PP_TIMESTAMP,
  CONSOLE_PARAM_BLOB_PP_BL_REVISION,
  CONSOLE_PARAM_BLOB_PP_BL_TIMESTAMP,
  CONSOLE_PARAM_BLOB_FPGA_REVISION,
  CONSOLE_PARAM_BLOB_FPGA_TIMESTAMP,
  CONSOLE_PARAM_BLOB_ADAPT_BOOT_REVISION,
  CONSOLE_PARAM_BLOB_ADAPT_BOOT_TIMESTAMP,
  CONSOLE_PARAM_BLOB_ADAPT_EGIA_REVISION,
  CONSOLE_PARAM_BLOB_ADAPT_EGIA_TIMESTAMP,
  CONSOLE_PARAM_BLOB_ADAPT_EEA_REVISION,
  CONSOLE_PARAM_BLOB_ADAPT_EEA_TIMESTAMP,
  CONSOLE_PARAM_PP_USE_COUNTS,
  CONSOLE_PARAM_BLOB_SYS_VERSION,
  CONSOLE_PARAM_COUNT
} CONSOLE_PARAM;

typedef struct
{
    uint8_t     *pDataIn;                               /* pointer to Rx Data buffer */
    uint8_t     *pDataOut;                              /* pointer to response Data buffer */
    uint16_t    RxDataSize;                             /* total bytes in message */
    uint16_t    TxDataCount;                            /* total bytes in message */
    uint8_t     *pValidCommands[MAX_VALID_COMMANDS];    /* pointer to Valid commands */
    uint8_t     ValidCommandCount;                      /* Valid commands counter */
    uint8_t     CommandCounter;                         /* command counter being processed */
    uint16_t    PacketStartIndex[MAX_VALID_COMMANDS];   /* Frame start location for each command */
    uint16_t    DataSize;                               /* Data in message */
    COMM_IF     *pDataIF;                               /* which interface */
} PROCESSDATA;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
/// \todo 06/01/2021 CPK The below function to be integrated to streaming task once done
extern void SendStatusVars(void *pArg);
extern void SetUSBPortMode(bool Value);
extern bool GetUSBPortMode(void);
extern void ClearPasswordReceived(void);
extern void FormatDeviceAddr(uint8_t *InputId, uint8_t *OutputId);
/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_CONSOLECOMMANDS_H */

