#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif
  
/* ========================================================================== */
/**
 * \addtogroup Include
 * \{
 * \brief   Public interface for signia platform modules
 *
 * \details This file has contains all signia platform symbolic constants and
 *          function prototypes.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Common.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "MisraConfig.h"
MISRAC_DISABLE
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <intrinsics.h>
#include <string.h>
#include <stdbool.h>
#include <micrium.h>
#include <stdbool.h>
#include <math.h>
#include "qpc.h"
#include "Signals.h"
#include "TaskPriority.h"
#include "Crc.h"
#include "CpuCycles.h"
#include  <cpu.h>
#include "Logger.h"
#include "Osal.h"
#include "Config.h"
MISRAC_ENABLE

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define BITS_PER_BYTE       (8u)            ///< 8 bits per byte
#define BYTE_MASK           (0xFFu)         ///< mark to extract lower byte
#define LOW_NIBBLE_MASK     (0x0Fu)         ///< Lower nibble mask
#define NUL                 ('\0')          ///< Use this for ordinal 0

#define UINT8_MAX_VALUE     (0xFFu)         ///< Unsigned int  8 maximum value.
#define UINT16_MAX_VALUE    (0xFFFFu)       ///< Unsigned int 16 maximum value.
#define UINT32_MAX_VALUE    (0xFFFFFFFFu)   ///< Unsigned int 32 maximum value.

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define BREAK_IF(Cond)      {if(Cond)break;}

// Default system start time in UTC format corresponding to 2013-07-24 12:25:36 GMT.
#define SIGNIA_RTC_DEFAULT_VALUE (1374668736u)

#define TICKS_TO_MICROSECONDS(u32Ticks)   ((u32Ticks)/(60u))    ///< 1 micro seconds = 60 ticks
#define SigTime()           OSTimeGet()     ///< API to get UTC milliseconds
#define SigTimeSet(m)       OSTimeSet(m)    ///< API to set UTC milliseconds

#ifdef DEBUG_CODE
    /* Debug mode features */
    #define TEST_STUBS             // To Enable/Disable Test stub functions
    #define dprintf(x,...)         Log_Msg(DBG, LOG_GROUP_GENERAL, __LINE__, x)
#else   
    /* Release mode features */
    #define dprintf(x,...)          ///< Define dprintf to nothing
#endif

#ifdef __ARMVFP__
    #define  OS_TASK_OPTIONS    (OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR | OS_TASK_OPT_SAVE_FP)
#else
    #define  OS_TASK_OPTIONS    (OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR)
#endif

/* Generic Time Values */
#define MSEC_PER_SEC    (1000UL)
#define SEC_PER_MIN     (60UL)
#define MIN_PER_HOUR    (60UL)

#define OS_NO_DELAY     (0UL)

#define MSEC_1          (1UL)
#define MSEC_2          (2UL)
#define MSEC_3          (3UL)
#define MSEC_4          (4UL)
#define MSEC_5          (5UL)
#define MSEC_10         (10UL)
#define MSEC_15         (15UL)
#define MSEC_20         (20UL)
#define MSEC_30         (30UL)
#define MSEC_40         (40UL)
#define MSEC_50         (50UL)
#define MSEC_60         (60UL)
#define MSEC_70         (70UL)
#define MSEC_80         (80UL)
#define MSEC_90         (90UL)
#define MSEC_100        (100UL)
#define MSEC_200        (MSEC_100*2)
#define MSEC_250        (MSEC_50*5)
#define MSEC_300        (MSEC_100*3)
#define MSEC_400        (MSEC_100*4)
#define MSEC_500        (MSEC_100*5)
#define SEC_1           (MSEC_100*10)
#define SEC_2           (SEC_1*2)
#define SEC_3           (SEC_1*3)
#define SEC_4           (SEC_1*4)
#define SEC_5           (SEC_1*5)
#define SEC_6           (SEC_1*6)
#define SEC_7           (SEC_1*7)
#define SEC_8           (SEC_1*8)
#define SEC_9           (SEC_1*9)
#define SEC_10          (SEC_1*10)
#define SEC_13          (SEC_1*13)
#define SEC_15          (SEC_1*15)
#define SEC_20          (SEC_1*20)
#define SEC_25          (SEC_1*25)
#define SEC_30          (SEC_1*30)
#define SEC_40          (SEC_1*40)
#define SEC_45          (SEC_1*45)
#define SEC_50          (SEC_1*50)
#define SEC_60          (SEC_1*60)
#define SEC_70          (SEC_1*70)
#define SEC_80          (SEC_1*80)
#define SEC_90          (SEC_1*90)
#define MIN_1           (SEC_1*60)
#define MIN_2           (MIN_1*2)
#define MIN_3           (MIN_1*3)
#define MIN_4           (MIN_1*4)
#define MIN_5           (MIN_1*5)
#define MIN_6           (MIN_1*6)
#define MIN_7           (MIN_1*7)
#define MIN_8           (MIN_1*8)
#define MIN_9           (MIN_1*9)
#define MIN_10          (MIN_1*10)
#define MIN_15          (MIN_1*15)
#define MIN_18          (MIN_1*18)
#define MIN_20          (MIN_1*20)
#define MIN_25          (MIN_1*25)
#define MIN_30          (MIN_1*30)
#define MIN_60          (MIN_1*60)
#define HOUR_1          (MIN_60)
#define HOUR_2          (HOUR_1*2)
#define HOUR_3          (HOUR_1*3)
#define HOUR_4          (HOUR_1*4)
#define HOUR_5          (HOUR_1*5)
#define HOUR_6          (HOUR_1*6)
#define HOUR_7          (HOUR_1*7)
#define HOUR_8          (HOUR_1*8)
#define HOUR_9          (HOUR_1*9)
#define HOUR_10         (HOUR_1*10)
#define HOUR_24         (HOUR_1*24)

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef volatile unsigned char vuint8_t;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* COMMON_H */

