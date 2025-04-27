#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \brief   Header file for Active Object framework interface
 *
 * \details Global defines and prototypes defined here.
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    ActiveObject.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/
#define EVENT_MSG_BUF_RDF_MAX       500
#define EVENT_MSG_BUF_RDF_SIZE      48

#define EVENT_MSG_BUF_PRINTF_MAX    50
#define EVENT_MSG_BUF_PRINTF_SIZE  (MAX_DEBUG_STRING_LEN + 10)

#define EVENT_MSG_BUF_1_MAX         100
#define EVENT_MSG_BUF_1_SIZE        48

#define EVENT_MSG_BUF_2_MAX         300
#define EVENT_MSG_BUF_2_SIZE        192  

#define ENABLE_EVENT_DEBUG_INFO     0

#define EVENT_MSG_BUF_RDF_TOTAL_SIZE      (EVENT_MSG_BUF_RDF_MAX    *  EVENT_MSG_BUF_RDF_SIZE    )
#define EVENT_MSG_BUF_PRINTF_TOTAL_SIZE   (EVENT_MSG_BUF_PRINTF_MAX *  EVENT_MSG_BUF_PRINTF_SIZE )
#define EVENT_MSG_BUF1_TOTAL_SIZE         (EVENT_MSG_BUF_1_MAX      *  EVENT_MSG_BUF_1_SIZE      )
#define EVENT_MSG_BUF2_TOTAL_SIZE         (EVENT_MSG_BUF_2_MAX      *  EVENT_MSG_BUF_2_SIZE      )

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
void  AO_Init(void);
void  AO_Start(QActive * const pAO, QStateHandler pInitial, uint_fast8_t Prio, QEvt const * * const pEvtQue,
               uint16_t const EvtQueLen, void * const pStack, uint16_t const StackLen,
               void const * const pPar, uint8_t *pTaskName);
void  AO_TimerCtor(QTimeEvt * const pTimer, QActive * const pAO, SIGNAL Sig);
bool  AO_TimerArm(QTimeEvt * const Timer, QTimeEvtCtr const Ticks, QTimeEvtCtr const Interval);
bool  AO_TimerDisarm(QTimeEvt * const pTimer);
bool  AO_TimerRearm(QTimeEvt * const pTimer, QTimeEvtCtr const Ticks);
bool  AO_TimerIsRunning(QTimeEvt * const pTimer);
void *AO_EvtNew(SIGNAL Sig, uint16_t EvtSize);
void  AO_Publish(void * pEvt, void const * pSender);
bool  AO_Post(QActive * const pAO, QEvt const * const pEvt, void const * pSender);
void  AO_Subscribe(QActive const * const pAO, enum_t const Sig);
void  AO_Unsubscribe(QActive const * const pAO, enum_t const Sig);
void  AO_UnsubscribeAll(QActive const * const pAO);
bool  AO_Defer(QActive const * const pAOQueue, QEQueue * const pDeferQueue, QEvt const * const pEvent);
bool  AO_Recall(QActive * const pAOQueue, QEQueue * const pDeferQueue);
void  AO_QueueInit(QEQueue * const pQueue, QEvt const * * const pSto, uint_fast16_t const Qlen);

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

