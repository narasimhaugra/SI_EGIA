#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Gpio
 * \{
 * \brief   Layer 2 GPIO driver
 *
 * \details This driver handles the MK20's direct port GPIOs.
 *          The functions contained in this module provide the following
 *          capabilities:
 *             - Configuring Direct GPIO Ports/Pins.
 *             - Setting GPIO Pins.
 *             - Clearing GPIO Pins.
 *             - Toggling GPIO Pins.
 *             - Getting GPIO Pin Values.
 *             - Setting GPIO Pin Interrupts.
 *
 * \note 1)   GPIO expander is not handled in this file.
 *       2)   This module will not configure GPIO pin additional config like
 *             - Drive Strength Enable  [DSE]
 *             - Open Drain Enable      [ODE]
 *             - Passive Filter Enable  [PFE]
 *             - Slew Rate Enable       [SRE]
 *             - Pull Enable            [PE]
 *             - Pull Select            [PS]
 *          Update as per the need in L2_PortCtrl.c
 *
 * \sa      K20 SubFamily Reference Manual
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file L2_Gpio.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L2_Gpio.h"
#include "Board.h"

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
typedef struct                      /// GPIO uP Port Register Table
{
    GPIO_uP_PORT        ePort;        ///< GPIO uP Port Enum
    PORT_MemMapPtr      pRegPCR;      ///< Port Control Register (PCR)
    volatile uint32_t  *pu32RegPDDR;  ///< Port Data Direction Reg (GPIOx_PDDR)
    volatile uint32_t  *pu32RegPSOR;  ///< Port Set Output Register (GPIOx_PSOR)
    volatile uint32_t  *pu32RegPCOR;  ///< Port Clear Output Register (GPIOx_PCOR)
    volatile uint32_t  *pu32RegPTOR;  ///< Port Toggle Output Register (GPIOx_PTOR)
    volatile uint32_t  *pu32RegPDIR;  ///< Port Data Input Register (GPIOx_PDIR)
    volatile uint32_t  *pu32RegISFR;  ///< Interrupt Status Flag Register (PORTx_ISFR)
    uint8_t             u8PortIRQ;    ///< Port Interrupt IRQ
} GPIO_uP_PORT_REG_TABLE_T;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
static const GPIO_uP_PORT_REG_TABLE_T mxRegisterTable[] =
{
    /*  ePort,          pu32RegPCR,     pu32RegPDDR, pu32RegPSOR, pu32RegPCOR, pu32RegPTOR, pu32RegPDIR, pu32RegISFR, u8PortIRQ  */
    { GPIO_uP_PORT_A, PORTA_BASE_PTR, &GPIOA_PDDR, &GPIOA_PSOR, &GPIOA_PCOR, &GPIOA_PTOR, &GPIOA_PDIR, &PORTA_ISFR, GPIO_PORT_A_IRQ },
    { GPIO_uP_PORT_B, PORTB_BASE_PTR, &GPIOB_PDDR, &GPIOB_PSOR, &GPIOB_PCOR, &GPIOB_PTOR, &GPIOB_PDIR, &PORTB_ISFR, GPIO_PORT_B_IRQ },
    { GPIO_uP_PORT_C, PORTC_BASE_PTR, &GPIOC_PDDR, &GPIOC_PSOR, &GPIOC_PCOR, &GPIOC_PTOR, &GPIOC_PDIR, &PORTC_ISFR, GPIO_PORT_C_IRQ },
    { GPIO_uP_PORT_D, PORTD_BASE_PTR, &GPIOD_PDDR, &GPIOD_PSOR, &GPIOD_PCOR, &GPIOD_PTOR, &GPIOD_PDIR, &PORTD_ISFR, GPIO_PORT_D_IRQ },
    { GPIO_uP_PORT_E, PORTE_BASE_PTR, &GPIOE_PDDR, &GPIOE_PSOR, &GPIOE_PCOR, &GPIOE_PTOR, &GPIOE_PDIR, &PORTE_ISFR, GPIO_PORT_E_IRQ },
    { GPIO_uP_PORT_F, PORTF_BASE_PTR, &GPIOF_PDDR, &GPIOF_PSOR, &GPIOF_PCOR, &GPIOF_PTOR, &GPIOF_PDIR, &PORTF_ISFR, GPIO_PORT_F_IRQ }
};

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static GPIO_uP_INT_CALLBACK_T mpxInterruptCallBackTable[GPIO_uP_PORT_LAST][GPIO_PIN_LAST];  ///< Table that holds the GPIO Callbacks.
static bool mbPinInitTable[GPIO_uP_PORT_LAST][GPIO_PIN_LAST];                               ///< Table that holds the GPIO Pin Init Details.

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void GpioPerformCallBacks(GPIO_uP_PORT ePort);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Port Interrupt Call Back Function. Called from ISR.
 *
 * \details All pins in PORTx shares same IRQ. So once the interrupt is hit,
 *          Check the PORTX_ISFR register for the PORTx to know which pin
 *          raised the interrupt
 *
 * \param   ePort - GPIO uP Port to configure
 *
 * ========================================================================== */
static void GpioPerformCallBacks(GPIO_uP_PORT ePort)
{
    uint8_t u8Pin;

    /* Is Input Valid */
    if(GPIO_uP_PORT_LAST > ePort)
    {
        /* Iterate over the bits in PORTx_ISFR and see
        which GPIO Pin raised the interrupt. */
        for(u8Pin = 0; u8Pin < GPIO_PIN_LAST; u8Pin++)
        {
            /* Is PORTx_ISFR bit is set? */
            if((*(mxRegisterTable[ePort].pu32RegISFR) & GPIO_MASK_PIN(u8Pin)))
            {
                /* Is there a Call back already registered ? */
                if(NULL != mpxInterruptCallBackTable[ePort][u8Pin])
                {
                    /* Yes, a valid Call back is registered, Do the call back */
                    mpxInterruptCallBackTable[ePort][u8Pin]();
                }

                /* Interrupt serviced, clear the bit by writing a 1 to it */
                *(mxRegisterTable[ePort].pu32RegISFR) |= GPIO_MASK_PIN(u8Pin);
            }
        }
    }
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Initialize the specified GPIO Pin in the Microcontroller.
 *
 * \details Uses following Micro Registers
 *          Port Control (PCR) - Sets Pin as GPIO, Sets Interrupts
 *          GPIO Port Data Dir Reg (GPIOx_PDDR)- GPIO Input or Output
 *
 * \param   ePort - GPIO uP Port to configure
 * \param   ePin - GPIO uP Pin Number to configure
 * \param   ePinDirection - GPIO Pin Direction Type (In/Out)
 * \param   pxIntConfig - Gpio Pin Interrupt Configuration.
 *
 * \return  GPIO_STATUS - Status return
 * \retval      GPIO_STATUS_INVALID_INPUT - If Invalid Input.
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
GPIO_STATUS L2_GpioConfigPin(GPIO_uP_PORT ePort,
                                      GPIO_PIN ePin,
                                      GPIO_DIR ePinDirection,
                                      GPIO_uP_PIN_INT_CONFIG_T * pxIntConfig)
{
    GPIO_STATUS eStatusReturn;  /* Gpio Return Status Variable */

    /* Is Input Valid */
    if((GPIO_uP_PORT_LAST > ePort) &&
       (GPIO_PIN_LAST > ePin) &&
       (GPIO_DIR_LAST > ePinDirection))
    {
        /* Is there an Interrupt Config Available? */
        if(NULL != pxIntConfig)
        {
            /* Is there an Interrupt callback registered for this GPIO? */
            if((NULL != pxIntConfig->pxInterruptCallback) &&
               (GPIO_uP_INT_DISABLED != pxIntConfig->eInterruptType))
            {
                /* Clear any prior Mux & IRQC values */
                PORT_PCR_REG(mxRegisterTable[ePort].pRegPCR, ePin) &=
                    ~(PORT_PCR_MUX_MASK | PORT_PCR_IRQC_MASK);

                /* Set the port control register as GPIO and set type of
                Pin interrupt */
                PORT_PCR_REG(mxRegisterTable[ePort].pRegPCR, ePin) |=
                    PORT_PCR_MUX(1) |
                    PORT_PCR_IRQC(pxIntConfig->eInterruptType);

                /* Clear any prior Interrupt, by writing a 1 to corresponding ISFR bit */
                *(mxRegisterTable[ePort].pu32RegISFR) |= GPIO_MASK_PIN(ePin);

                /* Map the call back to the proper port and pin slot */
                mpxInterruptCallBackTable[ePort][ePin] =
                    pxIntConfig->pxInterruptCallback;

                /* Enable Port IRQ */
                Enable_IRQ(mxRegisterTable[ePort].u8PortIRQ);
            }
        }
        else
        {
            /* Clear any prior Mux values */
            PORT_PCR_REG(mxRegisterTable[ePort].pRegPCR, ePin)  &=
                ~(PORT_PCR_MUX_MASK | PORT_PCR_IRQC_MASK);

            /* Set the Port Config Register as GPIO */
            PORT_PCR_REG(mxRegisterTable[ePort].pRegPCR, ePin) |=
                PORT_PCR_MUX(1);

            /* Clear any prior Interrupt, by writing a 1 to corresponding ISFR bit */
            *(mxRegisterTable[ePort].pu32RegISFR) |= GPIO_MASK_PIN(ePin);

            /* Clear the call back */
            mpxInterruptCallBackTable[ePort][ePin] = NULL;

            /* We cannot Disable IRQ, because a Single IRQ is shared
            accross an entire Port */
        }

        /* Set the Port Data Direction Reg (GPIOx_PDDR) By default, the
        pins are input [Logic Low], set it Logic High for Output pins.*/
        if(GPIO_DIR_OUTPUT == ePinDirection)
        {
            *(mxRegisterTable[ePort].pu32RegPDDR) |= GPIO_MASK_PIN(ePin);
        }

        /* Update the Global Pin Init Table,
        that the Pin is initialized */
        mbPinInitTable[ePort][ePin] = true;

        /* Set Return Status as OK */
        eStatusReturn = GPIO_STATUS_OK;
    }
    else
    {
        /* Set Return Status as Invalid Input */
        eStatusReturn = GPIO_STATUS_INVALID_INPUT;
    }

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   Sets the specified GPIO pin
 *
 * \details This function sets the GPIO pin - Uses Micro Register
 *          Port Set Output Register (GPIOx_PSOR) - Port Set Output
 *          Writing to this register will update the contents of the
 *          corresponding bit in the Port Data Output Register
 *          (PDOR) as follows:
 *          0 - Corresponding bit in PDORn does not change.
 *          1 - Corresponding bit in PDORn is set to logic one.
 *
 * \param   ePort - GPIO Port where the Pin is present
 * \param   ePin - GPIO Pin to set
 *
 * \return  GPIO_STATUS - Status return
 * \retval      GPIO_STATUS_INVALID_INPUT - If Invalid Input.
 * \retval      GPIO_STATUS_NOT_CONFIGURED - If Not Configured.
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
GPIO_STATUS L2_GpioSetPin(GPIO_uP_PORT ePort, GPIO_PIN ePin)
{
    GPIO_STATUS eStatusReturn;  /* Gpio Return Status Variable */

    /* Is Input Valid */
    if((GPIO_uP_PORT_LAST > ePort) && (GPIO_PIN_LAST > ePin))
    {
        /* Is the Pin configured Prior? */
        if(mbPinInitTable[ePort][ePin])
        {
            /* Set the Port Set Output register */
            *(mxRegisterTable[ePort].pu32RegPSOR) = GPIO_MASK_PIN(ePin);

            /* Set Return Status as OK */
            eStatusReturn = GPIO_STATUS_OK;
        }
        else
        {
            /* Set Return Status as Not Configured */
            eStatusReturn = GPIO_STATUS_NOT_CONFIGURED;
        }
    }
    else
    {
        /* Set Return Status as Invalid Input */
        eStatusReturn = GPIO_STATUS_INVALID_INPUT;
    }

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   Clears the specified GPIO pin
 *
 * \details This function sets the GPIO pin - Uses Micro Register
 *          Port Clear Output Register (GPIOx_PCOR) - Port Clear Output
 *          Writing to this register will update the contents of the
 *          corresponding bit in the Port Data Output Register
 *          (PDOR) as follows:
 *          0 - Corresponding bit in PDORn does not change.
 *          1 - Corresponding bit in PDORn is cleared to logic zero.
 *
 * \param   ePort - GPIO Port where the Pin is present
 * \param   ePin - GPIO Pin to clear
 *
 * \return  GPIO_STATUS - Status return
 * \retval      GPIO_STATUS_INVALID_INPUT - If Invalid Input.
 * \retval      GPIO_STATUS_NOT_CONFIGURED - If Not Configured.
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
GPIO_STATUS L2_GpioClearPin(GPIO_uP_PORT ePort, GPIO_PIN ePin)
{
    GPIO_STATUS eStatusReturn;  /* Gpio Return Status Variable */

    /* Is Input Valid */
    if((GPIO_uP_PORT_LAST > ePort) && (GPIO_PIN_LAST > ePin))
    {
        /* Is the Pin configured Prior? */
        if(mbPinInitTable[ePort][ePin])
        {
            /* Set the Port Clear Output register */
            *(mxRegisterTable[ePort].pu32RegPCOR) = GPIO_MASK_PIN(ePin);

            /* Set Return Status as OK */
            eStatusReturn = GPIO_STATUS_OK;
        }
        else
        {
            /* Set Return Status as Not Configured */
            eStatusReturn = GPIO_STATUS_NOT_CONFIGURED;
        }
    }
    else
    {
        /* Set Return Status as Invalid Input */
        eStatusReturn = GPIO_STATUS_INVALID_INPUT;
    }

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   Toggles the Output of specified GPIO pin
 *
 * \details This function sets the GPIO pin - Uses Micro Register
 *          Port Toggle Output Register (GPIOx_PTOR) - Port Toggle Output
 *          Writing to this register will update the contents of the
 *          corresponding bit in the Port Data Output Register
 *          (PDOR) as follows:
 *          0 - Corresponding bit in PDORn does not change.
 *          1 - Corresponding bit in PDORn is set to the inverse of its
 *          existing logic state.
 *
 * \param   ePort - GPIO Port where the Pin is present
 * \param   ePin - GPIO Pin to Toggle
 *
 * \return  GPIO_STATUS - Status return
 * \retval      GPIO_STATUS_INVALID_INPUT - If Invalid Input.
 * \retval      GPIO_STATUS_NOT_CONFIGURED - If Not Configured.
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
GPIO_STATUS L2_GpioTogglePin(GPIO_uP_PORT ePort, GPIO_PIN ePin)
{
    GPIO_STATUS eStatusReturn;  /* Gpio Return Status Variable */

    /* Is Input Valid */
    if((GPIO_uP_PORT_LAST > ePort) && (GPIO_PIN_LAST > ePin))
    {
        /* Is the Pin configured Prior? */
        if(mbPinInitTable[ePort][ePin])
        {
            /* Set the Port Toggle Output register */
            *(mxRegisterTable[ePort].pu32RegPTOR) = GPIO_MASK_PIN(ePin);

            /* Set Return Status as OK */
            eStatusReturn = GPIO_STATUS_OK;
        }
        else
        {
            /* Set Return Status as Not Configured */
            eStatusReturn = GPIO_STATUS_NOT_CONFIGURED;
        }
    }
    else
    {
        /* Set Return Status as Invalid Input */
        eStatusReturn = GPIO_STATUS_INVALID_INPUT;
    }

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief   Reads the state of a specified GPIO pin
 *
 * \details This function reads the GPIO pin - Uses Micro Register
 *          Port Data Input Register (GPIOx_PDIR) - Port Data Input
 *          Unimplemented pins for a particular device read as zero.
 *          Pins that are not configured for a digital function read as
 *          zero. If the corresponding Port Control and Interrupt module
 *          is disabled, then that Port Data Input Register does not update.
 *          0 - Pin logic level is logic zero or is not configured for use
 *          by digital function.
 *          1 - Pin logic level is logic one.
 *
 * \param   ePort - GPIO Port where the Pin is present
 * \param   ePin - GPIO Pin to Read
 * \param   *pbGetValue - Boolean Value to Get.
 *              - true, GPIOx_PDIR, Pin logic level is logic 1
 *              - false, GPIOx_PDIR, Pin logic level is logic 0
 *                      or is not configured for use by digital function.
 *
 * \return  GPIO_STATUS - Status return
 * \retval      GPIO_STATUS_INVALID_INPUT - If Invalid Input.
 * \retval      GPIO_STATUS_NOT_CONFIGURED - If Not Configured.
 * \retval      GPIO_STATUS_OK - If success.
 *
 * ========================================================================== */
GPIO_STATUS L2_GpioGetPin(GPIO_uP_PORT ePort, GPIO_PIN ePin, volatile bool *pbGetValue)
{
    GPIO_STATUS eStatusReturn;  /* Gpio Return Status Variable */

    /* Is Input Valid */
    if((NULL != pbGetValue) && (GPIO_uP_PORT_LAST > ePort) && (GPIO_PIN_LAST > ePin))
    {
        /* Is the Pin configured Prior? */
        if(mbPinInitTable[ePort][ePin])
        {
            /* Read the Port Data Input register */
            *pbGetValue = (((*(mxRegisterTable[ePort].pu32RegPDIR)) & GPIO_MASK_PIN(ePin))? true : false);

            /* Set Return Status as OK */
            eStatusReturn = GPIO_STATUS_OK;
        }
        else
        {
            /* Set Return Status as Not Configured */
            eStatusReturn = GPIO_STATUS_NOT_CONFIGURED;
        }
    }
    else
    {
        /* Set Return Status as Invalid Input */
        eStatusReturn = GPIO_STATUS_INVALID_INPUT;
    }

    /* Return the status */
    return eStatusReturn;
}

/* ========================================================================== */
/**
 * \brief  Port A Interrupt Handler
 *
 * \details All pins in PORT A shares same IRQ. So once the interrupt is hit,
 *          Check the PORTA_ISFR register to know which pin raised the
 *          interrupt. Below Information is common for all PORTx_ISFRs
 *
 *          Interrupt Status Flag Register (PORTx_ISFR) - Interrupt Status Flag
 *          Each bit in the field indicates the detection of the configured
 *          interrupt of the same number as the bit.
 *          0 - Configured interrupt has not been detected.
 *          1 - Configured interrupt has been detected. If pin is configured to
 *          generate a DMA request then the corresponding flag will be cleared
 *          automatically at the completion of the requested DMA transfer,
 *          otherwise the flag remains set until a logic one is written to the
 *          flag. If configured for a level sensitive interrupt and the pin
 *          remains asserted then the flag will set again immediately after
 *          it is cleared.
 *
 * ========================================================================== */
void L2_GpioPortA_ISR( void )
{
    OS_CPU_SR  cpu_sr;

    /* \ToDo Legacy OS Code update later */
    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    /* Perform Call Back on Port A*/
    GpioPerformCallBacks(GPIO_uP_PORT_A);

    /* \ToDo Legacy OS Code update later */
    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief  Port B Interrupt Handler
 *
 * \details All pins in PORT B shares same IRQ. So once the interrupt is hit,
 *          Check the PORTB_ISFR register to know which pin raised the
 *          interrupt
 *
 * ========================================================================== */
void L2_GpioPortB_ISR( void )
{
    OS_CPU_SR  cpu_sr;

    /* \ToDo Legacy OS Code update later */
    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    /* Perform Call Back on Port B */
    GpioPerformCallBacks(GPIO_uP_PORT_B);

    /* \ToDo Legacy OS Code update later */
    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief  Port C Interrupt Handler
 *
 * \details All pins in PORT C shares same IRQ. So once the interrupt is hit,
 *          Check the PORTC_ISFR register to know which pin raised the
 *          interrupt
 *
 * ========================================================================== */
void L2_GpioPortC_ISR( void )
{
    OS_CPU_SR  cpu_sr;

    /* \ToDo Legacy OS Code update later */
    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    /* Perform Call Back on Port C */
    GpioPerformCallBacks(GPIO_uP_PORT_C);

    /* \ToDo Legacy OS Code update later */
    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief  Port D Interrupt Handler
 *
 * \details All pins in PORT D shares same IRQ. So once the interrupt is hit,
 *          Check the PORTD_ISFR register to know which pin raised the
 *          interrupt
 *
 * ========================================================================== */
void L2_GpioPortD_ISR( void )
{
    OS_CPU_SR  cpu_sr;

    /* \ToDo Legacy OS Code update later */
    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    /* Perform Call Back on Port D */
    GpioPerformCallBacks(GPIO_uP_PORT_D);

    /* \ToDo Legacy OS Code update later */
    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief  Port E Interrupt Handler
 *
 * \details All pins in PORT E shares same IRQ. So once the interrupt is hit,
 *          Check the PORTE_ISFR register to know which pin raised the
 *          interrupt
 *
 * ========================================================================== */
void L2_GpioPortE_ISR( void )
{
    OS_CPU_SR  cpu_sr;

    /* \ToDo Legacy OS Code update later */
    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    /* Perform Call Back on Port E */
    GpioPerformCallBacks(GPIO_uP_PORT_E);

    /* \ToDo Legacy OS Code update later */
    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief  Port F Interrupt Handler
 *
 * \details All pins in PORT F shares same IRQ. So once the interrupt is hit,
 *          Check the PORTF_ISFR register to know which pin raised the
 *          interrupt
 *
 * ========================================================================== */
void L2_GpioPortF_ISR( void )
{
    OS_CPU_SR  cpu_sr;

    /* \ToDo Legacy OS Code update later */
    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    /* Perform Call Back on Port F */
    GpioPerformCallBacks(GPIO_uP_PORT_F);

    /* \ToDo Legacy OS Code update later */
    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   Returns the Callback function of a specified GPIO pin
 *
 * \details gives access to the pin interrupt callback functions.
 *           This is for now used by Test Manager to simulate a pin change interrupt
 *
 * \param   ePort - GPIO Port where the Pin is present
 * \param   ePin - GPIO Pin to Read
 *
 * \return  GPIO_uP_INT_CALLBACK_T - returns the pointer to callback functon for pin change interrupt
 *
 * ========================================================================== */

GPIO_uP_INT_CALLBACK_T L2_GpioGetPinConfig(GPIO_uP_PORT ePort, GPIO_PIN ePin)
{
    return mpxInterruptCallBackTable[ePort][ePin];

}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
