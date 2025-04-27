#ifndef PIDCONTROLLER_H
#define PIDCONTROLLER_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \brief   Public interface for PID module.
 *
 * \details This file contains the enumerations as well as the function prototypes
 *          for the PID module
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
#define PID_OUTPUT_MAX          (1.0f)       ///< Maximum PID value
#define PID_OUTPUT_MIN          (0.0f)       ///< Minimum PID value
#define PID_INTEGRATOR_HIGH     (1.0f)       ///< Maximum Integrator value
#define PID_INTEGRATOR_LOW      (0.0f)       ///< Minimum Integrator value
#define PID_MAX_INTERP_ENTRIES  (10u)        ///< Maximum interpolate entries
#define PID_MAX_TABLE_SIZE      (2u)         ///< Maximum number of PID tables

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef struct                                           ///< PID Controller
{
  float32_t  Output;                                     ///< Output of PID controller
  float32_t  OutputMax;                                  ///< Output clamp high value
  float32_t  OutputMin;                                  ///< Output clamp low value
  float32_t  Error;                                      ///< Error
  float32_t  ErrorSum;                                   ///< Summation Error
  float32_t  ErrorDiff;                                  ///< Differential Error
  float32_t  Kp;                                         ///< Proportional gain
  float32_t  Ki;                                         ///< Integral gain
  float32_t  Kd;                                         ///< Derivative gain
  float32_t  IntegratorHighClamp;                        ///< Integral clamp high value
  float32_t  IntegratorLowClamp;                         ///< Integral clamp low value
} PID;

typedef struct                                           ///< PID Interpolate table
{
  uint32_t    DataInput[PID_MAX_INTERP_ENTRIES];         ///< Data Input
  float32_t   Proportional[PID_MAX_INTERP_ENTRIES];      ///< Proportional values
  float32_t   Integral[PID_MAX_INTERP_ENTRIES];          ///< Integral values
  float32_t   Differential[PID_MAX_INTERP_ENTRIES];      ///< Differential values
  uint8_t     Taps[PID_MAX_INTERP_ENTRIES];              ///< Number of filter taps
  uint32_t    RpmThreshold[PID_MAX_INTERP_ENTRIES];      ///< Minimum speed error threshold for correction
} PID_INTERP_TABLE;

typedef struct
{
  PID_INTERP_TABLE   *PidDataTable[PID_MAX_TABLE_SIZE];   ///< PID data tables
} PID_TABLES;

typedef struct                                            ///< Motor PID Controller
{
  uint8_t    TableId;                                     ///< Table
  uint32_t   DataInput;                                   ///< Data input
  PID_TABLES PidInterpTables;                             ///< PID Interpolation table
} PID_TABLE_DATA;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void PidInit(PID *pPid, float32_t Kp, float32_t Ki, float32_t Kd);
extern void PidReset(PID *pPID, float32_t OutputMax, float32_t OutputMin, float32_t IntegratorHighClamp, float32_t IntegratorLowClamp);
extern void PidController(PID *pPID, float32_t Error);
extern void PidInterpolation(PID_TABLE_DATA *TableData, float32_t *Kp, float32_t *Ki, float32_t *Kd);
extern void PidSetTapsRpmThreshold(PID_TABLE_DATA *pTableData, uint8_t *pTaps, uint32_t *pRpmThresh);

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif  /* PIDCONTROLLER_H */
