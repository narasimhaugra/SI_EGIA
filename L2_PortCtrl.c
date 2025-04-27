#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_PortCtrl
 * \{
 * \brief   Layer 2 Port Control and Interrupts
 *
 * \details This driver handles the MK20's Port Pin's configuration.
 *          This includes configuring the type of the pin [Analog/Digital],
 *          Interrupts config, Pin Drive type requirements, etc.
 *          The different options possible are
 *             - K20 Signal Mux Type
 *                  - Analog     [0]
 *                  - GPIO       [1]
 *                  - SPI/I2C    [2]
 *                  - UART       [3]
 *                  - SDHC       [4]
 *                  - FLEXBUS    [5]
 *             - Interrupt configuration
 *                  - 0000 Disabled.
 *                  - 0001 DMA request on rising edge.
 *                  - 0010 DMA request on falling edge.
 *                  - 0011 DMA request on either edge.
 *                  - 1000 Interrupt when logic 0.
 *                  - 1001 Interrupt on rising-edge.
 *                  - 1010 Interrupt on falling-edge.
 *                  - 1011 Interrupt on either edge.
 *             - Drive Strength Enable  [DSE]
 *             - Open Drain Enable      [ODE]
 *             - Passive Filter Enable  [PFE]
 *             - Slew Rate Enable       [SRE]
 *             - Pull Enable            [PE]
 *             - Pull Select            [PS]
 *
 * \sa      K20 SubFamily Ref Manual, Ch 11: Port Control and Interrupts (PORT)
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file L2_PortCtrl.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L2_PortCtrl.h"
#include "L2_Gpio.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
/* Internal Pull-Up Resistor Enabled | Pull Selected | High drive strength */
#define SDHC_EXTRA_PCR_CONFIG (PORT_PCR_PE_MASK | PORT_PCR_PS_MASK | PORT_PCR_DSE_MASK )

/* Internal Pull-Up Resistor Enabled */
#define ACCEL_EXTRA_PCR_CONFIG (PORT_PCR_PE_MASK)

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

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   This function configures the port control registers with
 *          its default values. Called during CPU init.
 *
 * \details Enables PORTA through PORTF Clock Gate Control
 *          Setting Port-Control Registers for Specific to Signia
 *            -  Setting Internal Pull-Up Resistor Enabled State
 *            -  Whether Pull Selected to Internal Pull-Up Resistor
 *            -  High Drive Strength Setting
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_PortCtrlInit(void)
{
    /* Enable PORTA thru PORTE clock gate control */
    SIM_SCGC5 |= ( SIM_SCGC5_PORTE_MASK |
                   SIM_SCGC5_PORTD_MASK |
                   SIM_SCGC5_PORTC_MASK |
                   SIM_SCGC5_PORTB_MASK |
                   SIM_SCGC5_PORTA_MASK |
                   SIM_SCGC5_LPTIMER_MASK);

    /* K20 Signal Mux Type 0 - Analog Signals */
    PORTA_PCR6   |= ALT_0_ANALOG;                           //TEMP_M1
    PORTA_PCR8   |= ALT_0_ANALOG;                           //M0_CURRENT_LIMIT
    PORTA_PCR9   |= ALT_0_ANALOG;                           //PEAK_CURR_M1
    PORTA_PCR10  |= ALT_0_ANALOG;                           //AVG_CURR_M1
    PORTA_PCR18  |= ALT_0_ANALOG;                           //EXTAL0
    PORTA_PCR19  |= ALT_0_ANALOG;                           //XTAL0
    PORTA_PCR26  |= ALT_0_ANALOG;                           //M2_CURRENT_LIMIT
    PORTA_PCR27  |= ALT_0_ANALOG;                           //TEMP_M2
    PORTA_PCR28  |= ALT_0_ANALOG;                           //PEAK_CURR_M2
    PORTA_PCR29  |= ALT_0_ANALOG;                           //AVG_CURR_M2
    PORTB_PCR7   |= ALT_0_ANALOG;                           //ADC1_SE13
    PORTE_PCR11  |= ALT_0_ANALOG;                           //M1_CURRENT_LIMIT

    /* K20 Signal Mux Type 1 - GPIO Signals */
    PORTA_PCR5   |= ALT_1_GPIO;                             //GPIO_MOT0_ADC_TRIG,
    PORTA_PCR7   |= ALT_1_GPIO;                             //GPIO_1W_SHELL_EN,
    PORTA_PCR11  |= ALT_1_GPIO;                             //GPIO_WIFI_FORCE_AWAKE,
    PORTA_PCR12  |= ALT_1_GPIO;                             //GPIO_SLP_1Wn,
    PORTA_PCR13  |= ALT_1_GPIO | ACCEL_EXTRA_PCR_CONFIG;    //GPIO_DUAL_ACCEL_INT,
    PORTA_PCR24  |= ALT_1_GPIO;                             //GPIO_EXTRA_IO_uC0,
    PORTA_PCR25  |= ALT_1_GPIO;                             //GPIO_1W_AD_EN,
    PORTB_PCR0   |= ALT_1_GPIO;                             //GPIO_PERIPHERAL_WUn,
    PORTB_PCR1   |= ALT_1_GPIO;                             //GPIO_MOT1_ADC_TRIG,
    PORTB_PCR4   |= ALT_1_GPIO;                             //GPIO_WIFI_ENn,
    PORTB_PCR5   |= ALT_1_GPIO;                             //GPIO_WIFI_RESETn,
    PORTB_PCR6   |= ALT_1_GPIO;                             //GPIO_EN_BATT_15V,
    PORTB_PCR8   |= ALT_1_GPIO;                             //GPIO_GN_LED,
    PORTB_PCR9   |= ALT_1_GPIO;                             //GPIO_GN_KEY1n,
    PORTB_PCR10  |= ALT_1_GPIO;                             //GPIO_GN_KEY2n,
    PORTB_PCR11  |= ALT_1_GPIO;                             //GPIO_EXTRA_IO_uC3,
    PORTC_PCR3   |= ALT_1_GPIO;                             //GPIO_KEY_WAKEn,
    PORTC_PCR12  |= ALT_1_GPIO;                             //GPIO_OPEN_KEYn,
    PORTC_PCR13  |= ALT_1_GPIO;                             //GPIO_LEFT_CW_KEYn,
    PORTC_PCR18  |= ALT_1_GPIO;                             //GPIO_LEFT_CCW_KEYn,
    PORTC_PCR19  |= ALT_1_GPIO;                             //GPIO_RIGHT_CW_KEYn,
    PORTD_PCR7   |= ALT_1_GPIO;                             //GPIO_GPIO_INTn,
    PORTD_PCR8   |= ALT_1_GPIO;                             //GPIO_IM_GOOD,
    PORTD_PCR9   |= ALT_1_GPIO;                             //GPIO_RIGHT_CCW_KEYn,
    PORTD_PCR11  |= ALT_1_GPIO;                             //GPIO_LEFT_ARTIC_KEYn,
    PORTD_PCR12  |= ALT_1_GPIO;                             //GPIO_RIGHT_ARTIC_KEYn,
    PORTD_PCR13  |= ALT_1_GPIO;                             //GPIO_CLOSE_KEYn,
    PORTE_PCR6   |= ALT_1_GPIO;                             //GPIO_EXTRA_IO_uC2,
    PORTE_PCR7   |= ALT_1_GPIO;                             //GPIO_MOT2_ADC_TRIG,
    PORTE_PCR10  |= ALT_1_GPIO;                             //GPIO_EXTRA_IO_uC1,
    PORTE_PCR12  |= ALT_1_GPIO;                             //GPIO_SDHC0_LED,

    /* K20 Signal Mux Type 2 - SPI/I2C Signals */
    PORTA_PCR16  |= ALT_2_SPI_I2C;                          //SPI0_MOSI
    PORTA_PCR17  |= ALT_2_SPI_I2C;                          //SPI0_MISO
    PORTB_PCR2   |= ALT_2_SPI_I2C;                          //SCL0
    PORTB_PCR3   |= ALT_2_SPI_I2C;                          //SDA0
    PORTB_PCR20  |= ALT_2_SPI_I2C;                          //SPI2_CS0n
    PORTB_PCR21  |= ALT_2_SPI_I2C;                          //SPI2_SCLK
    PORTB_PCR22  |= ALT_2_SPI_I2C;                          //SPI2_MOSI
    PORTD_PCR1   |= ALT_2_SPI_I2C;                          //SPI0_SCLK
    PORTD_PCR14  |= ALT_2_SPI_I2C;                          //SPI2_MISO
    PORTD_PCR15  |= ALT_2_SPI_I2C;                          //SPI2_PCS1

    /* K20 Signal Mux Type 3 - SPI - Special case for SPI0_CS5n */
    PORTB_PCR23  |= ALT_3_SPI;                              //SPI0_CS5n

    /* K20 Signal Mux Type 3 - UART Signals */
    PORTA_PCR14  |= ALT_3_UART;                             //UART0_TX
    PORTA_PCR15  |= ALT_3_UART;                             //UART0_RX
    PORTC_PCR14  |= ALT_3_UART;                             //UART4_RX
    PORTC_PCR15  |= ALT_3_UART;                             //UART4_TX
    PORTE_PCR8   |= ALT_3_UART;                             //UART5_TX
    PORTE_PCR9   |= ALT_3_UART;                             //UART5_RX

    /* K20 Signal Mux Type 4 - SDHC Signals */
    PORTE_PCR0   |= ALT_4_SDHC | SDHC_EXTRA_PCR_CONFIG;     //SDHC0_D1
    PORTE_PCR1   |= ALT_4_SDHC | SDHC_EXTRA_PCR_CONFIG;     //SDHC0_D0
    PORTE_PCR2   |= ALT_4_SDHC | SDHC_EXTRA_PCR_CONFIG;     //SDHC0_DCLK
    PORTE_PCR3   |= ALT_4_SDHC | SDHC_EXTRA_PCR_CONFIG;     //SDHC0_CMD
    PORTE_PCR4   |= ALT_4_SDHC | SDHC_EXTRA_PCR_CONFIG;     //SDHC0_D3
    PORTE_PCR5   |= ALT_4_SDHC | SDHC_EXTRA_PCR_CONFIG;     //SDHC0_D2

    /* K20 Signal Mux Type 5 - FlexBus Signals */
    PORTB_PCR16  |= ALT_5_FLEXBUS;                          //FB_AD17
    PORTB_PCR17  |= ALT_5_FLEXBUS;                          //FB_AD16
    PORTB_PCR18  |= ALT_5_FLEXBUS;                          //FB_AD15
    PORTB_PCR19  |= ALT_5_FLEXBUS;                          //FB_OEn
    PORTC_PCR0   |= ALT_5_FLEXBUS;                          //FB_AD14
    PORTC_PCR1   |= ALT_5_FLEXBUS;                          //FB_AD13
    PORTC_PCR2   |= ALT_5_FLEXBUS;                          //FB_AD12
    PORTC_PCR4   |= ALT_5_FLEXBUS;                          //FB_AD11
    PORTC_PCR5   |= ALT_5_FLEXBUS;                          //FB_AD10
    PORTC_PCR6   |= ALT_5_FLEXBUS;                          //FB_AD9
    PORTC_PCR7   |= ALT_5_FLEXBUS;                          //FB_AD8
    PORTC_PCR8   |= ALT_5_FLEXBUS;                          //FB_AD7
    PORTC_PCR9   |= ALT_5_FLEXBUS;                          //FB_AD6
    PORTC_PCR10  |= ALT_5_FLEXBUS;                          //FB_AD5
    PORTC_PCR11  |= ALT_5_FLEXBUS;                          //FB_RWn
    PORTC_PCR16  |= ALT_5_FLEXBUS;                          //FB_CS5_b
    PORTC_PCR17  |= ALT_5_FLEXBUS;                          //FB_CS4_b
    PORTD_PCR0   |= ALT_5_FLEXBUS;                          //FB_ALE
    PORTD_PCR2   |= ALT_5_FLEXBUS;                          //FB_AD4
    PORTD_PCR3   |= ALT_5_FLEXBUS;                          //FB_AD3
    PORTD_PCR4   |= ALT_5_FLEXBUS;                          //FB_AD2
    PORTD_PCR5   |= ALT_5_FLEXBUS;                          //FB_AD1
    PORTD_PCR6   |= ALT_5_FLEXBUS;                          //FB_AD0

    /* K20 Signal Mux Type 6 - FlexBus Signals */
    PORTD_PCR10  |= ALT_6_FLEXBUS;                          //FB_AD18 for V5

    /* EN_BATT_15V must come up as an GPIO output, set low, as quickly as possible.*/
    L2_GpioConfigPin(GPIO_uP_PORT_B, GPIO_PIN_06, GPIO_DIR_OUTPUT, NULL);
    L2_GpioSetPin(GPIO_uP_PORT_B, GPIO_PIN_06);
}

/* ========================================================================== */
/**
 * \brief   Configures PCR Register of a K20 Pin
 *
 * \details This function can be used to configure the PCR register of a K20
 *          Port Pin. During system boot up, the pins will be configured
 *          using L2_PortCtrl_Init(), however, this function helps to change
 *          the configuration during any special condition. For example:
 *          During I2C bus contention, this function can be called to
 *          re-configure the I2C lines from 'ALT_2_SPI_I2C' config to GPIO
 *          config 'ALT_1_GPIO' and then toggle the lines. Once the
 *          contention is over it can be reverted to I2C signals
 *          'ALT_2_SPI_I2C' using this function.
 *
 * \param   pPCR - Port Control Register (PCR)
 * \param   ePin - uP Pin Number to configure
 * \param   u32PCRConfig - 32 bit PCR configuration.
 *
 * \return  None
 *
 * ========================================================================== */
void L2_PortCtrlConfigPin(PORT_MemMapPtr pPCR, GPIO_PIN ePin, uint32_t u32PCRConfig)
{
    /* Is Input Valid */
    if( (NULL != pPCR) && (GPIO_PIN_LAST > ePin) )
    {
        /* Update the 32 bit PCR with the new config passed */
        PORT_PCR_REG(pPCR, ePin) = u32PCRConfig;
    }
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
