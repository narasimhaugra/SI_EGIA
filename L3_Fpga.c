#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_FPGA
 * \{
 * \brief   Support routines for reading/writing FPGA registers (motor/piezo control),
 *          and downloading code to FPGA.
 *
 * \details The L3_FpgaReadxxx functions return the shadow register value in RAM. This
 *          is the last known value, and assumes that the read function is being called
 *          periodically.
 * \n \n
 *          The L3_FpgaWritexxx functions update the shadow register, initiate transfer over the
 *          L2_SPI0 interface. The SPI0 interrupt posts the semaphore upon completion.
 *          In the FPGA task, registers are 1st read then written with successful posting and release
 *          of the L2_SPI semaphore to advance the transfer.
 * \n \n
 *          As a consequence of this (read before write), if a register is scheduled to be both
 *          read and written, the FPGA task ordering ensures the read operation completes before
 *          the write to avoid overwriting the value scheduled to be written.
 *
 * \sa      Lattice MachXO2 Programming and Configuration Usage Guide
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_Fpga.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include <Common.h>
#include "L2_Spi.h"
#include "L3_Fpga.h"
#include "L3_FpgaMgr.h"
#include "L3_Motor.h"
#include "uC-CRC/Source/edc_crc.h"
#include "L3_GpioCtrl.h"
#include "TestManager.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
#define FPGA_CTRL_TASK_STACK      (512u)       ///< FPGA Controller task stack size

OS_STK   FpgaControllerTaskStack[FPGA_CTRL_TASK_STACK + MEMORY_FENCE_SIZE_DWORDS]; /* Stack for fpga controller task */
/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER      LOG_GROUP_FPGA  ///< Identifier for log entries

#define MAX_TRANSFER_WAIT         (2u)          ///< Maximum write/read transfer wait duration

#define FPGA_REG_SIZ_HEADER       (4u)          ///< Size of Register packet Header
#define FPGA_MAX_ERRORS           (0xFFFFu)     ///< Max FPGA Errors
#define FPGA_BYTE_MASK            (0xFFu)       ///< Byte Mask
#define FPGA_BYTE_SHIFT           (8u)          ///< Byte shift
#define FPGA_RESET_DELAY          (20u)         ///< Delay for Allegro Stabilizing after RESET (mSec)

#define FPGA_BUFFER_MAX   ((FPGA_REG_COUNT * 7) + 67)   ///< Maximum possible buffer size; possibly read all registers at once
#define FPGA_TX_ADDRESS_OFFSET    (0u)          ///< Offset in TX buffer of the byte containg the register address

#define REG_BYTE                    (1u)        ///< Size of byte register
#define REG_WORD                    (2u)        ///< Size of word register
#define REG_LONG                    (4u)        ///< Size of long register
#define FPGA_SYNC_TIME              (3u)        ///< Milliseconds to sync with FPGA read/write cycle
#define FPGA_SERVO_TIME             (1000u)     ///< FPGA servo interrupt rate in uSec

#define FPGA_SPI_WR_BIT             (0x80)      ///< SPI: MSB of FPGA register address byte is the W/R bit
#define FPGA_SPI_CRC_SIZE           (2u)        ///< Size of FPGA message packet CRC in bytes
#define FPGA_SPI_ADDR_SIZE          (1u)        ///< Size of FPGA register address in bytes
#define FPGA_READ_REG_REQ_SIZE      (4u)        ///< Size of register read request (ADDR + 1 + CRC sizes)
#define FPGA_SPI_WR_ACK             (0x0Au)     ///< FPGA Register write acknowledge

#define FPGA_RETRY_MAX              (10u)       ///< Number of CRC errors before signaling error on register read
#define FPGA_MAX_WRITE_NACKS        (10u)       ///< Number of write NACKS before readReg returns error
#define FPGA_ERR_LOG_REG_SIZE       (4u)        ///< Size of FPGA error log register data
#define FPGA_MAX_ERR_LOG            (20u)       ///< Max number of local FPGA error log entries
#define FPGA_COMM_ERROR_MARGIN      (2u)        ///< FPGA communication error margin

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef struct          /* The fpga static map defines the starting address and number of bytes for each register in the fpga */
{
    uint8_t Adr;        ///< Adr (byte 0-255) of the start of the data for this register
    uint8_t RegSize;    ///< Register size in bytes
} FPGA_REG_STATIC;

typedef struct          ///< The fpga register data for DMA transfers to the FPGA
{
    bool ReadRequest;   ///< When true, the fpga DMA read request will be sent for this register at next FPGA PIT ISR. Cleared when read successful.
    bool WriteRequest;  ///< When true, the fpga DMA  write request will be sent at next FPGA PIT ISR. Cleared when write succesful.
    union {             ///< FPGA registers are 1, 2 or 4 bytes, this provides acess to those values
        uint8_t  Val8;  ///< 8 bit register access
        uint16_t Val16; ///< 16 bit register access
        uint32_t Val32; ///< 32 bit register access
    };
    uint16_t ReadCRC;   ///< CRC of data for read request is saved here, sent to FPGA as last 2 bytes of the packet.
    uint16_t WriteCRC;  ///< CRC of data for write request is saved here, sent to FPGA as last 2 bytes of the packet.
    uint8_t  CrcError;  ///< This count is incremented at each register transfer failure, and cleared when transfer is successful
} FPGA_REG_DYNAMIC;

/* ERROR CODES FOR FPGA COM TEST */
typedef enum
{
    FPGA_COMM_ERROR_NONE,               ///< No error
    FPGA_COMM_ERROR_RX_NO_TX,           ///< Data received w/o transmission
    FPGA_COMM_ERROR_RX_ADR_SYNC,        ///< Read address in sync between Rx / Tx buffers
    FPGA_COMM_ERROR_RX_WR_BIT,          ///< Wr bit set on read
    FPGA_COMM_ERROR_RX_CRC,             ///< CRC error
    FPGA_COMM_ERROR_WR_NO_TX,           ///< Not used
    FPGA_COMM_ERROR_WR_ADR_SYNC,        ///< Write address not in sync between Rx / Tx buffers
    FPGA_COMM_ERROR_WR_BIT,             ///< Write bit not found
    FPGA_COMM_ERROR_WR_ACK,             ///< Write ACK not received
    FPGA_COMM_MISSED_DMA,               ///< Missed DMA transfer (processing overrun)
    FPGA_COMM_DMA_STOP,                 ///< DMA stall (processing overrun)
    FPGA_COMM_DMA_BUSY,                 ///< DMA busy (unused)
    FPGA_SPIO_START_ERROR               ///< SPI start error (unused)
} FPGA_COMM_ERROR;

/* FPGA Error log strcuture */
typedef struct                              ///< FPGA error log entry
{
    uint8_t  TxRegAdr;                      ///< FPGA register address transmitted to FPGA
    uint8_t  RxRegAdr;                      ///< FPGA register address echoed from FPGA
    uint8_t  RegVal[FPGA_ERR_LOG_REG_SIZE]; ///< FPGA register value read
    uint16_t RcvCRC;                        ///< Received CRC
    uint16_t CalcCRC;                       ///< Calculated CRC
    uint8_t  ErrCode;                       ///< FPGA com test error codes
    uint8_t  TotalWrAckErrors;              ///< Write ACK errors
    uint8_t  TotalCrcErrors;                ///< CRC error count
    uint32_t TimerError;                    ///< Timer error
} FPGA_ERROR_REC;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
static bool FpgaRefreshRequest = false; // True if FPGA request pending

static const CRC_MODEL_16 CrcModel =    /* CRC-16 calculation parameter block */
{
    0x8005u,                    // Polynomial
    0xFFFFu,                    // Initial value (Not 0 like in micrium!)
    DEF_NO,                     // Do not reflect value (MS/LS bit swap)
    0x0000u,                    // Xor final CRC with this value. (No inversion)
    &CRC_TblCRC16_8005[0]       // CRC calculation table
};

/* The lookup table translates FGPA register address to enumerated Command Ids.
   Enumerated command IDs are used in code to make register access simple.
*/
static const uint8_t AdrToIdx[] =
{
    0,   0, 0,  0,  1,  2,  0,  3,  0,  0,  0,  0,  0, 0,  0,  0,   /* 0x00 - 0x0F */
    4,   5, 0,  6,  0,  0,  7,  0,  8,  0,  0,  0,  9, 0,  0,  0,   /* 0x10 - 0x1F */
    11, 12, 0, 13,  0,  0, 14,  0, 15,  0,  0,  0, 16, 0,  0,  0,   /* 0x20 - 0x2F */
    18, 19, 0, 20,  0,  0, 21,  0, 22,  0,  0,  0, 23, 0,  0,  0,   /* 0x30 - 0x3F */
    10,  0, 0,  0, 17,  0,  0,  0, 24,  0,  0,  0, 25, 0,  0,  0,   /* 0x40 - 0x4F */
    26,  0, 0,  0                                                   /* 0x50 - 0x53 */
};

/* CommandId -> Address Lookup table */
static const FPGA_REG_STATIC IdxToAdr[FPGA_REG_COUNT] =
{
    /* Register address, Length */
    { 0x00, 4 },  /* FPGA_REG_SW_VERSION */
    { 0x04, 1 },  /* FPGA_REG_HW_VERSION */
    { 0x05, 2 },  /* FPGA_REG_PIEZO_PWM */
    { 0x07, 1 },  /* FPGA_REG_CONTROL */

    { 0x10, 1 },  /* FPGA_REG_MOT0_CONTROL */
    { 0x11, 2 },  /* FPGA_REG_MOT0_CURR_PWM */
    { 0x13, 2 },  /* FPGA_REG_MOT0_VEL_PWM */
    { 0x16, 1 },  /* FPGA_REG_MOT0_STATUS */
    { 0x18, 4 },  /* FPGA_REG_MOT0_POSITION */
    { 0x1C, 4 },  /* FPGA_REG_MOT0_PERIOD */
    { 0x40, 4 },  /* FPGA_REG_DELTA_COUNT_MOTOR_0, Part of MISC group of register set (0x4x)  */

    { 0x20, 1 },  /* FPGA_REG_MOT1_CONTROL */
    { 0x21, 2 },  /* FPGA_REG_MOT1_CURR_PWM */
    { 0x23, 2 },  /* FPGA_REG_MOT1_VEL_PWM */
    { 0x26, 1 },  /* FPGA_REG_MOT1_STATUS */
    { 0x28, 4 },  /* FPGA_REG_MOT1_POSITION */
    { 0x2C, 4 },  /* FPGA_REG_MOT1_PERIOD */
    { 0x44, 4 },  /* FPGA_REG_DELTA_COUNT_MOTOR_1, Part of MISC group of register set (0x4x)  */

    { 0x30, 1 },  /* FPGA_REG_MOT2_CONTROL */
    { 0x31, 2 },  /* FPGA_REG_MOT2_CURR_PWM */
    { 0x33, 2 },  /* FPGA_REG_MOT2_VEL_PWM */
    { 0x36, 1 },  /* FPGA_REG_MOT2_STATUS */
    { 0x38, 4 },  /* FPGA_REG_MOT2_POSITION */
    { 0x3C, 4 },  /* FPGA_REG_MOT2_PERIOD */
    { 0x48, 4 },  /* FPGA_REG_DELTA_COUNT_MOTOR_2, Part of MISC group of register set (0x4x)  */

    { 0x4C, 1 },  /* FPGA_REG_OK */
    { 0x50, 4 },  /* FPGA_REG_BAD_CRC_COUNT */
};

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static OS_EVENT *pSemFpgaWriteDone = NULL;                  /* Semaphore used to FPGA write done. */
static OS_EVENT *pSemFpgaReadDone = NULL;                   /* Semaphore used to FPGA read done. */

static uint32_t FpgaWriteErrorCount = 0;                    /* Count of FPGA register write errors */

static uint8_t  RegRdTxPkts;                                /* Number of register read  packets to transmit to FPGA */
static uint8_t  RegWrTxPkts;                                /* Number of register write packets to transmit to FPGA */
static uint16_t FpgaDataSetErrors;                          /* When an FPGA CRC error occurs this is set to a non-zero value */


uint8_t  FpgaTxBuffer[FPGA_BUFFER_MAX + MEMORY_FENCE_SIZE_BYTES];              /* SPI buffer for transmit to FPGA */

uint8_t  FpgaRxBuffer[FPGA_BUFFER_MAX + MEMORY_FENCE_SIZE_BYTES];              /* SPI buffer for receive from FPGA */

//#pragma location=".sram"
static FPGA_REG_DYNAMIC FpgaDynamicMap[FPGA_REG_COUNT];     /* FPGA shadow register allocation */

/* FPGA error log register: Debug logs are time consuming, hence logged in RAM */
//#pragma location=".sram"
static FPGA_ERROR_REC FpgaErrorLog[FPGA_MAX_ERR_LOG];       /* FPGA error log register */
static uint8_t  FpgaErrorCount = 0;                         /* FPGA error count */

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void FpgaControllerTask(void *pArg);
static bool FpgaStartReadDataSet(void);
static bool FpgaStartWriteDataSet(void);
static bool FpgaProcessReadDataSet(void);
static void FpgaErrorLogAdd(FPGA_ERROR_REC *pNewFpgaErr);
static void FpgaReadDoneEvent(void);
static void FpgaWriteDoneEvent(void);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Compose consolidated read request packet stream
 *
 * \note    If a register has its WriteRequest flag set as well as the ReadRequest
 *          flag, the read request packet will NOT be generated to ensure that the
 *          value to write is not overwritten by the read operation.
 * \n \n
 *          This could be handled by having separate members in the FpgaDynamicMap,
 *          but it is considered to be a rare occurrence (if ever), and does not
 *          warrant the extra storage.
 *
 * \param   < None >
 *
 * \return  true if DMA transfer started (data to read)
 *
 * ========================================================================== */
static bool FpgaStartReadDataSet(void)
{
    uint8_t   Idx;             /* Register index */
    uint16_t  SpiBufIdx;       /* DMA transmit buffer index */
    SPIIO     SpiPacket;       /* SPI I/O packet */
    uint8_t   IndexResp;
    uint16_t  SpiPktIdx;       /* Index for filling SPI packet */
    bool      firstPacket;     /* Flag first packet in DMA transfer */

    SpiBufIdx = 0;
    RegRdTxPkts = 0;    /* Reset number of transactions */
    FpgaDynamicMap[FPGA_REG_HW_VERSION].ReadRequest = true;    /* Always force a read */
    firstPacket = true;

    /* Go through each register and see if read is requested (and write is not): */

    for (Idx = 0; Idx < FPGA_REG_COUNT; Idx++)
    {
        SpiPktIdx = SpiBufIdx;
        if ((FpgaDynamicMap[Idx].ReadRequest) && (!FpgaDynamicMap[Idx].WriteRequest))
        {
            /* Read request packet format:
                    Register Address (MSB = 0 for read)
                    Null
                    CRC 16 of above
                    1 Null to clock in address
                    n Nulls to clock in data - 1, 2 or 4 bytes
                    2 Nulls to clock in CRC16 of data returned */

            /* Send register address followed by null byte, followed by CRC: */
            FpgaTxBuffer[SpiBufIdx++] = IdxToAdr[Idx].Adr;    /* Register address */
            FpgaTxBuffer[SpiBufIdx++] = 0;                    /* Null */

            /* add CRC (pre-calculated) to message */
            FpgaTxBuffer[SpiBufIdx++] = FpgaDynamicMap[Idx].ReadCRC >> FPGA_BYTE_SHIFT;
            FpgaTxBuffer[SpiBufIdx++] = FpgaDynamicMap[Idx].ReadCRC & FPGA_BYTE_MASK;

            /* Filler 8 bits for ADDR echo on MISO from slave (fpga) */
            FpgaTxBuffer[SpiBufIdx++] = 0;
            /*  Filler zeros for data response */
            for (IndexResp = 0; IndexResp < IdxToAdr[Idx].RegSize; IndexResp++)
            {
                FpgaTxBuffer[SpiBufIdx++] = 0;
            }

            /* filler zeros for crc */
            FpgaTxBuffer[SpiBufIdx++] = 0;
            FpgaTxBuffer[SpiBufIdx++] = 0;

            L2_Spi0TxPacket(firstPacket, SpiBufIdx - SpiPktIdx, &FpgaTxBuffer[SpiPktIdx]);
            firstPacket = false;    /* first packet has been buffered */

            RegRdTxPkts++;          /* Increment number of packets */
        }
    }

    /* If anything has been packetized, then start DMA transfer */
    if (SpiBufIdx > 0)
    {
        SpiPacket.SpiPort = SPI_PORT_ZERO;
        SpiPacket.pSpiTxData = (uint8_t *)FpgaTxBuffer;
        SpiPacket.pSpiRxData = FpgaRxBuffer;
        SpiPacket.Nbytes     = SpiBufIdx;
        SpiPacket.pCallback  = &FpgaReadDoneEvent;
        L2_SpiDataIO(&SpiPacket);
    }

    return (SpiBufIdx > 0);     // Return true if DMA transfer started
}

/* ========================================================================== */
/**
 * \brief   Compose consolidated write request packet stream
 *
 * \details For all registers that have their WriteRequest flag set, prepare write
 *          packets, add a CRC to the end of each packet & start sending it to the
 *          FPGA.
 *
 * \param   < None >
 *
 * \return  true if DMA transfer started (data to write)
 *
 * ========================================================================== */
static bool FpgaStartWriteDataSet(void)
{
    uint8_t   Idx;                          /* FPGA register index */
    uint16_t  SpiBufIdx;                    /* DMA buffer index */
    uint16_t  SpiPktIdx;                    /* Index for filling SPI packet */
    uint16_t  Crc;                          /* CRC result */
    EDC_ERR   Err;                          /* CRC error code */
    uint8_t   WrData[sizeof(uint32_t) + 1]; /* Buffer for calculating CRC of reg address & data */
    uint8_t   DataType;                     /* Register address */
    uint8_t   *pReg;                        /* Pointer to register data to write */
    uint8_t   DataSize;                     /* Size of register data to write */
    uint8_t   Counter;                      /* Temp counter for CRC calc */
    bool      firstPacket;                  /* Flag first packet in DMA transfer */
    SPIIO     SpiPacket;                    /* SPI I/O packet */

    Idx = 0;            /* Reset register index */
    SpiBufIdx = 0;      /* Reset dma buffer index */
    RegWrTxPkts = 0;    /* Initialize packet counter */
    firstPacket = true;

    /* Go through each register and see if write is requested */
    for (Idx = 0; Idx < FPGA_REG_COUNT; Idx++)
    {
        SpiPktIdx = SpiBufIdx;      /* Reset packetizer index */
        if (FpgaDynamicMap[Idx].WriteRequest == true)
        {
            RegWrTxPkts++;                                  /* Bump packet count */
            DataType = IdxToAdr[Idx].Adr;                   /* Get register address */
            pReg = (uint8_t *)&FpgaDynamicMap[Idx].Val8;    /* Get pointer to register data */
            DataSize = IdxToAdr[Idx].RegSize;               /* Get register data size */

            /* Create write data packet: */
            WrData[0] = FPGA_SPI_WR_BIT | DataType;         /* Set Write bit in register address */

            for (Counter = DataSize; Counter; Counter--)                      /* switch endian-ness when copying over */
            {
                WrData[Counter] = *pReg;
                pReg++;
            }

            /* Calculate CRC for write data packet: */
            Crc = CRC_ChkSumCalc_16Bit((CRC_MODEL_16 *)&CrcModel, WrData, sizeof(uint8_t) * (DataSize + 1), &Err);

            /* Move write data packet to transmit buffer followed by CRC: */
            for (Counter = 0; Counter <= DataSize; Counter++)
            {
                FpgaTxBuffer[SpiBufIdx++] = WrData[Counter];
            }

            /* add CRC to message */
            FpgaTxBuffer[SpiBufIdx++] = Crc >> FPGA_BYTE_SHIFT;
            FpgaTxBuffer[SpiBufIdx++] = Crc & FPGA_BYTE_MASK;

            /* filler 8 bits for ADDR echo on MISO from slave (fpga) */
            FpgaTxBuffer[SpiBufIdx++] = 0;

            /* filler zero for response  (CRC return is ACK) */
            FpgaTxBuffer[SpiBufIdx++] = 0;
            L2_Spi0TxPacket(firstPacket, SpiBufIdx - SpiPktIdx, &FpgaTxBuffer[SpiPktIdx]);
            firstPacket = false;                            /* first packet has been buffered */
        }
    }

    /* If anything is requested, then start DMA transfer */
    if (SpiBufIdx > 0)
    {
        SpiPacket.SpiPort = SPI_PORT_ZERO;
        SpiPacket.pSpiTxData = (uint8_t *)FpgaTxBuffer;
        SpiPacket.pSpiRxData = FpgaRxBuffer;
        SpiPacket.Nbytes     = SpiBufIdx;
        SpiPacket.pCallback  = &FpgaWriteDoneEvent;
        L2_SpiDataIO(&SpiPacket);
    }

    return (SpiBufIdx > 0); // Return true if DMA transfer started
}

/* ========================================================================== */
/**
 * \brief   Process the response to read packets sent to the FPGA.
 *
 * \details The response is validated and parsed, and the data returned is sent
 *          into the FPGA shadow FPGA shadow registers (pointed to by FpgaDynamicMap).
 *
 * \sa      R0042522 FPGA Software Design Document
 *
 * \param   < None >
 *
 * \return  bool - Error status: False if data processed OK. True if no data read.
 *
 * ========================================================================== */
static bool FpgaProcessReadDataSet(void)
{
    bool            Status;                     /* Operation Status */
    uint16_t        SpiBufIdx;                  /* Receive buffer index */
    uint16_t        FpgaAddrIdx;                /* Index of transmitted register adcdress */
    uint16_t        Crc;                        /* Received packet CRC */
    uint16_t        CalcCRC;                    /* Calculated packet CRC */
    EDC_ERR         Err;                        /* CRC calc error status */
    uint8_t         Idx;                        /* Response buffer processing index */
    uint8_t         DataSize;                   /* Register size in bytes */
    uint8_t         DataOffset;                 /* Index of start of data in Rx'd packet */
    bool            FpgaCrcErrorFound;          /* Set if one or more CRC errors found. (Could be caused by sync problem) */
    bool            FpgaResponseOutOfSync;      /* If TRUE, the response in the receive buffer does not correspond to the transmitted command, addresses do not match. */
    uint8_t         StartOfDataIdx;             /* Index of start of received register data */
    uint16_t        FpgaEchoAdr;                /* FPGA register address in FPGA response */
    FPGA_ERROR_REC  FpgaError;                  /* FPGA error record */

    SpiBufIdx = 0;                          /* Initialize buffer index */
    FpgaAddrIdx = FPGA_TX_ADDRESS_OFFSET;   /* Initialize FPGA address index */
    FpgaCrcErrorFound = false;              /* Initialize CRC error flag */
    FpgaResponseOutOfSync = false;          /* Initialize sync flag */
    StartOfDataIdx = 0;                     /* Initialize data index */
    FpgaEchoAdr = 0;
    Status = false;                          /* Default to processing OK */

    /* Master Read transaction:
       Packet format:
            Tx Reg Addr      (1 byte w/ MSB clear)
            Tx Pad byte      (1 byte, always 0)
            Tx CRC           (2 bytes - CRC of addr & pad byte)
            Rx Reg Addr      (1 byte)
            Rx Reg Data      (1/2/4 bytes)
            Rx CRC           (2 bytes)

            In the Tx buffer, the Rx bytes have pad values.
            In the Rx buffer, the Tx bytes have pad values. */

    for (uint8_t i = 0; i < RegRdTxPkts; i++)
    {
        FpgaAddrIdx = SpiBufIdx;                                    /* Save index of FPGA register address */
        Idx = AdrToIdx[FpgaTxBuffer[SpiBufIdx]];                    /* Get Register Index from Register address in Tx buffer */
        DataSize = IdxToAdr[Idx].RegSize;                           /* Get Register size */
        SpiBufIdx += FPGA_READ_REG_REQ_SIZE;                        /* Increment index to returned data for this packet (Skip over request bytes) */

        /* Verify the register address returned with response against the one transmitted */

        FpgaEchoAdr = FpgaRxBuffer[SpiBufIdx];                      /* Get register address from FPGA response */
        if ((FpgaTxBuffer[FpgaAddrIdx] & FPGA_BYTE_MASK) != FpgaRxBuffer[SpiBufIdx])
        {
            FpgaResponseOutOfSync = true;
            if (0 == (FpgaRxBuffer[SpiBufIdx] & FPGA_SPI_WR_BIT))
            {
                FpgaError.ErrCode = FPGA_COMM_ERROR_RX_ADR_SYNC;
            }
            else
            {
                FpgaError.ErrCode = FPGA_COMM_ERROR_RX_WR_BIT;          /* This should never happen. This is a read. */
            }
        }

        StartOfDataIdx = SpiBufIdx;                                     /* Point to start of returned packet data */
        SpiBufIdx++;                                                    /* Bump to start of data (past address) */
        DataOffset = SpiBufIdx;                                         /* Save index of data */
        SpiBufIdx += DataSize;                                          /* Skip over data to CRC */

        Crc = FpgaRxBuffer[SpiBufIdx++] << FPGA_BYTE_SHIFT;             /* Extract CRC & skip index over CRC to next packet */
        Crc |= FpgaRxBuffer[SpiBufIdx++];
        CalcCRC = CRC_ChkSumCalc_16Bit((CRC_MODEL_16 *)&CrcModel, &FpgaRxBuffer[StartOfDataIdx], (DataSize + 1), &Err); /* Calc CRC of addr & data */

        if (Crc == CalcCRC && (false == FpgaResponseOutOfSync))
        {
            /* CRC OK. Extract data - switch endian-ness when copying */
            FpgaDynamicMap[Idx].ReadRequest = false;                   /* Read request has been processed */
            FpgaDynamicMap[Idx].CrcError = 0;                          /* Clear error count for good crc */
            uint8_t *pReg = (uint8_t *)&FpgaDynamicMap[Idx].Val8;
            for (uint8_t j = DataSize; j > 0; j--)
            {
                *pReg++ = FpgaRxBuffer[DataOffset + j - 1];
            }
        }
        else /* Some kind of error in data set read. Log error:  */
        {
            Status = true;                                              /* Had some kind of read error - exit bad? /// \todo 10/07/20 DAZ - */
            FpgaCrcErrorFound = true;
            if (!FpgaResponseOutOfSync)
            {
                FpgaError.ErrCode = FPGA_COMM_ERROR_RX_CRC;             /* Not out of sync. Must be CRC error */
            }
            FpgaError.TxRegAdr = FpgaTxBuffer[FpgaAddrIdx] & FPGA_BYTE_MASK;
            FpgaError.RxRegAdr = FpgaEchoAdr;
            memcpy(&FpgaError.RegVal[0], &FpgaRxBuffer[DataOffset], FPGA_ERR_LOG_REG_SIZE);
            FpgaError.RcvCRC = Crc;
            FpgaError.CalcCRC = CalcCRC;
            FpgaError.TotalWrAckErrors = FpgaWriteErrorCount;
            FpgaError.TotalCrcErrors = FpgaDataSetErrors;
            FpgaErrorLogAdd(&FpgaError);
            if (FpgaDynamicMap[Idx].CrcError <= FPGA_RETRY_MAX)
            {
                FpgaDynamicMap[Idx].CrcError += 1;
            }
        }
    }

    if (FpgaCrcErrorFound == true)
    {
        if (FpgaDataSetErrors < FPGA_MAX_ERRORS)
        {
            FpgaDataSetErrors++;
        }
    }
    else
    {
        FpgaDataSetErrors = 0;          /* No CRC errors, reset count */
    }

    return (Status);
}

/* ========================================================================== */
/**
 * \brief   Checks the response to Write packets sent to the FPGA.
 *
 * \details An acknowledge must be returned for each packet for success. The
 *          transmitted write packet(s) are used to determine where the ACK
 *          is located in the Rx buffer.
 *
 * \sa      R0042522 FPGA Software Design Document
 *
 * \param   < None >
 *
 * \return  bool - Error Status: False for success, True for retry on one or more packets
 *                        due to an error.
 *
 * ========================================================================== */
static bool FPGA_ProcessWriteDataSet(void)
{
    bool            ErrorStatus;                /* Error Status */
    uint16_t        SpiBufIdx;                  /* Receive buffer index */
    uint16_t        FpgaAddrIdx;                /* Index of transmitted register adcdress */
    uint8_t         Idx;                        /* Response buffer processing index */
    uint8_t         PacketIndex;                /* Index of packet being processed */
    uint16_t        TableIndex;                 /* Index of register to write (Register offset of packet) */
    bool            FpgaResponseOutOfSync;      /* If TRUE the response in the receive buffer does not correspond to the transmitted command, address do not match */
    FPGA_ERROR_REC  FpgaError;                  /* FPGA error record */
    uint16_t        FpgaEchoAdr;                /* FPGA register address in FPGA response */
    uint8_t         i;                          /* Misc index */

    /* Reset indices & error flags */
    ErrorStatus = false;
    SpiBufIdx = 0u;
    FpgaAddrIdx = FPGA_TX_ADDRESS_OFFSET;       /* Set index to 1st transmitted register address */
    FpgaResponseOutOfSync = false;
    FpgaEchoAdr = 0u;

    /* Master Write transaction:
       Packet format:
        Tx Reg Addr (1 byte w/MSB set)
        Tx Reg Data (1/2/4 bytes)
        Tx CRC      (2 bytes)
        Rx Reg Addr (1 byte)
        Rx CRC ACK  (1 byte - 0x0A CRC OK, reg writtten, 0xFF CRC bad, write failed)

        In the Tx buffer, the Rx bytes have pad values.
        In the Rx buffer, the Tx bytes have pad values. */

    for (PacketIndex = 0; PacketIndex < RegWrTxPkts; PacketIndex++)
    {
        /* look into packets sent to FPGA to determine where ACK is in response */
        FpgaAddrIdx = SpiBufIdx;
        TableIndex = FpgaTxBuffer[SpiBufIdx] & FPGA_BYTE_MASK;      /* Find out FPGA register address sent */
        Idx = AdrToIdx[TableIndex & ~FPGA_SPI_WR_BIT];              /* Convert register address to register index (into FpgaDynamicMap) */
                                                                    /* index to first byte of response from FPGA */
        SpiBufIdx += IdxToAdr[Idx].RegSize + FPGA_SPI_CRC_SIZE + FPGA_SPI_ADDR_SIZE;    /* Skip over echoed Tx packet. */

        /* Verify the register address returned with response */
        FpgaEchoAdr = FpgaRxBuffer[SpiBufIdx];                      /* Retrieve register address echoed */

        if ((FpgaTxBuffer[FpgaAddrIdx] & FPGA_BYTE_MASK) != FpgaEchoAdr)
        {
            FpgaResponseOutOfSync = true;

            if (FPGA_SPI_WR_BIT == (FpgaRxBuffer[SpiBufIdx] & FPGA_SPI_WR_BIT))
            {
                FpgaError.ErrCode =  FPGA_COMM_ERROR_WR_ADR_SYNC;   /* MSB's equal. Address out of sync */
            }
            else
            {
                FpgaError.ErrCode =  FPGA_COMM_ERROR_WR_BIT;        /* MSB's not equal. Write bit not set. */
            }

        }
        SpiBufIdx++;                                                /* Bump index to point to ACK byte */

        /* Is this a write and is the index valid? */
        if ((Idx < FPGA_REG_COUNT) && (FPGA_SPI_WR_BIT == (TableIndex & FPGA_SPI_WR_BIT)))
        {
            if ((FPGA_SPI_WR_ACK == FpgaRxBuffer[SpiBufIdx]) && (!FpgaResponseOutOfSync))
            {
                FpgaDynamicMap[Idx].WriteRequest = false;          /* Packet succeeded - mark register as written */
            }
            else
            {
                /* FPGA register write has not completed successfully - Out of sync or NAK. */
                ErrorStatus = true;
                if (!FpgaResponseOutOfSync)
                {
                    FpgaError.ErrCode = FPGA_COMM_ERROR_WR_ACK;                     /* Not out of sync. Must be ACK error */
                }

                FpgaError.TxRegAdr = FpgaTxBuffer[FpgaAddrIdx] & FPGA_BYTE_MASK;    /* Save register address transmitted */
                FpgaError.RxRegAdr = FpgaEchoAdr;                                   /* Save register address received */

                for (i = 0; i < FPGA_ERR_LOG_REG_SIZE; i++)
                {
                    /* Copy register data bytes to be written */
                    FpgaError.RegVal[i] = FpgaTxBuffer[FpgaAddrIdx + 1 + i] & FPGA_BYTE_MASK;
                }
                FpgaError.RcvCRC = FpgaRxBuffer[SpiBufIdx];                     /* Save CRC received */
                FpgaError.CalcCRC = FPGA_SPI_WR_ACK;                            /* Write ACK */
                FpgaError.TotalWrAckErrors = FpgaWriteErrorCount;               /* Write ACK errors */
                FpgaError.TotalCrcErrors = FpgaDataSetErrors;                   /* CRC errors */
                FpgaErrorLogAdd(&FpgaError);
            }
        }

        SpiBufIdx++;      /* Bump index to point to next packet */
    }

    return ErrorStatus;
}

/* ========================================================================== */
/**
 * \brief   Write an error record to the fpga_error_log in RAM.
 *
 * \details This log is sent to the event log should an FPGA_ReProgram occur
 *
 * \param   pNewFpgaErr - Pointer to FPGA error record to add to log
 *
 * \return  None
 *
 * ========================================================================== */
static void FpgaErrorLogAdd(FPGA_ERROR_REC *pNewFpgaErr)
{
    if  (FpgaErrorCount >=  FPGA_MAX_ERR_LOG)
    {
        FpgaErrorCount =  FPGA_MAX_ERR_LOG - 1;
    }
    FpgaErrorLog[FpgaErrorCount].TxRegAdr = pNewFpgaErr->TxRegAdr;
    FpgaErrorLog[FpgaErrorCount].RxRegAdr = pNewFpgaErr->RxRegAdr;
    memcpy(&FpgaErrorLog[FpgaErrorCount].RegVal[0], &pNewFpgaErr->RegVal[0], FPGA_ERR_LOG_REG_SIZE);
    FpgaErrorLog[FpgaErrorCount].RcvCRC = pNewFpgaErr->RcvCRC;
    FpgaErrorLog[FpgaErrorCount].CalcCRC = pNewFpgaErr->CalcCRC;
    FpgaErrorLog[FpgaErrorCount].ErrCode = pNewFpgaErr->ErrCode;
    FpgaErrorLog[FpgaErrorCount].TotalWrAckErrors = pNewFpgaErr->TotalWrAckErrors;
    FpgaErrorLog[FpgaErrorCount].TotalCrcErrors = pNewFpgaErr->TotalCrcErrors;

    if (FpgaErrorCount < FPGA_MAX_ERR_LOG)
    {
        FpgaErrorCount++;
    }
}

/* ========================================================================== */
/**
 * \brief   FPGA Controller task function
 *
 * \details The task executes all pending FPGA read and write requests.
 *          The task sleeps for about 1 mS to yeild CPU to other tasks.
 *
 * \param   pArg - Task arguments, needed by OS but unused.
 *
 * \return  None
 *
 * ========================================================================== */
static void FpgaControllerTask(void *pArg)
{
    uint8_t OsError;
    uint32_t TimeLastIter;
    uint32_t TimeNow;

    /// \todo 02/25/2022 DAZ - Really only need one semaphore here. Read & Write transfers are sequential.
    /* Create semaphores for SPI DMA completion */
    pSemFpgaReadDone = SigSemCreate(0, "FPGA-Sem-Read", &OsError);      // Semaphore to block data access while reading FPGA
    pSemFpgaWriteDone = SigSemCreate(0, "FPGA-Sem-Write", &OsError);    // Semaphore to block data access while writing FPGA

    if ((NULL == pSemFpgaReadDone) || (NULL == pSemFpgaWriteDone))
    {
        /* Semaphore not available, return error */
        Log(ERR, "FpgaControllerTask: Read/Write Semaphore Create Failed: 0x%lx, 0x%lx", pSemFpgaReadDone, pSemFpgaWriteDone);
        /* \todo: Add assertion */
    }

    TimeLastIter = SigTime();

    /* FPGA communication loop */
    while(true)
    {
        do
        {
            // Process read request queue. If queue is empty, skip queue processing.
            if (FpgaStartReadDataSet())
            {
                  // Wait until read commands are transferred, response is received
              OSSemPend(pSemFpgaReadDone, MAX_TRANSFER_WAIT, &OsError);

                  // If transfer timed out, we have a severe error.
              if( OS_ERR_NONE != OsError)
              {
                  /// \todo 09/29/2021 DAZ - This needs some additional error information from FPGA.
                      /// \todo 07/06/2022 DAZ - Request FPGA refresh if this happens?
                  Log(FLT, "FPGA Read DMA Transfer Timeout");
                  break;
              }

              TM_Hook(HOOK_MTRSERVOSTART, NULL);
              /* Process echoed data */
              FpgaProcessReadDataSet();
            }

            // Process motor servo
            L3_MotorServo();        // Process motor servos
            
            // Process write request queue.
            if (FpgaStartWriteDataSet())
            {
              /* Process write request Q.*/
              TM_Hook(HOOK_MTRSERVOEND, NULL);

              /// \todo 09/29/2021 DAZ - Only pend if DMA transfer has been started!
                  // Wait until write commands are transferred, response is received
              OSSemPend(pSemFpgaWriteDone, MAX_TRANSFER_WAIT, &OsError);

                  // If transfer timed out, we have a severe error.
              if( OS_ERR_NONE != OsError)
              {
                  /* Transfer did not complete in time, abort remaining transfer with error */
                  /// \todo 09/29/2021 DAZ - This needs some additional error information from FPGA.
                      /// \todo 07/06/2022 DAZ - Request FPGA refresh if this happens?
                      Log(FLT, "FPGA Write DMA Transfer Timeout");
                  break;
              }

              /* Process echoed data */
              FPGA_ProcessWriteDataSet();
            }

        } while(false);

        /* Check if periodicity is delayed. \todo: Disable after testing */
        TimeNow = OSTimeGet();
        TimeLastIter = TimeNow - TimeLastIter;  /* Storing delta time in 'TimeLastIter' */

        if (TimeLastIter > FPGA_SYNC_PERIOD + FPGA_COMM_ERROR_MARGIN)
        {
            /* This should never happen. */
            Log(DBG, "FpgaControllerTask: FPGA Tick Delayed: %d mS", TimeLastIter);
        }

        TimeLastIter = TimeNow;              /* Update time snapshot */

        /// \todo 09/29/2021 DAZ - This works ONLY because the delay is the minimum resolution of the timer & processing time is less than that.
        /// \todo 09/29/2021 DAZ - Sync period is in fact < 1mS. (1mS - processing time)
        OSTimeDly(FPGA_SYNC_PERIOD);
    }
}

/* ========================================================================== */
/**
 * \brief   Notify read request transfer completion
 *
 * \details Notify the FPGA controller task about read command transfer completion.
 *          The task initiates the SPI trasfer and waits until the transfer is completed.
 *
 * \param  < None >
 *
 * \return  None
 *
 * ========================================================================== */
void FpgaReadDoneEvent(void)
{
    OSSemPost(pSemFpgaReadDone);
}

/* ========================================================================== */
/**
 * \brief   Notify write request transfer completion
 *
 * \details Notify the FPGA controller task about write command transfer completion.
 *          The task initiates the SPI transfer and waits until the transfer is completed.
 *
 * \param  < None >
 *
 * \return  None
 *
 * ========================================================================== */
void FpgaWriteDoneEvent(void)
{
    OSSemPost(pSemFpgaWriteDone);
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Clear FPGA error log
 *
 * \details Clears FPGA error log, resets log entry index
 *
 * \param  < None >
 *
 * \return None
 *
 * ========================================================================== */
void L3_FpgaErrorLogClear(void)
{
    uint16_t Index;

    /* Clear all log entries */
    for (Index=0; Index < FpgaErrorCount; Index++ )
    {
        FpgaErrorLog[Index].TxRegAdr = 0;
        FpgaErrorLog[Index].RxRegAdr = 0;
        memset(&FpgaErrorLog[Index].RegVal[0], 0, FPGA_ERR_LOG_REG_SIZE);
        FpgaErrorLog[Index].RcvCRC = 0;
        FpgaErrorLog[Index].CalcCRC = 0;
        FpgaErrorLog[Index].ErrCode = 0;
        FpgaErrorLog[Index].TotalWrAckErrors = 0;
        FpgaErrorLog[Index].TotalCrcErrors = 0;
    }
    Log(TRC, "FPGA Error log cleared: %d", Index);
}

/* ========================================================================== */
/**
 * \brief   Dump FPGA error log
 *
 * \details Dumps and clear all FPGA error log entries, resets log entry index
 *
 * \param  < None >
 *
 * \return None
 *
 * ========================================================================== */
void L3_FpgaErrorLogDump(void)
{
    uint16_t Index;
    FPGA_ERROR_REC *pRec;

    if(FpgaErrorCount)
     {
        Log(TRC, "FPGA Error log dump start: %d", FpgaErrorCount);
        for (Index=0; Index < FpgaErrorCount; Index++ )
        {
            pRec = &FpgaErrorLog[Index];

            Log(TRC,"<%03d> Tx: 0x%x, Rx: 0x%x, Val: 0x%x 0x%x 0x%x 0x%x, RxCRC: 0x%x, CalcCRC: 0x%x",
                    Index, pRec->TxRegAdr, pRec->RxRegAdr, pRec->RegVal[0], pRec->RegVal[1], pRec->RegVal[2], pRec->RegVal[3], pRec->RcvCRC, pRec->CalcCRC);
            Log(TRC,"       ErrCode: 0x%x, AccErrs: %d, CrcErrs: %d", pRec->ErrCode, pRec->TotalWrAckErrors, pRec->TotalCrcErrors);
        }
        Log(TRC, "FPGA Error log end");
    }
    else
    {
        Log(TRC, "FPGA Error log is empty");
    }
}

/* ========================================================================== */
/**
 * \brief   Initialize the FPGA. SPI & its associated DMA initialization are taken
 *          care of by SPI.
 *
 * \details The SPI channel & its associated DMA initialization are taken care of
 *          by the SPI module. The FpgaDynamic Map which contains the FPGA register
 *          data (shadow registers) is initialized here.
 *
 * \param  < None >
 *
 * \return  bool - Initialization status. True if error.
 *
 * ========================================================================== */
bool L3_FpgaInit(void)
{
    bool            ErrorStatus;    /* Initialization error status */
    uint8_t         Idx;            /* Register Index */
    EDC_ERR         CrcErr;         /* Error return from micrium's CRC calculation routine. */
    FPGA_MGR_STATUS eFgpaMgrStaus;  /* Fpga Manager Status. */
    uint8_t         OsError;        /* OS errors */
    uint8_t rdData[2];              /* Temporary buffer used in CRC calculation */

    /* Initialize the variables */
    OsError = 0u;

    do
    {
        ErrorStatus = true;         /* Default to error */
        FpgaDataSetErrors = 0;      /* Reset error counter */

        /* Disable 2.5Vref until motor power is enabled to prevent loading of 2.5V supply */
        L3_GpioCtrlSetSignal(GPIO_EN_2P5V);

        /* calculate CRC's for reading (avoids calculation in ISR) */
        for (Idx = 0; Idx < FPGA_REG_COUNT; Idx++)
        {
            rdData[0] = IdxToAdr[Idx].Adr;
            rdData[1] = 0x00;

            FpgaDynamicMap[Idx].ReadCRC = CRC_ChkSumCalc_16Bit((CRC_MODEL_16 *)&CrcModel, rdData, sizeof(uint16_t), &CrcErr);
        }

        /* Call Fpga Manager to Initialize FPGA, \todo: FPGA Manager should be out of FPGA controller, Move to L3_Init().*/
        eFgpaMgrStaus = L3_FpgaMgrInit();

        /* Check the return, if Error then return. */
        if (!((FPGA_MGR_REFRESH_FAILED == eFgpaMgrStaus) || (FPGA_MGR_OK == eFgpaMgrStaus)))
        {
            break;
        }

        /* Configure SPI0 & associated DMA channels: */
        if(SPI_STATUS_OK != L2_SpiEnable(SPI_PORT_ZERO, true))
        {
            /* SPI configuration error, exit */
            ErrorStatus = true;
            Log(ERR, "L3_FpgaInit: Spi Configuration Error - %d", OsError);
            break;
        }

        /*  \todo: FPGA task MUST have highest priority among all other tasks when motor is running.
            Currently the priority set above all other platform task but below USB which is having hardcoded
            to somewhere in the range 1-4. FPGA task priority is set just below this range to avoid conflicts.
            Modifications to USB tasks to be considered at a later stage.
            */
        OsError = SigTaskCreate(&FpgaControllerTask,
                                            NULL,
                                            &FpgaControllerTaskStack[0],
                                            TASK_PRIORITY_FPGA_CNTLR,
                                            FPGA_CTRL_TASK_STACK,
                                "FpgaCtrl");

        if (OsError != OS_ERR_NONE)
        {
            /* Couldn't create task, exit with error */
            ErrorStatus = true;
            Log(ERR, "L3_FpgaInit: Task Create Error - %d", OsError);
            break;
        }

        ErrorStatus = false;    /* No error */

    } while (false);

    return ErrorStatus;
}

/* ========================================================================== */
/**
 * \brief   Issue request to read an FPGA Register
 *
 * \details Returns the last known value of the specified register & schedules it
 *          for reading on the next timer interrupt.
 *
 * \note    This function always returns a 32 bit value. If the register is smaller,
 *          the value is padded with zeroes. It is the user's responsiblity to supply
 *          a 32 bit value to receive the result & assign it to a smaller value (16, 8 bit)
 *          if desired. The maximum value returned is determined by the underlying
 *          register properties.
 * \n \n
 *          If an error is returned (and the input parameters are valid), the FPGA
 *          has stopped responding, and the last known value (if any) is returned.
 *
 * \sa      Register memory map in R004522 - Gen II FPGA Software Design Document
 *
 * \param   Reg     - Register index
 * \param   pRegVal - Pointer to data to return. If pRegVal is NULL, then a read is
 *                    scheduled and no data is returned.
 *
 * \return  bool - Function status, true if error.
 *
 * ========================================================================== */
bool L3_FpgaReadReg(FPGA_REG Reg, uint32_t *pRegVal)
{
    bool     ErrorStatus;       /* Error status */
    uint8_t  Idx;               /* Index into IdxToAdr for register */

    ErrorStatus = false;
    Idx = (uint8_t)Reg;

    do
    {
        /* Validate register */
        if (Reg >= FPGA_REG_COUNT)
        {
            Log(DBG, "L3_FpgaReadReg(): Error index reg = %d ptr = %08x", Reg, pRegVal);
            ErrorStatus = true;
            break;
        }

        /* CrcError > 0 indicates that register value requested has not been updated successfully since startReadDataSet.
          Last known value is returned. Error is returned only if N consecutive crc errors have occurred.
          When a CRC error occurs, the read is automatically retried on the next tick. */

        if (FpgaDynamicMap[Idx].CrcError >= FPGA_RETRY_MAX)
        {
            ErrorStatus = true;
        }

        if (FpgaWriteErrorCount >= FPGA_MAX_WRITE_NACKS)
        {
            /* Fpga has stopped responding - set error condition */
            ErrorStatus = true;
        }

        if (pRegVal != NULL)
        {
            /* Retrieve value based on size */
            switch (IdxToAdr[Idx].RegSize)
            {
                case REG_BYTE:
                    *pRegVal = (uint32_t)FpgaDynamicMap[Idx].Val8;
                    break;

                case REG_WORD:
                    *pRegVal = (uint32_t)FpgaDynamicMap[Idx].Val16;
                    break;

                case REG_LONG:
                    *pRegVal = FpgaDynamicMap[Idx].Val32;
                    break;

                default:
                    Log(DBG, "L3_FpgaReadReg(): Error in Size: RegSize = %d", IdxToAdr[Idx].RegSize);
                    ErrorStatus = true;
                    break;
            }
        }
        FpgaDynamicMap[Idx].ReadRequest = true;  /* Schedule read request */

    } while (false);

    return ErrorStatus;
}

/* ========================================================================== */
/**
 * \brief   Writes an FPGA Register
 *
 * \details Sets the FPGA shadow register. The FPGA_Controller_Task will complete
 *          the pending write as part of the task loop.
 *
 * \note    If the register to be written is smaller than 32 bits (ie. 8, 16 bit),
 *          only the least significant bits of the register will be written. Higher
 *          order bits will be lost.
 *
 * \param   Reg    - Register index
 * \param   RegVal - Value to write
 *
 * \return  bool - Error Status: True if error. (Invalid index, mutex or value size)
 *
 * ========================================================================== */
bool L3_FpgaWriteReg(FPGA_REG Reg, uint32_t RegVal)
{
    bool    ErrorStatus;        /* Function error status. True if error. */
    uint8_t Idx;                /* Index into IdxToAdr for register */

    ErrorStatus = false;
    Idx = (uint8_t)Reg;

    do
    {
        /* Validate register */
        if (Reg >= FPGA_REG_COUNT)
        {
            Log(DBG, "L3_Fpga: Error in L3_FpgaWriteReg(): reg = %d", Reg);
            ErrorStatus = true;
            break;
        }

        /* Write value based on register size */
        switch (IdxToAdr[Idx].RegSize)
        {
            case REG_BYTE:
                FpgaDynamicMap[Idx].Val8 = (uint8_t)RegVal;
                break;

            case REG_WORD:
                FpgaDynamicMap[Idx].Val16 = (uint16_t)RegVal;
                break;

            case REG_LONG:
                FpgaDynamicMap[Idx].Val32 = RegVal;
                break;

            default:
                Log(DBG, "L3_Fpga: Error in L3_FpgaWriteReg() Size: RegVal = %d", RegVal);
                ErrorStatus = true;
                break;
        }

        FpgaDynamicMap[Idx].WriteRequest = true; /* Schedule write request */

    } while (false);

    return ErrorStatus;
}

/* ========================================================================== */
/**
 * \brief   Set/Clear FPGA refresh request flag
 *
 * \details This function sets/clears the fpga refresh request flag. The flag is
 *          used to indicate an error has occurred that requires FPGA refresh.
 *          This function is also used to clear the flag after the refresh has
 *          been performed.
 *
 * \param   Request - Value to set the request flag to.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_FpgaRequestRefresh(bool Request)
{
    FpgaRefreshRequest = Request;       // Set/Clear FPGA Refresh Request flag
}

/* ========================================================================== */
/**
 * \brief   Determine if FPGA refresh request is pending
 *
 * \details This function returns the state of the FpgaRequest flag to allow
 *          the caller to determine if a refresh has been requested.
 *
 * \param    < None >
 *
 * \return  true if FPGA refresh has been requested, false otherwise.
 *
 * ========================================================================== */
bool L3_FpgaIsRefreshPending(void)
{
    return FpgaRefreshRequest;          // Return state of refresh request flag
}

/* ========================================================================== */
/**
 * \brief   Reload selected FPGA registers.
 *
 * \details After an FPGA refresh, certain FPGA registers, particularly those
 *          controlling the motors should be restored to their previous states
 *          before restarting any motor moves. This is done by setting the
 *          WriteRequest bit for the appropriate registers. The registers currently
 *          reloaded are:
 *              - FPGA_REG_CONTROL
 *              - FPGA_REG_MOT0_CONTROL
 *              - FPGA_REG_MOT1_CONTROL
 *              - FPGA_REG_MOT2_CONTROL
 *
 * \note    After calling this routine, the caller should wait for the FPGA to update
 *          and the Allegro chips controlled by the FPGA to settle.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L3_FpgaReload(void)
{
    FpgaDynamicMap[FPGA_REG_CONTROL].WriteRequest = true;           // Request FPGA control reg to be written

    FpgaDynamicMap[FPGA_REG_MOT0_CONTROL].WriteRequest = true;      // Request motor control registers to be written
    FpgaDynamicMap[FPGA_REG_MOT1_CONTROL].WriteRequest = true;
    FpgaDynamicMap[FPGA_REG_MOT2_CONTROL].WriteRequest = true;

    FpgaDynamicMap[FPGA_REG_MOT0_CURR_PWM].WriteRequest = true;     // Request motor current limit registers to be written
    FpgaDynamicMap[FPGA_REG_MOT1_CURR_PWM].WriteRequest = true;
    FpgaDynamicMap[FPGA_REG_MOT2_CURR_PWM].WriteRequest = true;
}


/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
