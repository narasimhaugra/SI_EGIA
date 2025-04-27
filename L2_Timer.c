#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Timer
 * \{
 * \brief   Timer control routines
 *
 * \details This file contians timer functionality implementation.
 *
 * \sa      K20 SubFamily Reference Manual.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_Timer.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Includes                                       */
/******************************************************************************/
#include "L2_Timer.h"
#include "Board.h"
#include "CpuInit.h"

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define L2_MAX_TIMERS               (4u)                                    ///< Max Timers.
#define L2_TIMER_PRESCALE           ((SYSTEM_FREQ_HZ / 1000000u) / 2u)        ///< with 120Mhz System Clock, 1uS = 120 Cycles.
#define L2_TIMER_MAX_MICROSECONDS   (0xFFFFFFFFu / L2_TIMER_PRESCALE)       ///< timer capacity
#define PIT_TCTRL_CHAIN_MASK        (0x4u)                                  ///< Mask for Timer Chain mode.

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef struct              /// Local configuration structure
{  
  TIMER_MODE      Mode;     ///< Timer mode, One shot or Periodic
  TIMER_EVT_HNDLR pHandler; ///< Timer expiry call back handler
  TIMER_STATUS    Status;   ///< Current state of the timer
  uint32_t        Value;    ///< Reload value
}TimerLocal;

/******************************************************************************/
/*                             Global Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static TimerLocal TimerList[L2_MAX_TIMERS];
static bool IsTimerInitDone = false;

/* ========================================================================== */
/**
 * \brief   Timer module initialization.
 * 
 * \details Initializes timer hardware and software components
 * 
 * \note    This function is intended to be called once during the system 
 *          initialization to initialize platform timers.  
 *          Any other timer interface functions should be called only after 
 *          calling this function.
 *
 * \param   < None >
 * 
 * \return  None
 * 
 *========================================================================== */
void L2_TimerInit(void)
{
    uint8_t Index;

    // Initialize local control structure copy
    for (Index = 0; Index < TIMER_ID_LAST; Index++)
    {
        TimerList[Index].pHandler = NULL;
        TimerList[Index].Mode    = TIMER_MODE_PERIODIC;
        TimerList[Index].Status  = TIMER_STATUS_DISABLED;
        TimerList[Index].Value   = 0;
    }

    // Disable timer and interrupt
    PIT_TCTRL0 &= ~(PIT_TCTRL_TIE_MASK| PIT_TCTRL_TEN_MASK | PIT_TCTRL_CHAIN_MASK);
    PIT_TCTRL1 &= ~(PIT_TCTRL_TIE_MASK| PIT_TCTRL_TEN_MASK | PIT_TCTRL_CHAIN_MASK);
    PIT_TCTRL2 &= ~(PIT_TCTRL_TIE_MASK| PIT_TCTRL_TEN_MASK | PIT_TCTRL_CHAIN_MASK);
    PIT_TCTRL3 &= ~(PIT_TCTRL_TIE_MASK| PIT_TCTRL_TEN_MASK | PIT_TCTRL_CHAIN_MASK);

    // Enable clock
    SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;

    // Disable the timer, Will be enabled when timers are configured. Freeze timer during debug
    PIT_MCR |= (PIT_MCR_FRZ_MASK | PIT_MCR_MDIS_MASK); 

    IsTimerInitDone = true;     // Mark init done

    return;
}

/* ========================================================================== */
/**
 * \brief   Timer Configuration
 *
 * \details This function configures a timers module with specified parameters. 
 *
 * \note    Configuring a timer which is already running will stop the timer and
 *          configure with new values.
 * 
 * \param   *pControl - Pointer to configuration structure.
 *
 * \return  Timer status
 *
 * \retval  TIMER_STATUS_STOPPED    - Timer not running
 * \retval  TIMER_STATUS_ERROR      - Configuration error.
 * ========================================================================== */
TIMER_STATUS L2_TimerConfig (TimerControl *pControl)
{
   TIMER_STATUS Status;
   TIMER_ID Id;

   Status = TIMER_STATUS_ERROR;

   if ((NULL != pControl) && ( true == IsTimerInitDone))
   {
       Id = pControl->TimerId;

       // Check for validity of the TimerControl Params  before configuring the Timers.
       if ((NULL != pControl->pHandler) \
           && ( TIMER_ID_LAST > Id ) \
           && (0 < pControl->Value )\
           && ( L2_TIMER_MAX_MICROSECONDS > pControl->Value ))
       {
           // All good, register timer handler and reload value 
            TimerList[Id].pHandler = pControl->pHandler; 
            TimerList[Id].Mode = pControl->Mode;
            TimerList[Id].Value = pControl->Value;

            //Now default the timers - disable, load timer
            Status = TIMER_STATUS_STOPPED;
            TimerList[Id].Status = TIMER_STATUS_STOPPED;

            switch (Id)
            {
                case TIMER_ID1:
                    PIT_TCTRL0 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK | PIT_TCTRL_CHAIN_MASK);
                    PIT_LDVAL0  = L2_TIMER_PRESCALE * pControl->Value;
                    break;

                case TIMER_ID2:
                    PIT_TCTRL1 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK | PIT_TCTRL_CHAIN_MASK);
                    PIT_LDVAL1  = L2_TIMER_PRESCALE * pControl->Value;
                    break;

                case TIMER_ID3:
                    PIT_TCTRL2 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK | PIT_TCTRL_CHAIN_MASK);
                    PIT_LDVAL2  = L2_TIMER_PRESCALE * pControl->Value;
                    break;

                case TIMER_ID4:
                    PIT_TCTRL3 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK | PIT_TCTRL_CHAIN_MASK);
                    PIT_LDVAL3  = L2_TIMER_PRESCALE * pControl->Value;
                    break;

                default:
                    Status = TIMER_STATUS_ERROR;  // Caller should worry why return status is error.
                    break;
            }
       }
   }

   return Status;
}


