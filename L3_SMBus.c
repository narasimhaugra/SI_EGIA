#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_SMBus
 * \{
 * \brief   This module contains support functions for communicating to a
 *          Smart Device at L3 over the SMBus.
 *
 * \details Routines for interacting with a Smart Device via the SMBus as an
 *          I2C port are implemented in this file.
 *
 * \note    This file includes functions for sending requests for data, status
 *          and Smart device specific parameters.
 *          There are 15 protocols to support the SMBus. They are:
 *             - Quick Command
 *             - Send Byte
 *             - Receive Byte
 *             - Write Byte
 *             - Write Word
 *             - Read Byte
 *             - Read Word
 *             - Process Call
 *             - Block Read
 *             - Block Write
 *             - Block Write-Block Read Process Call
 *             - Write 32
 *             - Read 32
 *             - Write 64
 *             - Read 64
 *
 * \sa      http://smbus.org/specs/SMBus_3_0_20141220.pdf
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_SMBus.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include <Common.h>
#include "L3_I2c.h"
#include "L3_GpioCtrl.h"
#include "L3_SMBus.h"
#include "Logger.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER     (LOG_GROUP_BATTERY)

#define SMBUS_MUTEX_TIMEOUT      (100u)           ///< SMBus Mutex Timeout

#define I2C_REGSIZE_1            (1u)             ///< I2C Command Data size
#define TRANSFER_WRITE           true             ///< Flag I2C Write transfer
#define TRANSFER_READ            false            ///< Flag I2C Read transfer

#define SMBUSBUFSIZE                  (40u)      ///< Local buffer for SMBUS size

#define SMBUS_ADDR_BYTE(ADDR, Dir)       (((ADDR) << 1u) | ((!(Dir)) ? 1u : 0u))

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static OS_EVENT *pMutexSMBus = NULL;  /* I2C access mutex, synchronizes L2 I2C calls */

static bool SMBus_PECEnable;
static uint8_t SMBusBuf[SMBUSBUFSIZE];
/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void SetSMBusPortPcr(void);
static SMBUS_STATUS TransferI2CPacket(uint16_t addr, uint8_t cmd, uint8_t size, uint8_t *pOpData, uint8_t opDataSize, bool xferWrite);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Sets I2C Interface Pins for Battery SMBus.
 *
 * \return  None
 *
 * ========================================================================== */
static void SetSMBusPortPcr(void)
{
  uint8_t u8OsError;

    pMutexSMBus = SigMutexCreate("L3-SMBus", &u8OsError);

    if (NULL != pMutexSMBus)
    {
       /*  Mutex lock */
       OSMutexPend(pMutexSMBus, SMBUS_MUTEX_TIMEOUT, &u8OsError);
       if (OS_ERR_NONE == u8OsError)
       {
         PORTB_PCR2 |= ((INT32U)PORT_PCR_ODE_MASK);
         PORTB_PCR3 |= ((INT32U)PORT_PCR_ODE_MASK);
       }
    }
    OSMutexPost(pMutexSMBus);    /* Mutex release */
}

/* ========================================================================== */
/**
 * \brief   Reads or Writes a data packet to the I2C Interface.
 *
 * \return  None
 *
 * ========================================================================== */
static SMBUS_STATUS TransferI2CPacket(uint16_t addr, uint8_t cmd, uint8_t size, uint8_t *pOpData, uint8_t opDataSize, bool xferWrite)
{
    SMBUS_STATUS eStatusReturn;  /* SMBus Status Return */
    I2CDataPacket DataPacket;    /* Argument block for I2C transmission */
    uint8_t Count;
    uint8_t Address;
    uint8_t Crc;
    bool PECMode;

    PECMode = L3_SMBus_GetPECFlag();

    if (GPIO_STATUS_OK == L3_GpioCtrlSetSignal(GPIO_EN_SMB))
    {
       DataPacket.Address = addr;
       DataPacket.pReg = &cmd;
       DataPacket.nRegSize = size;
       DataPacket.pData = SMBusBuf;
       DataPacket.nDataSize = opDataSize;
       if (PECMode)
       {
          DataPacket.nDataSize++;
       }
       if (TRANSFER_WRITE == xferWrite)
       {
           memcpy(SMBusBuf, pOpData, opDataSize);
           if (PECMode)
           {

              Address = SMBUS_ADDR_BYTE(DataPacket.Address, xferWrite);
              Crc = 0;
              Crc = DoSMBusCRC8(Crc,Address);
              Crc = DoSMBusCRC8(Crc,cmd);
              Count= 0;
              do
              {
                  Crc = DoSMBusCRC8(Crc,SMBusBuf[Count]);
                  Count++;
              } while ( Count < opDataSize);
              SMBusBuf[opDataSize] = Crc;
          }

          eStatusReturn = (I2C_STATUS_SUCCESS != L3_I2cWrite(&DataPacket) ? SMBUS_WRITE_ERROR : SMBUS_NO_ERROR);
       }
       else
       {

          eStatusReturn = (I2C_STATUS_SUCCESS != L3_I2cRead(&DataPacket) ? SMBUS_READ_ERROR : SMBUS_NO_ERROR);
          if (PECMode)
          {
              //validate CRC. if error then update return status to fault
              Address = SMBUS_ADDR_BYTE(DataPacket.Address, TRANSFER_WRITE);
              Crc = 0;
              Crc = DoSMBusCRC8(Crc,Address);
              Crc = DoSMBusCRC8(Crc,cmd);
              Address = SMBUS_ADDR_BYTE(DataPacket.Address, TRANSFER_READ);
              Crc = DoSMBusCRC8(Crc,Address);
              Count= 0;
              do
              {
                  Crc = DoSMBusCRC8(Crc,SMBusBuf[Count]);
                  Count++;
              } while ( Count < opDataSize);

              if (Crc != SMBusBuf[opDataSize])
              {
                  eStatusReturn = SMBUS_READ_ERROR;
              }
          }
          memcpy(pOpData, SMBusBuf, opDataSize);

       }
    }
    else
    {
       eStatusReturn = SMBUS_BUSY_ERROR;
    }

    L3_GpioCtrlClearSignal(GPIO_EN_SMB);    /* Clear to disable SMBus */

    return (eStatusReturn);
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Configures the SMBus I2C interface.
 *
 * \details Used to configure the SMBus I2C interface.
 *
 * \param   devAddr  - Smart Device address
 * \param   timeOut  - Bus timeout
 *
 * \return  SMBUS_STATUS - SMBus Status
 * \retval      See L3_SMBus.h
 *
 * ========================================================================== */
SMBUS_STATUS L3_SMBusInit(uint16_t devAddr, uint16_t timeOut)
{
  SMBUS_STATUS eStatusReturn;  /* SMBus Status Return */
  I2CControl   I2cConfig;      /* I2C configuration parameter block */

    ///< \\todo: Port Pins need to be set for open drain, need an L3_I2C function for this
    SetSMBusPortPcr();

    /* Disable the battery analog switch until we need to talk to the BQ chip */
    /* This keeps the capacitance down. */
    if (GPIO_STATUS_OK == L3_GpioCtrlClearSignal(GPIO_EN_SMB))
    {
          /* Configure I2C for Battery */
       I2cConfig.AddrMode = I2C_ADDR_MODE_7BIT;
       I2cConfig.Device   = devAddr;
       I2cConfig.Clock    = I2C_CLOCK_78K;
       I2cConfig.State    = I2C_STATE_ENA;
       I2cConfig.Timeout  = timeOut;

       eStatusReturn = (I2C_STATUS_SUCCESS != L3_I2cConfig(&I2cConfig) ? SMBUS_CONFIG_ERROR : SMBUS_NO_ERROR);
    }
    else
    {
       eStatusReturn = SMBUS_BUSY_ERROR;
    }

    SMBus_PECEnable = false;

    return (eStatusReturn);
}

/* ========================================================================== */
/**
 * \brief   Send a read byte request to the Smart Device via the SMBus.
 *
 * \details Used for SMBus Quick Access commands which require sending a
 *          command with data.
 *
 * \param   devAddr    - Smart Device address
 * \param   devCmd     - SMBus Host to SMBus Device request
 * \param   devCmdSize - SMBus Host to SMBus Device request size
 * \param   pOpData    - Pointer to operand data to send with the request
 *
 * \return  SMBUS_STATUS - SMBus Status
 * \retval      See L3_SMBus.h
 *
 * ========================================================================== */
SMBUS_STATUS L3_SMBusQuickCommand(uint16_t devAddr, uint8_t devCmd, uint8_t devCmdSize, uint8_t *pOpData)
{
    return (TransferI2CPacket(devAddr, devCmd, devCmdSize, pOpData,
                              SMBUS_WORD_NUMBYTES, TRANSFER_WRITE));
}

/* ========================================================================== */
/**
 * \brief   Send a read byte request to the Smart Device via the SMBus.
 *
 * \details Used for SMBus commands which require reading a byte of data.
 *
 * \param   devAddr  - Smart Device address
 * \param   devCmd   - SMBus Host to SMBus Device request
 * \param   pOpData  - Pointer to operand data to send with the request
 *
 * \return  SMBUS_STATUS - SMBus Status
 * \retval      See L3_SMBus.h
 *
 * ========================================================================== */
SMBUS_STATUS L3_SMBusReadByte(uint16_t devAddr, uint8_t devCmd, uint8_t *pOpData)
{
    return (TransferI2CPacket(devAddr, devCmd, I2C_REGSIZE_1, pOpData,
                              SMBUS_BYTE_NUMBYTES, TRANSFER_READ));
}

/* ========================================================================== */
/**
 * \brief   Send a write byte request to the Smart Device via the SMBus.
 *
 * \details Used for SMBus commands which require writing a byte of data.
 *
 * \param   devAddr - Smart Device address
 * \param   devCmd  - SMBus Host to SMBus Device request
 * \param   opData  - Operand data to send with the request
 *
 * \return  SMBUS_STATUS - SMBus Status
 * \retval      See L3_SMBus.h
 *
 * ========================================================================== */
SMBUS_STATUS L3_SMBusWriteByte(uint16_t devAddr, uint8_t devCmd, uint8_t opData)
{
    return (TransferI2CPacket(devAddr, devCmd, I2C_REGSIZE_1, (uint8_t *)&opData,
                              SMBUS_BYTE_NUMBYTES, TRANSFER_WRITE));
}

/* ========================================================================== */
/**
 * \brief   Send a read word request to the Smart Device via the SMBus.
 *
 * \details Used for SMBus commands which require reading a word of data.
 *
 * \param   devAddr  - Smart Device address
 * \param   devCmd   - SMBus Host to SMBus Device request
 * \param   pOpData  - Pointer to operand data to send with the request
 *
 * \return  SMBUS_STATUS - SMBus Status
 * \retval      See L3_SMBus.h
 *
 * ========================================================================== */
SMBUS_STATUS L3_SMBusReadWord(uint16_t devAddr, uint8_t devCmd, uint8_t *pOpData)
{
    return (TransferI2CPacket(devAddr, devCmd, I2C_REGSIZE_1, pOpData,
                              SMBUS_WORD_NUMBYTES, TRANSFER_READ));
}

/* ========================================================================== */
/**
 * \brief   Send a write word request to the Smart Device via the SMBus.
 *
 * \details Used for SMBus commands which require writing a word of data.
 *
 * \param   devAddr - Smart Device address
 * \param   devCmd  - SMBus Host to SMBus Device request
 * \param   opData  - Operand data to send with the request
 *
 * \return  SMBUS_STATUS - SMBus Status
 * \retval      See L3_SMBus.h
 *
 * ========================================================================== */
SMBUS_STATUS L3_SMBusWriteWord(uint16_t devAddr, uint8_t devCmd, uint16_t opData)
{
    return (TransferI2CPacket(devAddr, devCmd, I2C_REGSIZE_1, (uint8_t *)&opData,
                              SMBUS_WORD_NUMBYTES, TRANSFER_WRITE));
}

/* ========================================================================== */
/**
 * \brief   Send a read 32 request to the Smart Device via the SMBus.
 *
 * \details Used for SMBus commands which require reading 32 bits of data.
 *
 * \param   devAddr  - Smart Device address
 * \param   devCmd   - SMBus Host to SMBus Device request
 * \param   pOpData  - Pointer to operand data to send with the request
 *
 * \return  SMBUS_STATUS - SMBus Status
 * \retval      See L3_SMBus.h
 *
 * ========================================================================== */
SMBUS_STATUS L3_SMBusRead32(uint16_t devAddr, uint8_t devCmd, uint8_t *pOpData)
{
    return (TransferI2CPacket(devAddr, devCmd, I2C_REGSIZE_1, pOpData,
                              SMBUS_RDWR32_NUMBYTES, TRANSFER_READ));
}

/* ========================================================================== */
/**
 * \brief   Send a write 32 request to the Smart Device via the SMBus.
 *
 * \details Used for SMBus commands which require writing 32 bits of data.
 *
 * \param   devAddr - Smart Device address
 * \param   devCmd  - SMBus Host to SMBus Device request
 * \param   opData  - Operand data to send with the request
 *
 * \return  SMBUS_STATUS - SMBus Status
 * \retval      See L3_SMBus.h
 *
 * ========================================================================== */
SMBUS_STATUS L3_SMBusWrite32(uint16_t devAddr, uint8_t devCmd, uint32_t opData)
{
    return (TransferI2CPacket(devAddr, devCmd, I2C_REGSIZE_1, (uint8_t *)&opData,
                              SMBUS_RDWR32_NUMBYTES, TRANSFER_WRITE));
}

/* ========================================================================== */
/**
 * \brief   Send a read block request to the Smart Device via the SMBus.
 *
 * \details Used for SMBus commands which require reading a block of  data.
 *
 * \param   devAddr    - Smart Device address
 * \param   devCmd     - SMBus Host to SMBus Device request
 * \param   devCmdSize - Number of bytes requested by SMBus Host to SMBus Device
 * \param   pOpData    - Pointer to operand data to send with the request
 *
 * \return  SMBUS_STATUS - SMBus Status
 * \retval      See L3_SMBus.h
 *
 * ========================================================================== */
SMBUS_STATUS L3_SMBusReadBlock(uint16_t devAddr, uint8_t devCmd, uint8_t devCmdSize, uint8_t *pOpData)
{
    return (TransferI2CPacket(devAddr, devCmd, I2C_REGSIZE_1,
                              pOpData, devCmdSize, TRANSFER_READ));
}

/* ========================================================================== */
/**
 * \brief   Send a write block request to the Smart Device via the SMBus.
 *
 * \details Used for SMBus commands which require writing a block of  data.
 *
 * \param   devAddr    - Smart Device address
 * \param   devCmd     - SMBus Host to SMBus Device request
 * \param   devCmdSize - Number of bytes written by SMBus Host to SMBus Device
 * \param   pOpData    - Pointer to operand data to send with the request
 *
 * \return  SMBUS_STATUS - SMBus Status
 * \retval      See L3_SMBus.h
 *
 * ========================================================================== */
SMBUS_STATUS L3_SMBusWriteBlock(uint16_t devAddr, uint8_t devCmd, uint8_t devCmdSize, uint8_t *pOpData)
{
    return (TransferI2CPacket(devAddr, devCmd, I2C_REGSIZE_1,
                              pOpData, devCmdSize, TRANSFER_WRITE));
}

/* ========================================================================== */
/**
 * \brief   .API to update the PEC enable status
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */

void L3_SMBus_UpdatePECFlag(bool Status)
{
    OS_CPU_SR cpu_sr;   // Status register save for enter/exit critical section

    OS_ENTER_CRITICAL();                    // Insure both values are updated without interruption
    SMBus_PECEnable = Status;
    OS_EXIT_CRITICAL();
}

/* ========================================================================== */
/**
 * \brief   .API to read the PEC enable status
 *
 * \param   < None >
 *
 * \return  bool
 *
 * ========================================================================== */

bool L3_SMBus_GetPECFlag(void)
{
    bool Status;
    OS_CPU_SR cpu_sr;   // Status register save for enter/exit critical section

    OS_ENTER_CRITICAL();                    // Insure both values are updated without interruption
    Status = SMBus_PECEnable;
    OS_EXIT_CRITICAL();
    return Status;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif




