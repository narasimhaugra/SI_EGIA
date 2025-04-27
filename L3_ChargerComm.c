#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Charger
 * \{
 *
 * \brief   Cartridge Definition functions
 *
 * \details The Cartridge Definition defines all the interfaces used for
            communication between Handle and Cartridge.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_ChargerComm.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_ChargerComm.h"
#include "L3_Spi.h"
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
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_CHARGER)     ///< Log Group Identifier
#define MAX_CHARGER_RESPONSE_SIZE   (20u)                   ///< Charger response size
#define CHRG_PACKET_DATA_SIZE       (4u)                    ///< Max data size
#define CHRG_PACKET_FRAME_SIZE      (6u)                    ///< Packet size exculding data
#define CHRG_PACKET_BUFFER          (5)
#define CHRG_PACKET_DATA_OFFSET     (4u)
#define CHRG_PACKET_START           (0xAAu)
#define CHRG_PACKET_SIZE_LSB        (0x07u)
#define CHRG_PACKET_SIZE_MSB        (0x00u)
#define CHRG_PACKET_DATA            (0x00u)
#define CHRG_PACKET_DATA_REQSMBUS   (0x01u)
#define CHRG_PACKET_DATA_RELSMBUS   (0x00u)
#define CHRG_MESSAGE_OVERHEAD       (6u)      ///< message payload except data
#define CHGR_CMD_COOLOFF_DURATION   (20u)     ///< Cool off period between consecutive commands.

#define CHGR_CMD_PING                          0x01u
#define CHGR_CMD_GET_VERSION                   0x02u
#define CHGR_CMD_BLOB_DATA_SETUP               0x03u
#define CHGR_CMD_BLOB_DATA_PACKET              0x04u
#define CHGR_CMD_BLOB_DATA_VALIDATE            0x05u
#define CHGR_CMD_ERASE_HANDLE_TIMESTAMP        0x06u
#define CHGR_CMD_ERASE_HANDLE_BL_TIMESTAMP     0x07u
#define CHGR_CMD_SLEEPING                      0x08u
#define CHGR_CMD_BEGIN_CHARGING                0x09u
#define CHGR_CMD_STOP_CHARGING                 0x0Au
#define CHGR_CMD_CHARGE_AT_THRESHOLD           0x0Bu
#define CHGR_CMD_CHARGE_AT_100                 0x0Cu
#define CHGR_CMD_AUTHENTICATE                  0x0Du
#define CHGR_CMD_SHUT_DOWN_NOW                 0x0Eu
#define CHGR_CMD_REBOOT_NOW                    0x0Fu
#define CHGR_CMD_GET_CTS_DATA                  0x10u
#define CHGR_CMD_ERROR_CODE                    0x11u
#define CHGR_CMD_FAST_POLLING_RATE             0x12u
#define CHGR_CMD_QUERY_NEEDED                  0x13u
#define CHGR_CMD_PP_IS_MASTER_SMB              0x14u
#define CHGR_CMD_CHARGER_COMMUNICATIONS_ERROR  0x15u



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
/* ========================================================================== */
/**
 * \brief   Send command packet and receive response
 *
 * \details This function sends communication packet to charger and receive
 *          response. The function also strips out the initial padding bytes,
 *          and validates the response packet. If valid, then extracts data
 *          from the packet to copy to the user poiner.
 *
 * \param   pOutPacket - pointer to Communication packet to be sent out
 *          OutPacketSize - Size of the packet to be sentout
 *          pInData - pointer to Buffer to store data from response packet
 *
 * \return  CHARGER_COMM_STATUS - Status
 *
 * ========================================================================== */