/* ========================================================================== */
/**
 * \brief   Timer Start
 *  
 * \details This function starts a configured timer. Do nothing if timer is 
 *          already running. 
 *
 * \param   TimerId - Timer ID
 *
 * \return  Timer status
 *
 * \retval  TIMER_STATUS_RUNNING  - Timer running 
 * \retval  TIMER_STATUS_ERROR    - Error, couldn't start the timer
 * ========================================================================== */
TIMER_STATUS L2_TimerStart(TIMER_ID TimerId)
{
    TIMER_STATUS Status;

    Status = TIMER_STATUS_RUNNING;

    do
    {
        // Check if the timer is initialzed and the TimerId is valid TIMER_ID value.
        if ((false == IsTimerInitDone) || (TIMER_ID_LAST <= TimerId))
        {
            Status = TIMER_STATUS_ERROR;
            break;
        }

        // Now we can index through the array, check for current status and reload value
        if ((TIMER_STATUS_STOPPED != TimerList[TimerId].Status) || (0 == TimerList[TimerId].Value))
        {
            Status = TIMER_STATUS_ERROR;
            break;
        }

        TimerList[TimerId].Status = Status;  // Updating the status here to save code line in switch case
        
        // Any valid timer request will re-renable timer hardware module 
        // Enable timer and debug freeze mask. Freeze flag freezes timer count during debugging

        PIT_MCR &= ~(PIT_MCR_MDIS_MASK); 

        // Stop and start the timer again to force reload.
        switch (TimerId)
        {
            case TIMER_ID1:
                PIT_TCTRL0 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK);
                PIT_LDVAL0 = L2_TIMER_PRESCALE * TimerList[TIMER_ID1].Value; 
                PIT_TCTRL0 |=  (PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK);
                Enable_IRQ(L2_Timer1_IRQ);
                break;

            case TIMER_ID2:
                PIT_TCTRL1 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK);
                PIT_LDVAL1 = L2_TIMER_PRESCALE * TimerList[TIMER_ID2].Value; 
                PIT_TCTRL1 |=  (PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK);
                Enable_IRQ(L2_Timer2_IRQ);
                break;

            case TIMER_ID3:
                PIT_TCTRL2 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK);
                PIT_LDVAL2 = L2_TIMER_PRESCALE * TimerList[TIMER_ID3].Value; 
                PIT_TCTRL2 |=  (PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK);
                Enable_IRQ(L2_Timer3_IRQ);
                break;

            case TIMER_ID4:
                PIT_TCTRL3 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK);
                PIT_LDVAL3 = L2_TIMER_PRESCALE * TimerList[TIMER_ID4].Value; 
                PIT_TCTRL3 |=  (PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK);
                Enable_IRQ(L2_Timer4_IRQ);
                break;

            default:
                Status = TIMER_STATUS_ERROR;
                break;
        }
    } while (false);

    return Status; 
}

