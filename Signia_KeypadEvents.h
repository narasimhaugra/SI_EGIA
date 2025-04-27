#ifndef SIGNIA_KEYPADEVENTS_H
#define SIGNIA_KEYPADEVENTS_H

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
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_Keypad.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"                      /*! Import common definitions such as types, etc */
#include "Signia_Keypad.h"
/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum                 ///< Keypad structure identifying the Side
{
    KEY_SIDE_LEFT,           ///< Left side of the forward grip on signia Handle
    KEY_SIDE_RIGHT,          ///< Right side of the forward grip on signia Handle
    KEY_SIDE_UNDEFINED       ///< Defined for Keys not requiring to idenfy with a Key Side
} KEY_SIDE;

/// Key press/release event
typedef struct
{
    QEvt Event;          ///< QPC event header
    KEY_ID Key;          ///< Key ID
    KEY_STATE State;     ///< Key State
    uint8_t KeySide;     ///< Key Side
    uint16_t KeyState;   ///< current Status of all the keys
} QEVENT_KEY;

typedef enum
{
    SIGNIA_KEYPAD_STATUS_OK,              ///< Status OK
    SIGNIA_KEYPAD_STATUS_INVALID_PARAM,   ///< Invalid Parammeter
    SIGNIA_KEYPAD_STATUS_ERROR,           ///< Error Status
    SIGNIA_KEYPAD_STATUS_LAST             ///< End of status
} SIGNIA_KEYPAD_EVENTS_STATUS;

typedef struct
{
   QEvt              Event;       ///< QPC event header
   uint8_t           Requester;  ///< Keypad event request coming from
} QEVENT_SHIP_MODE;

typedef enum
{
    SIGNIA_SHIPMODE_VIA_KEYPAD,  ///< Ship Mode request via keypad
    SIGNIA_SHIPMODE_VIA_CONSOLE  ///< Ship Mode request via CONSOLE
} SIGNIA_SHIPMODE_REQUESTER;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void Signia_RotationConfigReqEvent(void);
extern void Signia_UpdateRotationConfigStatus(uint8_t eKeySide);
extern bool Signia_GetRotationConfigStatus(uint8_t eKeySide);
extern void Signia_ShipModeReqEvent(SIGNIA_SHIPMODE_REQUESTER requester);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SIGNIA_KEYPADEVENTS_H */

