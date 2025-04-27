#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/** \addtogroup LPTMR
 * \{
 * \brief  Low power Timer module routines
 *
 * \details This module provides an interface to the LowPowerTimer hardware
 *
 * \sa      K20 SubFamily Reference Manual.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file L2_Lptmr.c
 *
 * ========================================================================== */


/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L2_Lptmr.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static LPTMR_HANDLER pLptmrHandler;  ///< Interrupt Callback for LPTMR interrupt
/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/


/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief  The LPTMR init fucntion
 *
 * \details The fucntion initiliazes the LPTMR by resetting the flags
 *
 * \param[in]  < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_LptmrInit(void)
{
    pLptmrHandler = NULL;
    L2_LptrmStop();
}
/* ========================================================================== */
/**
 * \brief  The function configures the Lowpower timer
 *              for now we are supporing only the Timer counter mode
 *
 * \details For now the config only supports the Timer mode, Pulse counter mode is not supported
 *              It configures the Compare registers, set the prescalar and clocksource
 *              The timer is in stop even after configuring and needs to be started by start api
 *
 * \param[in]  *pTmrConfig - Pointer to Timer configuration structure
 *
 * \return  Status of the LPTMR
 *
 * ========================================================================== */
LPTMR_STATUS L2_LptmrConfig(LptmrControl *pTmrConfig)
{
    LPTMR_STATUS Status;

    Status = LPTMR_STATUS_ERROR;

    do
    {
        BREAK_IF(LPTMR_MODE_TIME != pTmrConfig->Mode); // Currently the configuration is supported for LPTMR timer mode only
        LPTMR0_CMR = LPTMR_CMR_COMPARE(pTmrConfig->Value);
        // Select prescaler clock to LPO - 1kHz clock
        LPTMR0_PSR = LPTMR_PSR_PRESCALE(pTmrConfig->Prescalar) | LPTMR_PSR_PCS(pTmrConfig->ClkSource); // Select prescaler clock to LPO - 1kHz clock
        pLptmrHandler = pTmrConfig->pHandler;
        Status = LPTMR_STATUS_DISABLED;
    } while (false);

    return Status;
}


/* ========================================================================== */
/**
 * \brief Start Low power timer
 *
 * \details The function does the following
 *              1. Enables Timer interrupt
 *              2. Enables Timer
 *              3. Clears the Interrupt flag
 *
 * \param < None >
 *

 * \return  None
 *
 * ========================================================================== */
void L2_LptrmStart(void)
{
    uint32_t CsrReg;

    CsrReg = LPTMR0_CSR;
    L2_LptrmStop();

    CsrReg |= LPTMR_CSR_TIE_MASK; // Enable the LPTMR interrupt
    CsrReg |= LPTMR_CSR_TEN_MASK;   // Enable the LPTMR
    // We're on the charger, so enable the LPTMR to wake and check the battery level
    LPTMR0_CSR = CsrReg;
    LPTMR0_CSR |= LPTMR_CSR_TCF_MASK;
    Enable_IRQ(L2_LPTMR_IRQ);

}


/* ========================================================================== */
/**
 * \brief Stop Low Power Timer
 *
 * \details The function does the following
 *              1. Clears the Interrupt flagX
 *              2. Clears the Interrupt Enable bit
 *              3. Dissables Low Power timer
 * \param < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_LptrmStop(void)
{
    // Clear Low power timer interrupt flag
    LPTMR0_CSR |= LPTMR_CSR_TCF_MASK;
    // We're on the charger, so enable the LPTMR to wake and check the battery level
    LPTMR0_CSR &= ~(LPTMR_CSR_TIE_MASK |       // Disable the LPTMR interrupt
                    LPTMR_CSR_TEN_MASK);       // Disable the LPTMR

}


/* ========================================================================== */
/**
 * \brief   LPTMR ISR
 *
 * \details This is one time LPTMR Compare match interrupt service routine.
 *
 * \note    This function should be defined according RTOS ISR template.
 *
 * \param       < None >
 *
 * \return	None
 *
 * ========================================================================== */
void L2_Lptmr_ISR(void)
{
    OS_CPU_SR  cpu_sr;

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    if(LPTMR0_CSR & LPTMR_CSR_TCF_MASK){
        LPTMR0_CSR &= ~LPTMR_CSR_TIE_MASK;
        LPTMR0_CSR |= LPTMR_CSR_TCF_MASK;
    }

    if(NULL != pLptmrHandler)
    {
        (*pLptmrHandler)();     // Invoke registered handler
    }

    OSIntExit();
}

/**
 * \}   <if using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
