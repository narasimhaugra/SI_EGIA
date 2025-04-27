#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
* \addtogroup L3_FPGA
* \{
* \brief   This module contains support functions for programming the FPGA.
*
* \details The routines for interacting with the FPGA via the I2C port are
*          implemented here. This includes functions for sending commands,
*          reading and writing registers used in the FPGA configuration and
*          programming. It is recommended to read the following reference
*          datasheets / technical notes to get more details about the FPGA
*          programming.
*
* \sa      Lattice: DS1035 - MachXO2 Family Data Sheet
* \sa      Lattice: TN1204 - MachXO2 Programming and Configuration Usage Guide
* \sa      Lattice: TN1246 - Using User Flash Memory and Hardened Control Functions in MachXO2 Devices Reference Guide
* \sa      Lattice: RD1129 - MachXO2 I2C Embedded Programming Access Firmware
*
* \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
*
* \file    L3_FpgaMgr.c
*
* ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_I2c.h"
#include "L3_GpioCtrl.h"
#include "Logger.h"
#include "L3_Fpga.h"
#include "L3_FpgaMgr.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_FPGA) /// Identifier for log entries

#define FPGA_I2C_TIMEOUT            (10U)            /// FPGA I2C transaction timeout
#define FPGA_PROGRAMN_NOP_DELAY     (25U)            /// 2uS NOP delay count for program N assert
#define FPGA_DEF_OP_BUFF_CNT        (3U)             /// Default Operands Count [reference TN1204]
#define FPGA_I2C_CMD_BUFF_SIZE      (64U)            /// I2C (ISC/LSC) Command Buffer Size.

#define FPGA_FILEDATA_MAX_SIZE      (512U)           /// Maximum FileData per transaction.
#define FPGA_BYTES_PER_PAGE         (16U)            /// Bytes Per FPGA Page memory.
#define FPGA_PAGE_SHIFT_VALUE       (4U)             /// Shift value corresponding to a page.
#define FPGA_ERASE_MASK             (0x0FU)          /// Fpga Erase Memory Mask.

#define FPGA_ONE_BYTE_SHIFT         (8U)             /// Bits to shift 1 byte
#define FPGA_TWO_BYTE_SHIFT         (16U)            /// Bits to shift 2 bytes
#define FPGA_THREE_BYTE_SHIFT       (24U)            /// Bits to shift 3 bytes
#define FPGA_BYTE_MASK              (0xFFU)          /// Byte Mask

#define FPGA_DATA_0                 (0U)             /// Index to data byte 0
#define FPGA_DATA_1                 (1U)             /// Index to data byte 1
#define FPGA_DATA_2                 (2U)             /// Index to data byte 2
#define FPGA_DATA_3                 (3U)             /// Index to data byte 3
#define FPGA_DATA_4                 (4U)             /// Index to data byte 4

#define IS_REFRESH_SUCCESS(Reg)     (((Reg) & 0x3300U) == 0x0100U)  /// B=0, F=0, C=0, D=1 [Reference: TN1246]
#define IS_BUSY_BIT_SET(Reg)        ((Reg) & 0x1000U)               /// B=1 [Reference: TN1246]

#define IS_CONFIG_MEM_SET(MemArea)  ((MemArea) & FPGA_CONFIG)       /// Config Memory Area Mask [0x04u - ISC_ERASE]
#define IS_UFM_MEM_SET(MemArea)     ((MemArea) & FPGA_UFM)          /// UFM Memory Area Mask [0x08u - ISC_ERASE]

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                /// MachXO2 FPGA Programming Commands [Names as per TN1204]
{
    IDCODE_PUB,             ///< Read Device ID
    ISC_ENABLE_X,           ///< Enable Config Interface(Transparent Mode)
    ISC_ENABLE,             ///< Enable Config Interface(Offline Mode)
    LSC_CHECK_BUSY,         ///< Read Busy Flag
    LSC_READ_STATUS,        ///< Read Status Register
    ISC_ERASE,              ///< Erase
    LSC_ERASE_TAG,          ///< Erase UFM
    LSC_INIT_ADDRESS,       ///< Reset Configuration Flash Address
    LSC_WRITE_ADDRESS,      ///< Set Address
    LSC_PROG_INCR_NV,       ///< Program Page
    LSC_INIT_ADDR_UFM,      ///< Reset UFM Address
    LSC_PROG_TAG,           ///< Program UFM Page
    ISC_PROGRAM_USERCODE,   ///< Program USERCODE
    USERCODE,               ///< Read USERCODE
    LSC_PROG_FEATURE,       ///< Write Feature Row
    LSC_READ_FEATURE,       ///< Read Feature Row
    LSC_PROG_FEABITS,       ///< Write FEABITS
    LSC_READ_FEABITS,       ///< Read FEABITS
    LSC_READ_INCR_NV,       ///< Read Flash
    LSC_READ_UFM,           ///< Read UFM Flash
    ISC_PROGRAM_DONE,       ///< Program DONE
    LSC_PROG_OTP,           ///< Program OTP Fuses
    LSC_READ_OTP,           ///< Read OTP Fuses
    ISC_DISABLE,            ///< Disable Configuration Interface
    ISC_NOOP,               ///< Bypass
    LSC_REFRESH,            ///< Refresh
    ISC_PROGRAM_SECURITY,   ///< Program SECURITY
    ISC_PROGRAM_SECPLUS,    ///< Program SECURITY PLUS
    UIDCODE_PUB             ///< Read TraceID code
} FPGA_ISC_COMMAND;

typedef struct              /// MachXO2 FPGA sysCONFIG Programming Commands format
{
    FPGA_ISC_COMMAND  eCmdType;                                 ///< Command Type Enum
    uint8_t           u8CmdValue;                               ///< Command Value
    uint8_t           u8OperandsBuff[FPGA_DEF_OP_BUFF_CNT];     ///< Default Operands Buffer [3 bytes]
    uint8_t           u8OperandsCount;                          ///< Default Operands Count [For most case, it is 3 bytes]
    uint8_t           u8ReadDataCount;                          ///< Default Read Date Bytes Count
} FPGA_PROGRAM_CMD;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
static FPGA_PROGRAM_CMD mxFpgaProgramCmdTable[] =             /// FPGA Program Command Table [Refer TN1204 MachXO2 Programming and Configuration Usage Guide]
{   /* eCmdType,         u8CmdValue, {u8OperandsBuff}, u8OperandsCount, u8ReadDataCount */
    { IDCODE_PUB,              0xE0, {0x00, 0x00, 0x00}, 3, 0 }, ///< Read Device ID
    { ISC_ENABLE_X,            0x74, {0x08, 0x00, 0x00}, 2, 0 }, ///< Enable Config Interface(Transparent Mode) [Note: 2 operand bytes when using the I2C port]
    { ISC_ENABLE,              0xC6, {0x08, 0x00, 0x00}, 2, 0 }, ///< Enable Config Interface(Offline Mode) [Note: 2 operand bytes when using the I2C port]
    { LSC_CHECK_BUSY,          0xF0, {0x00, 0x00, 0x00}, 3, 0 }, ///< Read Busy Flag
    { LSC_READ_STATUS,         0x3C, {0x00, 0x00, 0x00}, 3, 4 }, ///< Read Status Register
    { ISC_ERASE,               0x0E, {0x00, 0x00, 0x00}, 3, 0 }, ///< Erase
    { LSC_ERASE_TAG,           0xCB, {0x00, 0x00, 0x00}, 3, 0 }, ///< Erase UFM
    { LSC_INIT_ADDRESS,        0x46, {0x00, 0x00, 0x00}, 3, 0 }, ///< Reset Configuration Flash Address
    { LSC_WRITE_ADDRESS,       0xB4, {0x00, 0x00, 0x00}, 3, 0 }, ///< Set Address
    { LSC_PROG_INCR_NV,        0x70, {0x00, 0x00, 0x01}, 3, 0 }, ///< Program Page
    { LSC_INIT_ADDR_UFM,       0x47, {0x00, 0x00, 0x00}, 3, 0 }, ///< Reset UFM Address
    { LSC_PROG_TAG,            0xC9, {0x00, 0x00, 0x01}, 3, 0 }, ///< Program UFM Page
    { ISC_PROGRAM_USERCODE,    0xC2, {0x00, 0x00, 0x00}, 3, 0 }, ///< Program USERCODE
    { USERCODE,                0xC0, {0x00, 0x00, 0x00}, 3, 0 }, ///< Read USERCODE
    { LSC_PROG_FEATURE,        0xE4, {0x00, 0x00, 0x00}, 3, 0 }, ///< Write Feature Row
    { LSC_READ_FEATURE,        0xE7, {0x00, 0x00, 0x00}, 3, 0 }, ///< Read Feature Row
    { LSC_PROG_FEABITS,        0xF8, {0x00, 0x00, 0x00}, 5, 0 }, ///< Write FEABITS [Note: 3 + 2 Bytes]
    { LSC_READ_FEABITS,        0xFB, {0x00, 0x00, 0x00}, 3, 2 }, ///< Read FEABITS
    { LSC_READ_INCR_NV,        0x73, {0x00, 0x00, 0x00}, 3, 0 }, ///< Read Flash  [Note: Dummy operands]
    { LSC_READ_UFM,            0xCA, {0x00, 0x00, 0x00}, 3, 0 }, ///< Read UFM Flash [Note: Dummy operands]
    { ISC_PROGRAM_DONE,        0x5E, {0x00, 0x00, 0x00}, 3, 0 }, ///< Program DONE
    { LSC_PROG_OTP,            0xF9, {0x00, 0x00, 0x00}, 3, 0 }, ///< Program OTP Fuses
    { LSC_READ_OTP,            0xFA, {0x00, 0x00, 0x00}, 3, 0 }, ///< Read OTP Fuses
    { ISC_DISABLE,             0x26, {0x00, 0x00, 0x00}, 2, 0 }, ///< Disable Configuration Interface [Note: Default 2 operands]
    { ISC_NOOP,                0xFF, {0xFF, 0xFF, 0xFF}, 3, 0 }, ///< Bypass
    { LSC_REFRESH,             0x79, {0x00, 0x00, 0x00}, 2, 0 }, ///< Refresh [Note: Default 2 operands].
    { ISC_PROGRAM_SECURITY,    0xCE, {0x00, 0x00, 0x00}, 3, 0 }, ///< Program SECURITY
    { ISC_PROGRAM_SECPLUS,     0xCF, {0x00, 0x00, 0x00}, 3, 0 }, ///< Program SECURITY PLUS
    { UIDCODE_PUB,             0x19, {0x00, 0x00, 0x00}, 3, 0 }, ///< Read TraceID code
};

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static FPGA_MGR_STATUS meFpgMgrStatus;  ///< Fpga Current Status.

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static bool FpgaMgrSendCmd(uint8_t u8Cmd, uint8_t *pOpData, uint16_t u16NumBytes);
static bool FpgaMgrSendCmdGetResponse(uint8_t u8Cmd, uint8_t *pOpData, uint16_t u16NumBytes,
                                      uint8_t *pDataDest, uint16_t u16DataSize);

static bool FpgaMgrAssertProgramNSignal(void);
static bool FpgaMgrPerformFPGARefresh(void);
static bool FpgaMgrGetStatusRegValue(uint32_t *pFpgaStatusRegValue);

static bool FpgaMgrUpdateFpgaMemory(MACHX02 *pJedInfo, FPGA_MEM_AREA eMemArea);

static bool FpgaMgrEnterProgrammingMode(void);
static bool FpgaMgrEraseMemory(FPGA_MEM_AREA eMemArea);
static bool FpgaMgrCheckBusyFlagAfterErase(void);
static bool FpgaMgrEnterWriteMode(FPGA_MEM_AREA eMemArea);
static bool FpgaMgrWriteDataToFpgaMemory(const uint8_t *pu8Data, uint16_t u16DataLength);
static bool FpgaMgrExitProgrammingMode(void);

/// \todo 12/07/20 GK - This structure is as per the Legacy Blob Handler and Signia Blobs.
/// \todo 12/07/20 GK - Replace these with proper calls once after Blob Handler implementation.
extern uint32_t FpgaMgrTempFun_BlobGetData(FPGA_MEM_AREA eMemArea, uint8_t *destPtr,
                                           uint32_t sourceOffset, uint32_t dataSize);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
* \brief   Send a command to the FPGA via I2C.
*
* \details Used for FPGA ISC/LSC commands which do not return data.
*
* \param   u8Cmd       - ISC or LSC command to send to the FPGA
* \param   pOpData     - Pointer to operand data to send with the command
* \param   u16NumBytes - Number of operand bytes
*
* \return  bool - Error status: True if failure, false if success
*
* ========================================================================== */
#pragma optimize = none
static bool FpgaMgrSendCmd(uint8_t u8Cmd, uint8_t *pOpData, uint16_t u16NumBytes)
{
    I2CDataPacket xDataPacket;   /* Argument block for I2C transmission */
    bool          bStatus;       /* Function error status. True if error. */

    /* Start with Error status */
    bStatus = true;

    do
    {   /* Check Validity */
        if (NULL == pOpData)
        {
            Log(ERR, "FpgaMgrSendCmd: Invalid input");
            break;
        }

        /* Assemble the I2C Data Packet */
        xDataPacket.Address = FPGA_SLAVE_ADDRESS;
        xDataPacket.pReg = &u8Cmd;
        xDataPacket.nRegSize = 1u;
        xDataPacket.pData = pOpData;
        xDataPacket.nDataSize = u16NumBytes;

        /* Call I2C Write */
        if (I2C_STATUS_SUCCESS != L3_I2cWrite(&xDataPacket))
        {
            Log(ERR, "FpgaMgrSendCmd: L3_I2cWrite failed");
            break;
        }

        /* Success */
        bStatus = false;

    } while (false);

    return bStatus;
}

/* ========================================================================== */
/**
* \brief   Send a command to the FPGA via I2C and collect the response.
*
* \details Used for FPGA ISC/LSC commands that returns data.
*
* \param   u8Cmd       - ISC or LSC command to send to the FPGA
* \param   pOpData     - Pointer to operand data to send with the command
* \param   u16NumBytes - Number of operand bytes to send
* \param   pDataDest   - Pointer to buffer to store response
* \param   u16DataSize - Number of bytes to retrieve
*
* \return  bool - Error status: True if failure, false if success
*
* ========================================================================== */
#pragma optimize = none
static bool FpgaMgrSendCmdGetResponse(uint8_t u8Cmd, uint8_t *pOpData, uint16_t u16NumBytes,
                                      uint8_t *pDataDest, uint16_t u16DataSize)
{
    I2CDataPacket xDataPacket;                              /* Argument block for I2C transmission */
    bool          bStatus;                                  /* Function error status. True if error. */
    uint8_t       u8CmdOpCodeBuff[FPGA_I2C_CMD_BUFF_SIZE];  /* Buffer for sending command & operand data. */

    do
    {   /* Start with Error status */
        bStatus = true;

        /* Check Validity */
        if ((NULL == pOpData) || (NULL == pDataDest))
        {
            Log(ERR, "FpgaMgrSendCmdGetResponse: Invalid input");
            break;
        }

        /* Assemble command & operands into "register write" buffer,
        so that L3_I2cRead() writes and then reads the response */
        memset(u8CmdOpCodeBuff, 0x00u, sizeof(u8CmdOpCodeBuff));
        u8CmdOpCodeBuff[FPGA_DATA_0] = u8Cmd;
        memcpy(&u8CmdOpCodeBuff[FPGA_DATA_1], pOpData, u16NumBytes);

        xDataPacket.Address = FPGA_SLAVE_ADDRESS;
        xDataPacket.pReg = u8CmdOpCodeBuff;
        xDataPacket.nRegSize = (u16NumBytes + 1);
        xDataPacket.pData = pDataDest;
        xDataPacket.nDataSize = u16DataSize;

        /* Call I2C Write */
        if (I2C_STATUS_SUCCESS != L3_I2cRead(&xDataPacket))
        {
            Log(ERR, "FpgaMgrSendCmdGetResponse: L3_I2cRead failed");
            break;
        }

        /* Success */
        bStatus = false;

    } while (false);

    return bStatus;
}

/* ========================================================================== */
/**
* \brief   This function force the Fpga to load its program
*
* \details This function pulls the ProgramN pin on the FPGA low for 2uS then
*          releases it. ProgramN pin is marked as GPIO_EXTRA_IO_uC0 GPIO
*          signal.
*
* \todo    12/07/20 GK - Need better way to get 2uS delay.
*
* \param   < None >
*
* \return  bool - Error status - True if fail, False if success
*
* ========================================================================== */
#pragma optimize = none
static bool FpgaMgrAssertProgramNSignal(void)
{
    uint16_t    u16DelayCount;  /* Nop counter */
    GPIO_STATUS eGpioStatus;    /* Gpio Status Variable */
    bool        bStatus;        /* Function error status. True if error. */

    do
    {   /* Start with Error status */
        bStatus = true;

        /* Assert Fpga Program N Line */
        eGpioStatus = L3_GpioCtrlClearSignal(GPIO_EXTRA_IO_uC0);

        if(GPIO_STATUS_OK != eGpioStatus)
        {
            Log(ERR, "FpgaMgr: AssertProgramNSignal, GPIO_EXTRA_IO_uC0 Assert Failed, Gpio Status = %d", eGpioStatus);
            break;
        };

        /* Wait for 2uS */
        for (u16DelayCount = 0; u16DelayCount < FPGA_PROGRAMN_NOP_DELAY; u16DelayCount++)
        {
            asm("nop");
        }

        /* De-Assert Fpga Program N Line */
        eGpioStatus = L3_GpioCtrlSetSignal(GPIO_EXTRA_IO_uC0);

        if(GPIO_STATUS_OK != eGpioStatus)
        {
            Log(ERR, "FpgaMgr: AssertProgramNSignal, GPIO_EXTRA_IO_uC0 De-Assert Failed, Gpio Status = %d", eGpioStatus);
            break;
        };

        /* Return Success */
        bStatus = false;

    } while (false);

    return bStatus;
}

/* ========================================================================== */
/**
* \brief   Send the LSC_REFRESH command to the FPGA via I2C.
*
* \details This function sends the refresh command & verify it succeeded.
*
* \param   < None >
*
* \return  bool - Error status: True if failure, false if success
*
* ========================================================================== */
static bool FpgaMgrPerformFPGARefresh(void)
{
    bool     bFailed;               /* Function error status. True if error. */
    uint32_t u32StatusReg;          /* Status register read from FPGA. */
    uint32_t u32GiveUpTime;         /* Operation Give Up Time */

    u32StatusReg = 0u; // Initialize variable

    /* Set the Next timeout value */
    u32GiveUpTime = OSTimeGet() + MSEC_100;

    do
    {   /* Set Status Failed. */
        bFailed = true;

        /* Is Timeout? */
        if (OSTimeGet() > u32GiveUpTime)
        {
            Log(ERR, "FpgaMgr: Refresh, Failed!, Status Reg = 0x%08X", u32StatusReg);
            break;
        }
        /* Send the LSC_REFRESH command */
        bFailed = FpgaMgrSendCmd(mxFpgaProgramCmdTable[LSC_REFRESH].u8CmdValue,
                                 mxFpgaProgramCmdTable[LSC_REFRESH].u8OperandsBuff,
                                 mxFpgaProgramCmdTable[LSC_REFRESH].u8OperandsCount);

        /* tRefresh */
        OSTimeDly(MSEC_5);

        /* Get the Fpga Status Register Value */
        bFailed |= FpgaMgrGetStatusRegValue(&u32StatusReg);

        /* Retry if "Refresh Failed" or "Any Error?" */
    } while (bFailed || !IS_REFRESH_SUCCESS(u32StatusReg));

    if (!bFailed)
    {
        Log(REQ, "FpgaMgr: Refresh Succeeded, Status Reg = 0x%08X", u32StatusReg);
    }
    else
    {
        Log(REQ, "FpgaMgr: Refresh Failed, Status Reg = 0x%08X", u32StatusReg);
    }

    /* Return Status */
    return bFailed;
}

/* ========================================================================== */
/**
* \brief   Retrieve the 32 bit status register from the FPGA
*
* \details Status bits: [Refer TN1246, NOT TN1204]
*
*          Data Format (Binary)
*          - xxxx IxEE Exxx xxxx xxFB xxCD xxxx xxxx
*
*          - D bit 8: Flash or SRAM Done Flag
*              - When C = 0 SRAM Done bit has been programmed
*                  - D = 1 Successful Flash to SRAM transfer
*                  - D = 0 Failure in the Flash to SRAM transfer
*              - When C = 1 Flash Done bit has been programmed
*                  - D = 1 Programmed
*                  - D = 0 Not Programmed
*
*          - C bit 9: Enable Configuration Interface (1=Enable, 0=Disable)
*          - B bit 12: Busy Flag (1 = busy)
*          - F bit 13: Fail Flag (1 = operation failed)
*
*          - EEE bits[25:23]: Configuration Check Status
*              - 000: No Error
*              - 001: ID ERR
*              - 010: CMD ERR
*              - 011: CRC ERR
*              - 100: Preamble ERR
*              - 101: Abort ERR
*              - 110: Overflow ERR
*              - 111: SDM EOF
*
*          - I bit 27: I=0 Device verified correct, I=1 Device failed to verify
*          (all other bits reserved)
*
* \sa      TN1246 - "Using User Flash Memory and Hardened Control Functions in
*                    MachXO2 Devices Reference Guide"
*
* \param   pFpgaStatusRegValue - Pointer to status register var where status
*                            data will be stored( output parameter ).
*
* \return  bool - Error status - True if fail, False if success
*
* ========================================================================== */
static bool FpgaMgrGetStatusRegValue(uint32_t *pFpgaStatusRegValue)
{
    bool                bStatus;                                 /* Function error status. True if error. */
    FPGA_PROGRAM_CMD    xFpgaCmd;                                /* Local Cmd Variable */
    uint8_t             u8ReadBuff[FPGA_I2C_CMD_BUFF_SIZE];      /* Buffer to Store the Status Registers. */

    /* Set to defaults */
    bStatus = true;
    memset(u8ReadBuff, 0x00u, sizeof(u8ReadBuff));

    do
    {
        /* Check Input Validity */
        if (NULL == pFpgaStatusRegValue)
        {
            Log(ERR, "FpgaMgr: GetStatusRegValue: Invalid input");
            break;
        }

        /* Get the LSC_READ_STATUS params from the global table */
        xFpgaCmd = mxFpgaProgramCmdTable[LSC_READ_STATUS];

        /* Send the command to FPGA and Get Res */
        bStatus = FpgaMgrSendCmdGetResponse(xFpgaCmd.u8CmdValue, xFpgaCmd.u8OperandsBuff, xFpgaCmd.u8OperandsCount,
                                            u8ReadBuff, xFpgaCmd.u8ReadDataCount);
        /* Is it Success? */
        if (bStatus)
        {
            Log(ERR, "FpgaMgr: GetStatusRegValue: LSC_READ_STATUS Cmd Failed! ");
            break;
        }
        else
        {
            /* Yes, Return the Status Reg Value to the caller. */
            *pFpgaStatusRegValue = (u8ReadBuff[FPGA_DATA_0] << FPGA_THREE_BYTE_SHIFT)|
                                   (u8ReadBuff[FPGA_DATA_1] << FPGA_TWO_BYTE_SHIFT  )|
                                   (u8ReadBuff[FPGA_DATA_2] << FPGA_ONE_BYTE_SHIFT  )|
                                   (u8ReadBuff[FPGA_DATA_3]);
        }

    } while (false);

    return bStatus;
}

/* ========================================================================== */
/**
* \brief   Update the FPGA Memory Area [Config or UFM]
*
* \details This function will update the FPGA Flash Memory Area.
*
* \param   pJedInfo - pointer to Jedec info
* \param   eMemArea - Update Memory Area [Config or UFM]
*
* \return  bool - Error status: True if failure, false if success
*
* ========================================================================== */
static bool FpgaMgrUpdateFpgaMemory(MACHX02 *pJedInfo, FPGA_MEM_AREA eMemArea)
{
    bool bStatus;                                       /* Function error status. True if error. */
    uint32_t u32BytesRead;                              /* Bytes Read from Blob Variable */
    uint32_t u32BytesToRead;                            /* Bytes to Read from Blob Variable */
    uint32_t u32MaxBytesToRead;                         /* Max Bytes to Read from Blob Variable  */
    uint32_t u32DataOffset;                             /* The Blob data area offset */
    uint8_t  pu8FileDataBuff[FPGA_FILEDATA_MAX_SIZE];   /* The data buffer */
    uint32_t u32JedDataSize;                            /* The Jed Data Size Variable [For Config and UFM] */

    do
    {   /* Set to defaults */
        bStatus = true;
        memset(pu8FileDataBuff, 0x00u, sizeof(pu8FileDataBuff));
        u32DataOffset = 0;
        u32MaxBytesToRead = FPGA_FILEDATA_MAX_SIZE;

        /* Check the Input */
        if (NULL == pJedInfo)
        {
            Log(ERR, "FpgaMgr: Null Parameter Passed");
            break;
        }

        /* Get the Memory area data size */
        if(FPGA_CONFIG == eMemArea)
        {
            /* Get the Config Area Data Size */
            u32JedDataSize = pJedInfo->FuseDataSize;
        }
        else if(FPGA_UFM == eMemArea)
        {
            /* Get the UFM Area Data Size */
            u32JedDataSize = pJedInfo->UfmDataSize;
        }
        else
        {
            /* All other memory areas, are not handled in this function */
            Log(ERR, "FpgaMgr: Invalid Memory Area Parameter Passed");
            break;
        }

        /* 16 bytes per page for FPGA programming */
        while (u32MaxBytesToRead % FPGA_BYTES_PER_PAGE)
        {
            u32MaxBytesToRead -= 1;
        }

        /* Enter Config Data Write Mode */
        if (FpgaMgrEnterWriteMode(eMemArea))
        {
            Log(ERR, "FpgaMgrUpdateFpgaMemory: FpgaMgrEnterWriteMode failed.");
            break;
        }

        /* Set Return Status as Success and loop through the bytes, any failure will set the status Fails.  */
        bStatus = false;

        /* Loop through entire Data Size*/
        while (u32DataOffset < u32JedDataSize)
        {
            /* Get the next Bytes count */
            u32BytesToRead = MIN(u32JedDataSize - u32DataOffset, u32MaxBytesToRead);

            /* Read the bytes from the Blob */
            /// \todo 12/07/20 GK - This code is as per the Legacy Blob Handler and Signia Blobs.
            /// \todo 12/07/20 GK - Re-visit this during/after Blob Handler implementation
            u32BytesRead = FpgaMgrTempFun_BlobGetData(eMemArea, pu8FileDataBuff, u32DataOffset, u32BytesToRead);

            /* Do we have something */
            if (u32BytesRead == 0)
            {
                Log(ERR, "FpgaMgrUpdateFpgaMemory: BlobGetData failed. u32DataOffset = %d", u32DataOffset);

                /* Set Status Failed. */
                bStatus = true;
                break;
            }

            /* Update the offset */
            u32DataOffset += u32BytesRead;

            /* 16 bytes per page for FPGA programming */
            while (u32BytesRead % FPGA_BYTES_PER_PAGE)
            {
                u32BytesRead += 1;
            }

            /* Write the read data to FPGA. */
            if (FpgaMgrWriteDataToFpgaMemory(pu8FileDataBuff, u32BytesRead))
            {
                Log(ERR, "FpgaMgrUpdateFpgaMemory: FPGA_WriteData failed. u32DataOffset = %d", u32DataOffset);

                /* Set Status Failed. */
                bStatus = true;
                break;
            }
        }

    } while (false);

    return bStatus;
}

/* ========================================================================== */
/**
* \brief   Enter Programming Mode.
*
* \details This function puts FPGA into programming mode.
*
* \param   < None >
*
* \return  bool - Error status: True if failure, false if success
*
* ========================================================================== */
static bool FpgaMgrEnterProgrammingMode(void)
{
    bool     bFailed;               /* Function error status. True if error. */
    uint32_t u32GiveUpTime;         /* Operation Give Up Time */

    /* Set the Next timeout value */
    u32GiveUpTime = OSTimeGet() + MSEC_500;

    do
    {   /* Set Status Failed. */
        bFailed = true;

        /* Is Timeout? */
        if (OSTimeGet() > u32GiveUpTime)
        {
            Log(ERR, "FpgaMgr: EnterProgrammingMode: Send ISC_ENABLE_X [0x74] Cmd: Failed");
            break;
        }

        /* Send the ISC_ENABLE_X [0x74] command to FPGA, (background: FPGA runs normally while updating FLASH) */
        bFailed = FpgaMgrSendCmd(mxFpgaProgramCmdTable[ISC_ENABLE_X].u8CmdValue,
                                 mxFpgaProgramCmdTable[ISC_ENABLE_X].u8OperandsBuff,
                                 mxFpgaProgramCmdTable[ISC_ENABLE_X].u8OperandsCount);

        /* Delay a While */
        OSTimeDly(MSEC_1);

    } while (bFailed);

    /* Return Status */
    return bFailed;
}

/* ========================================================================== */
/**
* \brief   Erase any/all entire sectors of the XO2 Flash memory.
*
* \details Erase sectors based on the bitmap of parameter mode passed in.
*
* \param   eMemArea - Bit map of the memory aread to erase
*
* \return  bool - Error status: True if failure, false if success
*
* ========================================================================== */
static bool FpgaMgrEraseMemory(FPGA_MEM_AREA eMemArea)
{
    bool             bFailed;               /* Function error status. True if error. */
    uint32_t         u32GiveUpTime;         /* Operation Give Up Time */
    FPGA_PROGRAM_CMD xFpgaCmd;              /* Local Cmd Variable */
    uint8_t          u8EraseMemoryArea;     /* Erase Memory Area */

    /* Set the Next timeout value */
    u32GiveUpTime = OSTimeGet() + SEC_8;

    do
    {   /* Set Status Failed. */
        bFailed = true;

        /* Is Timeout? */
        if (OSTimeGet() > u32GiveUpTime)
        {
            Log(ERR, "FpgaMgr: EnterProgrammingMode: Send ISC_ENABLE_X [0x74] Cmd: Failed");
            break;
        }

        /* Make Sure we have proper value. */
        u8EraseMemoryArea = eMemArea & FPGA_ERASE_MASK;

        /* Get the ISC_ERASE params from the global table */
        xFpgaCmd = mxFpgaProgramCmdTable[ISC_ERASE];

        /* Update the Operand Buffer argument with Area to clear */
        xFpgaCmd.u8OperandsBuff[FPGA_DATA_1] = u8EraseMemoryArea;

        /* Send the ISC_ERASE [0x0E] command to FPGA */
        bFailed = FpgaMgrSendCmd(xFpgaCmd.u8CmdValue, xFpgaCmd.u8OperandsBuff, xFpgaCmd.u8OperandsCount);

        /* Config Erase needs more time */
        if (IS_CONFIG_MEM_SET(eMemArea))
        {
            /* Wait 4 seconds for Config Erase to complete */
            OSTimeDly(SEC_4);
        }
        else
        {
            /* Wait 1 seconds for UFM / SRAM Erase to complete */
            OSTimeDly(SEC_1);
        }

    } while (bFailed);

    /* Return Status */
    return bFailed;
}

/* ========================================================================== */
/**
* \brief   Check the Busy Flag after an Erase.
*
* \details This function checks for Busy bit to become clear.
*
* \param   < None >
*
* \return  bool - Error status: True if failure, false if success
*
* ========================================================================== */
static bool FpgaMgrCheckBusyFlagAfterErase(void)
{
    bool     bFailed;               /* Function error status. True if error. */
    uint32_t u32GiveUpTime;         /* Operation Give Up Time */
    uint32_t u32StatusReg;          /* Status register read from FPGA. */

    u32StatusReg = 0u; // Initialize variable

    /* Set the Next timeout value */
    u32GiveUpTime = OSTimeGet() + SEC_5;

    /* Wait for busy flag to clear */
    do
    {   /* Set Status Failed. */
        bFailed = true;

        /* Is Timeout? */
        if (OSTimeGet() > u32GiveUpTime)
        {
            /* Is Busy Flag set even after retries? */
            if (IS_BUSY_BIT_SET(u32StatusReg))
            {
                Log(ERR, "FpgaMgr: Fpga Failed to become Idle");
            }
            else
            {
                Log(ERR, "FpgaMgr: FpgaMgrGetStatusRegValue: Failed");
            }

            break;
        }

        /* Get the Status Register Value. */
        bFailed = FpgaMgrGetStatusRegValue(&u32StatusReg);

        /* Delay a While */
        OSTimeDly(MSEC_1);

    } while (bFailed || IS_BUSY_BIT_SET(u32StatusReg));

    /* Return Status */
    return bFailed;
}

/* ========================================================================== */
/**
* \brief   Fpga Enter Write Mode.
*
* \details This function enables the FPGA to enter into write mode.
*
* \param   eMemArea - Update Memory Area
*
* \return  bool - Error status: True if failure, false if success
*
* ========================================================================== */
static bool FpgaMgrEnterWriteMode(FPGA_MEM_AREA eMemArea)
{
    bool bStatus;  /* Function error status. True if error. */

    /* Set Status Failed. */
    bStatus = true;

    switch (eMemArea)
    {
      case FPGA_CONFIG:
        /* Send the LSC_INIT_ADDRESS command to FPGA  */
        bStatus = FpgaMgrSendCmd(mxFpgaProgramCmdTable[LSC_INIT_ADDRESS].u8CmdValue,
                                 mxFpgaProgramCmdTable[LSC_INIT_ADDRESS].u8OperandsBuff,
                                 mxFpgaProgramCmdTable[LSC_INIT_ADDRESS].u8OperandsCount);
        break;

      case FPGA_UFM:
        /* Send the LSC_INIT_ADDR_UFM command to FPGA  */
        bStatus = FpgaMgrSendCmd(mxFpgaProgramCmdTable[LSC_INIT_ADDR_UFM].u8CmdValue,
                                 mxFpgaProgramCmdTable[LSC_INIT_ADDR_UFM].u8OperandsBuff,
                                 mxFpgaProgramCmdTable[LSC_INIT_ADDR_UFM].u8OperandsCount);
        break;

      default:
        /* Status already set to failed. */
        break;
    }

    return bStatus;
}

/* ========================================================================== */
/**
* \brief   Write Fpga Data to Memory
*
* \details This function writes data to FPGA memory.
*
* \param   pu8Data - Pointer to the data buffer
* \param   u16DataLength - Length of the data buffer
*
* \return  bool - Error status: True if failure, false if success
*
* ========================================================================== */
static bool FpgaMgrWriteDataToFpgaMemory(const uint8_t *pu8Data, uint16_t u16DataLength)
{
    bool bStatus;                                       /* Function error status. True if error. */
    FPGA_PROGRAM_CMD    xFpgaCmd;                       /* Local Cmd Variable */
    uint8_t  u8DataPageBuff[FPGA_FILEDATA_MAX_SIZE];    /* FPGA data page buffer */
    uint16_t u16PageIndex;                              /* Fpga Page Index */
    uint16_t u16PageCount;                              /* Fpga Total Page Count */
    uint16_t u16DataIndex;                              /* Write Data Index */
    uint16_t u16DataPageIndex;                          /* The Final Operand Data count Index */

    /* Initialize the variables */
    bStatus = false;

    /* Get the LSC_PROG_INCR_NV params from the global table */
    xFpgaCmd = mxFpgaProgramCmdTable[LSC_PROG_INCR_NV];

    /* 16 bytes per page */
    u16PageCount = u16DataLength >> FPGA_PAGE_SHIFT_VALUE;

    /* Iterate over every page data */
    for (u16PageIndex = 0; u16PageIndex < u16PageCount; u16PageIndex++)
    {
        /* Clear the local Buffer */
        memset(u8DataPageBuff, 0x00u, sizeof(u8DataPageBuff));

        /* LSC_PROG_INCR_NV operands are {00, 00, 00}, the above memset handles it, so no need of copy */

        /* Plan is to send the data along with the operands, so set the index accordingly */
        u16DataPageIndex = xFpgaCmd.u8OperandsCount;

        /* Copy the Incoming data to the buffer */
        for (u16DataIndex = 0; u16DataIndex < FPGA_BYTES_PER_PAGE; u16DataIndex++)
        {
            u8DataPageBuff[u16DataPageIndex] = pu8Data[(u16PageIndex * FPGA_BYTES_PER_PAGE) + u16DataIndex];
            u16DataPageIndex++;
        }

        /* Send the LSC_PROG_INCR_NV [0x70] command to FPGA, along with the data. */
        bStatus = FpgaMgrSendCmd(mxFpgaProgramCmdTable[LSC_PROG_INCR_NV].u8CmdValue,
                                          u8DataPageBuff,
                                          u16DataPageIndex);
        /* Check the return */
        if(bStatus)
        {
            Log(ERR, "FpgaMgr: WriteDataToFpgaMemory, Send LSC_PROG_INCR_NV [0x70] Cmd, Failed");
            break;
        }

        /* Wait 1ms for Flash program */
        OSTimeDly(MSEC_1);
    }

    /* Return Status */
    return bStatus;
}

/* ========================================================================== */
/**
* \brief   Exit Programming Mode.
*
* \details This function exits the Fpga from programming mode.
*
* \param   < None >
*
* \return  bool - Error status: True if failure, false if success
*
* ========================================================================== */
static bool FpgaMgrExitProgrammingMode(void)
{
    bool     bFailed;               /* Function error status. True if error. */
    uint32_t u32GiveUpTime;         /* Operation Give Up Time */

    /* Set the Next timeout value */
    u32GiveUpTime = OSTimeGet() + MSEC_500;

    do
    {   /* Set Status Failed. */
        bFailed = true;

        /* Is Timeout? */
        if (OSTimeGet() > u32GiveUpTime)
        {
            Log(ERR, "FpgaMgr: ExitProgrammingMode, Failed");
            break;
        }

        /* Send the ISC_PROGRAM_DONE [0x5E] command to FPGA */
        bFailed = FpgaMgrSendCmd(mxFpgaProgramCmdTable[ISC_PROGRAM_DONE].u8CmdValue,
                                          mxFpgaProgramCmdTable[ISC_PROGRAM_DONE].u8OperandsBuff,
                                          mxFpgaProgramCmdTable[ISC_PROGRAM_DONE].u8OperandsCount);
        if (!bFailed)
        {
           /* Send the ISC_DISABLE [0x26] command to FPGA */
            bFailed = FpgaMgrSendCmd(mxFpgaProgramCmdTable[ISC_DISABLE].u8CmdValue,
                                     mxFpgaProgramCmdTable[ISC_DISABLE].u8OperandsBuff,
                                     mxFpgaProgramCmdTable[ISC_DISABLE].u8OperandsCount);
        }

        /* Delay a While */
        OSTimeDly(MSEC_1);

    } while (bFailed);

    /* Return Status */
    return bFailed;
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Updates Program Feature Bits.
 *
 * \details This function updates the FPGA Feature Row bits via I2C
 *
 * \param   u16FeatureBitsToUpdate - Feature Row bits to update
 *
 * \return  FPGA_MGR_STATUS - Fpga Manager Status return
 * \retval      FPGA_MGR_PROGRAM_FAILED - For any error exits.
 * \retval      FPGA_MGR_REFRESH_FAILED  - Fpga Refresh Failed
 * \retval      FPGA_MGR_OK - If Update Success.
 *
 * ========================================================================== */
FPGA_MGR_STATUS L3_FpgaMgrUpdateFeatureBits(uint16_t u16FeatureBitsToUpdate)
{
    bool                bStatus;                              /* Function error status. True if error. */
    FPGA_PROGRAM_CMD    xFpgaCmd;                             /* Local Cmd Variable */
    uint16_t            u16FeatureBitsRead;                   /* Feature Row bits to Read */
    uint8_t             u8UpdateBuff[FPGA_I2C_CMD_BUFF_SIZE]; /* Buffer to Store the Status Registers. */
    FPGA_MGR_STATUS     eFpgaMgrStatus;                       /* Fpga Status Variable */
    bool                bIsFpgaInProgrammingMode;             /* True, if FPGA in Programming Mode, false otherwise */

    do
    {   /* Set to defaults */
        eFpgaMgrStatus = FPGA_MGR_PROGRAM_FAILED;
        bStatus = true;
        bIsFpgaInProgrammingMode = false;
        memset(u8UpdateBuff, 0x00u, sizeof(u8UpdateBuff));

        Log(REQ, "FpgaMgr: UpdateFeatureBits, Starting FPGA FeatureBits Update...");

        /* Enter into Programming Mode */
        if (FpgaMgrEnterProgrammingMode())
        {
            break;
        }

        /* Set the variable */
        bIsFpgaInProgrammingMode = true;

        Log(DBG, "FpgaMgr: UpdateFeatureBits, Send Cmd, ISC_ENABLE_X [0x74]: Success");

         /* Get the LSC_READ_FEABITS params from the global table */
        xFpgaCmd = mxFpgaProgramCmdTable[LSC_READ_FEABITS];

        /* Send the LSC_READ_FEABITS [0xFB] command to FPGA and Read the FEABITS */
        bStatus = FpgaMgrSendCmdGetResponse(xFpgaCmd.u8CmdValue, xFpgaCmd.u8OperandsBuff, xFpgaCmd.u8OperandsCount,
                                            u8UpdateBuff, xFpgaCmd.u8ReadDataCount);
        /* Is it Success? */
        if (bStatus)
        {
            Log(ERR, "FpgaMgr: UpdateFeatureBits, Send Cmd, LSC_READ_FEABITS [0xFB]: Failed! ");
            break;
        }
        else
        {
            /* Yes, Update the Read Result Variable */
            u16FeatureBitsRead = ((u8UpdateBuff[FPGA_DATA_0] << FPGA_ONE_BYTE_SHIFT)  | u8UpdateBuff[FPGA_DATA_1]);
        }

        /* Delay a While  */
        OSTimeDly(MSEC_5);

        Log(DEV, "FpgaMgr: UpdateFeatureBits, Send Cmd, LSC_READ_FEABITS [0xFB]: Success");
        Log(DEV, "FpgaMgr: UpdateFeatureBits, Read FEABITS = 0x%04X", u16FeatureBitsRead);

        /* Check if the feature bits are same as passed */
        if (u16FeatureBitsRead == u16FeatureBitsToUpdate)
        {
            Log(DBG, "FpgaMgr: UpdateFeatureBits, FEABITS Already same, No Update Required");

            /* Set Success */
            eFpgaMgrStatus = FPGA_MGR_OK;

            break;
        }

        /* Feature Bits are different, Lets write the new value to the FPGA */
        Log(DBG, "FpgaMgr: UpdateFeatureBits, FEABITS Different, Going to Update, Write FEABITS = 0x%04X", u16FeatureBitsToUpdate);

        /* Set to defaults */
        memset(u8UpdateBuff, 0x00u, sizeof(u8UpdateBuff));

        /* Get the LSC_PROG_FEABITS params from the global table */
        xFpgaCmd = mxFpgaProgramCmdTable[LSC_PROG_FEABITS];

        /* Create the Write Buffer operands. */
        u8UpdateBuff[FPGA_DATA_0] = xFpgaCmd.u8OperandsBuff[FPGA_DATA_0];
        u8UpdateBuff[FPGA_DATA_1] = xFpgaCmd.u8OperandsBuff[FPGA_DATA_1];
        u8UpdateBuff[FPGA_DATA_2] = xFpgaCmd.u8OperandsBuff[FPGA_DATA_2];
        u8UpdateBuff[FPGA_DATA_3] = (uint8_t)(u16FeatureBitsToUpdate >> FPGA_ONE_BYTE_SHIFT);
        u8UpdateBuff[FPGA_DATA_4] = (uint8_t)(u16FeatureBitsToUpdate & FPGA_BYTE_MASK);

        /* Send the LSC_PROG_FEABITS [0xF8] command to FPGA */
        bStatus = FpgaMgrSendCmd(xFpgaCmd.u8CmdValue, u8UpdateBuff, xFpgaCmd.u8OperandsCount);

        /* Check the return */
        if(bStatus)
        {
            Log(ERR, "FpgaMgr: UpdateFeatureBits, Send Cmd, LSC_PROG_FEABITS [0xF8]: Failed");
            break;
        }

        /* Delay a While  */
        OSTimeDly(MSEC_5);

        Log(DEV, "FpgaMgr: UpdateFeatureBits, Send Cmd, LSC_PROG_FEABITS [0xF8]: Success");

        /* Write Successful, Read it back and compare */
        Log(DBG, "FpgaMgr: UpdateFeatureBits, Reading FEABITS after write");

        /* Set to defaults */
        memset(u8UpdateBuff, 0x00u, sizeof(u8UpdateBuff));

         /* Get the LSC_READ_FEABITS params from the global table */
        xFpgaCmd = mxFpgaProgramCmdTable[LSC_READ_FEABITS];

        /* Send the LSC_READ_FEABITS [0xFB] command to FPGA and Read the FEABITS */
        bStatus = FpgaMgrSendCmdGetResponse(xFpgaCmd.u8CmdValue, xFpgaCmd.u8OperandsBuff, xFpgaCmd.u8OperandsCount,
                                            u8UpdateBuff, xFpgaCmd.u8ReadDataCount);
        /* Is it Success? */
        if (bStatus)
        {
            Log(ERR, "FpgaMgr: UpdateFeatureBits, Send Cmd, LSC_READ_FEABITS [0xFB]: Failed! ");
            break;
        }
        else
        {
            /* Yes, Update the Read Result Variable */
            u16FeatureBitsRead = ((u8UpdateBuff[FPGA_DATA_0] << FPGA_ONE_BYTE_SHIFT)  | u8UpdateBuff[FPGA_DATA_1]);
        }

        /* Delay a While  */
        OSTimeDly(MSEC_5);

        Log(DEV, "FpgaMgr: UpdateFeatureBits, Send Cmd, LSC_READ_FEABITS [0xFB]: Success");
        Log(DEV, "FpgaMgr: UpdateFeatureBits, Read FEABITS = 0x%04X", u16FeatureBitsRead);

        /* Check if the feature bits are same */
        if (u16FeatureBitsRead != u16FeatureBitsToUpdate)
        {
            Log(ERR, "FpgaMgr: UpdateFeatureBits, FEABITS Read different from Written: Failed");
            break;
        }

        Log(REQ, "FpgaMgr: UpdateFeatureBits: Success");

        /* Set Success */
        eFpgaMgrStatus = FPGA_MGR_OK;

    } while (false);

    /* If the Fpga is in Programming mode, exit from it */
    if (bIsFpgaInProgrammingMode)
    {
        if (!FpgaMgrExitProgrammingMode())
        {
            Log(DBG, "FpgaMgr: UpdateFeatureBits, Send Cmd, ISC_PROGRAM_DONE [0x5E]: Success");
            Log(DBG, "FpgaMgr: UpdateFeatureBits, Send Cmd, ISC_DISABLE [0x26]: Success");
        }
        /* else not required */
    }

    /* All Good, Lets do a FPGA Refresh */
    if (FpgaMgrPerformFPGARefresh())
    {
        Log(ERR, "FpgaMgr: UpdateFeatureBits, FpgaMgrPerformFPGARefresh: Failed");

        /* Set to Status */
        eFpgaMgrStatus = FPGA_MGR_REFRESH_FAILED;
    }
    else
    {
        Log(DBG, "FpgaMgr: UpdateFeatureBits, Send Cmd, LSC_REFRESH [0x79]: Success");
    }

    Log(REQ, "FpgaMgr: UpdateFeatureBits, End of FPGA FeatureBits Update");

    /* Update the Global Status Variable */
    meFpgMgrStatus = eFpgaMgrStatus;

    return eFpgaMgrStatus;
}

/* ========================================================================== */
/**
* \brief   Updates the FPGA Config and UFM.
*
* \details This function updates the FPGA Configuration Flash and UFM.
*
* \param   pJedInfo - pointer to the Jed file [update file] info.
* \param   eMemArea - Update Memory Area
*
* \return  FPGA_MGR_STATUS - Fpga Manager Status return
* \retval      FPGA_MGR_OK              - Fpga No Error.
* \retval      FPGA_MGR_INVALID_PARAM   - Any Invalid Parameter error.
* \retval      FPGA_MGR_REFRESH_FAILED  - Fpga Refresh Failed
* \retval      FPGA_MGR_PROGRAM_FAILED  - Fpga Program Failed.
*
* ========================================================================== */
FPGA_MGR_STATUS L3_FpgaMgrUpdateFPGA(MACHX02 *pJedInfo, FPGA_MEM_AREA eMemArea)
{
    FPGA_MGR_STATUS eFpgaMgrStatus;             /* Fpga Status Variable */
    bool            bIsFpgaInProgrammingMode;   /* True, if FPGA in Programming Mode, false otherwise */

    do
    {   /* Set to defaults */
        eFpgaMgrStatus = FPGA_MGR_PROGRAM_FAILED;
        bIsFpgaInProgrammingMode = false;

        /* Check the input Validity */
        if (NULL == pJedInfo)
        {
            eFpgaMgrStatus = FPGA_MGR_INVALID_PARAM;
            Log(ERR, "FpgaMgr: UpdateFPGA, Null parameter Passed");
            break;
        }

        Log(REQ, "FpgaMgr: UpdateFPGA, Starting FPGA programming...");

        /* Try to Enter FPGA programming mode */
        if (FpgaMgrEnterProgrammingMode())
        {
            break;
        }

        /* FPGA entered into Programming mode */
        bIsFpgaInProgrammingMode = true;

        Log(DBG, "FpgaMgr: UpdateFPGA, EnterProgrammingMode ISC_ENABLE_X [0x74]: Success");

        /* Based on the Input Memory Area, Erase the FPGA Memory */
        if (FpgaMgrEraseMemory(eMemArea))
        {
            break;
        }

        Log(DBG, "FpgaMgr: UpdateFPGA, EraseMemory [0x%02X]: Success", eMemArea);

        /* Erase Success, Lets wait a for Busy Flag to Clear */
        if (FpgaMgrCheckBusyFlagAfterErase())
        {
            break;
        }

        Log(DBG, "FpgaMgr: UpdateFPGA, FpgaMgrCheckBusyFlagAfterErase: Success");

        /* Do we need FPGA Config Memory Area Update? */
        if (IS_CONFIG_MEM_SET(eMemArea))
        {
            /* Yes, Update the FPGA Config Memory Area */
            if(FpgaMgrUpdateFpgaMemory(pJedInfo, FPGA_CONFIG))
            {
                Log(ERR, "FpgaMgr: UpdateFPGA, FpgaMgrUpdateFpgaMemory, FPGA_CONFIG: Failed");
                break;
            }

            Log(REQ, "FpgaMgr: UpdateFPGA, FpgaMgrUpdateFpgaMemory, FPGA_CONFIG: Success");
        }

        /* Do we need FPGA User Flash Memory Area Update? */
        if (IS_UFM_MEM_SET(eMemArea))
        {
            /* Yes, Update the FPGA User Flash Memory Area */
            if(FpgaMgrUpdateFpgaMemory(pJedInfo, FPGA_UFM))
            {
                Log(ERR, "FpgaMgr: UpdateFPGA, FpgaMgrUpdateFpgaMemory, FPGA_UFM: Failed");
                break;
            }

            Log(REQ, "FpgaMgr: UpdateFPGA, FpgaMgrUpdateFpgaMemory, FPGA_UFM: Success");
        }

        Log(REQ, "FpgaMgr: UpdateFPGA, FPGA Programming: Success");

        /* Set Success */
        eFpgaMgrStatus = FPGA_MGR_OK;

    } while (false);

    /* If the Fpga is in Programming mode, exit from it */
    if (bIsFpgaInProgrammingMode)
    {
        if (!FpgaMgrExitProgrammingMode())
        {
            Log(DBG, "FpgaMgr: UpdateFPGA, Send Cmd, ISC_PROGRAM_DONE [0x5E]: Success");
            Log(DBG, "FpgaMgr: UpdateFPGA, Send Cmd, ISC_DISABLE [0x26]: Success");
        }
        /* else not required */
    }

    /* All Good, Lets do a FPGA Refresh, If we Erase SRAM, this reloads it. */
    if (FpgaMgrPerformFPGARefresh())
    {
        Log(ERR, "FpgaMgr: UpdateFPGA, FpgaMgrPerformFPGARefresh: Failed");

        /* Set to Status */
        eFpgaMgrStatus = FPGA_MGR_REFRESH_FAILED;
    }
    else
    {
        Log(DBG, "FpgaMgr: UpdateFPGA, Send Cmd, LSC_REFRESH [0x79]: Success");
    }

    Log(REQ, "FpgaMgr: UpdateFPGA, End of FPGA Programming...");

    /* Update the Global Status Variable */
    meFpgMgrStatus = eFpgaMgrStatus;

    return eFpgaMgrStatus;
}

/* ========================================================================== */
/**
* \brief   Initializes the Signia Layer 3 Fpga Manager.
*
* \details Initializes Fpga Manager.
*
* \param   < None >
*
* \return  FPGA_MGR_STATUS - Fpga Manager Status return
* \retval      FPGA_MGR_OK              - Fpga No Error.
* \retval      FPGA_MGR_ERROR           - For any Generic Error
* \retval      FPGA_MGR_REFRESH_FAILED  - Fpga Refresh Failed
*
* ========================================================================== */
FPGA_MGR_STATUS L3_FpgaMgrInit(void)
{
    I2CControl      I2cConfig;      /* I2C Configuration Argument Block */
    I2C_STATUS      eI2cStatus;     /* I2C Status Variable */
    FPGA_MGR_STATUS eFpgaMgrStatus; /* Fpga Status Variable */

    do
    {   /* Start with Error status */
        eFpgaMgrStatus = FPGA_MGR_ERROR;

        /* FPGA I2C Configuration */
        I2cConfig.AddrMode = I2C_ADDR_MODE_7BIT;
        I2cConfig.Clock = I2C_CLOCK_312K;
        I2cConfig.Device = FPGA_SLAVE_ADDRESS;
        I2cConfig.State = I2C_STATE_ENA;
        I2cConfig.Timeout = FPGA_I2C_TIMEOUT;

        /* Configure FPGA I2C Channel */
        eI2cStatus = L3_I2cConfig(&I2cConfig);

        /* Configure FPGA I2C Channel */
        if(I2C_STATUS_SUCCESS != eI2cStatus)
        {
            Log(ERR, "FpgaMgr: Init: L3_I2cConfig() Failed, I2C Status = %d", eI2cStatus);
            break;
        }

        /* Make sure the Fpga is not in Sleep */
        if(FPGA_MGR_OK != L3_FpgaMgrSleepEnable(false))
        {
            Log(ERR, "FpgaMgr: Init: L3_FpgaMgrSleepEnable(), Failed");
            break;
        }

        /* Assert the FPGA's Program N pin to load its program */
        if(FpgaMgrAssertProgramNSignal())
        {
            Log(ERR, "FpgaMgr: Init: FpgaMgrAssertProgramNSignal Failed");
            break;
        }

        /// \todo 02/28/2022 DAZ - An FpgaRefresh takes about 5.6mS. Why 500mS here?
        /* Wait a while */
        OSTimeDly(MSEC_500);

        /* Set the GPIO_FPGA_SPI_RESET */
        L3_GpioCtrlSetSignal(GPIO_FPGA_SPI_RESET);

        /* Wait until the FPGA has finished */
        OSTimeDly(MSEC_10);

        /* Call FPGA Refresh */
        if(FpgaMgrPerformFPGARefresh())
        {
            Log(ERR, "FpgaMgr: Init: FpgaMgrPerformFPGARefresh Failed");

            /* If we dont have a valid FPGA program, this refresh fails */
            eFpgaMgrStatus = FPGA_MGR_REFRESH_FAILED;
        }
        else
        {
            /* Return Success */
            eFpgaMgrStatus = FPGA_MGR_OK;
        }

        (void) L3_GpioCtrlClearSignal(GPIO_PZT_EN);          ///<  Enable Piezo Audio

        Log(REQ, "FpgaMgr: Initialized");

    } while (false);

    /* Update the Global Status Variable */
    meFpgMgrStatus = eFpgaMgrStatus;

    return eFpgaMgrStatus;
}

/* ========================================================================== */
/**
* \brief   Layer 3 Fpga Manager Check Status.
*
* \details Returns the Fpga Current Status.
*
* \param   < None >
*
* \return  FPGA_MGR_STATUS - Fpga Manager Status return
* \retval      FPGA_MGR_OK              - Fpga No Error.
* \retval      FPGA_MGR_INVALID_PARAM   - Any Invalid Parameter error.
* \retval      FPGA_MGR_REFRESH_FAILED  - Fpga Refresh Failed
* \retval      FPGA_MGR_PROGRAM_FAILED  - Fpga Program Failed.
* \retval      FPGA_MGR_ERROR           - Any other Generic Error.
*
* ========================================================================== */
FPGA_MGR_STATUS L3_FpgaMgrCheckStatus(void)
{
    return meFpgMgrStatus;
}

/* ========================================================================== */
/**
* \brief   Layer 3 Fpga Manager Enable Sleep Mode
*
* \details This function puts the FPGA to sleep based on the input
*
* \param   bEnable - If 'True' puts the FPGA to sleep, 'False' otherwise
*
* \return  FPGA_MGR_STATUS - Fpga Manager Status return
* \retval      FPGA_MGR_OK    - If GPIO operation is success.
* \retval      FPGA_MGR_ERROR - If GPIO operation Fails.
*
* ========================================================================== */
FPGA_MGR_STATUS L3_FpgaMgrSleepEnable(bool bEnable)
{
    GPIO_STATUS     eGpioStatus;    /* Gpio Status Variable */
    FPGA_MGR_STATUS eFpgaMgrStatus; /* Fpga Status Variable */

    do
    {   /* Start with Error status */
        eFpgaMgrStatus = FPGA_MGR_ERROR;

        if(bEnable)
        {
            /* Set the FPGA Sleep GPIO Signal. */
            eGpioStatus = L3_GpioCtrlSetSignal(GPIO_FPGA_SLEEP);
        }
        else
        {
            /* Clear the FPGA Sleep GPIO Signal. */
            eGpioStatus = L3_GpioCtrlClearSignal(GPIO_FPGA_SLEEP);
        }

        if(GPIO_STATUS_OK != eGpioStatus)
        {
            Log(ERR, "FpgaMgr: SleepEnable, GPIO_FPGA_SLEEP '%s' Failed, Gpio Status = %d", (bEnable? "Set" : "Clear"), eGpioStatus);
            break;
        }
        else
        {
            Log(REQ, "FpgaMgr: SleepEnable, GPIO_FPGA_SLEEP, %s", (bEnable? "Enabled" : "Disabled"));
        }

        /* Return Success */
        eFpgaMgrStatus = FPGA_MGR_OK;

    } while (false);

    return eFpgaMgrStatus;
}

/* ========================================================================== */
/**
 * \brief   Refresh FPGA
 *
 * \details This function is called when there has been an FPGA SPI communcations
 *          error in the motor servo. This function attempts to clear the error
 *          by reloading the FPGA by sending a Refresh command via the I2C bus.
 *
 * \note    This function does not toggle the ProgramN pin. That is performed by
 *          L3_FpgaMgrReset. (As opposed to Refresh)
 *
 *          This function may incur significant delays as it may have to wait
 *          for the I2C bus.
 *
 * \param   < None >
 *
 * \return  bool - Error status: true if error, false if success
 *
 * ========================================================================== */
bool L3_FpgaMgrRefresh(void)
{
    return FpgaMgrPerformFPGARefresh();
}

/* ========================================================================== */
/**
 * \brief   Reset FPGA
 *
 * \details This function is called before each motor move to insure that the
 *          FPGA is reset. This is done by toggling the ProgramN pin, forcing the
 *          FPGA to reload RAM.
 *
 * \param   < None >
 *
 * \return  bool - Error status: true if error, false if success
 *
 * ========================================================================== */
bool L3_FpgaMgrReset(void)
{
    return FpgaMgrAssertProgramNSignal();
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

