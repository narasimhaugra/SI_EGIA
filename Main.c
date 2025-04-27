  #ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Main
 * \{
 * \brief   Main entry function
 *
 * \details This file contians main entry function initializing system, hardware,
 *          Operating system, and launches application modules.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Main.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Common.h"
#include "CpuInit.h"
#include "ActiveObject.h"
#include "L2_Init.h"
#include "L3_Init.h"
#include "L3_GpioCtrl.h"
#include "L4_Init.h"
#include "L5_Init.h"
#include "Logger.h"
#include "McuX.h"
#include "TaskMonitor.h"
#include "FaultHandler.h"
#include "TestManager.h"
#include "BackgroundDiagTask.h"
#include "NoInitRam.h"

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER     (LOG_GROUP_MAIN)    ///< Log Group Identifier
#define STARTUP_TASK_STACK_SIZE  ((uint32_t)1024u)   ///< Startup Task Stack Size
#define MAIN_LOOP_DELAY          ((uint32_t)(1000u)) ///< Main While Loop Delay

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
  
/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
 OS_STK StartupTaskTask[STARTUP_TASK_STACK_SIZE+MEMORY_FENCE_SIZE_DWORDS];

/******************************************************************************/
/*                             Global Function Prototype(s)                    */
/******************************************************************************/
extern void Common_Startup(void);
extern void wdog_refresh(void);

#ifdef SIGNIA_APP_EGIA
extern void SigniaAppInitEgia(void);  /// \todo 10/19/2021 DAZ -  This creates new problems & doesn't solve the old ones. EGIA is NOT an app.
#endif

#ifdef SIGNIA_APP_EEA
extern void SigniaAppInitEea(void);
#endif

#ifdef SIGNIA_APP_NGSL
extern void SigniaAppInitNgsl(void);
#endif

#ifdef  TEST_STUBS
extern void TestStubsInit(void);
#endif

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void HardwareInit(void);
static void StartupTask(void *p_arg);

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static uint8_t *StartTaskName = "Startup";

/* ========================================================================== */
/**
 * \brief   main entry function
 *
 * \details Initializes all hardware, RTOS, application modules.
 *
 * \note    Initialize all hardware, operating system and create application
 *          tasks.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void main(void)
 {
   SigTimeSet(0);  // coming out of reset, initialize cycle counter to 0

   /* Copy any vector or data sections that need to be in RAM */
   Common_Startup();

   HardwareInit();

   /// \todo 04/12/2022 KA: zero initialize external byte wide sram. Review 1MB_Pflash.icf
   uint8_t     *bss_start = (uint8_t *)0xC0000000u;
   uint32_t             n = 0x00040000u; // 256Kbytes (2Mbit)
   while (n--)
   {
      *bss_start++ = 0u;
   }

   /* Initialize active object framework. Also initializes micrium */
   AO_Init();

   /* Create the Startup task - \todo: Remove this task eventually */
   SigTaskCreate(StartupTask,
                        NULL,
                        &StartupTaskTask[0],
                        TASK_PRIORITY_STARTUP,
                        STARTUP_TASK_STACK_SIZE,
                        StartTaskName);

   /* QF_run calls OSStart() */
   QF_run();        /* Start operating system (This will never return) */
}

/* ========================================================================== */
/**
 * \brief   Hardware Init
 *
 * \details Initializes all hardware
 *
 * \note    Initialize Clock, Basic IOs, RTOS Tick.
 *          tasks.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
 static void HardwareInit(void)
 {
    SigniaCpuInit();     /* Perform processor initialization */
    CPU_Init();          /* Is this really needed??? OSinit() should take of CPU init. */
    Mem_Init();          /* Initialize the Memory Management Module */
    Math_Init();         /* Initialize the Mathematical Module */
    McuXInit();          /* Initialize the MCU Exception Handler */
 }

/* ========================================================================== */
/**
 * \brief   main entry function
 *
 * \details Initializes all hardware, RTOS, application modules.
 *
 * \note    Initialize All hardware, operating system and creat applicaiton
 *          tasks.
 *
 * \param   p_arg - Task argument
 *
 * \return  None
 *
 * ========================================================================== */
