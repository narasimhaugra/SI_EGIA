#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Uart
 * \{
 * \brief   Layer 2 UART driver
 *
 * \details This driver handles the following MK20 UARTs in the PowerPack:
 *             - UART0: (UART0_BASE_PTR) Adapter
 *             - UART4: (UART4_BASE_PTR) Debug, IrDA
 *             - UART5: (UART5_BASE_PTR) WiFi
 *
 *          All UARTS are operated w/ No parity, 8 data bits, 1 stop bit, and no
 *          flow control. Both transmit and receive are DMA driven. All interrupt
 *          service and DMA buffer allocation are handled here.
 * \n \n
 *          The functions contained in this module provide the following capabilities:
 *             - Initialize a UART interface
 *             - Placing data into the transmit dma buffer & start transmission
 *             - Retrieving data from the receive DMA buffer
 *             - Check how many bytes are waiting in the UART receive buffer
 *             - Check how many bytes are pending for transmission to the UART
 *
 *          All necessary interrupt / DMA handling to implement this functionality
 *          is defined here.
 *             - UART transmit interrupt handling
 *             - UART transmit DMA handling
 *             - UART receive interrupt handling
 *             - UART receive DMA handling
 *             - UART receive error handling
 * \n \n
 *          UART0 and UART1 modules operate from the core/system clock, which provides
 *          higher performance level for these modules.
 *          All other UART modules operate from the bus clock.
 *
 *          MCGOUTCLK divided by OUTDIV1 clocks UART0 and UART1.
 *          MCGOUTCLK divided by OUTDIV2 clocks peripherals.
 *
 *          A 13-bit modulus counter and a 5-bit fractional fine-adjust counter in the
 *          baud rate generator derive the baud rate for both the receiver and the
 *          transmitter. The value from 1 to 8191 written to SBR[12:0] determines the
 *          module clock divisor. The SBR bits are in the UART baud rate registers,
 *          BDH and BDL. The baud rate clock is synchronized with the module clock and
 *          drives the receiver. The fractional fine-adjust counter adds fractional
 *          delays to the baud rate clock to allow fine trimming of the baud rate.
 *          Baud Rate Fractional Adjust BRFA[4:0] bits are in UART register C4.
 *
 *          Baud rate equation:
 *          UART baud rate = UART module clock / (16 * (SBR[12:0] + BRFD))
 *          where BRFD = BRFA/32
 *
 * \sa      Chapter 52 of K20 SubFamily Reference Manual (provide link to K20P144M120SF3RM.pdf)
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_Uart.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L2_Uart.h"
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
#define UART_CLOCK              (SYSTEM_FREQ_HZ/2u)          ///< module clock for UART 0,1
#define UART_CLOCK_SCL          (16L)   ///< UART clock prescale for baud rate

#define UART0_FIFO_SIZE         (8U)    ///< UART 0 FIFO size in bytes
#define UART4_FIFO_SIZE         (1U)    ///< UART 4 FIFO size in bytes
#define UART5_FIFO_SIZE         (1U)    ///< UART 5 FIFO size in bytes

#define UART_SBR_MASK           ((UART_BDH_SBR_MASK << 8) | UART_BDL_SBR_MASK)  ///< UART baud rate mask

#define UART_FLUSH_RX_BUFFER    (0x40U) ///< UART Rx flush buffer mask
#define UART_FLUSH_TX_BUFFER    (0x80U) ///< UART Tx flush buffer mask

#define UART0_RX_DMA_CHANNEL    (2U)    ///< UART 0 (Adapter) Rx DMA channel
#define UART4_RX_DMA_CHANNEL    (3U)    ///< UART 4 (IrDA)    Rx DMA channel
#define UART5_RX_DMA_CHANNEL    (4U)    ///< UART 5 (WiFi)    Rx DMA channel
#define UART0_TX_DMA_CHANNEL    (11U)   ///< UART 0 (Adapter) Tx DMA channel
#define UART4_TX_DMA_CHANNEL    (10U)   ///< UART 4 (IrDA)    Tx DMA channel
#define UART5_TX_DMA_CHANNEL    (9U)    ///< UART 5 (WiFi)    Tx DMA channel

#define DMABUFFER0_RX_SIZE      (512U)  ///< UART 0 (Adapter) Rx DMA buffer size (Must be power of 2)
#define DMABUFFER4_RX_SIZE      (128U)  ///< UART 4 (IrDA)    Rx DMA buffer size (Must be power of 2)
#define DMABUFFER5_RX_SIZE      (1024U)  ///< UART 5 (WiFi)    Rx DMA buffer size (Must be power of 2)

#define DMABUFFER0_TX_SIZE      (512U)  ///< UART 0 (Adapter) Tx DMA buffer size (Must be power of 2)
#define DMABUFFER4_TX_SIZE      (128U)  ///< UART 4 (IrDA)    Tx DMA buffer size (Must be power of 2)
#define DMABUFFER5_TX_SIZE      (1024U)  ///< UART 5 (WiFi)    Tx DMA buffer size (Must be power of 2)

#define FRACTIONAL_BAUD(baud, div) (((uint16_t)((UART_CLOCK * 32L) / ((baud) * 16L)) - ((div) * 32L)))     ///< Fractional baud divisor calculation. See Chapter 52 of K20 SubFamily Reference Manual (provide link to K20P144M120SF3RM.pdf)

#define MAX_UINT16              (0xFFFFU)   ///< Max value for uint16_t type

#define UART_LOOPBACK_ENABLE    UART_C1_LOOPS_MASK   ///< Loopback bit mask set
#define UART_LOOPBACK_DISABLE  ~UART_C1_LOOPS_MASK   ///< Loopback bit mask clear

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef struct                          ///  UART configuration structure
{
    UART_MemMapPtr  UartAdr;            ///< UART module address
    uint32_t        RxDmaChannel;       ///< Rx DMA channel number
    uint32_t        TxDmaChannel;       ///< Tx DMA channel number
    uint8_t         *pRxDmaBuf;         ///< Pointer to Rx DMA buffer
    uint8_t         *pTxDmaBuf;         ///< Pointer to Tx DMA buffer
    uint16_t        RxDmaBufSize;       ///< Rx DMA buffer size
    uint16_t        TxDmaBufSize;       ///< Tx DMA buffer size
    uint32_t        *pClockGate;        ///< Pointer to UART clock gate register
    uint32_t        ClockMask;          ///< UART clock gate mask
    uint16_t        *pRxTailCtr;        ///< Pointer to Rx Tail Counter
    uint8_t         DmaIsrPriority;     ///< DMA ISR priority (Rx & Tx)
    uint32_t        RxDmaEnableMask;    ///< Mask to enable Rx DMA
    uint32_t        TxDmaEnableMask;    ///< Mask to enable Tx DMA
    uint8_t         RxDmaIrq;           ///< Rx DMA Interrupt Vector
    uint8_t         TxDmaIrq;           ///< Tx DMA Interrupt Vector
    uint8_t         UartErrorIrq;       ///< UART Error Interrupt Vector
    uint8_t         UartFifoSize;       ///< UART FIFO size
    uint8_t         *pErr;              ///< Pointer to accumulated UART error
} UART_CFG;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static uint8_t Uart0Err;        // Accumulated UART0 errors
static uint8_t Uart4Error;      // Accumulated UART4 errors
static uint8_t Uart5Err;        // Accumulated UART5 errors

#pragma location=".ramdyndata"                                // Store buffer in external RAM
static uint8_t  DmaBuffer0Rx[DMABUFFER0_RX_SIZE];       // Rx DMA buffer UART 0 (Adapter)

static uint16_t Dma0TailCounter;                        // Index of oldest unretrieved byte in Rx DMA buffer 0

#pragma location=".ramdyndata"                                // Store buffer in external RAM
static uint8_t  DmaBuffer4Rx[DMABUFFER4_RX_SIZE];       // Rx DMA buffer UART 4 (IrDA)

static uint16_t Dma4TailCounter;                        // Index of oldest unretrieved byte in Rx DMA buffer 4

#pragma location=".ramdyndata"
static uint8_t  DmaBuffer5Rx[DMABUFFER5_RX_SIZE];       // Rx DMA buffer UART 5 (WiFi)

static uint16_t Dma5TailCounter;                        // Index of oldest unretrieved byte in Rx DMA buffer 5

#pragma location=".ramdyndata"                                // Store buffer in external RAM
static uint8_t  DmaBuffer0Tx[DMABUFFER0_TX_SIZE];       // Tx DMA buffer UART 0 (Adapter)

#pragma location=".ramdyndata"                                // Store buffer in external RAM
static uint8_t  DmaBuffer4Tx[DMABUFFER4_TX_SIZE];       // Tx DMA buffer UART 4 (IrDA)

#pragma location=".ramdyndata"                                // Store buffer in external RAM
static uint8_t  DmaBuffer5Tx[DMABUFFER5_TX_SIZE];       // Tx DMA buffer UART 5 (WiFi)

// Channel DMA config array

static UART_CFG UartCfg[3] =
{
    {
        UART0_BASE_PTR,             ///< UART base pointer
        UART0_RX_DMA_CHANNEL,       ///< Rx DMA channel number
        UART0_TX_DMA_CHANNEL,       ///< Tx DMA channel number
        &DmaBuffer0Rx[0],           ///< Pointer to Rx DMA buffer
        &DmaBuffer0Tx[0],           ///< Pointer to Tx DMA buffer
        DMABUFFER0_RX_SIZE,         ///< Rx DMA buffer size
        DMABUFFER0_TX_SIZE,         ///< Tx DMA buffer size
        (uint32_t *)&SIM_SCGC4,     ///< Pointer to UART clock gate register
        SIM_SCGC4_UART0_MASK,       ///< UART clock gate mask
        &Dma0TailCounter,           ///< Pointer to Rx Tail Counter
        UART0_ISR_PRIORITY,         ///< DMA ISR priority (Rx & Tx)
        DMA_ERQ_ERQ2_MASK,          ///< Rx DMA enable mask
        DMA_ERQ_ERQ11_MASK,         ///< Tx DMA enable mask
        UART0_RX_DMA_IRQ,           ///< Rx DMA interrupt vector
        UART0_TX_DMA_IRQ,           ///< Tx DMA interrupt vector
        UART0_ERROR_IRQ,            ///< UART error interrupt vector
        UART0_FIFO_SIZE,            ///< UART FIFO size
        &Uart0Err                   ///< Pointer to accumulated UART error
    },

    {
        UART4_BASE_PTR,             ///< UART base pointer
        UART4_RX_DMA_CHANNEL,       ///< Rx DMA channel number
        UART4_TX_DMA_CHANNEL,       ///< Tx DMA channel number
        &DmaBuffer4Rx[0],           ///< Pointer to Rx DMA buffer
        &DmaBuffer4Tx[0],           ///< Pointer to Tx DMA buffer
        DMABUFFER4_RX_SIZE,         ///< Rx DMA buffer size
        DMABUFFER4_TX_SIZE,         ///< Tx DMA buffer size
        (uint32_t *)&SIM_SCGC1,     ///< Pointer to UART clock gate register
        SIM_SCGC1_UART4_MASK,       ///< UART clock gate mask
        &Dma4TailCounter,           ///< Pointer to Rx Tail Counter
        UART4_ISR_PRIORITY,         ///< DMA ISR priority (Rx & Tx)
        DMA_ERQ_ERQ3_MASK,          ///< Rx DMA enable mask
        DMA_ERQ_ERQ10_MASK,         ///< Tx DMA enable mask
        UART4_RX_DMA_IRQ,           ///< Rx DMA interrupt vector
        UART4_TX_DMA_IRQ,           ///< Tx DMA interrupt vector
        UART4_ERROR_IRQ,            ///< UART error interrupt vector
        UART4_FIFO_SIZE,            ///< UART FIFO size
        &Uart4Error                 ///< Pointer to accumulated UART error
    },

    {
        UART5_BASE_PTR,             ///< UART base pointer
        UART5_RX_DMA_CHANNEL,       ///< Rx DMA channel number
        UART5_TX_DMA_CHANNEL,       ///< Tx DMA channel number
        &DmaBuffer5Rx[0],           ///< Pointer to Rx DMA buffer
        &DmaBuffer5Tx[0],           ///< Pointer to Tx DMA buffer
        DMABUFFER5_RX_SIZE,         ///< Rx DMA buffer size
        DMABUFFER5_TX_SIZE,         ///< Tx DMA buffer size
        (uint32_t *)&SIM_SCGC1,     ///< Pointer to UART clock gate register
        SIM_SCGC1_UART5_MASK,       ///< UART clock gate mask
        &Dma5TailCounter,           ///< Pointer to Rx Tail Counter
        UART5_ISR_PRIORITY,         ///< DMA ISR priority (Rx & Tx)
        DMA_ERQ_ERQ4_MASK,          ///< Rx DMA enable mask
        DMA_ERQ_ERQ9_MASK,          ///< Tx DMA enable mask
        UART5_RX_DMA_IRQ,           ///< Rx DMA interrupt vector
        UART5_TX_DMA_IRQ,           ///< Tx DMA interrupt vector
        UART5_ERROR_IRQ,            ///< UART error interrupt vector
        UART5_FIFO_SIZE,            ///< UART FIFO size
        &Uart5Err                   ///< Pointer to accumulated UART error
    }
};

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
 * \brief   Initialize the specified UART
 *
 * \details The specified UART is initialized for 8N1 operation, no hardware
 *          flow-control, DMA driven on both tx & rx channels.
 *
 * \note    UART clocks must have previously been initialized and enabled
 *          The clock speed used for UART baud rate calculation is defined by
 *          the symbol UART_CLOCK.
 *
 * \param   Chan - UART channel to initialize
 * \param   Baud - Baud rate to set
 *
 * \return  UART_STATUS - Initialization status
 *
 * ========================================================================== */
UART_STATUS L2_UartInit(UART_CHANNEL Chan, uint32_t Baud)
{
    register uint16_t   Sbr;            // Calculated divisor for specified Baud rate
    register uint16_t   Brfa;           // Calculated fractional divisor for spedified Baud rate
    uint8_t             Temp;           // Temp value for register configurations
    UART_MemMapPtr      UartChannel;    // UART hardware base address
    UART_STATUS         Status;         // Error Status
    UART_CFG            *pCfg;          // Pointer to configuration for given channel

    Status = UART_STATUS_OK;

    // NOTE: do/while(false) loop is a convenient device to avoid nested ifs.
    //       when an error is detected, the Status is set and a break is executed
    //       to exit the routine immediately.

    do
    {
        if (Chan >= UART_COUNT)      // Validate UART channel
        {
            Status = UART_STATUS_INVALID_UART;
            break;
        }

        if (0 == Baud)              // Check for 0 baud rate
        {
            Status = UART_STATUS_BAUD_ERR;
            break;
        }

        Sbr = (uint16_t)((UART_CLOCK) / (Baud * UART_CLOCK_SCL));   // Calculate baud rate divisor & exit bad if out of range

        if (Sbr & ~(UART_SBR_MASK) || (Sbr == 0))                                // Set error if divisor too big
        {
            Status = UART_STATUS_BAUD_ERR;                                       // Can't achieve requested baud rate with requested clock
            break;
        }

        Brfa = FRACTIONAL_BAUD(Baud, Sbr);                                       // Baud rate OK - Calculate fractional divider to increase baud rate accuracy

        // Initialize DMA for the selected UART:

        pCfg = &UartCfg[Chan];                                                   // Pointer to UART configuration parameters
        UartChannel = pCfg->UartAdr;                                             // UART base address

        *pCfg->pClockGate |= pCfg->ClockMask;                                    // Enable UART clock. Do NOT disturb any existing clocks.

        UART_C2_REG(UartChannel) = 0;                                            // Disable UART TX/RX
        UART_C5_REG(UartChannel) = UART_C5_RDMAS_MASK | UART_C5_TDMAS_MASK;      // Enable DMA on the UART for RX & TX

        // Rx DMA initialization:

        DMA_SADDR_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = (uint32_t)&UART_D_REG(UartChannel);    // Source address
        DMA_SOFF_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = 0;                                      // Source offset per read
        DMA_DADDR_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = (uint32_t)pCfg->pRxDmaBuf;             // Destination address
        DMA_DOFF_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = 1;                                      // Destination offset per read
        DMA_SLAST_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = 0;                                     // Source Last Address Adjustment
        DMA_DLAST_SGA_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = 0;                                 // Destination Last Address Adjustment

        // set source and destination transfer sizes to 8-bit,
        // source modulo = 0, destination modulo = 0
        DMA_ATTR_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = DMA_ATTR_SSIZE(0) | DMA_ATTR_DSIZE(0) | DMA_ATTR_SMOD(0) | DMA_ATTR_DMOD(0);

        DMA_NBYTES_MLOFFNO_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = DMA_NBYTES_MLOFFNO_NBYTES(1U);               // Minor byte transfer count
        DMA_CSR_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = DMA_CSR_DREQ_MASK | DMA_CSR_INTMAJOR_MASK;
        DMA_BITER_ELINKNO_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = DMA_BITER_ELINKNO_BITER(pCfg->RxDmaBufSize);  // Beginning interation count (reset value)
        DMA_CITER_ELINKNO_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = DMA_CITER_ELINKNO_CITER(pCfg->RxDmaBufSize);  // Current interation count (byte transfer)
        DMA_ERQ |= pCfg->RxDmaEnableMask;

        *pCfg->pRxTailCtr = 0;                                 // Reset current number of bytes in DMA buffer
        *pCfg->pErr = 0;                                       // Reset accumulated UART errors

        // Tx DMA initialization:

        DMA_SADDR_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) = (uint32_t)pCfg->pTxDmaBuf;             // Source address
        DMA_SOFF_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) = 1;                                      // Source offset per read
        DMA_DADDR_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) = (uint32_t)&UART_D_REG(UartChannel);    // Destination address
        DMA_DOFF_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) = 0;                                      // Destination offset per read
        DMA_SLAST_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) = 0;                                     // Source Last Address Adjustment
        DMA_DLAST_SGA_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) = 0;                                 // Destination Last Address Adjustment

        // set source and destination transfer sizes to 8-bit,
        // source modulo = 0, destination modulo = 0
        DMA_ATTR_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) = DMA_ATTR_SSIZE(0) | DMA_ATTR_DSIZE(0) | DMA_ATTR_SMOD(0) | DMA_ATTR_DMOD(0);

        DMA_NBYTES_MLOFFNO_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) = DMA_NBYTES_MLOFFNO_NBYTES(1U);   // Minor byte transfer count
        DMA_CSR_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) = DMA_CSR_DREQ_MASK | DMA_CSR_INTMAJOR_MASK;  // Clear ERQ bit on completion and interrupt
                                                                                                    // Tx BITER and CITER are set at transmit time
        // Enable the TX and RX fifos.
        // UART0 and UART1 contains 8-entry transmit and 8-entry receive FIFOs.
        // All other UARTs contain a 1-entry transmit and receive FIFOs.
        UART_PFIFO_REG(UartChannel) = UART_PFIFO_RXFIFOSIZE(pCfg->UartFifoSize) |
                                      UART_PFIFO_TXFIFOSIZE(pCfg->UartFifoSize) |
                                      UART_PFIFO_TXFE_MASK |
                                      UART_PFIFO_RXFE_MASK;

        // When the number of data words in the Receive FIFO/buffer is equal to or greater than
        // the value in this register field, the event is flagged
        UART_RWFIFO_REG(UartChannel) = 1U;
        UART_TWFIFO_REG(UartChannel) = 0;

        // DMA initialization complete. Initialize UART:

        UART_C2_REG(UartChannel) &= ~(UART_C2_TCIE_MASK | UART_C2_TE_MASK | UART_C2_RE_MASK);                       // Disable Tx & Rx while we change settings.
        UART_C1_REG(UartChannel) = 0;                                                                               // Configure the UART for 8-bit mode, no parity (All default settings)
        UART_C3_REG(UartChannel) |= UART_C3_PEIE_MASK | UART_C3_FEIE_MASK | UART_C3_NEIE_MASK | UART_C3_ORIE_MASK;  // Enable Parity Error, Overrun Error, Noise Error, Framing Error

        Temp = UART_BDH_REG(UartChannel) & ~(UART_BDH_SBR(UART_BDH_SBR_MASK));  // Save the current value of the UARTx_BDH except for the SBR field

        // Set SBR bits and clear RXEDGIE bit (disable RxD interrupts)
        UART_BDH_REG(UartChannel) = (Temp |  UART_BDH_SBR(((Sbr >> 8) & UART_BDH_SBR_MASK)));   // NOTE: K20 manual specifies BDH must be written first!
        UART_BDL_REG(UartChannel) = (uint8)(Sbr & UART_BDL_SBR_MASK);

        Temp = UART_C4_REG(UartChannel) & ~(UART_C4_BRFA(UART_C4_BRFA_MASK));   // Save current value of UARTx_C4 register except for BRFA field

        UART_C4_REG(UartChannel) = Temp | UART_C4_BRFA(Brfa);                   // Set fractional divisor (if any)
        UART_C2_REG(UartChannel) |= (UART_C2_TIE_MASK | UART_C2_RIE_MASK);      // Enable Tx/Rx DMA requests
        UART_C2_REG(UartChannel) |= (UART_C2_TE_MASK | UART_C2_RE_MASK);        // Enable receiver and transmitter

        // Set Tx & Rx DMA interrupts:

        Enable_IRQ(pCfg->TxDmaIrq);
        Set_IRQ_Priority(pCfg->TxDmaIrq, pCfg->DmaIsrPriority);

        Enable_IRQ(pCfg->RxDmaIrq);
        Set_IRQ_Priority(pCfg->RxDmaIrq, pCfg->DmaIsrPriority);

        Enable_IRQ(pCfg->UartErrorIrq);

    } while (false);

    return (Status);
}

/* ========================================================================== */
/**
 * \brief   Flush the specified UART FIFO
 *
 * \details Flush the specified UART Rx & Tx FIFO.
 *
 * \note    This function is intended to be called when the adapter is removed,
 *          in order to prevent partial data in the UART Rx buffer from going into
 *          the DMA buffer once the UART buffer is filled at the next adapter response.
 *          This can cause out of sync responses. (Original design intent)
 * \n \n
 *          Both the UART FIFOs are cleared, and the DMA Rx buffers are marked as
 *          empty. Any transmission in progress will be lost.
 * \n \n
 *          The user may have to call L2_UartFlush multiple times if a transmission
 *          to the UART is in progress. The caller should use L2_UartGetRxByteCount
 *          to verify that the receive buffer has been truly flushed.
 *
 * \param   Chan - UART to flush.
 *
 * \return  UART_STATUS - Status of operation. (UART_STATUS_INVALID_UART if chan invalid)
 *
 * ========================================================================== */
UART_STATUS L2_UartFlush(UART_CHANNEL Chan)
{
    UART_CFG    *pCfg;
    UART_STATUS Status;

    Status = UART_STATUS_INVALID_UART;      // Default to bad uart channel

    if (Chan < UART_COUNT)
    {
        pCfg = &UartCfg[Chan];
        DMA_CERQ = pCfg->RxDmaChannel;                                                                      // Stop Rx DMA
        DMA_CERQ = pCfg-> TxDmaChannel;                                                                     // Stop Tx DMA
        UART_CFIFO_REG(pCfg->UartAdr) |= UART_FLUSH_RX_BUFFER | UART_FLUSH_TX_BUFFER;                       // Clear UART FIFOs
        UART_CFIFO_REG(pCfg->UartAdr) &= ~(UART_FLUSH_RX_BUFFER | UART_FLUSH_TX_BUFFER);                    // Release UART FIFOs
        *pCfg->pRxTailCtr = pCfg->RxDmaBufSize - DMA_CITER_ELINKNO_REG(DMA_BASE_PTR, pCfg->RxDmaChannel);   // Set tail ctr = head ctr (buffer empty) CITER counts down, TailCtr counts up
        DMA_SERQ = pCfg->RxDmaChannel;                                                                      // Start Rx DMA (Tx DMA will start at next WriteBlock)
        Status = UART_STATUS_OK;
    }

    return (Status);
}

