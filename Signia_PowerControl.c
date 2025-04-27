#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_PowerControl
 * \{
 *
 * \brief   Display Manager Functions
 *
 * \details Display manager allows applications to develop graphical content in
 *          the form of screens. A screen is a collection of graphical elements
 *          called widgets(Window Objects) and methods to interact with them.
 *          Display manager is responsible drawing these elements on screen,
 *          refresh widgets periodically or when updated
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_PowerControl.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Common.h"                 // Import common definitions such as types, etc.
#include "Mcg.h"
#include "L2_Lptmr.h"
#include "L2_Llwu.h"
#include "L2_PortCtrl.h"
#include "L3_GpioCtrl.h"            // Import Gpio controller module interfaces
#include "L3_OneWireController.h"   // Import 1-Wire controller interfaces
#include "L3_FpgaMgr.h"             // Import FPGA Manager interfaces
#include "Signia_Accelerometer.h"   // Import accelerometer module interfaces
#include "L3_DispPort.h"            // Import display module interfaces
#include "L3_Battery.h"             // Import battery module interfaces

#include "Signia_PowerControl.h"        // Import power controller module interfaces
#include "Signia_CommManager.h"     // Import communication manager interfaces
#include "Signia_Keypad.h"          // Import Keypad module interfaces
#include "Signia_AdapterManager.h"  // Import Adapter manager module
#include "Signia_BatteryHealthCheck.h"
#include "McuX.h"                   // Import McuX interfaces

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER              (LOG_GROUP_POWER)     ///< Log Group Identifier

// Peripheral Wake Up mask.
// Signaled from the wired-or of ADAPTER_MONn, SHELL_MONn, and CHARGER_ENn
#define PERIPHERAL_WU_PIN_MASK             (0x00000001)
#define GPIO_PTB19_MASK                    (0x00080000)
#define GPIO_PTC16_MASK                    (0x00010000)

#define PERIPHERAL_WU         LLWU_P5
#define KEY_WAKE              LLWU_P7
#define WUP_LPTRMTIMEOUT      LLWU_M0IF_LPTRM

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

typedef struct
{
    SLEEP_CAUSE PowerMode_SleepCause;
    POWER_MODE  ActivePowerMode;
} POWERMODEINFO;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static POWERMODEINFO PowerModeInfo;

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static POWER_STATUS PowerModeSetActive(void);
static POWER_STATUS PowerModeSetStandby(void);
static POWER_STATUS PowerModeSetSleep(void);

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Activate normal power mode
 *
 * \details In this mode most of modules are enabled
 *
 * \param   < None >
 *
 * \return  POWER_STATUS - Status
 *
 * ========================================================================== */
static POWER_STATUS PowerModeSetActive(void)
{
    POWER_STATUS Status;

    do
    {
        Status = POWER_STATUS_ERROR;

        // turn on Piezo
        L3_GpioCtrlClearSignal(GPIO_PZT_EN);
        OSTimeDly(10);
        // Set 3V rail
        L3_GpioCtrlSetSignal(GPIO_EN_3V);
        OSTimeDly(10);

        // Enable display power rail
        L3_GpioCtrlSetSignal(GPIO_EN_VDISP);

        // Turn on display
        L3_DisplayOn(true);

        // Disable all 1-Wire communication
        BREAK_IF(ONEWIRE_STATUS_OK != L3_OneWireEnable(true));
        OSTimeDly(100);

        // Enable FPGA HW
        BREAK_IF(FPGA_MGR_OK != L3_FpgaMgrSleepEnable(false));

        Status = POWER_STATUS_OK;
    }while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Activate Standby mode
 *
 * \details Activates Standby power mode.
 *
 * \param   < None >
 *
 * \return  POWER_STATUS - Status
 *
 * ========================================================================== */
static POWER_STATUS PowerModeSetStandby(void)
{
    POWER_STATUS Status;

    do
    {
        Status = POWER_STATUS_ERROR;

        // Turn off display
        L3_DisplayOn(false);

        Status = POWER_STATUS_OK;
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Configure LLWU Modules
 *
 * \details This function configures LLWU registers. LLWU is enabled for KEY_WAKEn,
 *          and PERIPHERAL_WUn.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void ConfigureLLWUPins(void)
{
    // enable KEYWAKEn as a wakeup input pin
    // clear interrupt flag, set pin functionality (GPIO), falling edge interrupt enable
    L2_PortCtrlConfigPin(PORTC_BASE_PTR, GPIO_PIN_03, (PORT_PCR_ISF_MASK | PORT_PCR_MUX(1)|PORT_PCR_IRQC(0x0A)) );

    // enable PERIPHERAL_WUn as a wakeup input pin
    // clear interrupt flag, set pin functionality (GPIO), On rising edge interrupt enable
    L2_PortCtrlConfigPin(PORTB_BASE_PTR, GPIO_PIN_00, (PORT_PCR_ISF_MASK | PORT_PCR_MUX(1)|PORT_PCR_IRQC(0x09)) );


    // Turn on the LLWU clock to enable the LLWU inputs to be wake-up sources
    LLWU_CLOCKENABLE();

    // Enable various LLWU inputs to cause the system to wake from deep sleep
    // falling edge detection for input keys
    L2_LlwuSetWakeupSource(KEY_WAKE, WAKEUP_FALLINGGEDGE);
    // either edge detection for CHARGE_EN
    L2_LlwuSetWakeupSource(PERIPHERAL_WU, WAKEUP_ANYEDGE);
    // LPTMR (LLWU_M0IF)
    L2_LlwuSetWakeupSource(WUP_LPTRMTIMEOUT, WAKEUP_RAISINGEDGE_FLAG);

    LLWU_IRQEN();  // Enable LLWU IRQ in NVIC


    MCG_CLKMON_DISABLE();        // CME=0 clock monitor disable

    // Write to PMPROT to allow LLS & VLLS power modes
    SMC_PMPROT = SMC_PMPROT_ALLS_MASK | SMC_PMPROT_AVLLS_MASK;
}

/* ========================================================================== */
/**
 * \brief   Put CPU in LLS mode
 *
 * \details  This function is to be run from RAM, because coming out of sleep
 *           from the WFI command can cause hard faults if the flash memory is
 *           shut down.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
// Ignore Warning[Ta023]: Call to a non __ramfunc function.
// It's OK to call non __ramfunc functions after we're sure flash has been initialized.
#pragma diag_suppress=Ta023
// We have to disable optimization for this function, or the time delay for flash
// initialization will get optimized out.
#pragma optimize = none

__ramfunc void SM_WaitForInterrupt(void)
{
    uint16_t        timeNow;
    bool            OnChargerStatus;
    uint16_t        Temp16;
    uint8_t         Size;
    BATTERY_STATUS  BatSts;

    SetSystemStatus(SYSTEM_STATUS_DEEP_SLEEP_ACTIVATED);
    OSSchedLock();

    if ((SLEEP_CAUSE_CHARGER == PowerModeInfo.PowerMode_SleepCause) || (SLEEP_CAUSE_BATTERY_CHECK == PowerModeInfo.PowerMode_SleepCause))
    {
        L2_LptrmStart();
    }

    SCB_SCR |= SCB_SCR_SLEEPDEEP_MASK;          // enable deep sleep mode
    DisableInterrupts;                          // disable all configurable interrupts
    SMC_PMCTRL = SMC_PMCTRL_STOPM(3);           // set STOPM = Low leakage stop (LLS)
    volatile uint32_t dummyread = SMC_PMCTRL;   // do a dummy read to give time for LLS to set

    // Data Synchronization Barrier acts as a special kind of memory barrier.
    // No instruction in program order after this instruction executes until
    // this instruction completes. This instruction completes when:
    //    All explicit memory accesses before this instruction complete.
    //    All Cache, Branch predictor and TLB maintenance operations before this instruction complete.
    asm("DSB");

    // WFI instruction will start entry into low-power mode
    // From the ARMv7-M Architecture Reference Manual, page B1-684:
    // "a processor can exit the low-power state spuriously"
    asm("WFI");

    // Coming out of sleep mode...
    // check if wakeup is due to LPTMR => wake from sleep for battery health check
    // Wait a short time to allow flash memory to initialize
    // Also briefly flash the LED
    /* at this point the MCG is not in PEE mode (PLL is disabled. Refer K20P144M120SF3_ReferenceManual - MCG section).
       PLL is enabled back after the delay */
    L3_GpioCtrlSetSignal(GPIO_IM_GOOD);

    for (timeNow = 0; timeNow < 0x2FFF; timeNow++)
    {
        asm("nop");
    }

    L3_GpioCtrlClearSignal(GPIO_IM_GOOD);

    MCGC1_SET(0x20);                                        // set CLKS to 00 - to set the MCG in PEE mode
    MCG_CLKMON_ENABLE();                                    // CME=1 clock monitor Enable
    MCG_WAIT_FOR_PLL();                                     // Wait until the pll is enabled
                                                            // Enable various LLWU inputs to cause the system to wake from deep sleep



    if (L2_LlwuGetWakeupFlagStatus(LLWU_M0IF_LPTRM))
    {
        L2_LptrmStop();
        EnableInterrupts;

        // falling edge detection for input keys
        L2_LlwuClearWakeupSource(PERIPHERAL_WU);
        // either edge detection for CHARGE_EN
        L2_LlwuClearWakeupSource(LLWU_P5);
        // LPTMR (LLWU_M0IF)
        L2_LlwuClearWakeupSource(WUP_LPTRMTIMEOUT);


        Signia_ChargerManagerSetWakupstate(true);
        SetSystemStatus(SYSTEM_STATUS_LLS_RESET);


        OSSchedUnlock();
        OSTimeDlyResume(TASK_PRIORITY_L4_CHARGER_MANAGER); // This shall elapse the delay timer and trigger a context switch to Chargermanager
    }
    else // cause of wakeup is other than Lowpower timer wakeup then reset
    {
        L2_LptrmStop();
        EnableInterrupts;

        // falling edge detection for input keys
        L2_LlwuClearWakeupSource(PERIPHERAL_WU);
        // either edge detection for CHARGE_EN
        L2_LlwuClearWakeupSource(LLWU_P5);
        // LPTMR (LLWU_M0IF)
        L2_LlwuClearWakeupSource(WUP_LPTRMTIMEOUT);
        OSSchedUnlock();

        BatSts = L3_BatteryGetStatus(CMD_GAUGING_STATUS, &Size, (uint8_t *)&Temp16); // Read Battery Gaug Status
        L3_GpioCtrlGetSignal((GPIO_SIGNAL)GPIO_PERIPHERAL_WUn, (bool *)&OnChargerStatus);
        OnChargerStatus = !OnChargerStatus;

        if ((OnChargerStatus) && (BatSts == BATTERY_STATUS_OK) && (GET_BIT(Temp16, BATTERY_TCABIT)))
        {
            //reset Bq chip
            ///\todo 3/14/2022 - BS: Log BQ chip Reset message. this to be done after bootup as calling Rom function from ram is raising a warning.
            L3_BatteryResetBQChip();
            SetSystemStatus(SYSTEM_STATUS_BATTERY_SHUTDOWN);
            OSTimeDly(200); // Time delay for the BQ chip to reset
        }
        else
        {
            /* Do Nothing - only Reset */
        }

        ClearSystemStatus();
        SoftReset();  //reset device
    }
}

/* ========================================================================== */
/**
 * \brief   Prepare to go into deep sleep
 *
 * \details Puts the CPU and various modules into very low-power mode and
 *          halts the CPU.Function does not return until being awakened from sleep.
 *
 * \param   < None >
 *
 * \return  none
 *
 * ========================================================================== */
static void PrepareForDeepSleep(void)
{
    ///\note the below code should not be enabled. Reason: any change to the OLED flexbus pins is causing the external sram corruption (as both share the flexbus) and resulting in BUS exceptions.
    // Set the OLED CS low (pin 19)
    //PORTC_PCR16 = PORT_PCR_MUX(1);   // set pin functionality (GPIO)
    //GPIOC_PDDR |= GPIO_PTC16_MASK;   // Output
    //GPIOC_PCOR = GPIO_PTC16_MASK;    // Low

    // Set the OLED E/RD low (pin 13)
    //PORTB_PCR19 = PORT_PCR_MUX(1);   // set pin functionality (GPIO)
    //GPIOB_PDDR |= GPIO_PTB19_MASK;   // Output
    //GPIOB_PCOR = GPIO_PTB19_MASK;    // Low

    // Turn off Status LED
    L3_GpioCtrlClearSignal(GPIO_SDHC0_LED);
    L3_GpioCtrlClearSignal(GPIO_IM_GOOD);

    //configure LLU pins
    ConfigureLLWUPins();

    // Change the watchdog so it is disabled in wait mode and stop mode.
    // It also won't generate an interrupt upon activation, it will just
    // cause a reset.
    ///\todo 3/3/2022 - BS: The below code should be revisited once Watchdog has been enabled
//   DisableInterrupts;
//   WDOG_UNLOCK = 0xC520;   // Write 0xC520 to the unlock register
//   WDOG_UNLOCK = 0xD928;   // Followed by 0xD928 to complete the unlock
//   WDOG_STCTRLH = 0x13;
//   EnableInterrupts;

    SM_WaitForInterrupt();  // This does not return
}

/* ========================================================================== */
/**
 * \brief   Activate sleep mode
 *
 * \details Activates sleep power mode.
 *
 * \param   < None >
 *
 * \return  POWER_STATUS - status
 *
 * ========================================================================== */
static POWER_STATUS PowerModeSetSleep(void)
{
    POWER_STATUS Status;
    Status = POWER_STATUS_OK;

    do
    {
        BREAK_IF(GetSystemStatus(SYSTEM_STATUS_DEEP_SLEEP_ACTIVATED));

        Status = POWER_STATUS_ERROR;
        BREAK_IF(POWER_STATUS_OK != PowerModeSetStandby());
        // turn off Piezo
        BREAK_IF(GPIO_STATUS_OK != L3_GpioCtrlSetSignal(GPIO_PZT_EN));      // Active low signal, gets enabled in FpgaMgr.c
                                                                            // Clear 3V rail
        BREAK_IF(GPIO_STATUS_OK != L3_GpioCtrlClearSignal(GPIO_EN_3V));     // Gets enabled in DispPort.c during reboot
                                                                            // Disable display power rail
        BREAK_IF(GPIO_STATUS_OK != L3_GpioCtrlClearSignal(GPIO_EN_VDISP));  // Gets enabled in DispPort.c during reboot
        OSTimeDly(10);
        BREAK_IF(GPIO_STATUS_OK != L3_GpioCtrlClearSignal(GPIO_EN_BATT_15V));
        OSTimeDly(10);
        BREAK_IF(GPIO_STATUS_OK != L3_GpioCtrlSetSignal(GPIO_EN_2P5V));     // disable 2.5 ADC reference
        OSTimeDly(10);
        Status = POWER_STATUS_OK;
        // suspend the DisplayManager task as the OLED is off and not updating any screens.
        OSTaskSuspend(TASK_PRIORITY_L4_DISPMANAGER);
    } while (false);

    // Prepare to go into deep sleep
    if (POWER_STATUS_OK == Status)
    {
        PrepareForDeepSleep();
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   PowerMode SetShip
 *
 * \details Prepares device for ship mode.
 *
 * \param   < None >
 *
 * \return  POWER_STATUS - returns error if function fails
 *
 * ========================================================================== */
static POWER_STATUS PowerModeSetShip(void)
{
    POWER_STATUS Status;

    do
    {
        Status = POWER_STATUS_ERROR;

        // Shutdown battery - CPU and entire HW loses power after this function call.
        BREAK_IF(BATTERY_STATUS_OK != L3_BatteryShutdown());

        // Place holder function to handle ship mode implementation.

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Shutdown
 *
 * \details Battery is below operating voltage, shutdown the device
 *
 * \param   < None >
 *
 * \retval  None
 *
 * ========================================================================== */
void PowerModeShutdown(void)
{
    BATTERY_STATUS Status;

    Status = BATTERY_STATUS_ERROR;

    PowerModeSetStandby();

    // turn off Piezo
    L3_GpioCtrlSetSignal(GPIO_PZT_EN);

    // Clear 3V rail
    L3_GpioCtrlSetSignal(GPIO_EN_3V);

    // Disable display power rail
    L3_GpioCtrlClearSignal(GPIO_EN_VDISP);

    do
    {
        // This is a point of no return. Battery critically low. Execute command to shutdown battery.
        // Shutdown battery - CPU and entire HW loses power after this function call.
        if (BATTERY_STATUS_OK != Status)
        {
            Status = L3_BatteryShutdown();
            OSTimeDly(50);               //delay between retry, if needed
        }
    } while (true);
}

/******************************************************************************/
/*                                 Global Functions                           */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Activate specified mode
 *
 * \details Activates user specified power mode.
 *
 * \param   PowerMode - Desired power mode
 *
 * \return  POWER_STATUS - Status
 *
 * ========================================================================== */
POWER_STATUS Signia_PowerModeSet(POWER_MODE PowerMode)
{
    POWER_STATUS Status;
    bool         ReloadConnected;
    uint8_t *ModeName[] = { "ACTIVE", "STANDBY", "SLEEP", "SHIP" };
    static POWER_MODE CurrentMode = POWER_MODE_LAST;

    Status = POWER_STATUS_OK;

    if ((CurrentMode != PowerMode) || (POWER_MODE_SLEEP == PowerMode))
    {
        Log(TRC, "Power mode [%s] requested", ModeName[PowerMode]);
        switch (PowerMode)
        {
            case POWER_MODE_ACTIVE:
                Status = PowerModeSetActive();
                break;

            case POWER_MODE_STANDBY:
                ReloadConnected = Signia_IsReloadConnected();
                /* Reload connected do not allow standby */
                if (!ReloadConnected)
                {
                    Status = PowerModeSetStandby();
                }
                else
                {
                    Log(TRC, "Power mode [%s] not entered due to Reload Connected", ModeName[PowerMode]);
                }
                break;

            case POWER_MODE_SLEEP:
                Status = PowerModeSetSleep();
                break;

            case POWER_MODE_SHUTDOWN:
                PowerModeShutdown();
                break;

            case POWER_MODE_SHIP:
                Status = PowerModeSetShip();
                break;

            default:
                Status = POWER_STATUS_INVALID_PARAM;
                break;
        }
    }

    if (POWER_STATUS_OK != Status)
    {
        Log(TRC, "Power mode activation failed");
    }
    else
    {
        CurrentMode = PowerMode;
        PowerModeInfo.ActivePowerMode = PowerMode;
    }

    return Status;
}
/* ========================================================================== */
/**
 * \brief   Get the Active Power mode
 *
 * \param   < none >
 *
 * \return  POWER_MODE - Active Power Mode
 *
 * ========================================================================== */
POWER_MODE Signia_PowerModeGet(void)
{
    return PowerModeInfo.ActivePowerMode;
}

/* ========================================================================== */
/**
 * \brief   To Set Battery Disable Error
 *
 * \details To Set Battery Disable Error for battery charging disabled condition.
 *
 * \param   < None >
 *
 * \return  POWER_STATUS
 *
 * ========================================================================== */
POWER_STATUS Signia_BatDisableFault(void)
{
    POWER_STATUS Status;
    Status = POWER_STATUS_OK;

    Log(ERR, "Battery Charging Disabled");

    // Turn off display
    L3_DisplayOn(false);

    // turn off Piezo
    L3_GpioCtrlSetSignal(GPIO_PZT_EN);

    /// \todo NP-11/29/2021 - LED disable function to be added

    return Status;
}

/* ========================================================================== */
/**
 * \brief   To Set Battery Disable Error
 *
 * \details To Set Battery Disable Error for battery charging disabled condition.
 *
 * \param   cause - sleep cause
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_PowerModeSetSleepCause(SLEEP_CAUSE cause)
{
    PowerModeInfo.PowerMode_SleepCause = cause;
}

/**
*\}
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

