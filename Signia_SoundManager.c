/*.$file${.::Signia_SoundManager.c} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*
* Model: Signia_SoundManager.qm
* File:  ${.::Signia_SoundManager.c}
*
* This code has been generated by QM 5.1.0 <www.state-machine.com/qm/>.
* DO NOT EDIT SECTIONS BETWEEN THE COMMENTS "$...vvv".."$end...^^^".
* All your changes in these sections will be lost.
*
* This code is covered by the following QP license:
* License #   : QPC-SP-170817A
* Issued to   : Covidien LP
* Framework(s): qpc
* Support ends: 2022-08-17
* Product(s)  :
* Signia Powered Stapler
*/
/*.$endhead${.::Signia_SoundManager.c} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_SoundManager
 * \{
 *
 * \brief   Sound Manager Active Object
 *
  * \details The Sound Manager is solely responsible for playing the various
 *          tones in the Signia Rearchitecture Software. It mainly consists
 *          of a Sound Manager task and a Tone Queue. The Sound Manager task
 *          creates and maintains the Tone Queue. All other modules can post
 *          tone requests to this queue, and the sound manager task plays it
 *          as per the order of arrival [FIFO]. Once a valid tone is present
 *          in the FIFO, the sound manager task comes out of sleep and processes
 *          the tone request. The sound manager uses L3 Tone APIs to interact
 *          with the FPGA.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_SoundManager.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/

#include "Common.h"
#include "ActiveObject.h"
#include "Signia_SoundManager.h"
#include "Logger.h"
#include "L3_Tone.h"
#include "L3_GpioCtrl.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/

#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_TONE)

#define SOUNDMGR_STACK_SIZE     (512u)                  // AO stack size
#define SOUNDMGR_EVQ_SIZE       (10u)                   // AO event queue size

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
OS_STK      SoundMgrStack[SOUNDMGR_STACK_SIZE + MEMORY_FENCE_SIZE_DWORDS];       /* Stack for AO */

/******************************************************************************/
/*                    Local Type Definition(s)  / Function Prototypes         */
/******************************************************************************/

// QM defines machine generated types and function prototypes here:

/*.$declare${AOs::SoundManager} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*.${AOs::SoundManager} ....................................................*/
typedef struct {
/* protected: */
    QActive super;
} SoundManager;

/* protected: */

