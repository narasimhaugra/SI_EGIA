#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ============================================================================================== */
/** \addtogroup TaskMonitor
 * \{
 * \brief   This module monitors each task's information
 *
 * \details This module monitors each task's execution times, context s/w etc.
 *
 * \todo None
 *
 * \bug None
 *
 * \copyright 2019 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file TaskMonitor.c
 *
 * ============================================================================================== */
/**************************************************************************************************/
/*                                         Includes                                               */
/**************************************************************************************************/
#include <common.h>
#include <string.h>
#include "vectors.h"
#include "TaskMonitor.h"
#include "L2_Timer.h"
#include "cpuinit.h"
#include "Logger.h"
#include "FaultHandler.h"
#include "Signia_PowerControl.h"
#include "TestManager.h"

/**************************************************************************************************/
/*                                         Global Constant Definitions(s)                         */
/**************************************************************************************************/
/**************************************************************************************************/
/*                                         Local Conditional Compile Define(s) (Macros)           */
/**************************************************************************************************/
#define TASKMONITOR_PRINT_STATUS_ENABLE                                ///< Define this to print the task monitor statistics periodically to the console
/**************************************************************************************************/
/*                                         Local Define(s) (Macros)                               */
/**************************************************************************************************/
#define LOG_GROUP_IDENTIFIER              (LOG_GROUP_TESTS)     ///< Log Group Identifier
#define TASK_MON_STACK_SIZE               (512u)                ///< Stack size of task monitor task
#define MAX_TASKMONITOR_NAME_LEN          (18u)                 ///< Task Monitor name length.
#define TASK_MONITOR_CSV_LINE_BUFF_LEN    (100u)                ///< CSV Line Buffer Length.
#define TASK_MONITOR_UPDATE_PERIOD        (SEC_1*5)             ///< Update the Task Montior every 5 secs
#define TASK_MONITOR_STARTUP_PRINT_PERIOD (SEC_20)              ///< Log the current task load information after this delay on startup
#define TASK_MONITOR_PRINT_PERIOD         (MIN_1*30)            ///< Log the current task load information every 30 minutes +/- 1 minute
#define TASK_MONITOR_LOG_PERIOD           (MIN_30)              ///< Print the Tasks details to SD card
#define TASK_MONITOR_PERIOD               (MSEC_200)            ///< Task period every 200 millisec
#define TASKMON_WDOG_250MSECVALH          (0x002Du)             ///< WDOG high byte value for 250 millisecond refresh
#define TASKMON_WDOG_250MSECVALL          (0xC6C0u)             ///< WDOG low byte value for 250 millisecond refresh
#define TASKMONITOR_TIMER_PRESCALE                 ((SYSTEM_FREQ_HZ / 1000000u) / 2u)                    ///< with 120Mhz System Clock, 1uS = 120 Cycles.
#define TASKMONITOR_TIMER_MAX_MICROSECONDS         (UINT32_MAX_VALUE / TASKMONITOR_TIMER_PRESCALE)       ///< timer capacity
#define TASKMONITOR_DEFAULT_CHECKIN_TIME  (SEC_1*30)            ///< Default checkin time set during initialization
#define TASKMOMITOR_MAX_CHECKIN_TIMEOUT   (SEC_1*30)            ///< Maximum checkin time set during initialization
#define GET_PIT_CVAL1_TICK                (UINT32_MAX_VALUE  - (uint32_t) PIT_CVAL1)  ///< This macro returns the current PIT timer value which is a countdown timer
#define TASKMONITOR_PEAKLOAD_THRESHOLD   (9000u)                 ///< Peak Load threshold value (90%) ,in 100th of percent, above which fault is raised
#define TASKMONITOR_STACKSPACE_LOWTHD    (10u)                   ///< Peak Load threshold value (90%) above which fault is raised

/* State to String */
#define TASK_STATE_TO_STRING(u32State) \
        ((u32State) == OS_STAT_RDY ? "Ready":\
         (u32State) == OS_STAT_SEM ? "P Sem":\
         (u32State) == OS_STAT_MBOX ? "P MBx" :\
         (u32State) == OS_STAT_Q ? "P Que" :\
         (u32State) == OS_STAT_SUSPEND ? "Suspd" :\
         (u32State) == OS_STAT_MUTEX ? "P MuX" :\
         (u32State) == OS_STAT_FLAG ? "P Eve" :\
         (u32State) == OS_STAT_MULTI ? "P Mul" :\
         "Others")

/* Set bits*/
#define TASKMONITOR_SETFAULTBIT(x,fault)      x |= (1<<fault)
/* clear bits*/
#define TASKMONITOR_CLEARFAULTBIT(x,fault)    x &= ~(1<<fault)
/**************************************************************************************************/
/*                                         Global Variable Definitions(s)                         */
/**************************************************************************************************/
OS_STK TaskMonitorStack[TASK_MON_STACK_SIZE+MEMORY_FENCE_SIZE_DWORDS];
/**************************************************************************************************/
/*                                         Local Type Definition(s)                               */
/**************************************************************************************************/
typedef struct                  ///< Structure to hold the task monitor info
{
    uint32_t     u32SwitchedInTick;                       ///< Switch In Tick PIT value
    uint32_t     u32SwitchedOutTick;                      ///< Switch Out Tick PIT value

    uint32_t     u32ContextSwitches;                      ///< Context Switch Count
    uint32_t     u32ContextSwitchesOneUserPeriod;         ///< Context Switch One UserPeriod

    uint32_t     u32ElapsedTicks;                         ///< Last Elapsed Ticks
    uint32_t     u32ElapsedTicksOneUserPeriod;            ///< Last Elapsed Ticks One UserPeriod
    uint32_t     u32PeakElapsedTick;                      ///< Peak Elapsed Ticks
    uint32_t     u32CumulativeElapsedTicks;               ///< Cumulative Elapsed Ticks
    uint32_t     u32PeakCumulativeElapsedTicks;           ///< Peak Cumulative Elapsed Ticks

    uint32_t  u32TicksSuspended;                          ///< Ticks Suspended
    uint32_t  u32PeakTicksSuspended;                      ///< Peak Ticks Suspended

    uint32_t  u32LoadAverageOneUserPeriod;                ///< Load Average One UserPeriod
    uint32_t  u32LoadPeakOneUserPeriod;                   ///< Load Peak One UserPeriod

    uint32_t  u32LastCheckIn;                             ///< Last Checkin Time.
    uint32_t  u32CheckInDifference;                       ///< Current Checkin Difference.
    uint32_t  u32PeakCheckInDifference;                   ///< Peak Checkin Difference.

    uint32_t  u32TaskCheckinTimeout;                      ///< Current Checkin Difference.

    uint32_t  u32FreeStackSpace;                          ///< Free stack space in percentage.
    bool    bIsRegistered;                                ///< true if Task Registerred-
    bool    bWdogTimedout;                                ///< Status of whether watchdog timedout for this task
} xTaskInformation_t;