/* ========================================================================== */
/**
 * \brief   Retrieve a block of data from the specified UART Rx circular buffer.
 *
 * \details Copy all currently received data from the specified UART to the
 *          specified buffer. This function is non-blocking. It is the caller's
 *          responsibility to determine if the requisite data has been received.
 *
 * \param   Chan          - UART to retrieve data from
 * \param   pDataIn       - Pointer to buffer to receive data
 * \param   MaxDataCount  - Maximum number of bytes to copy
 * \param   pBytesRcd     - Pointer to store number of bytes returned. NULL if not used
 *
 * \return  UART_STATUS
 * \retval  UART_STATUS_OK - Success
 * \retval  UART_STATUS_INVALID_UART - Bad UART channel specified
 * \retval  UART_STATUS_INVALID_PTR - Bad pointer to return data
 *
 * ========================================================================== */
UART_STATUS L2_UartReadBlock(UART_CHANNEL Chan, uint8_t *pDataIn, uint16_t MaxDataCount, uint16_t *pBytesRcd)
{
    uint16_t    DataCount;      // Number of characters returned
    uint16_t    HeadCounter;    // Index into dma buffer
    UART_CFG    *pCfg;          // Pointer to configuration for given channel
    UART_STATUS Status;         // Error status

    Status = UART_STATUS_OK;
    DataCount = 0;              // Default to no data transfer

    do
    {
        // Check channel & pointer for validity:

        if (Chan >= UART_COUNT)
        {
            Status = UART_STATUS_INVALID_UART;
            break;
        }

        if (NULL == pDataIn)
        {
            Status = UART_STATUS_INVALID_PTR;
            break;
        }

        pCfg = &UartCfg[Chan];          // Pointer to UART configuration for this channel

        // HeadCounter and dmaXTailCounter start at zero and count up to ->RxDmaBufSize.
        HeadCounter = pCfg->RxDmaBufSize - DMA_CITER_ELINKNO_REG(DMA_BASE_PTR, pCfg->RxDmaChannel);
        if (HeadCounter == *pCfg->pRxTailCtr)
        {
            DataCount = 0;                                                      // No data in DMA buffer
            Status = UART_STATUS_RX_BUFFER_EMPTY;
        }
        else                                                                    // DMA buffer not empty
        {
            // Copy data to output:

            if (HeadCounter > *pCfg->pRxTailCtr)
            {
                // Data does not wrap around in DMA buffer. It can be extracted with a single memcpy call.

                DataCount = HeadCounter - *pCfg->pRxTailCtr;                        // Calclate number of bytes in buffer

                if (DataCount > MaxDataCount)
                {
                    DataCount = MaxDataCount;                                       // More data is available than requested. Return only the requested amount.
                }

                memcpy(pDataIn, pCfg->pRxDmaBuf + *pCfg->pRxTailCtr, DataCount);
            }
            else
            {
                uint32_t DataCountToEnd;    // Number of bytes to end of buffer

                // Data in DMA buffer wraps around to the beginning of the buffer. We may need two memcpy calls to extract data.

                DataCount = pCfg->RxDmaBufSize - (*pCfg->pRxTailCtr - HeadCounter); // Calculate number of bytes in buffer
                DataCountToEnd = pCfg->RxDmaBufSize - *pCfg->pRxTailCtr;            // Calculate number of bytes to end of buffer

                if (DataCount > MaxDataCount)
                {
                    DataCount = MaxDataCount;                                       // More data is available than requested. Return only the requested amount.
                }

                if (DataCount < DataCountToEnd)                                     // Data requested does not wrap around - Single memcpy call is OK
                {
                    memcpy(pDataIn, pCfg->pRxDmaBuf + *pCfg->pRxTailCtr, DataCount);
                }
                else                                                                // Data requested DOES wrap around - two memcpy calls are needed
                {
                    memcpy(pDataIn, pCfg->pRxDmaBuf + *pCfg->pRxTailCtr, DataCountToEnd);           // Copy data to end of buffer
                    memcpy(pDataIn + DataCountToEnd, pCfg->pRxDmaBuf, DataCount - DataCountToEnd);  // Copy remaining data
                }
            }

            *pCfg->pRxTailCtr += DataCount;
            if (*pCfg->pRxTailCtr >= pCfg->RxDmaBufSize)
            {
                *pCfg->pRxTailCtr -= pCfg->RxDmaBufSize;    // Pointer past end of buffer - wrap around
            }
        }
    } while (false);

    if (NULL != pBytesRcd)
    {
        *pBytesRcd = DataCount;         // Return byte count
    }

    return (Status);                    // Return status
}

