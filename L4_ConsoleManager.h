#ifndef L4_CONSOLEMANAGER_H
#define L4_CONSOLEMANAGER_H

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
 * \file    L4_ConsoleManager.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"                      /*! Import common definitions such as types, etc */
#include "L4_ConsoleCommands.h"
#include "Signia_CommManager.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define SHIFT_8_BITS                  (8u)
#define SHIFT_16_BITS                 (16u)
#define SHIFT_24_BITS                 (24u)
#define PARTIAL_DATA_BUFF_SIZE        (1000u)              /*! Partial data buffer*/
#define MAX_DATA_BYTES                      (1024u)              ///< Maximum data bytes

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum
{
    CONS_MGR_STATE_WAIT_FOR_EVENT,     ///< Default console task state
    CONS_MGR_STATE_CONNECTED,          ///< Device state connected
    CONS_MGR_STATE_DISCONNECTED,       ///< Device state disconnected
    CONS_MGR_STATE_GET_PCKT,           ///< task state to receive new data
    CONS_MGR_STATE_VALIDATE_PCKT,      ///< task state to verify received data frame
    CONS_MGR_STATE_PROCESS_COMMAND,    ///< task state to process the received command
    CONS_MGR_STATE_SEND_RESPONSE,      ///< task state to Send response for the command
    CONS_MGR_STATE_CHECK_SEND_REQ,     ///< task state to receive new data
    CONS_MGR_STATE_LAST_STATE          ///< last state

} CONS_MGR_STATE;

typedef enum
{
    CONS_MGR_STATUS_OK,
    CONS_MGR_STATUS_INVALID_COMMAND,
    CONS_MGR_STATUS_TIMEOUT,
    CONS_MGR_STATUS_ERROR

} CONS_MGR_STATUS;

typedef struct
{
    bool        Flag;
    uint8_t     Data[PARTIAL_DATA_BUFF_SIZE + MEMORY_FENCE_SIZE_BYTES];
    uint16_t    Timeout;
    uint16_t    PacketSize;
    uint16_t    RemainingDataSize;
} PARTIALDATA;

typedef void ( *CONS_CMD_HANDLER ) ( void *pPayload, uint8_t  PayloadSize ); /*! Event handler function */

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern CONS_MGR_STATUS L4_ConsoleMgrInit(void);
extern CONS_MGR_STATUS L4_ConsoleMgrRegisterHandler(SERIAL_CMD Cmd, CONS_CMD_HANDLER pHandler );
extern CONS_MGR_STATE  ProcessCommand(PROCESSDATA  *pDataRx);
extern CONS_MGR_STATUS L4_ConsoleMgrSendRequest(COMM_CONN ComID, SERIAL_CMD Cmd, uint8_t *pData, uint16_t DataCount);
extern void SendStatusVars(void *pArg);
extern bool L4_ConsoleMgrInitDone(void);
extern void CommEventHandler(COMM_MGR_EVENT Event);
extern bool L4_ConsoleMgrUpdateInterface(COMM_IF *pActiveInterface);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_CONSOLEMANAGER_H */