typedef enum                                             ///< Stat
{
   WDOG_FLAG_REFRESH,                                    ///< Status to indicate Refresh to WDOG required
   WDOG_FLAG_NO_REFRESH                                  ///< Status to indicate Refresh to WDOG required
} WDOG_REFRESH_STATUS;

typedef enum                                            ///< TaskMonitor Faults
{
    TASK_LOADCHECK_FAIL,                                ///< Task Load check Failure
    TASK_CHECKIN_FAIL,                                  ///< Task Checkin Failure
    TASK_STACKCHECK_FAIL,                               ///< Task Stack Check Failure
    TASKMONITOR_FAULTCOUNT                              ///< Task Failure count
} TASKMONITOR_FAULTS;

/**************************************************************************************************/
/*                                         Local Constant Definition(s)                           */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                         Local Variable Definition(s)                           */
/**************************************************************************************************/
static xTaskInformation_t  gxTaskInfoUserPeriod;                ///< Object for User Period Calculation

//#pragma location=".sram"   
static xTaskInformation_t  gxTaskInfo[OS_LOWEST_PRIO + 1];      ///< Global task info

static bool gbIsTaskMonitorInitialized = false;                 ///< Init Checker
static bool bIsWdogEnabled = false;                             ///< Watchdog Enable Status
static bool bIsTaskMonitorEnabled = false;                      ///< Task Monitoring enabled status
static uint32_t u32NextTaskMonitorPrintTime;           ///< Task Monitoring Event Logging Periodicity
static uint8_t TaskMonitorFaults;                               ///< Bit coded Flag to hold the TaskMonitor faults, failure position given by
static bool FaultReqRaised;                                     ///< Flag to indicate if Fault Req is raised

/**************************************************************************************************/
/*                                         Local Function Prototype(s)                            */
/**************************************************************************************************/
#ifdef ENABLE_TASKMON_WDOG_FEATURE
static void WdogEnable(void);
static void WdogDisable(void);
static void WdogRefresh(void);
static void WdogUnlock(void);
#endif
static void TaskMonitorTask(void *p_arg);
static void TaskMonitorIntHandler(void);
#ifdef TASKMONITOR_PRINT_STATUS_ENABLE
static void TaskMonitorPrintStatus(void);
#endif
static void ComputeTaskInfoParams(uint32_t u32TotalTime);
void TaskMonitorTaskSwitch(void);
void TaskMonitorUpdateLoads(void);
void Wdog_ISR(void);

/**************************************************************************************************/
/*                                         Local Function(s)                                      */
/**************************************************************************************************/
/* ========================================================================== */
/**
 * \brief    Task Monitor Task.
 *
 * \details  Taskmonitor task.periodically checks if registered tasks repot in-time
 *           Also, Refreshes the watchdog if all reporting times are within configured ranges.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void TaskMonitorTask(void *p_arg)
{
    POWER_MODE PowerMode;

    #ifdef ENABLE_TASKMON_WDOG_FEATURE
    WDOG_REFRESH_STATUS u8WdogRefreshFlag;
    #endif
    uint8_t u8TaskPriority;

    #ifdef ENABLE_TASKMON_WDOG_FEATURE
    u8WdogRefreshFlag = WDOG_FLAG_REFRESH;
    #endif
    u32NextTaskMonitorPrintTime = OSTimeGet() + TASK_MONITOR_STARTUP_PRINT_PERIOD;
    TaskMonitorEnable();
    /// \todo 04/05/2021 CPK change below reporting time to a lower value currently set to 2Seconds based on TASK_MONITOR_PERIOD
    TaskMonitorRegisterTask(SEC_2);
    while (true)
    {
        TaskMonitorTaskCheckin(OSTCBCur->OSTCBPrio);
        OSSchedLock();
        {
            /*  perform the reporting time check for each registered task*/
            for (u8TaskPriority = 0; u8TaskPriority < (OS_LOWEST_PRIO + 1); u8TaskPriority++)
            {
                if(!bIsTaskMonitorEnabled)
                {
                    break;
                }
                if((OSTCBPrioTbl[u8TaskPriority] != NULL) && (OSTCBPrioTbl[u8TaskPriority]->OSTCBCtxSwCtr > 1))
                {
                   if( (gxTaskInfo[u8TaskPriority].bIsRegistered) && (gxTaskInfo[u8TaskPriority].u32CheckInDifference > gxTaskInfo[u8TaskPriority].u32TaskCheckinTimeout))
                   {
                       Log(DBG, "Task %s with TaskPrio %lu not Responding", OSTCBPrioTbl[u8TaskPriority]->OSTCBTaskName,u8TaskPriority);
                       TASKMONITOR_SETFAULTBIT(TaskMonitorFaults, TASK_CHECKIN_FAIL);

                       #ifdef ENABLE_TASKMON_WDOG_FEATURE
                       u8WdogRefreshFlag =WDOG_FLAG_NO_REFRESH;
                       /// \todo 04/07/2021 CPK add break here to break from the loop. currently logging all tasks missing the deadline
                       #endif
                   }

                   /// \todo 04/05/2021 CPK Below condition to check default checkin-time on startup Commented until each task enables task monitoring and is tested
                   #ifdef DEFAULT_CHECKIN_ENABLE
                   /* check for unregistered task, if the task reporting time is less than the default checkin time - this is mostly encountered during startup */
                   if((gxTaskInfo[u8TaskPriority].u32CheckInDifference > (TASKMONITOR_DEFAULT_CHECKIN_TIME)) && (!gxTaskInfo[u8TaskPriority].bIsRegistered))
                   {
                       Log(DBG, "Task %s with TaskPrio %lu not Responding - Default timeout crossed", OSTCBPrioTbl[u8TaskPriority]->OSTCBTaskName,u8TaskPriority);
                       TASKMONITOR_SETFAULTBIT(TaskMonitorFaults, TASK_CHECKIN_FAIL);

                       /*u8WdogRefreshFlag =WDOG_FLAG_NO_REFRESH; Enable the watdog refresh while testing respective task*/
                   }
                   #endif

                }
            }
        }
        /* Resume the Scheduler */
        OSSchedUnlock();

        /* Raise a fault request if not already raised */
        if(TaskMonitorFaults && !FaultReqRaised)
        {
            FaultHandlerSetFault(REQRST_TASKMONITOR_FAIL, SET_ERROR);
            FaultReqRaised = true;
        }

        #ifdef ENABLE_TASKMON_WDOG_FEATURE
        /* Refresh the watchdog based on the taks loop above */
        if((WDOG_FLAG_REFRESH ==u8WdogRefreshFlag) && (bIsWdogEnabled))
        {
            WdogRefresh();
        }
        #endif

        #ifdef TASKMONITOR_PRINT_STATUS_ENABLE
        /* log the current task load information to the EVENT_LOG every 30 minutes +/- 1 minute when not in STANDBY_MODE or SLEEP_MODE. */
         PowerMode = Signia_PowerModeGet();
        /* periodically print task monitor status */
        if ( POWER_MODE_ACTIVE == PowerMode )
            TaskMonitorPrintStatus();
        #endif

        OSTimeDly(TASK_MONITOR_PERIOD);
    }
}

