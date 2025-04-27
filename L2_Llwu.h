#ifndef L2_LLWU_H
#define L2_LLWU_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/** * \addtogroup LLWU <if adding defines to a group>
 * \{
 * \brief Low Leakage Wakeup Unit configuration.
 *
 * \details <if necessary>
 *
 * \bug  <if necessary>
 *
 * \copyright  2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * * \file  Llwu.h
 *
* ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         // Import common definitions such as types, etc.


/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

#define LLWU_CLOCKENABLE()  SIM_SCGC4 |= SIM_SCGC4_LLWU_MASK

#define LLWU_IRQEN()        Enable_IRQ(LLWU_IRQ)
#define LLWU_IRQDIS()       Disable_IRQ(LLWU_IRQ)

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

typedef enum                    /// Wake Sources
{
    LLWU_P0,                    ///< Wakeup from External input 0
    LLWU_P1,                    ///< Wakeup from External input 1
    LLWU_P2,                    ///< Wakeup from External input 2
    LLWU_P3,                    ///< Wakeup from External input 3
    LLWU_P4,                    ///< Wakeup from External input 4
    LLWU_P5,                    ///< Wakeup from External input 5
    LLWU_P6,                    ///< Wakeup from External input 6
    LLWU_P7,                    ///< Wakeup from External input 7
    LLWU_P8,                    ///< Wakeup from External input 8
    LLWU_P9,                    ///< Wakeup from External input 9
    LLWU_P10,                   ///< Wakeup from External input 10
    LLWU_P11,                   ///< Wakeup from External input 11
    LLWU_P12,                   ///< Wakeup from External input 12
    LLWU_P13,                   ///< Wakeup from External input 13
    LLWU_P14,                   ///< Wakeup from External input 14
    LLWU_P15,                   ///< Wakeup from External input 15
    LLWU_M0IF_LPTRM,            ///< Wakeup from LPTMR Module
    LLWU_M1IF_COMP0,            ///< Wakeup from CMP0 Module
    LLWU_M2IF_COMP1,            ///< Wakeup from CMP1 Module
    LLWU_M3IF_COMP2_3,          ///< Wakeup from CMP2_3 Module
    LLWU_M4IF_TSI,              ///< Wakeup from Touch Sense input Module
    LLWU_M5IF_RTCALARM,         ///< Wakeup from RTC Alarm Module
    LLWU_M6IF_RESERVED,         ///< Reserved
    LLWU_M7IF_RTCSEC,           ///< Wakeup from RTC Seconds
    LLWU_WUPSOURCECOUNT
} LLWU_WUPSOURCE;

typedef enum                    /// WakeUp Events
{
    WAKEUP_Dummy,               ///< Dummy
    WAKEUP_RAISINGEDGE_FLAG,    ///< Wakeup on Raising edge or Module Interrupt Flag set
    WAKEUP_FALLINGGEDGE,        ///< Wakeup on Falling edge
    WAKEUP_ANYEDGE,             ///< Wakeup on both edges
    WAKEUP_COUNT
} LLWU_WUPEVENT;

typedef void (*LLWU_HANDLER)(void);  ///< LLWU interrupt callback

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

extern void L2_LlwuSetWakeupSource(LLWU_WUPSOURCE WupSource, LLWU_WUPEVENT Event);
extern void L2_LlwuClearWakeupSource(LLWU_WUPSOURCE WupSource);
extern bool L2_LlwuGetWakeupFlagStatus(uint32_t WupSource);
void L2_LlwuSetISRCallback(LLWU_HANDLER pHandler);
/**
 * \}  <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L2_LLWU_H */
