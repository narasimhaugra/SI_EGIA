#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_Keypad
 * \{

 * \brief   Public interface for Keypad Handler Module.
 *
 * \details This module provides an interface between Application and keypad module.
 *          Various Application specific functions,QPC events for keypad signals
 *          are included with the interface prototypes.
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_KeypadEvents.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Includes                                       */
/******************************************************************************/
#include "Common.h"
#include "Signia_Keypad.h"
#include "ActiveObject.h"
#include "Signia_KeypadEvents.h"
#include "Signals.h"
#include "L2_Timer.h"
#include "L3_OneWireController.h"
#include "L3_DispPort.h"

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
void Signia_KeypadEventHandlerInit(void);

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER   (LOG_GROUP_KEYPAD)    ///< Log Group Identifier
#define VALID_KEY_STATES       (2u)                  ///< Considering Press, release as valid states
#define RESET_KEY_TIME         (4000u)               ///< 4-Sec delay after Soft Resekey press
#define SOFT_RESET_KEY_TIME    (4000u)               ///< 4-Sec delay after Soft Reset key press
#define HARD_RESET_KEY_TIME    (5000u)               ///< 5-Sec delay after Hard Reset key press
#define SECOND_TO_MICRO        (1000u)               ///< Seconds to Micro Seconds

///< Below Keys are masked based upo KEY_ID. Any changes in this enum will impact on Soft Reset logic

#define MASK_RESET_KEYS                  (0x03F0u)         ///< Mask all Soft Reset combinational key (Safty Keys + Lateral Keys)             
#define END_OF_PATTERN                   (0)               ///< End Pattern
#define GET_KEY(KeyID)                   (1 << (KeyID))   ///< Get the key pattern based on the KeyId
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
/// Keypad Data structure
typedef struct
{
    KEY_ID Key;                                     ///< Key Pressed
    SIGNAL PhysicalKeySignal[VALID_KEY_STATES];     ///< Valid Key states - Press, release
    KEY_SIDE KeySide;                               ///< Position of the key (Lefts side, Right side)
} KeyInfoStruct;

/// Hold the Active Reset Pattern currently set on the handle
typedef enum
{
    HARD_RESET_PATTERN,           ///< Hard Reset pattern (both safety keys pressed)
    SOFT_RESET_PATTERN,           ///< Soft Reset pattern (both safety keys pressed + any one rotate key)
    RESET_PATTERN_NONE            ///< No Reset Pattern selected
} ACTIVE_RESET_PATTERN;
/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void KeypadEventHandler(KEY_ID Key, KEY_STATE State, uint16_t KeyChanged);
static void Signia_PrintKeypadEvent(KEY_ID Key,  KEY_STATE State, SIGNAL PublishSignal);

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/* Below structure array holding Key data for each Key*/
static const KeyInfoStruct KeyData[] =
{
    /* Keyid              Release Signal                        Press Signal                   Key Side*/

    { TOGGLE_DOWN,        P_TOGGLE_DOWN_RELEASE_SIG,        P_TOGGLE_DOWN_PRESS_SIG,           KEY_SIDE_UNDEFINED },
    { TOGGLE_UP,          P_TOGGLE_UP_RELEASE_SIG,          P_TOGGLE_UP_PRESS_SIG,             KEY_SIDE_UNDEFINED },
    { TOGGLE_LEFT,        P_TOGGLE_LEFT_RELEASE_SIG,        P_TOGGLE_LEFT_PRESS_SIG,           KEY_SIDE_LEFT      },
    { TOGGLE_RIGHT,       P_TOGGLE_RIGHT_RELEASE_SIG,       P_TOGGLE_RIGHT_PRESS_SIG,          KEY_SIDE_RIGHT     },

    { LATERAL_LEFT_UP,    P_LATERAL_LEFT_UP_RELEASE_SIG,    P_LATERAL_LEFT_UP_PRESS_SIG,       KEY_SIDE_LEFT      },
    { LATERAL_RIGHT_UP,   P_LATERAL_RIGHT_UP_RELEASE_SIG,   P_LATERAL_RIGHT_UP_PRESS_SIG,      KEY_SIDE_RIGHT     },
    { LATERAL_LEFT_DOWN,  P_LATERAL_LEFT_DOWN_RELEASE_SIG,  P_LATERAL_LEFT_DOWN_PRESS_SIG,     KEY_SIDE_LEFT      },
    { LATERAL_RIGHT_DOWN, P_LATERAL_RIGHT_DOWN_RELEASE_SIG, P_LATERAL_RIGHT_DOWN_PRESS_SIG,    KEY_SIDE_RIGHT     },

    { SAFETY_RIGHT,       P_SAFETY_RELEASE_SIG,             P_SAFETY_PRESS_SIG,                 KEY_SIDE_RIGHT    },
    { SAFETY_LEFT,        P_SAFETY_RELEASE_SIG,             P_SAFETY_PRESS_SIG,                 KEY_SIDE_LEFT     }
};

static bool IsRotationDisabled[KEY_SIDE_UNDEFINED] = {0, 0};  ///< Rotation is allowed or not


/// \todo 10/28/2021 DAZ - Local constants

/// \todo 10/28/2021 DAZ - Signals for key press/release:
/// Below is an array of constant events, one for each key press/release.
/// The events are predeclared in this array, and a pointer to the appropriate
/// event is published. Since the events are not allocated from a memory pool (pool ID 0),
/// there is no need to call AO_EvtNew, and the dispatcher does not need to deallocate
/// the event.

/// \todo 10/28/2021 DAZ - This could be done w/ the thing above, but it needs EvtNew... We could add the below stuff to it....

static const QEvt KeySig[KEY_COUNT][2] =
{
    //  Signal,  Pool ID,  refCtr
    { { P_TOGGLE_DOWN_RELEASE_SIG,          0, 0 }, { P_TOGGLE_DOWN_PRESS_SIG,          0, 0 } },     // Close key
    { { P_TOGGLE_UP_RELEASE_SIG,            0, 0 }, { P_TOGGLE_UP_PRESS_SIG,            0, 0 } },     // Open key
    { { P_TOGGLE_LEFT_RELEASE_SIG,          0, 0 }, { P_TOGGLE_LEFT_PRESS_SIG,          0, 0 } },     // Artic left key
    { { P_TOGGLE_RIGHT_RELEASE_SIG,         0, 0 }, { P_TOGGLE_RIGHT_PRESS_SIG,         0, 0 } },     // Artic right key
    { { P_LATERAL_LEFT_UP_RELEASE_SIG,      0, 0 }, { P_LATERAL_LEFT_UP_PRESS_SIG,      0, 0 } },     // Left CW key
    { { P_LATERAL_RIGHT_UP_RELEASE_SIG,     0, 0 }, { P_LATERAL_RIGHT_UP_PRESS_SIG,     0, 0 } },     // Right CCW key
    { { P_LATERAL_LEFT_DOWN_RELEASE_SIG,    0, 0 }, { P_LATERAL_LEFT_DOWN_PRESS_SIG,    0, 0 } },     // Left CCW key
    { { P_LATERAL_RIGHT_DOWN_RELEASE_SIG,   0, 0 }, { P_LATERAL_RIGHT_DOWN_PRESS_SIG,   0, 0 } },     // Right CW key
    { { P_SAFETY_RELEASE_SIG,               0, 0 }, { P_SAFETY_PRESS_SIG,               0, 0 } },     // Safety key (Left)
    { { P_SAFETY_RELEASE_SIG,               0, 0 }, { P_SAFETY_PRESS_SIG,               0, 0 } }      // Safety key (Right)
};

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Handler for Keypad Events
 *
 * \details This Function receives Key pressed and Key state to publish appropriate
 *          signal to L5.
 *
 * \param   Key           : Key Pressed identified by KEY_ID
 * \param   State         : Current Key State (Pressed/ Released)
 *
 * \return  None
 *
 * ========================================================================== */
/// \todo 10/28/2021 DAZ - Why does this have to be outside of keypad?
static void KeypadEventHandler(KEY_ID Key, KEY_STATE State, uint16_t KeyState)
{
    QEVENT_KEY *pKeyEvent;


    pKeyEvent = AO_EvtNew(P_KEYPRESS_SIG, sizeof(QEVENT_KEY));

    if (NULL != pKeyEvent)
    {
        pKeyEvent->Key = Key;
        pKeyEvent->State = State;
        pKeyEvent->KeyState = KeyState;
        /* Log Key,State,Signal - Debug purpose */
        Signia_PrintKeypadEvent(Key, State, KeyData[Key].PhysicalKeySignal[State]);

        /* Publish Key signal event with all key state*/
        AO_Publish(pKeyEvent, NULL);
    }
    else
    {
        Log(DBG, "KeypadEventHandler :Signia Event allocation error");
    }

    /* Publish appropriate signal */
    ///\todo 02/17/2022 JA - Note for the future: This signal will have to be allocated via AO_EvtNew,
    ///as it will contain the keypad image as data
    AO_Publish((void *)(&KeySig[Key][State]), NULL);
}

/* ========================================================================== */
/**
 * \brief   Logs Keypad Events  on Terminal for Debug purpose
 *
 * \details This Function Logs the keypad events on the terminal
 *
 * \param   Key           : Key Pressed identified by KEY_ID
 * \param   State         : Current Key State (Pressed/ Released)
 * \param   PublishSignal : Signal published when the key is pressed
 *
 * \return  None
 *
 * ========================================================================== */
static void Signia_PrintKeypadEvent(KEY_ID Key,  KEY_STATE State, SIGNAL PublishSignal)
{
    /* Key Data - Logging purpose */
    static uint8_t *SigniaKeypadEvent[] =
    {
        "TOGGLE_DOWN ",
        "TOGGLE_UP",
        "TOGGLE_LEFT",
        "TOGGLE_RIGHT",
        "LATERAL_LEFT_UP",
        "LATERAL_RIGHT_UP",
        "LATERAL_LEFT_DOWN",
        "LATERAL_RIGHT_DOWN",
        "SAFETY_LEFT",
        "SAFETY_RIGHT",
    };

    /* Key States - Logging purpose */
    static uint8_t *SigniaKeypadState[] =
    {
        "Released",
        "Pressed",
        "Stuck"
    };

    if (PublishSignal != NULL)
    {
        Log(DBG, " Keypad event >> %s Key %s ", SigniaKeypadEvent[Key], SigniaKeypadState[State]);
    }  
}



/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Signia_KeypadEventHandlerInit
 *
 * \details Registers Keypad Handler with the Keypad module.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_KeypadEventHandlerInit(void)
{
    /*register key change handler*/
    L4_KeypadHandlerSetup(&KeypadEventHandler);
}

/* ========================================================================== */
/**
 * \brief   Signia_RotationConfig Request Event
 *
 * \details Publishes the Rotation Config request signal.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_RotationConfigReqEvent(void)
{
    QEVENT_KEY *pRotationConfigReq;

    /* Publish the Rotation Config Press Signal r*/
    pRotationConfigReq = AO_EvtNew(P_ROTATION_CONFIG_PRESS_SIG, sizeof(QEVENT_KEY));

    Log(DBG, "Keypad: Received Rotation Configuration keys Sequence");
    AO_Publish(pRotationConfigReq, NULL);
}