/* ========================================================================== */
/**
 * \brief    This function computes the Parameters  of the gxTaskInfoUserPeriod structure.
 *
 * \details  This is a helper function of TaskMonitorUpdateLoads to compute the gxTaskInfoUserPeriod parameters
 *
 * \param   u32TotalTime - cummulative Total elapsed time
 *
 * \return  None
 *
 * ========================================================================== */
static void ComputeTaskInfoParams(uint32_t u32TotalTime)
{
    /* Record the System Maximum Elapsed Ticks for any User Period . */
    if ( gxTaskInfoUserPeriod.u32CumulativeElapsedTicks >
        gxTaskInfoUserPeriod.u32PeakElapsedTick )
    {
        gxTaskInfoUserPeriod.u32PeakElapsedTick =
            gxTaskInfoUserPeriod.u32CumulativeElapsedTicks;
    }

    /* Record the System Maximums for any User Period */
    if ( gxTaskInfoUserPeriod.u32LoadAverageOneUserPeriod >
        gxTaskInfoUserPeriod.u32LoadPeakOneUserPeriod )
    {
        gxTaskInfoUserPeriod.u32LoadPeakOneUserPeriod =
            gxTaskInfoUserPeriod.u32LoadAverageOneUserPeriod;
    }

    /* 4) Find Interrupt overhead ticks */
    /* Add all task's elapsed ticks including Idle Task. */
    gxTaskInfoUserPeriod.u32TicksSuspended += gxTaskInfoUserPeriod.u32CumulativeElapsedTicks;

    /* Subtract All task's elapsed ticks sum from the Main tick difference,
    this gives ticks not available for any tasks */
    gxTaskInfoUserPeriod.u32TicksSuspended =
        ( gxTaskInfoUserPeriod.u32SwitchedOutTick +
         ~( gxTaskInfoUserPeriod.u32SwitchedInTick ) + 1UL ) -
            gxTaskInfoUserPeriod.u32TicksSuspended;

    /* Convert to a load. */
    if(0u != u32TotalTime)
    {
        gxTaskInfoUserPeriod.u32TicksSuspended /= u32TotalTime;
    }
    /* Record the peak. */
    if ( gxTaskInfoUserPeriod.u32TicksSuspended > gxTaskInfoUserPeriod.u32PeakTicksSuspended )
    {
        gxTaskInfoUserPeriod.u32PeakTicksSuspended = gxTaskInfoUserPeriod.u32TicksSuspended;
    }
}

#ifdef TASKMONITOR_PRINT_STATUS_ENABLE
//* ============================================================================================= */
/**
 * \brief   Logs the status to console
 *
 * \details This function logs the status of tasks to Console [Debug UART].
 *
 * \note
 *
 * \warning None
 *
 * \param   < None >
 *
 * \return  None
 *
 * ============================================================================================= */
static void TaskMonitorPrintStatus(void)
{
    uint8_t u8TaskCount;
    uint8_t u8TaskPriority;
    OS_CPU_SR   cpu_sr;
    uint32_t u32TimeNow;
    u8TaskCount = 0;
    u8TaskPriority = 0;
    u32TimeNow = 0;

    /* Start logging only after init and Task Monitor Log Period is Set above 0 */
    if ((true == gbIsTaskMonitorInitialized) && (u32NextTaskMonitorPrintTime > 0))
    {
        /* Get the current time in millisecs */
        u32TimeNow  = OSTimeGet();

        /* Task Monitor is ready, Is it time to run */
        if (u32TimeNow >= u32NextTaskMonitorPrintTime)
        {
            /* Add header line */
            Log(DBG,"%5s %4s %-14s %5s %8s %8s %7s %7s %8s %8s",
                    "Count,",
                    "Prio,",
                    "TaskName,",
                    "State,",
                    "Now(uS),",
                    "Peak(uS),",
                    "Now(%),",
                    "Peak(%),",
                    "Curr WD(mS),",
                    "Peak WD(mS),"
                    "Enabled(Y/N)");

            /* Have a count */
            u8TaskCount = 0;

            OS_ENTER_CRITICAL();
            /* Print info from all available tasks. */
            for (u8TaskPriority = 0; u8TaskPriority < (OS_LOWEST_PRIO + 1); u8TaskPriority++)
            {
                if((OSTCBPrioTbl[u8TaskPriority] != NULL) && (OSTCBPrioTbl[u8TaskPriority]->OSTCBCtxSwCtr > 1))
                {
                    Log(DBG,"%5d %5d  %-14s %5s %8lu %8lu   %3lu.%02.2lu %3lu.%02.2lu %10lu %10lu %10d",
                            ++u8TaskCount,
                            u8TaskPriority,
                            OSTCBPrioTbl[u8TaskPriority]->OSTCBTaskName,
                            TASK_STATE_TO_STRING(OSTCBPrioTbl[u8TaskPriority]->OSTCBStat),
                            TICKS_TO_MICROSECONDS(gxTaskInfo[u8TaskPriority].u32ElapsedTicks),
                            TICKS_TO_MICROSECONDS(gxTaskInfo[u8TaskPriority].u32PeakElapsedTick),
                            ( gxTaskInfo[u8TaskPriority].u32LoadAverageOneUserPeriod / 100 ),
                            ( gxTaskInfo[u8TaskPriority].u32LoadAverageOneUserPeriod % 100 ),
                            ( gxTaskInfo[u8TaskPriority].u32LoadPeakOneUserPeriod / 100 ),
                            ( gxTaskInfo[u8TaskPriority].u32LoadPeakOneUserPeriod % 100 ),
                            gxTaskInfo[u8TaskPriority].u32CheckInDifference,
                            gxTaskInfo[u8TaskPriority].u32PeakCheckInDifference,
                            gxTaskInfo[u8TaskPriority].bIsRegistered);
                }
            }

            /* Print Interrupt stats */
            Log(DBG,"Interrupts:  Now(%2lu.%02.2lu%%) Peak(%2lu.%02.2lu%%)",
                    ( gxTaskInfoUserPeriod.u32TicksSuspended / 100 ),
                    ( gxTaskInfoUserPeriod.u32TicksSuspended % 100 ),
                    ( gxTaskInfoUserPeriod.u32PeakTicksSuspended / 100 ),
                    ( gxTaskInfoUserPeriod.u32PeakTicksSuspended % 100 ) );

            /* Print System Load: */
            Log(DBG,"System Load: Now(%2lu.%02.2lu%%) Peak(%2lu.%02.2lu%%)",
                    ( gxTaskInfoUserPeriod.u32LoadAverageOneUserPeriod / 100 ),
                    ( gxTaskInfoUserPeriod.u32LoadAverageOneUserPeriod % 100 ),
                    ( gxTaskInfoUserPeriod.u32LoadPeakOneUserPeriod / 100 ),
                    ( gxTaskInfoUserPeriod.u32LoadPeakOneUserPeriod % 100 ));

            OS_EXIT_CRITICAL();

            /* Set the next Print period. */
            u32NextTaskMonitorPrintTime = u32TimeNow + TASK_MONITOR_PRINT_PERIOD;
        }
    }
}
#endif

