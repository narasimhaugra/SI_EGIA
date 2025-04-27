#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/** \addtogroup LLWU
 * \{
 * \brief   Low Leakage Wakeup Unit Routines
 *
 * \details This module provides an interface to the Low Leakage Wakeup Unit hardware
 *
 * \sa K20 SubFamily Reference Manual.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file L2_Llwu.c
 *
 * ========================================================================== */


/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L2_Llwu.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/



/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/

#define ISBITSET(x,pos)   (x & (1<<pos))?true:false
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef struct                          /// Wakeup unit Register
{
    LLWU_WUPSOURCE      Source;         ///< Wakeup Source
    volatile uint8_t   *pu32RegEn;      ///< Wakeup Enable Register
    volatile uint8_t   *pu32RegWupFlag; ///< Wakeup Flag
    uint8_t             Offset;         ///< Offset in Enable Register
    uint8_t             FlagOffset;     ///< Offset for Wakeup Flag
}LLWU_WUPSOURCE_REG_TABLE_T;
/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
static const LLWU_WUPSOURCE_REG_TABLE_T WupRegTable[LLWU_WUPSOURCECOUNT] =
{
    /*Source,            pu32RegEn,        pu32RegWupFlag,      Offset       FlagOffset*/
    LLWU_P0,             &LLWU_PE1,           &LLWU_F1,          0,             0,
    LLWU_P1,             &LLWU_PE1,           &LLWU_F1,          2,             1,
    LLWU_P2,             &LLWU_PE1,           &LLWU_F1,          4,             2,
    LLWU_P3,             &LLWU_PE1,           &LLWU_F1,          6,             3,
    LLWU_P4,             &LLWU_PE2,           &LLWU_F1,          0,             4,
    LLWU_P5,             &LLWU_PE2,           &LLWU_F1,          2,             5,
    LLWU_P6,             &LLWU_PE2,           &LLWU_F1,          4,             6,
    LLWU_P7,             &LLWU_PE2,           &LLWU_F1,          6,             7,
    LLWU_P8,             &LLWU_PE3,           &LLWU_F2,          0,             0,
    LLWU_P9,             &LLWU_PE3,           &LLWU_F2,          2,             1,
    LLWU_P10,            &LLWU_PE3,           &LLWU_F2,          4,             2,
    LLWU_P11,            &LLWU_PE3,           &LLWU_F2,          6,             3,
    LLWU_P12,            &LLWU_PE4,           &LLWU_F2,          0,             4,
    LLWU_P13,            &LLWU_PE4,           &LLWU_F2,          2,             5,
    LLWU_P14,            &LLWU_PE4,           &LLWU_F2,          4,             6,
    LLWU_P15,            &LLWU_PE4,           &LLWU_F2,          6,             7,
    LLWU_M0IF_LPTRM,     &LLWU_ME,            &LLWU_F3,          0,             0,
    LLWU_M1IF_COMP0,     &LLWU_ME,            &LLWU_F3,          1,             1,
    LLWU_M2IF_COMP1,     &LLWU_ME,            &LLWU_F3,          2,             2,
    LLWU_M3IF_COMP2_3,   &LLWU_ME,            &LLWU_F3,          3,             3,
    LLWU_M4IF_TSI,       &LLWU_ME,            &LLWU_F3,          4,             4,
    LLWU_M5IF_RTCALARM,  &LLWU_ME,            &LLWU_F3,          5,             5,
    LLWU_M6IF_RESERVED,  &LLWU_ME,            &LLWU_F3,          6,             6,
    LLWU_M7IF_RTCSEC,    &LLWU_ME,            &LLWU_F3,          7,             7
};
/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/


/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static LLWU_HANDLER pLlwuHandler = NULL;
/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief       The fucntion sets the Wakeup source and the trigger for wakeup
 *
 * \details     Wakeup source is selected and the trigger of wakeup is set by
 *              setting either LLWU_PEx or LLWU_ME appropriately
 *
 * \param[in]   WupSource - Wakeup source
 *
 * \param[in]   Event - trigger for wakeup Falling edge or Raisign edge or Flag status
 *
 * \return      None
 *
 * ========================================================================== */
void L2_LlwuSetWakeupSource(LLWU_WUPSOURCE WupSource, LLWU_WUPEVENT Event)
{
    *(WupRegTable[WupSource].pu32RegEn) |= (Event << WupRegTable[WupSource].Offset);
}

/* ========================================================================== */
/**
 * \brief       The fucntion clears the Weakeup source
 *
 * \details     Dissables the Wakeup source
 *
 * \param[in]   WupSource - Wakeup source that needs to be dissabled
 *
 * \return      None
 *
 * ========================================================================== */

void L2_LlwuClearWakeupSource(LLWU_WUPSOURCE WupSource)
{
    *(WupRegTable[WupSource].pu32RegEn) &= ~(3 << WupRegTable[WupSource].Offset);
}

/* ========================================================================== */
/**
 * \brief       The fucntion returns the status of Wakeup flag
 *
 * \details     The wakeup flag corresponding to the Wakeup Source will be returned
 *
 * \param       WupSource - Wakeup source
 *
 * \return      Status of Wake up flag
 *
 * ========================================================================== */
bool L2_LlwuGetWakeupFlagStatus(uint32_t WupSource)
{
    bool Status;

    Status = *(WupRegTable[WupSource].pu32RegWupFlag) & (1 << WupRegTable[WupSource].FlagOffset)? true:false;
    return Status;
}

/* ========================================================================== */
/**
 * \brief   LLWU ISR
 *
 * \details This is LLWU interrupt service routine.
 *
 * \note    This function should be defined according RTOS ISR template.
 *
 * \param	< None >
 *
 * \return	None
 *
 * ========================================================================== */
void L2_Llwu_ISR(void)
{
    OS_CPU_SR  cpu_sr;

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    // clear the wakeup flag if set
    if(LLWU_F1 & LLWU_F1_WUF5_MASK){
        LLWU_F1 |= LLWU_F1_WUF5_MASK;
    }
    if(LLWU_F1 & LLWU_F1_WUF7_MASK){
        LLWU_F1 |= LLWU_F1_WUF7_MASK;
    }

    if(NULL != pLlwuHandler)
    {
        (*pLlwuHandler)();     // Invoke registered handler
    }

    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   LLWU Register callback
 *
 * \details The function registers the callback function called from LLWU ISR
 *
 * \param	< None >
 *
 * \return	None
 *
 * ========================================================================== */
void L2_LlwuSetISRCallback(LLWU_HANDLER pHandler)
{
    pLlwuHandler = pHandler;
}


/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
