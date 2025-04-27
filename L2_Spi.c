#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Spi
 * \{
 * \brief   Layer 2 SPI driver
 *
 * \details This driver handles the following MK20 SPIs in the MotorPack:
 *             - SPI0: FPGA
 *             - SPI2: Charger, Accelerometer
 * \n \n
 *          The functions contained in this module provide the following capabilities:
 *             - Initialize a SPI interface
 *             - SPI transmit interrupt handling
 *             - SPI receive interrupt handling
 *
 * \sa      K20 SubFamily Reference Manual (provide link to K20P131M100SF2RM.pdf
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_Spi.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Board.h"
#include "L2_Spi.h"
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
#define LOG_GROUP_IDENTIFIER       (LOG_GROUP_SPI)        ///< Log Group Identifier
#define SPI0_PCS                   (0x20u)       ///< SPI0 Peripheral Chip Select (Inactive High)
#define SPI2_PCS                   (0x02u)       ///< SPI2 Peripheral Chip Select (Inactive High)
#define SPI_PCS_DATA_MASK          (0x0000FFFFu) ///< Mask control bytes with data to clean before OR'ng
#define DMASPIBUFFER_SIZE          (256u)        ///< Size of DMA buffer (32-bit) needed by dedicated FPGA device
#define DMASPISHAREBUFF_SIZE       (128u)        ///< Size of DMA buffer (32-bit) needed by L3_Spi sharing devices

/// Offset values used to configure txSrcOff,txDstOff,rxSrcOff and rxDstOff
#define DMA_TCD_OFF_4              (4u)
#define DMA_TCD_OFF_1              (1u)

/// SPI0_CTAR Register fields (nibble is shifted in initialization)
#define SPI0_DBR              (0x0u)     ///< Bit  31    Double Baud Rate
#define SPI0_FMSZ             (0x7u)     ///< Bits 30-27 Frame Size (value + 1)
#define SPI0_CPOL             (0x0u)     ///< Bit  26    Clock Polarity (0 inactive state value SCK is low)
#define SPI0_CPHA             (0x1u)     ///< Bit  25    Clock Phase (1 Data changes rising edge, sampled on falling)
#define SPI0_LSBFE            (0x0u)     ///< Bit  24    LSB First (0 MSB first)
#define SPI0_PCSSCK           (0x1u)     ///< Bits 23-22 PCS to SCK Delay Prescalar (3)
#define SPI0_PASC             (0x1u)     ///< Bits 21-20 After SCK Delay Prescalar (3)
#define SPI0_PDT              (0x3u)     ///< Bits 19-18 Delay after Transfer Prescaler (7)
#define SPI0_PBR              (0x1u)     ///< Bits 17-16 Baud Rate Prescaler (3)
#define SPI0_CSSCK            (0x0u)     ///< Bits 15-12 PCS to SCK Delay Scaler value (2)
#define SPI0_ASC              (0x0u)     ///< Bits 11-8  After SCK Delay Scaler value
#define SPI0_DT               (0x1u)     ///< Bits 7-4   Delay After Transfer Scaler
#define SPI0_BR               (0x0u)     ///< Bits 3-0   Baud Rate Scaler Value (2)

/// SPI2_CTAR Register fields (nibble is shifted in initialization) for Accelerometer
#define SPI2A_DBR             (0x0u)     ///< Bit  31    Double Baud Rate
#define SPI2A_FMSZ            (0xFu)     ///< Bits 30-27 Frame Size (value + 1)
#define SPI2A_CPOL            (0x1u)     ///< Bit  26    Clock Polarity (0 inactive state value SCK is low)
#define SPI2A_CPHA            (0x1u)     ///< Bit  25    Clock Phase (1 Data changes rising edge, sampled on falling)
#define SPI2A_LSBFE           (0x0u)     ///< Bit  24    LSB First (0 MSB first)
#define SPI2A_PCSSCK          (0x1u)     ///< Bits 23-22 PCS to SCK Delay Prescalar (3)
#define SPI2A_PASC            (0x1u)     ///< Bits 21-20 After SCK Delay Prescalar (3)
#define SPI2A_PDT             (0x1u)     ///< Bits 19-18 Delay after Transfer Prescaler (3)
#define SPI2A_PBR             (0x0u)     ///< Bits 17-16 Baud Rate Prescaler (2)
#define SPI2A_CSSCK           (0x4u)     ///< Bits 15-12 PCS to SCK Delay Scaler value (32)
#define SPI2A_ASC             (0x4u)     ///< Bits 11-8  After SCK Delay Scaler value
#define SPI2A_DT              (0x4u)     ///< Bits 7-4   Delay After Transfer Scaler
#define SPI2A_BR              (0x4u)     ///< Bits 3-0   Baud Rate Scaler Value (16)

  /// SPI2_CTAR Register fields (nibble is shifted in initialization) for Charger
#define SPI2C_DBR             (0x1u)     ///< Bit  31    Double Baud Rate
#define SPI2C_FMSZ            (0x7u)     ///< Bits 30-27 Frame Size (value + 1)
#define SPI2C_CPOL            (0x0u)     ///< Bit  26    Clock Polarity (0 inactive state value SCK is low)
#define SPI2C_CPHA            (0x1u)     ///< Bit  25    Clock Phase (1 Data changes rising edge, sampled on falling)
#define SPI2C_LSBFE           (0x0u)     ///< Bit  24    LSB First (0 MSB first)
#define SPI2C_PCSSCK          (0x1u)     ///< Bits 23-22 PCS to SCK Delay Prescalar (3)
#define SPI2C_PASC            (0x1u)     ///< Bits 21-20 After SCK Delay Prescalar (3)
#define SPI2C_PDT             (0x3u)     ///< Bits 19-18 Delay after Transfer Prescaler (7)
#define SPI2C_PBR             (0x1u)     ///< Bits 17-16 Baud Rate Prescaler (3)
#define SPI2C_CSSCK           (0x0u)     ///< Bits 15-12 PCS to SCK Delay Scaler value (2)
#define SPI2C_ASC             (0x4u)     ///< Bits 11-8  After SCK Delay Scaler value
#define SPI2C_DT              (0x2u)     ///< Bits 7-4   Delay After Transfer Scaler
#define SPI2C_BR              (0xCu)     ///< Bits 3-0   Baud Rate Scaler Value (4096)

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
/// SPI DMA Config Structure.
typedef struct
{
    SPI_MemMapPtr     SpiChanMemPtr;      ///< SPI peripheral register structure
    uint8_t           TxChanId;           ///< DMA Channel Id for Tx (For SPI0 it is 0, SPI2 it is 12)
    uint8_t           RxChanId;           ///< DMA Channel Id for Rx (For SPI0 it is 1, SPI2 it is 13)
    uint32_t          TxDmaEnableMask;    ///< Mask to enable Tx DMA
    uint32_t          RxDmaEnableMask;    ///< Mask to enable Rx DMA
    uint8_t           TxDmaIrq;           ///< Tx DMA Interrupt Vector.
    uint8_t           RxDmaIrq;           ///< Rx DMA Interrupt Vector
    uint8_t           DmaIsrPriority;     ///< DMA ISR priority (Rx & Tx)
    uint16_t          TxDmaIntMask;       ///< Interrupt mask for Tx DMA. Currently we are not using interrupt for Tx complete,added this for future reference.
    uint16_t          RxDmaIntMask;       ///< Interrupt mask for Tx DMA
    uint8_t           TxSrcOff;           ///< Src address offset for Tx channel
    uint8_t           TxDstOff;           ///< Dst address offset for Tx channel
    uint8_t           RxSrcOff;           ///< Src address offset for Rx channel
    uint8_t           RxDstOff;           ///< Dst address offset for Rx channel
    uint8_t           TxFrmSize;          ///< Tx channel data transfer size
    uint8_t           RxFrmSize;          ///< Rx channel data transfer size
    uint32_t          *pTxChanDummy;      ///< Pointer to dummy data used for tx (Used for IO_RX_ONLY)
    uint32_t          *pRxChanDummy;      ///< Pointer to dummy data used for rx (Used for IO_TX_ONLY)
    uint8_t           MLByteTxCount;      ///< No of bytes to transfer per request on tx channel
    uint8_t           MLByteRxCount;      ///< No of bytes to transfer per request on rx channel
    uint32_t          Timeout;            ///< Timeout value used for DataIO, timeout=0 means blocking call,
                                          ///< This timeout is applicable when pCallback registered by L2_SPI_DataIO is NULL
} SPI_DMA_CONFIG;

/// Enum used to determine the type of Data IO
typedef enum
{
    IO_TX_ONLY,
    IO_RX_ONLY,
    IO_TX_RX,
    IO_LAST
} IO_TYPE;

// Callback handlers for SPI channel 0 and SPI channel 2
static SPI_CALLBACK_HNDLR     pCallback0;

// SPI0 DMA transmit buffer
static uint16_t Spi0BuffIndex;

#pragma location=".ramdyndata"  
static uint32_t Spi0TxBuffer[DMASPIBUFFER_SIZE];    /* SPI0 transmit buffer */

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
// Dummy Values used for either Tx only or Rx only
static uint32_t TxChan0Dummy = 0xFF;
static uint32_t RxChan0Dummy = 0xFF;

static OS_EVENT *pSpi0Sem    = NULL;    // SPI channel 0 transfer complete semaphore.
static bool SpiInitalized = false;      // Flag to indicate SPI is initialized.

static SPI_DMA_CONFIG Spi0DmaCfg =
{
    SPI0_BASE_PTR, SPI0_TX_DMA_IRQ, SPI0_RX_DMA_IRQ, DMA_ERQ_ERQ0_MASK, DMA_ERQ_ERQ1_MASK, SPI0_TX_DMA_IRQ, SPI0_RX_DMA_IRQ,
    SPI0_DMA_ISR_PRIORITY, DMA_INT_INT0_MASK, DMA_INT_INT1_MASK, DMA_TCD_OFF_4, 0, 0, DMA_TCD_OFF_1, 2, 0,&TxChan0Dummy,
    &RxChan0Dummy, 4, 1, 0
};

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static SPI_STATUS Spi_DmaInit(void);
static void L2_SpiConfigP0C0(void);
static void L2_SpiConfigP2C0(void);
static void L2_SpiConfigP2C1(void);
static void SPI_DmaStartTransfer(SPIIO *pDataIO,IO_TYPE IOType);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Initialize DMA configuration for data transfer
 *
 * \details This routine initializes/configures the specific attributes
 *          for a given DMA channel.
 *
 * \note    DMA Channel #0, 1 set for SPI0   (TX, RX)
 *          DMA Channel #12, 13 set for SPI2 (TX, RX)
 *
 * \param   pSpiDmaCfg - Pointer to DMA_CONFIG parameters like channel Id
 *                       src offset, dst offset, tx/rx frame size.
 *
 * \return  SPI_STATUS - returns status of DMA configuration.
 *
 * ========================================================================== */
static SPI_STATUS Spi_DmaInit(void)
{
    SPI_STATUS Status;
    Status = SPI_STATUS_OK;


    // tx channel configuration
    DMA_ATTR_REG(DMA_BASE_PTR, Spi0DmaCfg.TxChanId) = DMA_ATTR_SSIZE(Spi0DmaCfg.TxFrmSize) | DMA_ATTR_DSIZE(Spi0DmaCfg.TxFrmSize); //Frame Size
    DMA_SOFF_REG(DMA_BASE_PTR, Spi0DmaCfg.TxChanId) = Spi0DmaCfg.TxSrcOff;                                     // Source offset per read
    DMA_DADDR_REG(DMA_BASE_PTR, Spi0DmaCfg.TxChanId) = (uint32_t)&SPI_PUSHR_REG(Spi0DmaCfg.SpiChanMemPtr);     // Destination address
    DMA_DOFF_REG(DMA_BASE_PTR, Spi0DmaCfg.TxChanId) = Spi0DmaCfg.TxDstOff;        // Tx Destination offset.
    DMA_SLAST_REG(DMA_BASE_PTR, Spi0DmaCfg.TxChanId) = 0;
    DMA_DLAST_SGA_REG(DMA_BASE_PTR, Spi0DmaCfg.TxChanId) = 0;
    DMA_NBYTES_MLOFFNO_REG(DMA_BASE_PTR, Spi0DmaCfg.TxChanId) = DMA_NBYTES_MLOFFNO_NBYTES(Spi0DmaCfg.MLByteTxCount);   // Minor byte transfer count

    /* rx channel configuration */
    DMA_ATTR_REG(DMA_BASE_PTR, Spi0DmaCfg.RxChanId) = DMA_ATTR_SSIZE(Spi0DmaCfg.RxFrmSize) | DMA_ATTR_DSIZE(Spi0DmaCfg.RxFrmSize); //Frame Size.
    DMA_SOFF_REG(DMA_BASE_PTR, Spi0DmaCfg.RxChanId) = Spi0DmaCfg.RxSrcOff;                                     // Source offset per read
    DMA_SADDR_REG(DMA_BASE_PTR, Spi0DmaCfg.RxChanId) = (uint32_t)&SPI_POPR_REG(Spi0DmaCfg.SpiChanMemPtr);      // Destination address
    DMA_DOFF_REG(DMA_BASE_PTR, Spi0DmaCfg.RxChanId) = Spi0DmaCfg.RxDstOff;        // Rx Destination offset.
    DMA_SLAST_REG(DMA_BASE_PTR, Spi0DmaCfg.RxChanId) = 0;
    DMA_DLAST_SGA_REG(DMA_BASE_PTR, Spi0DmaCfg.RxChanId) = 0;
    DMA_NBYTES_MLOFFNO_REG(DMA_BASE_PTR, Spi0DmaCfg.RxChanId) = DMA_NBYTES_MLOFFNO_NBYTES(Spi0DmaCfg.MLByteRxCount);   // Minor byte transfer count

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Start the data transfer through DMA
 *
 * \details Setup the data transfer and enable  DMA for data transfer and enable the IRQ for transfer complete.
 *
 * \param   pDataIO - ponter to SPI Data Input Output Structure which has the spi channel value,
 *                    pointer to tx/rx data buffer, number of bytes to be tranfered
 * \param   IOType - direction of data transfer
 *
 * \return  None
 *
 * ========================================================================== */
static void SPI_DmaStartTransfer(SPIIO *pDataIO, IO_TYPE IOType)
{
    uint8_t CiterCount;

    CiterCount = pDataIO->Nbytes;

    switch (IOType)
    {
        case IO_TX_ONLY:

            DMA_SADDR_REG(DMA_BASE_PTR,Spi0DmaCfg.TxChanId) = (uint32_t) pDataIO->pSpiTxData;
            DMA_SOFF_REG(DMA_BASE_PTR, Spi0DmaCfg.TxChanId) = Spi0DmaCfg.TxSrcOff;
            DMA_CITER_ELINKNO_REG(DMA_BASE_PTR,Spi0DmaCfg.TxChanId) = DMA_CITER_ELINKNO_CITER(CiterCount);
            DMA_BITER_ELINKNO_REG(DMA_BASE_PTR,Spi0DmaCfg.TxChanId) = DMA_BITER_ELINKNO_BITER(CiterCount);
            DMA_CSR_REG(DMA_BASE_PTR,Spi0DmaCfg.TxChanId) = DMA_CSR_DREQ_MASK | DMA_CSR_INTMAJOR_MASK;

            DMA_DADDR_REG(DMA_BASE_PTR,Spi0DmaCfg.RxChanId) = (uint32_t) Spi0DmaCfg.pRxChanDummy;
            DMA_SOFF_REG(DMA_BASE_PTR, Spi0DmaCfg.RxChanId) = 0;
            DMA_CITER_ELINKNO_REG(DMA_BASE_PTR,Spi0DmaCfg.RxChanId) = DMA_CITER_ELINKNO_CITER(CiterCount);
            DMA_CSR_REG(DMA_BASE_PTR,Spi0DmaCfg.RxChanId) = DMA_CSR_DREQ_MASK;

            break;

        case IO_RX_ONLY:
            DMA_DADDR_REG(DMA_BASE_PTR,Spi0DmaCfg.RxChanId) = (uint32_t) pDataIO->pSpiRxData;
            DMA_SOFF_REG(DMA_BASE_PTR, Spi0DmaCfg.RxChanId) = Spi0DmaCfg.RxSrcOff;
            DMA_CITER_ELINKNO_REG(DMA_BASE_PTR,Spi0DmaCfg.RxChanId) = DMA_CITER_ELINKNO_CITER(CiterCount);
            DMA_BITER_ELINKNO_REG(DMA_BASE_PTR,Spi0DmaCfg.RxChanId) = DMA_BITER_ELINKNO_BITER(CiterCount);
            DMA_CSR_REG(DMA_BASE_PTR,Spi0DmaCfg.RxChanId) = DMA_CSR_DREQ_MASK | DMA_CSR_INTMAJOR_MASK;

            DMA_DADDR_REG(DMA_BASE_PTR,Spi0DmaCfg.TxChanId) = (uint32_t) Spi0DmaCfg.pTxChanDummy;
            DMA_SOFF_REG(DMA_BASE_PTR, Spi0DmaCfg.TxChanId) = 0;
            DMA_CITER_ELINKNO_REG(DMA_BASE_PTR,Spi0DmaCfg.TxChanId) = DMA_CITER_ELINKNO_CITER(CiterCount);
            DMA_CSR_REG(DMA_BASE_PTR,Spi0DmaCfg.TxChanId) = DMA_CSR_DREQ_MASK;
            break;

    case IO_TX_RX:
            DMA_SADDR_REG(DMA_BASE_PTR,Spi0DmaCfg.TxChanId) = (uint32_t) pDataIO->pSpiTxData;      // User supplied Tx data buffer
            DMA_SOFF_REG(DMA_BASE_PTR, Spi0DmaCfg.TxChanId) = Spi0DmaCfg.TxSrcOff;                // Source address offset for Tx
            DMA_CITER_ELINKNO_REG(DMA_BASE_PTR,Spi0DmaCfg.TxChanId) = DMA_CITER_ELINKNO_CITER(CiterCount);     // Current Major Iteration count.
            DMA_BITER_ELINKNO_REG(DMA_BASE_PTR,Spi0DmaCfg.TxChanId) = DMA_BITER_ELINKNO_BITER(CiterCount);     // Beginning Iteration count
            DMA_CSR_REG(DMA_BASE_PTR,Spi0DmaCfg.TxChanId) = DMA_CSR_DREQ_MASK | DMA_CSR_INTMAJOR_MASK;

            DMA_DADDR_REG(DMA_BASE_PTR,Spi0DmaCfg.RxChanId) = (uint32_t) pDataIO->pSpiRxData;      // User supplied Rx data buffer
            DMA_SOFF_REG(DMA_BASE_PTR, Spi0DmaCfg.RxChanId) = Spi0DmaCfg.RxSrcOff;                // Source address offset for Rx
            DMA_CITER_ELINKNO_REG(DMA_BASE_PTR,Spi0DmaCfg.RxChanId) = DMA_CITER_ELINKNO_CITER(CiterCount);    // Current Major Iteration count.
            DMA_BITER_ELINKNO_REG(DMA_BASE_PTR,Spi0DmaCfg.RxChanId) = DMA_BITER_ELINKNO_BITER(CiterCount);    // Beginning Iteration count
            DMA_CSR_REG(DMA_BASE_PTR,Spi0DmaCfg.RxChanId) = DMA_CSR_DREQ_MASK | DMA_CSR_INTMAJOR_MASK;
            break;

    default:
            break;
    }

    // Enable DMA channels for Tx and Rx and also Enable Interrupt for DMA Rx complete
    DMA_INT |= Spi0DmaCfg.RxDmaIntMask;
    DMA_ERQ |= Spi0DmaCfg.TxDmaEnableMask | Spi0DmaCfg.RxDmaEnableMask;

    SPI_RSER_REG(Spi0DmaCfg.SpiChanMemPtr) = SPI_RSER_TFFF_RE_MASK | SPI_RSER_TFFF_DIRS_MASK |SPI_RSER_RFDF_RE_MASK | SPI_RSER_RFDF_DIRS_MASK;

    // Un-halt SPI port
    SPI_MCR_REG(Spi0DmaCfg.SpiChanMemPtr) &= ~SPI_MCR_HALT_MASK;
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Initialize SPI
 *
 * \param   < None >
 *
 * \return  SPI_STATUS - returns initialization status
 *
 * ========================================================================== */
SPI_STATUS L2_SpiInit(void)
{
    SPI_STATUS Status = SPI_STATUS_OK;
    uint8_t Error;          // Error Id from below OS calls.

    do
    {
        // Creating semaphore which is used to signal the completion the DMA transfer.
        pSpi0Sem = SigSemCreate(0, "SPI0-TC", &Error);

        if (NULL == pSpi0Sem)
        {
            // Semaphore not available, return error
            Status = SPI_STATUS_ERROR;
            break;
        }

        SpiInitalized = true;

    } while (false);

  return Status;
}

/* ========================================================================== */
/**
 * \brief   Activate configuration for specified port and channel
 *
 * \details This routine setups required hardware configuration register as
 *          required by the specified port and channel.
 *
 * \param   Port - SPI port ID
 * \param   Channel - SPI channel identified by a chip select over a port
 *
 * \return  SPI_STATUS - returns status
 *
 * ========================================================================== */
SPI_STATUS L2_SpiSetup(SPI_PORT Port, SPI_CHANNEL Channel)
{
    SPI_STATUS Status;

    Status = SPI_STATUS_OK;

    do
    {
        if ( SPI_PORT_ZERO == Port)
        {
            SIM_SCGC6 |=  SIM_SCGC6_DSPI0_MASK; // Enable clock
            L2_SpiConfigP0C0();
            break;
        }

        if (( SPI_PORT_TWO == Port) && (SPI_CHANNEL0 == Channel))
        {
            SIM_SCGC3 |=  SIM_SCGC3_DSPI2_MASK; // Enable clock
            L2_SpiConfigP2C0();
            break;
        }

        if (( SPI_PORT_TWO == Port) && (SPI_CHANNEL1 == Channel))
        {
            SIM_SCGC3 |=  SIM_SCGC3_DSPI2_MASK; // Enable clock
            L2_SpiConfigP2C1();
            break;
        }

        Status = SPI_STATUS_PARAM_INVALID;

    }while (false);

    return Status;

}

/* ========================================================================== */
/**
 * \brief   Transfer a byte/word
 *
 * \details This routine sends a byte/word and reads a byte/word over specified
 *          SPI port and channel
 *
 * \param   Port - SPI port ID
 * \param   Channel - SPI channel identified by a chip select over a port
 * \param   Data - User data to be transferred
 * \param   LastTxfer - Last word transfer indicator. If set to true, chip
 *          select is de-asserted after the transfer.
 * \note    size(byte or word) of data transmitted depends on SPI framesize
 *
 * \return  uint16_t - returns data read from the SPI bus.
 *
 * ========================================================================== */
uint16_t L2_SpiTransfer(SPI_PORT Port, SPI_CHANNEL Channel, uint16_t Data, bool LastTxfer)
{
    uint16_t ReadData;
    uint32_t WriteData;
    SPI_MemMapPtr PortHwAddr;

    PortHwAddr = (SPI_PORT_ZERO == Port) ? SPI0_BASE_PTR : SPI2_BASE_PTR;

    WriteData = SPI_PUSHR_PCS(1<<Channel) | (Data) | ( LastTxfer ? SPI_PUSHR_EOQ_MASK : SPI_PUSHR_CONT_MASK);

    SPI_PUSHR_REG(PortHwAddr) = WriteData ;


    /* Wait for transfer complete */
    while((SPI_SR_REG(PortHwAddr) & SPI_SR_TCF_MASK)==0)

    /* \todo: is this really required? */
    for (uint16_t i = 0; i < 650; i++)
    {
      asm("nop");
    }

    /* Clear status register */
    SPI_SR_REG(PortHwAddr) |= SPI_SR_TCF_MASK;

    ReadData = (uint16_t)(SPI_POPR_REG(PortHwAddr) & 0xFFFFu);

    return ReadData;

}

/* ========================================================================== */
/**
 * \brief   Setup SPI2-Channel 0 for transfer
 *
 * \details This routine configures SPI-2 port for for transfer. Channel 0 is
 *          selected with CS0 on SPI-2 port.
 *
 * \note    The confuguration is specifically designed for charger HW
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void L2_SpiConfigP2C0(void)
{

     SPI2_MCR = SPI_MCR_HALT_MASK;

    // SCK baud rate = ( f SYS /PBR) x [(1+DBR)/BR]
    // Configured for 8 bit frame size, Alternate phase, @40 Kbps Clock.
    // \todo: Define macro based on separate category of settings.

    SPI2_CTAR0 = SPI_CTAR_DBR_MASK |  // double baud rate
               SPI_CTAR_FMSZ(7) |       // 8 bit frames
               SPI_CTAR_CPHA_MASK | // alternate phase - must be disabled if using MTFE in MCR
               SPI_CTAR_PCSSCK(0x1) |
               SPI_CTAR_PDT(7) |    // delayed phase timing in clock cycles
               SPI_CTAR_PBR(0xd) |
               SPI_CTAR_DT(2) |    // delayed timing
               SPI_CTAR_BR(0xc) |   // about 40 kbps w/ DBR mask
               SPI_CTAR_PASC(1) |
               SPI_CTAR_ASC(4);

      SPI2_CTAR1 = SPI_CTAR_DBR_MASK |  // double baud rate
               SPI_CTAR_FMSZ(7) |  // 8 bit frames
               SPI_CTAR_CPHA_MASK | // alternate phase - must be disabled if using MTFE in MCR
               SPI_CTAR_PCSSCK(0x1) |
               SPI_CTAR_PDT(7) |    // delayed phase timing in clock cycles
               SPI_CTAR_PBR(0xd) |
               SPI_CTAR_DT(2) |    // delayed timing
               SPI_CTAR_BR(0xc) |   // about 40 kbps w/ DBR maskSPI_CTAR_PASC(1) |
               SPI_CTAR_ASC(4);

    SPI2_MCR = SPI_MCR_MSTR_MASK |  // Master mode

               SPI_MCR_SMPL_PT(1) |  // can be 0, 1, 2 sys clks
               SPI_MCR_PCSIS(0x3) |   // CS0, CS1 high inactive
               SPI_MCR_CLR_TXF_MASK | // clear Tx FIFO
               SPI_MCR_CLR_RXF_MASK | // Clear Rx FIFO
               SPI_MCR_HALT_MASK;

  // Clear flags
  SPI2_SR = SPI_SR_TCF_MASK |
            SPI_SR_EOQF_MASK |
            SPI_SR_TFUF_MASK |
            SPI_SR_TFFF_MASK |
            SPI_SR_RFOF_MASK |
            SPI_SR_RFDF_MASK;

    // Interrupts / DMA
    SPI2_RSER = 0;

    SIM_SCGC3 |=  SIM_SCGC3_DSPI2_MASK;

    // Un-halt SPI port
    SPI2_MCR &= ~SPI_MCR_HALT_MASK;

}

/* ========================================================================== */
/**
 * \brief   Setup SPI2-Channel 1 for transfer
 *
 * \details This routine configures SPI-2 port for transfer. Channel 1 is
 *          selected with CS1 on SPI-2 port.
 *
 * \note    The configuration is specifically designed for accelerometer
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void L2_SpiConfigP2C1(void)
{
    INT32U SPI2_MCRdata;
    INT32U SPI2_CTAR0data = 0;

    // Halt prior to config of SPI2 module
    SPI2_MCR = SPI_MCR_HALT_MASK;    // Clears all MCR bits but Halt

    // Configure SPI2 Control registers while HALTed:
    SPI2_MCRdata = SPI_MCR_MSTR_MASK    |  // Master mode
                  SPI_MCR_CLR_TXF_MASK |  // clear Tx FIFO
                  SPI_MCR_CLR_RXF_MASK |  // Clear Rx FIFO
                  SPI_MCR_PCSIS_MASK;     // all CSn active HIGH

    SPI2_MCR     = SPI2_MCRdata;

    SPI2_CTAR0 = 0; // Clears reg so other '1' settings can be ORed in
    // Baud Rate = 1.25MHz
    SPI2_CTAR0data = SPI_CTAR_FMSZ(0xF)   |    // Frame size = 16 bits
                    SPI_CTAR_CPOL_MASK   |   // Clock Polarity - inactive state of SCK is high
                    SPI_CTAR_CPHA_MASK   |   // Clock Phase = 1 changes on falling edge sampled on rising
                    SPI_CTAR_PCSSCK(0x1) |   // PCS to SCK Delay Prescaler 1=>/3
                    SPI_CTAR_PASC(0x1)   |   // After SCK Delay Prescaler 1=>/3
                    SPI_CTAR_PDT(0x1)    |   // Delay after Transfer Prescaler 1=>/3
                    SPI_CTAR_PBR(0x8)    |   // Baud Rate Prescaler 1=>/3
                    SPI_CTAR_CSSCK(0x4)  |   // PCS to SCK Delay Scaler 4=>/32
                    SPI_CTAR_ASC(0x4)    |   // After SCK Delay Scaler 4=>/32
                    SPI_CTAR_DT(0x4)     |   // Delay after Transfer Scaler 4=>/32
                    SPI_CTAR_BR(0x4);        // Baud Rate Scaler 4=>/32

    SPI2_CTAR0 = SPI2_CTAR0data;

    SPI2_RSER = 0; // clears interrupt requests

    // SIM - System Integration Module:   System Clock Gate Control
    SIM_SCGC3 |= SIM_SCGC3_DSPI2_MASK;    // Enable SPI2 Clock

    /****************** SPI2 is configured ****************************************/
    /****************** Start SPI2 Module  ****************************************/
    SPI2_MCRdata &= ~SPI_MCR_HALT_MASK    &   // Init complete: Remove any Halt
                   ~SPI_MCR_CLR_TXF_MASK &   // Remove Tx FIFO clear command
                   ~SPI_MCR_CLR_RXF_MASK;    // Remove Rx FIFO clear command
    SPI2_MCR = SPI2_MCRdata;

}

/* ========================================================================== */
/**
 * \brief   Setup SPI0-Channel0 for transfer
 *
 * \details This routine configures SPI-0 port for for transfer.
 *
 * \note    The configuration is specifically designed for FGPA communication
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void L2_SpiConfigP0C0(void)
{
    SPI_MemMapPtr    SpiChanMemPtr;     // Memory mapped base address for SPI interface
    uint32_t         SpiRegData;        // local variable for setting register value

    SpiChanMemPtr = Spi0DmaCfg.SpiChanMemPtr;

    // Enable System clock gating and control for SPI0
    SIM_SCGC6 |= SIM_SCGC6_DSPI0_MASK;  // Bit 12 SPI0 clock gate control (1 is enabled)

    // Set Module Configuration (MCR Register)
    SpiRegData = SPI_MCR_MSTR_MASK |         // Bit 31 - Master Mode (1 enable)
                SPI_MCR_CLR_TXF_MASK |      // Bit 11 - Flush Tx FIFO (W1C counter)
                SPI_MCR_CLR_RXF_MASK |      // Bit 10 - Flush Rx FIFO (W1C counter)
                SPI_MCR_HALT_MASK |         // Bit 0 - Halt (1 stops transfers)
                SPI_MCR_PCSIS(SPI0_PCS);    // Bits 21 to 16 - Peripheral Chip Select x Inactive State

    SPI_MCR_REG(SpiChanMemPtr) = SpiRegData;

    // Set Clock and Transfer Attributes (CTAR Register)
    SPI_CTAR_REG(SpiChanMemPtr,0) = (SPI_CTAR_BR(SPI0_BR) |
                                    SPI_CTAR_DT(SPI0_DT) |
                                    SPI_CTAR_ASC(SPI0_ASC) |
                                    SPI_CTAR_CSSCK(SPI0_CSSCK) |
                                    SPI_CTAR_PBR(SPI0_PBR) |
                                    SPI_CTAR_PDT(SPI0_PDT) |
                                    SPI_CTAR_PASC(SPI0_PASC) |
                                    SPI_CTAR_PCSSCK(SPI0_PCSSCK) |
                                    SPI_CTAR_CPHA_MASK |
                                    SPI_CTAR_FMSZ(SPI0_FMSZ));

    // Add last word indicator, chip select, dummy data byte
    TxChan0Dummy = SPI_PUSHR_CONT_MASK | SPI_PUSHR_PCS(SPI0_PCS) | SPI_PUSHR_TXDATA(0);

    // Configure DMA Enable Channel Preemption for SPI0 Channels 0,1
    DMA_DCHPRI0 &= ~DMA_DCHPRI0_ECP_MASK;    // Bit 7 - (0 is Ch cannot be suspended by higher priority ch's DMA transfer request)
    DMA_DCHPRI1 &= ~DMA_DCHPRI1_ECP_MASK;

    // Clear SPI Flags
    SPI_SR_REG(SpiChanMemPtr) = SPI_SR_TCF_MASK  |          // Bit 31 - Transfer complete flag
                                SPI_SR_EOQF_MASK |          // Bit 28 - End of queue flag
                                SPI_SR_TFUF_MASK |          // Bit 27 - Transmit fifo underflow flag
                                SPI_SR_TFFF_MASK |          // Bit 25 - Transmit fifo fill flag
                                SPI_SR_RFOF_MASK |          // Bit 19 - Receive fifo overflow flag
                                SPI_SR_RFDF_MASK;           // Bit 17 - Receive fifo drain flag

    SPI_RSER_REG(SpiChanMemPtr) = 0;                        // Interrupts and DMA disabled

    // Un-halt SPI port
    SPI_MCR_REG(SpiChanMemPtr) &= ~SPI_MCR_HALT_MASK &      // Config complete remove any Halt
                                  ~SPI_MCR_CLR_TXF_MASK &   // Remove Tx FIFO clear command
                                  ~SPI_MCR_CLR_RXF_MASK;    // Remove Rx FIFO clear command

    SPI_SR_REG(SpiChanMemPtr) |= SPI_SR_TCF_MASK;           // W1C - any spurious transfer flag
    Spi0DmaCfg.Timeout = 50;
    Spi_DmaInit();

    Set_IRQ_Priority(Spi0DmaCfg.RxDmaIrq, Spi0DmaCfg.DmaIsrPriority);
    Enable_IRQ(Spi0DmaCfg.RxDmaIrq);
}

/* ========================================================================== */
/**
 * \brief   Build DMA transmit buffer
 *
 * \details Setup the transmit DMA data.
 *
 * \param   firstPkt  - 1 resets the Spi buffer index to 0
 *                      0 appends the packet to the buffer at the current buffer index
 * \param   numBytes  - number of bytes in packet
 * \param   pPktBytes - pointer to byte packet to add to transmit buffer
 *
 * \return  SPI_STATUS - Return status of data transfer
 *
 * ========================================================================== */
SPI_STATUS L2_Spi0TxPacket(bool firstPkt, uint8_t numBytes, uint8_t *pPktBytes)
{
    SPI_STATUS Status;                             ///< Return Status
    uint8_t buffIndex;                             ///< Index for converting passed data to long words

    Status = SPI_STATUS_OK;
    do
    {
        if ((0 == numBytes) || (NULL == pPktBytes))
        {
            Status = SPI_STATUS_PARAM_INVALID;
            break;
        }

        if (firstPkt)
        {
            Spi0BuffIndex = 0;   // reset buffer index to beginning of buffer for first packet
        }

        for (buffIndex = 1; buffIndex < numBytes; buffIndex++)
        {
            Spi0TxBuffer[Spi0BuffIndex++] = SPI_PUSHR_CONT_MASK | SPI_PUSHR_PCS(SPI0_PCS) | SPI_PUSHR_TXDATA(*pPktBytes++);
        }
        Spi0TxBuffer[Spi0BuffIndex++] = SPI_PUSHR_PCS(SPI0_PCS) | SPI_PUSHR_TXDATA(*pPktBytes);

    } while (false);

    return(Status);
}

/* ========================================================================== */
/**
 * \brief   Enable/Disable SPI port
 *
 * \details Enables/Disable specified SPI port.
 *
 * \param   Port - SPI port ID
 * \param   Enable - Desired port status. 'true' enables the port.
 *
 * \return  SPI_STATUS - returns status
 *
 * ========================================================================== */
SPI_STATUS L2_SpiEnable(SPI_PORT Port, bool Enable)
{
    /* \todo: Power modes to be implemented */
  return SPI_STATUS_OK;
}

/* ========================================================================== */
/**
 * \brief   Send/Receive data through SPI
 *
 * \note    This routine is used only for data transfers on SPI0 port. To transfer
 *          data over SPI2, use the function L2_SpiTransfer.
 *
 * \param   pDataIO - pointer to SPI Data Input Output Structure
 *
 * \return  SPI_STATUS - Return status of data transfer
 *
 * ========================================================================== */
SPI_STATUS L2_SpiDataIO(SPIIO *pDataIO)
{
    SPI_STATUS Status;                               ///< Return Status
    IO_TYPE    IOType;                               ///< Type of DMA transfer depending upon SPIIO data ptrs
    uint8_t  Error;                                  ///< Semaphore error value

    Status = SPI_STATUS_OK;
    do
    {
        if (!SpiInitalized)
        {
            Status = SPI_STATUS_UNINITIALIZED;
            break;
        }

        if (NULL == pDataIO)
        {
            Status = SPI_STATUS_PARAM_INVALID;
            break;
        }

        // This routine support only SPI0
        if (SPI_PORT_ZERO != pDataIO->SpiPort)
        {
            Status = SPI_STATUS_PARAM_INVALID;
            break;
        }

        // Check for valid data
        if ((0 == pDataIO->Nbytes) || ((NULL==pDataIO->pSpiTxData) && (NULL==pDataIO->pSpiRxData)) || (DMASPIBUFFER_SIZE < pDataIO->Nbytes))
        {
            Status = SPI_STATUS_PARAM_INVALID;
            break;
        }

        if ((NULL != pDataIO->pSpiTxData) && (NULL != pDataIO->pSpiRxData))
        {
            IOType = IO_TX_RX;
        }
        else if ( NULL != pDataIO->pSpiTxData)
        {
            IOType = IO_TX_ONLY;
        }
        else
        {
            IOType = IO_RX_ONLY;
        }

        pCallback0 = pDataIO->pCallback;
        /* FPGA buffer is converted to long words during packetization and saved in Spi0TxBuffer */
                    /* Tx longword is:  Control byte | SPI chip select | MSB data (always 0 - we only send LS byte) | LSB Data */
        if (NULL != pDataIO->pSpiTxData)
        {
            pDataIO->pSpiTxData = (uint8_t *)Spi0TxBuffer;
                        pDataIO->Nbytes = Spi0BuffIndex;
        }

        // Start the DMA transfer after setting up the DMA channels with the required data
        SPI_DmaStartTransfer(pDataIO, IOType);

        // Please refer to the function header for detail notes on the callback and timeout.
        if ((SPI_PORT_ZERO == pDataIO->SpiPort) && (NULL == pCallback0))
        {
            OSSemPend(pSpi0Sem,  Spi0DmaCfg.Timeout, &Error);  // Blocking if the timeout is 0
        }

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   SPI0, DMA Channel 1 ISR handler.
 *
 * \details This interrupt handler will get called when the DMA Transfer is completed.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_Spi0RxDma_ISR(void)
{
    OS_CPU_SR cpu_sr;
    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    // Clear the DMA Intterupt flag
    DMA_CINT = DMA_INT_INT1_SHIFT;

    if (DMA_TCD1_CSR & DMA_CSR_DONE_MASK)
    {
        // Halt SPI Port and Update Transfer complete flag in Status register
        SPI0_MCR |= SPI_MCR_HALT_MASK;
        SPI0_SR |= SPI_SR_TCF_MASK;

        // Invove the registered callback to handle the response or Post the sempahore to the DataIO call
        if (NULL != pCallback0)
        {
            pCallback0();
        }
        else
        {
            OSSemPost(pSpi0Sem);
        }
    }

    OSIntExit();
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