#ifdef ENABLE_TASKMON_WDOG_FEATURE
//* ============================================================================================= */
/**
 * \brief   Enables the watchdog
 *
 * \details Watchdog timer enable routine. The Watchdog related functions are
 *          private to Task monitor.
 *
 * Notes -
 *       1 second timeout         =  12,000,000
 *       2 second timeout         =  2 * 12,000,000 = 0x016E 3600
 *       5 second timeout         =  5 * 12,000,000 = 0x0393 8700
 *       30 second timeout        =  30 * 12,000,00 = 0x1575 2A00
 *       250 millisecond timeout  =  12,000,000/4   = 0x002D C6C0
 *
 * \warning None
 *
 * \param   < None >
 *
 * \return  None
 *
 * ============================================================================================= */
static void WdogEnable(void)
{
    bool WatchDogInit;

    /* This sequence must execute within 20 clock cycles, so disable
    interrupts will keep the code atomic and ensure the timing. */
    DisableInterrupts;
    WdogUnlock();                            /* Unlock the watchdog so that we can write to registers*/
    WDOG_TOVALH = TASKMON_WDOG_250MSECVALH;  /* Watchdog time-out register high*/
    WDOG_TOVALL = TASKMON_WDOG_250MSECVALL;  /* Watchdog time-out register low*/

    WDOG_WINH   = TASKMON_WDOG_250MSECVALH;  /* Watchdog window register high */
    WDOG_WINL   = TASKMON_WDOG_250MSECVALL;  /* Watchdog window register low */

    /* configure watchdog Masks - Add notes below before review*/
    WDOG_STCTRLH = WDOG_STCTRLH_CLKSRC_MASK |
                   WDOG_STCTRLH_WDOGEN_MASK |
                   WDOG_STCTRLH_ALLOWUPDATE_MASK |
                   WDOG_STCTRLH_STOPEN_MASK |
                   WDOG_STCTRLH_WAITEN_MASK |
                   WDOG_STCTRLH_IRQRSTEN_MASK;

    EnableInterrupts;
    bIsWdogEnabled = true;                    /* watchdog enabled status*/

    Enable_IRQ(WDOG_IRQ);
    Set_IRQ_Priority(WDOG_IRQ, WDOG_ISR_PRIORITY);

    WatchDogInit =  (WDOG_WINH == TASKMON_WDOG_250MSECVALH);  /// \\todo 02/10/2022 KA: why checking values set above?
    WatchDogInit &= (WDOG_WINL == TASKMON_WDOG_250MSECVALL);  /// \\todo 02/10/2022 KA: possibly unsafe - boolean &=
    WatchDogInit &= (WDOG_TOVALH == TASKMON_WDOG_250MSECVALH);
    WatchDogInit &= (WDOG_TOVALL == TASKMON_WDOG_250MSECVALL);
    WatchDogInit &= (WDOG_STCTRLH & WDOG_STCTRLH_WDOGEN_MASK);

    TM_Hook(HOOK_WATCHDOGINIT, &WatchDogInit);

    bIsWdogEnabled = WatchDogInit;

    if ( !WatchDogInit )
    {
        /* WatchDog Initialization Failed */
        FaultHandlerSetFault(REQRST_TASKMONITOR_FAIL, SET_ERROR);
    }
}

//* ============================================================================================= */
/**
 * \brief   Disables the watchdog
 *
 * \details Watchdog timer disable routine. Called when Task monitor is disabled.
 *
 *
 * \warning None
 *
 * \param   < None >
 *
 * \return  None
 *
 * ============================================================================================= */
static void WdogDisable(void)
{
    INT16U stCtrlH;

    /* This sequence must execute within 20 clock cycles, so disable
     interrupts will keep the code atomic and ensure the timing.*/
    DisableInterrupts;
    WdogUnlock();    // Unlock the watchdog so that we can write to registers
    stCtrlH = WDOG_STCTRLH;
    stCtrlH &= ~WDOG_STCTRLH_WDOGEN_MASK;   /* Clear the WDOGEN bit to disable the watchdog */
    WDOG_STCTRLH = stCtrlH;
    EnableInterrupts;
    bIsWdogEnabled = false;
}

//* ============================================================================================= */
/**
 * \brief   Refreshes the watchdog
 *
 * \details Watchdog timer refresh routine.
 *          Writing 0xA602 followed by 0xB480 within 20 bus cycles will refresh
 *          the watchdog timer.
 *
 *
 * \warning DO NOT SINGLE STEP THROUGH THIS FUNCTION!!!
 *          There are timing requirements for the execution of the refresh.
 *          If you single step through the code you will cause the CPU to reset.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ============================================================================================= */
static void WdogRefresh(void)
{
   if (bIsWdogEnabled)
   {
        /* This sequence must execute within 20 clock cycles, so disable
         interrupts will keep the code atomic and ensure the timing.*/
        DisableInterrupts;
        WDOG_REFRESH = 0xA602u;  /* Write 0xA602 to the refresh register */
        WDOG_REFRESH = 0xB480u;  /* Followed by 0xB480 to complete the refresh as mentioned in processor reference manual*/
        EnableInterrupts;
    }
}

//* ============================================================================================= */
/**
 * \brief   Unlocks the watchdog for configuration
 *
 * \details Watchdog timer unlock routine.
 *          Writing 0xC520 followed by 0xD928 will unlock the write once
 *          registers in the WDOG so they are writeable within the WCT period.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ============================================================================================= */
static void WdogUnlock(void)
{
    WDOG_UNLOCK = 0xC520u;   /* Write 0xC520 to the unlock register */
    WDOG_UNLOCK = 0xD928u;   /* Followed by 0xD928 to complete the unlock as mentioned in processor reference manual*/
}
#endif

//* ============================================================================================= */
/**
 * \brief   Watchdog ISR
 *
 * \details Watchdog timer Interrupt service routine. This is called before
 *          the watchdog resets the processor
 *
 * \param   < None >
 *
 * \return  None
 *
 * ============================================================================================= */
#undef  LOG_GROUP_IDENTIFIER        /// \todo 08/01/2022 DAZ - This is a hack to avoid conflict with define in Screen_AdapterVerify.h. This should NOT be defined there.
#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_MCUX)

