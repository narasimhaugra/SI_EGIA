#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_OneWire
 * \{
 * \brief   One Wire Bus primitives
 *
 * \details This module supplies functions necessary for manipulating devices on
 *          the One Wire bus(es). Functions are provided to achieve the following
 *          necessary One Wire bus operations:
 *              - Initialize interface hardware
 *              - Select One Wire Bus to operate on
 *              - Scan selected bus to detect all connected devices
 *              - Select a specified device on a specified bus
 *              - Authenticate a selected device
 *              - Transfer data to/from selected device
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_OneWireLink.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_I2c.h"
#include "L3_GpioCtrl.h"
#include "L3_OneWireLink.h"
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
#define LOG_GROUP_IDENTIFIER            (LOG_GROUP_1W)///< Log Group Identifier
#define DS2465_ADDRESS                  (0x18u)       /// < I2C address for One Wire master (DS2465)
#define DS2465_TXFR_TIMEOUT             (200u)        /// < I2C busy retry timeout
#define OW_IDLE_WAIT_RETRY_COUNT        (10u)         /// < Attempts to retry while waiting for bus IDLE
#define OW_LINK_WAIT_COUNT_10           (10u)         /// <  Delay Count - Wait for approximately 1 uS.
#define OW_LINK_WAIT_COUNT_100K         (100000u)         /// <  Delay Count - Wait for approximately 1 uS.

/* See DS2465 datasheet for details on registers & their values */

// Register addresses on the DS2465.
#define OWM_REG_FUNC                    (0x60u)      ///< DS2465 Command register
#define OWM_REG_MASTER_STATUS           (0x61u)      ///< DS2465 Status register
#define OWM_REG_READ_DATA               (0x62u)      ///< DS2465 One Wire Bus Read Data register
#define OWM_REG_MAC_READOUT             (0x63u)      ///< DS2465 Message Authentication Code result register

// Command functions for the DS2465.
// Called using OWM_REG_COMMAND.
#define OWM_FUNC_COPY_SCRATCHPAD         (0x5Au)      ///< DS2465 Copy Scratchpad command
#define OWM_FUNC_COMPUTE_S_SECRET        (0x4Bu)      ///< DS2465 Compute Slave Secret
#define OWM_FUNC_COMPUTE_S_AUTHEN_MAC    (0x3Cu)      ///< DS2465 Compute Slave authentication MAC
#define OWM_FUNC_COMPUTE_S_WRITE_MAC     (0x2Du)      ///< DS2465 Compute MAC for authenticated write access
#define OWM_FUNC_COMPUTE_NEXT_M_SECRET   (0x1Eu)      ///< DS2465 Compute new Master Secret from old one
#define OWM_FUNC_SET_PROTECTION          (0x0Fu)      ///< DS2465 Set Write Protection for memory/secret (not reversible)
#define OWM_FUNC_1WIRE_MASTER_RESET      (0xF0u)      ///< DS2465 Full reset of DS2465
#define OWM_FUNC_1WIRE_RESET_PULSE       (0xB4u)      ///< DS2465 Send reset pulse on OneWire bus, and check for presence pulse (in status reg)
#define OWM_FUNC_1WIRE_SINGLE_BIT        (0x87u)      ///< DS2465 Write single bit to OneWire bus and read bus state (in status reg)
#define OWM_FUNC_1WIRE_WRITE_BYTE        (0xA5u)      ///< DS2465 Write a byte to the OneWire bus
#define OWM_FUNC_1WIRE_READ_BYTE         (0x96u)      ///< DS2465 Read a byte from the OneWire bus (Result in Read Data register)
#define OWM_FUNC_1WIRE_TRIPLET           (0x78u)      ///< DS2465 Perform search ROM sequence (Two reads and one write)
#define OWM_FUNC_1WIRE_XMIT_BLOCK        (0x69u)      ///< DS2465 Transmit a block of data from scratchpad or MAC result to the OneWire bus.
#define OWM_FUNC_1WIRE_RECV_BLOCK        (0xE1u)      ///< DS2465 Receive a page MAC from secure OneWire memory device and store in DS2465 scratchpad memory

// DS2465 config bit masks
#define CONFIG_REG_MASK_APU              (0x01u)      ///< Active pullup enable
#define CONFIG_REG_MASK_PDN              (0x02u)      ///< One Wire power down (force reset)
#define CONFIG_REG_MASK_SPU              (0x04u)      ///< Strong pullup enable
#define CONFIG_REG_MASK_1WS              (0x08u)      ///< One Wire speed. (1 = Overdrive)

// DS2465 status bit masks
#define OWM_STATUS_REG_MASK_1WB         (0x01u)      ///< One Wire busy flag
#define OWM_STATUS_REG_MASK_PPD         (0x02u)      ///< Presence pulse detect
#define OWM_STATUS_REG_MASK_SD          (0x04u)      ///< One Wire short detect bit
#define OWM_STATUS_REG_MASK_LL          (0x08u)      ///< One Wire logic level
#define OWM_STATUS_REG_MASK_RST         (0x10u)      ///< One Wire reset performed
#define OWM_STATUS_REG_MASK_SBR         (0x20u)      ///< Single Bit command result
#define OWM_STATUS_REG_MASK_TSB         (0x40u)      ///< Triplet Second Bit (Second bit from One Wire Triplet command)
#define OWM_STATUS_REG_MASK_DIR         (0x80u)      ///< Branch Direction Taken (One Wire Triplet command)

#define ONEWIRE_COVIDIEN_MANUF_ID1      (0x60u)      ///< Covidien manufacturer's ID
#define ONEWIRE_COVIDIEN_MANUF_ID2      (0x00u)      ///< Covidien manufacturer's ID

// DS2465 configuration registers
#define OWM_REG_MST_CONFIG             (0x67u)      ///< One Wire master configuration register address
#define OWM_REG_tRSTL                  (0x68u)      ///< One Wire overdrive configuration tRSTL
#define OWM_REG_tMSP                   (0x69u)      ///< One Wire overdrive configuration tMSP
#define OWM_REG_tW0L                   (0x6Au)      ///< One Wire overdrive configuration tWOL
#define OWM_REG_tREC0                  (0x6Bu)      ///< One Wire configuration tREC0
#define OWM_REG_rWPU                   (0x6Cu)      ///< One Wire configuration Rwpu (pullup)
#define OWM_REG_tW1L                   (0x6Du)      ///< One Wire overdrive speed configuration tW1L
#define OWM_REG_MANUF_ID1              (0x71u)      ///< Mfr ID register 1 (Read only)
#define OWM_REG_MANUF_ID2              (0x72u)      ///< Mfr ID register 2 (Read only)
#define OWM_REG_PERSONALITY            (0x73u)      ///< Personality register (always reads 0x00)

// DS2465 configuration values
#define ONEWIRE_REGISTER_tW0L_VALUE     (0x33u)      ///< 6.5uS
#define ONEWIRE_REGISTER_tW1L_VALUE     (0x03u)      ///< 0.75uS


// Command functions for the DS2465.
#define OWM_CMD_COPY_SCRATCHPAD             (0x5Au)
#define OWM_CMD_COPY_COMPUTE_S_SECRET       (0x4Bu)
#define OWM_CMD_COPY_COMPUTE_S_AUTHEN_MAC   (0x3Cu)
#define OWM_CMD_COPY_COMPUTE_S_WRITE_MAC    (0x2Du)
#define OWM_CMD_COPY_COMPUTE_NEXT_M_SECRET  (0x1Eu)
#define OWM_CMD_COPY_SET_PROTECTION         (0x0Fu)
#define OWM_CMD_COPY_1WIRE_MASTER_RESET     (0xF0u)
#define OWM_CMD_COPY_1WIRE_RESET_PULSE      (0xB4u)
#define OWM_CMD_COPY_1WIRE_SINGLE_BIT       (0x87u)
#define OWM_CMD_COPY_1WIRE_WRITE_BYTE       (0xA5u)
#define OWM_CMD_COPY_1WIRE_READ_BYTE        (0x96u)
#define OWM_CMD_COPY_1WIRE_TRIPLET          (0x78u)
#define OWM_CMD_COPY_1WIRE_XMIT_BLOCK       (0x69u)
#define OWM_CMD_COPY_1WIRE_RECV_BLOCK       (0xE1u)

// DS2465 configuration values
#define OWM_REG_tW0L_VALUE                  (0x33u)      ///< 6.5uS
#define OWM_REG_tW1L_VALUE                  (0x03u)      ///< 0.75uS

#define OWM_CFG_SET_BIT(m)   (OwmConfig = ((OwmConfig) | (m)))
#define OWM_CFG_GET_BIT(m)   ((bool)((OwmConfig) & (m)))

#define OWM_CFG_CLR_BIT(m)   (OwmConfig = ((OwmConfig) & ~(m)))
#define OWM_CFG_BYTE()       ((OwmConfig & 0x0F) | (~OwmConfig << 4))

#define BYTES2_SEND_MASK_TRUE      (0x80u)
#define BYTES2_SEND_MASK_FALSE     (0x0u)

#define OWM_CMD_REGISTER_INDEX  (0u)
#define OWM_CMD_INDEX           (1u)
#define OWM_DATA_INDEX          (2u)
#define OWM_CMD_SIZE            (3u)
#define OWM_REG_SIZE            (1u)
#define OWM_DATA_SIZE           (2u)
#define OWM_SHORT_CHECK_VAL     (0xA4)

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static uint8_t OwmConfig;      /* 1-Wire master DS2465 configuration */
static bool PendingConfig;
static uint8_t  tempStatus; 
/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static ONEWIRE_STATUS OwErrorTranslate(I2C_STATUS I2cError);
static ONEWIRE_STATUS OwLinkWaitForIdle(bool *pDevicePresent);
static ONEWIRE_STATUS OwmRegRead(uint8_t RegAdr,  uint8_t *pRegData);
static ONEWIRE_STATUS OwmRegWrite(uint8_t RegAdr,  uint8_t RegData);
static ONEWIRE_STATUS OwmFunction(uint8_t Cmd,  uint8_t Data);
static void OwWaitByCount(uint32_t Count);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Waits for the One Wire busy flag to clear
 *
 * \details Checks One Wire master status until busy bit is clear or specified number
 *          of status reads has been performed.
 *
 * \note    pDevicePresent can be NULL if status is not required.
 *
 * \param   pDevicePresent - Device presence status updated here
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwLinkWaitForIdle(bool *pDevicePresent)
{
    ONEWIRE_STATUS Status;      // Return status
    uint16_t retryCount;        // Retry counter

    tempStatus = 0;
    retryCount = OW_IDLE_WAIT_RETRY_COUNT;

    Status = ONEWIRE_STATUS_BUSY;
    do
    {
        Status = OwmRegRead(OWM_REG_MASTER_STATUS, &tempStatus);

        if ((Status == ONEWIRE_STATUS_OK) && !(tempStatus & OWM_STATUS_REG_MASK_1WB))
        {
            break;      /* Bus is free - continue */
        }

        retryCount--;
        OSTimeDly(1);   /* The bus is busy, let's try after a bit */

    } while (retryCount > 0);

    
    if (0 == retryCount)
    {
        /* Timeout occurred, return error */
        Status = ONEWIRE_STATUS_BUSY;
    }
    else
    {
        /* Bus free, update the device presence */
        Status = ONEWIRE_STATUS_OK;
    }
    
    /* Update device presence status if expected by the caller */
    if (pDevicePresent)
    {      
        *pDevicePresent = (tempStatus & OWM_STATUS_REG_MASK_PPD) ? true : false;
       
        if (OWM_STATUS_REG_MASK_SD == tempStatus)
        {
            /* Bus short detected on Local Bus during Handle Initialization */
            FaultHandlerSetFault(ERR_PERMANENT_FAIL_ONEWIRE_SHORT,SET_ERROR);
        }
        else if ((false == *pDevicePresent) && (OWM_SHORT_CHECK_VAL == tempStatus))
        {
            /* Bus short detected, return error */
            Status = ONEWIRE_STATUS_BUS_ERROR;
        }
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Translate I2C error 1-Wire relevant errors
 *
 * \details I2C errors are low level 1-Wire controller specific errors and they
 *          mean the same thing. This function maps these errors relevant 1-Wire
 *          specific errors.
 *
 * \param   I2cError - I2C Error, Check enum 'I2C_STATUS' for details.
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
static ONEWIRE_STATUS OwErrorTranslate(I2C_STATUS I2cError)
{
    ONEWIRE_STATUS OwError;


    switch (I2cError)
    {
        case I2C_STATUS_BUSY:
            OwError = ONEWIRE_STATUS_TIMEOUT;
            break;

        case I2C_STATUS_FAIL:
            OwError = ONEWIRE_STATUS_ERROR;
            break;

        case I2C_STATUS_FAIL_CONFIG:
            OwError = ONEWIRE_STATUS_ERROR;
            break;

        case I2C_STATUS_FAIL_INVALID_PARAM:
            OwError = ONEWIRE_STATUS_PARAM_ERROR;
            break;

        case I2C_STATUS_FAIL_NO_RESPONSE:
            OwError = ONEWIRE_STATUS_TIMEOUT;
            break;

        case I2C_STATUS_FAIL_TIMEOUT:
            OwError = ONEWIRE_STATUS_TIMEOUT;
            break;

        case I2C_STATUS_SUCCESS:
            OwError = ONEWIRE_STATUS_OK;
            break;

        default:
            OwError = ONEWIRE_STATUS_ERROR;
            break;
    }

    return OwError;
}

/* ========================================================================== */
/**
 * \brief   Delay utility function
 *
 * \details This is a no-os delay function used for duration lesser than tick.
 *
 * \param   Count
 *
 * \return  None
 *
 * ==========================================================================*/
static void OwWaitByCount(uint32_t Count)
{
    while(Count) { Count--; }
}

/******************************************************************************/
/*                             Global Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Initialize 1-Wire link layer
 *
 * \details The link layer implements routines to communicate with the DS2465.
 *
 * \param   < None >
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwLinkInit(void)
{
    I2CControl      Config;         /* I2C configuration parameter block */
    ONEWIRE_STATUS  Status;         /* Operation status */
    I2C_STATUS CommStatus;
    uint8_t mfr_data[2];
    Status = ONEWIRE_STATUS_ERROR;

    PendingConfig = true;       /* Force configuration */

    /* Configure I2C for One Wire */
    Config.Clock = I2C_CLOCK_312K;
    Config.State = I2C_STATE_ENA;
    Config.AddrMode = I2C_ADDR_MODE_7BIT;
    Config.Device = DS2465_ADDRESS;
    Config.Timeout = DS2465_TXFR_TIMEOUT;

    CommStatus = L3_I2cConfig(&Config);
    do 
    {       
        if (I2C_STATUS_SUCCESS != CommStatus)
        {
            break;
        }

        Status = OwLinkSleep(false);
        if (ONEWIRE_STATUS_OK != Status)
        {
            break;
        }
        Status = OwmRegRead(OWM_REG_MANUF_ID1, &mfr_data[0]);
        if (ONEWIRE_STATUS_OK != Status)
        {
            break;
        }
        Status = OwmRegRead(OWM_REG_MANUF_ID2, &mfr_data[1]);
        if (ONEWIRE_STATUS_OK != Status)
        {
            break;
        }
        TM_Hook(HOOK_ONEWIREMASTERFAIL, (void *)(&mfr_data));
        if ((ONEWIRE_COVIDIEN_MANUF_ID1 != mfr_data[0]) ||
           (ONEWIRE_COVIDIEN_MANUF_ID2 != mfr_data[1]))
        {
            Status = ONEWIRE_STATUS_ERROR;
            Log(ERR, "OwTask: Ow MasterID not matched");
            FaultHandlerSetFault(PERMFAIL_ONEWIREMASTER_COMMFAIL, SET_ERROR);
        }
        else
        {
            Log(ERR, "OwTask: Ow MasterID Matched");
        }
    }while (false);   

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Reset the selected One Wire bus and return the presence of any device.
 *
 * \details A command is sent to the DS2465 to issue a reset pulse on the One Wire
 *          bus. This function then checks the status register of the DS2465 to see
 *          if a presence pulse was detected after the reset pulse.
 *
 * \note    pDevicePresent can be NULL if device status is not required.
 *
 * \param   pDevicePresent - Device presence status updated here
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwLinkReset(bool *pDevicePresent)
{
    ONEWIRE_STATUS  Status;
    ONEWIRE_STATUS  TempStatus;

    Status = OwLinkWaitForIdle(NULL);

    /* Don't issue a reset if the bus is busy doing something */
    if (ONEWIRE_STATUS_BUSY != Status)
    {
        /* Send bus reset pulse - Even if the bus is short so that we come back and
        check to see if the short is removed */

        TempStatus = OwmFunction(OWM_CMD_COPY_1WIRE_RESET_PULSE, 0);

        if (ONEWIRE_STATUS_OK == TempStatus)
        {
            TempStatus = OwLinkWaitForIdle(pDevicePresent);
        }

        /* Retain the first status if it was a short as it will take some time to
        detect the short again after the reset */

        if (ONEWIRE_STATUS_BUS_ERROR != Status)
        {
            Status = TempStatus;
        }
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Write a single bit of data to the One Wire bus.
 *
 * \details A single bit is sent over the One Wire bus via the DS2465 1-Wire
 *          Single Bit command and compares the return value of the SBR bit in the
 *          DS2465 status register. The SBR register reflects the actual state of
 *          the bus rather than the intended value of the bit.
 *
 * \param   BitValue - Value to send over the bus
 * \param   *pReturnValue - Value received from the bus
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwLinkWriteBit(bool BitValue, bool *pReturnValue)
{
    ONEWIRE_STATUS  Status;         // Return status value
    uint8_t masterStatus;   // Master status returned
    uint8_t ByteToSend;

    masterStatus = 0;
    Status = ONEWIRE_STATUS_BUSY;                 // Default error

    OwLinkWaitForIdle(NULL);

    ByteToSend = (BitValue) ? BYTES2_SEND_MASK_TRUE : BYTES2_SEND_MASK_FALSE;       // Force bit_value to either 0 or 0x80.

    Status = OwmFunction(OWM_FUNC_1WIRE_SINGLE_BIT, ByteToSend);

    if (ONEWIRE_STATUS_OK == Status)
    {
        Status = OwmRegRead(OWM_REG_MASTER_STATUS, &masterStatus);
    }

    /* Update the status if requestor is expcting it */
    if (NULL != pReturnValue)
    {
        *pReturnValue = (masterStatus & OWM_STATUS_REG_MASK_SBR) ? true : false;
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Write a byte of data to the One Wire bus.
 *
 * \details Sends a byte of data to the DS2465 for transmission to the One Wire bus.
 *
 * \note    The One Wire bus must be active, and the desired device already selected.
 *
 * \param   Data - Data to send
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwLinkWriteByte(uint8_t Data)
{
    ONEWIRE_STATUS  Status;
    uint16_t Wait;

    Status = ONEWIRE_STATUS_BUSY;                 // Default error

    if (ONEWIRE_STATUS_OK == OwLinkWaitForIdle(NULL))
    {
      /* Consecutive byte read need some wait time depending upon bus speed */
        Wait = OWM_CFG_GET_BIT(CONFIG_REG_MASK_1WS)? OW_LINK_WAIT_COUNT_10 : OW_LINK_WAIT_COUNT_100K;

        OwWaitByCount(Wait);

        Status = OwmFunction(OWM_FUNC_1WIRE_WRITE_BYTE, Data);
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Read a byte of data from the One Wire bus.
 *
 * \details Requests the DS2465 to read a byte of data from the One Wire bus,
 *          and returns the byte read.
 *
 * \note    The One Wire bus must be active, and the desired device already selected.
 *
 * \param   pData - Pointer to byte to store data read
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwLinkReadByte(uint8_t *pData)
{
    ONEWIRE_STATUS  Status;
    uint16_t Wait;

    Status = ONEWIRE_STATUS_BUSY;         // Default error

    if (ONEWIRE_STATUS_OK == OwLinkWaitForIdle(NULL))
    {
        Status = OwmFunction(OWM_FUNC_1WIRE_READ_BYTE, 0);

        /* Consecutive byte read need some wait time depending upon bus speed */
        Wait = OWM_CFG_GET_BIT(CONFIG_REG_MASK_1WS)? OW_LINK_WAIT_COUNT_10 : OW_LINK_WAIT_COUNT_100K;

        OwWaitByCount(Wait);

        if (ONEWIRE_STATUS_OK == Status)
        {
            Status = OwmRegRead(OWM_REG_READ_DATA, pData);
        }
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Read a register from the One Wire bus master
 *
 * \details Issues an I2C read request to read the specified register from the DS2465.
 *
 * \param   RegAdr  - Address of register to read
 * \param   pRegData - Pointer to byte to store return data
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwmRegRead(uint8_t RegAdr,  uint8_t *pRegData)
{
    I2CDataPacket packet;               // I2C transfer packet
    I2C_STATUS Status;

    packet.Address = DS2465_ADDRESS;    // Address of One Wire bus master
    packet.pReg = &RegAdr;              // Register address to read
    packet.nRegSize = 1;                // Size of register address
    packet.pData = pRegData;            // Pointer to result
    packet.nDataSize = 1;               // Result size

    Status = L3_I2cRead(&packet);

    return OwErrorTranslate(Status);
}

/* ========================================================================== */
/**
 * \brief   Write data to a register of the One Wire bus master
 *
 * \details Issues an I2C read request to read the specified register from the DS2465.
 *
 * \param   RegAdr  - Address of register to write
 * \param   regData - Data to store to register
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwmRegWrite(uint8_t RegAdr, uint8_t RegData)
{
    I2CDataPacket packet;               // I2C transfer packet
    I2C_STATUS Status;

    packet.Address = DS2465_ADDRESS;    // Address of One Wire bus master
    packet.pReg = &RegAdr;              // Register address to read
    packet.nRegSize = 1;                // Size of register address
    packet.pData = &RegData;            // Pointer to result
    packet.nDataSize = 1;               // Result size

    Status = L3_I2cWrite(&packet);

    return OwErrorTranslate(Status);
}

/* ========================================================================== */
/**
 * \brief   Write a command and data to the One Wire bus master (DS2465)
 *
 * \details This is a special form of OW_MasterRegWrite, that writes a command
 *          to the command register of the DS2465, followed by a byte of data to
 *          the same register. In the case of the One Wire Read Byte command, no
 *          data is written.
 *
 * \note    The One Wire Read Byte command does not have any data.
 *
 * \param   cmd     - Command to be written
 * \param   data    - Data to be written
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwmFunction(uint8_t Cmd, uint8_t Data)
{
    I2CDataPacket packet;               // I2C transfer packet
    uint8_t       buf[3];               // Data to write. (Register, command, and data)
    I2C_STATUS    Status;

    buf[0] = OWM_REG_FUNC;              // Register to write to
    buf[1] = Cmd;                       // Command to write
    buf[2] = Data;                      // Data (if any) to write

    packet.Address = DS2465_ADDRESS;    // Address of One Wire bus master
    packet.pReg = &buf[0];              // Register address to write to
    packet.nRegSize = 1;                // Size of register address
    packet.pData = &buf[1];             // Pointer to data to write
    packet.nDataSize = 2;               // Data size (command & data)

    switch (Cmd)
    {
        case OWM_FUNC_1WIRE_MASTER_RESET:
        case OWM_FUNC_1WIRE_RESET_PULSE:
        case OWM_FUNC_1WIRE_READ_BYTE:
            packet.nDataSize = 1;           /*Ignore data part */
            break;                          /* Data is included */
        case OWM_FUNC_1WIRE_SINGLE_BIT:
        case OWM_FUNC_1WIRE_WRITE_BYTE:
        case OWM_FUNC_COPY_SCRATCHPAD:
        case OWM_FUNC_COMPUTE_S_SECRET:
        case OWM_FUNC_COMPUTE_S_AUTHEN_MAC:
        case OWM_FUNC_COMPUTE_S_WRITE_MAC:
        case OWM_FUNC_COMPUTE_NEXT_M_SECRET:
        case OWM_FUNC_SET_PROTECTION:
        case OWM_FUNC_1WIRE_TRIPLET:
        case OWM_FUNC_1WIRE_XMIT_BLOCK:
        case OWM_FUNC_1WIRE_RECV_BLOCK:
            break;

        default:
            break;
    }

    Status = L3_I2cWrite(&packet);

    return OwErrorTranslate(Status);
}

/* ========================================================================== */
/**
 * \brief   Set 1-Wire bus speed
 *
 * \details Request the DS2465 to set the specified bus speed
 *
 * \param   Speed - Bus speed
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwLinkSetSpeed(ONEWIRE_SPEED Speed)
{
    ONEWIRE_STATUS Status;

    do
    {
        /* Validate speed specified */
        if (Speed >= ONEWIRE_SPEED_COUNT)
        {
            /* Invalid speed */
            Status = ONEWIRE_STATUS_PARAM_ERROR;
            break;
        }

        Status = ONEWIRE_STATUS_OK;

        /* Do we already have overspeed */
        if ((OWM_CFG_GET_BIT(CONFIG_REG_MASK_1WS)) &&  (ONEWIRE_SPEED_OD == Speed))
        {
            break;
        }

        /* Do we already have standard speed */
        if ((0 == OWM_CFG_GET_BIT(CONFIG_REG_MASK_1WS)) &&  (ONEWIRE_SPEED_STD == Speed))
        {
            break;
        }

        /* Speed has changed, update the configuration and mark for change */
        PendingConfig = true;

        if (ONEWIRE_SPEED_OD == Speed)
        {
            OWM_CFG_SET_BIT(CONFIG_REG_MASK_1WS);
        }
        else
        {
            OWM_CFG_CLR_BIT(CONFIG_REG_MASK_1WS);
        }

    }while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Set 1-Wire bus speed
 *
 * \details Request the DS2465 to set the specified bus pullup
 *
 * \param   Pullup - Desired bus pullup
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwLinkSetPullup(OW_NET_BUS_PULLUP Pullup)
{
    ONEWIRE_STATUS Status;

    Status = ONEWIRE_STATUS_OK;

    switch (Pullup)
    {
        case OW_NET_BUS_PULLUP_ACTIVE:
            OWM_CFG_SET_BIT(CONFIG_REG_MASK_APU);
            OWM_CFG_CLR_BIT(CONFIG_REG_MASK_SPU);
            break;

        case OW_NET_BUS_PULLUP_STRONG:
            OWM_CFG_SET_BIT(CONFIG_REG_MASK_SPU);
            OWM_CFG_CLR_BIT(CONFIG_REG_MASK_APU);
            break;

        case OW_NET_BUS_PULLUP_PASSIVE:
            OWM_CFG_CLR_BIT(CONFIG_REG_MASK_SPU);
            OWM_CFG_CLR_BIT(CONFIG_REG_MASK_APU);
            break;

        default:
            /*Do nothing */
            break;
    }

    /* Mark configuration change. Changes to the bus will be done when needed */
    PendingConfig = true;

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Update 1-Wire master config
 *
 * \details Update 1-Wire master config from the stored configuration
 *
 * \param   < None >
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwLinkUpdateConfig(void)
{
    ONEWIRE_STATUS Status;

    Status = ONEWIRE_STATUS_OK;
    if (PendingConfig)
    {
        Status = OwmRegWrite(OWM_REG_MST_CONFIG, OWM_CFG_BYTE());   // Set high speed bit

        if (ONEWIRE_STATUS_OK == Status)
        {
            PendingConfig = false;
        }
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Control DS2465 sleep mode.
 *
 * \details Upon awaking from sleep, this function also initializes the DS2465.
 *
 * \param   Sleep - true to put 2465 in sleep mode, false to wake up / initialize
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwLinkSleep(bool Sleep)
{
    ONEWIRE_STATUS    Status;           // Operation status
    uint8_t results;                    // Results from register read

    Status = ONEWIRE_STATUS_ERROR;      // Default to error

    if (Sleep)
    {
        OWM_CFG_SET_BIT(CONFIG_REG_MASK_PDN);         /* Clear to power down */
        Status = OwmRegWrite(OWM_REG_MST_CONFIG, OWM_CFG_BYTE());
        L3_GpioCtrlClearSignal(GPIO_SLP_1Wn);       /* Put the DS2465 to sleep */
    }
    else
    {
        /* Waking up, Issue reset if all good */

        L3_GpioCtrlSetSignal(GPIO_SLP_1Wn);       /* Wake up the DS2465 and initialize it*/
        OSTimeDly(2);                               /* \todo: failsafe delay, but code works without delay */

        OWM_CFG_CLR_BIT(CONFIG_REG_MASK_PDN);       /* Clear power down bit */
        OWM_CFG_SET_BIT(CONFIG_REG_MASK_APU);       /* Enable active pullup */
        OWM_CFG_SET_BIT(CONFIG_REG_MASK_1WS);       /* Set standard speed by default */

        Status = OwmRegWrite(OWM_REG_MST_CONFIG, OWM_CFG_BYTE());
        Status = OwmRegRead(OWM_REG_MST_CONFIG, &results);

        OSTimeDly(2);                               /* \todo: failsafe delay, but code works without delay */

        Status = OwmRegWrite(OWM_REG_tW1L, ONEWIRE_REGISTER_tW1L_VALUE);    /* Set overdrive bydefault */

        OSTimeDly(2);                               /* \todo: failsafe delay, but code works without delay */

        if (ONEWIRE_STATUS_OK == Status)
        {
            OwLinkReset(NULL);                      /* Issue reset, Don't care about device presence */
        }
        else
        {
            Log(ERR, "FATAL ERROR: ONEWIRE_REGISTER_tW1L failed!");
        }
    }
    return Status;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif


