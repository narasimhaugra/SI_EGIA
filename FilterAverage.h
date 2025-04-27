#ifndef FILTERAVERAGE_H
#define FILTERAVERAGE_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \brief   Public interface for Motor current averaging routines.
 *
 * \details This file contains the enumerations as well as the function prototypes
 *          for the Motor current filtering routines
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define FILTER_SIZE_MAX                 (128u)      ///< Maximum velocity filter size
#define CURRENT_FILTER_SIZE             (10u)       ///< Current filter size

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef struct                ///< Averaging filter
{
  int16_t  Output;            ///< Output value
  uint8_t  Index;             ///< Index
  uint8_t  Length;            ///< Filter Lenghth
  int32_t  SumData;           ///< Sum of data samples
  int16_t  *pData;            ///< Pointer to the filter data
} FILTER_AVERAGE;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void FilterAverage(FILTER_AVERAGE *pFilter, int16_t Sample);
extern void FilterAverageExcludeMinMax(FILTER_AVERAGE *pFilter);
extern void FilterAverageInit(FILTER_AVERAGE *pFilter, int16_t  *pData, uint8_t Length);

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif  /* FILTERAVERAGE_H */
