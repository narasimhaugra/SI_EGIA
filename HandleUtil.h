#ifndef HANDLEUTIL_H
#define HANDLEUTIL_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Handle
 * \{
 *
 * \brief   Prototypes for Handle helper functions
 *
 * \details These are the prototypes for handle helper routines.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    HandleUtil.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Signia.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

#define BAT_READ_TIMEOUT_ONCHARGER                      (1000u)     // Timeout to Read Battery Charge count cycle
#define IDLE_TIME_BEFORE_STANDBY_NO_CLAMSHELL           (SEC_30)    // Idle time without clamshell connected
#define IDLE_TIME_BEFORE_STANDBY_WITH_CSHELL_ADAPTER    (MIN_15)    // Idle time with Clamshell or With Adapter connected
#define STANDBY_BEFORE_SLEEP_ONCHARGER_NOUSB            (2 * MIN_1) // Standby time before sleep when oncharger
#define IDLE_TIME_TO_SLEEP                              (MIN_15 )   // Idle time in standby mode before entering sleep mode
#define ADAPT_COMPAT_SCREEN_DUR_ON_STARTUP              (SEC_15)    // Adapter compatible screen show duration
#define ADAPT_COMPAT_SCREEN_DUR_RUNTIME                 (1u)       // Adapter compatible screen show duration
#define BATT_INSUFF_WITHOUT_CS_TIMEOUT                  (500u)      // Idle time without adapter connected
#define BATT_LOW_WITHOUT_CS_TIMEOUT                     (500u)      // Battery Low time out with out clamshell
#define BATT_TEMP_HI_LIMIT                              (70)
#define BATT_TEMP_LO_LIMIT                              (-20)
#define CHARGER_IDLEMODE_TIMEOUT                        (HOUR_3 + MIN_30) // Idle Mode Charger timeout
#define MAX_CHARGING_COUNT                              (300u)
#define REQRESET_SEQA_TIMEOUT                           (5000u)     // Req Reset Seq A timeout 5 seconds
#define REQRESET_SEQSCREEN_TIMEOUT                      (500u)      // Req Reset Screen Sequence timeout 500ms
#define MAX_REQRST_SCREENSDISP                          (4u)
#define ADAPT_REQUEST_SCREEN_DUR                        (1000u)     // Adapter Request screen show duration
#define DEV_ADDR_LENGTH                                 (8u)        // Device Address Length
#define ZERO_BATTERYCHARGE_CYCLECOUNT                   (0u)        // Handle EOL Count
#define BAT_CHRG_CNT_WARNING                            (295u)      // Battery Charge Cycle Count Warning  \todo 10122022 KIA  remove?

// Motor test constants:
#define MOTEST_SPD      (1000u)     ///< Speed to move motor for test
#define MOTEST_POS      (600u)      ///< Position to move motor to for test
#define MOTEST_CUR_LIM  (0x1FF)     ///< Motor current limit (PWM) for test
#define MOTEST_TIMEOUT  (6000u)     ///< Maximum time allowed for move (mSec)
#define MOTEST_DELAY    (30u)       ///< Motor startup delay (mSec)
#define MOTEST_CUR_TRIP (4000u)     ///< Motor test current trip

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/


/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/


/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/


/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void HNutil_ProcessAccelEvents(ACCELINFO *pAccelInfo);
extern void HNutil_ProcessChargerEvents(CHARGER_INFO *pChargerInfo);
extern void HNutil_SystemClockUpdate(void);
extern void HNutil_ProcessDeviceConnEvents(Handle * const pMe, QEvt const * const pSig);
extern void HNutil_UpdateHeartBeatPeriod(QEvt const * const pSig);
extern void HNutil_DisplayRequestScreenSeq(Handle *const pMe);
extern void HNutil_RotationConfigDisplayCountDownScreens(Handle *const pMe);
extern void HNutil_StartMotorTest(Handle *const pMe, QEvt const *const pSig);
extern AM_STATUS HNutil_InitAdapterComm(Handle * const pMe);
extern void Signia_BatteryUpdateErrors(CHARGER_INFO *pInfo, Handle *const pMe, bool ReloadConnected);


/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* APPSTUBS_H */