void Wdog_ISR(void)
{
    Log(FLT, "****WDOG ISR****");       /// \todo 05/27/2021 DAZ - Not sure this will ever finish processing before reset. Maybe direct file write?

/// \todo 03/26/2021 CPK wdogTask status must be in Noinit RAM
#ifdef  LOG_TO_NOINITRAM
    // Don't fill in this info if a hard fault has already occurred.
    // hard_fault_isr would have already filled it in.
    // Also don't fill in if deepSleepActivated == true.
    if ((noInitRam.hardFaultTask == 0) && (noInitRam.deepSleepActivated == false))
    {
        // Save the index of the currently running task when the watchdog caused this interrupt.
        noInitRam.wdogTask = (uint32_t)OSTCBCur->OSTCBPrio;
        noInitRam.wdogProgramCounter = *(CPU_ADDR *)(__get_PSP() + 0x18u);
        noInitRam.OSTCBStkPtr = OSTCBCur->OSTCBStkPtr;
        noInitRam.OSTCBStkBottom = OSTCBCur->OSTCBStkBottom;
        noInitRam.OSTCBStkBase = OSTCBCur->OSTCBStkBase;
        noInitRam.OSTCBStkSize = OSTCBCur->OSTCBStkSize;
        noInitRam.OSTCBStkUsed = OSTCBCur->OSTCBStkUsed;
    }
#endif
}

//* ============================================================================================= */
/**
 * \brief   Interrupt handler for PIT timer.
 *
 * \details currently a placeholder function for future use.
 *
 * \note
 *
 * \warning None
 *
 * \param   < None >
 *
 * \return  None
 *
 * ============================================================================================= */
static void TaskMonitorIntHandler(void)
{
    /* Do Nothing */
}

/**************************************************************************************************/
/*                                         Global Function(s)                                     */
/**************************************************************************************************/
//* ============================================================================================= */
/**
 * \brief   Initializes the Task Monitor.
 *
 * \details This function initializes the Task Monitor. It clears the global TaskInfo object amd assigns a default
 *          reporting time until the respective tasks register.
 *
 * \note    Called in dataManager Init during system initialzation
 *
 * \warning None
 *
 * \param   < None >
 *
 * \return  TASKMONITOR_STATUS
 * \retval  See TaskMonitor.h
 *
 * ============================================================================================= */
TASKMONITOR_STATUS TaskMonitorInit(void)
{
    TimerControl pControl;
    uint8_t u8OsError;
    TASKMONITOR_STATUS Status;
    TIMER_STATUS TimerStatus;
    uint8_t u8TaskPriority;

    Status = TASKMONITOR_ERROR;
    do
    {
        /* Clear the gxTaskInfo array */
        (void)memset(&gxTaskInfo, 0x00, sizeof(gxTaskInfo));

        /* Initialise the System Lifetime Loads. */
        (void)memset( &gxTaskInfoUserPeriod, 0, sizeof( xTaskInformation_t ) );
        /* configure PIT timer1*/
        pControl.TimerId = TIMER_ID2;
        pControl.Mode = TIMER_MODE_PERIODIC;
        pControl.Value = TASKMONITOR_TIMER_MAX_MICROSECONDS-1;
        pControl.pHandler = (&TaskMonitorIntHandler);
        TimerStatus = L2_TimerConfig (&pControl);
        if(TIMER_STATUS_ERROR == TimerStatus)
        {
            Log(ERR,"TaskMonitorInit: L2_TimerConfig Error ");
            Status = TASKMONITOR_ERROR;
            break;
        }

        TimerStatus = L2_TimerStart(TIMER_ID2);
        if(TIMER_STATUS_ERROR == TimerStatus)
        {
            Log(ERR,"TaskMonitorInit: L2_TimerStart Error ");
            Status = TASKMONITOR_ERROR;
            break;
        }
        u8OsError = SigTaskCreate(&TaskMonitorTask,
                                  NULL,
                                  &TaskMonitorStack[0],
                                  TASK_PRIORITY_TASK_MONITOR,
                                  TASK_MON_STACK_SIZE,
                                  "Task Monitor");
        if (u8OsError != OS_ERR_NONE)
        {
            /* Couldn't create task, exit with error */
            Log(ERR, "TaskMonitorInit: OSTaskCreateExt Falied, Error- %d", u8OsError);
            Status = TASKMONITOR_ERROR;
            break;
        }
        for (u8TaskPriority = 0; u8TaskPriority < (OS_LOWEST_PRIO + 1); u8TaskPriority++)
        {
            gxTaskInfo[u8TaskPriority].u32TaskCheckinTimeout = TASKMONITOR_DEFAULT_CHECKIN_TIME;
        }
        /* Set the global flag*/
        TaskMonitorFaults = 0;
        FaultReqRaised = false;
        gbIsTaskMonitorInitialized = true;
        Status = TASKMONITOR_OK;
    } while (false);

    return Status;
}

//* ============================================================================================= */
/**
 * \brief   This function Registers the calling task to the Task Monitor.
 *
 * \details Task monitor also monitors the checkin time of each task when registered. All Registered tasks
 *           need to checkin with a configured checkintime, failing which will cause watchdog reset.
 *
 * \note    Called during system initialzation
 *
 * \warning None
 *
 * \param   checkInTimeout - Timeout value in milliseconds. if the task doesnt checkin within
 *                           this time the task monitor doesnt refresh the watchdog and results
 *                           in a watchdog reset.
 *
 * \return  TASKMONITOR_STATUS
 * \retval  See TaskMonitor.h
 *
 * ============================================================================================= */
TASKMONITOR_STATUS TaskMonitorRegisterTask(uint32_t checkInTimeout)
{
    TASKMONITOR_STATUS Status;
    do
    {
        if (checkInTimeout > TASKMOMITOR_MAX_CHECKIN_TIMEOUT)
        {
            Status = TASKMONITOR_INVALD_PARAM;
            break;
        }
        gxTaskInfo[OSTCBCur->OSTCBPrio].bIsRegistered = true; /* Register with Task Monitor  */
        gxTaskInfo[OSTCBCur->OSTCBPrio].u32TaskCheckinTimeout = checkInTimeout; /* save the reporting timeout */

        if (bIsWdogEnabled == false)
        {
            #ifdef ENABLE_TASKMON_WDOG_FEATURE
            WdogEnable();
            #endif
            bIsWdogEnabled = true;
        }
    } while (false);
    Status = TASKMONITOR_OK;
    return Status;
}

//* ============================================================================================= */
/**
 * \brief   This function Un-Registers the calling task to the Task Monitor.
 *
 * \details Task monitor shall not monitor the checkin time of each task when unregistered.
 *
 * \note
 *
 * \warning None
 *
 * \param   < None >
 *
 * \return  TASKMONITOR_STATUS
 * \retval  See TaskMonitor.h
 *
 * ============================================================================================= */
