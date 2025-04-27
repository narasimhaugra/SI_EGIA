#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Adc
 * \{
 * \brief   Analog to Digital Converter driver
 *
 * \details This module handles all processing related to the Analog to Digital
 *          converter (ADC), which resides on the MK20 CPU chip. Functions are
 *          provided to initialize the ADC, start a new conversion, and return
 *          raw or calibrated ADC samples.
 *
 * \sa      Chapter 35 of K20 SubFamily Reference Manual (provide link to K20P144M102SF3RM.pdf)
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_Adc.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Common.h"
#include "L2_Adc.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
// ADC init defines:
#define ADC_CAL_TIME                (20U)           ///< Max cal time, mS
#define ADC_MS_BIT_MASK             (0x8000U)       ///< MS bit mask for uint16
#define ADC_FAST_CLOCK_SETUP        (0U)            ///< Fast ADC clock setup
#define ADC_SLOW_CLOCK_SETUP        (1U)            ///< Slow ADC clock setup

// ADC channel definitions:
#define ADC0_SE16   (0x0010)    ///< ADC0 channel SE16
#define ADC0_SE22   (0x0016)    ///< ADC0 channel SE22
#define ADC0_SE23   (0x0017)    ///< ADC0 channel SE23
#define ADC0_VREFH  (0x001D)    ///< ADC0 reference voltage high channel
#define ADC0_VREFL  (0x001E)    ///< ADC0 reference voltage low channel

#define ADC1_SE13   (0x000D)    ///< ADC1 channel SE13
#define ADC1_SE23   (0x0017)    ///< ADC1 channel SE23
#define ADC1_DP1    (0x0001)    ///< ADC1 channel DP1

#define ADC2_SE12   (0x000C)    ///< ADC2 channel SE12
#define ADC2_SE13   (0X000D)    ///< ADC2 channel SE13
#define ADC2_SE14   (0x000E)    ///< ADC2 channel SE14
#define ADC2_VREFH  (0x001D)    ///< ADC2 reference voltage high channel
#define ADC2_VREFL  (0x001E)    ///< ADC2 reference voltage low channel

#define ADC3_SE4A   (0x0004)    ///< ADC3 channel SE4A
#define ADC3_SE5A   (0x0005)    ///< ADC3 channel SE5A
#define ADC3_SE6A   (0x0006)    ///< ADC3 channel SE6A
#define ADC3_VREFH  (0x001D)    ///< ADC3 reference voltage high channel
#define ADC3_VREFL  (0x001E)    ///< ADC3 reference voltage low channel

#define ADC_DISABLE (0x001F)    ///< ADC input disabled

// ADC DMA defines
#define DMA_CHAN6   (6)         ///< DMA trigger channel #6
#define DMA_CHAN7   (7)         ///< DMA trigger channel #7
#define DMA_CHAN8   (8)         ///< DMA trigger channel #8

#define ADC_DMA_DSIZE_32    (2) ///< DMA Data Size - 32 bits
#define ADC_DMA_MBTC        (4) ///< DMA Minor Byte Transfer Count

#define A                 (0x0)                 ///< SC register A (Status/control)
#define B                 (0x1)                 ///< SC register B (Status/control)

// NOTE: the following defines relate to the ADC register definitions
// and the content follows the reference manual, using the same symbols.

/// ADCSC1 (register)

// Conversion Complete (COCO) mask
#define COCO_COMPLETE     ADC_SC1_COCO_MASK     ///< Conversion complete
#define COCO_NOT          (0x00)                ///< Conversion not complete

// ADC interrupts: enabled, or disabled.
#define AIEN_ON           ADC_SC1_AIEN_MASK     ///< Enable ADC interrupt
#define AIEN_OFF          (0x00)                ///< Disable ADC interrupt

// Differential or Single ended ADC input
#define DIFF_SINGLE       (0x00)                ///< Single ended input
#define DIFF_DIFFERENTIAL ADC_SC1_DIFF_MASK     ///< Differential input

/// ADCCFG1

// Power setting of ADC
// Power setting of ADC
#define ADLPC_LOW         ADC_CFG1_ADLPC_MASK   ///< Low power
#define ADLPC_NORMAL      (0x00)                ///< Normal power

// Clock divisor
#define ADIV_1            (0x00)                ///< Divide by 1
#define ADIV_2            (0x01)                ///< Divide by 2
#define ADIV_4            (0x02)                ///< Divide by 4
#define ADIV_8            (0x03)                ///< Divide by 8

// Long sample time, or Short sample time
#define ADLSMP_LONG       ADC_CFG1_ADLSMP_MASK  ///< Long sample time
#define ADLSMP_SHORT      (0x00)                ///< Short sample time

// How many bits for the conversion?  8, 12, 10, or 16 (single ended).
#define MODE_8            (0x00)                ///< 8 bit conversion
#define MODE_12           (0x01)                ///< 12 bit conversion
#define MODE_10           (0x02)                ///< 10 bit conversion
#define MODE_16           (0x03)                ///< 16 bit conversion

// ADC Input Clock (ADCK) Source choice? Bus clock, Bus clock/2, "altclk", or the
//                                ADC's own asynchronous clock for less noise
#define ADICLK_BUS        (0x00)                ///< Bus clock for conversion
#define ADICLK_BUS_2      (0x01)                ///< Bus clock / 2 for conversion
#define ADICLK_ALTCLK     (0x02)                ///< Alternate clock (ALTCLK) for conversion
#define ADICLK_ADACK      (0x03)                ///< Asynchronous clock (ADACK) for conversion

//// ADCCFG2

// Select between B or A channels
#define MUXSEL_ADCB       ADC_CFG2_MUXSEL_MASK  ///< Select Multiplexer "B" channels
#define MUXSEL_ADCA       (0x00)                ///< Select Multiplexer "A" channels

// Ansync clock output enable: enable, or disable the output of it
#define ADACKEN_ENABLED   ADC_CFG2_ADACKEN_MASK ///< Asynchronous ADC clock enabled
#define ADACKEN_DISABLED  (0x00)                ///< Asynchronous ADC clock disabled

// High speed or low speed conversion mode
#define ADHSC_HISPEED     ADC_CFG2_ADHSC_MASK   ///< High speed conversion mode
#define ADHSC_NORMAL      (0x00)                ///< Normal speed conversion mode

// Long Sample Time selector: 20, 12, 6, or 2 extra clocks for a longer sample time
#define ADLSTS_20          (0x00)               ///< Default longest sample time - 20 extra ADCK cycles (24 total)
#define ADLSTS_12          (0x01)               ///< 12 extra ADCK cycles
#define ADLSTS_6           (0x02)               ///< 6 extra ADCK cycles
#define ADLSTS_2           (0x03)               ///< 2 extra ADCK cycles

////ADCSC2

// Read-only status bit indicating conversion status
#define ADACT_ACTIVE       ADC_SC2_ADACT_MASK   ///< Conversion in progess
#define ADACT_INACTIVE     (0x00)               ///< Conversion not in progress

// Trigger for starting conversion: Hardware trigger, or software trigger.
// For using PDB, the Hardware trigger option is selected.
#define ADTRG_HW           ADC_SC2_ADTRG_MASK   ///< Hardware trigger
#define ADTRG_SW           (0x00)               ///< Software trigger

// ADC Compare Function Enable: Enabled or Disabled.
#define ACFE_ENABLED       ADC_SC2_ACFE_MASK    ///< ADC compare function enabled
#define ACFE_DISABLED      (0x00)               ///< ADC compare function disabled

// Compare Function Greater Than Enable: Greater, or Less.
#define ACFGT_GREATER      ADC_SC2_ACFGT_MASK   ///< Compare function greater than or equal
#define ACFGT_LESS         (0x00)               ///< Compare function less than

// Compare Function Range Enable: Enabled or Disabled.
#define ACREN_ENABLED      ADC_SC2_ACREN_MASK   ///< Compare range enabled
#define ACREN_DISABLED     (0x00)               ///< Compare range disabled

// DMA enable: enabled or disabled.
#define DMAEN_ENABLED      ADC_SC2_DMAEN_MASK   ///< DMA enabled
#define DMAEN_DISABLED     (0x00)               ///< DMA disabled

// Voltage Reference selection for the ADC conversions
// (***not*** the PGA which uses VREFO only).
// VREFH and VREFL (0) , or VREFO (1).

#define REFSEL_EXT         (0x00)               ///< External - Vrefh, Vrefl pins.
#define REFSEL_ALT         (0x01)               ///< Alternate reference pair: Valth, Valtl - may be external or internal.
#define REFSEL_RES         (0x02)               ///< Reserved
#define REFSEL_RES_EXT     (0x03)               ///< Reserved but defaults to Vref

////ADCSC3

// Calibration begin or off
#define CAL_BEGIN          ADC_SC3_CAL_MASK     ///< Begin calibration sequence
#define CAL_OFF            (0x00)               ///< Calibration not started

// Status indicating Calibration failed, or normal success
#define CALF_FAIL          ADC_SC3_CALF_MASK    ///< Calibration sequence failed
#define CALF_NORMAL        (0x00)               ///< Calibration sequence OK

// ADC to continously convert, or do a sinle conversion
#define ADCO_CONTINUOUS    ADC_SC3_ADCO_MASK    ///< Continuous conversion
#define ADCO_SINGLE        (0x00)               ///< Single conversion

// Averaging enabled in the ADC, or not.
#define AVGE_ENABLED       ADC_SC3_AVGE_MASK    ///< ADC averaging enabled
#define AVGE_DISABLED      (0x00)               ///< ADC averaging disabled

// Number of samples to create the ADC average result
#define AVGS_4             (0x00)               ///< 4 samples averaged
#define AVGS_8             (0x01)               ///< 8 samples averaged
#define AVGS_16            (0x02)               ///< 16 samples averaged
#define AVGS_32            (0x03)               ///< 32 samples averaged

////PGA

// PGA enabled or not?
#define PGAEN_ENABLED      ADC_PGA_PGAEN_MASK   ///< PGA Enabled
#define PGAEN_DISABLED     (0x00)               ///< PGA Disabled

// Chopper stabilization of the amplifier, or not.
#define PGACHP_CHOP        ADC_PGA_PGACHP_MASK  ///< Chopper stablilzation enabled
#define PGACHP_NOCHOP      (0x00)               ///< Chopper stabilization disabled

// PGA in low power mode, or normal mode.
#define PGALP_LOW          ADC_PGA_PGALP_MASK   ///< PGA in low power mode
#define PGALP_NORMAL       (0x00)               ///< PGA in normal power mode

// Gain of PGA.  Selectable from 1 to 64.
#define PGAG_1             (0x00)               ///< PGA gain 1
#define PGAG_2             (0x01)               ///< PGA gain 2
#define PGAG_4             (0x02)               ///< PGA gain 4
#define PGAG_8             (0x03)               ///< PGA gain 8
#define PGAG_16            (0x04)               ///< PGA gain 16
#define PGAG_32            (0x05)               ///< PGA gain 32
#define PGAG_64            (0x06)               ///< PGA gain 64

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef struct
{
  ADC_MemMapPtr pBasePtr;       // Ptr to base of ADC Memory Map

  uint8_t  DmaTrigChan;         // DMA Trigger channel
  volatile uint32_t *pPcrReg;   // Ptr to PCR Reg
  uint32_t TrigSource;          // Trigger Source
  uint16_t SeChannel;           // Channel

  uint8_t  DmaTransferChan;     // DMA Transfer channel
  uint32_t TransSource;         // Transfer Source
  uint32_t TransDest;           // Transfer Destination

  uint16_t Offset;             // Offset
  uint16_t Result;             // Result

} ADC_STRUCT;

typedef struct
{
    ADC_MemMapPtr               BaseAdr;                // ADC base address
    volatile uint32_t *const    pSimReg;                // Pointer to ADC clock control register
    volatile uint32_t *const    pPcrReg;                // Pointer to PCR register
    uint32_t                    SimMask;                // ADC clock control mask
    uint8_t                     DmaTrigChan;            // DMA trigger channel
    uint8_t                     AdcClockSetup;          // ADC clock setup (slow/fast)
    uint16_t                    AdcMux[ADC_CH_COUNT];   // ADC channel muxes
} ADC_SETUP;

/* Original configuration arrays for reference:
// Configuration arrays:
    static const ADC_MemMapPtr        baseLookUp[] = { ADC0_BASE_PTR,         ADC1_BASE_PTR,       ADC2_BASE_PTR,       ADC3_BASE_PTR };        // ADC base address
    static volatile uint32_t *const       simReg[] = { &SIM_SCGC6,            &SIM_SCGC3,          &SIM_SCGC6,          &SIM_SCGC3 };           // ADC clock control reg
    static volatile uint32_t *const pcrRegLookUp[] = { &PORTA_PCR5,           NULL,                &PORTE_PCR7,         &PORTB_PCR1 };          // Trigger Port / bit control register (Unused for ADC1)
    static const uint32_t                SimMask[] = { SIM_SCGC6_ADC0_MASK,   SIM_SCGC3_ADC1_MASK, SIM_SCGC6_ADC2_MASK, SIM_SCGC3_ADC3_MASK };  // ADC clock control mask
    static const uint8_t       DmaTrigChanLookUp[] = { 6,                     0xFF,                8,                   7 };                    // ADC DMA trigger channel (Unused for ADC1)
    static const uint8_t           AdcClockSetup[] = { ADC_FAST_CLOCK_SETUP,  ADC_SLOW_CLOCK_SETUP, ADC_FAST_CLOCK_SETUP, ADC_FAST_CLOCK_SETUP };   // ADC clock setup

    static const uint16_t AdcMuxLookUp[ADC_COUNT][ADC_CH_COUNT] =    // Channel # to multiplexer channel translation (SC1 register value)
    {
        { ADC0_SE16, ADC0_SE22, ADC0_SE23 },    // Channel translations for ADC0
        { ADC1_SE13, ADC1_SE23, ADC1_DP1  },    // Channel translations for ADC1
        { ADC2_SE12, ADC2_SE13, ADC2_SE14 },    // Channel translations for ADC2
        { ADC3_SE4A, ADC3_SE5A, ADC3_SE6A }     // Channel translations for ADC3
    }
*/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
static const ADC_SETUP adcSetup[ADC_COUNT] =
{
    {
        ADC0_BASE_PTR,                          // ADC base address
        &SIM_SCGC6,                             // Pointer to ADC clock control register
        &PORTA_PCR5,                            // Pointer to PCR register
        SIM_SCGC6_ADC0_MASK,                    // ADC clock control mask
        DMA_CHAN6,                              // DMA trigger channel
        ADC_FAST_CLOCK_SETUP,                   // ADC clock setup (slow/fast)
        { ADC0_SE16, ADC0_SE22, ADC0_SE23 },    // Channel translations for ADC0 (ADC channel muxes)
    },

    {
        ADC1_BASE_PTR,
        &SIM_SCGC3,
        NULL,
        SIM_SCGC3_ADC1_MASK,
        0xFF,
        ADC_SLOW_CLOCK_SETUP,
        { ADC1_SE13, ADC1_SE23, ADC1_DP1  },    // Channel translations for ADC1
    },

    {
        ADC2_BASE_PTR,
        &SIM_SCGC6,
        &PORTE_PCR7,
        SIM_SCGC6_ADC2_MASK,
        DMA_CHAN8,
        ADC_FAST_CLOCK_SETUP,
        { ADC2_SE12, ADC2_SE13, ADC2_SE14 },    // Channel translations for ADC2
    },

    {
        ADC3_BASE_PTR,
        &SIM_SCGC3,
        &PORTB_PCR1,
        SIM_SCGC3_ADC3_MASK,
        DMA_CHAN7,
        ADC_FAST_CLOCK_SETUP,
        { ADC3_SE4A, ADC3_SE5A, ADC3_SE6A }     // Channel translations for ADC3
    }
};

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/// \todo 07/15/2022 DAZ - This does NOT need to be in ramdyndata. NO data in this structure is accessed by DMA. Should be in internal RAM.
//#pragma location=".sram"
#pragma location=".ramdyndata"
static ADC_STRUCT AdcCfg[ADC_COUNT];    // ADC configurations for all ADC instances

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static bool ADC_Cal(ADC_MemMapPtr AdcMap);  // ADC auto calibration

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Self calibrate the specified ADC module
 *
 * \details This function must be run in order to meet specifications after reset
 *          and before a conversion is initiated.
 *
 * \param   AdcMap - Base address of the ADC module to calibrate
 *
 * \sa      Chapter 35 of K20P144M102SF3RM K20 Sub-Family Reference Manual
 *
 * \return  bool - true if error
 *
 * ========================================================================== */
static bool ADC_Cal(ADC_MemMapPtr AdcMap)
{
    bool     ErrorStatus;   // Function error status
    uint32_t CalTime;       // Calibration time limit
    uint16_t CalVar;        // Temporary for auto calibration

    ErrorStatus = false;                                                // Default to no error
    ADC_SC2_REG(AdcMap) &=  ~ADC_SC2_ADTRG_MASK;                        // Enable Software Conversion Trigger for Calibration Process
    ADC_SC3_REG(AdcMap) &= (~ADC_SC3_ADCO_MASK & ~ADC_SC3_AVGS_MASK);   // Set single conversion, clear avgs bitfield for next writing
    ADC_SC3_REG(AdcMap) |= (ADC_SC3_AVGE_MASK | ADC_SC3_AVGS(AVGS_32)); // Turn averaging ON and set at max value ( 32 )

    do
    {
        ADC_SC3_REG(AdcMap) |= ADC_SC3_CAL_MASK;                            // Start CAL

        CalTime = OSTimeGet() + ADC_CAL_TIME;
        while ((ADC_SC1_REG(AdcMap, A) & ADC_SC1_COCO_MASK) == COCO_NOT)    // Wait calibration end
        {
            if (OSTimeGet() > CalTime)
            {
                ErrorStatus = true;         // Timeout - break out of wait loop
                break;
            }
        }

        if (ErrorStatus || ((ADC_SC3_REG(AdcMap) & ADC_SC3_CALF_MASK) == CALF_FAIL))
        {
            ErrorStatus = true;             // Calibration failed or timeout - exit bad
            break;
        }

        // Calculate plus-side calibration

        CalVar =  ADC_CLP0_REG(AdcMap);
        CalVar += ADC_CLP1_REG(AdcMap);
        CalVar += ADC_CLP2_REG(AdcMap);
        CalVar += ADC_CLP3_REG(AdcMap);
        CalVar += ADC_CLP4_REG(AdcMap);
        CalVar += ADC_CLPS_REG(AdcMap);

        CalVar = CalVar >> 1;
        CalVar |= ADC_MS_BIT_MASK;          // Set MS bit

        ADC_PG_REG(AdcMap) = ADC_PG_PG(CalVar);

        // Calculate minus-side calibration
        CalVar =  ADC_CLM0_REG(AdcMap);
        CalVar += ADC_CLM1_REG(AdcMap);
        CalVar += ADC_CLM2_REG(AdcMap);
        CalVar += ADC_CLM3_REG(AdcMap);
        CalVar += ADC_CLM4_REG(AdcMap);
        CalVar += ADC_CLMS_REG(AdcMap);

        CalVar = CalVar >> 1;
        CalVar |= ADC_MS_BIT_MASK;          // Set MS bit

        ADC_MG_REG(AdcMap) = ADC_MG_MG(CalVar);

    } while (false);

    ADC_SC3_REG(AdcMap) &= ~ADC_SC3_CAL_MASK; // Clear CAL bit even if error

    return (ErrorStatus);
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Initialize all ADC modules to their default configurations.
 *
 * \details Calls L2_AdcConfig() to initialize all ADCs to their default
 *          configurations, preparing them for use. The caller must set the
 *          Offset, if any.
 *
 * \note    A/D Offsets must be calibrated by L3.
 *
 * \param   < None >
 * \return  ADC_STATUS - Status of operation
 * \retval  ADC_STATUS_OK - Success
 * \retval  ADC_PARAMETER_ERROR - Bad argument to AdcConfig
 * \retval  ADC_CAL_FAIL - AdcConfig failed auto cal
 *
 * ========================================================================== */
ADC_STATUS L2_AdcInit(void)
{
    ADC_STATUS  Status;                 // Function return status

    do
    {
        //setup VREF out for temperature and HW version
        VREF_SC = (VREF_SC_VREFEN_MASK | VREF_SC_REGEN_MASK | VREF_SC_MODE_LV(1)); //Turn Vref on

        Status = L2_AdcConfig(ADC0, ADC_CH0);   // Initialize motor ADC's to use average current input
        if (Status != ADC_STATUS_OK)    { break; }

        Status = L2_AdcConfig(ADC2, ADC_CH0);
        if (Status != ADC_STATUS_OK)    { break; }

        Status = L2_AdcConfig(ADC3, ADC_CH0);
        if (Status != ADC_STATUS_OK)    { break; }

        Status = L2_AdcConfig(ADC1, ADC_CH2);   // Initialize aux ADC to use board version input

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Initialize an ADC instance
 *
 * \details Initializes ADC instance and appropriate DMA channels. The internal ADC
 *          configuration structure is initialized. This structure is used by other
 *          functions that operate the ADC. ADC1 does not use DMA.
 *
 * \note    This function supports 4 channels of ADC:
 *              - ADC0 can be connected to motor 0 average current, peak current, or temperature.
 *              - ADC3 can be connected to motor 1 average current, peak current, or temperature.
 *              - ADC2 can be connected to motor 2 average current, peak current, or temperature.
 *              - ADC1 can be connected to thermistor, battery voltage, or hardware version voltage.
 *
 *          Peak currents and motor temps are not used. On ADC1, only the hardware version voltage is currently used. (04/10/20)
 *
 * \param   Num     - ADC instance number
 * \param   Chan    - DMA channel number
 *
 * \return  ADC_STATUS - Status of operation
 * \retval  ADC_STATUS_OK - Success
 * \retval  ADC_STATUS_INVALID_PARAMETER - Invalid pointer, ADC number or channel
 *
 * ========================================================================== */
ADC_STATUS L2_AdcConfig(ADC_INSTANCE Num, ADC_CHANNEL Chan)
{
    ADC_STATUS Status;          // Function return status
    ADC_STRUCT *pCfg;           // Pointer to ADC configuration
    const ADC_SETUP  *pSetup;   // Pointer to ADC setup information

    Status = ADC_STATUS_OK;     // Default to no error

    do
    {
        // Validate parameters
        if ((ADC_COUNT <= Num) || (ADC_CH_COUNT <= Chan))
        {
            Status = ADC_STATUS_INVALID_PARAMETER;
            break;
        }

        pCfg = &AdcCfg[Num];                                    // Point to configuration block for selected ADC
        pSetup = &adcSetup[Num];                                // Point to setup information for selected ADC

        // Initialize ADC info structure:

        pCfg->pBasePtr = pSetup->BaseAdr;                       // Set ADC base pointer
        pCfg->DmaTrigChan = pSetup->DmaTrigChan;                // Set DMA channel for ADC trigger
        pCfg->pPcrReg = pSetup->pPcrReg;                        // Set control register pointer for trigger input
        pCfg->SeChannel = pSetup->AdcMux[Chan];                 // Set ADC multiplexer channel
        pCfg->Offset = 0;                                       // Set initial user defined Offset to 0

        // Enable clock gate control
        *pSetup->pSimReg |= pSetup->SimMask;                    // Enable ADC system clock

        // Setup the initial ADC default configuration
        if (pSetup->AdcClockSetup == ADC_SLOW_CLOCK_SETUP)
        {
            ADC_CFG1_REG(pCfg->pBasePtr) = ADC_CFG1_ADIV(ADIV_8) |          // Clock divide (by 8)
                                           ADC_CFG1_ADLSMP_MASK |           // Sample Time (Long)
                                           ADC_CFG1_MODE(MODE_16) |         // Conversion (16-bit)
                                           ADC_CFG1_ADICLK(ADICLK_BUS_2);   // Input Clock (Bus clock/2)

            ADC_CFG2_REG(pCfg->pBasePtr) = MUXSEL_ADCA |                    // ADC Mux Select ('a' channels)
                                           ADACKEN_DISABLED |               // Asynchronous clock enabled
                                           ADC_CFG2_ADLSTS(ADLSTS_20);      // Long sample time Select (20 extra ADCK cycles)

            ADC_SC2_REG(pCfg->pBasePtr) =  ADC_SC2_REFSEL(REFSEL_EXT);      // Vref(h/l) as reference

            ADC_SC3_REG(pCfg->pBasePtr) =  (ADC_SC3_AVGE_MASK |             // Hardware Averaging (Enabled)
                                            ADC_SC3_AVGS(AVGS_32));         // Samples Averaged (32)
        }
        else //default is ADC_FAST_CLOCK_SETUP, original setup
        {
            ADC_CFG1_REG(pCfg->pBasePtr) = ADC_CFG1_ADIV(ADIV_2) |          // Clock divide (by 2)
                                           ADC_CFG1_ADLSMP_MASK |           // Sample Time (Long)
                                           ADC_CFG1_MODE(MODE_16) |         // Conversion (16-bit)
                                           ADC_CFG1_ADICLK(ADICLK_BUS_2);   // Input Clock (Bus clock/2)

            ADC_CFG2_REG(pCfg->pBasePtr) = MUXSEL_ADCA |                    // ADC Mux Select ('a' channels)
                                           ADACKEN_DISABLED |               // Asynchronous clock enabled
                                           ADC_CFG2_ADLSTS(ADLSTS_2);       // Long sample time Select (2 extra ADCK cycles)

            ADC_SC2_REG(pCfg->pBasePtr) =  ADC_SC2_REFSEL(REFSEL_EXT);      // Vref(h/l) as reference

            ADC_SC3_REG(pCfg->pBasePtr) =  (ADC_SC3_AVGE_MASK |             // Hardware Averaging (Enabled)
                                            ADC_SC3_AVGS(AVGS_32));         // Samples Averaged (32)
        }


        ADC_SC1_REG(pCfg->pBasePtr, 0) = ADC_SC1_ADCH(ADC_DISABLE);         // Module disabled
        ADC_SC1_REG(pCfg->pBasePtr, 1) = ADC_SC1_ADCH(ADC_DISABLE);

        // Calibrate the ADC in the configuration in which it will be used:
        if (ADC_Cal(pCfg->pBasePtr)) {                                      // Do the calibration
            Status = ADC_STATUS_CAL_FAIL;                                   // Only Cal has failed - finish the rest of setup & exit bad
        }

        ADC_SC3_REG(pCfg->pBasePtr) =  (ADC_SC3_AVGE_MASK |                 // Hardware Averaging (Enabled)
                                        ADC_SC3_AVGS(AVGS_32));             // Samples Averaged (32)

#ifdef UNUSED_CODE       // { 04/10/20 DAZ -    For reference only
        //=========================================================================
        // Determine Offset voltage
        //=========================================================================

        ADC_SC1_REG(pCfg->pBasePtr, 0) = pCfg->SeChannel;                       // Set ADC input multiplexer & start conversion
        while ((ADC_SC1_REG(pCfg->pBasePtr, 0) & ADC_SC1_COCO_MASK) == FALSE)   // Wait for conversion complete
        {};
        pCfg->Offset = ADC_R_REG(pCfg->pBasePtr, 0);
#endif                   // }

        //=========================================================================
        // Set up trigger DMA channel for all channels except ADC1
        //=========================================================================

        if (ADC1 != Num)
        {
            // source & destination are 32-bit
            DMA_ATTR_REG(DMA_BASE_PTR, pCfg->DmaTrigChan) = DMA_ATTR_SSIZE(ADC_DMA_DSIZE_32) | DMA_ATTR_DSIZE(ADC_DMA_DSIZE_32);

            pCfg->TrigSource = pCfg->SeChannel;
            DMA_SADDR_REG(DMA_BASE_PTR, pCfg->DmaTrigChan) = (uint32_t)&pCfg->TrigSource;               // Source address for transfer
            DMA_DADDR_REG(DMA_BASE_PTR, pCfg->DmaTrigChan) = (uint32_t)&ADC_SC1_REG(pCfg->pBasePtr, 0); // Destination address for transfer

            DMA_SOFF_REG(DMA_BASE_PTR, pCfg->DmaTrigChan) = 0;
            DMA_DOFF_REG(DMA_BASE_PTR, pCfg->DmaTrigChan) = 0;
            DMA_SLAST_REG(DMA_BASE_PTR, pCfg->DmaTrigChan) = 0;
            DMA_DLAST_SGA_REG(DMA_BASE_PTR, pCfg->DmaTrigChan) = 0;

            DMA_NBYTES_MLOFFNO_REG(DMA_BASE_PTR, pCfg->DmaTrigChan) = DMA_NBYTES_MLOFFNO_NBYTES(ADC_DMA_MBTC);  // Number of bytes to transfer / channel service request

            DMA_CITER_ELINKNO_REG(DMA_BASE_PTR, pCfg->DmaTrigChan) = DMA_CITER_ELINKNO_CITER(1);                // Number of service requests to make
            DMA_BITER_ELINKNO_REG(DMA_BASE_PTR, pCfg->DmaTrigChan) = DMA_BITER_ELINKNO_BITER(1);
        }

    } while (false);

    return (Status);
}

/* ========================================================================== */
/**
 * \brief   Set Offset for ADC reading correction
 *
 * \details The Offset field in the ADC configuration structure is set. The
 *          Offset register in the ADC itself must not be changed as it is
 *          set by L2_ADC_Cal(). This Offset is in addition to the factory
 *          calibration.
 *
 * \param   Num - ADC to set Offset for.
 * \param   Ofst - Offset value (unsigned)
 *
 * \return  ADC_STATUS - Status of operation
 * \retval  ADC_STATUS_OK - Success
 * \retval  ADC_STATUS_INVALID_PARAMETER - ADC number out of range
 *
 * ========================================================================== */
ADC_STATUS L2_AdcSetOffset(ADC_INSTANCE Num, uint16_t Ofst)
{
    ADC_STATUS Status;      // Function return status
    ADC_STRUCT *pCfg;       // Pointer to ADC configuration

    Status = ADC_STATUS_OK; // Default to no error

    do
    {
        // Validate parameters
        if (ADC_COUNT <= Num)
        {
            Status = ADC_STATUS_INVALID_PARAMETER;
            break;
        }

        pCfg = &AdcCfg[Num];                        // Point to configuration block for selected ADC
        pCfg->Offset = Ofst;                        // Save in configuration block
    } while (false);

    return(Status);
}

/* ========================================================================== */
/**
 * \brief   Retrieve ADC status
 *
 * \details Checks conversion complete bit to see if new data is available.
 *
 * \param   Num - ADC to get status for
 *
 * \return  ADC_STATUS - Status of ADC
 * \retval  ADC_STATUS_INVALID_PARAMETER - ADC number out of range
 * \retval  ADC_STATUS_BUSY - Conversion in progress
 * \retval  ADC_STATUS_DATA_OLD - Data not current. (Read from structure)
 * \retval  ADC_STATUS_DATA_NEW - New data available. (Conversion completed since
 *                                                     last call to function)
 *
 * ========================================================================== */
ADC_STATUS L2_AdcGetStatus(ADC_INSTANCE Num)
{
    ADC_STATUS Status;      // Function return status
    ADC_STRUCT *pCfg;       // Pointer to ADC configuration

    Status = ADC_STATUS_DATA_OLD; // Default to no new data available

    do
    {
        // Validate parameters
        if (ADC_COUNT <= Num)
        {
            Status = ADC_STATUS_INVALID_PARAMETER;
            break;
        }

        pCfg = &AdcCfg[Num];                        // Point to configuration block for selected ADC

        if (ADC_SC2_REG(pCfg->pBasePtr) & ADC_SC2_ADACT_MASK)   // See if conversion in progress
        {
            Status = ADC_STATUS_BUSY;
        }

        if (ADC_SC1_REG(pCfg->pBasePtr, 0) & ADC_SC1_COCO_MASK)
        {
            Status = ADC_STATUS_DATA_NEW;                       // Conversion complete - Indicate new data available
        }
    } while (false);

    return(Status);
}

/* ========================================================================== */
/**
 * \brief   Initiate an ADC conversion for the specified ADC instance
 *
 * \param   Num - ADC module to start conversion for
 * \param   HwTrig - True to arm hardware triggering, false for software trigger.
 *
 * \note    ADC1 is software triggered, regardless of hwTrig.
 *          \n
 *          When HwTrig is true (except for ADC1), calling this function arms the
 *          DMA to start the conversion on the next motor PWM falling edge. This
 *          signal is supplied from the FPGA for the corresponding motor.
 *          \n
 *          When HwTrig is false, the call to AdcStart starts the conversion.
 *
 * \return  ADC_STATUS - Status of operation
 * \retval  ADC_STATUS_OK - Success
 * \retval  ADC_STATUS_INVALID_PARAMETER - Bad ADC number
 *
 * ========================================================================== */
ADC_STATUS L2_AdcStart(ADC_INSTANCE Num, bool HwTrig)
{
    ADC_STATUS Status;      // Function return status
    ADC_STRUCT *pCfg;       // Pointer to ADC configuration

    Status = ADC_STATUS_OK; // Default to no error

    do
    {
        // Validate parameters
        if (ADC_COUNT <= Num)
        {
            Status = ADC_STATUS_INVALID_PARAMETER;
            break;
        }

        pCfg = &AdcCfg[Num];                                    // Point to configuration block for selected ADC

        if (!HwTrig || ADC1 == Num)                             // Use software triggering
        {
            ADC_SC1_REG(pCfg->pBasePtr, 0) = pCfg->SeChannel;   // Trigger ADC by writing channel directly
        }
        else                                                    // Trigger ADC by enabling DMA
        {
            *pCfg->pPcrReg |= PORT_PCR_ISF_MASK;                // Clear port interrupt pin

            DMA_CSR_REG(DMA_BASE_PTR, pCfg->DmaTrigChan) = DMA_CSR_DREQ_MASK;   // Start DMA trigger transfer
            DMA_SERQ = DMA_SERQ_SERQ(pCfg->DmaTrigChan);
        }

    } while (false);

    return (Status);
}

/* ========================================================================== */
/**
 * \brief   Read last Result for this ADC module
 *
 * \details Checks for ADC conversion complete - If set, the new Result is returned,
 *          otherwise the last Result is returned. The Result has the Offset, if any,
 *          subtracted from it.
 *
 * \note    If the value returned by the ADC is less than the user supplied Offset,
 *          then 0 is returned.
 *
 * \param   Num - ADC to retrieve value from.
 * \param   pResult - Pointer to word to store Result in.
 *
 * \return  ADC_STATUS - Status of operation
 * \retval  ADC_STATUS_BUSY - Conversion in progress
 * \retval  ADC_STATUS_DATA_OLD - Data from previous conversion
 * \retval  ADC_STATUS_DATA_NEW - Conversion completed - new data returned
 * \retval  ADC_STATUS_INVALID_PARAMTER - Bad ADC number
 *
 * ========================================================================== */
ADC_STATUS L2_AdcRead(ADC_INSTANCE Num, uint16_t *pResult)
{
    ADC_STATUS Status;      // Function return status
    ADC_STRUCT *pCfg;       // Pointer to ADC configuration

    Status = ADC_STATUS_DATA_OLD; // Default to old data

    do
    {
        // Validate parameters
        if ((ADC_COUNT <= Num)||(NULL == pResult))
        {
            Status = ADC_STATUS_INVALID_PARAMETER;
            break;
        }

        pCfg = &AdcCfg[Num];                                    // Point to configuration block for selected ADC

        if (ADC_SC2_REG(pCfg->pBasePtr) & ADC_SC2_ADACT_MASK)   // See if conversion in progress
        {
            Status = ADC_STATUS_BUSY;
        }

        if (ADC_SC1_REG(pCfg->pBasePtr, 0) & ADC_SC1_COCO_MASK)
        {
            pCfg->Result = ADC_R_REG(pCfg->pBasePtr, 0);        // Conversion complete - update Result & reset complete bit
            Status = ADC_STATUS_DATA_NEW;
        }

        *pResult = 0;                                           // Default to value of 0.

        if (pCfg->Result > pCfg->Offset)
        {
            *pResult = pCfg->Result - pCfg->Offset;             // Return Result (old or new)
        }

    } while (false);

    return (Status);
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

