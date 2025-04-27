#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_OnchipRtc
 * \{
 * \brief   On-Chip RTC routines
 *
 * \details This file contains MCU On-Chip RTC functionality implementation.
 *          On-Chip RTC is used as one of the real time clock information
 *          source in the system which is volatile in nature i.e. clock information
 *          is lost when the power is removed. The On-Chip RTC clock needs to
 *          be synced again on power up using any external source.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_OnchipRtc.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Includes                                       */
/******************************************************************************/
#include "Board.h"
#include "L2_OnchipRtc.h"

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_GENERAL)
#define CRYSTAL_STABILIZE_CLOCKS    (0x600000u)     /* Refer to crystal datasheet */
#define OC_RTC_CLOCK_SETTLE_TIME    ((CRYSTAL_STABILIZE_CLOCKS * 1000u)/SYSTEM_FREQ_HZ)     /* Clock settling time */
#define OC_RTC_DEFAULT_SECONDS      (SIGNIA_RTC_DEFAULT_VALUE)        /* Default counter value */

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
void L2_OnchipRtc_ISR(void);

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static OC_RTC_HANDLER pRtcNotifyHandler = NULL;

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   On-Chip RTC hardware initialization routine.
 *
 * \details Initializes MCU On-Chip RTC clock module
 *
 * \note    This function is intended to be called once during the system
 *          initialization to initialize MCU On-Chip RTC module.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_OnchipRtcInit(void)
{
    RTC_SECONDS RtcTime;
    uint8_t err;

    /* Enable RTC module clock */
    SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;

    if (RTC_TSR < OC_RTC_DEFAULT_SECONDS)
    {
        /* Issue reset, disables clock before configuration */
        RTC_CR  = RTC_CR_SWR_MASK;
        RTC_CR  &= ~RTC_CR_SWR_MASK;

        /* Oscillator capacitor load configuration */
        RTC_CR |= RTC_CR_SC2P_MASK | RTC_CR_SC4P_MASK;

        /* Enable RTC clock */
        RTC_CR |= RTC_CR_OSCE_MASK;

        /* Allow 32 kHz clock to stabilize, refer to crystal startup time in the crystal datasheet*/
        OSTimeDly(OC_RTC_CLOCK_SETTLE_TIME);

        /* Set time compensation parameters - effectively no compensation used */
        /* CIR - Comp. Interval in Seconds, set to 0 */
        /* TCR - Number of clocks in each interval, set to 0 */
        RTC_TCR = RTC_TCR_CIR(0u) | RTC_TCR_TCR(0u);

        /* Default init value for seconds counter */
        RTC_TSR = OC_RTC_DEFAULT_SECONDS;
    }

    /* Enable counter */
    RTC_SR |= RTC_SR_TCE_MASK;

	/* Keep second interrupt and notification disabled */
    L2_OnchipRtcConfig(true, NULL);

    Set_IRQ_Priority(OC_RTC_SECONDS_IRQ, OC_RTC_ISR_PRIORITY);
    Enable_IRQ(OC_RTC_SECONDS_IRQ);

    RtcTime = L2_OnchipRtcRead();

    Clk_Init(&err);
    if ( CLK_ERR_NONE == err)
    {
        /* Initialize clock with RTC Time */
        Clk_SetTS_Unix(RtcTime);
        /* Initialize Timezone - Setting  to UTC+ 0 */
        CLK_TZ_SEC tz_sec = 0;
        Clk_SetTZ(tz_sec);
    }
    else
    {
        Log(DBG,"Onchip clock initialization Failed");
    }
}

/* ========================================================================== */
/**
 * \brief   Configure On-Chip RTC hardware.
 *
 * \details Allows caller to Enabled/Disable On-Chip RTC module. Also allows
 *          caller register a callback function to be called in the event of
 *          'second change' interrupt. Notification callback is invoked only
 *          when a valid callback is registered.
 *
 * \note    Callback is invoked from ISR, keep it light weight. Disabled by default.
 *
 * \param   Enable   - On-Chip RTC enabled if true, otherwise disabled.
 * \param   pHandler - 'Second change' notification handler callback.
 *
 * \return  Status - On-Chip Configuration request status.
 *
 * ========================================================================== */
OC_RTC_STATUS L2_OnchipRtcConfig(bool Enable, OC_RTC_HANDLER pHandler)
{
    OC_RTC_STATUS Status;   /* Function return status */

    Status = OC_RTC_STATUS_OK;  /* Currently returning OK always */

    if(Enable)
    {
        /* Enable On-Chip RTC clock */
        RTC_CR |= RTC_CR_OSCE_MASK;
        /* Allow 32 kHz clock to stabilize, refer to crystal startup time in the crystal datasheet*/
        OSTimeDly(OC_RTC_CLOCK_SETTLE_TIME);
    }
    else
    {
        /* Disable On-Chip RTC clock */
        RTC_CR &= ~(RTC_CR_OSCE_MASK);
    }

    /* Copy new callback handler */
    pRtcNotifyHandler = pHandler;

    if(NULL == pRtcNotifyHandler)
    {
        /* Disable 'second' interrupt */
        RTC_IER &= ~RTC_IER_TSIE_MASK;
    }
    else
    {
        /* Enable 'second' interrupt */
        RTC_IER |= RTC_IER_TSIE_MASK;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Write new value to On-Chip RTC.
 *
 * \details Writes new time value to On-Chip RTC module.
 *
 * \note    None.
 *
 * \param   Seconds - New time value.
 *
 * \return  None
 *
 * ========================================================================== */
void L2_OnchipRtcWrite(uint32_t Seconds)
{
    /* Stop counter before setting new value */
    RTC_SR &= ~RTC_SR_TCE_MASK;

    /* Write new value to the counter register */
    RTC_TSR = Seconds;

    /* Restart the counter */
    RTC_SR |= RTC_SR_TCE_MASK;

    /* Set Clock timestamp Received in Seconds (UTC+00) */
    Clk_SetTS_Unix(Seconds);
}

/* ========================================================================== */
/**
 * \brief   Read current time from On-Chip RTC.
 *
 * \details Reads current time from On-Chip RTC module. The time value is the
 *          count of seconds from a reference date and time.
 *
 * \note    On-Chip RTC time does not have a separate battery backup. The time
 *          may be incorrect if the handle battery drops to very low voltage.
 *
 * \param   < None >
 *
 * \return  RTC_SECONDS - On-Chip RTC time
 *
 * ========================================================================== */
RTC_SECONDS L2_OnchipRtcRead(void)
{
    return RTC_TSR;
}

/* ========================================================================== */
/**
 * \brief   On-Chip RTC 'Second change' ISR
 *
 * \details Interrupt Service Routine to handle 'second change' event from
 *          On-Chip RTC HW module.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_OnchipRtc_ISR(void)
{
    OS_CPU_SR cpu_sr;
    uint32_t Seconds;

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    if (NULL != pRtcNotifyHandler)
    {
        Seconds = RTC_TSR;
        (*pRtcNotifyHandler)(Seconds);
    }

    OSIntExit();
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