TASKMONITOR_STATUS TaskMonitorUnRegisterTask(void)
{
    TASKMONITOR_STATUS Status;

    do
    {
        gxTaskInfo[OSTCBCur->OSTCBPrio].bIsRegistered = false; /* un Register with Task Monitor  */
        gxTaskInfo[OSTCBCur->OSTCBPrio].u32TaskCheckinTimeout = TASKMONITOR_DEFAULT_CHECKIN_TIME; /* set the timeout to default reporting time*/

        /*check to disable wdog */
    } while (false);
    Status = TASKMONITOR_OK;
    return Status;

}

//* ============================================================================================= */
/**
 * \brief   This function disables the task monitoring feature.
 *
 * \details Disabling task monitoring causes all tasks to be not monitored for reporting time.
 *          However, the statistics are still calculated without causing any exceptions.
 *
 * \note
 *
 * \warning None
 *
 * \param   < None >
 *
 * \return  TASKMONITOR_STATUS
 * \retval  See TaskMonitor.h
 *
 * ============================================================================================= */

TASKMONITOR_STATUS TaskMonitorDisable( void )
{
    TASKMONITOR_STATUS Status;

    do
    {
        #ifdef ENABLE_TASKMON_WDOG_FEATURE
        /* Disable the watchdog */
        if (bIsWdogEnabled)
        {
            WdogDisable();
        }
        #endif
        bIsTaskMonitorEnabled = false; /* set task monitoring status to Disabled */
    } while (false);

    Status = TASKMONITOR_OK;
    return Status;
}

//* ============================================================================================= */
/**
 * \brief   This function enables the task monitoring feature.
 *
 * \details Enables Task monitoring for all registered tasks. When Enabled, checkin time is updated to all
 *          registered tasks with the current time.
 *
 * \note
 *
 * \warning None
 *
 * \param   < None >
 *
 * \return  TASKMONITOR_STATUS
 * \retval  See TaskMonitor.h
 *
 * ============================================================================================= */
TASKMONITOR_STATUS TaskMonitorEnable(void)
{
    TASKMONITOR_STATUS Status;
    uint8_t u8TaskPriority;
    uint32_t u32TimeNow;
    uint32_t u32TaskCheckinTimeout;

    do
    {
        u32TimeNow  = OSTimeGet();
        OSSchedLock();
        for (u8TaskPriority = 0; u8TaskPriority < (OS_LOWEST_PRIO + 1); u8TaskPriority++)
        {
            if (gxTaskInfo[u8TaskPriority].bIsRegistered)
            {
                u32TaskCheckinTimeout = gxTaskInfo[u8TaskPriority].u32TaskCheckinTimeout;
                (void)memset(&gxTaskInfo[u8TaskPriority], 0x00, sizeof(xTaskInformation_t));

                /* Initialise the System Lifetime Loads. */
                (void)memset( &gxTaskInfoUserPeriod, 0, sizeof( xTaskInformation_t ) );

                /* re-set the checkin timout for all registered tasks */
                gxTaskInfo[u8TaskPriority].bIsRegistered = true;
                gxTaskInfo[u8TaskPriority].u32LastCheckIn = u32TimeNow;
                gxTaskInfo[u8TaskPriority].u32TaskCheckinTimeout = u32TaskCheckinTimeout;
            }
            else
            {
                 /* if not registered already, zero the statistics of task specific global struct */
                 (void)memset(&gxTaskInfo[u8TaskPriority], 0x00, sizeof(xTaskInformation_t));
                /* Initialise the System Lifetime Loads. */
                (void)memset( &gxTaskInfoUserPeriod, 0, sizeof( xTaskInformation_t ) );
                gxTaskInfo[u8TaskPriority].bIsRegistered = false;
            }
        }
        if (OSTCBHighRdy != (OS_TCB *)0)
        {
            /* Update the next incoming task with the tick count */
            if (OSTCBHighRdy->OSTCBPrio <= OS_LOWEST_PRIO)
            {
                /* Update the Tick count*/
                gxTaskInfo[OSTCBHighRdy->OSTCBPrio].u32SwitchedInTick = GET_PIT_CVAL1_TICK;
            }
        }
        OSSchedUnlock();
        #ifdef ENABLE_TASKMON_WDOG_FEATURE
        /* enable the watchdog */
        if (!bIsWdogEnabled)
        {
            WdogEnable();
        }
        #endif
        bIsTaskMonitorEnabled = true;
    } while (false);

    Status = TASKMONITOR_OK;
    return Status;

}

//* ============================================================================================= */
/**
 * \brief   Task Monitor Logging Perdiocity .
 *
 * \details This function Sets the Peridicity for the EVENT Logs (Task Statistics).
 *
 * \note    Parsing Zero as Periodicity will disable the Event Logging.
 *
 * \warning None
 *
 * \param   Seconds - Event Log Period in Seconds
 *
 * \return  None
 *
 * ============================================================================================= */
void TaskMonitorSetLogPeriod(uint8_t Seconds)
{
    u32NextTaskMonitorPrintTime = (Seconds * SEC_1);
}

//* ============================================================================================= */
/**
 * \brief   Updates the Task Watch dog Checkin.
 *
 * \details This function updates the task checkin time.
 *
 * \note    Called in TaskWatchdog() function.
 *
 * \warning None
 *
 * \param   u8TaskPriority - Task Priority
 *
 * \return  TASKMONITOR_STATUS
 * \retval  See TaskMonitor.h
 *
 * ============================================================================================= */
TASKMONITOR_STATUS TaskMonitorTaskCheckin(uint8_t u8TaskPriority)
{
    TASKMONITOR_STATUS Status;

    do
    {
        if (!bIsTaskMonitorEnabled)
        {
            /* if task monitoring disabled, no effect of check-in */
            Status = TASKMONITOR_STATUS_DISABLED;
            break;
        }
        /* if task monitoring disabled, no effect of check-in */
        if (!gxTaskInfo[u8TaskPriority].bIsRegistered)
        {
            /* task monitoring not registered for this specific task ..no effect of check-in */
            Status = TASKMONITOR_STATUS_DISABLED;
            break;
        }
        if (OS_LOWEST_PRIO < u8TaskPriority)
        {
            Status = TASKMONITOR_INVALD_PARAM;
            break;
        }
        /* Update the Checkin Time with current tick count*/
        gxTaskInfo[u8TaskPriority].u32LastCheckIn = OSTimeGet();
        Status = TASKMONITOR_OK;
    } while (false);

    return Status;

}

/* ============================================================================================== */
/**
 * \brief   Monitors the Task Switch
 *
 * \details This function implements the application context-switch hook.
 *          This will be called when context switch occurs in the OS.
 *
 * \note    Called from App_TaskSwHook()
 *
 * \warning None
 *
 * \param   < None >
 *
 * \return  None
 *
 * ============================================================================================== */