/* ========================================================================== */
/**
 * \brief   Queue a block of data for transmission by the specified UART
 *
 * \details Data is placed in the specified transmit DMA buffer and transmission is
 *          started. This function is non-blocking. It is the caller's responsibility
 *          to determine when the transmission is complete.
 *
 * \note    BytesQueued is 0 if the UART is currently busy transmitting data. No data
 *          will be queued. If the number of bytes reqested to be sent is greater
 *          than the DMA buffer size, only the DMA buffer size number of bytes will
 *          be queued. The caller is responsible for queueing the rest of the data
 *          via a subsequent L2_UartWriteBlock call.
 *
 * \param   Chan          - UART to send data from
 * \param   pDataOut      - Pointer to data to send
 * \param   MaxDataCount  - Maximum number of bytes to send
 * \param   pBytesQueued  - Pointer to store number of bytes queued. NULL if not used
 *
 * \return  UART_STATUS
 * \retval  UART_STATUS_OK - Success
 * \retval  UART_STATUS_TX_BUSY - Transmit in progress, data not queued
 * \retval  UART_STATUS_INVALID_UART - Bad UART channel specified
 * \retval  UART_STATUS_INVALID_PTR - Bad pointer to return data
 *
 * ========================================================================== */
UART_STATUS L2_UartWriteBlock(UART_CHANNEL Chan, uint8_t *pDataOut, uint16_t MaxDataCount, uint16_t *pBytesQueued)
{
    uint16_t    DataQueued;     // Number of bytes queued for output
    UART_CFG    *pCfg;          // Pointer to UART parameters
    UART_STATUS Status;         // Return status

    Status = UART_STATUS_OK;
    DataQueued = 0;             // Default to no data transfer

    do
    {
        // Check channel & pointer for validity:

        if (Chan >= UART_COUNT)
        {
            Status = UART_STATUS_INVALID_UART;
            break;
        }

        if (NULL == pDataOut)
        {
            Status = UART_STATUS_INVALID_PTR;
            break;
        }

        pCfg = &UartCfg[Chan];                      // Point to configuration for this UART

        if (0 != (DMA_ERQ & pCfg->TxDmaEnableMask) || (0 == (UART_S1_REG(pCfg->UartAdr) & UART_S1_TC_MASK )))
        {
            Status = UART_STATUS_TX_BUSY;
            break;
        }

        if (MaxDataCount > pCfg->TxDmaBufSize)          // Number of bytes to transfer must not exceed dma buffer size
        {
            DataQueued = pCfg->TxDmaBufSize;
        }
        else
        {
            DataQueued = MaxDataCount;
        }

        memcpy(pCfg->pTxDmaBuf, pDataOut, DataQueued);
        DMA_SADDR_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) = (uint32_t)pCfg->pTxDmaBuf;                    // Set source address
        DMA_BITER_ELINKNO_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) = DMA_BITER_ELINKNO_BITER(DataQueued);  // CITER reset value
        DMA_CITER_ELINKNO_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) = DMA_CITER_ELINKNO_CITER(DataQueued);  // CITER initial value
        DMA_SERQ = pCfg->TxDmaChannel;                                                                  // Set dma request

    } while (false);

    if (NULL != pBytesQueued)
    {
        *pBytesQueued = DataQueued;      // Return bytes queued if pointer specified
    }
    return (Status);
}

