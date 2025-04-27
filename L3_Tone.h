#ifndef L3_TONE_H
#define L3_TONE_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_Tone
 * \{
 * \brief   Public interface for Tone.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_Tone.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"               ///< Import common definitions such as types, etc
#include "Logger.h"
#include "L3_Fpga.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define MAX_TONE_NAME_LEN     (30u)         ///< Max length for tone name
/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/*! \struct ToneNote
 *  Tone Note structure [A single Note].
 */
typedef struct
{
  uint16_t Frequency;                   ///< Frequency – Frequency in Hz
  uint16_t Duration;                   ///< Duration – Duration in mS
}ToneNote;

/*! \struct Tone
 *  Tone structure [Group of Notes].
 */
typedef struct
{
  const ToneNote *pToneNotes;              ///< pToneNotes – Pointer to the Notes
  uint8_t  ToneName[MAX_TONE_NAME_LEN];    ///< ToneName – Tone Name for Logging
}Tone;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void L3_TonePlay(Tone *pTone);
extern bool L3_IsToneActive(void);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_TONE_H */
