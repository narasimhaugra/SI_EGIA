#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_OneWire
 * \{
 * \brief   Onewire EEPROM implementation
 *
 * \details This module implements functions necessary for reading and writing into
 *          1-Wire EEPROM user memory.
 *
 *              - Initialize 1-Wire EEPROM Module
 *              - Read from 1-Wire EEPROM Device
 *              - Write to 1-Wire EEPROM Device
  *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_OneWireEeprom.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_OneWireController.h"
#include "L3_OneWireEeprom.h"
#include "Crc.h"
#include "TestManager.h"
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER                       (LOG_GROUP_1W)  ///< Log Group Identifier

/* Please refer to Full Data sheet of DS28E15 for more details on the commands (Write Memory, Read Memory) */
/* and 1-Wire communication examples. */
#define OW_EEPROM_CMD_READ                         (0xF0u)         ///< 1 Wire EEPROM read command.
#define OW_EEPROM_CMD_WRITE                        (0x55u)         ///< 1 Wire EEPROM write command.
#define OW_EEPROM_RD_PACKET_SIZE                   (34u)           ///< EEPROM read data size (includes 2 bytes of CRC).
#define OW_EEPROM_CMD_PACKET_SIZE                  (2u)            ///< EEPROM payload size CMD, Page number.
#define OW_EEPROM_MEMORY_SEGMENT_SIZE              (4u)            ///< Memory Segment Size.
#define OW_EEPROM_NUM_SEGMENTS_PER_PAGE            (8u)            ///< No of Segment Per Page.
#define OW_EEPROM_NUM_PAGES                        (2u)            ///< Number of Pages.
#define OW_EEPROM_RELEASE_BYTE                     (0xAAu)         ///< Release Byte used to signal the start of the transfer.
#define OW_EEPROM_CS_SUCCESS                       (0xAAu)         ///< CS value for Command Success Indicator.
#define OW_EEPROM_CRC_BUF_SIZE                     (2u)            ///< CRC Size in bytes
#define OW_EEPROM_CMD_SUCCESS_INDICATOR_PKT_SIZE   (1u)            ///< CS value size in bytes.
#define OW_EEPROM_RELEASE_PKT_SIZE                 (1u)            ///< Release command size in bytes.
#define OW_EEPROM_CRC_CONST_VAL                    (0xB001u)       ///< CRC value of the Message transmitted and transmitted CRC bytes.
                                                                   /* Refer to https://www.maximintegrated.com/en/design/technical-documents/app-notes/2/27.html. */
#define OW_EEPROM_TXFER_WAIT                       (12u)           ///< Delay value used to wait after sending the Release command.
#define OW_EEPROM_NUM_PKTS_PER_SEGMENT             (3u)            ///< This value is used for converting the PacketIndex to Packet type while doing the write operation.
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                        /// 1-Wire EEPROM Pkt Type used for write operation.
{
    OW_EEPROM_PKT_CS      = 0,      ///< CS Packet Type.
    OW_EEPROM_PKT_RELEASE = 2,      ///< Relase Packet Type.
    OW_EEPROM_PKT_DEFAULT = 3       ///< Default Packet Type.
} OW_EEPROM_PKT_TYPE;

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

/* ========================================================================== */
/**
 * \brief   Handle interim data received during the Write operation to EEPROM.
 *
 * \param   PacketIndex - Packet index in the frame
 * \param   RxData - Pointer to data
 *
 * \return  bool - false if the CS field is SUCCESS(0xAA) else true.
 *
 * ========================================================================== */
static bool OwTransferHandler_Tx(uint8_t PacketIndex, uint8_t *RxData)
{
    uint8_t CSIndicator;   /* Variable to hold the CS byte value from the response */
    bool Status;           /* Variable to hold the return status value */
    uint8_t PktType;       /* Pkt Type value  */

    Status = false;
    PktType = OW_EEPROM_PKT_DEFAULT;

    /* Packets with Index 2,5,8,11,14,17,20,23 corresponds to Release Byte which translates to PktType 2 with below calculation  */
    /* Packets with Index 3,6,9,12,15,18,21,24 corresponds to CS Byte which translates to PktType 0 with below calculation.      */
    /* Converting PacketIndex to PktType to reduce the number of switch cases and thereby reduce the cyclomatic complexity value */
    if (0 != PacketIndex)
    {
        PktType = PacketIndex%OW_EEPROM_NUM_PKTS_PER_SEGMENT;
    }

    switch (PktType)
    {
        /* Packets corresponding the CS Byte from the Slave */
        case OW_EEPROM_PKT_CS:
            CSIndicator = *RxData;
            if (OW_EEPROM_CS_SUCCESS != CSIndicator)
            {
                Status = true;
            }
            break;

        /* Packets corresponding to the Release Byte, need to wait after initiating the transfer */
        case OW_EEPROM_PKT_RELEASE:
            OSTimeDly(OW_EEPROM_TXFER_WAIT);
            break;

        default:
            /* Do nothing */
            break;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Handle interim data received during the Read operation from EEPROM.
 *
 * \param   PacketIndex - Packet index in the frame
 * \param   RxData - Pointer to data
 *
 * \return  bool
 *
 * ========================================================================== */
static bool OwTransferHandler_Rx(uint8_t PacketIndex, uint8_t *RxData)
{
    switch (PacketIndex)
    {
        case 0:
            break;

        case 1:
            OSTimeDly(OW_EEPROM_TXFER_WAIT);
            break;

        case 2:
            break;

        default:
            /* Do nothing */
            break;
    }

    return false;
}


/* ========================================================================== */
/**
 * \brief   Translate 1-Wire error to EEPROM relevant errors
 *
 * \details This function converts 1-Wire Controller specific status codes to
 *          EEPROM specific status code.
 *
 * \param   OwStatus - 1-Wire Controller function return status
 *
 * \return  OW_EEP_STATUS - Eeprom specific Status
 *
 * ========================================================================== */
static OW_EEP_STATUS OwEepromTranslate(ONEWIRE_STATUS OwStatus)
{
    OW_EEP_STATUS EepromStatus; /* Variable to hold the Eeprom specific status value */

    switch(OwStatus)
    {
        case ONEWIRE_STATUS_OK:
            EepromStatus = OW_EEP_STATUS_OK;
            break;

        case ONEWIRE_STATUS_NO_DEVICE:
            EepromStatus = OW_EEP_STATUS_DEVICE_NOT_FOUND;
            break;

        case ONEWIRE_STATUS_PARAM_ERROR:
            EepromStatus = OW_EEP_STATUS_PARAM_ERROR;
            break;

        case ONEWIRE_STATUS_TIMEOUT:
            EepromStatus = OW_EEP_STATUS_TIMEOUT;
            break;

        case ONEWIRE_STATUS_BUS_ERROR:
            EepromStatus = OW_EEP_STATUS_COMM_ERROR;
            break;

        default:
            EepromStatus = OW_EEP_STATUS_ERROR;
            break;
    }

    return EepromStatus;
}

/* ========================================================================== */
/**
 * \brief   Default packet data
 *
 * \details Initialize all the packets in the frame to default values
 *
 * \param   pFrame - Frame
 *
 * \return  None
 *
 * ========================================================================== */
static void OwFrameClear(ONEWIREFRAME *pFrame)
{
    ONEWIREPACKET* pPacket;
    uint8_t Count;

    for (Count = 0; Count < ONEWIRE_MAX_PACKETS; Count++)
    {
         pPacket = &pFrame->Packets[Count];
         pPacket->nRxSize = 0;
         pPacket->pRxData = NULL;
         pPacket->nTxSize = 0;
         pPacket->pTxData = NULL;
    }
}

/* ========================================================================== */
/**
 * \brief   Start the Ow Transfer transaction.
 *
 * \param   pFrame - Frame to transfer
 *
 * \return  OW_EEP_STATUS - Eeprom specific Status
 * ========================================================================== */
static OW_EEP_STATUS OwFrameTransfer(ONEWIREFRAME *pFrame)
{
    OW_EEP_STATUS Status;       /* Variable to hold the Eeprom return status value */
    ONEWIRE_STATUS OwStatus;    /* Variable to hold the OneWire operation return status value */
    uint8_t RetryCount;         /* Retry Counter variable. */

    RetryCount = 0;

    do
    {
        OwStatus = L3_OneWireTransfer(pFrame);

        Status = OwEepromTranslate(OwStatus);        /* Translate status code */

        if (OW_EEP_STATUS_TIMEOUT != Status)
        {
            break;
        }

        RetryCount++;

    } while (RetryCount < OW_EEPROM_RDWR_MAX_RETRY);

    return Status;

}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Reads data from 1 Wire EEPROM device
 *
 * \details This function reads from specified 1-Wire EEPROM device.
 *
 * \param   Device - 64 Bit 1-Wire ROM ID.
 * \param   Page - EEPROM page number.
 * \param   pBuffer - Buffer to store data.
 *
 * \return  OW_EEP_STATUS - Status
 *
 * ========================================================================== */
OW_EEP_STATUS L3_OneWireEepromRead(ONEWIRE_DEVICE_ID Device, uint8_t Page, uint8_t *pBuffer)
{
    OW_EEP_STATUS Status;               /* Variable to hold the Eeprom return status value */
    ONEWIREFRAME EepromFrame;           /* Frame to hold all the packets used for tranmitting to OneWire Master */
    ONEWIREPACKET* pPacket;             /* Pointer to OneWire packet used while constructing the packets and adding to EepromFrame */
    uint8_t SendBuffer[OW_EEPROM_CMD_PACKET_SIZE];  /* Buffer used to send the command and paramter byte  */
    uint8_t CrcBuffer[OW_EEPROM_CRC_BUF_SIZE];      /* Buffer used to hold the CRC bytes received  */
    uint8_t *pBuf;                                  /* Helper  */
    uint16_t Crc16Val;                              /* Variable used to hold the CRC value computed on the data received */

    do
    {
        if ((NULL == pBuffer) || (OW_EEPROM_NUM_PAGES <= Page))
        {
            Status = OW_EEP_STATUS_PARAM_ERROR;
            Log(ERR, "L3_OneWireEepromRead: Invalid Parameter");
            break;
        }

        if(ONEWIRE_STATUS_OK != L3_OneWireDeviceCheck(Device))
        {
            Status = OW_EEP_STATUS_DEVICE_NOT_FOUND;
            Log(ERR, "L3_OneWireEepromRead: Device check failed");
            break;
        }

        OwFrameClear(&EepromFrame);                         /* Clear the Frame before preparing the packets */

        pBuf = SendBuffer;                                  /* Helper pointer */
        *pBuf++ = OW_EEPROM_CMD_READ;                       /* Command for Read Memory */
        *pBuf++ = Page;                                     /* Page number to Read the data */

        /* Prepare packet for Command and Packet byte */
        pPacket = &EepromFrame.Packets[0];
        pPacket->pTxData = SendBuffer;
        pPacket->nTxSize = OW_EEPROM_CMD_PACKET_SIZE;
        pPacket->pRxData = CrcBuffer;
        pPacket->nRxSize = OW_EEPROM_CRC_BUF_SIZE;

        /* Prepare Packet to receive the data (32 Bytes data) */
        pPacket++;
        pPacket->pTxData = NULL;
        pPacket->nTxSize = 0;
        pPacket->pRxData = pBuffer;
        pPacket->nRxSize = OW_EEPROM_MEMORY_PAGE_SIZE;

        /* Prepare Packet to receive the CRC bytes (2 Bytes CRC) */
        pPacket++;
        pPacket->pTxData = NULL;
        pPacket->nTxSize = 0;
        pPacket->pRxData = CrcBuffer;
        pPacket->nRxSize = OW_EEPROM_CRC_BUF_SIZE;

        /* Mark end of frame by adding invalid packet */
        pPacket++;
        pPacket->pTxData = NULL;
        pPacket->nTxSize = 0;
        pPacket->pRxData = NULL;
        pPacket->nRxSize = 0;

        EepromFrame.Device = Device;
        EepromFrame.pHandler = &OwTransferHandler_Rx; /* Registering callback handler to process any interim data/response */

        Status = OwFrameTransfer(&EepromFrame);

        TM_Hook(HOOK_ONEWIRE_READFAIL, &EepromFrame.Packets[0]);

        /* Computing the CRC on 34 bytes (32 data bytes + 2 CRC bytes), the resultant CRC value should be 0xB001 for a valid read */
        /* Refer to https://www.maximintegrated.com/en/design/technical-documents/app-notes/2/27.html for CRC computation */
        //Crc16Val = CRC16(0, ReadBuffer, OW_EEPROM_RD_PACKET_SIZE);
        Crc16Val = CRC16(0, pBuffer, OW_EEPROM_MEMORY_PAGE_SIZE);
        Crc16Val = CRC16(Crc16Val, CrcBuffer, OW_EEPROM_CRC_BUF_SIZE);

        if (OW_EEPROM_CRC_CONST_VAL != Crc16Val)
        {
            Status = OW_EEP_STATUS_ERROR;
            Log(ERR, "L3_OneWireEepromRead: CRC check failed on the read data");
        }

        TM_Hook(HOOK_ONEWIRECRCTEST, pBuffer);


    } while(false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Writes data to 1 Wire EEPROM device
 *
 * \details This function writes data to specified 1-Wire EEPROM device.
 *
 * \param   Device - 64 Bit 1-Wire ROM ID.
 * \param   Page - EEPROM page number.
 * \param   pData - Data source
 *
 * \return  OW_EEP_STATUS - Status
 *
 * ========================================================================== */
OW_EEP_STATUS L3_OneWireEepromWrite(ONEWIRE_DEVICE_ID Device, uint8_t Page, uint8_t *pData)
{
    uint8_t Buffer[OW_EEPROM_CMD_PACKET_SIZE];  /* Buffer used to send the command and paramter byte  */
    uint8_t CrcBuffer[OW_EEPROM_CRC_BUF_SIZE];  /* Buffer used to hold the CRC bytes received  */
    uint8_t ReleaseByte;                        /* Variable used to hold the ReleaseByte command value */
    uint8_t *pBuf;                              /* Helper Pointer */
    OW_EEP_STATUS Status;                       /* Variable to hold the Eeprom return status value */
    ONEWIREFRAME EepromFrame;                   /* Frame to hold all the packets used for tranmitting to OneWire Master */
    ONEWIREPACKET* pPacket;                     /* Pointer to OneWire packet used while constructing the packets and adding to EepromFrame */
    uint8_t SegmentIndex;                       /* Variable used to loop through the total no of segments in a Page */

    ReleaseByte = OW_EEPROM_RELEASE_BYTE;

    do
    {
        if ((NULL == pData) || (OW_EEPROM_NUM_PAGES <= Page))
        {
            Status = OW_EEP_STATUS_PARAM_ERROR;
            Log(ERR, "L3_OneWireEepromWrite: Invalid Parameter");
            break;
        }

        if(ONEWIRE_STATUS_OK != L3_OneWireDeviceCheck(Device))
        {
            Status = OW_EEP_STATUS_DEVICE_NOT_FOUND;
            Log(ERR, "L3_OneWireEepromWrite: Device check failed");
            break;
        }

        OwFrameClear(&EepromFrame);                         /* Clear the Frame before preparing the packets */

        pBuf = Buffer;                                      /* Helper pointer */
        *pBuf++ = OW_EEPROM_CMD_WRITE;                      /* Command for Write Memory */
        *pBuf++ = Page;                                     /* Page number to Write the data */

        /* Prepare packet for Command and Packet byte */
        pPacket = &EepromFrame.Packets[0];
        pPacket->pTxData = Buffer;
        pPacket->nTxSize = OW_EEPROM_CMD_PACKET_SIZE;       /* Only command goes out */
        pPacket->pRxData = CrcBuffer;                       /* Reuse the same buffer for storing read data */
        pPacket->nRxSize = OW_EEPROM_CRC_BUF_SIZE;

        for (SegmentIndex = 0; SegmentIndex < OW_EEPROM_NUM_SEGMENTS_PER_PAGE; SegmentIndex++)
        {
            /* Prepare Data packet in the frame (Actual data to write in Segment 'SegmentIndex') */
            pPacket++;
            pPacket->pTxData = pData + (SegmentIndex*OW_EEPROM_MEMORY_SEGMENT_SIZE);
            pPacket->nTxSize = OW_EEPROM_MEMORY_SEGMENT_SIZE;
            pPacket->pRxData = CrcBuffer;
            pPacket->nRxSize = OW_EEPROM_CRC_BUF_SIZE;

            /* Prepare Release packet */
            pPacket++;
            pPacket->nTxSize = OW_EEPROM_RELEASE_PKT_SIZE;
            pPacket->pTxData = &ReleaseByte;
            pPacket->nRxSize = 0;
            pPacket->pRxData = NULL;

            /* Prepare packet to read the CS (Command Success Indicator) */
            pPacket++;
            pPacket->nTxSize = 0;
            pPacket->pTxData = NULL;
            pPacket->nRxSize = OW_EEPROM_CMD_SUCCESS_INDICATOR_PKT_SIZE;
            pPacket->pRxData = CrcBuffer;

        }

        EepromFrame.Device = Device;
        EepromFrame.pHandler = &OwTransferHandler_Tx;

        Status = OwFrameTransfer(&EepromFrame);

        TM_Hook(HOOK_ONEWIRE_WRITEFAIL, &EepromFrame.Packets[0]);


        if (OW_EEPROM_CS_SUCCESS != CrcBuffer[0])
        {
            Status = OW_EEP_STATUS_ACCESS_DENIED;
            Log(ERR, "L3_OneWireEepromWrite: Memory block is write protected or Authentication required");
        }

    } while(false);


    return Status;
}

/* ========================================================================== */
/**
 * \brief   Erases 1 Wire EEPROM device
 *
 * \details This function Erases content in the 1-Wire EEPROM Memory.
 *
 * \param   Device - 64 Bit 1-Wire ROM ID.
 *
 * \return  OW_EEP_STATUS - Status
 *
 * ========================================================================== */
OW_EEP_STATUS L3_OneWireEepromErase(ONEWIRE_DEVICE_ID Device)
{
    OW_EEP_STATUS Status;                               /* Variable to hold the Eeprom return status value */
    uint8_t PageNum;                                    /* Variable to hold the Page number. */
    uint8_t EraseBuffer[OW_EEPROM_MEMORY_PAGE_SIZE];    /* Buffer to hold the Value used for Erase operation.*/

    Status = OW_EEP_STATUS_OK;
    PageNum = 0;

    /* Updating the Buffer with 0xFF value which is the value used to write for Erase operation */
    memset (EraseBuffer, 0xFF, OW_EEPROM_MEMORY_PAGE_SIZE);

    do
    {
        Status = L3_OneWireEepromWrite(Device, PageNum, EraseBuffer);
        if (OW_EEP_STATUS_OK != Status)
        {
            Log(ERR, "L3_OneWireEepromErase: Eeprom Erase failed");
            break;
        }

        PageNum++;

        Status = L3_OneWireEepromWrite(Device, PageNum, EraseBuffer);

    } while(false);

    return Status;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

