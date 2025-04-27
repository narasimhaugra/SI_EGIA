#ifndef  L4_COMMMANAGER_H
#define  L4_COMMMANAGER_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \brief   Public interface for Communication Manager module.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_CommManager.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         /* Import common definitions such as types, etc. */
#include "Signals.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum                            /*! Connection Manager function status codes */
{
    COMM_MGR_STATUS_OK,                /*! No error */
    COMM_MGR_STATUS_INVALID_PARAM,     /*! Invalid parameter */
    COMM_MGR_STATUS_DISCONNECTED,      /*! Disconnected */
    COMM_MGR_STATUS_Q_FULL,            /*! Queue is full */
    COMM_MGR_STATUS_Q_EMPTY,           /*! Queue is empty */
    COMM_MGR_STATUS_ERROR,             /*! Error */
    COMM_MGR_STATUS_COUNT
} COMM_MGR_STATUS;

typedef enum                            /*! Communication interface types */
{
    COMM_CONN_WLAN,                     /*! Wi-Fi connection */
    COMM_CONN_USB,                      /*! USB connection */
    COMM_CONN_IR,                       /*! IR connection */
    COMM_CONN_UART0,                  /*! Adapter connection */
    COMM_CONN_ACTIVE,                   /*! Active connection: Transfers data over the active connection. */
    COMM_CONN_COUNT
} COMM_CONN;

typedef enum                            /*! Communication manager event types */
{
  COMM_MGR_EVENT_NEWDATA,              /*! New data */
  COMM_MGR_EVENT_CONNECT,              /*! Connect event */
  COMM_MGR_EVENT_DISCONNECT,           /*! Disconnect event */
  COMM_MGR_EVENT_RESET,                /*! Reset */
  COMM_MGR_EVENT_SUSPEND,              /*! Suspend */
  COMM_MGR_EVENT_RESUME,               /*! Resume */
  COMM_MGR_EVENT_ERROR,                /*! Error */
  COMM_MGR_EVENT_COUNT
} COMM_MGR_EVENT;

typedef void (*COMM_HANDLER)(COMM_MGR_EVENT Event);                    /*! Event handler function
                                                                            Event: Event to be notified  */
typedef COMM_MGR_STATUS (*COMM_TXFR)(uint8_t *pData, uint16_t *pSize); /*! Transfer function
                                                                            pData: Data to be transferred
                                                                            pSize: Data size, will be updated as per actual transferred data. */
typedef COMM_MGR_STATUS (*COMM_PEEK)(uint16_t *pCount);                /*! Checks if any data available to read.
                                                                            pCount: Size of unread data */
typedef struct                          /*! Communication interface object returned when a connection is open */
{
   COMM_TXFR Send;                      /*! Interface function to send data */
   COMM_TXFR Receive;                   /*! Interface function to received data */
   COMM_PEEK Peek;                      /*! Interface function to check if new any data available to read */
} COMM_IF;

typedef struct                          /// USB connected signal
{
    QEvt                Event;          ///< QPC event header
} QEVENT_USB;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern COMM_MGR_STATUS L4_CommManagerInit(void);
extern COMM_IF *L4_CommManagerConnOpen(COMM_CONN Type, COMM_HANDLER pHandler);
extern COMM_MGR_STATUS L4_CommManagerConnClose(COMM_IF *pConnection);
extern uint8_t L4_USBConnectionStatus(void);
extern bool L4_CommStatusActive(void);
extern void SetUSBPortMode(bool Value);
extern bool GetUSBPortMode(void);
extern void ClearPasswordReceived(void);

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_COMMMANAGER_H */

