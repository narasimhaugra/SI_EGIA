#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Dma
 * \{
 * \brief   Layer 2 DMA module
 *
 * \details This module enables the DMA subsystem used in the PowerPack. It also
 *          associates the DMA channels with a specific peripheral, and sets the
 *          DMA processing priority for each channel. This is detailed in L2_DmaInit()
 *          below. Each individual channel is further configured by the peripheral
 *          with which it is associated. This module also provides DMA error interrupt
 *          processing.
 *
 * \sa      Chapters 21, 22 of K20 SubFamily Reference Manual (provide link to K20P144M120SF3RM.pdf)
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_Dma.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L2_Dma.h"
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
#define DMA_CHANNEL_MASK_0          (1 << 0)   ///< DMA channel 0 error interrupt enable mask
#define DMA_CHANNEL_MASK_1          (1 << 1)   ///< DMA channel 1 error interrupt enable mask
#define DMA_CHANNEL_MASK_2          (1 << 2)   ///< DMA channel 2 error interrupt enable mask
#define DMA_CHANNEL_MASK_3          (1 << 3)   ///< DMA channel 3 error interrupt enable mask
#define DMA_CHANNEL_MASK_4          (1 << 4)   ///< DMA channel 4 error interrupt enable mask

#define DMA_CHANNEL_MASK_9         (1 <<  9)   ///< DMA channel 9 error interrupt enable mask
#define DMA_CHANNEL_MASK_10        (1 << 10)   ///< DMA channel 10 error interrupt enable mask
#define DMA_CHANNEL_MASK_11        (1 << 11)   ///< DMA channel 11 error interrupt enable mask

/* DMA multiplexer source slot assignments: */
#define DMA_MUX0_SOURCE_DISABLED        (0u)   ///< DMA Mux 0 Source DISABLED
#define DMA_MUX0_SOURCE_RESERVED        (1u)   ///< DMA Mux 0 Source RESERVED
#define DMA_MUX0_SOURCE_UART0_RX        (2u)   ///< DMA Mux 0 Source UART0_RX
#define DMA_MUX0_SOURCE_UART0_TX        (3u)   ///< DMA Mux 0 Source UART0_TX
#define DMA_MUX0_SOURCE_UART1_RX        (4u)   ///< DMA Mux 0 Source UART1_RX
#define DMA_MUX0_SOURCE_UART1_TX        (5u)   ///< DMA Mux 0 Source UART1_TX
#define DMA_MUX0_SOURCE_UART2_RX        (6u)   ///< DMA Mux 0 Source UART2_RX
#define DMA_MUX0_SOURCE_UART2_TX        (7u)   ///< DMA Mux 0 Source UART2_TX
#define DMA_MUX0_SOURCE_UART3_RX        (8u)   ///< DMA Mux 0 Source UART3_RX
#define DMA_MUX0_SOURCE_UART3_TX        (9u)   ///< DMA Mux 0 Source UART3_TX
#define DMA_MUX0_SOURCE_UART4_RX        (10u)  ///< DMA Mux 0 Source UART4_RX
#define DMA_MUX0_SOURCE_UART4_TX        (11u)  ///< DMA Mux 0 Source UART4_TX
#define DMA_MUX0_SOURCE_UART5_RX        (12u)  ///< DMA Mux 0 Source UART5_RX
#define DMA_MUX0_SOURCE_UART5_TX        (13u)  ///< DMA Mux 0 Source UART5_TX
#define DMA_MUX0_SOURCE_I2S0_RX         (14u)  ///< DMA Mux 0 Source I2S0_RX
#define DMA_MUX0_SOURCE_I2S0_TX         (15u)  ///< DMA Mux 0 Source I2S0_TX
#define DMA_MUX0_SOURCE_SPI0_RX         (16u)  ///< DMA Mux 0 Source SPI0_RX
#define DMA_MUX0_SOURCE_SPI0_TX         (17u)  ///< DMA Mux 0 Source SPI0_TX
#define DMA_MUX0_SOURCE_SPI1_RX         (18u)  ///< DMA Mux 0 Source SPI1_RX
#define DMA_MUX0_SOURCE_SPI1_TX         (19u)  ///< DMA Mux 0 Source SPI1_TX
#define DMA_MUX0_SOURCE_SPI2_RX         (20u)  ///< DMA Mux 0 Source SPI2_RX
#define DMA_MUX0_SOURCE_SPI2_TX         (21u)  ///< DMA Mux 0 Source SPI2_TX
#define DMA_MUX0_SOURCE_I2C0            (22u)  ///< DMA Mux 0 Source I2C0
#define DMA_MUX0_SOURCE_I2C1            (23u)  ///< DMA Mux 0 Source I2C1
#define DMA_MUX0_SOURCE_FTM0_CHAN0      (24u)  ///< DMA Mux 0 Source FTM0_CHAN0
#define DMA_MUX0_SOURCE_FTM0_CHAN1      (25u)  ///< DMA Mux 0 Source FTM0_CHAN1
#define DMA_MUX0_SOURCE_FTM0_CHAN2      (26u)  ///< DMA Mux 0 Source FTM0_CHAN2
#define DMA_MUX0_SOURCE_FTM0_CHAN3      (27u)  ///< DMA Mux 0 Source FTM0_CHAN3
#define DMA_MUX0_SOURCE_FTM0_CHAN4      (28u)  ///< DMA Mux 0 Source FTM0_CHAN4
#define DMA_MUX0_SOURCE_FTM0_CHAN5      (29u)  ///< DMA Mux 0 Source FTM0_CHAN5
#define DMA_MUX0_SOURCE_FTM0_CHAN6      (30u)  ///< DMA Mux 0 Source FTM0_CHAN6
#define DMA_MUX0_SOURCE_FTM0_CHAN7      (31u)  ///< DMA Mux 0 Source FTM0_CHAN7
#define DMA_MUX0_SOURCE_FTM1_CHAN0      (32u)  ///< DMA Mux 0 Source FTM1_CHAN0
#define DMA_MUX0_SOURCE_FTM1_CHAN1      (33u)  ///< DMA Mux 0 Source FTM1_CHAN1
#define DMA_MUX0_SOURCE_FTM2_CHAN0      (34u)  ///< DMA Mux 0 Source FTM2_CHAN0
#define DMA_MUX0_SOURCE_FTM2_CHAN1      (35u)  ///< DMA Mux 0 Source FTM2_CHAN1
#define DMA_MUX0_SOURCE_1588_TIMER0     (36u)  ///< DMA Mux 0 Source 1588_TIMER
#define DMA_MUX0_SOURCE_1588_TIMER1     (37u)  ///< DMA Mux 0 Source 1588_TIMER
#define DMA_MUX0_SOURCE_1588_TIMER2     (38u)  ///< DMA Mux 0 Source 1588_TIMER
#define DMA_MUX0_SOURCE_1588_TIMER3     (39u)  ///< DMA Mux 0 Source 1588_TIMER
#define DMA_MUX0_SOURCE_ADC0            (40u)  ///< DMA Mux 0 Source ADC0
#define DMA_MUX0_SOURCE_ADC1            (41u)  ///< DMA Mux 0 Source ADC1
#define DMA_MUX0_SOURCE_CMP0            (42u)  ///< DMA Mux 0 Source CMP0
#define DMA_MUX0_SOURCE_CMP1            (43u)  ///< DMA Mux 0 Source CMP1
#define DMA_MUX0_SOURCE_CMP2            (44u)  ///< DMA Mux 0 Source CMP2
#define DMA_MUX0_SOURCE_DAC0            (45u)  ///< DMA Mux 0 Source DAC0
#define DMA_MUX0_SOURCE_DAC1            (46u)  ///< DMA Mux 0 Source DAC1
#define DMA_MUX0_SOURCE_CMT             (47u)  ///< DMA Mux 0 Source CMT
#define DMA_MUX0_SOURCE_PDB             (48u)  ///< DMA Mux 0 Source PDB
#define DMA_MUX0_SOURCE_PORTA           (49u)  ///< DMA Mux 0 Source PORTA
#define DMA_MUX0_SOURCE_PORTB           (50u)  ///< DMA Mux 0 Source PORTB
#define DMA_MUX0_SOURCE_PORTC           (51u)  ///< DMA Mux 0 Source PORTC
#define DMA_MUX0_SOURCE_PORTD           (52u)  ///< DMA Mux 0 Source PORTD
#define DMA_MUX0_SOURCE_PORTE           (53u)  ///< DMA Mux 0 Source PORTE
#define DMA_MUX0_SOURCE_DMA_MUX         (54u)  ///< DMA Mux 0 Source DMA Mux

#define DMA_MUX1_SOURCE_DISABLED        (0u)   ///< DMA Mux 1 Source DISABLED
#define DMA_MUX1_SOURCE_RESERVED        (1u)   ///< DMA Mux 1 Source RESERVED
#define DMA_MUX1_SOURCE_UART0_RX        (2u)   ///< DMA Mux 1 Source UART0_RX
#define DMA_MUX1_SOURCE_UART0_TX        (3u)   ///< DMA Mux 1 Source UART0_TX
#define DMA_MUX1_SOURCE_UART1_RX        (4u)   ///< DMA Mux 1 Source UART1_RX
#define DMA_MUX1_SOURCE_UART1_TX        (5u)   ///< DMA Mux 1 Source UART1_TX
#define DMA_MUX1_SOURCE_UART2_RX        (6u)   ///< DMA Mux 1 Source UART2_RX
#define DMA_MUX1_SOURCE_UART2_TX        (7u)   ///< DMA Mux 1 Source UART2_TX
#define DMA_MUX1_SOURCE_UART3_RX        (8u)   ///< DMA Mux 1 Source UART3_RX
#define DMA_MUX1_SOURCE_UART3_TX        (9u)   ///< DMA Mux 1 Source UART3_TX
#define DMA_MUX1_SOURCE_UART4_RX        (10u)  ///< DMA Mux 1 Source UART4_RX
#define DMA_MUX1_SOURCE_UART4_TX        (11u)  ///< DMA Mux 1 Source UART4_TX
#define DMA_MUX1_SOURCE_UART5_RX        (12u)  ///< DMA Mux 1 Source UART5_RX
#define DMA_MUX1_SOURCE_UART5_TX        (13u)  ///< DMA Mux 1 Source UART5_TX
#define DMA_MUX1_SOURCE_I2S1_RX         (14u)  ///< DMA Mux 1 Source I2S1_RX
#define DMA_MUX1_SOURCE_I2S1_TX         (15u)  ///< DMA Mux 1 Source I2S1_TX
#define DMA_MUX1_SOURCE_SPI0_RX         (16u)  ///< DMA Mux 1 Source SPI0_RX
#define DMA_MUX1_SOURCE_SPI0_TX         (17u)  ///< DMA Mux 1 Source SPI0_TX
#define DMA_MUX1_SOURCE_SPI1_RX         (18u)  ///< DMA Mux 1 Source SPI1_RX
#define DMA_MUX1_SOURCE_SPI1_TX         (19u)  ///< DMA Mux 1 Source SPI1_TX
#define DMA_MUX1_SOURCE_SPI2_RX         (20u)  ///< DMA Mux 1 Source SPI2_RX
#define DMA_MUX1_SOURCE_SPI2_TX         (21u)  ///< DMA Mux 1 Source SPI2_TX
#define DMA_MUX1_SOURCE_FTM3_CHAN0      (24u)  ///< DMA Mux 1 Source FTM3_CHAN0
#define DMA_MUX1_SOURCE_FTM3_CHAN1      (25u)  ///< DMA Mux 1 Source FTM3_CHAN1
#define DMA_MUX1_SOURCE_FTM3_CHAN2      (26u)  ///< DMA Mux 1 Source FTM3_CHAN2
#define DMA_MUX1_SOURCE_FTM3_CHAN3      (27u)  ///< DMA Mux 1 Source FTM3_CHAN3
#define DMA_MUX1_SOURCE_FTM3_CHAN4      (28u)  ///< DMA Mux 1 Source FTM3_CHAN4
#define DMA_MUX1_SOURCE_FTM3_CHAN5      (29u)  ///< DMA Mux 1 Source FTM3_CHAN5
#define DMA_MUX1_SOURCE_FTM3_CHAN6      (30u)  ///< DMA Mux 1 Source FTM3_CHAN6
#define DMA_MUX1_SOURCE_FTM3_CHAN7      (31u)  ///< DMA Mux 1 Source FTM3_CHAN7
#define DMA_MUX1_SOURCE_ADC0            (40u)  ///< DMA Mux 1 Source ADC0
#define DMA_MUX1_SOURCE_ADC1            (41u)  ///< DMA Mux 1 Source ADC1
#define DMA_MUX1_SOURCE_ADC2            (42u)  ///< DMA Mux 1 Source ADC2
#define DMA_MUX1_SOURCE_ADC3            (43u)  ///< DMA Mux 1 Source ADC3
#define DMA_MUX1_SOURCE_DAC0            (45u)  ///< DMA Mux 1 Source DAC0
#define DMA_MUX1_SOURCE_DAC1            (46u)  ///< DMA Mux 1 Source DAC1
#define DMA_MUX1_SOURCE_CMP0            (47u)  ///< DMA Mux 1 Source CMP0
#define DMA_MUX1_SOURCE_CMP1            (48u)  ///< DMA Mux 1 Source CMP1
#define DMA_MUX1_SOURCE_CMP2            (49u)  ///< DMA Mux 1 Source CMP2
#define DMA_MUX1_SOURCE_CMP3            (50u)  ///< DMA Mux 1 Source CMP3
#define DMA_MUX1_SOURCE_PORTF           (53u)  ///< DMA Mux 1 Source PORTF
#define DMA_MUX1_SOURCE_DMA_MUX         (54u)  ///< DMA Mux 1 Source DMA Mux

#define DMA_CR_GRPXPRIO_HIGH            (1u)   ///< DMA_CR_GRPxPRI Priority level High
#define DMA_CR_GRPXPRIO_LOW             (0u)   ///< DMA_CR_GRPxPRI Priority level Low
#define DMAMUX_CHCFGX_DISABLE_MASK      (0u)   ///< DMAMUXx_CHCFGn Disable Mask
#define DMA_MAX_CH_NUMBER               (31u)  ///< Max DMA Channel Number

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
 * \brief   Enable the DMA channels on the Freescale Cortex-M4 processor.
 *
 * \details Transfers are enabled on the following channels:
 *              - DMA_MUX_0: Channels 0-4, 6-8, 9-11
 *              - DMA_MUX_1: Channels 0-2
 *              .
 *          All enabled channels support channel preemption; that is, a transfer
 *          request from a higher priority channel will preempt a lower channel
 *          transfer already in progress.
 * \n
 *          The Enable Mask is set for the following channels and associates them
 *          with their data sources as follows:
 *              - DMA_MUX_0 Channel 0          SPI0 TX
 *              - DMA_MUX_0 Channel 1          SPI0 RX
 * \n
 *              - DMA_MUX_0 Channel 2          UART0 RX
 *              - DMA_MUX_0 Channel 3          UART4 RX
 *              - DMA_MUX_0 Channel 4          UART5 RX
 * \n
 *              - DMA_MUX_0 Channel 6          PORT-A (ADC0)
 *              - DMA_MUX_0 Channel 7          PORT-B (ADC3)
 *              - DMA_MUX_0 Channel 8          PORT-E (ADC2)
 * \n
 *              - DMA_MUX_0 Channel 11         UART0 TX
 *              - DMA_MUX_0 Channel 10         UART4 TX
 *              - DMA_MUX_0 Channel 09         UART5 TX
 * \n
 *              - DMA_MUX_0 Channel 12         SPI2 TX
 *              - DMA_MUX_0 Channel 13         SPI2 RX
 * \n
 *              - DMA_MUX_1 Channel 0          ADC0
 *              - DMA_MUX_1 Channel 1          ADC3
 *              - DMA_MUX_1 Channel 2          ADC2
 *              .
 *          Note that DMA_MUX_1 channels 0-2 are overall DMA channels 16-18.
 * \n \n
 *          L2_DmaInit() also enables the DMA error interrupt for channels
 *          0-4 and 9-11.
 *
 * \todo    DMA error interrupt enables may need to be updated. 04/13/20 DAZ
 *
 * \return  None
 *
 * ========================================================================== */
void L2_DmaInit(void)
{
    SIM_SCGC6 |= SIM_SCGC6_DMAMUX0_MASK | SIM_SCGC6_DMAMUX1_MASK;   /* Enable clocks for DMA MUX 0 & 1 */
    SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;                                /* Enable DMA controller clock */

    DMA_CR = DMA_CR_GRP1PRI(DMA_CR_GRPXPRIO_LOW)  |                 /* Channels 16-31 are lowest priority */
             DMA_CR_GRP0PRI(DMA_CR_GRPXPRIO_HIGH) |                 /* Channels 0-15 are highest priority */
             DMA_CR_EMLM_MASK;                                      /* Enable minor loop mapping */

    /* Associate DMA channels with peripherals, and set their priorities: */

    /* SPI0 Tx/Rx DMA channels: */
    DMAMUX0_CHCFG0 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_SPI0_TX);
    DMA_DCHPRI0  = DMA_DCHPRI0_ECP_MASK  | DMA_DCHPRI0_CHPRI(DMA_CHANN_00_PRIORITY);

    DMAMUX0_CHCFG1 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_SPI0_RX);
    DMA_DCHPRI1  = DMA_DCHPRI1_ECP_MASK  | DMA_DCHPRI1_CHPRI(DMA_CHANN_01_PRIORITY);

    /* Uart Rx DMA channels: */
    DMAMUX0_CHCFG2 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_UART0_RX);
    DMA_DCHPRI2  = DMA_DCHPRI2_ECP_MASK  | DMA_DCHPRI2_CHPRI(DMA_CHANN_02_PRIORITY);

    DMAMUX0_CHCFG3 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_UART4_RX);
    DMA_DCHPRI3  = DMA_DCHPRI3_ECP_MASK  | DMA_DCHPRI4_CHPRI(DMA_CHANN_03_PRIORITY);

    DMAMUX0_CHCFG4 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_UART5_RX);
    DMA_DCHPRI4  = DMA_DCHPRI4_ECP_MASK  | DMA_DCHPRI4_CHPRI(DMA_CHANN_04_PRIORITY);

    /* Channel 5 is unused: */
    DMAMUX0_CHCFG5 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI5  = DMA_DCHPRI5_ECP_MASK  | DMA_DCHPRI5_CHPRI(DMA_CHANN_05_PRIORITY);

    /* Channels 6 - 8 are triggered by external FPGA signals to initiate ADC conversions.
    They are associated with bits on GPIO ports. */
    DMAMUX0_CHCFG6 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_PORTA);
    DMA_DCHPRI6  = DMA_DCHPRI6_ECP_MASK  | DMA_DCHPRI6_CHPRI(DMA_CHANN_06_PRIORITY);

    DMAMUX0_CHCFG7 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_PORTB);
    DMA_DCHPRI7  = DMA_DCHPRI7_ECP_MASK  | DMA_DCHPRI7_CHPRI(DMA_CHANN_07_PRIORITY);

    DMAMUX0_CHCFG8 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_PORTE);
    DMA_DCHPRI8  = DMA_DCHPRI8_ECP_MASK  | DMA_DCHPRI8_CHPRI(DMA_CHANN_08_PRIORITY);

    /* UART Tx DMA channels: */
    DMAMUX0_CHCFG9 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_UART5_TX);
    DMA_DCHPRI9  = DMA_DCHPRI9_ECP_MASK  | DMA_DCHPRI9_CHPRI(DMA_CHANN_09_PRIORITY);

    DMAMUX0_CHCFG10 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_UART4_TX);
    DMA_DCHPRI10 = DMA_DCHPRI10_ECP_MASK | DMA_DCHPRI10_CHPRI(DMA_CHANN_10_PRIORITY);

    DMAMUX0_CHCFG11 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_UART0_TX);
    DMA_DCHPRI11 = DMA_DCHPRI11_ECP_MASK | DMA_DCHPRI11_CHPRI(DMA_CHANN_11_PRIORITY);

    /* SPI2 Rx/Tx DMA channels: */
    DMAMUX0_CHCFG12 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_SPI2_TX);
    DMA_DCHPRI12  = DMA_DCHPRI12_ECP_MASK  | DMA_DCHPRI12_CHPRI(DMA_CHANN_12_PRIORITY);

    DMAMUX0_CHCFG13 = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(DMA_MUX0_SOURCE_SPI2_RX);
    DMA_DCHPRI13  = DMA_DCHPRI13_ECP_MASK  | DMA_DCHPRI13_CHPRI(DMA_CHANN_13_PRIORITY);

    /* Channels 14 - 15 unused: */
    DMAMUX0_CHCFG14 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI14 = DMA_DCHPRI14_ECP_MASK | DMA_DCHPRI14_CHPRI(DMA_CHANN_14_PRIORITY);

    DMAMUX0_CHCFG15 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI15 = DMA_DCHPRI15_ECP_MASK | DMA_DCHPRI15_CHPRI(DMA_CHANN_15_PRIORITY);

    /* Channels 16 - 31 disabled: */
    DMAMUX1_CHCFG0 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI16 = DMA_DCHPRI16_ECP_MASK | DMA_DCHPRI16_CHPRI(DMA_CHANN_16_PRIORITY);

    DMAMUX1_CHCFG1 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI17 = DMA_DCHPRI17_ECP_MASK | DMA_DCHPRI17_CHPRI(DMA_CHANN_17_PRIORITY);

    DMAMUX1_CHCFG2 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI18 = DMA_DCHPRI18_ECP_MASK | DMA_DCHPRI18_CHPRI(DMA_CHANN_18_PRIORITY);

    DMAMUX1_CHCFG3 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI19 = DMA_DCHPRI19_ECP_MASK | DMA_DCHPRI19_CHPRI(DMA_CHANN_19_PRIORITY);

    DMAMUX1_CHCFG4 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI20 = DMA_DCHPRI20_ECP_MASK | DMA_DCHPRI20_CHPRI(DMA_CHANN_20_PRIORITY);

    DMAMUX1_CHCFG5 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI21 = DMA_DCHPRI21_ECP_MASK | DMA_DCHPRI21_CHPRI(DMA_CHANN_21_PRIORITY);

    DMAMUX1_CHCFG6 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI22 = DMA_DCHPRI22_ECP_MASK | DMA_DCHPRI22_CHPRI(DMA_CHANN_22_PRIORITY);

    DMAMUX1_CHCFG7 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI23 = DMA_DCHPRI23_ECP_MASK | DMA_DCHPRI23_CHPRI(DMA_CHANN_23_PRIORITY);

    DMAMUX1_CHCFG8 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI24 = DMA_DCHPRI24_ECP_MASK | DMA_DCHPRI24_CHPRI(DMA_CHANN_24_PRIORITY);

    DMAMUX1_CHCFG9 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI25 = DMA_DCHPRI25_ECP_MASK | DMA_DCHPRI25_CHPRI(DMA_CHANN_25_PRIORITY);

    DMAMUX1_CHCFG10 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI26 = DMA_DCHPRI26_ECP_MASK | DMA_DCHPRI26_CHPRI(DMA_CHANN_26_PRIORITY);

    DMAMUX1_CHCFG11 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI27 = DMA_DCHPRI27_ECP_MASK | DMA_DCHPRI27_CHPRI(DMA_CHANN_27_PRIORITY);

    DMAMUX1_CHCFG12 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI28 = DMA_DCHPRI28_ECP_MASK | DMA_DCHPRI28_CHPRI(DMA_CHANN_28_PRIORITY);

    DMAMUX1_CHCFG13 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI29 = DMA_DCHPRI29_ECP_MASK | DMA_DCHPRI29_CHPRI(DMA_CHANN_29_PRIORITY);

    DMAMUX1_CHCFG14 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI30 = DMA_DCHPRI30_ECP_MASK | DMA_DCHPRI30_CHPRI(DMA_CHANN_30_PRIORITY);

    DMAMUX1_CHCFG15 = DMAMUX_CHCFGX_DISABLE_MASK;
    DMA_DCHPRI31 = DMA_DCHPRI31_ECP_MASK | DMA_DCHPRI31_CHPRI(DMA_CHANN_31_PRIORITY);

    /* Enable DMA error interrupt for channels 0-4 and 9-11 */
    DMA_EEI |= DMA_CHANNEL_MASK_0 |
               DMA_CHANNEL_MASK_1 |
               DMA_CHANNEL_MASK_2 |
               DMA_CHANNEL_MASK_3 |
               DMA_CHANNEL_MASK_4 |
               DMA_CHANNEL_MASK_9 |  /// \todo Are error interrupts necessary for UART? (2-4, 9-11)
               DMA_CHANNEL_MASK_10 |
               DMA_CHANNEL_MASK_11;

    /* Enable error interrupt */
    Enable_IRQ(DMA_ERROR_IRQ);
}

/* ========================================================================== */
/**
 * \brief   DMA error ISR handler
 *
 * \details This routine gets called when there is a DMA error. An error message
 *          indicating the channel of the error will be printed out, all DMA errors
 *          will be cleared, and the handler exits.
 *
 * \return  None
 *
 * ========================================================================== */
void L2_DmaError_ISR(void)
{
    OS_CPU_SR   cpu_sr;     /* Status register temporary */
    uint32_t    chMask;     /* Channel mask */
    uint16_t    chNum;      /* Channel number */

    OS_ENTER_CRITICAL();
    OSIntEnter();
    OS_EXIT_CRITICAL();

    chMask = 1;

    for (chNum = 0; chNum <= DMA_MAX_CH_NUMBER; chNum++)
    {
        /// \todo 10/22/2020 GK Enable below code when Logger Module is available.
        //        if (DMA_ERR & chMask)
        //        {
        //            Log(ERR, "DMA Error on channel %d", chNum);
        //        }

        chMask = chMask << 1;
    }

    /* Clear all DMA errors */
    DMA_CERR = DMA_CERR_CAEI_MASK;

    OSIntExit();
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