/* ========================================================================== */
/**
 * \brief   Timer Restart
 *  
 * \details This function restarts a timer. If already running, stop the timer,
 *          reload with new timer duration. 
 *
 * \param   TimerId  - Timer ID
 * \param   Duration - New time duration 
 *
 * \return  Timer status
 *  
 * \retval  TIMER_STATUS_RUNNING  - Timer running 
 * \retval  TIMER_STATUS_ERROR    - Error, couldn't restart the timer
 * ========================================================================== */
TIMER_STATUS L2_TimerRestart (TIMER_ID TimerId, uint32_t Duration)
{
    TIMER_STATUS Status;

    Status = L2_TimerStop(TimerId);      // Returns Disabled if TimerId is invalid.

    if (TIMER_STATUS_ERROR != Status)
    {
        if ((0 < Duration) && (L2_TIMER_MAX_MICROSECONDS > Duration))
        {
            TimerList[TimerId].Value = Duration;   // Update control structure copy
            Status = L2_TimerStart(TimerId);
        }
        else
        {
            Status = TIMER_STATUS_ERROR;
        }
    }
    return Status; 
}

/* ========================================================================== */
/**
 * \brief   Timer Stop
 *  
 * \details This function stops a running timer.
 *  
 * \param   TimerId - Timer ID
 *  
 * \return  Timer status
 *  
 * \retval  TIMER_STATUS_STOPPED  - Timer not running
 * \retval  TIMER_STATUS_ERROR    - Error, couldn't stop the timer
 * ========================================================================== */
TIMER_STATUS L2_TimerStop (TIMER_ID TimerId)
{
    TIMER_STATUS Status;

    Status = TIMER_STATUS_ERROR;

    if ((true == IsTimerInitDone) && (TIMER_ID_LAST > TimerId))
    {
        Status = TIMER_STATUS_STOPPED;      // Since we are here, let's assume all well.

        // Stop timer.
        switch (TimerId)
        {
            case TIMER_ID1:
                PIT_TCTRL0 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK | PIT_TCTRL_CHAIN_MASK);
                break;

            case TIMER_ID2:
                PIT_TCTRL1 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK | PIT_TCTRL_CHAIN_MASK);
                break;

            case TIMER_ID3:
                PIT_TCTRL2 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK | PIT_TCTRL_CHAIN_MASK);
                break;

            case TIMER_ID4:
                PIT_TCTRL3 &= ~(PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK | PIT_TCTRL_CHAIN_MASK);
                break;

            default:
                Status = TIMER_STATUS_ERROR;   // Oh, we just found out timer ID is invalid
                break;
        }
    }

    if ( TIMER_STATUS_STOPPED == Status )
    {
        // We were able to stop the timer hardware, so, mark it.
        TimerList[TimerId].Status = TIMER_STATUS_STOPPED;
    }

    return Status; 
}

/* ========================================================================== */
/**
 * \brief   Timer Status
 *  
 * \details This function returns current status of the specified timer. Does
 *          not affect the state of the timer.
 *
 * \param   TimerId - Timer ID
 *
 * \return  Timer status
 *
 * \retval  TIMER_STATUS_STOPPED  - Timer not running
 * \retval  TIMER_STATUS_RUNNING  - Timer running 
 * \retval  TIMER_STATUS_DISABLED - Timer disabled
 * \retval  TIMER_STATUS_ERROR    - Error, invalid timer ID specified.
 * ========================================================================== */