/* ========================================================================== */
/**
 * \brief   Get the number of bytes currently waiting to be transferred.
 *
 * \details Allows caller to estimate the number of bytes left to transmit.
 *          The value returned does not include any bytes in the FIFO, so it
 *          is a bit lower than the actual number of bytes to be transmitted.
 *
 * \note    When the DMA transfer is complete, but the UART TC (Transmit Complete)
 *          bit has not been set, this function returns 1 regardless of how many bytes
 *          may actually be in the FIFO. This guarantees that 0 will be returned
 *          0 only when transmission of all bytes is complete.
 *
 * \param   Chan - UART Tx DMA channel to check
 *
 * \return  uint16_t - Number of bytes remaining in the transmit DMA buffer. Returns 0xFFFF if chan is invalid.
 *
 * ========================================================================== */
uint16_t L2_UartGetTxByteCount(UART_CHANNEL Chan)
{
    uint16_t    DataCount;      // Number of characters returned
    UART_CFG    *pCfg;          // Pointer to UART configuration

    do
    {
        if (Chan >= UART_COUNT)      // Validate UART channel
        {
            DataCount = MAX_UINT16;
            break;
        }

        pCfg = &UartCfg[Chan];                      // Point to configuration for this UART

        // Test DMA_ERQ bit in addition to DONE bit in case this function is called without
        // a DMA transfer having been started.

        if ((DMA_CSR_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) & DMA_CSR_DONE_MASK) || (0 == (DMA_ERQ & pCfg->TxDmaEnableMask)))
        {
            DataCount = 0;                                              // Tx DMA is complete
            if (0 == (UART_S1_REG(pCfg->UartAdr) & UART_S1_TC_MASK ))
            {
                DataCount = 1;                                          // But UART is still transmitting
            }
        }
        else
        {
            DataCount = DMA_CITER_ELINKNO_REG(DMA_BASE_PTR, pCfg->TxDmaChannel) & DMA_CITER_ELINKNO_CITER_MASK; // Return current iteration count
        }
    } while (false);

    return (DataCount);
}


