#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_Accelerometer
 * \{
 * \brief   Accelerometer Functions
 *
 * \details Accelerometer is used to detect Handle movement. This Interface provides
 *          functions to initialize and configure the accelerometer module and also
 *          provides function for the application to get the accelerometer information.
 *          delete and manage the test tasks based on the test application requests.
 *
 * \note    \todo The drop and move threshold levels need to be given by the requirements team.
 *          Currently using the default values or legacy as thresholds.
 *
 * \sa      LIS3DH Reference Manual
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_Accelerometer.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Signia_Accelerometer.h"
#include "L3_Spi.h"
#include "L3_GpioCtrl.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER              (LOG_GROUP_ACCEL)   ///< Log Group Identifier

#define ACCEL_TASK_STACK                (512u)         ///< Accelerometer stack size.

#define ACCEL_BUF_SIZE                    (0x05u)             ///< Buffer length
#define ACCEL_TASK_DELAY                  (10000u)            ///< Delay value (10 seconds)

#define ACCEL_VALUE_SHIFT                 (6u)         ///< 10 bit values left aligned, shift by 6 to right align.
#define NUM_BITS_SHIFT                    (8u)         ///< Number of bits to shift
#define ACCEL_CTRL_REG1                   (0x20u)      ///< Register address for Ctrl Reg1.
#define ACCEL_CTRL_REG2                   (0x21u)      ///< Register address for Ctrl Reg2.
#define ACCEL_CTRL_REG3                   (0x22u)      ///< Register address for Ctrl Reg3.
#define ACCEL_CTRL_REG4                   (0x23u)      ///< Register address for Ctrl Reg4.
#define ACCEL_CTRL_REG5                   (0x24u)      ///< Register address for Ctrl Reg5.
#define ACCEL_CTRL_REG6                   (0x25u)      ///< Register address for Ctrl Reg6.

/* Values to be configured in Ctrl Registers  */
#define ACCEL_CTRL_REG1_XEN               (0x01u)      ///< Value to enable X axis.
#define ACCEL_CTRL_REG1_YEN               (0x02u)      ///< Value to enable Y axis.
#define ACCEL_CTRL_REG1_ZEN               (0x04u)      ///< Value to enable Z axis.
#define ACCEL_CTRL_REG1_10HZ              (0x20u)      ///< Value for ODR configuration.

#define ACCEL_CTRL_REG2_HPIS1             (0x01u)      ///< Value to enable High Pass filter on interrupt 1.
#define ACCEL_CTRL_REG2_FDS               (0x08u)      ///< Value to set Filtered data selection.
#define ACCEL_CTRL_REG3_I1_AOI1           (0x40u)      ///< Value to enable AOI1 interrupt on INT1.
#define ACCEL_TEST_READ_WHOAMI            (0x0Fu)
#define WHOAMI_VALUE                      (0x33u)      ///< WHO_AM_I Register value for verification.

#define ACCEL_CTRL_REG1_DEFAULT           (ACCEL_CTRL_REG1_XEN | ACCEL_CTRL_REG1_YEN | ACCEL_CTRL_REG1_ZEN | ACCEL_CTRL_REG1_10HZ)  ///< Default Value to write into register CTRL_REG1.
#define ACCEL_CTRL_REG2_DEFAULT           ((ACCEL_CTRL_REG2_FDS | ACCEL_CTRL_REG2_HPIS1))         ///< Default Value to write into register CTRL_REG2.
#define ACCEL_CTRL_REG3_DEFAULT           (ACCEL_CTRL_REG3_I1_AOI1)                             ///< Default Value to write into register CTRL_REG3.
#define ACCEL_CTRL_REG4_DEFAULT           (0x00u)      ///< Default Value to write into register CTRL_REG4.
#define ACCEL_CTRL_REG5_DEFAULT           (0x00u)      ///< Default Value to write into register CTRL_REG5.
#define ACCEL_CTRL_REG6_DEFAULT           (0x00u)      ///< Default Value to write into register CTRL_REG6.
#define ACCEL_CTRL_REG3_DISABLE_IA1       (0x00u)      ///< Value to disable the interrupts.

/*  Address of Int1 Registers The address is a byte wide and left shifted by 8 bits so that the MSB of 16 bit data has the address */
/* and the LSB is a pad byte */
#define ACCEL_INT1_CFG                    (0x30u)        ///< Address for INT1_CFG Register.
#define ACCEL_INT1_SRC                    (0x31u)        ///< Address for INT1_SRC Register.
#define ACCEL_INT1_THS                    (0x32u)        ///< Address for INT1_THS Register.
#define ACCEL_INT1_DUR                    (0x33u)        ///< Address for INT1_DUR Register.

/* Address of Int2 Registers The address is a byte wide and left shifted by 8 bits so that the MSB of 16 bit data has the address */
/* and the LSB is a pad byte */
#define ACCEL_INT2_CFG                    (0x34u)        ///< Address for INT2_CFG Register.
#define ACCEL_INT2_SRC                    (0x35u)        ///< Address for INT2_SRC Register.
#define ACCEL_INT2_THS                    (0x36u)        ///< Address for INT2_THS Register.
#define ACCEL_INT2_DUR                    (0x37u)        ///< Address for INT2_DUR Register.

/* Values to be configured in INT1 and INT2  Registers  */
#define ACCEL_INT1_CFG_XHIE               (0x02u)       ///< Value to Enable interrupt generation on X high event.
#define ACCEL_INT1_CFG_YHIE               (0x08u)       ///< Value to Enable interrupt generation on Y high event.
#define ACCEL_INT1_CFG_ZHIE               (0x20u)       ///< Value to Enable interrupt generation on Z high event.

#define ACCEL_INT1_CFG_DEFAULT            (ACCEL_INT1_CFG_XHIE | ACCEL_INT1_CFG_YHIE | ACCEL_INT1_CFG_ZHIE)   ///< Defalut Value to write into INT1_CFG register.
#define ACCEL_INT1_THS_DEFAULT            (0x10u)     ///< Value to write into INT1_THS register.
#define ACCEL_INT1_DUR_DEFAULT            (0x00u)     ///< Value to write into INT1_DUR register.

#define ACCEL_INT2_CFG_DEFAULT            (0x00u)     ///< Value to write into INT2_CFG register.
#define ACCEL_INT2_THS_DEFAULT            (0x7Fu)     ///< Value to write into INT2_THS register.
#define ACCEL_INT2_DUR_DEFAULT            (0x7Fu)     ///< Value to write into INT2_DUR register.

/* The address is a byte wide and left shifted by 8 bits so that the MSB of 16 bit data has the address */
/* and the LSB is a pad byte */
#define ACCEL_OUT_X_L                     (0x28u)        ///< Address of the register which has the LSB of the acceleration data on the X axis.
#define ACCEL_OUT_X_H                     (0x29u)        ///< Address of the register which has the MSB of the acceleration data on the X axis.
#define ACCEL_OUT_Y_L                     (0x2Au)        ///< Address of the register which has the LSB of the acceleration data on the Y axis.
#define ACCEL_OUT_Y_H                     (0x2Bu)        ///< Address of the register which has the MSB of the acceleration data on the Y axis.
#define ACCEL_OUT_Z_L                     (0x2Cu)        ///< Address of the register which has the LSB of the acceleration data on the Z axis.
#define ACCEL_OUT_Z_H                     (0x2Du)        ///< Address of the register which has the MSB of the acceleration data on the Z axis.

#define ACCEL_STATUS_REG                  (0x27u)        ///< Address of the status register.

#define ACCEL_WRITE_MASK                  (0x00u)        ///< Mask value used for Writing into accelerometer.
#define ACCEL_READ_MASK                   (0x80u)        ///< Mask value used for Reading from accelerometer.
#define ACCEL_DROP_MASK                   (0x15u)        ///< Mask value used to detect drop. Value corresponds to reading XL, YL and ZL value set in the INT_SRC register. When there is a drop all these values are set to '1'.

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
#pragma location=".sram"
OS_STK AccelTaskStack[ACCEL_TASK_STACK + MEMORY_FENCE_SIZE_DWORDS];     ///< Stack for accelrometer task.

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
/* None */

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
/* None */

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static OS_EVENT         *pAccelSem = NULL;          ///< Semaphore used to signal the ISR or timer expiry event.
static bool             AccelInitalized = false;    ///< Boolean to indicate the accelerometer initialization.
static bool             AccelEnabled = false;       ///< Boolean to check whether Accelerometer is enabled/disabled. By default disabled.
static ACCEL_CALLBACK   pAccelCallBack = NULL;      ///< Callback function to notify movement detection changes.
static uint32_t         NotifyDuration = 0;         ///< variable used to hold the duration of timer expiry.

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static ACCEL_STATUS Accel_ReadAxisData(AXISDATA *pAxisData, ACCEL_EVENT *AccelEvent);
static void         Accel_ReportAxisInfo(AXISDATA *pAxisData, ACCEL_EVENT AccelEvent);
static ACCEL_STATUS Accel_WriteReg(uint8_t RegAddr, uint8_t RegData);
static void         Accel_IntCallback(void);
static ACCEL_STATUS Accel_ReadReg(uint8_t  RegAddr, uint8_t *pRegData);

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Accelerometer task
 *
 * \details Once the Accelerometer is enabled, this task runs indefinitely, waiting
 *          on the semaphore event which is signalled from the interrupt from acceleromter
 *          or the expiry of the timer configured while enabling the accelerometer.
 *          Once the sempahore is signalled, this tasks reads the Axis data and reports it
 *          to the application through application registered callback function.
 *
 * \param   pArg - Pointer to task arguments (Required by micrium)
 *
 * \return  None
 *
 * ========================================================================== */
static void  Accel_Task(void *pArg)
{
    ACCEL_STATE  AccelState;                       // Used to hold the Accel_Task state.
    AXISDATA     AccelAxisData;                    // Contains the Axis Data read from accelerometer.
    uint8_t      Error;                            // Used for OS Errors.
    ACCEL_EVENT  AccelEvent;                       // Default value of Accel_Event being set to IDLE.

    AccelState = ACCEL_STATE_DISABLE;
    AccelEvent = ACCEL_EVENT_IDLE;

    while (true)
    {
        switch (AccelState)
        {
            case ACCEL_STATE_DISABLE:                        /* By default the Acceleromter Task will be in disabled state. */

                /* When we are in disable state we will wait forever on the pAccelSem, which will be posted from the Signia_AccelEnable */
                /* while enabling the Accelerometer.*/
                /* \todo whenever the task manager is implemented this wait forever can be configured with a timeout based on the task manager implementation*/
                OSSemPend(pAccelSem, OS_WAIT_FOREVER, &Error);
                AccelState = ACCEL_STATE_ENABLED;
                break;  /* Moving to ENABLED state. */

            case ACCEL_STATE_ENABLED:

                /* Wait for an interrupt from accelerometer or the duration set by the application 'NotifyDuration' to expire */
                /* and send the AxisData to the application using the callback registered. */
                /* If the NotifyDuration is '0' then only the interrupts from the accelerometer are notified. */
                OSSemPend(pAccelSem, NotifyDuration, &Error);

                if (ACCEL_STATUS_OK == Accel_ReadAxisData(&AccelAxisData, &AccelEvent))
                {
                    /* The below check is to know if the OSSemPend() returned because of timeout(NotifyDuration) configured , */
                    /* if it is timeout then the AccelEvent is set to ACCEL_EVENT_PERIODIC to inform the application that the source */
                    /* of the trigger is timer expiry */
                    if (OS_ERR_NONE != Error)
                    {
                        AccelEvent = ACCEL_EVENT_PERIODIC;
                    }

                    /* Report the AxisInfo data to the application. using the callback function registered by the application. */
                    Accel_ReportAxisInfo(&AccelAxisData, AccelEvent);
                }
                else
                {
                    Log(ERR, "Accel_Task: ReadAxisData Failed");
                }
                break;

            default:
                Log(ERR, "Accel_Task: Invalid Task State, Moving to default state");
                AccelState = ACCEL_STATE_DISABLE;
                break;
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Configures Accelerator for movement, drop detection
 *
 * \details This function configures the accelerometer registers Ctrl Reg1, Ctrl Reg2,
 *          Ctrl Reg3, Ctrl Reg4, Ctrl Reg5, Ctrl Reg6, INT1 and INT2 registers to
 *          detect the move and drop. INT1 is configured for Move and INT2 is configred
 *          Drop. Drop is detected by reading the value of INT2 Src register and
 *          comparing it with a Mask value ACCEL_DROP_MASK.
 *
 * \return  ACCEL_STATUS - status of accelerometer configuration for Move/Drop.
 *
 * ========================================================================== */
static ACCEL_STATUS Accel_Config(void)
{
    ACCEL_STATUS Status;           // Used to hold the return status.
    uint8_t ReadValue;
    Status = ACCEL_STATUS_ERROR;

    do
    {
        /* Reading the Device ID register value. */
        if (ACCEL_STATUS_OK != Accel_ReadReg(ACCEL_TEST_READ_WHOAMI, &ReadValue))
        {
            break;
        }

        /* these logs help to capture timeout or other errors*/
        if (WHOAMI_VALUE != ReadValue)
        {
            Log(DBG, "Accel_ReadAxisData: Reading Accelerometer ID failed : 0x%x", ReadValue);
            break;
        }

        /* Writing the values into Ctrl, Int1, Int2 registers for Move and Drop detection */

        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_CTRL_REG1, ACCEL_CTRL_REG1_DEFAULT))
        {
            break;
        }

        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_CTRL_REG2, ACCEL_CTRL_REG2_DEFAULT))
        {
            break;
        }
        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_CTRL_REG3, ACCEL_CTRL_REG3_DEFAULT))
        {
            break;
        }
        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_CTRL_REG4, ACCEL_CTRL_REG4_DEFAULT))
        {
            break;
        }
        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_CTRL_REG5, ACCEL_CTRL_REG5_DEFAULT))
        {
            break;
        }
        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_CTRL_REG6, ACCEL_CTRL_REG6_DEFAULT))
        {
            break;
        }
        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_INT1_THS, ACCEL_INT1_THS_DEFAULT))
        {
            break;
        }
        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_INT1_DUR, ACCEL_INT1_DUR_DEFAULT))
        {
            break;
        }
        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_INT1_CFG, ACCEL_INT1_CFG_DEFAULT))
        {
            break;
        }
        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_INT2_THS, ACCEL_INT2_THS_DEFAULT))
        {
            break;
        }
        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_INT2_DUR, ACCEL_INT2_DUR_DEFAULT))
        {
            break;
        }
        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_INT2_CFG, ACCEL_INT2_CFG_DEFAULT))
        {
            break;
        }

        Status = ACCEL_STATUS_OK;
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Read Register of Accelerometer
 *
 * \details This function reads the accelerometer register.
 *
 * \param   RegAddr - register address to read from.
 * \param   pRegData - value read from the register.
 *
 * \return  ACCEL_STATUS - status of read.
 *
 * ========================================================================== */