#pragma optimize = none
void TaskMonitorTaskSwitch(void)
{
    uint32_t u32TicksNowLocal;

    /* Start logging only after init*/
    do
    {
        /* check if task monitoring is initialized */
        /* No Logging in Application hook function called from context switch hooks */
        if (!gbIsTaskMonitorInitialized)
        {
            break;
        }
        /* check if task monitoring is Enabled */
        if (!bIsTaskMonitorEnabled)
        {
            break;
        }
        /* OSTCBCur points to the task being switched out (i.e. the preempted task). */
        if (OSTCBCur == (OS_TCB *)0)
        {
            break;
        }
        if (OSTCBCur->OSTCBPrio > OS_LOWEST_PRIO)
        {
            break;
        }
        /* Get the current time. */
        u32TicksNowLocal = GET_PIT_CVAL1_TICK;

        /* Consider the PIT overflow */
        if ( u32TicksNowLocal > gxTaskInfo[OSTCBCur->OSTCBPrio].u32SwitchedInTick)
        {
            /* No Overflow. Set current elapsed ticks for Current task */
            gxTaskInfo[OSTCBCur->OSTCBPrio].u32ElapsedTicks =
                u32TicksNowLocal + ~(gxTaskInfo[OSTCBCur->OSTCBPrio].u32SwitchedInTick) + 1UL;
        }
        else
        {   /* Yes, adjust overflow */
            gxTaskInfo[OSTCBCur->OSTCBPrio].u32ElapsedTicks =
                (UINT32_MAX_VALUE     + ~(gxTaskInfo[OSTCBCur->OSTCBPrio].u32SwitchedInTick) + 1UL) +
                    u32TicksNowLocal;
        }

        /* Update the Switchout tick */
        gxTaskInfo[OSTCBCur->OSTCBPrio].u32SwitchedOutTick = u32TicksNowLocal;

         /* Update the culmulative count */
        gxTaskInfo[OSTCBCur->OSTCBPrio].u32CumulativeElapsedTicks +=
            gxTaskInfo[OSTCBCur->OSTCBPrio].u32ElapsedTicks;

         /* Increment the Context Switch count */
        gxTaskInfo[OSTCBCur->OSTCBPrio].u32ContextSwitches++;

        /* Peak Execution Tick. */
        if (gxTaskInfo[OSTCBCur->OSTCBPrio].u32ElapsedTicks >
            gxTaskInfo[OSTCBCur->OSTCBPrio].u32PeakElapsedTick)
        {
            gxTaskInfo[OSTCBCur->OSTCBPrio].u32PeakElapsedTick =
                gxTaskInfo[OSTCBCur->OSTCBPrio].u32ElapsedTicks;
        }
    } while (false);

    /* OSTCBHighRdy points to the TCB of the task that will be switched to */
    /* Set start tick for Next High priority task */
    if ((OSTCBHighRdy != (OS_TCB *)0) &&  (bIsTaskMonitorEnabled))
    {
        if (OSTCBHighRdy->OSTCBPrio <= OS_LOWEST_PRIO)
        {
            /* Update the Tick */
            gxTaskInfo[OSTCBHighRdy->OSTCBPrio].u32SwitchedInTick = GET_PIT_CVAL1_TICK;
        }
    }
}

//* ============================================================================================= */
/**
 * \brief   Main function which calculates the System Load.
 *
 * \details This function is called from App_TaskStatHook() and executed every
 *          TASK_MONITOR_UPDATE_PERIOD time. The system loads are calculated from individual
 *          gxTaskInfo array and updated into gxTaskInfoUserPeriod object. This is then referenced
 *          with total ticks for every period to get the system load. The main steps are
 *          1) Udpate the gxTaskInfo objects in a loop of priorities
 *          2) Udpate the gxTaskInfoUserPeriod object from all gxTaskInfo Task objects
 *          3) Find the max and other parameters with gxTaskInfoUserPeriod object
 *          4) Find Interrupt overhead ticks
 *
 * \note    Called from App_TaskStatHook()
 *
 * \warning None
 *
 * \param   < None >
 *
 * \return  None
 *
 * ============================================================================================= */
