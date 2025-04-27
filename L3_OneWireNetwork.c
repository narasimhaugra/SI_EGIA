#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
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
 * \file    L3_OneWireNetwork.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_OneWireNetwork.h"
#include "L3_OneWireLink.h"
#include "Crc.h"
#include "FaultHandler.h"
/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_1W)  /// For timing tests

#define NET_CMD_MATCH           (0x55u)      ///< 1-Wire match ROM command
#define NET_CMD_SKIP            (0xCCu)      ///< 1-Wire skip ROM command
#define NET_CMD_MATCH_OD        (0x69u)     ///< 1-Wire match ROM with OD command
#define NET_CMD_SKIP_OD         (0x3Cu)      ///< 1-Wire skip ROM with OD command
#define NET_CMD_RESUME          (0xA5u)      ///< 1-Wire resume ROM command
#define NET_CMD_RESUME          (0xA5u)      ///< 1-Wire resume ROM command
#define NET_CMD_READ            (0x33u)      ///< 1-Wire read ROM command
#define NET_CMD_SEARCH_ALL      (0xF0u)      ///< 1-Wire search full ROM command
#define NET_CMD_SEARCH_ALARM    (0xFCu)      ///< 1-Wire search alarm ROM command

#define ONEWIRE_ID_BITS         (64u)        ///< Total Bits in onewire device ID
#define OW_DEVICE_ADDRESS_BITS  (56u)        ///< 1-Wire device address bits part in 64bit ROM ID

#define GET_64BIT(m, n)         ((bool)(((m) >> (n)) & 0x1))   ///< return bit value at n of m
#define SET_64BIT(m, n)         ((m) |=  (1 << (n)))           ///< Set bit value at n of m
#define CLR_64BIT(m, n)         ((m) &=  ~(0x1 << (n)))        ///< Set bit value at n of m

#define DEVICE_CRC(m)           ((uint8_t)((m) >> OW_DEVICE_ADDRESS_BITS))    ///< CRC is in MSB 8 bit in 64 bit ROM ID
#define CRC_BYTES               (7u)                     ///< Bytes involved in CRC
#define MAX_1W_COMM_RETRY       (5u) 

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
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static ONEWIRE_STATUS OwNetworkCommand(OW_NET_CMD Command, ONEWIRE_DEVICE_ID *pSlaveAddress);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Send a 1-Wire ROM command on the bus
 *
 * \details The function issues a reset and requested command.
 *
 * \param   Command - Command to execute
 * \param   pSlaveAddress - Pointer to the address
 *
 * \return  ONEWIRE_STATUS - Status
 * ========================================================================== */
static ONEWIRE_STATUS OwNetworkCommand(OW_NET_CMD Command, ONEWIRE_DEVICE_ID *pSlaveAddress)
{
    bool            DevicePresent;
    uint8_t         addrIdx;
    ONEWIRE_STATUS  Status;
    uint8_t         RcvdByte;
    uint8_t         ByteToSend;

    Status = ONEWIRE_STATUS_OK;              // Default to no error

    do
    {
        /* Update any pending configuration, device with OD search need this config after reset */
        Status = OwLinkUpdateConfig();

        if (Status != ONEWIRE_STATUS_OK)
        {
           break;
        }

        Status = OwLinkReset(&DevicePresent);   // Reset bus
        if (Status != ONEWIRE_STATUS_OK)
        {
            break;
        }

        if (!DevicePresent)
        {
            Status = ONEWIRE_STATUS_NO_DEVICE;
            break;
        }

        Status = OwLinkWriteByte(Command);
        if (Status != ONEWIRE_STATUS_OK)
        {
            break;
        }

        /* Read/writes followed by command - read, write address, etc*/
        switch(Command)
        {

            case OW_NET_CMD_MATCH_OD:
                //break;                /*Intentional drop */

            case OW_NET_CMD_MATCH:

                if (NULL == pSlaveAddress) // Null parameter - quit now
                {
                    Status = ONEWIRE_STATUS_PARAM_ERROR;
                }
                else
                {
                    /* Send address to match */
                    for (addrIdx = 0; addrIdx < ONEWIRE_ADDR_LENGTH; addrIdx++)
                    {
                        /*64 bit address, send byte-by-byte*/
                        ByteToSend = (*pSlaveAddress>>(addrIdx*BITS_PER_BYTE)) & BYTE_MASK;
                        Status = OwLinkWriteByte(ByteToSend);
                        if (Status != ONEWIRE_STATUS_OK)
                        {
                            break;
                        }
                    }
                }
                break;

            case OW_NET_CMD_READ:

                if (NULL == pSlaveAddress) // Null parameter - quit now
                {
                    Status = ONEWIRE_STATUS_PARAM_ERROR;
                    break;
                }

                *pSlaveAddress = 0;

                /* Read address, MSB first */
                for (addrIdx = 0; addrIdx < ONEWIRE_ADDR_LENGTH; addrIdx++)
                {
                    Status = OwLinkReadByte(&RcvdByte);
                    if (Status != ONEWIRE_STATUS_OK)
                    {
                        break;
                    }
                    *pSlaveAddress |= (((ONEWIRE_DEVICE_ID)RcvdByte) << (addrIdx * BITS_PER_BYTE)) ;
                }
                break;

            case OW_NET_CMD_RESUME:
                /*Do nothing */
                 break;

            case OW_NET_CMD_SKIP:
                /*Do nothing */
                 break;

            case OW_NET_CMD_SKIP_OD:
                /*Do nothing */
                 break;

            case OW_NET_CMD_SEARCH_ALL:
                /*Do nothing */
                 break;

            case OW_NET_CMD_SEARCH_ALM:
                /*Do nothing */
                 break;


            default:
                break;

        }/* end of switch */

    } while (false);

    return Status;
}