static ACCEL_STATUS Accel_ReadReg(uint8_t RegAddr, uint8_t *pRegData)
{
    ACCEL_STATUS    Status;             // Variable used to hold the return status.
    SPI_STATUS      SpiStatus;          // Variable used to hold the return status of L3 SPI calls.
    uint8_t         TxBuffer[2];        // Variable used to hold the command and register address for the read operation.
    uint8_t         RxBuffer[2];        // Variable used to hold the command and register address for the read operation.


    Status = ACCEL_STATUS_OK;

    /* Constructing the Tx Buffer to read the register with address regAddr. */
    TxBuffer[0] = 0;
    TxBuffer[1] = (ACCEL_READ_MASK | RegAddr);

    /* Sending the read command over SPI and reading the value of the register data in regData. */
    SpiStatus = L3_SpiTransfer(SPI_DEVICE_ACCELEROMETER, (uint8_t *)TxBuffer, 2, (uint8_t *)RxBuffer, 2);
    *pRegData = RxBuffer[0];

    if (SPI_STATUS_OK != SpiStatus)
    {
        Status = ACCEL_STATUS_ERROR;
        Log(ERR, "AccelReadReg: SPI Transfer Failed for RegAddr %x ", RegAddr);
    }
    // OSTimeDly(5);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Write into register of Accelerometer
 *
 * \details This function writes into the accelerometer register.
 *
 * \param   RegAddr - register address to write into.
 * \param   RegData - data to write into the register.
 *
 * \return  ACCEL_STATUS - status of write.
 *
 * ========================================================================== */
static ACCEL_STATUS Accel_WriteReg(uint8_t RegAddr, uint8_t RegData)
{
    ACCEL_STATUS Status;                         // Variable used to hold the return status.
    SPI_STATUS SpiStatus;                        // Variable used to hold the return status of L3 SPI calls.
    uint8_t TxBuffer[2];                           // Variable used to hold the command, register address and data to write .
    uint8_t RxBuffer[2];                           // Variable used to hold the response received from Accelerometer.

    Status = ACCEL_STATUS_OK;

    /* Constructing the Tx Buffer to Write the value regData into the register with address regAddr. */
    TxBuffer[0] = RegData;
    TxBuffer[1] = (ACCEL_WRITE_MASK | RegAddr);
    /* Sending the write command over SPI. */
    SpiStatus = L3_SpiTransfer(SPI_DEVICE_ACCELEROMETER, (uint8_t *)TxBuffer, 2, (uint8_t *)RxBuffer, 2);

    //SpiStatus |= Accel_ReadReg(RegAddr, RxBuffer);

    if (SPI_STATUS_OK != SpiStatus)
    {
        Status = ACCEL_STATUS_ERROR;
        Log(ERR, "AccelWriteReg: SPI Transfer Failed for regAddr %x", RegAddr);
    }

    //OSTimeDly(5);


    return Status;
}

/* ========================================================================== */
/**
 * \brief   Read Axis Data from Accelerometer
 *
 * \details This function reads the  Acceleration Data on X,Y and Z axes.
 *          This function also reads the value of the Status, INT1_SRC and INT2_SRC registers.
 *
 * \param   pAxisData  - Pointer to AxisData.
 * \param   AccelEvent - Accelerometer sense event.
 *
 * \return  ACCEL_STATUS - status of read.
 *
 * ========================================================================== */
static ACCEL_STATUS Accel_ReadAxisData(AXISDATA *pAxisData, ACCEL_EVENT *AccelEvent)
{
    ACCEL_STATUS Status;                             // Variable used to hold the return status.
    static uint8_t HData[ACCEL_BUF_SIZE];                   // Variable used to hold the upper part of  x-axis data.
    static uint8_t LData[ACCEL_BUF_SIZE];                   // Variable used to hold the lower part of  x-axis data.
    static uint8_t AccelStatusReg[ACCEL_BUF_SIZE];          // Variable used to read and hold the data from the status register.
    static uint8_t AccelInt1SrcReg[ACCEL_BUF_SIZE];         // Variable used to read the Int1Src register value.
    static uint8_t AccelInt2SrcReg[ACCEL_BUF_SIZE];         // Variable used to read the Int2Src register value.

    Status = ACCEL_STATUS_ERROR;

    do
    {

        /* Reading the Status register value. */
        if (ACCEL_STATUS_OK != Accel_ReadReg(ACCEL_STATUS_REG, AccelStatusReg))
        {
            break;
        }
        /* Reading the INT1_SRC register value.*/
        if (ACCEL_STATUS_OK != Accel_ReadReg(ACCEL_INT1_SRC, AccelInt1SrcReg))
        {
            break;
        }
        /* Reading the INT2_SRC register value. */
        if (ACCEL_STATUS_OK != Accel_ReadReg(ACCEL_INT2_SRC, AccelInt2SrcReg))
        {
            break;
        }
        /* Reading the Higher part of the  acceleration register on x-axis.*/
        if (ACCEL_STATUS_OK != Accel_ReadReg(ACCEL_OUT_X_H, HData))
        {
            break;
        }
        /* Reading the Lower part of the  acceleration register on x-axis. */
        if (ACCEL_STATUS_OK != Accel_ReadReg(ACCEL_OUT_X_L, LData))
        {
            break;
        }

        /* The axis values are in 2's complement, left-justified. The values are read from the sensor as two 8-bit values */
        /* (LSB, MSB) that are assembled together.*/
        pAxisData->x_axis = ((uint16_t)HData[0] << NUM_BITS_SHIFT) | (uint16_t)LData[0];

        /* Reading the Higher part of the    acceleration register on y-axis. */
        if (ACCEL_STATUS_OK != Accel_ReadReg(ACCEL_OUT_Y_H, HData))
        {
            break;
        }
        /* Reading the Lower part of the  acceleration register on y-axis. */
        if (ACCEL_STATUS_OK != Accel_ReadReg(ACCEL_OUT_Y_L, LData))
        {
            break;
        }

        pAxisData->y_axis = ((uint16_t)HData[0] << NUM_BITS_SHIFT) | (uint16_t)LData[0];

        /* Reading the Higher part of the    acceleration register on z-axis. */
        if (ACCEL_STATUS_OK != Accel_ReadReg(ACCEL_OUT_Z_H, HData))
        {
            break;
        }
        /* Reading the Lower part of the  acceleration register on z-axis. */
        if (ACCEL_STATUS_OK != Accel_ReadReg(ACCEL_OUT_Z_L, LData))
        {
            break;
        }

        pAxisData->z_axis = ((uint16_t)HData[0] << NUM_BITS_SHIFT) | (uint16_t)LData[0];

        pAxisData->Time = OSTimeGet();               ///\todo This needs to be replaced with RTC Timer later.

        //Log(DBG, "Accel Status: 0x%x",AccelInt2SrcReg[0]);


        /* Detection of Drop by comparing the value of the Int2Src register value with the DROP MASK.*/
        *AccelEvent = (ACCEL_DROP_MASK == AccelInt2SrcReg[0] & ACCEL_DROP_MASK) ? ACCEL_EVENT_DROP : ACCEL_EVENT_MOVING;

        /* All are 10 bit values and left aligned, hence shift right by 6 bit to get right algined values*/
        pAxisData->x_axis = pAxisData->x_axis >> ACCEL_VALUE_SHIFT;
        pAxisData->y_axis = pAxisData->y_axis >> ACCEL_VALUE_SHIFT;
        pAxisData->z_axis = pAxisData->z_axis >> ACCEL_VALUE_SHIFT;

        Status = ACCEL_STATUS_OK;
    } while (false);

    if (ACCEL_STATUS_OK != Status)
    {
        /* While returning with error, clear the Axis data buffer reads */
        memset(pAxisData, 0x00, sizeof(AXISDATA));
    }

    return Status;

}

/* ========================================================================== */
/**
 * \brief   Notify the application with the Axis Info of Accelerometer using
 *          the callback function registered by the application.
 *
 * \param   pAxisData - Pointer to AxisData.
 *
 * \return  none
 *
 * ========================================================================== */
static void Accel_ReportAxisInfo(AXISDATA *pAxisData, ACCEL_EVENT AccelEvent)
{
    ACCELINFO    AccelInfo;               // Variable used to hold the Accelerometer Information.

    memcpy(&AccelInfo.Data, pAxisData, sizeof(AXISDATA));
    AccelInfo.Event = AccelEvent;

    /* Notifying the caller with the AccelInfo using the callback registered. */
    if ((NULL != pAccelCallBack) && (AccelEnabled))
    {
        pAccelCallBack(&AccelInfo);
    }

    return;
}

/* ========================================================================== */
/**
 * \brief   Accelerometer Callback function called when a interrupt is received.
 *
 * \details This function responds to the accelerometer interrupt
 *          by setting the accelerometer semaphore pAccelSem.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ==========================================================================*/
static void Accel_IntCallback(void)
{
    OSSemPost(pAccelSem);
}

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Function to to initialize Accelerometer module.
 *
 * \details This function configures the SPI interface which is used for communicating
 *          to the Accelerometer. This function also configures the Ctrl,INT1,INT2 registers
 *          and creates the Acceleromter task.
 *
 * \return  ACCEL_STATUS - status
 *
 * ========================================================================== */
ACCEL_STATUS L4_AccelInit(void)
{
    ACCEL_STATUS Status = ACCEL_STATUS_OK;              // Variable used to hold the return status.
    uint8_t Error;                                      // Variable used for OS calls.

    do
    {
        /* Creating semaphore which is used to signal the interrupt from Accelerometer. */
        pAccelSem = SigSemCreate(0, "Accel-Sem", &Error);

        if (NULL == pAccelSem)
        {
            /* Semaphore not available, return error */
            Log(ERR, "AccelInit: SemCreate Failed");
            Status = ACCEL_STATUS_ERROR;
            break;
        }

        /* Configuring the Accelerometer to detect Move and Drop. */
        if (ACCEL_STATUS_OK != Accel_Config())
        {
            Status = ACCEL_STATUS_ERROR;
            Log(ERR, "AccelInit: Accel Config Failed");
            break;
        }

        /* Creating the Accelerometer task. */
        Error = SigTaskCreate(&Accel_Task,
                              NULL,
                              &AccelTaskStack[0],
                              TASK_PRIORITY_L4_ACCEL,
                              ACCEL_TASK_STACK,
                              "Accelerometer");

        if (OS_ERR_NONE != Error)
        {
            Status = ACCEL_STATUS_ERROR;
            Log(ERR, "AccelInit: Accel Task creation Failed");
            break;
        }

        AccelInitalized = true;

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Function to enable Accelerometer module and register the callback.
 *          This function must be called with ''Enable' value as true for the first time.
 *
 * \param   Enable   - Movement detection state. If ‘true’, the movement detection
 *                     module enabled, otherwise disabled. If the Enable is true but the pHandler is
 *                     NULL then the interrupts are disabled.
 * \param   Duration - Notify duration in Milliseconds. If duration is '0' then only state change
 *                     based on the itterupt is notified to the caller using callback. If the
 *                     value is greater than 0 then the caller is notified for every set duration
 *                     and also notified when there is an interrupt from the accelerometer.
 * \param   pHandler - Callback function to notify movement detection changes. If the Enable is true
 *                     but the pHandler is NULL then the interrupts are disabled.
 *
 * \return  ACCEL_STATUS - status of Enable.
 *
 * ========================================================================== */
ACCEL_STATUS Signia_AccelEnable(bool Enable, uint32_t Duration, ACCEL_CALLBACK pHandler)
{
    ACCEL_STATUS Status;                            // Variable used to hold the return status.
    GPIO_uP_PIN_INT_CONFIG_T AccelGpioIntConfig;    // Used to register the accelerometer ISR callback.

    Status = ACCEL_STATUS_OK;

    do
    {
        /* Disabling the Accelerometer based on the value of Enable/pHandler. */
        /* If the pHandler is NULL then there is no callback registered for receiving Axis data. */
        /* So disabling the interrupts. If the application wants to read Axis data on this state it can use L4_AccelGetAxisData. */
        if ((!Enable) || (NULL == pHandler))
        {
            AccelEnabled = false;
            NotifyDuration = 0;

            /* Disabling the Interrupt on Accelerometer */
            if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_CTRL_REG3, ACCEL_CTRL_REG3_DISABLE_IA1))
            {
                Status = ACCEL_STATUS_ERROR;
                Log(ERR, "AccelEnable: Disabling the Interrupt  Failed");
                break;
            }

            /* Disabling the Callback for the interrupt. */
            if (GPIO_STATUS_OK != L3_GpioCtrlDisableCallBack(GPIO_DUAL_ACCEL_INT))
            {
                Status = ACCEL_STATUS_ERROR;
                Log(ERR, "AccelEnable: GPIO Disable Callback Failed");
                break;
            }
        }

        pAccelCallBack = pHandler;

        /* Converting the duration value from milliseconds to ticks */
        NotifyDuration = (Duration * OS_TICKS_PER_SEC + (DEF_TIME_NBR_mS_PER_SEC - 1u)) / (DEF_TIME_NBR_mS_PER_SEC);

        if ((!AccelEnabled) && (NULL != pHandler))
        {
            /* Registering the  callback to handle the interrupt from Accelerometer */
            AccelGpioIntConfig.pxInterruptCallback = &Accel_IntCallback;
            AccelGpioIntConfig.eInterruptType = GPIO_uP_INT_RISING_EDGE;

            /* Enabling the Interrupt on Accelerometer */
            if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_CTRL_REG3, ACCEL_CTRL_REG3_I1_AOI1))
            {
                Status = ACCEL_STATUS_ERROR;
                Log(ERR, "AccelEnable: Enabling the Interrupt  Failed");
                break;
            }

            /* Enabling the callback to handle the interrupt. */
            if (GPIO_STATUS_OK != L3_GpioCtrlEnableCallBack(GPIO_DUAL_ACCEL_INT, &AccelGpioIntConfig))
            {
                Status = ACCEL_STATUS_ERROR;
                Log(ERR, "AccelEnable: GPIO Enable Callback Failed");
                break;
            }

            /* Enabling the Accelerometer and posting the sempahore to Acceleromter Task which move
            the accelerometer state to Enabled */
            AccelEnabled = true;
            OSSemPost(pAccelSem);
        }

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Function to read the Axis data from the  Accelerometer module.
 *
 * \param   *pAxisData   - Pointer to Axis data
 *
 * \return  ACCEL_STATUS - status of reading AxisData.
 *
 * ========================================================================== */
ACCEL_STATUS Signia_AccelGetAxisData(AXISDATA *pAxisData)
{
    ACCEL_STATUS   Status;        // Variable used to hold the return status.
    ACCEL_EVENT    AccelEvent;    // Variable defined to pass as argument to function reading  Axis data.

    Status = ACCEL_STATUS_OK;
    AccelEvent = ACCEL_EVENT_IDLE;

    do
    {
        if (NULL == pAxisData)
        {
            Status = ACCEL_STATUS_PARAM_ERROR;
            Log(ERR, "AccelGetAxisData: Invalid Parameter");
            break;
        }

        if ((ACCEL_STATE_DISABLE == AccelEnabled) || (!AccelInitalized))
        {
            Status = ACCEL_STATUS_ERROR;
            Log(ERR, "AccelGetAxisData: Accel not yet enabled/initialized");
            break;
        }

        if (ACCEL_STATUS_OK != Accel_ReadAxisData(pAxisData, &AccelEvent))
        {
            Status = ACCEL_STATUS_ERROR;
            Log(ERR, "AccelGetAxisData: Accel ReadAxisData Failed");
            break;
        }

    } while (false);

    return Status;

}

/* ========================================================================== */
/**
 * \brief   Function to set the threshold for Move and Drop detection.

 *
 * \param   MoveThreshold - Threshold for Move detection.
 * \param   DropThreshold - Threshold for Drop detection.
 *
 * \return  ACCEL_STATUS - status of setting threshold.
 *
 * ========================================================================== */
ACCEL_STATUS Signia_AccelSetThreshold(uint16_t MoveThreshold, uint16_t DropThreshold)
{
    ACCEL_STATUS Status;                   // Variable used to hold the return status.

    Status = ACCEL_STATUS_ERROR;

    do
    {
        if (!AccelInitalized)
        {
            Log(ERR, "AccelSetThreshold: Accel not initialized");
            break;
        }

        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_INT1_THS, MoveThreshold))
        {
            break;
        }

        if (ACCEL_STATUS_OK != Accel_WriteReg(ACCEL_INT2_THS, DropThreshold))
        {
            break;
        }

        Status = ACCEL_STATUS_OK;

    } while (false);

    return Status;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
