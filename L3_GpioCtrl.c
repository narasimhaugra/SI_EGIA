#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_GpioCtrl
 * \{
 * \brief   Layer 3 GPIO Controller
 *
 * \details This Module handles the Platform GPIO Signals.
 *          The functions contained in this module provide the following
 *          capabilities:
 *              - Configuring GPIO Signals.
 *              - Setting GPIO Signals.
 *              - Clearing GPIO Signals.
 *              - Toggling GPIO Signals.
 *
 * \copyright 2020 Medtronic - Surgical Innovations. All Rights Reserved.
 *
 * \file L3_GpioCtrl.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_GpioCtrl.h"
#include "L3_I2c.h"
#include "Logger.h"
#include "FaultHandler.h"
#include "TestManager.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER            (LOG_GROUP_GPIO) ///< Log Group Identifier
#define GPIO_EXP_I2C_ADDRESS            (0x21)           ///< GPIO Expander I2C Address
#define GPIO_EXP_I2C_COMM_MAX_RETRIES   (5u)             ///< GPIO Expander I2C Comm Maximum Retries.
#define GPIO_MUTEX_TIMEOUT              (200u)           ///< GPIO Mutex Timeout /// \todo 02/08/2022 SE: This is workaround for 1.1.25 and needs more investigation
#define GPIO_IOEXP_TIMEOUT              (50u)            ///< IO Expander transfer(busy wait) timeout

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                               ///  GPIO Signal Type
{
    GPIO_SIG_uP_K20,                       ///< GPIO Pin on K20 Micro
    GPIO_SIG_IO_EXP,                       ///< GPIO Pin on GPIO IO Expander
    GPIO_SIG_TYPE_LAST                     ///< GPIO Signal Type Last
} GPIO_SIG_TYPE;

typedef enum                               ///  GPIO Signal Update Type
{
    GPIO_SIG_SET,                          ///< GPIO Signal Set
    GPIO_SIG_CLEAR,                        ///< GPIO Signal Clear
    GPIO_SIG_TOGGLE,                       ///< GPIO Signal Toggle
    GPIO_SIG_UPDATE_LAST                   ///< GPIO Signal Update Last
} GPIO_SIG_UPDATE_TYPE;

typedef struct                             /// GPIO Signal Table
{
    GPIO_SIGNAL              eSignal;      ///< GPIO Signal Enum
    GPIO_SIG_TYPE            eSigType;     ///< GPIO Signal Type
    GPIO_PIN                 ePin;         ///< GPIO Signal Type
    GPIO_DIR                 ePinDir;      ///< GPIO Pin Direction
    uint8_t                  u8Port;       ///< GPIO Port Type
    GPIO_uP_PIN_INT_CONFIG_T *pxIntConfig; ///< GPIO Pin Interrupt Config [Expander Interrupts are OutofScope now]
} GPIO_TABLE_T;

typedef enum                               /// GPIO Expander Port Enum
{
    GPIO_EXP_PORT_0,                       ///< GPIO Expander Port 0
    GPIO_EXP_PORT_1,                       ///< GPIO Expander Port 1
    GPIO_EXP_PORT_LAST                     ///< GPIO Expander Port Last
} GPIO_EXP_PORT;

typedef enum                               /// GPIO Expander Registers
{
    GPIO_EXP_INPUT_PORT_REG,               ///< GPIO Expander Input Port Register
    GPIO_EXP_OUTPUT_PORT_REG,              ///< GPIO Expander Output Port Register
    GPIO_EXP_CONFIG_REG,                   ///< GPIO Expander Configuration Register
    GPIO_EXP_REG_LAST                      ///< GPIO Expander Register Last
} GPIO_EXP_REG;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
/* GPIO expander has 2 Ports P0 & P1, so registers are also in pairs */
static const uint8_t mxGPIOExpRegisterTable[GPIO_EXP_REG_LAST][GPIO_EXP_PORT_LAST] =  /// GPIO Expander Register LookUp Table
{
    { 0x00, 0x01 },                         ///<  GPIO Expander Input Port Registers for P0 & P1
    { 0x02, 0x03 },                         ///<  GPIO Expander Output Port Register for P0 & P1
    { 0x06, 0x07 }                          ///<  GPIO Expander Configuration Register for P0 & P1
};

/// \todo 07/08/2021 DAZ - Note: The ADC interrupt is used to trigger DMA. It does not issue actual interrupt,
/// \todo 07/08/2021 DAZ -       but L2_GpioConfigPin() requires a callback to initialize the pin properly -
/// \todo 07/08/2021 DAZ -       thus a dummy routine is provided here. It will never be invoked. L2_GpioConfigPin()
/// \todo 07/08/2021 DAZ -       should be changed to allow a null callback if the interrupt type is GPIO_uP_INT_DMA_xxxx.
static void AdcIntDummyCallback() { };

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static GPIO_uP_PIN_INT_CONFIG_T mxMotAdcIntCfg = { GPIO_uP_INT_DMA_RISING_EDGE, &AdcIntDummyCallback }; ///< Interrupt configuration for motor ADC trigger inputs

static GPIO_TABLE_T mxSignalTable[] =      /// GPIO Signal Table
{
    /* eSignal,                 eSigType,           ePin,           ePinDir,            u8Port,         *pxIntConfig */
    { GPIO_MOT0_ADC_TRIG,       GPIO_SIG_uP_K20,    GPIO_PIN_05,    GPIO_DIR_INPUT,     GPIO_uP_PORT_A,     &mxMotAdcIntCfg },
    { GPIO_1W_SHELL_EN,         GPIO_SIG_uP_K20,    GPIO_PIN_07,    GPIO_DIR_OUTPUT,    GPIO_uP_PORT_A,     NULL },
    { GPIO_WIFI_FORCE_AWAKE,    GPIO_SIG_uP_K20,    GPIO_PIN_11,    GPIO_DIR_OUTPUT,    GPIO_uP_PORT_A,     NULL },
    { GPIO_SLP_1Wn,             GPIO_SIG_uP_K20,    GPIO_PIN_12,    GPIO_DIR_OUTPUT,    GPIO_uP_PORT_A,     NULL },
    { GPIO_DUAL_ACCEL_INT,      GPIO_SIG_uP_K20,    GPIO_PIN_13,    GPIO_DIR_INPUT,     GPIO_uP_PORT_A,     NULL },
    { GPIO_EXTRA_IO_uC0,        GPIO_SIG_uP_K20,    GPIO_PIN_24,    GPIO_DIR_OUTPUT,    GPIO_uP_PORT_A,     NULL },
    { GPIO_1W_AD_EN,            GPIO_SIG_uP_K20,    GPIO_PIN_25,    GPIO_DIR_OUTPUT,    GPIO_uP_PORT_A,     NULL },
    { GPIO_PERIPHERAL_WUn,      GPIO_SIG_uP_K20,    GPIO_PIN_00,    GPIO_DIR_INPUT,     GPIO_uP_PORT_B,     NULL },
    { GPIO_MOT1_ADC_TRIG,       GPIO_SIG_uP_K20,    GPIO_PIN_01,    GPIO_DIR_INPUT,     GPIO_uP_PORT_B,     &mxMotAdcIntCfg },
    { GPIO_WIFI_ENn,            GPIO_SIG_uP_K20,    GPIO_PIN_04,    GPIO_DIR_OUTPUT,    GPIO_uP_PORT_B,     NULL },
    { GPIO_WIFI_RESETn,         GPIO_SIG_uP_K20,    GPIO_PIN_05,    GPIO_DIR_OUTPUT,    GPIO_uP_PORT_B,     NULL },
    { GPIO_EN_BATT_15V,         GPIO_SIG_uP_K20,    GPIO_PIN_06,    GPIO_DIR_OUTPUT,    GPIO_uP_PORT_B,     NULL },
    { GPIO_GN_LED,              GPIO_SIG_uP_K20,    GPIO_PIN_08,    GPIO_DIR_OUTPUT,    GPIO_uP_PORT_B,     NULL },
    { GPIO_GN_KEY1n,            GPIO_SIG_uP_K20,    GPIO_PIN_09,    GPIO_DIR_INPUT,     GPIO_uP_PORT_B,     NULL },
    { GPIO_GN_KEY2n,            GPIO_SIG_uP_K20,    GPIO_PIN_10,    GPIO_DIR_INPUT,     GPIO_uP_PORT_B,     NULL },
    { GPIO_EXTRA_IO_uC3,        GPIO_SIG_uP_K20,    GPIO_PIN_11,    GPIO_DIR_INPUT,     GPIO_uP_PORT_B,     NULL },
    { GPIO_KEY_WAKEn,           GPIO_SIG_uP_K20,    GPIO_PIN_03,    GPIO_DIR_INPUT,     GPIO_uP_PORT_C,     NULL },
    { GPIO_OPEN_KEYn,           GPIO_SIG_uP_K20,    GPIO_PIN_12,    GPIO_DIR_INPUT,     GPIO_uP_PORT_C,     NULL },
    { GPIO_LEFT_CW_KEYn,        GPIO_SIG_uP_K20,    GPIO_PIN_13,    GPIO_DIR_INPUT,     GPIO_uP_PORT_C,     NULL },
    { GPIO_LEFT_CCW_KEYn,       GPIO_SIG_uP_K20,    GPIO_PIN_18,    GPIO_DIR_INPUT,     GPIO_uP_PORT_C,     NULL },
    { GPIO_RIGHT_CW_KEYn,       GPIO_SIG_uP_K20,    GPIO_PIN_19,    GPIO_DIR_INPUT,     GPIO_uP_PORT_C,     NULL },
    { GPIO_GPIO_INTn,           GPIO_SIG_uP_K20,    GPIO_PIN_07,    GPIO_DIR_INPUT,     GPIO_uP_PORT_D,     NULL },
    { GPIO_IM_GOOD,             GPIO_SIG_uP_K20,    GPIO_PIN_08,    GPIO_DIR_OUTPUT,    GPIO_uP_PORT_D,     NULL },
    { GPIO_RIGHT_CCW_KEYn,      GPIO_SIG_uP_K20,    GPIO_PIN_09,    GPIO_DIR_INPUT,     GPIO_uP_PORT_D,     NULL },
    { GPIO_LEFT_ARTIC_KEYn,     GPIO_SIG_uP_K20,    GPIO_PIN_11,    GPIO_DIR_INPUT,     GPIO_uP_PORT_D,     NULL },
    { GPIO_RIGHT_ARTIC_KEYn,    GPIO_SIG_uP_K20,    GPIO_PIN_12,    GPIO_DIR_INPUT,     GPIO_uP_PORT_D,     NULL },
    { GPIO_CLOSE_KEYn,          GPIO_SIG_uP_K20,    GPIO_PIN_13,    GPIO_DIR_INPUT,     GPIO_uP_PORT_D,     NULL },
    { GPIO_EXTRA_IO_uC2,        GPIO_SIG_uP_K20,    GPIO_PIN_06,    GPIO_DIR_INPUT,     GPIO_uP_PORT_E,     NULL },
    { GPIO_MOT2_ADC_TRIG,       GPIO_SIG_uP_K20,    GPIO_PIN_07,    GPIO_DIR_INPUT,     GPIO_uP_PORT_E,     &mxMotAdcIntCfg },
    { GPIO_EXTRA_IO_uC1,        GPIO_SIG_uP_K20,    GPIO_PIN_10,    GPIO_DIR_OUTPUT,    GPIO_uP_PORT_E,     NULL },
    { GPIO_SDHC0_LED,           GPIO_SIG_uP_K20,    GPIO_PIN_12,    GPIO_DIR_OUTPUT,    GPIO_uP_PORT_E,     NULL },

    { GPIO_EN_5V,               GPIO_SIG_IO_EXP,    GPIO_PIN_00,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_0,    NULL },
    { GPIO_LCD_RESET,           GPIO_SIG_IO_EXP,    GPIO_PIN_01,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_0,    NULL },
    { GPIO_EXP_IO0,             GPIO_SIG_IO_EXP,    GPIO_PIN_02,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_0,    NULL },
    { GPIO_EXP_IO1,             GPIO_SIG_IO_EXP,    GPIO_PIN_03,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_0,    NULL },
    { GPIO_EXP_IO2,             GPIO_SIG_IO_EXP,    GPIO_PIN_04,    GPIO_DIR_INPUT,     GPIO_EXP_PORT_0,    NULL },
    { GPIO_1W_BATT_ENABLE,      GPIO_SIG_IO_EXP,    GPIO_PIN_05,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_0,    NULL },
    { GPIO_1W_EXP_ENABLE,       GPIO_SIG_IO_EXP,    GPIO_PIN_06,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_0,    NULL },
    { GPIO_EN_3V,               GPIO_SIG_IO_EXP,    GPIO_PIN_07,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_0,    NULL },

    { GPIO_EN_2P5V,             GPIO_SIG_IO_EXP,    GPIO_PIN_00,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_1,    NULL },
    { GPIO_EN_VDISP,            GPIO_SIG_IO_EXP,    GPIO_PIN_01,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_1,    NULL },
    { GPIO_PZT_EN,              GPIO_SIG_IO_EXP,    GPIO_PIN_02,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_1,    NULL },
    { GPIO_FPGA_READY,          GPIO_SIG_IO_EXP,    GPIO_PIN_03,    GPIO_DIR_INPUT,     GPIO_EXP_PORT_1,    NULL },
    { GPIO_FPGA_SLEEP,          GPIO_SIG_IO_EXP,    GPIO_PIN_04,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_1,    NULL },
    { GPIO_EN_SMB,              GPIO_SIG_IO_EXP,    GPIO_PIN_05,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_1,    NULL },
    { GPIO_FPGA_SPI_RESET,      GPIO_SIG_IO_EXP,    GPIO_PIN_06,    GPIO_DIR_OUTPUT,    GPIO_EXP_PORT_1,    NULL },
    { GPIO_FPGA_SAFETY,         GPIO_SIG_IO_EXP,    GPIO_PIN_07,    GPIO_DIR_INPUT,     GPIO_EXP_PORT_1,    NULL }
};

static OS_EVENT * mhGPIOMutex;      ///< GPIO Mutex
static bool mbGPIOCtrlInitialized;  ///< GPIO Initialized Status Variable

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static GPIO_STATUS GpioExpI2cConfigure(void);
static GPIO_STATUS GpioExpI2cRegisterWrite(uint8_t u8RegIn, uint8_t u8RegValueIn);
static GPIO_STATUS GpioExpI2cRegisterRead(uint8_t u8RegIn, uint8_t *pu8RegValueIn);

static GPIO_STATUS GpioUpdateK20Signal(GPIO_SIGNAL eSignal, GPIO_SIG_UPDATE_TYPE eSigUpdate);
static GPIO_STATUS GpioUpdateIOExpSignal(GPIO_SIGNAL eSignal, GPIO_SIG_UPDATE_TYPE eSigUpdate);
static GPIO_STATUS GpioCtrlUpdateSignal(GPIO_SIGNAL eSignal, GPIO_SIG_UPDATE_TYPE eSigUpdate);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Configure the I2C Channel to communicate with GPIO Expander
 *
 * \details This function creates an I2C control packet and configures
 *          the I2C channel for GPIO Expander communication.
 *
 * \todo    02/24/2021 GK - I2cResetModule() re-configures the PORTx_PCRn,
 *          re-visit the need of PORT_PCR_ODE_MASK for GPIO expander. Legacy
 *          software re-configures Open drain inside I2C_SetClk()
 *
 * \todo    02/24/2021 GK - PCAL6416A datasheet [GPIO expander] page 37 says
 *          the max fSCL in Standard-mode is 100 kHz and Fast-mode 400 kHz.
 *          There are many choices of I2C0_F divider, and we could have made
 *          a value that is a lot closer to 100Khz. The reason for selecting
 *          144 kHz [As per Legacy Software] is unknown at this moment. Also,
 *          how to set for Fast Mode is not clear from the GPIO expander
 *          datasheet. For the time being, the clock is set to 78K [a pre-
 *          defined value], and the code is working. Re-visit this once we
 *          have more clarity on 144 kHz or how to set the Fast-mode.
 *
 * \param   < None >
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
static GPIO_STATUS GpioExpI2cConfigure(void)
{
    I2CControl xI2CControl;     /* I2C Control Packet */
    GPIO_STATUS eStatusReturn;  /* GPIO Status Return */

     /* Set the Return status to Error */
    eStatusReturn = GPIO_STATUS_ERROR;

    /* Configure the GPIO Expander I2C Control settings. */
    xI2CControl.Device = GPIO_EXP_I2C_ADDRESS;
    xI2CControl.AddrMode = I2C_ADDR_MODE_7BIT;  /* 7 bit device addressing mode */
    xI2CControl.Clock = I2C_CLOCK_312K;   /// \todo 04/13/2021 DAZ - Was 78kC. Try at full speed       /* Clock 78 KHz                 */
    xI2CControl.State = I2C_STATE_ENA;          /* I2C Enabled                  */
    xI2CControl.Timeout = GPIO_IOEXP_TIMEOUT;   /* Busy wait timeout            */

    /* Configure the I2C Channel */
    if (I2C_STATUS_SUCCESS == L3_I2cConfig(&xI2CControl))
    {
        /* Set the Return status as OK */
        eStatusReturn = GPIO_STATUS_OK;
    }
    /* else not needed */

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   GPIO Expander I2C Interface Register Write
 *
 * \details This function does an I2C write to the GPIO expander
 *
 * \note    Assumes L3_I2C_Write() funciton provides I2C channel
 *          configuration and retries.
 *
 * \param   u8RegIn - GPIO Expander Register Pair Number
 * \param   u8RegValueIn - Register Value to Write
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
static GPIO_STATUS GpioExpI2cRegisterWrite(uint8_t u8RegIn, uint8_t u8RegValueIn)
{
    I2CDataPacket   xDataPacket;    /* I2C Data Packet    */
    GPIO_STATUS     eStatusReturn;  /* GPIO Status Return */
    uint8_t         u8RetryCount;   /* Retry Count        */

    /* Set the retry count to max */
    u8RetryCount = GPIO_EXP_I2C_COMM_MAX_RETRIES;

    /* Set the Return status to Error */
    eStatusReturn = GPIO_STATUS_ERROR;

   /* Config success, now create the Data Packet */
    xDataPacket.Address = GPIO_EXP_I2C_ADDRESS;
    xDataPacket.pReg = &u8RegIn;
    xDataPacket.nRegSize = 1;
    xDataPacket.pData = &u8RegValueIn;
    xDataPacket.nDataSize = 1;

    /* GPIO expander, retry approach */
    while (--u8RetryCount)
    {
        /* Allow some delay between consecutive access. */
        OSTimeDly(MSEC_2);

        /* Write the Data Packet. */
        if (I2C_STATUS_SUCCESS == L3_I2cWrite(&xDataPacket))
        {
            /* Set the Return status as OK */
            eStatusReturn = GPIO_STATUS_OK;

            /* Valid I2C Write, break */
            break;
        }
    }

    /* GPIO Error */
    if ( GPIO_STATUS_ERROR == eStatusReturn )
    {
        FaultHandlerSetFault(REQRST_GPIOEXP_COMMFAIL, SET_ERROR );
    }

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   GPIO Expander I2C Interface Register Read
 *
 * \details This function does an I2C Read to the GPIO expander
 *
 * \note    Assumes L3_I2C_Read() funciton provides I2C channel
 *          configuration and retries.
 *
 * \param   u8RegIn - GPIO Expander Register Pair Number
 * \param   pu8RegValueIn - pointer to Register Value to Read
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_INVALID_INPUT - For Invalid Inputs
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
static GPIO_STATUS GpioExpI2cRegisterRead(uint8_t u8RegIn, uint8_t *pu8RegValueIn)
{
    I2CDataPacket   xDataPacket;                /* I2C Data Packet     */
    GPIO_STATUS     eStatusReturn;              /* GPIO Status Return  */
    uint8_t         pu8RegValueRead[2] = {0};   /* Read Buffer         */
    uint8_t         u8RetryCount;               /* Retry Count         */

    /* Set the retry count to max */
    u8RetryCount = GPIO_EXP_I2C_COMM_MAX_RETRIES;

    /* Input Valid? */
    if (NULL == pu8RegValueIn)
    {
        /* Set the Return status as INVALID_INPUT */
        eStatusReturn = GPIO_STATUS_INVALID_INPUT;
    }
    else
    {   /* Set the Return status to Error */
        eStatusReturn = GPIO_STATUS_ERROR;

        /* Create Data Packet */
        xDataPacket.Address = GPIO_EXP_I2C_ADDRESS;
        xDataPacket.pReg = &u8RegIn;
        xDataPacket.nRegSize = 1;
        xDataPacket.pData = pu8RegValueRead;
        xDataPacket.nDataSize = 1;

        /* GPIO expander, retry approach */
        while (--u8RetryCount)
        {
            /* Allow some delay between consecutive access. */
            OSTimeDly(MSEC_2);

            /* Read the Data Packet. */
            if (I2C_STATUS_SUCCESS == L3_I2cRead(&xDataPacket))
            {
                /* Return the Data Read. */
                *pu8RegValueIn = pu8RegValueRead[0];

                /* Set the Return status as OK */
                eStatusReturn = GPIO_STATUS_OK;

                /* Valid I2C Read, break */
                break;
            }
        }
    }

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   Updates the specified GPIO K20 Signal [Set/Clear/Toggle]
 *
 * \details This function updates the K20 GPIO Signal based on the update type
 *          requested [Set/Clear/Toggle], it uses corresponding L2 Gpio calls.
 *
 * \param   eSignal - GPIO K20 Signal to Update.
 * \param   eSigUpdate - Update type needed [Set/Clear/Toggle].
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_NOT_CONFIGURED - If K20 Pins are Not Configured.
 * \retval      GPIO_STATUS_INVALID_INPUT - For Invalid Inputs
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
static GPIO_STATUS GpioUpdateK20Signal(GPIO_SIGNAL eSignal, GPIO_SIG_UPDATE_TYPE eSigUpdate)
{
    GPIO_STATUS  eStatusReturn;  /* GPIO Status Return */
    GPIO_PIN     ePortPin;       /* GPIO Signal Port Pin */
    GPIO_uP_PORT ePort;          /* GPIO uP Port */

    /* Private function, Inputs are already validated by the caller. */

    /* K20 Signal, Get the parameters from signal table */
    ePort = (GPIO_uP_PORT)mxSignalTable[eSignal].u8Port;
    ePortPin = mxSignalTable[eSignal].ePin;

    /* It is a K20 Micro(uP) Pin, call L2 Gpio Apis */
    switch(eSigUpdate)
    {
      case GPIO_SIG_SET:
        eStatusReturn = L2_GpioSetPin(ePort, ePortPin);
        break;

      case GPIO_SIG_CLEAR:
        eStatusReturn = L2_GpioClearPin(ePort, ePortPin);
        break;

      case GPIO_SIG_TOGGLE:
        eStatusReturn = L2_GpioTogglePin(ePort, ePortPin);
        break;

      default:
        /* Set the Return status to Error */
        eStatusReturn = GPIO_STATUS_ERROR;
        break;
    }

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   Updates the specified GPIO Expander Signal [Set/Clear/Toggle]
 *
 * \details This function update the GPIO Expander Signal based on the update
 *          type requested [Set/Clear/Toggle]. It performs the following
 *              - Read the current values of output register [P0 or P1]
 *              - Perform the requested update [Set/Clear/Toggle]
 *              - Write the updated value to output register
 *              - Read it back and compare it with written value.
 *
 * \param   eSignal - GPIO signal to Update.
 * \param   eSigUpdate - Update type needed [Set/Clear/Toggle].
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
static GPIO_STATUS GpioUpdateIOExpSignal(GPIO_SIGNAL eSignal, GPIO_SIG_UPDATE_TYPE eSigUpdate)
{
    GPIO_STATUS eStatusReturn;  /* GPIO Status Return */
    GPIO_PIN ePortPin;          /* GPIO Signal Port Pin */
    GPIO_EXP_REG eExpRegister;  /* GPIO expander Register value */
    uint8_t u8Port;             /* Port */
    uint8_t u8WriteValue;       /* Write value to expander Register */
    uint8_t u8ReadValue;        /* Read value from expander Register */
    uint8_t u8OSError;          /* OS error variable  */
    uint8_t u8PinMaskValue;     /* Pin Mask Value. */

    u8WriteValue = 0u; // Initialize variable

    /* Private function, Inputs are already validated by the caller. */

    /* Set the Return status to Error */
    eStatusReturn = GPIO_STATUS_ERROR;

    /* GPIO Expander Signal, Set a Mutex entry. */
    OSMutexPend(mhGPIOMutex, GPIO_MUTEX_TIMEOUT, &u8OSError);

    do
    {   /* Any Errors? */
        if (OS_ERR_NONE != u8OSError)
        {
            /* Error, Exit */
            break;
        }

        /* Get Expander parameters from signal table */
        u8ReadValue = 0u;
        u8Port = mxSignalTable[eSignal].u8Port;
        ePortPin = mxSignalTable[eSignal].ePin;
        eExpRegister = (GPIO_EXP_REG)mxGPIOExpRegisterTable[GPIO_EXP_OUTPUT_PORT_REG][u8Port];
        u8PinMaskValue = (uint8_t)GPIO_MASK_PIN(ePortPin);

        /* Read the current Expander register value */
        if (GPIO_STATUS_OK != GpioExpI2cRegisterRead(eExpRegister, &u8ReadValue))
        {
            /// \todo 11/03/2021 This API is called from Startup, Runtime. Need to Publish GPIO Communication Failure Event.
            break;
        }

        switch (eSigUpdate)
        {
          case GPIO_SIG_SET:
            /* Create write value, Read value | Set the required bit using Pin Mask */
            u8WriteValue = (u8ReadValue | u8PinMaskValue);
            break;

          case GPIO_SIG_CLEAR:
            /* Update the value, Read value & Clear the required bit using Pin Mask */
            u8WriteValue = u8ReadValue & ~u8PinMaskValue;
            break;

          case GPIO_SIG_TOGGLE:
            /* Check the bit is set or not? */
            if (u8ReadValue & u8PinMaskValue)
            {
                /* Bit is already set, Clear it */
                u8WriteValue = u8ReadValue & ~u8PinMaskValue;
            }
            else
            {
                /* Bit is already Clear, Set it */
                u8WriteValue = (u8ReadValue | u8PinMaskValue);
            }
            break;

          default:
            /* Do nothing, eStatusReturn is already set to GPIO_STATUS_ERROR. */
            break;
        }

        /* Defensive method, Write the new value to Expander Register */
        if (GPIO_STATUS_OK != GpioExpI2cRegisterWrite(eExpRegister, u8WriteValue))
        {
            break;
        }

        /* Write Success, to confirm let us read it back and compare */
        if (GPIO_STATUS_OK != GpioExpI2cRegisterRead(eExpRegister, &u8ReadValue))
        {
            break;
        }

        /* Alright, Read is also success, how about value? */
        if (u8WriteValue == u8ReadValue)
        {
            /* Value good, Set the Return status as OK */
            eStatusReturn = GPIO_STATUS_OK;
        }

    } while (false);

    /* If No Error */
    if (OS_ERR_NONE == u8OSError)
    {
        /* Done with the Signal, Post the GPIO Mutex */
        (void) OSMutexPost(mhGPIOMutex);
    }

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   Updates the specified GPIO Signal [Set/Clear/Toggle]
 *
 * \details This function Update the GPIO Signal based on the update type
 *          requested [Set/Clear/Toggle]
 *
 * \param   eSignal - GPIO signal to Update.
 * \param   eSigUpdate - Update type needed [Set/Clear/Toggle].
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_NOT_INIT - If GPIO Crtl is not initialized
 * \retval      GPIO_STATUS_NOT_CONFIGURED - If K20 Pins are Not Configured.
 * \retval      GPIO_STATUS_INVALID_INPUT - For Invalid Inputs
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
static GPIO_STATUS GpioCtrlUpdateSignal(GPIO_SIGNAL eSignal, GPIO_SIG_UPDATE_TYPE eSigUpdate)
{
    GPIO_STATUS eStatusReturn;  /* GPIO Status Return */

    do
    {   /* Is the GPIO controller initialized ? */
        if (!mbGPIOCtrlInitialized)
        {
            /* Set Return Status as Not Initialized */
            eStatusReturn = GPIO_STATUS_NOT_INIT;
            Log(DBG, "GpioCtrlUpdateSignal: Error! Sig = %d, UpdateType = %d, Status = %d", eSignal, eSigUpdate, eStatusReturn);
            break;
        }
        /* else not needed */

        /* Is Input Valid - Jira fix #695 */
        if ((GPIO_SIGNAL_LAST <= eSignal) ||
            (GPIO_SIG_UPDATE_LAST <= eSigUpdate) ||
            (GPIO_DIR_OUTPUT != mxSignalTable[eSignal].ePinDir))
        {
            /* Set Return Status as Invalid Input */
            eStatusReturn = GPIO_STATUS_INVALID_INPUT;
            Log(DBG, "GpioCtrlUpdateSignal: Error! Sig = %d, UpdateType = %d, Status = %d", eSignal, eSigUpdate, eStatusReturn);
            break;
        }
        /* else not needed */

        /* What type of signal we are handling? */
        if (GPIO_SIG_uP_K20 == mxSignalTable[eSignal].eSigType)
        {
            /* Set the Return status to Error */
            eStatusReturn = GpioUpdateK20Signal(eSignal, eSigUpdate);
        }
        else
        {
            /* Set the Return status to Error */
            eStatusReturn = GpioUpdateIOExpSignal(eSignal, eSigUpdate);
        }

    } while (false);

    /* Return the status */
    return eStatusReturn;
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Initializes the Signia Layer 3 GPIO Controller.
 *
 * \details This function configures the Signia System GPIO Signals
 *          as per the configuration table
 *          - Creates Mutex for Support simultaneous Signal Access
 *          - Configures the K20 Port Pins Direction & Default values
 *          - Configures the GPIO expander Port Pins Direction & Default values
 *
 * \param   < None >
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
GPIO_STATUS L3_GpioCtrlInit(void)
{
    uint8_t u8Signal = 0;                                   /* Signal loop Count  */
    uint8_t u8OSError = 0;                                  /* OS error variable */
    uint8_t u8ExpRegConfigValue[GPIO_EXP_PORT_LAST] = {0};  /* GPIO Exp Input Pin Value */
    GPIO_STATUS eStatusReturn;                              /* GPIO Status Return  */
    GPIO_EXP_REG eExpRegister;                              /* GPIO expander Register value */
    uint8_t u8WriteValue;                                   /* Write value to expander Register */
    uint8_t u8ReadValue;                                    /* Read value from expander Register */
    bool ErrorHandle;

    ErrorHandle = false;

    do
    {   /* Prevent multiple inits */
        if (mbGPIOCtrlInitialized)
        {
            /* Already Initialized. */
            eStatusReturn = GPIO_STATUS_OK;
            break;
        }

        /* Configure GPIO Expander */
        eStatusReturn = GpioExpI2cConfigure();

        /* Config Success? */
        if (GPIO_STATUS_OK != eStatusReturn)
        {
            /* No */
            ErrorHandle = true;
            break;
        }

        /* Create a Mutex to protect parallel GPIO calls. */
        mhGPIOMutex = SigMutexCreate("GPIO Exp", &u8OSError);

        /* Are we able to create the Mutex? */
        if (NULL == mhGPIOMutex)
        {
            /* Set the Return status to Error */
            eStatusReturn = GPIO_STATUS_ERROR;
            Log(ERR, "L3_GpioCtrlInit: OSMutexCreate Error!");
            break;
        }

        /* Iterate over the Signal Table and do the pin configuration. */
        for(u8Signal = 0; u8Signal < GPIO_SIGNAL_LAST; u8Signal++)
        {
            /* What type of signal we are handling? */
            if (GPIO_SIG_uP_K20 == mxSignalTable[u8Signal].eSigType)
            {
                /* It is a K20 Micro(uP) Pin, call L2 call to config the K20
                Pins as per the Signal Table */
                L2_GpioConfigPin((GPIO_uP_PORT)(mxSignalTable[u8Signal].u8Port),
                                     mxSignalTable[u8Signal].ePin,
                                     mxSignalTable[u8Signal].ePinDir,
                                     mxSignalTable[u8Signal].pxIntConfig);
            }
            else
            {
                /* It is a GPIO Expander Pin  */
                if (GPIO_DIR_INPUT == mxSignalTable[u8Signal].ePinDir)
                {
                    /* The Configuration registers (registers 6 and 7) configure
                    the direction of the I/O pins. If a bit in these registers is
                    set to 1, the corresponding port pin is enabled as a
                    high-impedance input. If a bit in these registers is cleared
                    to 0, the corresponding port pin is enabled as an output. */

                    /* Get all Input pin masks */
                    u8ExpRegConfigValue[mxSignalTable[u8Signal].u8Port] |=
                        (uint8_t)GPIO_MASK_PIN(mxSignalTable[u8Signal].ePin);
                }
                /* else not needed */
            }
        }

        eStatusReturn = GPIO_STATUS_ERROR; /* Let's start with default error */

        /* K20 GPIO pins are Done, Configure GPIO Expander Pins Next */

        /* Get Expander GPIO Expander Config Register 0 parameters from signal table */
        eExpRegister = (GPIO_EXP_REG)mxGPIOExpRegisterTable[GPIO_EXP_CONFIG_REG][GPIO_EXP_PORT_0];
        u8WriteValue = u8ExpRegConfigValue[GPIO_EXP_PORT_0];

        /* Config the GPIO Expander Config Register 0 based on the Port 0 Bit Masks, clear for Output */

        if (GPIO_STATUS_OK != GpioExpI2cRegisterWrite(eExpRegister, u8WriteValue))
        {
            ErrorHandle = true;
            Log(ERR, "L3_GpioCtrlInit: Error! GpioExpI2cRegisterWrite(), Config Reg, P0");
            break;
        }


        /* Write Success, to confirm let us read it back and compare */
        if (GPIO_STATUS_OK != GpioExpI2cRegisterRead(eExpRegister, &u8ReadValue))
        {
            ErrorHandle = true;
            Log(ERR, "L3_GpioCtrlInit: Error! GpioExpI2cRegisterRead(), Config Reg, P0");
            break;
        }

        /* Alright, Read is also success, how about value? */
        if (u8WriteValue != u8ReadValue)
        {
            Log(ERR, "L3_GpioCtrlInit: Error! u8ReadValue not matching, Config Reg, P0");
            break;
        }

        /* Get Expander GPIO Expander Config Register 1 parameters from signal table */
        eExpRegister = (GPIO_EXP_REG)mxGPIOExpRegisterTable[GPIO_EXP_CONFIG_REG][GPIO_EXP_PORT_1];
        u8WriteValue = u8ExpRegConfigValue[GPIO_EXP_PORT_1];

        /* K20 GPIO pins are Done, Configure GPIO Expander Pins Next */
        /* Config the GPIO Expander Config Register 0 based on the Port 0 Bit Masks, clear for Output */
        if (GPIO_STATUS_OK != GpioExpI2cRegisterWrite(eExpRegister, u8WriteValue))
        {
            ErrorHandle = true;
            Log(ERR, "L3_GpioCtrlInit: Error! GpioExpI2cRegisterWrite(), Config Reg, P1");
            break;
        }

        /* Write Success, to confirm let us read it back and compare */
        if (GPIO_STATUS_OK != GpioExpI2cRegisterRead(eExpRegister, &u8ReadValue))
        {
            ErrorHandle = true;
            Log(ERR, "L3_GpioCtrlInit: Error! GpioExpI2cRegisterRead(), Config Reg, P1");
            break;
        }

        /* Alright, Read is also success, how about value? */
        if (u8WriteValue != u8ReadValue)
        {
            Log(ERR, "L3_GpioCtrlInit: Error! u8ReadValue not matching, Config Reg, P1");
            break;
        }

        /* Set the Return status as OK */
        eStatusReturn = GPIO_STATUS_OK;


        Log(DBG, "L3_GpioCtrl: Initialized");

        /* Yes, All Good, now Set the global variable. */
        mbGPIOCtrlInitialized = true;

    } while (false);

    if ( ErrorHandle )
    {
        /* GPIO Expander Communication Failure */
        FaultHandlerSetFault(REQRST_GPIOEXP_COMMFAIL, SET_ERROR);
    }

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   Sets the specified GPIO Signal
 *
 * \details This function sets the GPIO Signal.
 *
 * \param   eSignal - GPIO signal to Set.
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_NOT_INIT - If GPIO Crtl is not initialized
 * \retval      GPIO_STATUS_INVALID_INPUT - For Invalid Inputs
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
GPIO_STATUS L3_GpioCtrlSetSignal(GPIO_SIGNAL eSignal)
{
    /* Call GpioCtrlUpdateSignal() with GPIO_SIG_SET */
    return GpioCtrlUpdateSignal(eSignal, GPIO_SIG_SET);
}

/* ========================================================================== */
/**
 * \brief   Clears the specified GPIO Signal
 *
 * \details This function clears the GPIO Signal.
 *
 * \param   eSignal - GPIO signal to clear.
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_NOT_INIT - If GPIO Crtl is not initialized
 * \retval      GPIO_STATUS_INVALID_INPUT - For Invalid Inputs
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
GPIO_STATUS L3_GpioCtrlClearSignal(GPIO_SIGNAL eSignal)
{
    /* Call GpioCtrlUpdateSignal() with GPIO_SIG_CLEAR  */
    return GpioCtrlUpdateSignal(eSignal, GPIO_SIG_CLEAR);
}

/* ========================================================================== */
/**
 * \brief   Toggles the specified GPIO Signal
 *
 * \details This function toggles the GPIO Signal.
 *
 * \param   eSignal - GPIO signal to toggle.
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_NOT_INIT - If GPIO Crtl is not initialized
 * \retval      GPIO_STATUS_INVALID_INPUT - For Invalid Inputs
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
GPIO_STATUS L3_GpioCtrlToggleSignal(GPIO_SIGNAL eSignal)
{
    /* Call GpioCtrlUpdateSignal() with GPIO_SIG_TOGGLE */
    return GpioCtrlUpdateSignal(eSignal, GPIO_SIG_TOGGLE);
}

/* ========================================================================== */
/**
 * \brief   Reads the state of a specified GPIO Signal
 *
 * \details This function reads the state of GPIO Signal.
 *
 * \param   eSignal - GPIO signal to read.
 * \param   pbGetValue - pointer to Boolean Value to Get.
 *              - true, If the Signal logic level is logic 1
 *              - false, If the Signal logic level is logic 0
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_NOT_INIT - If GPIO Crtl is not initialized
 * \retval      GPIO_STATUS_INVALID_INPUT - For Invalid Inputs
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
GPIO_STATUS L3_GpioCtrlGetSignal(GPIO_SIGNAL eSignal, bool *pbGetValue)
{
    GPIO_STATUS eStatusReturn = GPIO_STATUS_ERROR;  /* GPIO Status Return */
    GPIO_EXP_PORT eExpPort;                         /* GPIO expander Port */
    GPIO_EXP_REG  eExpRegister;                     /* GPIO expander Register value */
    uint8_t u8ReadValue = 0;                        /* Read value from expander Register */
    uint8_t u8OSError = 0;                          /* OS error variable  */

    GPIO_TM_DATA TmData;

    do
    {   /* Is the GPIO controller initialized ? */
        if (!mbGPIOCtrlInitialized)
        {
            /* Set Return Status as Not Initialized */
            eStatusReturn = GPIO_STATUS_NOT_INIT;
            Log(DBG, "L3_GpioCtrlGetSignal: Error! Sig = %d, Status = %d", eSignal, eStatusReturn);
            break;
        }
        /* else not needed */

        /* Is Input Valid ? */
        if ((NULL == pbGetValue) || (GPIO_SIGNAL_LAST <= eSignal))
        {
            /* Set Return Status as Invalid Input */
            eStatusReturn = GPIO_STATUS_INVALID_INPUT;
            Log(DBG, "L3_GpioCtrlGetSignal: Error! Sig = %d, Status = %d", eSignal, eStatusReturn);
            break;
        }
        /* else not needed */

        /* Set the Return status to Error */
        eStatusReturn = GPIO_STATUS_ERROR;

        /* What type of signal we are handling? */
        if (GPIO_SIG_uP_K20 == mxSignalTable[eSignal].eSigType)
        {
            /* It is a K20 Micro(uP) Pin, call L2 to Get the pin Value */
            eStatusReturn = L2_GpioGetPin((GPIO_uP_PORT)(mxSignalTable[eSignal].u8Port),
                                                    mxSignalTable[eSignal].ePin,
                                                    pbGetValue);
        }
        else
        {
             /* GPIO Expander Signal, Set a Mutex entry. */
            OSMutexPend(mhGPIOMutex, OS_WAIT_FOREVER, &u8OSError);

            /* It is an Expander IO, Get the Expander Port [0 or 1] */
            eExpPort = (GPIO_EXP_PORT)(mxSignalTable[eSignal].u8Port);

            /* Get the Exp Input Port Register Pair Details [First or Second] */
            eExpRegister = (GPIO_EXP_REG)mxGPIOExpRegisterTable[GPIO_EXP_INPUT_PORT_REG][eExpPort];

            /* Read the GPIO Exp Input Port Register */
            if (GPIO_STATUS_OK == GpioExpI2cRegisterRead(eExpRegister, &u8ReadValue))
            {
                /* Got the Port Values. Find the Pin State using Pin Mask */
                *pbGetValue = ((u8ReadValue &
                                (uint8_t)GPIO_MASK_PIN(mxSignalTable[eSignal].ePin))? true : false);

                /* Set the Return status as OK */
                eStatusReturn = GPIO_STATUS_OK;
            }
            else
            {
                /* GPIO Expander Communication Failure */
                FaultHandlerSetFault(REQRST_GPIOEXP_COMMFAIL, SET_ERROR);
            }

            /* Done with the Signal, Post the GPIO Mutex */
            OSMutexPost(mhGPIOMutex);
        }

    } while (false);

    /* structure for Test HOOK with signal and pointer to signal value as the attributes*/
    TmData.Signal = eSignal;
    TmData.pValue  = pbGetValue;
    TM_Hook( HOOK_GPIOSIGNAL, &TmData );

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   Enables the GPIO Signal Interrupt Call Backs.
 *
 * \details This function enables the GPIO Signal Interrupt Call Backs.
 *          CURRENTLY IT IS AVAILABLE FOR K20 PINS ONLY.
 *
 * \param   eSignal - GPIO signal to Enable.
 * \param   pxIntConfigIn - pointer to Gpio Pin Interrupt Configuration.
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_NOT_INIT - If GPIO Crtl is not initialized
 * \retval      GPIO_STATUS_INVALID_INPUT - For Invalid Inputs
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
GPIO_STATUS L3_GpioCtrlEnableCallBack(GPIO_SIGNAL eSignal, GPIO_uP_PIN_INT_CONFIG_T *pxIntConfigIn)
{
    GPIO_STATUS eStatusReturn;  /* GPIO Status Return */

    do
    {   /* Is the GPIO controller initialized ? */
        if (!mbGPIOCtrlInitialized)
        {
            /* Set Return Status as Not Initialized */
            eStatusReturn = GPIO_STATUS_NOT_INIT;
            break;
        }

        /* Only K20 Signals, Expander Interrupts are Out-of-Scope now*/
        if ((GPIO_SIGNAL_LAST <= eSignal) ||
            (GPIO_SIG_uP_K20 != mxSignalTable[eSignal].eSigType))
        {
            eStatusReturn = GPIO_STATUS_INVALID_INPUT;
            break;
        }

        /* Is there an Interrupt Config Available? */
        if(NULL == pxIntConfigIn)
        {
            eStatusReturn = GPIO_STATUS_INVALID_INPUT;
            break;
        }

        /* Is there an Interrupt callback registered for this GPIO? */
        if((NULL == pxIntConfigIn->pxInterruptCallback) &&
           (GPIO_uP_INT_DISABLED == pxIntConfigIn->eInterruptType))
        {
            eStatusReturn = GPIO_STATUS_INVALID_INPUT;
            break;
        }

        /* All good so far, Call L2 to re-configure the GPIO Pin with Call Back */
        eStatusReturn = L2_GpioConfigPin((GPIO_uP_PORT)(mxSignalTable[eSignal].u8Port),
                             mxSignalTable[eSignal].ePin,
                             mxSignalTable[eSignal].ePinDir,
                             pxIntConfigIn);

        if(GPIO_STATUS_OK == eStatusReturn)
        {
            mxSignalTable[eSignal].pxIntConfig = pxIntConfigIn;
        }

    } while (false);

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   Disables the GPIO Signal Interrupt Call Backs.
 *
 * \details This function Disables the GPIO Signal Interrupt Call Backs.
 *
 * \param   eSignal - GPIO signal to Disable.
 *
 * \return  GPIO_STATUS - GPIO Status
 * \retval      GPIO_STATUS_ERROR - If Error
 * \retval      GPIO_STATUS_NOT_INIT - If GPIO Crtl is not initialized
 * \retval      GPIO_STATUS_INVALID_INPUT - For Invalid Inputs
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
GPIO_STATUS L3_GpioCtrlDisableCallBack(GPIO_SIGNAL eSignal)
{
    GPIO_STATUS eStatusReturn;  /* GPIO Status Return */

    do
    {   /* Is the GPIO controller initialized ? */
        if (!mbGPIOCtrlInitialized)
        {
            /* Set Return Status as Not Initialized */
            eStatusReturn = GPIO_STATUS_NOT_INIT;
            break;
        }

        /* Only K20 Signals, Expander Interrupts are Out-of-Scope now*/
        if ((GPIO_SIGNAL_LAST <= eSignal) ||
            (GPIO_SIG_uP_K20 != mxSignalTable[eSignal].eSigType))
        {
            eStatusReturn = GPIO_STATUS_INVALID_INPUT;
            break;
        }

        /* Is there a Callback Already Enabled? */
        if(NULL == mxSignalTable[eSignal].pxIntConfig)
        {
            eStatusReturn = GPIO_STATUS_INVALID_INPUT;
            break;
        }

        /* All good so far, Call L2 to Disable the GPIO Pin Call Back */
        eStatusReturn = L2_GpioConfigPin((GPIO_uP_PORT)(mxSignalTable[eSignal].u8Port),
                             mxSignalTable[eSignal].ePin,
                             mxSignalTable[eSignal].ePinDir,
                             NULL);   /* Set Null */

        if(GPIO_STATUS_OK == eStatusReturn)
        {
            mxSignalTable[eSignal].pxIntConfig = NULL;
        }

    } while (false);

    /* Return the status */
    return eStatusReturn;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

