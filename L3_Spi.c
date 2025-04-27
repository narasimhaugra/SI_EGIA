#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_SPI
 * \{
 * \brief   Layer 3 SPI Sharing Routines
 *
 * \details This file contains IPC protected SPI interfaces implementation.
 *
 * \note    SPI0 is dedicated to the FPGA and does not using SPI sharing. The
 *          FPGA calls L2_SPI functions directly.
 *
 * \sa
 *
 * \copyright 2020 Medtronic - Surgical Innovations. All Rights Reserved.
 *
 * \file L3_Spi.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_Spi.h"
#include "Logger.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/
/* None */

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
/* None */

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_SPI)          ///< Log Group Identifier
#define SPI_DEFAULT_TIMEOUT         ((uint32_t)0)            ///< Time out in Clock ticks

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/// SPI configuration Structure.
typedef struct
{
    SPI_PORT    Port;           ///< SPI Port
    SPI_CHANNEL Channel;        ///< SPI Channel
    SPI_FRAME_SIZE FrameSize;   ///< SPI Frame size
    uint16_t DelayMs;           ///< Delay(ms) between transmit and receive transfers
    uint16_t Timeout;        ///< Transfer Timeout in ms
} SpiConfig;


/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
/* None */

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static OS_EVENT *pMutex_SPI2 = NULL;                     ///< SPI access mutex, synchronizes L2 SPI calls

/* The device interface information, the list to be in sync with SPI_DEVICE enum */
    
static SpiConfig DeviceConfig[SPI_DEVICE_COUNT] =      ///< Spi Config Control Params
{
    /* Default Configuration for Charger */
    {SPI_PORT_TWO, SPI_CHANNEL0, SPI_FRAME_SIZE8, 150, 100,},

    /* Default Configuration for Accelerometer */
    {SPI_PORT_TWO, SPI_CHANNEL1, SPI_FRAME_SIZE16, 0, 100,}
};

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Layer 3 SPI hardware initialization routine.
 *
 * \details Initializes SPI
 *
 * \note    This function is intended to be called once during the system
 *          initialization to initialize all applicable SPI hardware ports.
 *          Any other SPI interface functions should be called only after
 *          calling this function.
 *
 * \param   < None >
 *
 * \return  SPI_STATUS - Returned Configuration status
 * ========================================================================== */
SPI_STATUS L3_SpiInit(void)
{
    SPI_STATUS Status;          /* SPI Status to retun */
    uint8_t OsError;            /* OS Error Variable */

    Status = SPI_STATUS_OK;

    /* Default SPI0 and SPI2 initialization */    
    L2_SpiSetup(SPI_PORT_ZERO, SPI_CHANNEL0);
    L2_SpiSetup(SPI_PORT_TWO, SPI_CHANNEL0);

    pMutex_SPI2 = SigMutexCreate("L3-SPI2", &OsError);

    if ( NULL == pMutex_SPI2 )
    {
        Status = SPI_STATUS_ERROR;
        Log(ERR, "L3_SpiInit: L3 Spi Mutex Create Error - %d", OsError);
    }

    
    return (Status);
}

/* ========================================================================== */
/**
 * \brief   Layer 3 SPI Transfer
 *
 * \details This function is a wrapper function around layer 2 SPI read and write functions
 *
 * \param   Device - SPI device address
 * \param   *pDataOut - Data to be transmitted
 * \param   OutCount - Bytes to transmit
 * \param   *pDataIn - Buffer to hold received data
 * \param   InCount - Bytes to receive
 *
 * \return  Status - function execution status
 *
 * ========================================================================== */
SPI_STATUS L3_SpiTransfer(SPI_DEVICE Device, uint8_t *pDataOut, uint8_t OutCount, uint8_t *pDataIn, uint8_t InCount)
{
    SPI_STATUS Status;      // SPI Status to return
    uint8_t u8OsError;      // OS Error Variable
    uint16_t InWord;        // Holds data received from SPI 
    uint16_t OutWord;       // Holds data to be sent over SPI
    uint8_t DataSize;       // Data size set as per frame size
    uint16_t WaitPeriod;    // Wait period between transmit and receive
    SpiConfig *pConfig;     // SPI configuration
    
    
    do
    {
        Status = SPI_STATUS_PARAM_INVALID;
        
        if (((NULL == pDataOut) && (NULL == pDataIn)) || (Device >= SPI_DEVICE_COUNT))
        {
            Log(ERR, "L3_SpiTransfer: Invalid input(s)");
            break;
        }

        pConfig = &DeviceConfig[Device];

        if ( (InCount % pConfig->FrameSize) || (OutCount % pConfig->FrameSize))
        {
            Status = SPI_STATUS_PARAM_INVALID;
            Log(ERR, "L3_SpiTransfer: Unaligned data size ");
            break;
        }

        Status = SPI_STATUS_OK;
        
        /* Mutex Lock, ignoring error check as wait type is infinite */
        OSMutexPend(pMutex_SPI2, OS_WAIT_FOREVER, &u8OsError);

        L2_SpiSetup(pConfig->Port, pConfig->Channel);

        DataSize = pConfig->FrameSize; 
        WaitPeriod = pConfig->DelayMs;
        
        while(OutCount || InCount)
        {

            OutWord = 0;
            
            if(OutCount) { memcpy(&OutWord, pDataOut, DataSize);}

            /* Transfer */    
            InWord = L2_SpiTransfer(pConfig->Port, pConfig->Channel, OutWord, false);    

            OutCount -= (OutCount)? DataSize: 0;    // Update Tx count 
            pDataOut += DataSize;

            if (WaitPeriod && !OutCount)            // If all Tx done, check wait is needed
            {
               OSTimeDly(WaitPeriod);               
               WaitPeriod = 0;                      // no more wait needed.
            }
                        
            if (!WaitPeriod && InCount) 
            { 
                memcpy(pDataIn, (uint8_t*)&InWord, DataSize);                
                InCount  -=  (InCount)? DataSize: 0;
                pDataIn += DataSize;
            }
            
        }
        
    }while ( false );

    /* Release the Mutex */
    OSMutexPost(pMutex_SPI2);

    return Status;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