/* ========================================================================== */
/**
 * \brief   Signia_RotationConfig Status update
 *
 * \details Updated the Rotation Configuration Status
 *
 * \param   eKeySide - (Left/Right)
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_UpdateRotationConfigStatus(uint8_t eKeySide)
{
    IsRotationDisabled[eKeySide] = !IsRotationDisabled[eKeySide];
}

/* ========================================================================== */
/**
 * \brief   Signia_RotationConfig Status update
 *
 * \details Updated the Rotation Configuration Status
 *
 * \param   eKeySide - (Left/Right)
 *
 * \return  bool - true if rotation is deactivated
 *                 false if rotaiton is active
 *
 * ========================================================================== */
bool Signia_GetRotationConfigStatus(uint8_t eKeySide)
{
    return (IsRotationDisabled[eKeySide]);
}

/* ========================================================================== */
/**
 * \brief   Signia_ShipMode Request Event
 *
 * \details Publishes the shipmode request signal.
 *
 * \param   reqstr -   identifies the requester of shipmode
 *                     SIGNIA_SHIPMODE_VIA_KEYPAD (0)
 *                     SIGNIA_SHIPMODE_VIA_CONSOLE (1)
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_ShipModeReqEvent(SIGNIA_SHIPMODE_REQUESTER reqstr)
{
    QEVENT_SHIP_MODE *pShipModeReq;
    /*register key change handler*/
    pShipModeReq = AO_EvtNew(P_SHIPMODE_REQ_SIG, sizeof(QEVENT_SHIP_MODE));
    pShipModeReq->Requester = reqstr;
    Log(DBG, "Detected Ship Mode request");
    AO_Publish(pShipModeReq, NULL);
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