static CHARGER_COMM_STATUS ChargerSendReceive(uint8_t *pOutPacket, uint8_t OutPacketSize, uint8_t *pInData)
{
    uint16_t PacketLen;         // Variable to hold received packet length
    uint16_t ComputedCrc;       // Computed CRC, use for both outgoing and incoming packet
    uint16_t ReceivedCrc;       // CRC from received packet
    CHARGER_COMM_STATUS Status; // Function return status
    uint8_t PacketStart;        // Position of valid start by in the received packet
    uint8_t Response[MAX_CHARGER_RESPONSE_SIZE];    // Received response packet

    do
    {
        OSTimeDly(CHGR_CMD_COOLOFF_DURATION);   // Force a cool off period between consecutive commands.
        Status = CHARGER_COMM_STATUS_ERROR;

        /* Calculate CRC here */
        ComputedCrc = CRC16(0, pOutPacket, OutPacketSize - sizeof(ComputedCrc));
        memcpy(&pOutPacket[OutPacketSize - sizeof(ComputedCrc)], &ComputedCrc, sizeof(ComputedCrc));

        /* Clear response buffer */
        memset(Response, 0, sizeof(Response));
        /* Send command and receive response */
        if (SPI_STATUS_OK != L3_SpiTransfer(SPI_DEVICE_CHARGER, pOutPacket, OutPacketSize, Response, MAX_CHARGER_RESPONSE_SIZE))
        {
            Status = CHARGER_COMM_STATUS_ERROR;
            break;
        }

        /* The response may have some padding bytes; look for a valid start */
        PacketStart = 0;
        while((PacketStart < sizeof(Response)) && (CHRG_PACKET_START != Response[PacketStart]))
        {
            PacketStart++;
        }
        /// \todo 01/06/2022 KIA: Needs a design review to confirm logic in line 142, 152
        /* Valid start, and complete packet found ?*/
        if(PacketStart >= MAX_CHARGER_RESPONSE_SIZE - 2)
        {
            /// \todo 01/06/2022 KIA: Examine why code below is commented out
            /* Flush out the (possibly)remaning data in slave device */
            // memset(DummyTxData, 0, MAX_CHARGER_RESPONSE_SIZE);
            // L3_SpiTransfer(SPI_DEVICE_CHARGER, pOutPacket, OutPacketSize, Response, MAX_CHARGER_RESPONSE_SIZE);
            break;
        }

        /* Response packet integrity check */
        PacketLen =  Response[PacketStart + 1] | (Response[PacketStart + 2] << 8) ;
        ComputedCrc =  CRC16(0, (uint8_t *)&Response[PacketStart], PacketLen - 2);
        ReceivedCrc = Response[PacketStart + PacketLen-2] | ( Response[PacketStart + PacketLen-1] << 8);

        TM_Hook(HOOK_SPICRCFAIL, &ComputedCrc);

        if (ComputedCrc != ReceivedCrc)
        {
            /* CRC match failed, return with error */
            Status = CHARGER_COMM_STATUS_COM_ERROR;
            break;
        }

        /* Copy payload if caller requested for it */
        if (NULL != pInData)
        {
            memcpy(pInData, (void*)&Response[PacketStart + CHRG_PACKET_DATA_OFFSET], PacketLen - CHRG_PACKET_FRAME_SIZE);
        }

        Status = CHARGER_COMM_STATUS_OK;
    }while (false);

    return Status;
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Send ping command to charger
 *
 * \details This function sends PING command to charger and process response.
 *
 * \param   pDeviceType - pointer to Device type returned by the charger
 *
 * \return  CHARGER_COMM_STATUS - Status
 *
 * ========================================================================== */
CHARGER_COMM_STATUS L3_ChargerCommPing(uint8_t *pDeviceType)
{
    CHARGER_COMM_STATUS Status;
    uint8_t CrcLsb = 0x00;
    uint8_t CrcMsb = 0x00;
    uint8_t ChargerCmdPing[] = {CHRG_PACKET_START, CHRG_PACKET_SIZE_LSB, CHRG_PACKET_SIZE_MSB, CHGR_CMD_PING, CHRG_PACKET_DATA, CrcLsb, CrcMsb};

    Status = ChargerSendReceive(ChargerCmdPing, CHRG_PACKET_FRAME_SIZE+1, pDeviceType);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Request charger version
 *
 * \details This function sends version command to charger and process response.
 *
 * \param   pChargerVersion - pointer to Charger version
 *
 * \return  CHARGER_COMM_STATUS - Status
 *
 * ========================================================================== */
CHARGER_COMM_STATUS L3_ChargerCommGetVersion(uint16_t *pChargerVersion)
{
    CHARGER_COMM_STATUS Status;
    uint8_t CrcLsb = 0x00;
    uint8_t CrcMsb = 0x00;
    uint8_t ChargerCmdPing[] = {CHRG_PACKET_START, CHRG_PACKET_SIZE_LSB, CHRG_PACKET_SIZE_MSB, CHGR_CMD_GET_VERSION, CHRG_PACKET_DATA, CrcLsb, CrcMsb};
    uint8_t Response[MAX_CHARGER_RESPONSE_SIZE];

    Status = ChargerSendReceive(ChargerCmdPing, CHRG_PACKET_FRAME_SIZE+1, Response);

    if ((NULL != pChargerVersion) && (CHARGER_COMM_STATUS_OK == Status))
    {
        memcpy(pChargerVersion, Response, sizeof(uint16_t));
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Send begin charging command
 *
 * \details This function sends begin charging command to charger and process response.
 *
 * \param   < None >
 *
 * \return  CHARGER_COMM_STATUS - Status
 *
 * ========================================================================== */
CHARGER_COMM_STATUS L3_ChargerCommStartCharging(void)
{
    CHARGER_COMM_STATUS Status;
    uint8_t CrcLsb = 0x1F;
    uint8_t CrcMsb = 0x3C;
    uint8_t ChargerCmdPing[] = {CHRG_PACKET_START, CHRG_PACKET_SIZE_LSB, CHRG_PACKET_SIZE_MSB, CHGR_CMD_BEGIN_CHARGING, CHRG_PACKET_DATA, CrcLsb, CrcMsb};
    uint8_t Response[MAX_CHARGER_RESPONSE_SIZE];

    Status = ChargerSendReceive(ChargerCmdPing, CHRG_PACKET_FRAME_SIZE+1, Response);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Send stop charging command
 *
 * \details This function sends stop charging command to charger and process response.
 *
 * \param   < None >
 *
 * \return  CHARGER_COMM_STATUS - Status
 *
 * ========================================================================== */
CHARGER_COMM_STATUS L3_ChargerCommStopCharging(void)
{
    CHARGER_COMM_STATUS Status;
    uint8_t CrcLsb = 0x1F;
    uint8_t CrcMsb = 0xCC;
    uint8_t ChargerCmdPing[] = {CHRG_PACKET_START, CHRG_PACKET_SIZE_LSB, CHRG_PACKET_SIZE_MSB, CHGR_CMD_STOP_CHARGING, CHRG_PACKET_DATA, CrcLsb, CrcMsb};
    uint8_t Response[MAX_CHARGER_RESPONSE_SIZE];

    Status = ChargerSendReceive(ChargerCmdPing, CHRG_PACKET_FRAME_SIZE+1, Response);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Send charger authentication status
 *
 * \details This function sends charger authentication stuatus to charger.
 *
 * \param   Authenticated - Charger is authenticated if true.
 *
 * \return  CHARGER_COMM_STATUS - Status
 *
 * ========================================================================== */
CHARGER_COMM_STATUS L3_ChargerCommSetAuthResult(bool Authenticated)
{
    CHARGER_COMM_STATUS Status;
    uint8_t CrcLsb = 0x1D;
    uint8_t CrcMsb = 0xFC;
    uint8_t ChargerCmdPing[] = {CHRG_PACKET_START, CHRG_PACKET_SIZE_LSB, CHRG_PACKET_SIZE_MSB, CHGR_CMD_AUTHENTICATE, CHRG_PACKET_DATA, CrcLsb, CrcMsb};
    uint8_t Response[MAX_CHARGER_RESPONSE_SIZE];

    if(Authenticated)
    {
        ChargerCmdPing[4] = Authenticated;
        ChargerCmdPing[5] = 0xdc;
        ChargerCmdPing[6] = 0x3c;
    }

    Status = ChargerSendReceive(ChargerCmdPing, CHRG_PACKET_FRAME_SIZE+1, Response);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Send reboot command
 *
 * \details This function sends reboot command to charger and process response.
 *
 * \param   < None >
 *
 * \return  CHARGER_COMM_STATUS - Status
 *
 * ========================================================================== */
CHARGER_COMM_STATUS L3_ChargerCommReboot(void)
{
    CHARGER_COMM_STATUS Status;
    uint8_t CrcLsb = 0x1C;
    uint8_t CrcMsb = 0x9C;
    uint8_t ChargerCmdPing[] = {CHRG_PACKET_START, CHRG_PACKET_SIZE_LSB, CHRG_PACKET_SIZE_MSB, CHGR_CMD_REBOOT_NOW, CHRG_PACKET_DATA, CrcLsb, CrcMsb };
    uint8_t Response[MAX_CHARGER_RESPONSE_SIZE];

    Status = ChargerSendReceive(ChargerCmdPing, CHRG_PACKET_FRAME_SIZE+1, Response);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Request charger error code
 *
 * \details This function sends command to charger to request error code.
 *
 * \param   pErrorCode - pointer to Error code returned by charger
 *
 * \return  CHARGER_COMM_STATUS - Status
 *
 * ========================================================================== */
CHARGER_COMM_STATUS L3_ChargerCommGetError(int16_t *pErrorCode)
{
    CHARGER_COMM_STATUS Status;
    uint8_t CrcLsb = 0x15;
    uint8_t CrcMsb = 0x3C;

    uint8_t ChargerCmdPing[] = {CHRG_PACKET_START, CHRG_PACKET_SIZE_LSB, CHRG_PACKET_SIZE_MSB, CHGR_CMD_ERROR_CODE, CHRG_PACKET_DATA, CrcLsb, CrcMsb};
    uint8_t Response[MAX_CHARGER_RESPONSE_SIZE];

    Status = ChargerSendReceive(ChargerCmdPing, CHRG_PACKET_FRAME_SIZE+1, Response);

    if (CHARGER_COMM_STATUS_OK == Status)
    {
        *pErrorCode = Response[CHRG_PACKET_DATA_OFFSET] << 8 | Response[CHRG_PACKET_DATA_OFFSET+1] ;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Send power pack master set command
 *
 * \details This function sends command to set power pack as master of SM Bus to
 *          charger and process response.
 *
 * \param   < None >
 *
 * \return  CHARGER_COMM_STATUS - Status
 *
 * ========================================================================== */
CHARGER_COMM_STATUS L3_ChargerCommSetPowerPackMaster(void)
{
    CHARGER_COMM_STATUS Status;
    uint8_t CrcLsb;
    uint8_t CrcMsb;

    CrcLsb = 0x16;
    CrcMsb = 0x6C;
    uint8_t ChargerCmdSetPPMaster[] = {CHRG_PACKET_START, CHRG_PACKET_SIZE_LSB, CHRG_PACKET_SIZE_MSB, CHGR_CMD_PP_IS_MASTER_SMB, CHRG_PACKET_DATA_REQSMBUS, CrcLsb, CrcMsb};
    uint8_t Response[MAX_CHARGER_RESPONSE_SIZE];

    Status = ChargerSendReceive(ChargerCmdSetPPMaster, CHRG_PACKET_FRAME_SIZE+1, Response);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Send power pack master release command
 *
 * \details This function sends command to Release power pack as master of SM Bus to
 *          charger and process response.
 *
 * \param   < None >
 *
 * \return  CHARGER_COMM_STATUS - Status
 *
 * ========================================================================== */
CHARGER_COMM_STATUS L3_ChargerCommRelPowerPackMaster(void)
{
    CHARGER_COMM_STATUS Status;
    uint8_t CrcLsb;
    uint8_t CrcMsb;

    CrcLsb = 0x16;
    CrcMsb = 0x6C;
    uint8_t ChargerCmdSetPPMaster[] = {CHRG_PACKET_START, CHRG_PACKET_SIZE_LSB, CHRG_PACKET_SIZE_MSB, CHGR_CMD_PP_IS_MASTER_SMB, CHRG_PACKET_DATA_RELSMBUS, CrcLsb, CrcMsb};
    uint8_t Response[MAX_CHARGER_RESPONSE_SIZE];

    Status = ChargerSendReceive(ChargerCmdSetPPMaster, CHRG_PACKET_FRAME_SIZE+1, Response);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Send Query command to charger
 *
 * \details This function sends query command to charger.This charger responds to
 *          this command by sending the CTS command to query handle serial number.
 *
 * \param   < None >
 *
 * \return  CHARGER_COMM_STATUS - Status
 *
 * ========================================================================== */
CHARGER_COMM_STATUS L3_ChargerCommQuery(void)
{
    uint8_t Response[MAX_CHARGER_RESPONSE_SIZE];
    uint8_t ChargerCmdQuery[CHRG_PACKET_FRAME_SIZE+1];
    CHARGER_COMM_STATUS Status;
    uint8_t Index;

    Index=0;

    ChargerCmdQuery[Index++] = CHRG_PACKET_START;
    ChargerCmdQuery[Index++] = CHRG_PACKET_SIZE_LSB;
    ChargerCmdQuery[Index++] = CHRG_PACKET_SIZE_MSB;
    ChargerCmdQuery[Index++] = CHGR_CMD_QUERY_NEEDED;
    ChargerCmdQuery[Index++] = CHRG_PACKET_DATA;

    //CRC calculated with an external crc generator.
    //Since the command frame is not supposed to change using static CRC values
    //improves code execution time.
    ChargerCmdQuery[Index++] = 0x14; //CRC LSB value
    ChargerCmdQuery[Index] = 0x5C;   //CRC MSB Value

    Status = ChargerSendReceive(ChargerCmdQuery , Index, Response);

    return Status;
}
/* ========================================================================== */
/**
 * \brief   Send CTS response
 *
 * \details This function sends response to CTS command to charger. For response details
 *          check CTSData structure.
 *
 * \param   < None >
 *
 * \return  CHARGER_COMM_STATUS - Status
 *
 * ========================================================================== */
CHARGER_COMM_STATUS L3_ChargerCommSendCTSResponse(uint8_t *pCtsData, uint8_t CtsDataSize)
{
    uint8_t ChargerCmdQuery[CTS_DATA_SIZE + CHRG_MESSAGE_OVERHEAD];
    uint8_t Response[MAX_CHARGER_RESPONSE_SIZE];
    uint8_t Index;
    uint16_t CRC;
    CHARGER_COMM_STATUS Status;

    //Assemble the message packet
    Index = 0;
    ChargerCmdQuery[Index++] = CHRG_PACKET_START;
    ChargerCmdQuery[Index++] = CtsDataSize + CHRG_MESSAGE_OVERHEAD;
    ChargerCmdQuery[Index++] = CHRG_PACKET_SIZE_MSB;
    ChargerCmdQuery[Index++] = CHGR_CMD_GET_CTS_DATA;

    //get the data part in message array
    memcpy(&ChargerCmdQuery[Index], pCtsData, CtsDataSize);

    //calculate CRC for the message frame
    CRC = CRC16(0,ChargerCmdQuery,(CtsDataSize + CHRG_MESSAGE_OVERHEAD - sizeof(CRC)));

    //calculate the CRC position in message frame
    Index = CtsDataSize + CHRG_MESSAGE_OVERHEAD - sizeof(CRC);

    ChargerCmdQuery[Index++] = CRC & 0xFF; // get LSB
    ChargerCmdQuery[Index] = CRC >> 8;   // Get MSB


    Status = ChargerSendReceive(ChargerCmdQuery, CHRG_PACKET_FRAME_SIZE+CtsDataSize, Response);

    return Status;
}
/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