/* ========================================================================== */
static QState SoundManager_initial(SoundManager * const me, void const * const par);
static QState SoundManager_Run(SoundManager * const me, QEvt const * const e);
/*.$enddecl${AOs::SoundManager} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

typedef struct
{
    QEvt            Event;
    SNDMGR_TONE     ToneId;    // Tone to play
} QEVENT_TONE;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

// All Good Tone Frequency and Time
static const ToneNote AllGoodTone[] =
{
    {2100, 75}, {0, 40},  {2100, 75}, {0, 40}, {2100, 75}, {0, 40},
    {2100, 75}, {0, 160}, {2100, 75}, {0, 40}, {2100, 75}, {NULL, NULL}
};

// Ready Tone Frequency and Time
static const ToneNote ReadyTone[] =
{
    {1950, 75}, {0, 100}, {1950, 75}, {0, 40}, {2100, 150}, {NULL, NULL}
};

// Clamp Confirmation Tone Frequency and Time
static const ToneNote ClampConfirmTone[] =
{
    {2100, 75}, {NULL, NULL}
};

// Enter Firing Mode Tone Frequency and Time
static const ToneNote EnterFiringTone[] =
{
    {1950, 75}, {0, 40}, {2150, 150}, {NULL, NULL}
};

// Exiting Firing Mode Tone Frequency and Time
static const ToneNote ExitFiringTone[] =
{
    {2150, 75}, {0, 40}, {1950, 150}, {NULL, NULL}
};

// Medium Speed Tone Frequency and Time
static const ToneNote MediumSpeedTone[] =
{
    { 2000,75}, {0, 40}, {2000, 75}, {0, 40}, {NULL, NULL}
};

// Slow Speed Tone Frequency and Time
static const ToneNote SlowSpeedTone[] =
{
    {2000, 75}, {0, 40},{2000, 75}, {0, 40}, {2000, 75}, {0, 40}, {NULL, NULL}
};

// Limit Reached Tone Frequency and Time
static const ToneNote LimitReachTone[] =
{
    {700, 100}, {NULL, NULL}
};

// Low Battery Tone Frequency and Time
static const ToneNote LowBattTone[] =
{
    {2000, 50}, {0, 50}, {1800, 50}, {0, 50}, {1600, 50}, {0, 50}, {1400, 50}, {NULL, NULL}
};

// Insufficient Battery Tone Frequency and Time
static const ToneNote InsuffBattTone[] =
{
    {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50},
    {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50},
    {700, 750}, {NULL, NULL}
};

// Emergency Retract Tone Frequency and Time
static const ToneNote EmgRetractTone[] =
{
    //Play the EMG_RETRACT_TONE by selecting a PIEZO frequency of 700 Hz in the following sequence:
    //On for 500 ms and Off for 500 ms
    {700, 500}, {0, 500}, {NULL, NULL}
};

// Caution Tone Frequency and Time
static const ToneNote CautionTone[] =
{
    {700, 100}, {0, 50}, {700, 100}, {0, 50}, {700, 100}, {NULL, NULL}
};

// Fault Tone Frequency and Time
static const ToneNote FaultTone[] =
{
    {700, 225}, {550, 225}, {700, 225}, {550, 225}, {700, 225}, {550, 225}, {NULL, NULL}
};

// Long Test Tone Frequency and Time
static const ToneNote LongTestTone[] =
{
    {2100, 2500}, {NULL, NULL}
};

// Shutdown Tone Frequency and Time
static const ToneNote ShutDownTone[] =
{
    {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50},
    {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 50}, {0, 50},
    {700, 50}, {0, 50}, {700, 50}, {0, 50}, {700, 750}, {NULL, NULL}
};

// Tone Look up Table
static const Tone ToneList[SNDMGR_TONE_COUNT] =
{
    {AllGoodTone,       "All Good Tone" },              // SNDMGR_TONE_ALL_GOOD
    {ReadyTone,         "Ready Tone" },                 // SNDMGR_TONE_READY
    {ClampConfirmTone,  "Clamp Confirm Tone" },         // SNDMGR_TONE_CLAMP_CONFIRMATION
    {EnterFiringTone,   "Enter Firing Tone" },          // SNDMGR_TONE_ENTER_FIRE_MODE
    {ExitFiringTone,    "Exit Firing Tone" },           // SNDMGR_TONE_EXIT_FIRE_MODE
    {MediumSpeedTone,   "Medium Speed Tone" },          // SNDMGR_TONE_MEDIUM_SPEED
    {SlowSpeedTone,     "Slow Speed Tone" },            // SNDMGR_TONE_SLOW_SPEED
    {LimitReachTone,    "Limit Reach Tone" },           // SNDMGR_TONE_LIMIT_REACHED
    {LowBattTone,       "Low Battery Tone" },           // SNDMGR_TONE_LOW_BATTERY
    {InsuffBattTone,    "Insufficient Battery Tone" },  // SNDMGR_TONE_INSUFFICIENT_BATTERY
    {EmgRetractTone,    "Emergency Retract Tone" },     // SNDMGR_TONE_RETRACT
    {CautionTone,       "Caution Tone" },               // SNDMGR_TONE_CAUTION
    {FaultTone,         "Fault Tone" },                 // SNDMGR_TONE_FAULT
    {LongTestTone,      "Long Test Tone" },             // SNDMGR_TONE_LONG_TEST
    {ShutDownTone,      "Shut Down Tone" },             // SNDMGR_TONE_SHUTDOWN
};

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/

/* Instantiate the Template active object ------------------------------------*/

static SoundManager  LocalSoundManager;                      /* Local data */
QActive * const AO_SoundMgr = &LocalSoundManager.super;      /* Opaque pointer to SoundMgr Active Object */

static QEvt const *SoundMgrEventQueue[SOUNDMGR_EVQ_SIZE];    /* Event Queue for AO */
static uint8_t    *AOname = "SoundManager";                  /* AO (task) name definition */

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/* Local helper functions for Template: */

/* Ask QM to define the Template class (State machine) -----------------------*/

/*.$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*. Check for the minimum required QP version */
#if (QP_VERSION < 680U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpc version 6.8.0 or higher required
#endif
/*.$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*.$define${AOs::SoundManager} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*.${AOs::SoundManager} ....................................................*/
/*.${AOs::SoundManager::SM} ................................................*/

/**
 * \brief   Initial transition
 *
 * \details This function is called as part of the Active Object construction process.
 *          (SoundMgrCtor()). It runs in the startup task's context, using its stack and
 *          TCB.
 *
 *          One time initializations are typically performed here, such as local data values
 *          and subscription to events. (Unsubscribing and then re-subscribing to events is rare)
 *
 *          Timer structures may be initialized here via AO_TimerCtor(), but they are typically
 *          initialized in the constructor (SoundMgrCtor()) function.
 *
 *          The par parameter points to a user defined parameter block for initialization purposes.
 *          This is typically not used. (Set to NULL)
 *
 * \param   me - Pointer to AO's local data structure. (This includes the AO descriptor)
 * \param   par - Pointer to user defined parameter block
 *
 * \return  State status
 *
 * ========================================================================== */
static QState SoundManager_initial(SoundManager * const me, void const * const par) {
    /*.${AOs::SoundManager::SM::initial} */
    return Q_TRAN(&SoundManager_Run);
}
/*.${AOs::SoundManager::SM::Run} ...........................................*/
static QState SoundManager_Run(SoundManager * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /*.${AOs::SoundManager::SM::Run::PLAY_TONE} */
        case PLAY_TONE_SIG: {
            L3_TonePlay((Tone *)&ToneList[((QEVENT_TONE *)e)->ToneId]);
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*.$enddef${AOs::SoundManager} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* Global functions for SoundManager: */

/* Ask QM to define the SoundManager Constructor function: ------------------ */

/*.$define${AOs::L4_SoundManagerCtor} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/**
 * \brief   Constructor for the SoundMgr Active Object
 *
 * \details Establishes the Active Object (AO) and executes the initial transition
 *          and state entry code. Other AO specific objects, such as timers, may
 *          be initialized here via AO_TimerCtor().
 * \n \n
 *          The SoundMgr "constructor" is provided outside of the SoundMgr class so that it
 *          can be used independently from the class. This is part of the "opaque pointer"
 *          design idiom.
 * \n \n
 *          Establishing the AO encompasses the following activities:
 *              - Initialize and register the task control block with Micrium.
 *                  - Set the stack
 *                  - Set the priority
 *                  - Set the name
 *              - Initialize and register the active object control block with QP/C.
 *                  - Set the event queue
 *                  - Pass user defined initialization parameters
 *
 * \note    The initial transition and state entry code (the 1st time) are executed
 *          in the calling task's context so care must be taken to consider the calling
 *          task's stack constraints and task dependent defaults, if any, when writing
 *          that code. This is usually not a problem, but when using certain features of
 *          Micrium (such as the file system), Micrium keeps certain configuration information
 *          in the task control block.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
/*.${AOs::L4_SoundManagerCtor} .............................................*/
void L4_SoundManagerCtor(void) {
    AO_Start(AO_SoundMgr,                           // Pointer to QActive object (AO)
             Q_STATE_CAST(&SoundManager_initial),   // Pointer to initial transition
             TASK_PRIORITY_L4_SOUNDMGR,             // Task priority
             SoundMgrEventQueue,                    // Pointer to event queue
             SOUNDMGR_EVQ_SIZE,                     // Size of event queue (in entries)
             SoundMgrStack,                         // Pointer to stack (bottom)
             SOUNDMGR_STACK_SIZE,                   // Stack size (in longwords)
             NULL,                                  // Pointer to object specific initialization parameters
             AOname);                               // Pointer to name of active object
}
/*.$enddef${AOs::L4_SoundManagerCtor} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/* ========================================================================== */
/**
 * \brief   Play a tone
 *
 * \details This function queues the user supplied tone to be played. It will be
 *          played after any previously pending tones complete.
 *
 * \param   eTone - Tone to Play
 *
 * \return  SNDMGR_STATUS - Sound Manager Status
 * \retval  See L4_SoundMgr.h
 *
 * ========================================================================== */
SNDMGR_STATUS Signia_PlayTone(SNDMGR_TONE Tone)
{
    QEVENT_TONE    *pTone;           // Pointer to tone event
    SNDMGR_STATUS  StatusReturn;     // Sound Manager Status Return

    StatusReturn = SNDMGR_ERROR;

    do
    {
        if (Tone >= SNDMGR_TONE_COUNT)                      // Quit if invalid tone
        {
            Log(ERR, "SoundManager: Invalid Tone");
            break;
        }

        pTone = AO_EvtNew(PLAY_TONE_SIG, sizeof(QEVENT_TONE));

        if (NULL == pTone)                                 // Quit if no memory available
        {
            Log(ERR, "SoundManager: No Event memory available");
            break;
        }

        pTone->ToneId = Tone;    // Set tone ID in event

        // NOTE: AO_Post returns false on error. This is inconsistent with coding standard.
        if(!AO_Post(AO_SoundMgr, &pTone->Event, NULL))
        {
            Log(ERR, "SoundManager: Event Queue is Full");      // Log error
            break;
        }
        StatusReturn = SNDMGR_OK;                           // No errors
    } while(false);

    return (StatusReturn);
}

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif