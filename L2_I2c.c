
/// \todo 03/31/2021 DAZ - Issues to resolve:
///
/// \todo 03/29/2021 DAZ - L3 I2cTransfer has hard coded wait for Mutex.        Currently for debugging. Make permanent?
/// \todo 03/25/2021 DAZ - Hard code timeout in I2cWait?                        This was fixed at 2mS in legacy.
/// \todo 03/25/2021 DAZ - Remove functions from return statements in L3_OneWireLink.
/// \todo 04/13/2021 DAZ - Clock speed names should be frequency independent, ie: I2C_CLOCK_HIGH, I2C_CLOCK_STD, etc.
/// \todo 04/14/2021 DAZ - Remove necessity for block read function. (OneWire issue)
/// \todo 04/14/2021 DAZ - Callers should specify clock speed in frequency independent manner. (I2C_CLOCK_STD, I2C_CLOCK_FAST)

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_I2c
 * \{
 * \brief   I2C control routines
 *
 * \details This file contains I2C functionality implementation.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_I2c.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Includes                                       */
/******************************************************************************/
#include "Common.h"
#include "L2_I2c.h"
#include "L2_Gpio.h"
#include "L2_PortCtrl.h"
#include "FaultHandler.h"

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
/// \todo 01/06/2022 KIA: Need to properly format define comments for doxygen.
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_I2C) /* Log Group Identifier */
#define I2C_BAUD_MULT               (2u)            /* Baud rate multiplier for  94 Khz */
#define I2C_BAUD_MULT_FAST          (1u)            /* Baud rate multiplier for 375 Khz */

#define I2C_BAUD_ICR_PRESC_94K      (0x20u)         /* Baud rate prescaler for 93.75 Khz */
#define I2C_BAUD_ICR_PRESC_375K     (0x14u)         /* Baud rate prescaler for   375 Khz */

/* Macro to create address frame based on slave address and transfer direction */
#define I2C_ADDR_BYTE(ADDR, Dir)       (((ADDR) << 1u) | ((I2C_DIR_RD == (Dir)) ? 1u : 0u))

#define I2C_START()         (I2C0_C1 |= (I2C_C1_TX_MASK | I2C_C1_MST_MASK))     /*  Initiate start sequence, Configure as I2C Master  */
#define I2C_REPEAT_START()  (I2C0_C1 |= I2C_C1_RSTA_MASK)                       /*  Initiate repeated start sequence */
#define I2C_ENABLE()        (I2C0_C1 |= I2C_C1_IICEN_MASK)                      /*  Enable I2C HW module */
#define I2C_DISABLE()       (I2C0_C1 &= ~(I2C_C1_IICEN_MASK))                   /*  Disable I2C HW module */
#define I2C_STOP()          (I2C0_C1 &= ~(I2C_C1_MST_MASK | I2C_C1_TX_MASK))    /*  Initiate a stop sequence */
#define I2C_IS_BUS_BUSY()   (I2C0_S  &  I2C_S_BUSY_MASK)                        /*  Check bus busy flag */

#define I2C_SET_TXACK()     (I2C0_C1 |= I2C_C1_TXAK_MASK)           // Set TXACK bit as req'd
#define I2C_CLR_TXACK()     (I2C0_C1 &= ~I2C_C1_TXAK_MASK)          // Clear TXACK bit as req'd

#define I2C_GET_TXACK()     ((I2C0_C1  >> I2C_C1_TXAK_SHIFT)  & 1)  // Get TXACK bit value
#define I2C_GET_RXACK()     ((I2C0_S   >> I2C_S_RXAK_SHIFT)   & 1)  // Get RXACK bit value
#define I2C_GET_TCF()       ((I2C0_S   >> I2C_S_TCF_SHIFT)    & 1)  // Get TCF bit value

#define I2C_WRITE(data)     ((I2C0_D) = (data))     /*  Writes to slave device */
#define I2C_READ()          (I2C0_D)                /*  Reads from slave device */

