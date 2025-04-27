#ifndef  L4_POWERCONTROL_H
#define  L4_POWERCONTROL_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \brief   Public interface for Display Manager module.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_PowerControl.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum                    /// Power modes
{
   POWER_MODE_ACTIVE,       /// Full power mode
   POWER_MODE_STANDBY,      /// Low power mode
   POWER_MODE_SLEEP,        /// Extreme low power mode
   POWER_MODE_SHIP,         /// Ship mode
   POWER_MODE_SHUTDOWN,     /// Shutdown mode
   POWER_MODE_LAST          /// End of list
} POWER_MODE;

typedef enum                    /// Power control function call status
{
   POWER_STATUS_OK,             /// All good, no erros
   POWER_STATUS_ERROR,          /// Indicates error occured
   POWER_STATUS_INVALID_PARAM,  /// Invalid inputs
   POWER_STATUS_LAST            /// End of list

} POWER_STATUS;

typedef enum
{
   SLEEP_CAUSE_INVALID = 0,
   SLEEP_CAUSE_CHARGER,        //sleep triggered by charger
   SLEEP_CAUSE_BATTERY_CHECK,  //sleep triggered by battery monitor
   SLEEP_CAUSE_TIME,           //sleep triggered by idle timer
   SLEEP_CAUSE_COUNT
} SLEEP_CAUSE;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
POWER_STATUS Signia_PowerModeSet(POWER_MODE PowerMode);
POWER_STATUS Signia_BatDisableFault(void);
void         Signia_PowerModeSetSleepCause(SLEEP_CAUSE cause);
POWER_MODE   Signia_PowerModeGet(void);
void Signia_PowerModeSetSleepCause(SLEEP_CAUSE cause);

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_POWERCONTROL_H */