/* ========================================================================== */
/**
 * \brief   Get number of bytes currently in receive buffer.
 *
 * \details Allows caller to determine if a full message has been received.
 *
 * \note    Does not include any bytes that may be in the UART fifo.
 *
 * \param   Chan - UART buffer to check
 *
 * \return  uint16_t - Number of bytes in receive circular buffer. Returns 0xFFFF if chan is invalid
 *
 * ========================================================================== */
uint16_t L2_UartGetRxByteCount(UART_CHANNEL Chan)
{
    uint16_t    DataCount;          // Number of bytes in dma buffer
    uint16_t    HeadCounter;        // Index into dma buffer
    UART_CFG    *pCfg;              // Pointer to UART configuration

    do
    {
        if (Chan >= UART_COUNT)     // Validate UART channel
        {
            DataCount = MAX_UINT16;
            break;
        }

        pCfg = &UartCfg[Chan];      // Point to configuration for this UART

        // HeadCounter and dmaxTailCounter start at zero and count up to ->RxDmaBufSize.

        HeadCounter = pCfg->RxDmaBufSize - DMA_CITER_ELINKNO_REG(DMA_BASE_PTR, pCfg->RxDmaChannel);
        if (HeadCounter == *pCfg->pRxTailCtr)
        {
            DataCount = 0;                                                      // No data in DMA buffer
        }
        else if (HeadCounter > *pCfg->pRxTailCtr)                                // Calculate # of bytes in DMA buffer
        {
            DataCount = HeadCounter - *pCfg->pRxTailCtr;                         // No wraparound
        }
        else
        {
            DataCount =  pCfg->RxDmaBufSize - (*pCfg->pRxTailCtr - HeadCounter); // Wraparound
        }

    } while (false);

    return (DataCount);
}


