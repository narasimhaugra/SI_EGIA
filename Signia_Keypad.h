#ifndef SIGNIA_KEYPAD_H
#define SIGNIA_KEYPAD_H

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

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
 /*! \enum KEY_ID */
typedef enum
{
  TOGGLE_DOWN = 0,
  TOGGLE_UP,
  TOGGLE_LEFT,
  TOGGLE_RIGHT,
  LATERAL_LEFT_UP,
  LATERAL_RIGHT_UP,
  LATERAL_LEFT_DOWN,
  LATERAL_RIGHT_DOWN,
  SAFETY_LEFT,
  SAFETY_RIGHT,
  KEY_COUNT
} KEY_ID;

// Key states
typedef enum
{
  KEY_STATE_RELEASE,
  KEY_STATE_PRESS,
  KEY_STATE_STUCK
} KEY_STATE;

typedef void (*KEYPAD_PATTERN_HANDLER)(void);                ///< Keypad pattern match event handler function
typedef void (*KEYPAD_HANDLER)( KEY_ID Key, KEY_STATE State, uint16_t KeyState);///< Keypad event handler function

//Keypad function return status
typedef enum
{
   KEYPAD_STATUS_OK,                  ///< match found 
   KEYPAD_STATUS_ERROR,               ///< error 
   KEYPAD_STATUS_MATCH_COMPLETE,      ///< pattern sequence match found   
   KEYPAD_STATUS_MATCH_IN_PROGRESS,   ///< partial match found   
   KEYPAD_STATUS_LAST                 ///< enum end marker
} KEYPAD_STATUS;

// Key pattern definition with key state combination and hold duration
typedef struct
{
   uint16_t KeySet;         ///< Key pattern/key set
   uint32_t DurationMin;    ///< Key combination active duration minimum.
   uint32_t DurationMax;    ///< maximum duration to check for next pattern
   bool     ActOnRelease;   ///< bit to decide key pattern check after key press or release
} KEY_PATTERN;

// Key pattern register structure with call back handler
typedef struct
{
    uint8_t KeySetNumber;             ///< this is used to check next key pattern in a key pattern sequence
    KEYPAD_PATTERN_HANDLER phandler;  ///< handler to call upon match
    const KEY_PATTERN *pKeyPattern;   ///< pointer to key pattern struct 
    uint32_t DetectTimeout;           ///< keypattern detection timout
    uint32_t ValidMinTime;            ///< Minimum time the pattern should be stable 
    uint16_t PreviousKeySet;          ///< Previous Keyset for pattern matching
    bool MinStableDurationTimerFlag;  ///< Minimum duration variable to detect stable key press  
} keyPatternWatch;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern  KEYPAD_STATUS   L4_KeypadInit(void);
extern  KEY_STATE       L4_KeypadGetKeyState(KEY_ID Key);
extern  KEYPAD_STATUS   L4_KeypadHandlerSetup(KEYPAD_HANDLER pHandler);
extern  KEYPAD_STATUS   L4_KeypadWatchPattern(KEY_PATTERN const *pKeyPattern, KEYPAD_PATTERN_HANDLER pPatternHandler);
extern  KEYPAD_STATUS   L4_KeypadUnwatchPattern(KEY_PATTERN const *pKeyPattern);
extern  KEYPAD_STATUS   Signia_StartShipModePatternwatch(void);
extern  void            Signia_StopShipModePatternwatch(void);
extern  void            Signia_KeypadEventHandlerInit(void);
extern  void            Signia_KeypadScanPause(void);
extern  void            Signia_KeypadScanResume(void);
extern  KEYPAD_STATUS   Signia_StartRotationConfigPatternWatch(void);
extern  void            Signia_StopRotationConfigPatternWatch(void);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SIGNIA_KEYPAD_H */
