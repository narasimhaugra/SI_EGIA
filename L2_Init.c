#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Init
 * \{
 * \brief   Layer 2 Initialization function
 *
 * \details This file contains layer 2 initialization function which invokes
 *          all relevant module functions.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_Init.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/

#include "L2_Init.h"
#include "L2_Flash.h" 
#include "L2_Dma.h"
#include "L2_Adc.h"
#include "L2_I2c.h"
#include "L2_Spi.h"
#include "L2_Timer.h"
#include "Logger.h"
#include "L2_OnchipRtc.h"  
#include "FileSys.h"
#include "McuX.h"
#include "L2_OnchipRtc.h"
#include "L2_Flash.h"
#include "L2_Lptmr.h"


/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_INIT) ///< Log Group Identifier

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
/* None */
/******************************************************************************/
/*                             Global Function Prototype(s)                    */
/******************************************************************************/
/* None */
/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
/* None */
/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/* None */

/* ========================================================================== */
/**
 * \brief   Layer 2 initialization function
 *
 * \details Initializes Layer 2 functions and module
 *
 * \note    Initialize All L2 components - GPIO, UART, SPI, I2C, ADC, RTC, Flash,
 *          SDIO, RTC, PWM, DMA, etc.
 *
 * \todo 11/03/2020 GK Revisit the Log_Init & FsInit order, once we have FS Event logging.
 *
 * \param   < None >
 *
 * \return  bool - status
 *
 * ========================================================================== */
bool L2_Init(void)
{
    bool Status;

    Status = true;  // Default to no error

    /* Initialize L2 / Utils Modules in desired order */

        L2_OnchipRtcInit();           /* Initialize Onchip RTC module */
        L2_DmaInit();                 /* Initialize DMA */
        LoggerCtor();                 /* Initialize Logger early, so that we can Log all init errors */

        McuXLogSWDump();              /* If the current reset is due to an MCU Exception, Log the SW Dump */
        McuXGetPrevResetReason();     /* Log the previous reset reason to the Event Log, both for Normal and Exception Resets. */
        L2_TimerInit();               /* Initialize timers \todo: Wait until L4 completion and remove if not used anywhere? */
        L2_I2cInit();                 /* Initialize I2C */        

    	Status = (ADC_STATUS_OK != L2_AdcInit()); /* ADC initialization error - exit bad */
    	Status = Status || (SPI_STATUS_OK != L2_SpiInit()); /* SPI initialization error - exit bad */
        CpuTimeLogInit();               /* CPU Counter with event logs */
           
        L2_FlashInit(NULL);             /* Initialize the Flash config structures, "NULL" will initialize the structure with default parameters */
        L2_LptmrInit();                 /*Low power timer initialization */
    return Status;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