/* ========================================================================== */
/**
 * \brief   Returns accumulated errors for specified UART since last call.
 *
 * \details Retrieves UART error flags Overrun, Noise, Framing, and Parity,
 *          and clears the accumulated value. See UART_OR_MASK, UART_NF_MASK,
 *          UART_FE_MASK, UART_PF_MASK in L2_UART.h
 *
 * \note    Returns 0xFF if Chan is invalid
 *
 * \param   Chan - UART to retrieve errors for.
 *
 * \return  uint8_t - Accumulated errors. See K20P144M120F3RM.pdf for details
 *
 * ========================================================================== */
uint8_t L2_UartGetError(UART_CHANNEL Chan)
{
    uint8_t     Error;          // Error code returned
    UART_CFG    *pCfg;          // Pointer to UART configuration

    switch (Chan)
    {
        case UART0:                 // Processing for all UARTS is the same.
        case UART4:                 // This is simply a check for a valid
        case UART5:                 // UART channel number.
            pCfg = &UartCfg[Chan];
            Error = *pCfg->pErr;    // Retrieve Errors
            *pCfg->pErr = 0;        // Clear accumulated Errors
            break;

        default:
            Error = 0xFF;           // Non-existent uart channel
            break;
    }
    return (Error);
}

/* ========================================================================== */
/**
 * \brief   Enable loopback on a given UART port.
 *
 * \details Allows device level testing and diagnostics of a UART port by
 *          looping the transmitter to the receiver at the K20.
 *
 * \param   Chan - UART channel to loopback
 *
 * \return  None
 *
 * ========================================================================== */
void L2_UartLoopbackEnable(UART_CHANNEL Chan)
{
    UART_CFG    *pCfg;          // Pointer to UART configuration

    pCfg = &UartCfg[Chan];      // Point to configuration for this UART
    UART_C1_REG(pCfg->UartAdr) |= UART_LOOPBACK_ENABLE;
}

/* ========================================================================== */
/**
 * \brief   Disable loopback on a given UART port.
 *
 * \details Disables the looping of the transmitter to the receiver at the K20.
 *
 * \param   Chan - UART channel to disable loopback
 *
 * \return  None
 *
 * ========================================================================== */
void L2_UartLoopbackDisable(UART_CHANNEL Chan)
{
    UART_CFG    *pCfg;          // Pointer to UART configuration

    pCfg = &UartCfg[Chan];      // Point to configuration for this UART
    UART_C1_REG(pCfg->UartAdr) &= UART_LOOPBACK_DISABLE;
}

/* ========================================================================== */
/**
 * \brief   UART0 Error Handler ISR - Handles Parity-, Framing-, Noise-, and Overrun errors.
 *
 * \details The errors are accumulated for retrieval by L2_UartGetError().
 *
 * \sa      Chapter 52 of K20 SubFamily Reference Manual (provide link to K20P144M120SF3RM.pdf)
 *
 * \param   < None >
 * \return  None
 *
 * ========================================================================== */
