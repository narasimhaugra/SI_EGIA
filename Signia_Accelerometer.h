#ifndef L4_ACCELEROMETER_H
#define L4_ACCELEROMETER_H

#ifdef __cplusplus      /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_Accelerometer
 * \{
 * \brief   Interface for Accelerometer.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2019 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_Accelerometer.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         // Import common definitions such as types, etc.

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
/* None */

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

///\todo to replace this with a typedef for timestamp value which can be used systemwide.
typedef uint32_t TIMESTAMP;       ///< Timestamp in Unix UTC(32 bit seconds counter) format.

typedef enum                      /// Movement detection module states
{
    ACCEL_STATE_DISABLE,
    ACCEL_STATE_ENABLED
} ACCEL_STATE;

typedef enum                  /// Accelerometer sense events.
{
    ACCEL_EVENT_IDLE,
    ACCEL_EVENT_MOVING,       ///< Event used to indicate Movement of handle.
    ACCEL_EVENT_DROP,         ///< Event used to indicate Drop of handle.
    ACCEL_EVENT_PERIODIC,     ///< Event used to indicate the accel values coming from Periodic timer.
    ACCEL_EVENT_LAST
} ACCEL_EVENT;

typedef enum                     /// Movement detection module API return status.
{
    ACCEL_STATUS_OK,
    ACCEL_STATUS_PARAM_ERROR,
    ACCEL_STATUS_ERROR,
    ACCEL_STATUS_NO_INFO,
    ACCEL_STATUS_LAST
 } ACCEL_STATUS;

typedef struct                     /// Accelerometer data.
{
    int16_t x_axis;                ///< x axis data
    int16_t y_axis;                ///< y axis data
    int16_t z_axis;                ///< z axis data
    TIMESTAMP Time;                ///< Data capture timestamp.
} AXISDATA;

typedef struct                     /// Accelerometer data, containing Axis information and the Accel Event.
{
    ACCEL_EVENT Event;
    AXISDATA Data;
} ACCELINFO;

typedef void (*ACCEL_CALLBACK)( ACCELINFO* );    ///< Callback function to report accelerometer information

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

ACCEL_STATUS L4_AccelInit(void);
ACCEL_STATUS Signia_AccelEnable(bool Enable, uint32_t Duration, ACCEL_CALLBACK pHandler);
ACCEL_STATUS Signia_AccelSetThreshold(uint16_t MoveThreshold, uint16_t DropThreshold );
ACCEL_STATUS Signia_AccelGetAxisData(AXISDATA *pAxisData);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_ACCELEROMETER_H */
