#ifndef L2_lPTMR_H
#define L2_LPTMR_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/** * \addtogroup FPGA <if adding defines to a group>
 * \{
 * \brief Brief Description.
 *
 * \details <if necessary>
 *
 * \bug  <if necessary>
 *
 * \copyright  2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * * \file  Fpga.h
 *
* ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         // Import common definitions such as types, etc.


/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/


/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

typedef  void (*LPTMR_EVT_HNDLR)(void);             // LPTMR expiry callback function

typedef enum {                  /// LPTMR status
    LPTMR_STATUS_STOPPED,       ///< LPTMR is not running
    LPTMR_STATUS_RUNNING,       ///< LPTMR is running
    LPTMR_STATUS_DISABLED,      ///< LPTMR disabled
    LPTMR_STATUS_ERROR,         ///< Error Status
    LPTMR_STATUS_LAST           ///< LPTMR status ranage indicator
}LPTMR_STATUS;

typedef enum {                  /// LPTMR mode
    LPTMR_MODE_TIME,            ///<  Time Counter mode
    LPTMR_MODE_PULSE,           ///<  Pulse Counter mode.
    LPTMR_MODE_LAST
}LPTMR_MODE;

typedef enum {                  /// LPTMR Clock sources
    LPTMR_INTCLK,               ///< LPTMR internal clock
    LPTMR_LPO1KHZ,              ///< LPTMR LPO 1kHz
    LPTMR_ERCLK32K,             ///< LPTMR External 32KHz
    LPTMR_OSC0ERCLK,            ///< LPTMR External Reference clock
    LPTMR_CLKSRCLAST
}LPTMR_CLKSOURCE;

typedef enum {          /// Input pins for Pulse Counter mode
    LPTMR_CMP0,         ///< CMP0 Output
    LPTMR_LPTMR_ALT1,   ///< LPTMR_ALT1 pin
    LPTMR_LPTMR_ALT2,   ///< LPTMR_ALT2 pin
    LPTMR_INPUTCOUNT
}LPTMR_PULSECNTRINP;

typedef enum {                  /// LPTMR Prescalar values
    LPTMR_PRESCALAR_DIV2,       ///< Division by 2
    LPTMR_PRESCALAR_DIV4,       ///< Division by 4
    LPTMR_PRESCALAR_DIV8,       ///< Division by 8
    LPTMR_PRESCALAR_DIV16,      ///< Division by 16
    LPTMR_PRESCALAR_DIV32,      ///< Division by 32
    LPTMR_PRESCALAR_DIV64,      ///< Division by 64
    LPTMR_PRESCALAR_DIV128,     ///< Division by 128
    LPTMR_PRESCALAR_DIV256,     ///< Division by 256
    LPTMR_PRESCALAR_DIV512,     ///< Division by 512
    LPTMR_PRESCALAR_DIV1024,    ///< Division by 1024
    LPTMR_PRESCALAR_DIV2048,    ///< Division by 2048
    LPTMR_PRESCALAR_DIV4096,    ///< Division by 4096
    LPTMR_PRESCALAR_DIV8192,    ///< Division by 8192
    LPTMR_PRESCALAR_DIV16384,   ///< Division by 16384
    LPTMR_PRESCALAR_DIV32768,   ///< Division by 32768
    LPTMR_PRESCALAR_DIV65536,   ///< Division by 65536
    LPTRM_PRESCALARCOUNT
}LPTMR_PRESCALAR;


typedef struct                          ///  Periodic LPTMR Control structure
{
    LPTMR_MODE          Mode;           ///< LPTMR mode, One shot or Periodic
    LPTMR_CLKSOURCE     ClkSource;      ///< LPTMR Clocksource
    LPTMR_PULSECNTRINP  Inputpin;       ///< LPTMR Pulse input pin, considered only if the Mode is set to pulse
    LPTMR_PRESCALAR     Prescalar;      ///< Prescalar value
    uint32_t            Value;          ///< LPTMR Compare value
    LPTMR_EVT_HNDLR     pHandler;       ///< LPTMR expiry call back handler
}LptmrControl;

typedef void (*LPTMR_HANDLER)(void);    ///< LPTMR ISR callback typedef

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void L2_LptmrInit(void);
extern LPTMR_STATUS L2_LptmrConfig(LptmrControl *pTmrConfig);
extern void L2_LptrmStart(void);
extern void L2_LptrmStop(void);


/**
 * \}  <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L2_lPTMR_H */
