#ifndef L4_HANDLEKVF_H
#define L4_HANDLEKVF_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Kvf
 * \{
 *
 * \brief   Public interface for Clamshell Definitions.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_HandleKvf.h
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
typedef struct 
{
  float32_t GearRatio;
  INT32U TicksPerRev;
  INT32U DatalogPeriod;
} HANDLE_PARAM;
typedef enum 
{
  HANDLE_MODE_AUTO =1,
  HANDLE_MODE_EGIA,
  HANDLE_MODE_EEA,
  HANDLE_MODE_ASAP,
  HANDLE_MODE_ASAP_FLEX,
  HANDLE_MODE_ASAP_360,
  HANDLE_MODE_ASAP_PLAN,
  HANDLE_MODE_EEA_FLEX,
  HANDLE_MODE_DYNO,
  HANDLE_MODE_EGIA_REL,
  HANDLE_MODE_EEA_REL,
  HANDLE_MODE_DEBUG_REL,
  HANDLE_MODE_RDF_PLAYBACK,
  HANDLE_MODE_PIEZEO_DEMO,
  HANDLE_MODE_ABS,
  HANDLE_MODE_ABSA,
  HANDLE_MODE_HAND_CAL,
  HANDLE_MODE_REL,
  HANDLE_MODE_ES,
  HANDLE_MODE_TEST_FIXTURE
} HANDLE_MODE;

typedef enum 
{
  IS_C3_BOARD_FALSE,
  IS_C3_BOARD_TRUE,
  IS_C3_BOARD_UNKNOWN
} IS_C3_BOARD;
/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void HandleKvfInit( void );

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_HANDLEKVF_H */

