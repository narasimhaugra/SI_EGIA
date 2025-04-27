#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup ActiveObject
 * \{
 *
 * \brief   This module contains QPC interface routines for the active object
 *          framework.
 *
 * \details These functions encapsulate QPC functions, and in some cases, may
 *          provide additional functionality.
 * \n
 *          An active object is illustrated below:
 *          \image html activeObject.jpg
 * \n
 *          An Active object consists of three things:
 *              1. An hierarchichal state machine
 *              2. An event queue
 *              3. A context (thread of execution in OS)
 *              .
 * \n
 *          Once an event gets placed in an active object's queue, it will be sent
 *          to the active object's state machine for processing in the current
 *          state handler function.  It is important to note, that an active object
 *          will only execute user code inside the state handler functions and only
 *          when an event gets sent to that active object.
 * \n \n
 *          Active Objects will now be referred to as AOs.
 * \n \n
 * \par     Signals:
 *              Signals are defined in Signals.h (In module Signals) \n
 *              Signals are differentiated as follows:
 *                  - R_xxx_SIG signals are Reserved Signals
 *                  - P_xxx_SIG signals are Published Signals
 *                  -   xxx_SIG signals are directly posted signals
 *                  .
 *
 * \par     Event Posting:
 *              There are two methods used to make AOs aware of real world or system events:
 *
 * \par         Publish / Subscribe Events:
 *                  When an AO wishes to inform the system about events that are taking place,
 *                  it will publish the event as shown in the diagram above. Any AO that wishes
 *                  to be informed about the occurrence of an event must subscribe to the event.
 *                  Establishing which Active Objects subscribe to or publish an event is determined
 *                  at compile time.
 *
 * \par         Direct Post Events:
 *                  This is a faster mechanism for communicating events. It directly posts an event
 *                  from an AO to the event queue of of anther AO. The disadvantage of this method
 *                  is that an event is only communicated between two AOs, instead of multiple AOs.
 *                  Please refer to the image below for an example of direct posting of an event.
 *
 *  \par        Timers:
 *                  AOs can respond to one-shot or repetitive timers. Timers are useful for timing
 *                  of processes, generating delays or for transitioning between states. Timers must
 *                  be stopped before exiting an AO.
 *
 *  \image html AO_framework.jpg
 *
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Common.h"
#include "ActiveObject.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_AO)

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
Q_DEFINE_THIS_FILE      // Required to identify file for QPC asserts

/// \todo 01/20/2022 DAZ - PROBLEM. QSubscrList is defined as 32 bits wide. (1 per active task)
/// \todo 01/20/2022 DAZ - We have a system that can accommodate 64 tasks. If we have more than 32
/// \todo 01/20/2022 DAZ - tasks in the system, certain tasks may not be allowed to subscribe to an event.
/// \todo 01/20/2022 DAZ - This needs to be researched.
/* Subscriber list. This is an array of bitmap to keep track of who is subscribed to what signal */
static QSubscrList SubscrSto[LAST_SIG]; /// \todo 01/20/2022 DAZ - This should be P_LAST_SIG to save space

/* Buffers for allocating events: */
/* All buffers have been allocated in external RAM. */

/*!< \todo MISSING DAZ 02/16/21 - Allocate buffers for RDF logging (MsgBuf1?) & Log logging */
/// \todo 02/19/2021 DAZ - Only two event sizes here? Optimize depending on final event sizes

#pragma location=".sram"
uint8_t EventMsgBuf1[EVENT_MSG_BUF1_TOTAL_SIZE + MEMORY_FENCE_SIZE_BYTES];

#pragma location=".sram"
uint8_t EventMsgBuf2[EVENT_MSG_BUF2_TOTAL_SIZE + MEMORY_FENCE_SIZE_BYTES];

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Initialize the quantum framework
 *
 * \details This function calls the QPC function QF_Init() which in turn initializes
 *          the micrium RTOS. This provides an RTOS independent method for initializing
 *          the RTOS.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void AO_Init(void)
{
    QF_init();                                  /* Calls micrium OS init */
    QF_psInit(SubscrSto, Q_DIM(SubscrSto));     /* Initialize publish/subscribe */

    /* Initialize event pools... NOTE: They must be initialized in ascending event size */
    QF_poolInit(EventMsgBuf1, EVENT_MSG_BUF1_TOTAL_SIZE, EVENT_MSG_BUF_1_SIZE);
    QF_poolInit(EventMsgBuf2, EVENT_MSG_BUF2_TOTAL_SIZE, EVENT_MSG_BUF_2_SIZE);
}

/* ========================================================================== */
/**
 * \brief   Create and start an active object
 *
 * \details This function registers and initializes the Active Object and its
 *          underlying TCB. It initializes the task's stack and the AO's event
 *          queue, and executes the initial transition code and any state entry
 *          code. This is a combination of the QP/C calls QActive_ctor and QACTIVE_START.
 *
 * \note    All of the above is performed in the context of the task which invokes
 *          AO_start. In the initial transition and state entry code, care must be
 *          taken to respect the constraints of the calling context, particularly
 *          stack size and any task dependent defaults.
 *
 * \param   pAO       - Pointer to the Active Object to create
 * \param   pInitial  - Pointer to the initial transition function
 * \param   Prio      - Task priority
 * \param   pEvtQue   - Pointer to event queue
 * \param   EvtQueLen - Event queue length in storage units (longwords)
 * \param   pStack    - Pointer to task stack
 * \param   StackLen  - Stack size in storage units (longwords)
 * \param   pPar      - Pointer to parameter block for initial transition (user defined)
 * \param   pTaskName - Pointer to task name. Pointer must point to static location. Null if none.
 *
 * \return  None
 *
 * ========================================================================== */
void AO_Start(QActive * const pAO, QStateHandler pInitial, uint_fast8_t Prio, QEvt const * * const pEvtQue,
              uint16_t const EvtQueLen, void * const pStack, uint16_t const StackLen,
              void const * const pPar, uint8_t *pTaskName)
{
    uint8_t OsErr;

    QActive_ctor((QActive *)pAO, pInitial);                 // Set initial transition for AO
    QActive_setAttr((QActive *)pAO, OS_TASK_OPTIONS, 0);    // Set uC/OS-II attributes
    QACTIVE_START(pAO,                                      // Active object to start
                  Prio,                                     // Active object priority
                  pEvtQue,                                  // Event queue buffer pointer
                  EvtQueLen,                                // Event queue size (in longwords)
                  pStack,                                   // Pointer to task stack
                  StackLen,                                 // Stack size (in longwords)
                  pPar);                                    // User defined parameters

    if (pTaskName != NULL)
    {
        OSTaskNameSet(Prio, pTaskName,  &OsErr);            // Set task name only if pointer not null
    }
}

/* ========================================================================== */
/**
 * \brief   Initialize (construct) a timer event
 *
 * \details This function associates a timer event object with an active object
 *          and a specified signal.
 *
 * \param   pTimer - Pointer to timer event object.
 * \param   pAO    - Pointer to active object to post to
 * \param   Sig    - Signal to post
 *
 * \return  None -  This function asserts on out of bounds signal
 *
 * ========================================================================== */
void AO_TimerCtor(QTimeEvt * const pTimer, QActive * const pAO, SIGNAL Sig)
{
    QTimeEvt_ctorX(pTimer, pAO, Sig, 0);    // Timers are fixed to use tick rate 0.
}

/* ========================================================================== */
/**
 * \brief   Arm the specified timer
 *
 * \details This function sets the timer's initial, and subsequent timeout values
 *          and starts the timer. The signal issued upon timeout and the AO it is
 *          issued to are determined by AO_TimerCtor(). By specifing the subsequent
 *          timeout value (Interval), the timer can be periodic rather than one shot.
 *
 * \param   pTimer   - Pointer to timer event object
 * \param   Ticks    - Number of ticks (milliseconds) for initial delay
 * \param   Interval - Number of ticks (milliseconds) for subsequent delays (0 if none)
 *
 * \return  bool - 'true' if the time event was truly armed, that is, it was NOT running
 *            when AO_TimerArm() was called. The return of 'false' means that the timer
 *            was already running as the result of a previous Arm or Rearm. In the case
 *            of a false return, the timer is not changed, and continues to run.
 *
 * ========================================================================== */
bool AO_TimerArm(QTimeEvt * const pTimer, QTimeEvtCtr const Ticks, QTimeEvtCtr const Interval)
{
    bool Status;

    if (0 == pTimer->ctr)
    {
        QTimeEvt_armX(pTimer, Ticks, Interval);
        Status = true;
    }
    else
    {
        Log(ERR, "Timer signal %s in task %s already armed with %d msec left.", SigNameTable[pTimer->super.sig], OSTCBCur->OSTCBTaskName, pTimer->ctr);
        Status = false;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Disarm the specified timer event so it can be safely reused
 *
 * \details Disarm the time event so it can be safely reused. This is done by
 *          setting the ctr value in the timer event to 0.
 *
 * \param   pTimer - Pointer to timer event
 *
 * \note    There is no harm in disarming an already disarmed time event.
 *
 * \return  bool - 'true' if the time event was truly disarmed, that is, it was running.
 *            The return of 'false' means that the time event was not truly disarmed,
 *            because it was not running. The 'false' return is only possible for one-
 *            shot time events that have been automatically disarmed upon expiration.
 *            In this case the 'false' return means that the time event has already
 *            been posted or published and should be expected in the active object's
 *            state machine.
 *
 * ========================================================================== */
bool AO_TimerDisarm(QTimeEvt * const pTimer)
{
    return QTimeEvt_disarm(pTimer);
}

/* ========================================================================== */
/**
 * \brief   Check to see if timer is running.
 *
 * \details This function checks the ctr member of the timer structure to see
 *          if the timer is running, and returns true if the counter is non zero.
 *          This function is useful only with timers running in one-shot mode,
 *          as periodic timers reload ctr as soon as the timer times out.
 *
 * \param   pTimer - Pointer to timer event
 *
 * \return  bool - 'true' if timer's count value is 0. (Timer has expired),
 *                 'false' otherwise.
 *
 * ========================================================================== */
bool AO_TimerIsRunning(QTimeEvt * const pTimer)
{
    return   (0 != pTimer->ctr);
}

/* ========================================================================== */
/**
 * \brief   Rearm a timer event with a new number of clock ticks.
 *
 * \details Rearms a time event with a new number of clock ticks. This function
 *          can be used to adjust the current period of a periodic time event or
 *          to prevent a one-shot time event from expiring (e.g., a watchdog time
 *          event). Rearming a periodic timer leaves the interval unchanged and
 *          is a convenient method to adjust the phasing of a periodic time event.
 *
 * \note    This function only affects the initial value. It does not affect the
 *          interval value.
 *
 * \param   pTimer - Pointer to timer event
 * \param   Ticks -  New number of ticks to assign to timer
 *
 * \return  bool - 'true' if the time event was running as it was re-armed. The 'false'
 *           return means that the time event was not truly rearmed because it
 *           was not running. The 'false' return is only possible for one-shot
 *           time events that have been automatically disarmed upon expiration.
 *           In this case, the 'false' return means that the time event has
 *           already been posted or published and should be expected in the
 *           active object's state machine.
 *
 * ========================================================================== */
bool AO_TimerRearm(QTimeEvt * const pTimer, QTimeEvtCtr const Ticks)
{
    return QTimeEvt_rearm(pTimer, Ticks);
}

/* ========================================================================== */
/**
 * \brief   Allocate an event of the specified size and initialize it with the
 *          specified signal.
 *
 * \details Calls QF_newX_ to allocate the event and return a pointer to it.
 *
 * \param   Sig -       Signal to initialize the event with
 * \param   EvtSize -   Size of the event
 *
 * \return  Pointer to allocated event. NULL if none.
 *
 * ========================================================================== */
void *AO_EvtNew(SIGNAL Sig, uint16_t EvtSize)
{
    return (void *)QF_newX_((uint_fast16_t)EvtSize, 0, Sig);
}

/* ========================================================================== */
/**
 * \brief   Publish the specified event
 *
 * \details Given a pointer to an event structure, publish it to the active
 *          objects that have subscribed to it.
 *
 * \param   pEvt - Pointer to event to publish
 * \param   pSender - Pointer to sending AO.
 *
 * \sa      QF_PUBLISH header for details on sender.
 * \note    pSender is only used if QSPY is enabled, otherwise it is ignored.
 * \todo 02/05/2021 DAZ - We may remove pSender argument.
 *
 * \return  None
 *
 * ========================================================================== */
void AO_Publish(void * pEvt, void const * pSender)
{
    /// \todo 02/05/2021 DAZ - We may wish to log the publishing of certain events...
    QF_PUBLISH(pEvt, pSender);
}

/* ========================================================================== */
/**
 * \brief   Post the specified event to the specified active object
 *
 * \details Puts the specified event pointer on the event queue of the specified AO.
 *
 * \param   pAO - Pointer to AO to queue to.
 * \param   pEvt - Pointer to event to queue.
 * \param   pSender - Pointer to AO of sender.
 *
 * \sa      QACTIVE_POST_X header for details on sender.
 * \note    pSender is only used if QSPY is enabled, otherwise it is ignored.
 *
 * \todo 02/05/2021 DAZ - We may remove pSender argument.
 * \todo 06/08/2021 DAZ - We may allow the caller to specify the margin.
 *
 * \note    If destination queue is full, routine asserts. QACTIVE_POST_X will not
 *          assert, but return false if queueing failed.
 *
 * \return  Post operation status
 * \retval  true - Post succeeded.
 * \retval  false - Post failed. Event queue full.
 *
 * ========================================================================== */
bool AO_Post(QActive * const pAO, QEvt const * const pEvt, void const * pSender)
{
    return QACTIVE_POST_X(pAO, pEvt, 0, pSender);  // Margin is set to 0.
}

/* ========================================================================== */
/**
 * \brief   Subscribe the Active Object to the specified signal.
 *
 * \details When the specified signal is published, a pointer to it will be put
 *          on the AO's event queue, and the AO will be invoked to process it.
 *
 * \param   pAO - Pointer to subscribing Active Object
 * \param   Sig -  Signal to subscribe to
 *
 * \return  None
 *
 * ========================================================================== */
void AO_Subscribe(QActive const * const pAO, enum_t const Sig)
{
    QActive_subscribe(pAO, Sig);
}

/* ========================================================================== */
/**
 * \brief   Unsubscribe the specified signal from the Active Object
 *
 * \details This function removes the association of a signal with a specified
 *          Active Object. Published signal will no longer be sent to this object.
 *
 * \param   pAO - Pointer to subscribing Active Object
 * \param   Sig -  Signal to unsubscribe
 *
 * \return  None
 *
 * ========================================================================== */
void AO_Unsubscribe(QActive const * const pAO, enum_t const Sig)
{
    QActive_unsubscribe(pAO, Sig);
}

/* ========================================================================== */
/**
 * \brief   Unsubscribe all signals from the specified Active Object
 *
 * \details This function removes all signals associated with the specified
 *          Active Object. The object no longer subscribes to any signals.
 *          The object will still respond to posted signals, however.
 *
 * \param   pAO - Pointer to Active Object to unsubscibe signals from
 *
 * \return  None
 *
 * ========================================================================== */
void AO_UnsubscribeAll(QActive const * const pAO)
{
    QActive_unsubscribeAll(pAO);
}

/* ========================================================================== */
/**
 * \brief   Move event to defer queue for later processing
 *
 * \details If a state receives an event that cannot be processed at that time,
 *          it can be moved to a queue defined by the user to defer its processing
 *          to a later time. (Usually another state)
 *
 * \param   pAOQueue - Pointer to the Active Object's queue (typically &me->super)
 * \param   pDeferQueue - Pointer to the queue for deferring the event to
 * \param   pEvent - Event pointer to be deferred
 *
 * \return  bool - Operation status
 * \retval  true - The event was queued to the defer queue successfully
 * \retval  false - The event was not queued. (Defer queue full)
 *
 * ========================================================================== */
bool AO_Defer(QActive const * const pAOQueue, QEQueue * const pDeferQueue, QEvt const * const pEvent)
{
    return QActive_defer(pAOQueue, pDeferQueue, pEvent);
}

/* ========================================================================== */
/**
 * \brief   Recall an event from the defer queue for deferred processing
 *
 * \details This function recalls the latest event from the defer queue and
 *          posts it to the front of the AO's event queue so that is the next
 *          event to be processed.
 *
 * \param   pAOQueue - Pointer to the Active Object's queue (typically &me->super)
 * \param   pDeferQueue - Pointer to the defer queue to recall the event from
 *
 * \return  bool - Operation status
 * \retval  true - The latest event was recalled from the defer queue.
 * \retval  false - The defer queue was empty. No event to recall.
 *
 * ========================================================================== */
bool AO_Recall(QActive * const pAOQueue, QEQueue * const pDeferQueue)
{
    return QActive_recall(pAOQueue, pDeferQueue);
}

/* ========================================================================== */
/**
 * \brief   Initialize event queue
 *
 * \details This function initializes an event queue by giving it storage for the
 *          ring buffer. It is not used to initialize the event queue for an active
 *          object - that is handled by AO_Start(). This function is used for initializing
 *          the defer queue used by AO_Defer() and AO_Recall().
 * \n \n
 *          The queue is declared as:
 *              - QEQueue      DeferQ;          // Queue control block
 *              - QEvt const * DeferQSto[n];    // Ptr to Queue storage (n is number of events)
 *              .
 *
 * \param   pQueue - Pointer to the queue to initialize (QEQueue object)
 * \param   pSto - Pointer to the queue storage (QEvt const * DeferQSto[n])
 * \param   Qlen - Length of queue (in events)
 *
 * \return  None
 *
 * ========================================================================== */
void AO_QueueInit(QEQueue * const pQueue, QEvt const * * const pSto, uint_fast16_t const Qlen)
{
    QEQueue_init(pQueue, pSto, Qlen);
}

/* ========================================================================== */
/**
 * \brief   QPC callbacks
 *
 * \details These are user defined callbacks. Null for now. Providing basic
 *          assert. Just Logs, does not currently stop system. Needs development.
 *
 * ========================================================================== */
/// \todo 02/15/2021 DAZ - Providing assert for QPC here. Can also define Q_NASSERT to run without it.
/// \todo 02/18/2021 DAZ - Put these routines in qf_port.c? Fixed for system?

/* callback functions needed by the framework ------------------------------*/
void QF_onStartup(void) {}
void QF_onCleanup(void) {}
void QF_onClockTick(void) {}

#ifndef Q_UTEST          // { 02/15/2021 DAZ -

/// \todo 02/15/2021 DAZ - This assert is a work in progress. Minimal information for debugging.
/// \todo 06/08/2021 DAZ - In the event of queue overflow, it would be good to know
/// \todo 06/08/2021 DAZ - which queue overflowed.
/// \todo 08/06/2021 DAZ - This function should have a proper header.

Q_NORETURN Q_onAssert(char_t const * const module, int_t const location) {
  Log(DEV, "\r\nAssert @ module: %s, Location: %d, Task: %s", module, location, OSTCBCur->OSTCBTaskName);

  // Dead end this task so it doesn't do any harm.

  do
  {
    OSTimeDly(1000);
  } while (true);
}

#endif                   // }

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