/******************************************************************************/
/*                             Global Function(s)                              */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Initialize 1-Wire transport layer
 *
 * \details Call link layer initialize function which initialize I2C communication
 *          channel
 *
 * \param   < None >
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetInit(void)
{
    ONEWIRE_STATUS Status;
    uint8_t CommRetry;

    CommRetry = 0;
    /* Initialize the link layer which also initialize the DS2465 */
    do
    {
        Status  = OwLinkInit();
        if (ONEWIRE_STATUS_OK != Status)
        {
            CommRetry++;
            if (MAX_1W_COMM_RETRY == CommRetry)
            {
                /* Master chip DS2465 COMM FAIL */
                /* REQ ID : 242590 */
                Log(ERR, "OW Comm Failure, Exiting after 5 Retry");
                FaultHandlerSetFault(PERMFAIL_ONEWIREMASTER_COMMFAIL, SET_ERROR);
                break;
            }
        }
        else
        {
            break;
        }
    }while (MAX_1W_COMM_RETRY > CommRetry);
    
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Scan the selected bus
 *
 * \details Using the ROM SEARCH command find 1-Wire slaves on the selected bus.
 *
 * \note    See Maxim Application Note 187 for search algorithm details.
 *
 * \param   pSearchCtx - Search context.
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetSearch(OWSearchContext *pSearchCtx)
{

    uint8_t BitPos;         /* Current bit position during the 64 bit search */
    uint8_t LastZero;       /* Position of last zero detected */
    bool DevicePresent;     /* Device Presence indicator from bus reset */
    bool TrueBit;           /* Actual status in search operation */
    bool CompBit;           /* Compliment in search operation */
    ONEWIRE_DEVICE_ID RomId;/* ROM address detected */
    bool search_direction;  /* Search direction : 1 path or 0 path */
    ONEWIRE_STATUS Status;  /* This function return status */
    uint8_t     crc8;
    uint8_t ByteCount;
    bool BitValue;          /* Value to send over the bus */

    BitValue = true;

    do
    {
        /* Check search context pointer, return with error if not valid  */
        if (NULL == pSearchCtx)
        {
            Status = ONEWIRE_STATUS_PARAM_ERROR;
            break;
        }

        Status = OwNetCmdSearch(&DevicePresent);
        if ((ONEWIRE_STATUS_BUS_ERROR == Status) && (!DevicePresent))
        {
            pSearchCtx->RomId = 0;
            break;
        }
        
        if (!DevicePresent)
        {
            pSearchCtx->RomId = 0;

            /* No device present, exit scan */
            Status = ONEWIRE_STATUS_OK;
            break;
        }

        /* Execute search algorithm */

        /* Default variable initializations */
        LastZero = 0;
        RomId = pSearchCtx->LastConflict;
        DevicePresent = false;

        for (BitPos = 0; BitPos < ONEWIRE_ID_BITS; BitPos++)
        {
            /* Check MAXIM website for 1-wire search algorithm, application note 187  */

            Status = OwLinkWriteBit(BitValue, &TrueBit);
            if (ONEWIRE_STATUS_OK != Status )
            {
                /* Couldn't write a bit, exit with error */
                break;
            }

            Status = OwLinkWriteBit(BitValue, &CompBit);
            if (ONEWIRE_STATUS_OK != Status )
            {
                /* Couldn't write a bit, exit with error */
                break;
            }

            if (TrueBit && CompBit)
            {
                /* No device present, exit scan */
                pSearchCtx->RomId = 0;
                Status = ONEWIRE_STATUS_OK;
                break;
            }
            else if (TrueBit != CompBit)
            {
                /* Either all 0's or all 1's */
                search_direction = TrueBit;
            }
            else
            {
                /* Conflict found here; Both 0's or 1's in current position
                 If this discrepancy is before the last discrepancy
                 from a previous scan, then pick the same as last time */

                if (BitPos < pSearchCtx->LastConflict)
                {
                    search_direction = GET_64BIT(RomId, BitPos);
                }
                else
                {
                    search_direction = (BitPos == pSearchCtx->LastConflict)? true : false;
                }

                /* if 0 was picked then record its position in LastZero */
                if (search_direction == 0)
                {
                    LastZero = BitPos;
                }
            }

            if (search_direction)
            {
                RomId |= ((ONEWIRE_DEVICE_ID)0x1 << BitPos);
            }else
            {
                RomId &= ~((ONEWIRE_DEVICE_ID)0x1 << BitPos);
            }

            Status = OwLinkWriteBit(search_direction, NULL);

            if (Status != ONEWIRE_STATUS_OK)
            {
                Status = ONEWIRE_STATUS_ERROR;     /* Error in writing bit */
                break;
            }

        }  /* 64 bit search loop*/

        /* Validate if a valid ROM ID is found */
        if ((BitPos < ONEWIRE_ID_BITS) || (Status != ONEWIRE_STATUS_OK))
        {
            /* error occured, exit */
            break;
        }

        /* All 64 spins are done, not exited abnormally */
        /* Calculate ROM ID CRC excluding hardcoded CRC. Reusing BitPos as index */
        crc8 = 0;
        for (ByteCount=0; ByteCount < CRC_BYTES; ByteCount++)
        {
            crc8 = DoCRC8(crc8, (uint8_t)((RomId >> (ByteCount * BITS_PER_BYTE)) & BYTE_MASK));
        }

        /* Compare CRC in ROM ID and the calculated CRC. CRC is in update 8 bits of 64 bit ROM ID */
        if (crc8 == DEVICE_CRC(RomId))
        {
            /* Good ROM ID, return to the caller */
            pSearchCtx->LastConflict = LastZero;
            pSearchCtx->RomId = RomId;

            pSearchCtx->LastDevice = (0 == pSearchCtx->LastConflict)? true:false;
        }
    }
    while (false);

  return Status;
}

/* ========================================================================== */
/**
 * \brief   Set 1-Wire bus speed
 *
 * \details This is a passthrough function, invokes function in 1-Wire link layer.
 *
 * \param   Speed - Communication speed, standard or overdrive.
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetSetSpeed(ONEWIRE_SPEED Speed)
{
    return OwLinkSetSpeed(Speed);
}

/* ========================================================================== */
/**
 * \brief   Send data over 1-Wire bus
 *
 * \details Send data over 1-Wire bus supplied by the caller
 *
 * \param   pData - Pointer data to sent
 * \param   Len - Data size
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetSend(uint8_t *pData, uint8_t Len)
{
    uint8_t Count;
    ONEWIRE_STATUS Status;

    Status = ONEWIRE_STATUS_OK;

    for (Count=0; Count < Len ; Count++)
    {
        Status = OwLinkWriteByte(pData[Count]);

        if (ONEWIRE_STATUS_OK != Status)
        {
            /* Error in sending, exit */
            break;
        }
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Receive data from a device over 1-Wire bus
 *
 * \details Receive data from the selected slave 1-Wire device
 *
 * \param   pData - Pointer data to sent
 * \param   Len - Data size
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetRecv(uint8_t *pData, uint8_t Len)
{
    uint8_t Count;
    ONEWIRE_STATUS Status;

    Status = ONEWIRE_STATUS_OK;

    for (Count=0; Count < Len ; Count++)
    {
        Status = OwLinkReadByte(pData++);

        if (ONEWIRE_STATUS_OK != Status)
        {
            /* Error in receiving data, exit */
            break;
        }
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Send a 1-Wire SELECT ROM command on the bus
 *
 * \details Sends a 1-Wire SELECT ROM command on the bus to select a device
 *
 * \note    This command should not be used when multiple devices are expected
 *          on the selected 1-Wire bus.
 *
 * \param   pAddress - Device address
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetCmdSelect(ONEWIRE_DEVICE_ID *pAddress)
{
    return OwNetworkCommand(OW_NET_CMD_MATCH, pAddress);
}

/* ========================================================================== */
/**
 * \brief   Check a known device connectivity
 *
 * \details Sends a 1-Wire SELECT ROM command and attempts to write a bit 0.
 *          if device present, then the response would be 0 as well, otherwise 1.
 *
 * \param   pAddress - Device address
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetDeviceCheck(ONEWIRE_DEVICE_ID *pAddress)
{
    ONEWIRE_STATUS Status;
    ONEWIRE_DEVICE_ID AddrOnBus;    /* ANDed Address of all avilable devices */

    Status = OwNetworkCommand(OW_NET_CMD_READ, &AddrOnBus);

    if (ONEWIRE_STATUS_OK == Status)
    {
        /* ANDed address is available, All '0' from the device address must be '0'
           If any '1' found in place or '0', then the device didn't respond */
        Status = (AddrOnBus & ~(*pAddress)) ? ONEWIRE_STATUS_NO_DEVICE : ONEWIRE_STATUS_OK;
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Send a 1-Wire SELECT_OD ROM command on the bus
 *
 * \details Sends a 1-Wire SELECT_OD ROM command on the bus to select a device
 *          with overdrive speed capabilities.
 *
 * \note    This command should not be used when multiple devices are expected
 *          on the selected 1-Wire bus.
 *
 * \param   pAddress - Device address
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetCmdSelectOD(ONEWIRE_DEVICE_ID *pAddress)
{
    return OwNetworkCommand(OW_NET_CMD_SKIP_OD, pAddress);
}

/* ========================================================================== */
/**
 * \brief   Send a 1-Wire SKIP ROM command on the bus
 *
 * \details Sends a 1-Wire SKIP ROM command on the bus to select a device without
 *          sending address.
 *
 * \note    This command should not be used when multiple devices are expected
 *          on the selected 1-Wire bus.
 *
 * \param   < None >
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetCmdSkip(void)
{
    return OwNetworkCommand(OW_NET_CMD_SKIP, NULL);
}

/* ========================================================================== */
/**
 * \brief   Send a 1-Wire SKIP_OD ROM command on the bus
 *
 * \details Sends a 1-Wire SKIP_OD ROM command on the bus to select a device without
 *          sending address. The device will respond with if overdrive speed
 *          capable.
 *
 * \note    This command should not be used when multiple devices are expected
 *          on the selected 1-Wire bus.
 *
 * \param   < None >
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetCmdSkipOD(void)
{
    return OwNetworkCommand(OW_NET_CMD_SKIP, NULL);
}

/* ========================================================================== */
/**
 * \brief   Send a 1-Wire RESUME ROM command on the bus
 *
 * \details Send a 1-Wire RESUME ROM command on the bus
 *
 * \param   < None >
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetCmdResume(void)
{
    return OwNetworkCommand(OW_NET_CMD_RESUME, NULL);
}

/* ========================================================================== */
/**
 * \brief   Read Device address on bus
 *
 * \details Sends a 1-Wire READ ROM command on the bus, and reads the address
 *          returned by the device
 *
 * \note    This command should not be used when multiple devices are expected
 *          on the selected 1-Wire bus.
 *
 * \param   Address - Pointer to store the read address
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetCmdRead(ONEWIRE_DEVICE_ID *Address)
{
    return OwNetworkCommand(OW_NET_CMD_READ, Address);
}

/* ========================================================================== */
/**
 * \brief   Start search algorithm
 *
 * \details Sends a 1-Wire search ROM command on the bus
 *
 * \param   *pDevicePresent - Pointer to store the read address
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
 ONEWIRE_STATUS OwNetCmdSearch(bool *pDevicePresent)
{
    ONEWIRE_STATUS Status;

    Status = OwNetworkCommand(OW_NET_CMD_SEARCH_ALL, NULL);

    if (ONEWIRE_STATUS_BUS_ERROR == Status)
    {  
        Status = ONEWIRE_STATUS_BUS_ERROR;
    }
    *pDevicePresent = (ONEWIRE_STATUS_OK == Status)? true: false;      
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Reset 1-Wire bus
 *
 * \details Issue reset on the selected -Wire bus.
 *
 * \param   < None >
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetReset(void)
{
    ONEWIRE_STATUS Status;
    bool HaveDevice;

    Status = ONEWIRE_STATUS_ERROR;

    if (ONEWIRE_STATUS_OK == OwLinkReset(&HaveDevice))
    {
        Status = HaveDevice? ONEWIRE_STATUS_OK: ONEWIRE_STATUS_NO_DEVICE;
    }

    return Status;
}


/* ========================================================================== */
/**
 * \brief   Control 1-Wire link state
 *
 * \details Enabled/Disable 1-Wire bus.
 *
 * \param   Enable - 1-Wire state, enabled if true otherwise disabled
 *
 * \return  ONEWIRE_STATUS - Status
 *
 * ========================================================================== */
ONEWIRE_STATUS OwNetEnable(bool Enable)
{
    return OwLinkSleep(!Enable);
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

