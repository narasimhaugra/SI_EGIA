#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_OneWire
 * \{
 * \brief   Onewire RTC clock implementation
 *
 * \details This module implements functions necessary for reading and updating
 *          1-Wire RTC clock DS2417 available in the hardware
 *
 *              - Initialize 1-Wire RTC hardware
 *              - Read from 1-Wire RTC clock
 *              - Update 1-Wire RTC clock
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_OneWireRtc.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_OneWireController.h"
#include "L3_OneWireRtc.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_1W)  /*! Log Group Identifier  */

/* DS2417 Control byte structure */
/* IE, IS2, IS1, IS0, OSC1, OSC2, 0, 0 */
#define OW_RTC_INT_ENABLE       (0x80u)     /*! Ext. Interrupt enabled  */
#define OW_RTC_INT_DISABLE      (0x00u)     /*! Ext. Interrupt disabled */
#define OW_RTC_INT_INTERVAL(m)  ((m) < 4u)  /*! 3 bits in upper nibble */
#define OW_RTC_RTC_ENABLE       (0x0Cu)     /*! Enable clock */
#define OW_RTC_RTC_DISABLE      (0x00u)     /*! Disable clock */

#define OW_RTC_CMD_READ         (0x66u)     /*! RTC read command */
#define OW_RTC_CMD_WRITE        (0x99u)     /*! RTC write command */
#define OW_RTC_WR_PACKET_SIZE   (2u + sizeof(RTC_SECONDS))   /*! RTC payload size CMD, CONTROL, 32 Bit counter */
#define OW_RTC_RD_PACKET_SIZE   (1u + sizeof(RTC_SECONDS))   /*! RTC payload size CONTROL, 32 Bit counter */
#define OW_RTC_CFG_PACKET_SIZE  (2u)        /*! RTC payload size CMD, CONTROL */
#define OW_RTC_DATA_OFFSET      (0x01u)
#define OW_RTC_CONTROL_OFFSET   (0x00u)

#define OW_RTC_DEVICE_COUNT     (1u)

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum
{                        /*! RTC clock interrupt configuration options      */
  BATT_RTC_INT_1S,       /*! RTC HW interrupt for every      1 second       */
  BATT_RTC_INT_4S,       /*! RTC HW interrupt for every      4 seconds      */
  BATT_RTC_INT_32S,      /*! RTC HW interrupt for every      32 seconds     */
  BATT_RTC_INT_64S,      /*! RTC HW interrupt for every      64 seconds     */
  BATT_RTC_INT_2048S,    /*! RTC HW interrupt for every      2048 seconds   */
  BATT_RTC_INT_4096S,    /*! RTC HW interrupt for every      4096 seconds   */
  BATT_RTC_INT_65536S,   /*! RTC HW interrupt for every      65536 seconds  */
  BATT_RTC_INT_131072S,  /*! RTC HW interrupt for every      131072 seconds */
} BATT_RTC_INT;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static uint8_t RtcControlByte;
static ONEWIRE_DEVICE_ID OwRtcDeviceID;

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static BATT_RTC_STATUS BatteryRtcKickStart(void);
static bool IsRtcDetected(void);
static BATT_RTC_STATUS OwRtcTranslate(ONEWIRE_STATUS OwStatus);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Configure 1-Wire RTC
 *
 * \details This function write control byte to 1-Wire RTC device to start it
 *
 * \note    RTC should never be stopped. Hence start/stop interface not
 *          provided. Interrupts are disabled as interrupt pin is not
 *          not connected to host MCU.
 *
 * \param   < None >
 *
 * \return  BATT_RTC_STATUS - Status
 *
 * ========================================================================== */
static BATT_RTC_STATUS BatteryRtcKickStart(void)
{
    uint8_t Buffer[OW_RTC_CFG_PACKET_SIZE];
    uint8_t *pBuf;
    BATT_RTC_STATUS Status;
    ONEWIRE_STATUS OwStatus;

    ONEWIREFRAME RtcFrame;
    ONEWIREPACKET* pPacket;

    Status = BATT_RTC_STATUS_NOT_FOUND; /* Default return status */

    if (IsRtcDetected())
    {
        pBuf = Buffer;  /* Helper pointer, helps avoid magic numbers */

        /* Prepare RTC frame CMD, Control, [32 bit Time] */
        *pBuf++ = OW_RTC_CMD_WRITE;
        *pBuf++ = RtcControlByte;

        RtcFrame.Device = OwRtcDeviceID;
        RtcFrame.pHandler = NULL;     /* Handler to process interim Rx data */

        /* Prepare 1st packet in the frame, RTC needs only packet */
        pPacket = &RtcFrame.Packets[0];

        pPacket->pTxData = Buffer;
        pPacket->nTxSize = OW_RTC_CFG_PACKET_SIZE;
        pPacket->pRxData = NULL;
        pPacket->nRxSize = 0;

        /* Mark end of frame by adding invalid packet */
        pPacket++;

        pPacket->pTxData = NULL;
        pPacket->nTxSize = 0;
        pPacket->pRxData = NULL;
        pPacket->nRxSize = 0;

        OwStatus = L3_OneWireTransfer(&RtcFrame);
        Status = OwRtcTranslate(OwStatus);        /* Translate error */
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Find if 1-Wire RTC device detected
 *
 * \details This function query 1-Wire controller to find if any RTC device
 *          is detected in the system. If detected, copies the device address
 *          in the buffer passed.
 *
 * \param   < None >
 *
 * \return  bool - 'true' if device found, else 'false'
 *
 * ========================================================================== */
static bool IsRtcDetected(void)
{
    uint8_t Count;
    bool Status;

    Count = OW_RTC_DEVICE_COUNT;    /* Only one 1-Wire RTC available in the system */
    Status = false;                 /* Default Status - no device */

    do
    {
        if(ONEWIRE_DEVICE_ID_INVALID != OwRtcDeviceID)
        {
            Status = true; /* Valid device already exists */
            break;
        }

        /// \\todo 02-13-2022 KA: should not happen, but if e.g 2 devices are found, memory after OwRtcDeviceID will be corrupted. Not safe, revise/refactor.
        if (ONEWIRE_STATUS_OK != L3_OneWireDeviceGetByFamily(ONEWIRE_DEVICE_FAMILY_RTC, &OwRtcDeviceID, &Count))
        {
            /* Error in accessing device list or no device found */
            break;
        }

        if (OW_RTC_DEVICE_COUNT == Count)
        {
            /* Found valid RTC device */
            Log(DBG, "1W RTC Device found %llx",OwRtcDeviceID);

            Status = true;
            break;
        }

    } while(false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Translate 1-Wire error to RTC relevant errors
 *
 * \details This function converts 1-Wire Controller specific status codes to
 *          RTC specific status code.
 *
 * \param   OwStatus - 1-Wire Controller function return status
 *
 * \return  BATT_RTC_STATUS - RTC specific Status
 *
 * ========================================================================== */
static BATT_RTC_STATUS OwRtcTranslate(ONEWIRE_STATUS OwStatus)
{
    BATT_RTC_STATUS RtcStatus;

    switch(OwStatus)
    {
        case ONEWIRE_STATUS_OK:
            RtcStatus = BATT_RTC_STATUS_OK;
            break;

        case ONEWIRE_STATUS_NO_DEVICE:
            RtcStatus = BATT_RTC_STATUS_NOT_FOUND;
            break;

        case ONEWIRE_STATUS_PARAM_ERROR:
            RtcStatus = BATT_RTC_STATUS_PARAM_ERROR;
            break;

        default:
            RtcStatus = BATT_RTC_STATUS_ERROR;
            break;
    }

    return RtcStatus;
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Initialize 1-Wire RTC
 *
 * \details This function initializes 1-Wire RTC device DS2417 without updating the time value.
 * \note    1-Wire Controller must be initialized and enabled before calling this function.
 *
 * \param   < None >
 *
 * \return  BATT_RTC_STATUS - Status
 *
 * ========================================================================== */
BATT_RTC_STATUS L3_BatteryRtcInit (void)
{
    BATT_RTC_STATUS Status;
    ONEWIREOPTIONS    Options;             /* One wire config options */
    ONEWIRE_STATUS    OwStatus;            /* One wire status */
//    uint32_t ReferenceClock;

    Status = BATT_RTC_STATUS_ERROR; /* Default return status */

    /* Override OS time */
/// \\todo 02-13-2022 KA : SigTime should just increment from 0, never set to RTC time.
//    ReferenceClock = SigTime() ;
//    if (ReferenceClock < SIGNIA_RTC_DEFAULT_VALUE)
//    {
//        SigTimeSet(SIGNIA_RTC_DEFAULT_VALUE);
//        Log(TRC, "Handle clock [was %ul] updated to default [%u]", ReferenceClock, SIGNIA_RTC_DEFAULT_VALUE);
//    }

    /* Configure RTC bus */

    /* \todo:   1) Enable RTC functionality only if HW support is available
                2) Move the battery RTC module to L4?  */

    Options.DeviceCount = 1;
    Options.KeepAlive = 0;
    Options.pHandler = NULL;
    Options.ScanInterval  = 1000;
    Options.Speed = ONEWIRE_SPEED_STD;
    Options.Bus = ONEWIRE_BUS_EXP;
    OwStatus = L3_OneWireBusConfig(&Options);

    if (ONEWIRE_STATUS_OK == OwStatus)
    {
        Status = BATT_RTC_STATUS_OK;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Read time
 *
 * \details This function reads current time from 1-Wire RTC DS2417.
 *
 * \param   pTime - Pointer variable to store the read time
 *
 * \return  BATT_RTC_STATUS - Status
 *
 * ========================================================================== */
BATT_RTC_STATUS L3_BatteryRtcRead(RTC_SECONDS *pTime)
{
    BATT_RTC_STATUS Status;     // RTC function Status
    ONEWIRE_STATUS OwStatus;    // 1-Wire function Status

    uint8_t Buffer[OW_RTC_RD_PACKET_SIZE];  // Buffer to hold RTC read data
    uint8_t *pBuf;                          // Helper buffer pointer
    static bool RtcInitDone = false;        // Init singleton flag

    ONEWIREFRAME RtcFrame;      // RTC communication frame
    ONEWIREPACKET* pPacket;     // 1-Wire communication packet

    if (!RtcInitDone)  /* RTC Already initialized? */
    {
        /* Initialize RTC before reading */
        do
        {
            /* Invalidate device address. Will be updated once the RTC is detected */
            OwRtcDeviceID = ONEWIRE_DEVICE_ID_INVALID;

            /* Setup control byte to be programmed - Clock enabled, Interrupt disabled, dummy interrupt interval  */
            RtcControlByte = (OW_RTC_RTC_ENABLE | OW_RTC_INT_DISABLE );

            if (!IsRtcDetected())
            {
                Status = BATT_RTC_STATUS_NOT_FOUND;
                break;  /* RTC not found */
            }

            Status = BatteryRtcKickStart();

            if (BATT_RTC_STATUS_OK != Status)
            {
                Log(TRC," 1-Wire RTC not found.");
            }
			else
            {
                Log(TRC," 1-Wire RTC found. Addr: 0x%16llX", OwRtcDeviceID);
                RtcInitDone = true;
            }
        } while (false);
	}

    /* Read RTC if initialized and configured */
    if (RtcInitDone)
    {
        pBuf = Buffer; /* Helper pointer */
        *pBuf++ = OW_RTC_CMD_READ;

        RtcFrame.Device = OwRtcDeviceID;
        RtcFrame.pHandler = NULL; /* Handler to process interim Rx data */

        /* Prepare 1st packet in the frame, RTC needs only packet */
        pPacket = &RtcFrame.Packets[0];

        pPacket->pTxData = Buffer;
        pPacket->nTxSize = 1;       /* Only command goes out */
        pPacket->pRxData = Buffer;  /* Reuse the same buffer for storing read data */
        pPacket->nRxSize = OW_RTC_RD_PACKET_SIZE;

        /* Mark end of frame by adding invalid packet */
        pPacket++;

        pPacket->pTxData = NULL;
        pPacket->nTxSize = 0;
        pPacket->pRxData = NULL;
        pPacket->nRxSize = 0;

        OwStatus = L3_OneWireTransfer(&RtcFrame);

        Status = OwRtcTranslate(OwStatus);        /* Translate status code */

        if (Status == BATT_RTC_STATUS_OK)
        {
            /* Transfer was successful, copy time info. pBuf is already pointing to second byte */
            *pTime = (Buffer[1] << 0) | (Buffer[2] << 8) |  (Buffer[3] << 16) | (Buffer[4] << 24) ;
        }
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Write time
 *
 * \details This function writes current time to 1-Wire RTC DS2417.
 *
 * \param   pTime - pointer to New time value to be updated
 *
 * \return  BATT_RTC_STATUS - Status
 *
 * ========================================================================== */
BATT_RTC_STATUS L3_BatteryRtcWrite(RTC_SECONDS *pTime)
{
    uint8_t Buffer[OW_RTC_WR_PACKET_SIZE];   /* one byte extra for command */
    uint8_t *pBuf;
    BATT_RTC_STATUS Status;
    ONEWIRE_STATUS OwStatus;

    ONEWIREFRAME RtcFrame;
    ONEWIREPACKET* pPacket;

    Status = BATT_RTC_STATUS_NOT_FOUND; /* Default return status */

    if (IsRtcDetected())
    {
        pBuf = Buffer;  /* Helper pointer, helps avoid magic numbers */

        /* Prepare RTC frame CMD, Control, [32 bit Time] */
        *pBuf++ = OW_RTC_CMD_WRITE;
        *pBuf++ = RtcControlByte;

        /* Copy time info to packet */
        *pBuf++ = (*pTime >> 0)  & BYTE_MASK;
        *pBuf++ = (*pTime >> 8)  & BYTE_MASK;
        *pBuf++ = (*pTime >> 16) & BYTE_MASK;
        *pBuf++ = (*pTime >> 24) & BYTE_MASK;

        RtcFrame.Device = OwRtcDeviceID;
        RtcFrame.pHandler = NULL;     /* Handler to process interim Rx data */

        /* Prepare 1st packet in the frame, RTC needs only packet */
        pPacket = &RtcFrame.Packets[0];

        pPacket->pTxData = Buffer;
        pPacket->nTxSize = OW_RTC_WR_PACKET_SIZE;
        pPacket->pRxData = NULL;
        pPacket->nRxSize = 0;

        /* Mark end of frame by adding invalid packet */
        pPacket++;

        pPacket->pTxData = NULL;
        pPacket->nTxSize = 0;
        pPacket->pRxData = NULL;
        pPacket->nRxSize = 0;

        OwStatus = L3_OneWireTransfer(&RtcFrame);
        Status = OwRtcTranslate(OwStatus);        /* Translate error */
    }

    return Status;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