#define I2C_BUSY_KILL_DURATION    (20u)             /*  Maximum retry duration(ticks) before we try bus recovery - About 500nS */
#define RESET_I2C_SCL_TOGGLE_MAX  (50u)             /*  Maximum clock toggle count before we give up */
#define RESET_I2C_SCL_TOGGLE_MIN  (10u)             /*  Minimum clock toggle before check if the bus is free */
#define I2C_CONT_RST_DEL          (200u)            /*  SCL & SCK toggle delay used during I2C bus recovery - About 5uS (25nS/count) */
#define DELAY_COUNT_1000          (1200u)           /* Delay constant - About 25uS (25nS/count) 26-SEP-22 */

#define GPIO_I2C_SCL0             (0x00000004u)     /* I2C Clock pin */
#define GPIO_I2C_SDA0             (0x00000008u)     /* I2C Data pin */

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                /* Used to indicate data transfer direction. */
{
    I2C_DIR_RD,             /*! Master is reading */
    I2C_DIR_WR,             /*! Master is writing */
    I2C_DIR_COUNT
} I2C_DIR;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static I2C_STATUS I2cWait(void);
static void I2cAddressFrame7(uint8_t Slave, I2C_DIR Dir);
static I2C_STATUS I2cBusyCheck(void);
static void I2cResetModule(void);
static void I2cDelay(uint32_t Count);

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static I2CControl ActiveI2cConfig;                          /*! Active I2C configuration, every configuration change will update this structure */
static bool       I2cInitalized = false;                    /*! Flag to indicate I2C is initialized and ready to use */
static OS_EVENT  *pSemaI2cWait = NULL;                      /*! I2C transfer complete semaphore. Caller waits, transfer ISR will releases wait. */

/* ========================================================================== */
/**
 * \brief   I2C hardware initialization routine.
 *
 * \details Initializes all I2C ports as per hardware design
 *
 * \note    This function is intended to be called once during the system
 *          initialization to initialize all applicable I2C hardware ports.
 *          Any other I2C interface functions should be called only after
 *          calling this function.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_I2cInit(void)
{
    uint8_t u8Error;  /* Error status from OS calls. */

    ActiveI2cConfig.AddrMode = I2C_ADDR_MODE_7BIT;	/* 7 bit slave address */
    ActiveI2cConfig.Clock = I2C_CLOCK_78K;          /* start with lowest clock speed */
    ActiveI2cConfig.State = I2C_STATE_ENA;			/* Enabled by default */
    ActiveI2cConfig.Timeout = 0;					/* Blocking call by default */

    // Create I2c Wait Semaphore
    pSemaI2cWait = SigSemCreate(0, "I2C-TCF", &u8Error);
    if (NULL != pSemaI2cWait)
    {
        SIM_SCGC4 |= SIM_SCGC4_IIC2_MASK;                                       /* Enable I2C0 (Mask naming error in MK20F12.h) */
        I2C0_F = I2C_F_MULT(I2C_BAUD_MULT) | I2C_F_ICR(I2C_BAUD_ICR_PRESC_94K); /* Set initial value to lowest bit rate used */ /// \todo 04/14/2021 DAZ - To be renamed
        I2C0_C1 = I2C_C1_IICIE_MASK | I2C_C1_IICEN_MASK;                        /* Enable I2C and interrupts */

        PORTB_PCR2 |= ((INT32U)PORT_PCR_ODE_MASK);                              /* Ensure I2C is running in open drain mode */
        PORTB_PCR3 |= ((INT32U)PORT_PCR_ODE_MASK);

        Enable_IRQ(I2C0_IRQ);
        Set_IRQ_Priority(I2C0_IRQ, I2C_ISR_PRIORITY);
        I2cInitalized = true;                                                   /* I2C ready to use */
    }
}

/* ========================================================================== */
/**
 * \brief   I2C Configuration
 *
 * \details This function configures the specified I2C interface with the parameters supplied to the function.
 *          The is a blocking function, can also be used to enable, disable, activate sleep mode.
 *
 * \param   pControl - Pointer to configuration structure.
 *
 * \return  I2C_STATUS - Return status
 *
 * ========================================================================== */
I2C_STATUS L2_I2cConfig(I2CControl *pControl)
{
    I2C_STATUS Status;

    do
    {
        if (!I2cInitalized)
        {
            /* Config request before I2C module init, return error */
            Status = I2C_STATUS_FAIL_CONFIG;
            break;
        }

        Status = I2C_STATUS_FAIL_INVALID_PARAM;
        if (NULL == pControl) { break; }

        /* Validate state requested */
        if (pControl->State >= I2C_STATE_LAST) { break; }

        /// \todo 03/29/2021 DAZ - Is this really necessary? Just enable all the time in Init()?
        /* Enable/Disable State */
        if (I2C_STATE_ENA == pControl->State)                              // Switch power state
        {
            I2C_ENABLE();
        }
        else
        {
            I2C_DISABLE();
        }

        /* Assume all good, set state succcess */
        Status = I2C_STATUS_SUCCESS;

        /* Set clock */
        switch (pControl->Clock)
        {

/// \todo 04/13/2021 DAZ - Clocks should be properly renamed.... (SLOW, FAST)

            case I2C_CLOCK_78K:
                I2C0_F = I2C_F_MULT(I2C_BAUD_MULT) | I2C_F_ICR(I2C_BAUD_ICR_PRESC_94K);     // 60000000 / (4 * 160) = 93.75K
                break;

            case I2C_CLOCK_1M:   /// \todo 04/13/2021 DAZ - FPGA / IO Expander / DS2465 all run @ 400kC.
            case I2C_CLOCK_144K:
            case I2C_CLOCK_312K:
                I2C0_F = I2C_F_MULT(I2C_BAUD_MULT_FAST) | I2C_F_ICR(I2C_BAUD_ICR_PRESC_375K);   // 60000000 / (2 * 80) = 375K /// \todo 04/13/2021 DAZ - Was 4 * 48 = 312K
                break;

            default:
                I2C0_F = I2C_F_MULT(I2C_BAUD_MULT) | I2C_F_ICR(I2C_BAUD_ICR_PRESC_94K);     // Set to lowest used frequency (93.75 Khz)
                Status = I2C_STATUS_FAIL_INVALID_PARAM;
                break;
        }

        /* All good, now update the active configuration */
        if (Status == I2C_STATUS_SUCCESS)
        {
            ActiveI2cConfig.AddrMode = pControl->AddrMode;
            ActiveI2cConfig.Clock = pControl->Clock;
            ActiveI2cConfig.State = pControl->State;
            ActiveI2cConfig.Timeout = pControl->Timeout;
            ActiveI2cConfig.Device = pControl->Device;  /* Unused in L2 though */
        }
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   I2C Data Write
 *
 * \details This function is intended to be used to write to an I2C slave device.
 *          Allows the function to complete write with appropriate timeout set.
 *          If the timeout is 0, then this function blocks until write if complete.
 *
 * \param   pPacket - Pointer to data packet.
 *
 * \return  I2C_STATUS - Return status
 *
 * ========================================================================== */
I2C_STATUS L2_I2cWrite(I2CDataPacket *pPacket)
{
    I2C_STATUS Status;
    uint16_t Index;

    do
    {
        Status = I2C_STATUS_FAIL_INVALID_PARAM;    /* Assign default return status. */

        /* Check if the hardware is initialized */
        if (!I2cInitalized)
        {
            Status = I2C_STATUS_FAIL;
            break;
        }
        if (NULL == pPacket) { break; }

        Status = I2cBusyCheck();
        if (I2C_STATUS_BUSY == Status) { break; }

        I2C_CLR_TXACK();            /* Ensure ACK is on - Bit is low to send ACK, high to send NACK */

        /* Send start sequence and device address */
        I2cAddressFrame7(pPacket->Address, I2C_DIR_WR);

        Status = I2cWait();
        if (I2C_STATUS_SUCCESS != Status) { break; }    // Timeout - exit now

        /* Send register address if requested */
        if ((NULL != pPacket->pReg) &&  (pPacket->nRegSize))
        {
            Index = 0;
            while ((Index < pPacket->nRegSize) && (Status == I2C_STATUS_SUCCESS))
            {
                I2C_WRITE(pPacket->pReg[Index]);
                Status = I2cWait();
                Index++;
            }
        }

        if (I2C_STATUS_SUCCESS != Status) { break; }

        /* Send data if requested  */
        if ((NULL != pPacket->pData) &&  (0 != pPacket->nDataSize))
        {
            Index = 0;
            while ((Index < pPacket->nDataSize) && (Status == I2C_STATUS_SUCCESS))
            {
                I2C_WRITE(pPacket->pData[Index]);
                Status = I2cWait();
                Index++;
            }
        }
    } while (false);

    I2C_STOP();

    return Status;
}

/* ========================================================================== */
/**
 * \brief   I2C Data Read
 *
 * \details This function is intended to be used to read from an I2C slave device.
 *          Allows the function to complete read with appropriate timeout set.
 *          If the timeout is 0, then this function blocks until read if complete.
 *
/// \todo 04/08/2021 DAZ - Should this timeout (in L2_I2C0_ISR) be fixed @ 2mS as per legacy? Timeout is time to transfer a byte.
 *
 * \param   pPacket - Pointer to data packet.
 *
 * \return  I2C_STATUS - Return status
 *
 * ========================================================================== */
I2C_STATUS L2_I2cRead(I2CDataPacket *pPacket)
{
    I2C_STATUS Status;
    uint16_t Index;

    do
    {
        Status = I2C_STATUS_FAIL_INVALID_PARAM;

        /* Check if the hardware is initialized */
        if (!I2cInitalized)
        {
            Status = I2C_STATUS_FAIL;
            break;
        }

        if (NULL == pPacket) { break; }

        Status = I2cBusyCheck();
        if (I2C_STATUS_BUSY == Status) { break; }

        I2C_CLR_TXACK();            /* Ensure ACK is on - Bit is low to send ACK, high to send NACK */

        /* Send start sequence and device address  */
        I2cAddressFrame7(pPacket->Address, I2C_DIR_WR);

        Status = I2cWait();
        if (I2C_STATUS_SUCCESS != Status) { break; }    // Timeout - exit now

        /* Send register address if requested */
        if ((NULL != pPacket->pReg) && (pPacket->nRegSize))
        {
            Index = 0;
            while ((Index < pPacket->nRegSize) && (Status == I2C_STATUS_SUCCESS))
            {
                I2C_WRITE(pPacket->pReg[Index]);
                Status = I2cWait();
                Index++;
            }
        }

        /* Transfer timeout? */
        if ((I2C_STATUS_SUCCESS != Status) || !pPacket->nDataSize) { break; }

        /* Send repeated start */
        I2C_REPEAT_START();
        I2C_WRITE((uint8_t)I2C_ADDR_BYTE(pPacket->Address, I2C_DIR_RD));
        Status = I2cWait();
        if (I2C_STATUS_SUCCESS != Status) { break; }        // Timeout - exit now

        I2C0_C1 &= ~(I2C_C1_TX_MASK | I2C_C1_TXAK_MASK);    /* Put in Rx Mode & enable ACK */

        /* Read data */
        Index = 0;
        pPacket->pData[Index] = I2C_READ();                 /* Dummy read */
        Status = I2cWait();
        if (I2C_STATUS_SUCCESS != Status) { break; }        // Timeout - exit now

        while ((Index < pPacket->nDataSize) && (Status == I2C_STATUS_SUCCESS))
        {
            if ((Index + 1u) >= pPacket->nDataSize)         /* Reading last byte? */
            {
                /* Turn off ACK since this is second to last read*/
                I2C_SET_TXACK();
            }

            pPacket->pData[Index] = I2C_READ();
            Status = I2cWait();
            Index++;
        }
    } while (false);

    I2C_STOP();

    return Status;
}

/* ========================================================================== */
/**
 * \brief   I2C Bus status check and recovery
 *
 * \details This function checks I2C bus status for a timeout duration and force
 *          issue a stop to attempt to get bus control
 *
 * \param   < None >
 *
 * \return  I2C_STATUS - Return status
 *
 * ========================================================================== */
static I2C_STATUS I2cBusyCheck(void)
{
    I2C_STATUS Status;
    uint16_t Duration;

    Status = I2C_STATUS_BUSY;
    Duration = I2C_BUSY_KILL_DURATION;

    do
    {
        I2cDelay(DELAY_COUNT_1000);
        Status = L2_I2cStatus();
        if (I2C_STATUS_BUSY == Status)
        {
            Duration--;
            if (0 == Duration)
            {
                Log(DEV, "I2C Reset from BusyCheck");
                /* Tried enough, giving up. Try reset I2C module */
                I2cResetModule();
                L2_I2cConfig(&ActiveI2cConfig);
                Status = L2_I2cStatus();    /* Get the bus status after reset */
                Log(DEV, "Status after I2C Reset: %d", Status);
                break;
            }
        }
    } while (I2C_STATUS_BUSY == Status);
    /* I2C is BUSY after I2C Reset is Completed */
    if ( ( I2C_STATUS_BUSY == Status ) && ( 0 == Duration ) )
    {
        FaultHandlerSetFault(REQRST_I2CBUSLOCKUP, SET_ERROR);    
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   I2C Bus status
 *
 * \details This function returns the current I2C bus status
 *
 * \param   < None >
 *
 * \return  I2C_STATUS - Return status
 *
 * ========================================================================== */
I2C_STATUS L2_I2cStatus(void)
{
    I2C_STATUS Status;

    Status = I2C_STATUS_IDLE;

    if (I2C_IS_BUS_BUSY())
    {
        Status = I2C_STATUS_BUSY;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Reset I2C module
 *
 * \details This function is invoked to attempt possible recovery from bus
 *          contention by just toggling clock signal multiple times.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void I2cResetModule(void)
{
    uint32_t   portB_pcr;
    int16_t   retryCount;

    /* Enable the below line, during I2C related testing */

    /* Check if arbitration lost exception is set, and clear if set. */
    if ((I2C0_S & I2C_S_ARBL_MASK) == I2C_S_ARBL_MASK)
    {
        I2C0_S |= I2C_S_ARBL_MASK;
    }

    /* disable I2C and interrupts */
    I2C0_C1 = 0;

    /* turn off the I2C clock */
    SIM_SCGC4 &= ~SIM_SCGC4_IIC2_MASK;      /* This is the bit for I2C0. See K20 manual / MK20F12.h */

    /* turn PTB2 into a GPIO */
    portB_pcr = PORTB_PCR2;
    portB_pcr &= ~(PORT_PCR_MUX(2));
    portB_pcr |= PORT_PCR_MUX(1);
    PORTB_PCR2 = portB_pcr;

    /* turn PTB3 into a GPIO */
    portB_pcr = PORTB_PCR3;
    portB_pcr &= ~(PORT_PCR_MUX(2));
    portB_pcr |= PORT_PCR_MUX(1);
    PORTB_PCR3 = portB_pcr;

    /* Set SCL as output and SDA as input. */
    GPIOB_PDDR |= GPIO_I2C_SCL0;

    /* Toggle SCL at least RESET_I2C_SCL_TOGGLE_MIN times to get slave to release line. */
    retryCount = RESET_I2C_SCL_TOGGLE_MIN;

    /* Toggle line @ 100kHz (5uS up, 5uS down) */
    do
    {
        GPIOB_PSOR = GPIO_I2C_SCL0;
        I2cDelay(I2C_CONT_RST_DEL);
        GPIOB_PCOR = GPIO_I2C_SCL0;
        I2cDelay(I2C_CONT_RST_DEL);
        retryCount--;
    } while (retryCount);

    /* Toggle SCL until SDA is released by the slave (goes high). */
    retryCount = RESET_I2C_SCL_TOGGLE_MAX;

    do
    {
        GPIOB_PSOR = GPIO_I2C_SCL0;
        I2cDelay(I2C_CONT_RST_DEL);
        GPIOB_PCOR = GPIO_I2C_SCL0;
        I2cDelay(I2C_CONT_RST_DEL);
        retryCount--;
    } while (((GPIOB_PDIR & GPIO_I2C_SDA0) == 0) && (retryCount));

    /*  Manually simulate a STOP signal */
    /* Set SDA as output (SCL is already output) */
    GPIOB_PDDR |= GPIO_I2C_SDA0;

    /* Start of STOP signal: SDA is low, SCL is high */
    GPIOB_PCOR = GPIO_I2C_SDA0;
    GPIOB_PSOR = GPIO_I2C_SCL0;
    I2cDelay(I2C_CONT_RST_DEL);

    /* End of STOP signal: SDA is high, SCL is still high */
    GPIOB_PSOR = GPIO_I2C_SDA0;
    I2cDelay(I2C_CONT_RST_DEL);

    /* Remove GPIO as outputs */
    GPIOB_PDDR &= ~((INT32U)(GPIO_I2C_SCL0 | GPIO_I2C_SDA0));

    /* turn PTB2 back into SCK0 */
    portB_pcr = PORTB_PCR2;
    portB_pcr &= ~(PORT_PCR_MUX(1));
    portB_pcr |= PORT_PCR_MUX(2);
    PORTB_PCR2 = portB_pcr;

    /* turn PTB3 back into SDA0 */
    portB_pcr = PORTB_PCR3;
    portB_pcr &= ~(PORT_PCR_MUX(1));
    portB_pcr |= PORT_PCR_MUX(2);
    PORTB_PCR3 = portB_pcr;

    /* turn the I2C clocks back on */
    SIM_SCGC4 |= SIM_SCGC4_IIC1_MASK | SIM_SCGC4_IIC2_MASK;
    I2C0_F = 0;
    /* enable I2C and interrupts */
    I2C0_C1 = I2C_C1_IICIE_MASK | I2C_C1_IICEN_MASK;

    I2cDelay(I2C_CONT_RST_DEL);

    return;
}

/* ========================================================================== */
/**
 * \brief   Sends 7 bit I2C Address frame
 *
 * \details This function is creates and sends 7 bit address frame on I2C bus along with read and write indicator
 *
 * \param   Slave - 7  bit slave address.
 *          Dir - Direction, write if set to true, read if FALSE.
 *
 * \return  None
 *
 * ========================================================================== */
static void I2cAddressFrame7(uint8_t Slave, I2C_DIR Dir)
{
    uint8_t Addr;

    Addr = I2C_ADDR_BYTE(Slave, Dir);   /* Prepare Address frame */
    I2C_START();                        /* send start signal */
    I2C_WRITE(Addr);                    /* send address frame */
}

/* ========================================================================== */
/**
 * \brief   Wait for I2C transfer complete
 *
 * \details This function waits for an ongoing I2C transfer to complete
 *
 * \param   < None >
 *
 * \return  I2C_STATUS - Return status
 *
 * ========================================================================== */
static I2C_STATUS I2cWait(void)
{
    I2C_STATUS Status;
    uint8_t error;

    Status = I2C_STATUS_SUCCESS;

    /* Wait for the byte transfer or timeout. I2C ISR notifies the semaphore */
    OSSemPend(pSemaI2cWait, ActiveI2cConfig.Timeout, &error);

    if (OS_ERR_NONE  != error)
    {
        Status = I2C_STATUS_FAIL_TIMEOUT;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Delay utility function
 *
 * \details This is a no-os delay function used for duration lesser than tick.
 *
 * \note    Testing with a 100000 count loop shows the loop time yields a 2500uSec
 *          delay, or 25nS per tick. The test was performed with interrupts disabled,
 *          so actual delay times may be longer due to interrupts or higher priority
 *          task preemption. This also does not include function call overhead.
 *          (expected to be less than 50nS)
 *
 * \param   Count - Delay in 25nS ticks.
 *
 * \return  None
 *
 * ==========================================================================*/
static void I2cDelay(uint32_t Count)
{
    while (Count)
    {
        Count--;
    }
}

/* ========================================================================== */
/**
 * \brief   I2C Interrupt Service Routine
 *
 * \details This is a I2C0 interrupt service routine, triggered when
 *          I2C transfer is complete
 *
 * \param   < None >
 *
 * \return  None
 *
 * ==========================================================================*/
void L2_I2C0_ISR(void)
{
    OS_CPU_SR  cpu_sr;

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    if (I2C0_S & I2C_S_IICIF_MASK)
    {
        I2C0_S |= I2C_S_IICIF_MASK;     /* Clear interrupt status flag */

        if (I2C_GET_TCF())
        {
            // If the TXACK bit is set, we should get the RXACK bit set (NACK returned)
            // If the TXACK bit is NOT set, we should get the RXACK bit clear (ACK returned)

            if ((I2C_GET_TXACK() && I2C_GET_RXACK()) ||
                (!I2C_GET_TXACK() && !I2C_GET_RXACK()))
            {
                OSSemPost(pSemaI2cWait);    /* Signal waiting I2cWait() function. */
            }
        }
    }
    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   I2C Data BurstRead
 *
 * \details This function is mis-named. It is like the standard I2cRead() function,
 *          except that a repeated start is NOT sent before reading. The device
 *          address is written, immediately followed by the reading of data. (The
 *          dummy read is performed to start the process the same way as in I2cRead()).
 *
 * \note    This function is ONLY used when reading the computed MAC from the DS2465
 *          OneWire bus master chip. It is not required anywhere else.
 *
 * \todo 04/19/2021 DAZ - Clear a "SendRepeatedStart" flag and call I2c_Read to avoid code duplication. Rename this function.
 *
 * \param   pPacket - Pointer to data packet.
 *
 * \return  I2C_STATUS - Return status
 *
 * ========================================================================== */

/// \todo 04/08/2021 DAZ - Need to determine why BurstRead is necessary.
I2C_STATUS L2_I2cBurstRead(I2CDataPacket *pPacket)
{
    I2C_STATUS Status;
    uint16_t Index;

    do
    {
        Status = I2C_STATUS_FAIL_INVALID_PARAM;

        /* Check if the hardware is initialized */
        if (!I2cInitalized)
        {
            Status = I2C_STATUS_FAIL;
            break;
        }

        if (NULL == pPacket) { break; }

        Status = I2cBusyCheck();
        if (I2C_STATUS_BUSY == Status) { break; }

        I2cAddressFrame7(pPacket->Address, I2C_DIR_RD);

        Status = I2cWait();
        if (I2C_STATUS_SUCCESS != Status) { break; }    // Timeout - exit now

        I2C_CLR_TXACK();            /* Ensure ACK is on - Bit is low to send ACK, high to send NACK */

        /* Send register address if requested */
        if ((NULL != pPacket->pReg) && (pPacket->nRegSize))
        {
            Index = 0;
            while ((Index < pPacket->nRegSize) && (Status == I2C_STATUS_SUCCESS))
            {
                I2C_WRITE(pPacket->pReg[Index]);
                Status = I2cWait();
                Index++;
            }
        }

        /* Transfer timeout? */
        if ((I2C_STATUS_SUCCESS != Status) || !pPacket->nDataSize) { break; }

        I2C0_C1 &= ~(I2C_C1_TX_MASK | I2C_C1_TXAK_MASK);   /* Put in Rx Mode & enable ACK */

        /* Read data */
        Index = 0;
        pPacket->pData[Index] = I2C_READ(); /* Dummy read */
        Status = I2cWait();
        Status = I2C_STATUS_SUCCESS;
        while ((Index < pPacket->nDataSize) && (Status == I2C_STATUS_SUCCESS))
        {
            if ((Index + 1u) >= pPacket->nDataSize)
            {
                /* Turn off ACK since this is second to last read*/
                I2C_SET_TXACK();
            }
            pPacket->pData[Index] = I2C_READ();
            Status = I2cWait();
            Index++;
        }
    } while (false);

    I2C_STOP();

    return Status;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