TIMER_STATUS L2_TimerStatus (TIMER_ID TimerId)
{
    TIMER_STATUS Status;

    if (false == IsTimerInitDone) 
    {
        Status = TIMER_STATUS_DISABLED;
    }
    else if (TIMER_ID_LAST > TimerId)
    {
        Status = TimerList[TimerId].Status;
    }
    else
    {
        Status = TIMER_STATUS_ERROR;
    }

    return Status; 
}

/* ========================================================================== */
/**
 * \brief   PIT Channel 0 (TIMER_ID1) ISR 
 *  
 * \details This is periodic timer Id 1 interrupt service routine.
 * 
 * \note    This function should be defined according RTOS ISR template.
 *
 * \param	< None >
 *
 * \return	None
 *
 * ========================================================================== */
void L2_Timer0_ISR(void)
{
    OS_CPU_SR  cpu_sr;

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    PIT_TFLG0 |= PIT_TFLG_TIF_MASK;         // Clear interrupt status flag

    if ( TIMER_MODE_ONESHOT == TimerList[TIMER_ID1].Mode )
    {
        (void)L2_TimerStop(TIMER_ID1);
    }

    if (NULL != TimerList[TIMER_ID1].pHandler)
    {
        TimerList[TIMER_ID1].pHandler();     // Invoke registered handler
    }

    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   PIT Channel 1 (TIMER_ID2) ISR 
 *  
 * \details This is periodic timer Id 2 interrupt service routine.
 * 
 * \note    This function should be defined according RTOS ISR template.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_Timer1_ISR(void)
{
    OS_CPU_SR  cpu_sr;

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    PIT_TFLG1 |= PIT_TFLG_TIF_MASK;         // Clear interrupt status flag

    if ( TIMER_MODE_ONESHOT == TimerList[TIMER_ID2].Mode )
    {
        (void)L2_TimerStop(TIMER_ID2);
    }

    if ( NULL != TimerList[TIMER_ID2].pHandler)
    {
        TimerList[TIMER_ID2].pHandler();     // Invoke registered handler
    }

    OSIntExit();
}


/* ========================================================================== */
/**
 * \brief   PIT Channel 2 (TIMER_ID3) ISR 
 *  
 * \details This is periodic timer Id 3 interrupt service routine.
 * 
 * \note    This function should be defined according RTOS ISR template.
 *
 * \param   < None >
 *
 * \return  None
 *  
 * ========================================================================== */
void L2_Timer2_ISR(void)
{
    OS_CPU_SR  cpu_sr;

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    PIT_TFLG2 |= PIT_TFLG_TIF_MASK;     // Clear interrupt status flag 

    if ( TIMER_MODE_ONESHOT == TimerList[TIMER_ID3].Mode )
    {
        (void)L2_TimerStop(TIMER_ID3);
    }

    if ( NULL != TimerList[TIMER_ID3].pHandler)
    {
        TimerList[TIMER_ID3].pHandler();     // Invoke registered handler
    }

    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   PIT Channel 3 (TIMER_ID4) ISR 
 *  
 * \details This is periodic timer Id 4 interrupt service routine.
 * 
 * \note    This function should be defined according RTOS ISR template.
 *
 * \param	< None >
 *
 * \return	None
 *
 * ========================================================================== */
void L2_Timer3_ISR(void)
{
    OS_CPU_SR  cpu_sr;

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    PIT_TFLG3 |= PIT_TFLG_TIF_MASK;     // Clear interrupt status flag    

    if ( TIMER_MODE_ONESHOT == TimerList[TIMER_ID4].Mode )
    {
        (void)L2_TimerStop(TIMER_ID4);
    }

    if ( NULL != TimerList[TIMER_ID4].pHandler)
    {
        TimerList[TIMER_ID4].pHandler();     // Invoke registered handler
    }

    OSIntExit();
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