static void StartupTask(void *p_arg)
{
    bool Status;            /* Module initialization status */
    uint32_t HeartBeatPeriod;
    bool L2Status;
    bool L3Status;
    bool L4Status;
    bool L5Status;

    Status = true;          /* Set to error by default */

    /*  Statistics initialization - used by Task monitor*/
    OSStatInit();

    /* Initialize fault handler to handle Startup Errors */
    FaultHandlerInit();

    L2Status = L2_Init();   /* Layer 2 initialization. */

    // The NoInitRam will be cleared when the battery is removed or if there is a deep discharge of the battery.
    // The magic number signifies the NoInitRam is set to valid parameters.
    // This number is used by both Bootloader and Main.
    // Change the magic number whenever there is a change to the noInitRam structure.
    // Changing the magic number will cause the NoInitRam data in memory to be cleared.
    // If the magic number is changed, both the main code and boot code need to be rebuilt.
    // If they are not both rebuilt after changing the magic number,
    // both the boot and main code will reset the NoInitRam on every startup.
    if (noInitRam.MagicNumber != NO_INIT_RAM_MAGIC_NUMBER)
    {
        memset((char *)&noInitRam, 0, sizeof(noInitRam));
        noInitRam.MagicNumber = NO_INIT_RAM_MAGIC_NUMBER;
        noInitRam.NoInitRamWasReset = true;
    }
    else
    {
        noInitRam.NoInitRamWasReset = false;
    }

    Log (REQ, "Bootloader status:");
    if ( noInitRam.NoInitRamWasReset )
    {
        Log (REQ, "  Unknown - noinit RAM was corrupt.");
    }
    else
    {
        Log (REQ, "  Blob is valid:                              %d", noInitRam.BootStatus.bit.BlobValid);
        Log (REQ, "  Blob Main app is encrypted:                 %d", noInitRam.BootStatus.bit.BlobEncrypted);
        Log (REQ, "  Blob Main app timestamp newer than handle:  %d", noInitRam.BootStatus.bit.BlobNewerTimestamp);
        Log (REQ, "  Handle Main app invalid:                    %d", noInitRam.BootStatus.bit.HandleMainCorrupt);
        Log (REQ, "  Handle Main app was updated:                %d", noInitRam.BootStatus.bit.HandleUpdate);
        Log (REQ, "  If Handle Main App updated, success:        %d", noInitRam.BootStatus.bit.HandleUpdateSuccess);
    }

    TaskMonitorInit();      /* Task Monitor functionality initialization */

    TestManagerCtor();

    L3Status = L3_Init();   /* Layer 3 initialization. */

    L4Status = L4_Init();   /* Layer 4 initialization. */

    /* Update Bootloader in Handle flash? (must come after L4_BlobHandlerInit()) */
    (void) L4_CheckHandleBootloader();

    /* Update FPGA? */
    (void) L4_CheckFPGA();

    L5Status = L5_Init();   /* Layer 5 and Clinical Common App initialization. */

    BackgroundDiagTaskInit();

#ifdef  TEST_STUBS
        TestStubsInit();        /* Test stub initialize functions MUST NOT block */
#endif
    Status = L2Status || L3Status || L4Status || L5Status;          /* All good, no errors */

    Log(REQ, "PowerPack initialization %s", (Status) ? "Failed" : "Successful");

    // 12/02/2021 DAZ - NOTE: If initialization has failed, this implements ST_ERR_PERM_FAIL in that
    // 12/02/2021 DAZ -       App has not started, and no operation is possible until a hard reset is issued.
    // 12/02/2021 DAZ -       The processor COULD be halted at this point to disable ALL functionality.

/// \\todo 02-13-2022 KA : don't think this SigTimeSet() is wanted
//    SigTimeSet(SIGNIA_RTC_DEFAULT_VALUE);
//    Log(REQ, "Handle Time [%uld] set to default [%uld]", SigTime(), SIGNIA_RTC_DEFAULT_VALUE);

/// \todo 12/01/2021 DAZ - Remove the following. All AppInit is performed by the entry function of the respective top level state.
#ifdef UNUSED_CODE       // { 12/01/2021 DAZ - This just creates an additional dependency between Platform & Applications
    // Clinical application entry hook functions. This function will be called
    // Only when the platform code is build with clinical applications such as
    // EGIA, EEA, NGSL, etc.

#ifdef SIGNIA_APP_EGIA
    SigniaAppInitEgia();    /// \todo 10/19/2021 DAZ -  This creates new problems & doesn't solve old ones. EGIA is NOT an app! It is part of the App state machine.
                            /// \todo 10/19/2021 DAZ -  The EGIA state machine takes care of its own initialization
#endif

#ifdef SIGNIA_APP_EEA
    SigniaAppInitEea();
#endif

#ifdef SIGNIA_APP_NGSL
    SigniaAppInitNgsl();
#endif
#endif                   // UNUSED_CODE

    /* Dead end this task here */

    while (true)
    {
        /* Get Heart Beat Led On Off Time */
        HeartBeatPeriod = GetHeartBeatLedPeriod();
        OSTimeDly(HeartBeatPeriod);

        if (Status)
        {
            L3_GpioCtrlSetSignal(GPIO_IM_GOOD);   // Keep the LED on in case of fault
        }
        else
        {
            /// \todo  02/17/21 GK - Heart Beat LED; helps in Batt Shutdown, McuX, Deadlock scenarios.
            /// \todo  02/17/21 GK - Revisit its place in future, Start-Up Application?
            L3_GpioCtrlToggleSignal(GPIO_IM_GOOD);
        }
    }
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
 }
#endif