#pragma optimize = none
void TaskMonitorUpdateLoads(void)
{
    uint8_t u8TaskPriority;
    uint32_t u32TotalTime;
    uint32_t u32TicksNowLocal;
    uint32_t u32TimeNow;
    static uint32_t u32NextTaskMonitorUpdateTime;

    u8TaskPriority = 0x0u;
    u32TotalTime   = 0x0u;
    u32TimeNow     = 0x0u;
    u32NextTaskMonitorUpdateTime = TASK_MONITOR_UPDATE_PERIOD;

    do
    {
        /* Start logging only after init*/
        if (!gbIsTaskMonitorInitialized)
        {
            break;
        }
        /* check if task monitoring is Enabled */
        if (!bIsTaskMonitorEnabled)
        {
            break;
        }
        /* Get the current time in millisecs */
        u32TimeNow  = OSTimeGet();

        /* Task Monitor is ready, Is it correct time to run */
        if (u32TimeNow < u32NextTaskMonitorUpdateTime)
        {
            break;
        }
        /* Get the current time. */
        u32TicksNowLocal = GET_PIT_CVAL1_TICK;

        OSSchedLock();
        {
            /* Consider the PIT overflow */
            if (u32TicksNowLocal > gxTaskInfoUserPeriod.u32SwitchedOutTick)
            {
                /* No Overflow. Set current elapsed ticks for Current task */
                u32TotalTime = u32TicksNowLocal + ~(gxTaskInfoUserPeriod.u32SwitchedInTick) + 1UL;
            }
            else
            {   /* Yes, adjust overflow */
                u32TotalTime = (UINT32_MAX_VALUE     + ~(gxTaskInfoUserPeriod.u32SwitchedInTick) + 1UL) +
                    u32TicksNowLocal;
            }

            /* Record the time now. */
            gxTaskInfoUserPeriod.u32SwitchedOutTick = u32TicksNowLocal;

            /* Adjust total time down to 100ths of a percentage. */
            u32TotalTime /= 10000UL;

            /* Reset the variables. */
            gxTaskInfoUserPeriod.u32LoadAverageOneUserPeriod = 0UL;
            gxTaskInfoUserPeriod.u32CumulativeElapsedTicks = 0UL;
            gxTaskInfoUserPeriod.u32ContextSwitchesOneUserPeriod = 0UL;

            /* Find the total time. */
            for (u8TaskPriority = 0; u8TaskPriority < (OS_LOWEST_PRIO + 1); u8TaskPriority++)
            {
                if ((OSTCBPrioTbl[u8TaskPriority] != NULL) && (OSTCBPrioTbl[u8TaskPriority]->OSTCBCtxSwCtr > 1))
                {
                    /* 1) Udpate the gxTaskInfo objects */

                    /* Record the Context Switches. */
                    gxTaskInfo[u8TaskPriority].u32ContextSwitchesOneUserPeriod =
                        gxTaskInfo[u8TaskPriority].u32ContextSwitches;

                    /* Given the time period, how many whole timeslices were used. */
                    gxTaskInfo[u8TaskPriority].u32ElapsedTicksOneUserPeriod =
                        gxTaskInfo[u8TaskPriority].u32CumulativeElapsedTicks;
                    if (0u != u32TotalTime)
                    {
                        gxTaskInfo[u8TaskPriority].u32LoadAverageOneUserPeriod =
                            gxTaskInfo[u8TaskPriority].u32CumulativeElapsedTicks / u32TotalTime;
                    }
                    /* Peak Load for any User Period. */
                    if ( gxTaskInfo[u8TaskPriority].u32LoadAverageOneUserPeriod >
                        gxTaskInfo[u8TaskPriority].u32LoadPeakOneUserPeriod )
                    {
                        gxTaskInfo[u8TaskPriority].u32LoadPeakOneUserPeriod =
                            gxTaskInfo[u8TaskPriority].u32LoadAverageOneUserPeriod;

                        /* check if the Task load is less than the Load Threahold. Idle task is exception */
                        if ((TASKMONITOR_PEAKLOAD_THRESHOLD < gxTaskInfo[u8TaskPriority].u32LoadPeakOneUserPeriod) && (u8TaskPriority < OS_TASK_IDLE_PRIO))
                        {
                            Log(DBG,"CPU Utilization for task %d is greater than 90% ",u8TaskPriority);

                            /// \todo 08/22/2022 CPK current build(1.1.38) has STARTUP task load > 90%. This is causing handle to goto Request reset state on startup
                            /// \todo 08/22/2022 CPK remove the below check for Startup task after the startup task load is fixed.
                            if (TASK_PRIORITY_STARTUP != u8TaskPriority)
                            {
                                TASKMONITOR_SETFAULTBIT(TaskMonitorFaults, TASK_LOADCHECK_FAIL);
                                /*  Load check failure indicates the task is blocking lower priority tasks to Run
                                    Lowering the priority of current "faulty" task so DISPLAY task can show the required screens  Load check fail causes Request Reset state.
                                    Since the next state is Request Reset, lowering the "Faulty" task priority has no impact on the system.
                                */
                                if (TASK_PRIORITY_L4_DISPMANAGER != u8TaskPriority)
                                    OSTaskChangePrio(u8TaskPriority, TASK_PRIORITY_LAST);
                            }
                        }
                    }

                    /* Peak Execution Time for any User Period. */
                    if ( gxTaskInfo[u8TaskPriority].u32ElapsedTicks >
                        gxTaskInfo[u8TaskPriority].u32PeakElapsedTick )
                    {
                        gxTaskInfo[u8TaskPriority].u32PeakElapsedTick =
                            gxTaskInfo[u8TaskPriority].u32ElapsedTicks;
                    }

                    /* Cumulative Peak Execution Time for any User Period */
                    if ( gxTaskInfo[u8TaskPriority].u32CumulativeElapsedTicks >
                        gxTaskInfo[u8TaskPriority].u32PeakCumulativeElapsedTicks )
                    {
                        gxTaskInfo[u8TaskPriority].u32PeakCumulativeElapsedTicks =
                            gxTaskInfo[u8TaskPriority].u32CumulativeElapsedTicks;
                    }

                    if (OSTCBPrioTbl[u8TaskPriority]->OSTCBStat == OS_STAT_RDY)
                    {
                        /* Find the Current and Peak Checkin Difference for any User Period */
                        if ( u32TimeNow > gxTaskInfo[u8TaskPriority].u32LastCheckIn)
                        {
                            gxTaskInfo[u8TaskPriority].u32CheckInDifference = u32TimeNow +
                                ~(gxTaskInfo[u8TaskPriority].u32LastCheckIn) + 1UL;
                        }
                        else
                        {
                            gxTaskInfo[u8TaskPriority].u32CheckInDifference = 0;
                        }

                        if ( gxTaskInfo[u8TaskPriority].u32CheckInDifference >
                            gxTaskInfo[u8TaskPriority].u32PeakCheckInDifference )
                        {
                            gxTaskInfo[u8TaskPriority].u32PeakCheckInDifference =
                                gxTaskInfo[u8TaskPriority].u32CheckInDifference;
                        }
                    }

                    /* 2) Update the gxTaskInfoUserPeriod object from all gxTaskInfo Task objects */

                    /* Include the Idle Task for the Context Switches. */
                    gxTaskInfoUserPeriod.u32ContextSwitchesOneUserPeriod +=
                        gxTaskInfo[u8TaskPriority].u32ContextSwitches;

                    /* Exclude the IDLE task. IDLE task got lowest priority */
                    if ( u8TaskPriority != OS_TASK_IDLE_PRIO )
                    {
                        gxTaskInfoUserPeriod.u32LoadAverageOneUserPeriod +=
                            gxTaskInfo[u8TaskPriority].u32LoadAverageOneUserPeriod;
                        gxTaskInfoUserPeriod.u32CumulativeElapsedTicks +=
                            gxTaskInfo[u8TaskPriority].u32CumulativeElapsedTicks;
                    }
                    else
                    {
                        /* Record specificaly the Idle task here for Interrupt overhead. */
                        gxTaskInfoUserPeriod.u32TicksSuspended =
                            gxTaskInfo[u8TaskPriority].u32CumulativeElapsedTicks;
                    }

                    /* Reset. */
                    gxTaskInfo[u8TaskPriority].u32ContextSwitches = 0UL;
                    gxTaskInfo[u8TaskPriority].u32CumulativeElapsedTicks = 0UL;
                }

                /* Measure the Stack freespace (adjusted to 10th of percent) for each task*/
                /*if ((OSTCBPrioTbl[u8TaskPriority] != NULL))
                {
                    gxTaskInfo[u8TaskPriority].u32FreeStackSpace = (uint32_t)(((uint32_t)(OSTCBPrioTbl[u8TaskPriority]->OSTCBStkSize - OSTCBPrioTbl[u8TaskPriority]->OSTCBStkUsed) * 1000) / OSTCBPrioTbl[u8TaskPriority]->OSTCBStkSize);
                    if (TASKMONITOR_STACKSPACE_LOWTHD > gxTaskInfo[u8TaskPriority].u32FreeStackSpace)
                    {
                        Log(DBG,"Available Stack space less than 10%");
                        TASKMONITOR_SETFAULTBIT(TaskMonitorFaults, TASK_STACKCHECK_FAIL);
                    }
                }*/
            }

            /* 3) Find the max and other parameters with gxTaskInfoUserPeriod object */
            ComputeTaskInfoParams(u32TotalTime);
             /* Record the start time for the next Load period. */
            gxTaskInfoUserPeriod.u32SwitchedInTick = gxTaskInfoUserPeriod.u32SwitchedOutTick;

             /* Set the next update period. */
            u32NextTaskMonitorUpdateTime = u32TimeNow + TASK_MONITOR_UPDATE_PERIOD;
        }
        OSSchedUnlock();
    } while (false);
}

/**
 * \}   <this marks the end of the Doxygen group>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