void L2_Uart0Error_ISR(void)
{
    uint8_t     TempCharIn;     // Dummy for register read
    uint8_t     RegS1Vaiue;     // UART S1 register temporary
    OS_CPU_SR   cpu_sr;         // CPU status register for critical section macro
    UART_CFG    *pCfg;          // Pointer to UART configuration

    TempCharIn = 0u; // Initialize variable

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    pCfg = &UartCfg[UART0];

    RegS1Vaiue = UART_S1_REG(pCfg->UartAdr);
    *pCfg->pErr |= RegS1Vaiue & (UART_S1_PF_MASK | UART_S1_FE_MASK | UART_S1_NF_MASK | UART_S1_OR_MASK);    // Update error flags

    if (RegS1Vaiue & (UART_S1_PF_MASK | UART_S1_FE_MASK | UART_S1_NF_MASK | UART_S1_OR_MASK))
    {
        TempCharIn = UART_D_REG(pCfg->UartAdr);                                                             // Read Data register to clear errors. See Ch 52 for details
    }

    TempCharIn = TempCharIn;        // Prevent compiler warning.
    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   Handle end of UART0 Rx DMA channel transfer (Buffer full)
 *
 * \note    This routine must also handle the DMA channel n+16 interrupt
 *          if present. (See UartCfg array for DMA channel details)
 *
 * \details Reset destination address to the start of the buffer
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_Uart0RxDma_ISR(void)
{
    OS_CPU_SR   cpu_sr;         // CPU status register for critical section macro
    UART_CFG    *pCfg;          // Pointer to UART configuration

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    pCfg = &UartCfg[UART0];

    if (DMA_INT & pCfg->RxDmaEnableMask)            // Interrupt mask same as DMA enable mask
    {
        DMA_CINT = pCfg->RxDmaChannel;              // Clear interrupt mask.
    }

    if (DMA_CSR_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) & DMA_CSR_DONE_MASK)
    {
        DMA_DADDR_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = (uint32_t)pCfg->pRxDmaBuf;    // Reset Destination Address
        DMA_SERQ = pCfg->RxDmaChannel;              // Restart DMA
    }

    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   Handle end of UART0 Tx DMA channel 11 transfer (Transmission complete)
 *
 * \note    This routine must also handle the DMA channel n+16 interrupt
 *          if present. (See UartCfg array for DMA channel details)
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_Uart0TxDma_ISR(void)
{
    OS_CPU_SR   cpu_sr;         // CPU status register for critical section macro
    UART_CFG    *pCfg;          // Pointer to UART configuration

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    pCfg = &UartCfg[UART0];

    if (DMA_INT & pCfg->TxDmaEnableMask)
    {
        DMA_CINT = pCfg->TxDmaChannel;     // Clear interrupt mask
    }

    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   UART4 Error Handler ISR - Handles Parity-, Framing-, Noise-, and Overrun errors.
 *
 * \details The errors are accumulated for retrieval by L2_UartGetError().
 *
 * \sa      Chapter 52 of K20 SubFamily Reference Manual (provide link to K20P144M120SF3RM.pdf)
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_Uart4Error_ISR(void)
{
    uint8_t     TempCharIn;     // Dummy for register read
    uint8_t     RegS1Vaiue;     // UART S1 register temporary
    OS_CPU_SR   cpu_sr;         // CPU status register for critical section macro
    UART_CFG    *pCfg;          // Pointer to UART configuration

    TempCharIn = 0u; // Initialize variable

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    pCfg = &UartCfg[UART4];

    RegS1Vaiue = UART_S1_REG(pCfg->UartAdr);
    *pCfg->pErr |= RegS1Vaiue & (UART_S1_PF_MASK | UART_S1_FE_MASK | UART_S1_NF_MASK | UART_S1_OR_MASK);    // Update error flags

    if (RegS1Vaiue & (UART_S1_PF_MASK | UART_S1_FE_MASK | UART_S1_NF_MASK | UART_S1_OR_MASK))
    {
        TempCharIn = UART_D_REG(pCfg->UartAdr);                                                             // Read D register to clear errors. See Ch 52 for details
    }

    TempCharIn = TempCharIn;        // Prevent compiler warning.
    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   Handle end of UART4 Rx DMA channel transfer (Buffer full)
 *
 * \note    This routine must also handle the DMA channel n+16 interrupt
 *          if present. (See UartCfg array for DMA channel details)
 *
 * \details Reset destination address to the start of the buffer
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_Uart4RxDma_ISR(void)
{
    OS_CPU_SR  cpu_sr;          // CPU status register for critical section macro
    UART_CFG    *pCfg;          // Pointer to UART configuration

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    pCfg = &UartCfg[UART4];

    if (DMA_INT & pCfg->RxDmaEnableMask)            // Interrupt mask same as DMA enable mask
    {
        DMA_CINT = pCfg->RxDmaChannel;              // Clear interrupt mask.
    }

    if (DMA_CSR_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) & DMA_CSR_DONE_MASK)
    {
        DMA_DADDR_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = (uint32_t)pCfg->pRxDmaBuf;    // Reset Destination Address
        DMA_SERQ = pCfg->RxDmaChannel;              // Restart DMA
    }

    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   Handle end of UART4 Tx DMA channel 10 transfer (Transmission complete)
 *
 * \note    This routine must also handle the DMA channel n+16 interrupt
 *          if present. (See UartCfg array for DMA channel details)
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_Uart4TxDma_ISR(void)
{
    OS_CPU_SR  cpu_sr;          // CPU status register for critical section macro
    UART_CFG    *pCfg;          // Pointer to UART configuration

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    pCfg = &UartCfg[UART4];

    if (DMA_INT & pCfg->TxDmaEnableMask)
    {
        DMA_CINT = pCfg->TxDmaChannel;     // Clear interrupt mask
    }

    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   UART5 Error Handler ISR - Handles Parity-, Framing-, Noise-, and Overrun errors.
 *
 * \details Currently, the errors are just cleared. They are not saved.
 *
 * \sa      Chapter 52 of K20 SubFamily Reference Manual (provide link to K20P144M120SF3RM.pdf)
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_Uart5Error_ISR(void)
{
    uint8_t     TempCharIn;     // Dummy for register read
    uint8_t     RegS1Vaiue;     // UART S1 register temporary
    OS_CPU_SR   cpu_sr;         // CPU status register for critical section macro
    UART_CFG    *pCfg;          // Pointer to UART configuration

    TempCharIn = 0u; // Initialize variable

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    pCfg = &UartCfg[UART5];

    RegS1Vaiue = UART_S1_REG(pCfg->UartAdr);
    *pCfg->pErr |= RegS1Vaiue & (UART_S1_PF_MASK | UART_S1_FE_MASK | UART_S1_NF_MASK | UART_S1_OR_MASK);    // Update error flags

    if (RegS1Vaiue & (UART_S1_PF_MASK | UART_S1_FE_MASK | UART_S1_NF_MASK | UART_S1_OR_MASK))
    {
        TempCharIn = UART_D_REG(pCfg->UartAdr);                                                             // Read D register to clear errors. See Ch 52 for details
    }

    TempCharIn = TempCharIn;        // Prevent compiler warning.
    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   Handle end of UART5 Rx DMA channel transfer (Buffer full)
 *
 * \note    This routine must also handle the DMA channel n+16 interrupt
 *          if present. (See UartCfg array for DMA channel details)
 *
 * \details Reset destination address to the start of the buffer
 *
 * \param   < None >
 *
 * \return  None
 *
 +* ========================================================================== */
void L2_Uart5RxDma_ISR(void)
{
    OS_CPU_SR  cpu_sr;          // CPU status register for critical section macro
    UART_CFG    *pCfg;          // Pointer to UART configuration

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    pCfg = &UartCfg[UART5];

    if (DMA_INT & pCfg->RxDmaEnableMask)            // Interrupt mask same as DMA enable mask
    {
        DMA_CINT = pCfg->RxDmaChannel;              // Clear interrupt mask.
    }

    if (DMA_CSR_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) & DMA_CSR_DONE_MASK)
    {
        DMA_DADDR_REG(DMA_BASE_PTR, pCfg->RxDmaChannel) = (uint32_t)pCfg->pRxDmaBuf;    // Reset Destination Address
        DMA_SERQ = pCfg->RxDmaChannel;              // Restart DMA
    }

    OSIntExit();
}

/* ========================================================================== */
/**
 * \brief   Handle end of UART5 Tx DMA channel 9 transfer (Transmission complete)
 *
 * \note    This routine must also handle the DMA channel n+16 interrupt
 *          if present. (See UartCfg array for DMA channel details)
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L2_Uart5TxDma_ISR(void)
{
    OS_CPU_SR  cpu_sr;          // CPU status register for critical section macro
    UART_CFG    *pCfg;          // Pointer to UART configuration

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    pCfg = &UartCfg[UART5];

    if (DMA_INT & pCfg->TxDmaEnableMask)
    {
        DMA_CINT = pCfg->TxDmaChannel;     // Clear interrupt mask
    }

    OSIntExit();
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
