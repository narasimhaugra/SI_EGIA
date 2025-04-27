#ifdef __cplusplus  /* Header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L4_BlobHandler
 * \{
 *
 * \brief   This module contains functions to support Blob Handling
 *
 * \details The Blob Handler interfaces to other L4 components and higher
 *          layer applications through APIs. The Blob Handler accesses
 *          the Non Volatile SD Card using the low layer file system wrapper
 *          interfaces.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_BlobHandler.h"
#include "FileSys.h"
#include "L2_Flash.h"
#include "Aes.h"
#include "NoInitRam.h"
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
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_BLOB)     ///< Log Group Identifier
#define K20_FLASH_SECTOR_SIZE       (0x1000)
#define FILE_DATA_MAX_SIZE          (512u)               ///< Maximum Data size to access File
#define BLOB_FILE_NAME              ("\\BlobFile")       ///< Fixed Blob File name in SD Card
/// \todo 03/12/2021 CPK remove BLOB_FILE_NAME_TEMP after Validation - retained for debug purposes
#define BLOB_FILE_NAME_TEMP         ("\\TempBlobFile")   ///< Temporary Blob File name in SD Card used for testing
#define BAD_BLOB_STRING             ("FF.F.F")           ///< Bad blob string to be returned for in valid blobs
#define FILE_WRITE_MAX_SIZE         (1024u)              ///< Max size for Blob Write
#define BLOB_MUTEX_TIMEOUT          (0xFFFFFu)           ///< Timeout for Blob Mutex
#define BLOB_VERSION_1              (1u)                 ///< Blob Version-1
#define BLOB_VERSION_2              (2u)                 ///< Blob Version-2
#define LOOPCOUNTER_200             (200u)               ///< Loop delay, currently set to 100KB (200*512)

#define FLASHPROGRAM_AREA2_END                  (0x1007D000u)
#define FLASHMEMORY_CRCCHECK_CHUNKSIZE          (256u)              ///< Flash Memory chunk to test in Kilo Byte

#define ROUNDUPPOWEROF2(Number,Multiple)     ( ((Number)+((Multiple)-1)) & (~((Multiple)-1)) )
#define ROUNDDOWNPOWEROF2(Number,Multiple)   ( (Number) & (~((Multiple)-1)) )

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                            ///<  Flash Section Validation States
{
    FLASH_CRCCHECK_BOOTLOADER,           ///<  Flash Validate BOOTLOADER
    FLASH_CRCCHECK_MAINAPP,              ///<  Flash Validate MAINAPP
    FLASH_CRCCHECK_BLOB,                 ///<  FLASH Validate BLOB
    FLASH_CRCCCHECK_END                  ///<  END OF FLASH VALIDATE
} FLASH_CRCCHECK_STATE;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
//#pragma location=".sram"
static BLOB_POINTERS        BlobPointers;                  ///< Structure that holds Blob information

static OS_EVENT *pMutexBlobHandler = NULL;                 ///< Mutex used to lock Blob access
static FS_ENTRY_INFO       BlobFileAttrib;                 ///< Structure to hold Blob file attributes
static ACTIVE_VERSION      ActiveVersionsStructVer = ACTIVE_VERSION_INVALID;

#pragma location=".sram"
static uint8_t             fileData[FILE_DATA_MAX_SIZE + (4*IV_OFFSET)]; //get memory for file write.Secure enough memory for IV required for CBC as well

#pragma location=".sram"
static uint8_t TempBuffer[3*AES_BLOCKLEN];
/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static BLOB_HANDLER_STATUS ReadBlobFilePointers(void);
static BLOB_HANDLER_STATUS ReadBlobPointerData(FS_FILE *pBlobFile, uint32_t FileOffset,uint8_t *pData,uint32_t BytesToRead, uint32_t *pBytesRead);
static BLOB_HANDLER_STATUS GetBlobSectionOffset(BLOB_SECTION BlobSection,int32_t Offset, uint32_t DataSize,uint32_t *pBytesToRead, uint32_t *pFileOffset);
//static bool ProcessGetTimestampCmd(BLOB_GETINFO_PARAM ParamId, uint32_t *pSectionTimestamp);
static BLOB_HANDLER_STATUS L4_UpdateFlashActiveVersion(void);
#ifdef PRINT_BLOB_POINTERS /*debug purpose */
static BLOB_HANDLER_STATUS PrintBlobPointers (BLOB_POINTERS *pBlobPointers);
#endif
static bool FlashFinalCrcCalculation( uint32_t TotalFlashCodesize, uint32_t HandleLowAddress, CRC_INFO *pCrcHanlde);
static bool ValidateFlashForTwoSections(CRC_INFO *pCrcHandle, ACTIVEVERSION_2 *pActiveVersions);


/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Reads the Blob pointers from SD Card BLobfile.
 *
 * \details This function reads the blob file and initializes all blob pointers.
 *          These blob pointers are used within the blob handler and also can be copied
 *          by the caller functions using L4_GetBlobPointers
 *
 * \param   < None >
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
static BLOB_HANDLER_STATUS ReadBlobFilePointers(void)
{
    BLOB_HANDLER_STATUS Status; /* Error status of the function */
    FS_ERR   FsError;           /* Contains the File System error status */
    uint8_t    OsErr;           /* Contains the OS error status */
    uint32_t   FileOffset;      /* File Offset used to traverse the file */
    uint32_t   BytesRead;       /* Number of bytes read as returned by File system calls*/
    FS_FILE *pBlobFile;         /* Blob file Pointer used to access the file */

    /* Initialize Variables */
    Status = BLOB_STATUS_ERROR;
    BytesRead = 0;
    pBlobFile = NULL;
    FileOffset = 0x0;

    do
    {
        /* Mutex Lock */
        OSMutexPend ( pMutexBlobHandler, OS_WAIT_FOREVER, &OsErr );

        FsError = FsOpen ( &pBlobFile, BLOB_FILE_NAME, FS_FILE_ACCESS_MODE_RD );
        if ( FS_ERR_NONE != FsError )
        {
            Log(ERR, "ReadBlobFilePointers: FsOpen Error %d", FsError);
            break;
        }

        FsError = FsGetInfo ( BLOB_FILE_NAME, &BlobFileAttrib );
        if ( FS_ERR_NONE != FsError )
        {
            Log (ERR, "ReadBlobFilePointers: FsGetInfo Error %d", FsError);
            break;
        }

        FsError = FsSeek ( pBlobFile, 0, FS_SEEK_SET );
        if ( FS_ERR_NONE != FsError )
        {
            Log (ERR, "ReadBlobFilePointers: FsSeek Error %d", FsError);
            break;
        }

        /* Read Blob Header from Blob file */
        Status = ReadBlobPointerData ( pBlobFile, FileOffset, (uint8_t *)&BlobPointers.StoredBlobHeader, sizeof (BLOB_HEADER), &BytesRead );
        if ( BLOB_STATUS_OK != Status )
        {
            break;
        }

        if ( BlobPointers.StoredBlobHeader.BlobVersion <= 3 )
        {   // version 3 and below don't support encryption
            BlobPointers.StoredBlobHeader.Encryption.Status = 0;
        }

        /* Move to just past the blob's Header */
        FileOffset = BlobPointers.StoredBlobHeader.BlobHeaderSize;

        /* Read Handle Header from Blob file */
        Status = ReadBlobPointerData ( pBlobFile, FileOffset,(uint8_t *)&BlobPointers.StoredHandleHeader, sizeof (BINARY_HEADER), &BytesRead );
        if ( BLOB_STATUS_OK != Status )
        {
            break;
        }
        else if( BlobPointers.StoredBlobHeader.Encryption.bit.HandleEncrypted )
        {
             // decrypt handle header here
             DecryptBinaryBuffer ( (uint8_t *)&BlobPointers.StoredHandleHeader, sizeof (BINARY_HEADER), false );
        }

        /* Assign Handle main Offset */
        BlobPointers.HandleMainOffset = FileOffset + BlobPointers.StoredHandleHeader.HeaderSize;
        FileOffset += BlobPointers.StoredBlobHeader.HandleDataSize;

        /* Read Blob Bootloader Header from Blob file */
        Status = ReadBlobPointerData ( pBlobFile, FileOffset,(uint8_t *)&BlobPointers.StoredHandleBLHeader, sizeof (BINARY_HEADER), &BytesRead );
        if ( BLOB_STATUS_OK != Status )
        {
            break;
        }
        else if ( BlobPointers.StoredBlobHeader.Encryption.bit.HandleBLEncrypted )
        {
             DecryptBinaryBuffer ( (uint8_t *)&BlobPointers.StoredHandleBLHeader, sizeof (BINARY_HEADER), false );
        }
        /* Assign Handle bootloader Offset */
        BlobPointers.HandleBootloaderOffset = FileOffset + BlobPointers.StoredHandleBLHeader.HeaderSize;
        FileOffset += BlobPointers.StoredBlobHeader.HandleBLDataSize;

        /* Read JED Info from Blob file */
        Status = ReadBlobPointerData ( pBlobFile, FileOffset, (uint8_t *)(&BlobPointers.JedInfo), sizeof (MACHX02), &BytesRead );
        if ( BLOB_STATUS_OK != Status )
        {
            break;
        }

        /* Assign JED Data Offset */
        BlobPointers.JedDataOffset = FileOffset + sizeof (MACHX02);
        FileOffset += sizeof (MACHX02) + BlobPointers.StoredBlobHeader.JedDataSize;

        /* Read Adapter Bootloader header from Blob file */
        Status = ReadBlobPointerData ( pBlobFile, FileOffset,(uint8_t *)&BlobPointers.StoredAdaptBLHeader, sizeof (BINARY_HEADER), &BytesRead );
        if ( BLOB_STATUS_OK != Status )
        {
            break;
        }
        else if ( BlobPointers.StoredBlobHeader.Encryption.bit.AdapterBLEncrypted )
        {
             DecryptBinaryBuffer ( (uint8_t *)&BlobPointers.StoredAdaptBLHeader, sizeof (BINARY_HEADER), false );
        }

        /* Assign Adapter Bootloader Offset in Blob Pointer*/
        BlobPointers.AdapterBootloaderOffset = FileOffset + BlobPointers.StoredAdaptBLHeader.HeaderSize;
        FileOffset += BlobPointers.StoredBlobHeader.AdaptBLDataSize;

        /* Read EGIA Header from Blob file */
        Status = ReadBlobPointerData (pBlobFile, FileOffset, (uint8_t *)&BlobPointers.StoredEgiaHeader, sizeof (BINARY_HEADER), &BytesRead );
        if ( BLOB_STATUS_OK != Status )
        {
            break;
        }
        else if ( BlobPointers.StoredBlobHeader.Encryption.bit.EGIAEncrypted )
        {
             DecryptBinaryBuffer ( (uint8_t *)&BlobPointers.StoredEgiaHeader, sizeof (BINARY_HEADER), false );
        }

        /// \todo 03/11/2021 CPK modify blob header to permit any number of adapter types. Will make it easy to add e.g. TS 3.0 adapter.
        /* Assign EGIA main Offset */
        BlobPointers.EgiaMainOffset = FileOffset + BlobPointers.StoredEgiaHeader.HeaderSize;
        FileOffset += BlobPointers.StoredBlobHeader.EgiaDataSize;

        Status = ReadBlobPointerData ( pBlobFile, FileOffset,(uint8_t *)&BlobPointers.StoredEeaHeader, sizeof (BINARY_HEADER), &BytesRead );
        if ( BLOB_STATUS_OK != Status )
        {
            break;
        }
        else if ( BlobPointers.StoredBlobHeader.Encryption.bit.EEAEncrypted )
        {
             DecryptBinaryBuffer ( (uint8_t *)&BlobPointers.StoredEeaHeader, sizeof (BINARY_HEADER), false );
        }

        /* Assign EEA main Offset */
        BlobPointers.EeaMainOffset = FileOffset + BlobPointers.StoredEeaHeader.HeaderSize;
        FileOffset += BlobPointers.StoredBlobHeader.EeaDataSize;

        FsError = FsSeek (pBlobFile, FileOffset, FS_SEEK_SET);  // Skip over the Eea Adapter main app
        if ( FS_ERR_NONE != FsError )
        {
            Log (ERR, "ReadBlobFilePointers: FsSeek Error %d", FsError);

            break;
        }

        Status = BLOB_STATUS_OK;
    } while ( false );

    FsError = FsClose (pBlobFile);
    OSMutexPost (pMutexBlobHandler);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Reads the Blob pointers at the offset specified from SD Card Blobfile.
 *
 * \details This function reads the blob file at the specified offset.The caller
 *          function provides the FileOffet and the pointer to copy the data.
 *
 * \param   pBlobFile  - pointer to Blob file
 * \param   FileOffset -  Offset to which the Blob file pointer should position
 * \param   pData      - pointer to Blob Data where data is to be read
 * \param   BytesToRead - Number of bytes to read
 * \param   pBytesRead - pointer to Bytes Read by the File sytem call
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
static BLOB_HANDLER_STATUS ReadBlobPointerData(FS_FILE *pBlobFile, uint32_t FileOffset, uint8_t *pData, uint32_t BytesToRead, uint32_t *pBytesRead)
{
    FS_ERR   FsError;              /* contains the File System error status */
    BLOB_HANDLER_STATUS Status;    /* Error status of the function */
    do
    {
        FsError = FsSeek (pBlobFile, FileOffset, FS_SEEK_SET);
        if ( FS_ERR_NONE != FsError )
        {
            Log (ERR, "ReadBlobPointerData: FsSeek Error %d", FsError);

            Status = BLOB_STATUS_ERROR;
            break;
        }

        FsError = FsRead (pBlobFile, (uint8_t *)pData, BytesToRead, pBytesRead);
        if ( pBytesRead == 0 )
        {
            Log (ERR, "ReadBlobPointerData: FsRead Error", FsError);

            Status = BLOB_STATUS_ERROR;
            break;
        }
        if ( FS_ERR_NONE != FsError )
        {
            Log (ERR, "ReadBlobPointerData: FsRead Error %d Bytes read %d", FsError,*pBytesRead);

            Status = BLOB_STATUS_ERROR;
            break;
        }

        Status = BLOB_STATUS_OK;
    } while ( false );

    return Status;
}

/* ========================================================================== */
/**
 * \brief   This is a helper function to read the Offset and size of a Blob section.
 *
 * \details This function reads the blob file at the specified offset.The caller
 *          function provides the FileOffet and the pointer to copy the data.
 *
 * \param   BlobSection  - Blob file
 * \param   Offset -  Offset to which the Blob file pointer should position
 * \param   DataSize   - Number of bytes to be written to Blob file
 * \param   pBytesToRead - pointer to Number of bytes to read
 * \param   pFileOffset - pointer to Bytes Read by the File sytem call
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
static BLOB_HANDLER_STATUS GetBlobSectionOffset(BLOB_SECTION BlobSection, int32_t Offset, uint32_t DataSize, uint32_t *pBytesToRead, uint32_t *pFileOffset)
{
    //BLOB_HANDLER_STATUS Status;
    uint32_t SectionDataSize;

    switch ( BlobSection )
    {
        /*  Below case statements calculate the bytes to be read and the blob file section offset of the respective BlobSections */
        case BLOB_SECTION_HANDLE_BL:
            SectionDataSize = BlobPointers.StoredBlobHeader.HandleBLDataSize;
            *pFileOffset = BlobPointers.HandleBootloaderOffset;
            break;

        case BLOB_SECTION_HANDLE_MAIN:
            SectionDataSize = BlobPointers.StoredBlobHeader.HandleDataSize;
            *pFileOffset = BlobPointers.HandleMainOffset;
            break;

        case BLOB_SECTION_JED_CONFIG:
            SectionDataSize =BlobPointers.JedInfo.FuseDataSize;
            *pFileOffset = BlobPointers.JedDataOffset;
            break;

        case BLOB_SECTION_JED_UFM:
            SectionDataSize = BlobPointers.JedInfo.UfmDataSize;
            *pFileOffset = BlobPointers.JedDataOffset + BlobPointers.JedInfo.FuseDataSize;
            break;

        case BLOB_SECTION_ADAPT_BL:
            SectionDataSize = BlobPointers.StoredBlobHeader.AdaptBLDataSize;
            *pFileOffset = BlobPointers.AdapterBootloaderOffset;
            break;

        case BLOB_SECTION_EGIA_MAIN:
            SectionDataSize = BlobPointers.StoredBlobHeader.EgiaDataSize;
            *pFileOffset = BlobPointers.EgiaMainOffset;
            break;

         case BLOB_SECTION_EEA_MAIN:
             SectionDataSize = BlobPointers.StoredBlobHeader.EeaDataSize;
             *pFileOffset = BlobPointers.EeaMainOffset;
            break;

        /// \todo 01/06/2022 KIA: Examine if this 'default' be reached under normal conditions, or is it an error? If error, should not return BLOB_STATUS_OK.
        default:
            SectionDataSize = 0u;
            break;
    }

    if ( Offset + DataSize < SectionDataSize )
    {
        *pBytesToRead = DataSize;
    }
    else if (Offset < SectionDataSize)
    {
        *pBytesToRead = SectionDataSize - Offset;
    }
    else
    {
        /* empty else{} for code standard compliance */
    }
    return BLOB_STATUS_OK;
}

/* ========================================================================== */
/**
 * \brief   Returns Blob pointer Timestamp specific Blob information as requested by the caller
 *
 * \details This call is used to read the blob file for a requested information pointed
 *          by the ParamId
 *
 * \param   ParamId           - Parameter as defined in BLOB_GETINFO_PARAM requested by the caller.
 * \param   pSectionTimestamp - pointer to Timestamp as read from the Blobpointers.
 *
 * \return  bool - ProcessGetTimestampCmd return 'false for success inline with coding guidelines  '
 *
 * ========================================================================== */
static bool ProcessGetTimestampCmd(BLOB_GETINFO_PARAM ParamId, uint32_t *pSectionTimestamp)
{
    bool ReturnStatus;

    ReturnStatus = true;

    /* Switch for Timestamp command and return command status */
    switch ( ParamId )
    {
        case GETINFO_PARAM_BLOB_ACTIVE_PP_TIMESTAMP:
            ReturnStatus= false;
            *pSectionTimestamp = BlobPointers.ActiveVersion.HandleTimestamp;
            break;

        case GETINFO_PARAM_BLOB_ACTIVE_PP_BL_TIMESTAMP:
            ReturnStatus= false;
            *pSectionTimestamp = BlobPointers.ActiveVersion.HandleBLTimestamp;
            break;

        case GETINFO_PARAM_BLOB_ACTIVE_FPGA_TIMESTAMP:
             ReturnStatus= false;
            *pSectionTimestamp = BlobPointers.ActiveVersion.JedTimestamp;
             break;

        case GETINFO_PARAM_BLOB_TIMESTAMP:
             ReturnStatus= false;
            *pSectionTimestamp = BlobPointers.StoredBlobHeader.BlobTimestamp;
             break;

        case GETINFO_PARAM_BLOB_PP_TIMESTAMP:
             ReturnStatus= false;
            *pSectionTimestamp = BlobPointers.StoredBlobHeader.HandleTimestamp;
             break;

        case GETINFO_PARAM_BLOB_PP_BL_TIMESTAMP:
             ReturnStatus= false;
            *pSectionTimestamp = BlobPointers.StoredBlobHeader.HandleBLTimestamp;
             break;

        case GETINFO_PARAM_BLOB_FPGA_TIMESTAMP:
             ReturnStatus= false;
            *pSectionTimestamp = BlobPointers.StoredBlobHeader.JedTimestamp;
             break;

        case GETINFO_PARAM_BLOB_ADAPT_BOOT_TIMESTAMP:
             ReturnStatus= false;
            *pSectionTimestamp = BlobPointers.StoredBlobHeader.AdaptBLTimestamp;
             break;

        case GETINFO_PARAM_BLOB_ADAPT_EGIA_TIMESTAMP:
             ReturnStatus= false;
            *pSectionTimestamp = BlobPointers.StoredBlobHeader.EgiaTimestamp;
             break;

        case GETINFO_PARAM_BLOB_ADAPT_EEA_TIMESTAMP:
             ReturnStatus= false;
            *pSectionTimestamp = BlobPointers.StoredBlobHeader.EeaTimestamp;
             break;

        default:
            ReturnStatus= true;
            break;
    }
    return ReturnStatus;
}

/* ========================================================================== */
/**
 * \brief   Returns Blob pointer Revision specific Blob information as requested by the caller
 *
 * \details This call is used to read the blob file for a requested information pointed
 *          by the ParamId
 *
 * \param   ParamId         - Parameter as defined in BLOB_GETINFO_PARAM requested by the caller.
 * \param   pRevisionString - pointer to Revision string as read from the Blobpointers.
 *
 * \return  bool - ProcessGetTimestampCmd return 'false for success as per the coding guidelines
 *                 for functions returning bool
 *
 * ========================================================================== */
static bool ProcessGetRevisionCmd(BLOB_GETINFO_PARAM ParamId, uint8_t *pRevisionString)
{
    bool ReturnStatus;

    ReturnStatus = true;
    static uint8_t BadBlobVersionString[MAX_REV_STRING_LENGTH];

    strncpy ( (char *)BadBlobVersionString,(char *)BAD_BLOB_STRING, MAX_REV_STRING_LENGTH );

    /* Switch for Revision command and return command status */
    switch ( ParamId )
    {
        case GETINFO_PARAM_BLOB_AGILE_NUMBER:
            strncpy ( (char *)pRevisionString, (char *)BlobPointers.StoredBlobHeader.BlobAgileNumber, MAX_REV_STRING_LENGTH );
            ReturnStatus= false;
            break;

        case GETINFO_PARAM_BLOB_PP_REVISION:
            strncpy ( (char *)pRevisionString, (char *)BlobPointers.StoredBlobHeader.BlobPowerPackRev, MAX_REV_STRING_LENGTH );
            ReturnStatus= false;
            break;

        case GETINFO_PARAM_BLOB_PP_BL_REVISION:
            strncpy ( (char *)pRevisionString, (char *)BlobPointers.StoredBlobHeader.BlobPowerPackBLRev, MAX_REV_STRING_LENGTH );
            ReturnStatus= false;
            break;

        case GETINFO_PARAM_BLOB_FPGA_REVISION:
            strncpy ( (char *)pRevisionString, (char *)BlobPointers.StoredBlobHeader.BlobJedRev, MAX_REV_STRING_LENGTH );
            ReturnStatus = false;
            break;

        case GETINFO_PARAM_BLOB_ADAPT_BOOT_REVISION:
            strncpy ( (char *)pRevisionString, (char *)BlobPointers.StoredBlobHeader.BlobAdapterBLRev, MAX_REV_STRING_LENGTH );
             ReturnStatus = false;
            break;

        case GETINFO_PARAM_BLOB_ADAPT_EGIA_REVISION:
            strncpy ( (char *)pRevisionString, (char *)BlobPointers.StoredBlobHeader.BlobAdapterEGIARev, MAX_REV_STRING_LENGTH );
            ReturnStatus = false;
            break;

        case GETINFO_PARAM_BLOB_ADAPT_EEA_REVISION:
            strncpy ( (char *)pRevisionString, (char *)BlobPointers.StoredBlobHeader.BlobAdapterEEARev, MAX_REV_STRING_LENGTH );
            ReturnStatus = false;
            break;

        case GETINFO_PARAM_BLOB_SYS_VERSION:
            strncpy ( (char *)pRevisionString, (char *)BadBlobVersionString, MAX_REV_STRING_LENGTH );
            if ( BLOB_STATUS_VALIDATED == noInitRam.BlobValidationStatus )
            {
                strncpy ( (char *)pRevisionString, (char *)BlobPointers.StoredBlobHeader.BlobSystemVersion, MAX_REV_STRING_LENGTH );
            }
            ReturnStatus = false;
            break;

        default:
            ReturnStatus = true;
            break;
    }
    return ReturnStatus;
}
/* ========================================================================== */
/**
 * \brief   Returns specific Blob information as requested by the caller
 *
 * \details This call is used to read the blob file for a requested information pointed
 *          by the ParamId
 *
 * \param   ParamId       - Parameter as defined in BLOB_GETINFO_PARAM requested by the caller.
 * \param   pData         - pointer to Data
 * \param   pResponseSize - pointer to Size of the response
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
BLOB_HANDLER_STATUS L4_BlobGetInfo(BLOB_GETINFO_PARAM ParamId, uint8_t *pData, uint16_t *pResponseSize)
{
    BLOB_HANDLER_STATUS Status;
    INT16U ResponseSize;
    uint32_t BlobFlags;
    uint8_t RevisionString[MAX_REV_STRING_LENGTH];
    uint32_t SectionTimestamp;
    bool CommandProcessed;

    Status = BLOB_STATUS_ERROR;
    ResponseSize = 0;
    BlobFlags = 0x0;
    SectionTimestamp = 0x0;
    CommandProcessed=true;
    do
    {
        if ( NULL == pData )
        {
            Status = BLOB_STATUS_INVALID_PARAM;
            Log (ERR, "L4_BlobGetInfo: Null parameter(pData) ");

            break;
        }

        if ( NULL == pResponseSize )
        {
            Status = BLOB_STATUS_INVALID_PARAM;
            Log (ERR, "L4_BlobGetInfo: Null parameter(pResponseSize)");
            break;
        }

        if ( (ParamId <= GETINFO_PARAM_BLOB_INVALID) || (ParamId >= GETINFO_PARAM_BLOB_LAST) )
        {
            Status = BLOB_STATUS_INVALID_PARAM;
            break;
        }

        CommandProcessed = ProcessGetTimestampCmd ( ParamId, &SectionTimestamp );
        if ( !CommandProcessed )
        {
            memcpy(pData, &SectionTimestamp, sizeof (BlobPointers.ActiveVersion.HandleTimestamp));
            *pResponseSize = sizeof (uint32_t);
            Status = BLOB_STATUS_OK;
            break;
        }

        CommandProcessed = ProcessGetRevisionCmd ( ParamId, RevisionString );
        if ( !CommandProcessed )
        {
            if ( BlobPointers.StoredBlobHeader.BlobVersion >= BLOB_VERSION_2 )
            {
                strncpy ( (char *)pData, (char *) RevisionString, MAX_REV_STRING_LENGTH );
                ResponseSize = strlen ( (char *)pData ) + 1;
            }

            if ( 0 == ResponseSize )
            {
                *pData = NULL;
                ResponseSize = 1;
            }

            *pResponseSize = ResponseSize;
            Status = BLOB_STATUS_OK;
            break;
        }

        switch ( ParamId )
        {
            case GETINFO_PARAM_BLOB_FLAGS:
                if ( L4_BlobValidate (false) == BLOB_STATUS_VALIDATED )
                {
                    BlobFlags |= DEVICE_PROPERTIES_MASK_BLOB_VALID;
                }

                memcpy ( pData, &BlobFlags, sizeof(BlobFlags) );
                ResponseSize = sizeof ( BlobFlags );
                break;

            case GETINFO_PARAM_PP_USE_COUNTS:
                /// \todo 03/11/2021 CPK Add GETINFO_PARAM_PP_USE_COUNTS functionality
                break;

            default:
                break;
        }
        Status = BLOB_STATUS_OK;
    } while ( false );

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Upgrade Handle bootloader
 *
 * \details This function updates the Handle bootloader from the SD Card
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
static BLOB_HANDLER_STATUS UpdateHandleBootloader(void)
{
    uint32_t BlockIndex;
    uint32_t DataOffset;
    uint32_t DataSize;
    uint32_t DataStart;
    uint32_t DestPtr;
    uint32_t EndDestPtr;
    uint32_t BytesRead;
    uint32_t BytesToRead;
    uint32_t MaxBytesToRead;
    uint32_t FirstReadSkipBytes;
    BLOB_HANDLER_STATUS Status;
    PROGRAM_BLOCK_INFO  nextBlockInfo;

    uint8_t AESReadBuffer[2*AES_BLOCKLEN];
    uint8_t AlignOffset;

    /* Initialize Variables */
    memset ( AESReadBuffer, 0, sizeof ( AESReadBuffer ) );
    Status             = BLOB_STATUS_OK;
    DestPtr            = 0x00u;
    DataOffset         = 0x00u;
    BlockIndex         = 0x00u;
    BytesRead          = 0x00u;
    AlignOffset        = 0x00u;
    FirstReadSkipBytes = sizeof ( PROGRAM_BLOCK_INFO );

    do
    {
        DataStart = BlobPointers.StoredHandleBLHeader.ProgramLowAddress;

        // Start erase at Flash sector boundary
        DataStart = ROUNDDOWNPOWEROF2 (DataStart,K20_FLASH_SECTOR_SIZE);

        DataSize = PROGRAM1_AREA_START - DataStart;

        // FlashEraseSector requires multiples of K20_FLASH_SECTOR_SIZE for DataSize
        DataSize = ROUNDUPPOWEROF2 (DataSize,K20_FLASH_SECTOR_SIZE);

        if ( FLASH_STATUS_OK != L2_FlashEraseSector(DataStart, DataSize) )
        {
            Status = BLOB_STATUS_ERROR;
            break;
        }

        MaxBytesToRead = FILE_DATA_MAX_SIZE;

        // FlashProgramPhrase requires multiples of 8 for DataSize
        MaxBytesToRead = ROUNDDOWNPOWEROF2(MaxBytesToRead, 8);

        while ( DestPtr < BlobPointers.StoredHandleBLHeader.ProgramHighAddress )
        {
            if ( BLOB_STATUS_OK != L4_BlobRead(BLOB_SECTION_HANDLE_BL, AESReadBuffer, (DataOffset - IV_OFFSET), (AES_BLOCKLEN + IV_OFFSET), &BytesRead) )
            {
                Status = BLOB_STATUS_ERROR;
                break;
            }

            if ( 0 == BytesRead )
            {
                Status = BLOB_STATUS_ERROR;
                break;
            }

            if ( BlobPointers.StoredBlobHeader.Encryption.bit.HandleBLEncrypted )
            {
                DecryptBinaryBuffer (AESReadBuffer,AES_BLOCKLEN,true);
            }
            //DataOffset += BytesRead;
            memcpy ( (void*)&nextBlockInfo,(void*)&AESReadBuffer[IV_OFFSET],sizeof(PROGRAM_BLOCK_INFO) );

            DestPtr = nextBlockInfo.AbsoluteAddress;
            EndDestPtr  = DestPtr + nextBlockInfo.Length;

            while ( DestPtr < EndDestPtr )
            {
                BytesToRead = MIN (EndDestPtr - DestPtr, MaxBytesToRead);
                AlignOffset = 0;

                if ( BytesToRead % AES_BLOCKLEN )
                {
                    AlignOffset = AES_BLOCKLEN - (BytesToRead % AES_BLOCKLEN);
                }

                if ( BLOB_STATUS_OK != L4_BlobRead(BLOB_SECTION_HANDLE_BL, fileData, (DataOffset - IV_OFFSET),(BytesToRead + AlignOffset + IV_OFFSET), &BytesRead) )
                {
                    Status = BLOB_STATUS_ERROR;
                    break;
                }

                if ( 0 == BytesRead )
                {
                    Status = BLOB_STATUS_ERROR;
                    break;
                }

                if ( BlobPointers.StoredBlobHeader.Encryption.bit.HandleBLEncrypted )
                {
                    DecryptBinaryBuffer (fileData, BytesRead, true);
                }

                BytesRead = BytesRead - AlignOffset - IV_OFFSET;

                DataOffset += BytesRead;

                if ( FirstReadSkipBytes )
                {
                    BytesRead = BytesRead - FirstReadSkipBytes;
                }

                // FlashProgramPhrase requires multiples of 8 for DataSize
                BytesRead = ROUNDUPPOWEROF2 (BytesRead, 8);

                if ( DestPtr >= PROGRAM1_AREA_START )
                {
                    Status = BLOB_STATUS_ERROR;
                    break;   //Bootloader blob hex out of bounds
                }

                if ( FLASH_STATUS_OK != L2_FlashWrite (DestPtr, BytesRead, (uint32_t)(fileData + FirstReadSkipBytes+IV_OFFSET)) )
                {
                    Status = BLOB_STATUS_ERROR;
                    break;
                }
                FirstReadSkipBytes = 0;

                DestPtr += BytesRead;
            }
            BlockIndex += 1;
        }

        BlobPointers.ActiveVersion.HandleBLTimestamp  = BlobPointers.StoredBlobHeader.HandleBLTimestamp;
        Status = L4_UpdateFlashActiveVersion();

    } while ( false );

    return Status;
}

#ifdef PRINT_BLOB_POINTERS /*debug purpose */
/* ========================================================================== */
/**
 * \brief   Prints the BlobPointers details to the logfile
 *
 * \details This call is used to print the blob pointers to be the logger.
 *
 *
 * \param   pBlobPointers       - Pointer to the blob pointers structure where data is to be copied.
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
static BLOB_HANDLER_STATUS PrintBlobPointers (BLOB_POINTERS *pBlobPointers)
{
    BLOB_HANDLER_STATUS Status = BLOB_STATUS_INVALID_PARAM;
    if (NULL!= pBlobPointers)
    {
        Log (REQ, "BlobPointers : HandleMainOffset  %x ", pBlobPointers->HandleMainOffset);
        Log (REQ, "BlobPointers : HandleBootloaderOffset  %x ", pBlobPointers->HandleBootloaderOffset);
        Log (REQ, "BlobPointers : JedDataOffset  %x ", pBlobPointers->JedDataOffset);
        Log (REQ, "BlobPointers : AdapterBootloaderOffset  %x ", pBlobPointers->AdapterBootloaderOffset);
        Log (REQ, "BlobPointers : EgiaMainOffset  %x ", pBlobPointers->EgiaMainOffset);
        Log (REQ, "BlobPointers : EeaMainOffset  %x ", pBlobPointers->EeaMainOffset);
        Status = BLOB_STATUS_OK;
    }
    return Status;
}
#endif

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Calculates CRC of memory chunk, returns CRC validation completion status
 *
 * \details This call is used calculate Flash Area CRC in chunks.
 *
 *
 * \param   TotalFlashCodesize       - total size of flash area
 *          HandleLowAddress         - Flash area lower address
 *          crcHandle                - crc Info
 * \return  Total memory CRC calculation status
 *          true : Program flash area CRC is calculated for entire region
 *          false: Program flash CRC calculation not completed
 * ========================================================================== */
static bool FlashFinalCrcCalculation( uint32_t TotalFlashCodesize, uint32_t HandleLowAddress, CRC_INFO *CrcHandle)
{
    uint32_t   Memorysize;
    uint32_t   Offset;
    bool       IsFinalCrcCalc;

    /* Initialize return status with false*/
    IsFinalCrcCalc = false;

    /* Memory chunk to calculate CRC */
    Memorysize = FLASHMEMORY_CRCCHECK_CHUNKSIZE;

    /* Last chunk might be < MEMORY_TEST_FLASH_SIZE(1024) */
    if ( CrcHandle->CrcCalculatedMemSize + Memorysize > TotalFlashCodesize )
    {
        Memorysize = TotalFlashCodesize - CrcHandle->CrcCalculatedMemSize;
    }

    /* Calculate offset address */
    Offset   =  HandleLowAddress + CrcHandle->CrcCalculatedMemSize;

    /* Calculate CRC for chunk */
    CrcHandle->CrcCalculated  = CRC32(CrcHandle->CrcCalculated, (uint8_t*)Offset, Memorysize);

    /* Update CRC calculated memory size */
    CrcHandle->CrcCalculatedMemSize += Memorysize;

    /* Is CRC calculated for entire memory area */
    if ( CrcHandle->CrcCalculatedMemSize == TotalFlashCodesize )
    {
        /* CRC calculation is complete, update status as completed*/
        IsFinalCrcCalc = true;
    }

    /* return FinalCrc done status */
    return IsFinalCrcCalc;
}

/* ========================================================================== */
/**
 * \brief   Calculates CRC of Program Area1, Program Area2
 *
 * \details When Program Area1, Area2 has code. Calculate CRC of Program Area1 and Parse it to Program Area2 CRC calculation.
 *
 *
 *
 * \param   pActiveVersions          - Metadata Info
 *          crcHandle                - crc Info
 * \return  Total memory CRC calculation status
 *          true : Program flash area CRC is calculated for entire region
 *          false: Program flash CRC calculation not completed
 * ========================================================================== */
static bool ValidateFlashForTwoSections(CRC_INFO *pCrcHandle, ACTIVEVERSION_2 *pActiveVersions)
{
    uint32_t TotalFlashCodesize;
    bool     IsFinalCrcCalc;

    IsFinalCrcCalc = false;
    /* Main App is in Program Area1, Area2. Calculate Area1 checksum and Parse Area1 checksum to Area2*/
    if ( (pCrcHandle->CrcCalculatedMemSize < pActiveVersions->HandleHighAddress1 - pActiveVersions->HandleLowAddress1) && !pCrcHandle->Area1CrcDone )
    {
        TotalFlashCodesize = pActiveVersions->HandleHighAddress1 - pActiveVersions->HandleLowAddress1;

        /* Calculate CRC for Program Area 1 */
        pCrcHandle->Area1CrcDone = FlashFinalCrcCalculation (TotalFlashCodesize,pActiveVersions->HandleLowAddress1, pCrcHandle);

        /* Is AREA1 CRC calculated */
        if ( pCrcHandle->Area1CrcDone )
        {
            /* Clear crc calcuated memory size. Parse calculated CRC to AREA2 */
            pCrcHandle->CrcCalculatedMemSize = 0;
        }
    }
    else
    {
        /* Calculate Program Area2 crc with parsed CRC from Program Area1. Remove Header, ProgramBlock size from total size */
        TotalFlashCodesize = pActiveVersions->HandleHighAddress2 - pActiveVersions->HandleLowAddress2;
        IsFinalCrcCalc     = FlashFinalCrcCalculation (TotalFlashCodesize,pActiveVersions->HandleLowAddress2, pCrcHandle);
    }

   return IsFinalCrcCalc;
}

/* ========================================================================== */
/**
 * \brief   Initialization of Blob Handler
 *
 * \details Used to initialize the Blob Handler and initialize blob pointer
 *
 * \param   < None >
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
BLOB_HANDLER_STATUS L4_BlobHandlerInit(void)
{
    BLOB_HANDLER_STATUS Status;     /* Error status of the function */
    uint8_t             OsError;    /* contains status of error */

    /* Initialize Variables */
    Status = BLOB_STATUS_ERROR;

    do
    {
        /* Create Blob Handler Mutex and set a name */
        pMutexBlobHandler = SigMutexCreate ("L4-BlobHandler", &OsError);

        if (NULL == pMutexBlobHandler)
        {
            /* Couldn't create mutex, exit with error */
            Status = BLOB_STATUS_ERROR;
            Log (ERR, "L4_BlobHandlerInit: Blob Handler Mutex Create Error - %d", OsError);

            break;
        }

        /* Validate the Active Version Structure */
        L4_ValidateFlashActiveVersionStruct();

        /* Read the Blobfile and Fill the Blobpointer */
        Status = ReadBlobFilePointers();

        if (BLOB_STATUS_OK != Status)
        {
            Log (ERR, "L4_BlobHandlerInit: ReadBlobFilePointers Error - %d", Status);
            break;
        }

        /*  Initialize the validation status to UNKNOWN */
        noInitRam.BlobValidationStatus = BLOB_VALIDATION_STATUS_UNKNOWN;
        Status = BLOB_STATUS_OK;

    } while ( false );

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Check the Handle Bootloader is the newest
 *
 * \details This function updates the Handle Bootloader if a newer version is available.
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
BLOB_HANDLER_STATUS L4_CheckHandleBootloader(void)
{
    BLOB_HANDLER_STATUS Status;

    Status = ReadBlobFilePointers();

    if ( BLOB_STATUS_OK == Status )
    {
        if ( (BlobPointers.StoredBlobHeader.HandleBLTimestamp != 0) &&
             (BlobPointers.ActiveVersion.HandleBLTimestamp != 0xFFFFFFFFu) )
        {
            if ( BlobPointers.ActiveVersion.HandleBLTimestamp < BlobPointers.StoredBlobHeader.HandleBLTimestamp )
            {
                Log (DBG, "Handle Bootloader is older than Blob copy");

                Status = UpdateHandleBootloader();
                if ( BLOB_STATUS_OK == Status )
                {
                    Log (DBG, "Handle Bootloader updated successfully");
                }
                else
                {
                    Log (DBG, "Handle Bootloader update failed");
                }
            }
        }
        else
        {
            Log (DBG, "Handle Bootloader timestamp problem");
            Status = BLOB_STATUS_ERROR;
        }
    }
    else
    {
        Log (DBG, "Problem detected with Handle / Blob file");
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Delete the Blob File from SD Card
 *
 * \details This call is used to Delete the Blob file from the SD Card File system.
 *
 * \param   pFsError - pointer to File system Error status
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
BLOB_HANDLER_STATUS L4_BlobErase(uint16_t *pFsError)
{
    BLOB_HANDLER_STATUS Status;    /* Error status of the function */
    FS_ERR   FsError;              /* contains the File System error status */
    uint8_t  OsErr;    /* contains status of error */

    /*Initialize Variables*/
    Status = BLOB_STATUS_ERROR;
    do
    {
        if ( NULL == pFsError )
        {
            Status = BLOB_STATUS_INVALID_PARAM;
            Log (ERR, "L4_BlobErase: Null parameter(pFsError) ");

            break;
        }

        /* Mutex Lock */
        OSMutexPend (pMutexBlobHandler, OS_WAIT_FOREVER, &OsErr);

        /* Delete the Blob File on the SD Card */
        FsError = FsDelete (BLOB_FILE_NAME);

        if (FsError != FS_ERR_NONE)
        {
            Log (ERR, "L4_BlobErase: FsDelete Error - %d", FsError);

            Status = BLOB_STATUS_ERROR;
            break;
        }

        /* update the return status to the function parameter*/
        *pFsError = (uint16_t)FsError;

        /* re-Initialize the Validation Status */
        noInitRam.BlobValidationStatus = BLOB_VALIDATION_STATUS_UNKNOWN;
        Status = BLOB_STATUS_OK;

    } while (false);

    /* Mutex Unlock */
    OSMutexPost (pMutexBlobHandler);
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Write to the Blob File in the SD Card
 *
 * \details This call is used to Write to the Blob file at a specified offset and Size.
 *          pData is written to blob file Offset. Caller functions must read the section offset
 *          from blob pointers to update a specific section. External tools updating the Blob file can
 *          directly update the Blob by performing block-wise write.
 *
 * \param   pData  - pointer to Data
 * \param   Offset - Offset of the file where data is to be written
 * \param   NumOfBytes - Number of bytes to be written to Blob file
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
BLOB_HANDLER_STATUS L4_BlobWrite (uint8_t *pData, uint32_t Offset, uint32_t NumOfBytes)
{
    BLOB_HANDLER_STATUS Status;  /* Error status of the function */
    FS_ERR   FsError;            /* contains the File System error status */
    FS_FILE *pBlobFile;          /* Blob file Pointer used to access the file */
    uint8_t    OsErr;            /* Contains status of OS error */
    CPU_SIZE_T     BytesWritten; /* Number of bytes written as returned by File system calls */

    /*Initialize Variables*/
    Status = BLOB_STATUS_ERROR;
    OsErr = 0u;
    pBlobFile = NULL;

    do
    {
        if ( NULL == pData )
        {
            Status = BLOB_STATUS_INVALID_PARAM;
            Log (ERR, "L4_BlobWrite: Null parameter(pData) ");

            break;
        }

        /* Mutex Lock */
        OSMutexPend (pMutexBlobHandler, BLOB_MUTEX_TIMEOUT , &OsErr);
        if (OS_ERR_NONE != OsErr)
        {
            Status = BLOB_STATUS_ERROR;
            Log (ERR, "L4_BlobWrite: Mutex Error ");
            break;
        }

        FsError = FsOpen (&pBlobFile, BLOB_FILE_NAME, FS_FILE_ACCESS_MODE_WR | FS_FILE_ACCESS_MODE_CREATE);
        if (FS_ERR_NONE != FsError)
        {
            Log (ERR, "L4_BlobWrite: FsOpen Error %d", FsError);
            break;
        }

        FsError = FsGetInfo (BLOB_FILE_NAME, &BlobFileAttrib);
        if (FsError != FS_ERR_NONE)
        {
            Log (ERR, "L4_BlobWrite: FsGetInfo Error %d", FsError);
            break;
        }

        if (NumOfBytes > FILE_WRITE_MAX_SIZE)
        {
            Status = BLOB_STATUS_INVALID_PARAM;
            Log (ERR, "L4_BlobWrite: Invalid Param (NumOfBytes)");
            break;
        }

        /* Validate the Requested offset and Size. Seek to Offset */
        if ((BlobFileAttrib.Size >= (Offset + NumOfBytes)) || (BlobFileAttrib.Size == 0))
        {
            FsError = FsSeek (pBlobFile, Offset, FS_FILE_ORIGIN_START);
        }
        else if (BlobFileAttrib.Size == Offset)
        {
            FsError = FsSeek(pBlobFile, 0, FS_FILE_ORIGIN_END);
        }
        else
        {
            FsError = FS_ERR_FILE_INVALID_OFFSET;
        }

        if (FsError != FS_ERR_NONE)
        {
            Log (ERR, "L4_BlobWrite: FsSeek Error %d", FsError);
            break;
        }

        /* Write to the Blob File */
        FsError = FsWrite (pBlobFile, pData, NumOfBytes, &BytesWritten);
        if (FsError != FS_ERR_NONE)
        {
            Log (ERR, "L4_BlobWrite: FsWrite Error %d", FsError);
            break;
        }

        /* FsWrite is success at this stage and any write would have impacted the integrity of the file -  invalidating BlobValidationStatus */
        noInitRam.BlobValidationStatus = BLOB_VALIDATION_STATUS_UNKNOWN;
        if (BytesWritten != NumOfBytes)
        {
            Log (ERR, "L4_BlobWrite: FsWrite Error NumOfBytes to Write %d BytesWritten %d", NumOfBytes,BytesWritten);
            break;
        }

        Status = BLOB_STATUS_OK;

    } while (false);

    /* Close file and Unlock Mutex */
    FsError = FsClose (pBlobFile);
    if (OS_ERR_NONE == OsErr)
    {
        OSMutexPost (pMutexBlobHandler);
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Read from the Blob File in the SD Card
 *
 * \details This call is used to Read data from the Blob file from a specified offset and Size
 *
 * \param   BlobSection - Section (Eg : Handle Main, Handle Bootloader etc.) specified in BLOB_SECTION
 * \param   pData     - pointer to Data
 * \param   Offset    - Offset of the file where data is to be Read
 * \param   DataSize  - Number of bytes to be read from Blob file
 * \param   pBytesRead - pointer to Number of bytes read from Blob file to be returned to the caller
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
BLOB_HANDLER_STATUS L4_BlobRead (BLOB_SECTION BlobSection, uint8_t *pData, uint32_t Offset, uint32_t DataSize, uint32_t *pBytesRead)
{
    BLOB_HANDLER_STATUS Status;   /* Error status of the function */
    FS_ERR   FsError;             /* Contains the File System error status */
    FS_FILE *pBlobFile;           /* Blob file Pointer used to access the file */
    uint8_t    OsErr;             /* Contains status of OS error */
    uint32_t   BytesToRead;       /* Number of bytes to read */
    uint32_t   FileOffset;        /* Offset based on the blob pointers */

    Status = BLOB_STATUS_ERROR;
    pBlobFile = NULL;
    BytesToRead = 0;
    FileOffset = 0;

    do
    {
        if (NULL == pData)
        {
            Status = BLOB_STATUS_INVALID_PARAM;
            Log (ERR, "L4_BlobRead: Null parameter(pData) ");

            break;
        }
        if (NULL == pBytesRead)
        {
            Status = BLOB_STATUS_INVALID_PARAM;
            Log (ERR, "L4_BlobRead: Null parameter(pBytesRead) ");

            break;
        }

        OSMutexPend (pMutexBlobHandler, OS_WAIT_FOREVER, &OsErr);

        /* Blob file layout has multiple sections as identified in BLOB_SECTION. These section pointers are maintained by blob pointers*/
        /* This is used so the callers need not know the layout of the blob and this function moves to the offset relative to the BlobSection*/
        Status = GetBlobSectionOffset (BlobSection, Offset,  DataSize, &BytesToRead, &FileOffset);
        if (BLOB_STATUS_OK != Status)
        {
            Log(ERR, "L4_BlobRead: GetBlobSectionOffset Error %d", Status);

            break;
        }

        FsError = FsOpen (&pBlobFile, BLOB_FILE_NAME, FS_FILE_ACCESS_MODE_RD);
        if (FsError == FS_ERR_NONE)
        {
            FsError = FsGetInfo (BLOB_FILE_NAME, &BlobFileAttrib);
            if (FS_ERR_NONE != FsError)
            {
                Log (ERR, "L4_BlobRead: FsOpen Error %d", FsError);

                break;
            }
        }

        FsError = FsSeek (pBlobFile, (FileOffset+Offset), FS_SEEK_SET);
        if (FS_ERR_NONE != FsError)
        {
            Log (ERR, "L4_BlobRead: FsSeek Error %d", FsError);

            break;
        }

        FsError = FsRead(pBlobFile, (uint8_t *)pData, BytesToRead, pBytesRead);
        if (FS_ERR_NONE != FsError)
        {
            Log (ERR, "L4_BlobRead: FsRead Error %d", FsError);

            break;
        }

        Status = BLOB_STATUS_OK;

    } while ( false );

    /* Close file and Unlock Mutex */
    FsError = FsClose (pBlobFile);
    OSMutexPost (pMutexBlobHandler);
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Validates the Blob File in the SD Card with the checksum available in the blob
 *
 * \details This call is used to Validate the blob using the blob checksum. Checksum is calculated by
 *          reading the blob file and compared with the blob checksum. Consumers of blob handler
 *          must validate the blob and proceed to other blob functionality.
 *
 * \param   ForceCheck - if true, CRC calculation is forced for comparison.
 *                       if False, a stored Validation status is returned
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
BLOB_HANDLER_STATUS L4_BlobValidate(bool ForceCheck)
{
    BLOB_HANDLER_STATUS Status;
    FS_ERR        FsError;
    FS_FILE       *pBlobFile;
    uint8_t       OsErr;
    uint32_t      BytesRead;
    uint32_t      BytesToRead;
    uint32_t      CalculatedChecksum;
    uint32_t      TotalBlobSize;
    uint32_t      FileOffset;
    uint32_t      LoopCounter;

    Status = BLOB_STATUS_ERROR;
    pBlobFile = NULL;
    CalculatedChecksum = 0;
    BytesRead = 0;
    LoopCounter = 0;

    do
    {
        if (ForceCheck)
        {
            noInitRam.BlobValidationStatus = BLOB_VALIDATION_STATUS_UNKNOWN;
        }
        else
        {
            if ((noInitRam.BlobValidationStatus == BLOB_STATUS_VALIDATED) || (noInitRam.BlobValidationStatus == BLOB_STATUS_BAD))
            {
                /* Return the stored validation status - maintaining the same enum value */
                Status = noInitRam.BlobValidationStatus;
                break;
            }
        }

        memset (&BlobPointers, 0, sizeof(BlobPointers));
        Status = L4_ValidateFlashActiveVersionStruct();
        if (BLOB_STATUS_OK != Status)
        {
            break;
        }

        if (0 == BlobPointers.StoredBlobHeader.HandleDataSize)
        {
            ReadBlobFilePointers();
        }

        /* force check */
        OSMutexPend (pMutexBlobHandler, OS_WAIT_FOREVER, &OsErr);

        FsError = FsOpen (&pBlobFile, BLOB_FILE_NAME, FS_FILE_ACCESS_MODE_RD);
        if (FS_ERR_NONE != FsError)
        {
            Log (ERR, "L4_BlobValidate: FsOpen Error %d", FsError);
            break;
        }

        FsError = FsGetInfo(BLOB_FILE_NAME, &BlobFileAttrib);
        if (FS_ERR_NONE != FsError)
        {
            Log (ERR, "L4_BlobValidate: FsGetInfo Error %d", FsError);
            break;
        }

        TotalBlobSize = BlobPointers.StoredBlobHeader.BlobHeaderSize +
                        BlobPointers.StoredBlobHeader.HandleDataSize +
                        BlobPointers.StoredBlobHeader.HandleBLDataSize +
                        sizeof(MACHX02) +
                        BlobPointers.StoredBlobHeader.JedDataSize +
                        BlobPointers.StoredBlobHeader.AdaptBLDataSize +
                        BlobPointers.StoredBlobHeader.EgiaDataSize +
                        BlobPointers.StoredBlobHeader.EeaDataSize;

        /* validate the blobsize */
        if (BlobFileAttrib.Size < TotalBlobSize)
        {
            Log (ERR, "L4_BlobValidate: Blob file size validation failed ");
            break;
        }

        /* Move to just past the blob's CRC */
        FsError = FsSeek (pBlobFile, sizeof(uint32_t), FS_FILE_ORIGIN_START);
        FileOffset = sizeof(uint32_t);

        /* Calculate the checksum over the entire blob, except for the checksum itself, which is the first data item. */
        while (FileOffset < TotalBlobSize)
        {
            BytesToRead = MIN (TotalBlobSize - FileOffset, FILE_DATA_MAX_SIZE);
            FsError = FsRead (pBlobFile, fileData, BytesToRead, &BytesRead);
            if (FS_ERR_NONE != FsError)
            {
                Log(ERR, "L4_BlobValidate: FsRead Error %d",FsError);

                break;
            }

            if (BytesRead != BytesToRead)
            {
                Log(ERR, "L4_BlobValidate: FsRead Error BytesToRead: %d   BytesRead = %d",BytesToRead,BytesRead);

                break;
            }

            CalculatedChecksum = CRC32(CalculatedChecksum, fileData, BytesRead);
            FileOffset += BytesRead;
            LoopCounter++;

            /* Adding delay for every 100KB read - current blobsize is ~700KB */
            if (0 == (LoopCounter % LOOPCOUNTER_200))
            {
                OSTimeDly(MSEC_1);
            }
        }

        /* Compare calculated CRC and stored CRC in blob */
        if (CalculatedChecksum == BlobPointers.StoredBlobHeader.BlobChecksum)
        {
            Status = BLOB_STATUS_VALIDATED;    /* Validated */
        }
        else
        {
            Status = BLOB_STATUS_BAD;          /* Failed checksum */
        }

    } while ( false );

    FsError = FsClose (pBlobFile); // !!! only FsClose() if the file is Open !!!

    /* Assign the Blob Validation Status */
    noInitRam.BlobValidationStatus = Status;
    OSMutexPost (pMutexBlobHandler);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Copies the BlobPointers to the user requested datastructure
 *
 * \details This call is used to copy the blob pointers to be used by the caller.
 *
 *
 * \param   pBlobPointers       - Pointer to the blob pointers structure where data is to be copied.
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
BLOB_HANDLER_STATUS L4_GetBlobPointers(BLOB_POINTERS *pBlobPointers)
{
    BLOB_HANDLER_STATUS Status;

    Status = BLOB_STATUS_ERROR;
    if (NULL!= pBlobPointers)
    {
        /* copy the blob pointers to the user rquested pointer */
        memcpy (pBlobPointers, &BlobPointers, sizeof(BlobPointers));
        Status = BLOB_STATUS_OK;
    }

    return Status;
}

/* ========================================================================== */
/**
 *
 * \details - This function updates the Active Versions section in Flash with the data
 *               from BlobPointers.ActiveVersion
 *
 * \param   < None >
 *
 * \return  BLOB_HANDLER_STATUS - Error status: returns BLOB_STATUS_ERROR if failed to update
 *                               ActiveVersion Struct
 *
 * ========================================================================== */
static BLOB_HANDLER_STATUS L4_UpdateFlashActiveVersion(void)
{
    uint32_t      DataSize;
    BLOB_HANDLER_STATUS Status;
 
    Status = BLOB_STATUS_OK;

    do
    {
        BlobPointers.ActiveVersion.StructChecksum = CRC32(0, (uint8_t *)&BlobPointers.ActiveVersion + sizeof(uint32_t), sizeof(ACTIVEVERSION) - sizeof(uint32_t));
        // Erase the ACTIVE_METADATA_START area
        if ((L2_FlashEraseSector(ACTIVE_METADATA_NEW_START, ACTIVE_METADATA_NEW_END - ACTIVE_METADATA_NEW_START)))
        {
            Log (DBG,"UpdateActiveVersion FlashEraseSector failed");

            Status = BLOB_STATUS_ERROR;
            break;
        }

        DataSize = sizeof (ACTIVEVERSION);

        DataSize = ROUNDUPPOWEROF2(DataSize,8);  // Flash programming requires multiples of 8 for DataSize

        if ((L2_FlashWrite ((uint32_t)ACTIVE_METADATA_NEW_START, DataSize, (uint32_t)&BlobPointers.ActiveVersion)))
        {
            Log (DBG,"UpdateActiveVersion FlashProgramPhrase failed");

            Status = BLOB_STATUS_ERROR;
            break;
        }

    } while (false);

   return Status;
}

/* ========================================================================== */
/**
 *
 * \details - Validates the ActiveVersion structure residing in Flash memory.
 *               Will update the version of the structure to the latest version if it is not
 *               already at that version.
 *
 * \param   < None >
 *
 * \return  BLOB_HANDLER_STATUS - Error status: returns BLOB_STATUS_ERROR if failed to update
 *                               ActiveVersion Struct
 * ========================================================================== */
BLOB_HANDLER_STATUS L4_ValidateFlashActiveVersionStruct(void)
{
    BLOB_HANDLER_STATUS  Status;
    uint32_t             CalculatedChecksum;
    bool                 ActiveVersionStructDirty;
    ACTIVEVERSION_1     *ActiveVersion_Ver1;
    ACTIVEVERSION_2     *ActiveVersion_Ver2;

    ActiveVersion_Ver1 = ((ACTIVEVERSION_1 *)ACTIVE_METADATA_NEW_START);
    ActiveVersion_Ver2 = ((ACTIVEVERSION_2 *)ACTIVE_METADATA_NEW_START);
    ActiveVersionStructDirty = false;
    Status = BLOB_STATUS_OK;

    do
    {
        // First check if this is an ActiveVersion_Ver1 struct, with no embedded version and size.
        CalculatedChecksum = CRC32 (0, (uint8_t *)ActiveVersion_Ver1 + sizeof(uint32_t), sizeof(ActiveVersion_Ver1) - sizeof(uint32_t));
        if (CalculatedChecksum == ActiveVersion_Ver1->StructChecksum)
        {
            ActiveVersionsStructVer = ACTIVE_VERSION_1;   // ActiveVersion_Ver1
        }
        else if ((ActiveVersion_Ver2->StructSize != 0) && (ActiveVersion_Ver2->StructSize <= sizeof(ACTIVEVERSION)))   // Sanity check on structSize
        {
            // All versions after Ver1 have embedded version and struct size.
            CalculatedChecksum = CRC32 (0, (uint8_t *)ActiveVersion_Ver2 + sizeof(uint32_t), ActiveVersion_Ver2->StructSize - sizeof(uint32_t));
            if (CalculatedChecksum == ActiveVersion_Ver2->StructChecksum)
            {
                ActiveVersionsStructVer = ActiveVersion_Ver2->StructVersion;
            }
        }
        else // empty 'else' for code standard compliance
        {
        }

        switch (ActiveVersionsStructVer)
        {
            case ACTIVE_VERSION_INVALID:  // No valid ActiveVersions
            memset (&BlobPointers.ActiveVersion, 0, sizeof (ACTIVEVERSION));
            BlobPointers.ActiveVersion.StructVersion       = LATEST_ACTIVE_VERSION_STRUCT;
            BlobPointers.ActiveVersion.StructSize          = sizeof (ACTIVEVERSION);
            ActiveVersionStructDirty = true;
            break;

            case ACTIVE_VERSION_1:  // Update ACTIVEVERSION_1 to latest version
            memset (&BlobPointers.ActiveVersion, 0, sizeof(ACTIVEVERSION));
            BlobPointers.ActiveVersion.StructVersion        = LATEST_ACTIVE_VERSION_STRUCT;
            BlobPointers.ActiveVersion.StructSize           = sizeof (ACTIVEVERSION);
            BlobPointers.ActiveVersion.HandleTimestamp      = ((ACTIVEVERSION_1 *)ACTIVE_METADATA_NEW_START)->HandleTimestamp;
            BlobPointers.ActiveVersion.HandleChecksum       = ((ACTIVEVERSION_1 *)ACTIVE_METADATA_NEW_START)->HandleChecksum;
            BlobPointers.ActiveVersion.HandleDataSize       = ((ACTIVEVERSION_1 *)ACTIVE_METADATA_NEW_START)->HandleDataSize;
            BlobPointers.ActiveVersion.HandleEntryAddress   = ((ACTIVEVERSION_1 *)ACTIVE_METADATA_NEW_START)->HandleEntryAddress;
            BlobPointers.ActiveVersion.HandleLowAddress1    = ((ACTIVEVERSION_1 *)ACTIVE_METADATA_NEW_START)->HandleLowAddress;
            BlobPointers.ActiveVersion.HandleHighAddress1   = BlobPointers.ActiveVersion.HandleLowAddress1 + BlobPointers.ActiveVersion.HandleDataSize;
            BlobPointers.ActiveVersion.HandleLowAddress2    = 0;
            BlobPointers.ActiveVersion.HandleHighAddress2   = 0;
            BlobPointers.ActiveVersion.HandleBLTimestamp    = ((ACTIVEVERSION_1 *)ACTIVE_METADATA_NEW_START)->HandleBLTimestamp;
            BlobPointers.ActiveVersion.JedTimestamp         = ((ACTIVEVERSION_1 *)ACTIVE_METADATA_NEW_START)->JedTimestamp;
            BlobPointers.ActiveVersion.AdaptBLTimestamp     = ((ACTIVEVERSION_1 *)ACTIVE_METADATA_NEW_START)->AdaptBLTimestamp;
            BlobPointers.ActiveVersion.EgiaTimestamp        = ((ACTIVEVERSION_1 *)ACTIVE_METADATA_NEW_START)->EgiaTimestamp;
            BlobPointers.ActiveVersion.EeaTimestamp         = ((ACTIVEVERSION_1 *)ACTIVE_METADATA_NEW_START)->EeaTimestamp;
            ActiveVersionStructDirty = true;
            break;

            default: // Any later version than 1.
            break;
        }

        if (ActiveVersionStructDirty)
        {
            Status = L4_UpdateFlashActiveVersion();
            if (BLOB_STATUS_OK != Status)
            {
                break;
            }
        }

        memcpy (&BlobPointers.ActiveVersion, (void *)ACTIVE_METADATA_NEW_START, sizeof (ACTIVEVERSION));

  } while (false);

  return Status;
}

/* ========================================================================== */
/**
 * \brief - This function erases the handle timestamp.
 *
 * \details Function clears the handle Timestamp for the Forced Upgrade
 *
 * \param   < None >
 *
 * return  bool - Error status: returns True if failed to erase handle timestamp,
 *                                      False if success
 * ========================================================================== */
bool EraseHandleTimestamp(void)
{
    BLOB_HANDLER_STATUS Status;

    BlobPointers.ActiveVersion.HandleTimestamp = 0;

    // update the ActiveVersion structure
    Status = L4_UpdateFlashActiveVersion();
    return (BLOB_STATUS_OK != Status) ? true : false;
}

/* ========================================================================== */
/**
 * \brief - This function erases the BL timestamp.
 *
 * \details This function erases the handle's boot loader timestamp.
 *
 * \param   < None >
 *
 * return  bool - Error status: returns True if failed to erase handle BL timestamp,
 *                                      False if success
 * ========================================================================== */
bool EraseHandleBLTimestamp(void)
{
    BLOB_HANDLER_STATUS Status;

    BlobPointers.ActiveVersion.HandleBLTimestamp = 0;

    // update the ActiveVersion structure
    Status = L4_UpdateFlashActiveVersion();
    return (BLOB_STATUS_OK != Status) ? true : false;
}

/* ========================================================================== */
/**
 * \brief - This function erases the FPGA timestamp.
 *
 * \details This function erases the FPGA jed timestamp.
 *
 * \param   < None >
 *
 * \return  bool - Error status: returns True if failed to erase FPGA timestamp,
 *                                      False if success
 * ========================================================================== */
bool FPGA_EraseTimestamp(void)
{
    BLOB_HANDLER_STATUS Status;

    BlobPointers.ActiveVersion.JedTimestamp = 0;

    // update the ActiveVersion structure
    Status = L4_UpdateFlashActiveVersion();
    return (BLOB_STATUS_OK != Status) ? true : false;
}

/* ========================================================================== */
/**
 * \brief - This function retrieves the FPGA jed timestamp.
 *
 * \details This function retrieves the FPGA jed timestamp.
 *
 * \param   < None >
 *
 * \return  uint32_t - timestamp
 *
 * ========================================================================== */
uint32_t FPGA_GetTimestamp(void)
{
    return ( BlobPointers.ActiveVersion.JedTimestamp );
}

/* ========================================================================== */
/**
 * \brief - This function sets the FPGA jed timestamp.
 *
 * \details This function sets the FPGA jed timestamp.
 *
 * \param   < None >
 *
 * \return  bool - Error status: returns True if failed to set FPGA timestamp,
 *                                      False if success
 * ========================================================================== */
bool FPGA_SetTimestamp(uint32_t Timestamp)
{
    BLOB_HANDLER_STATUS Status;

    BlobPointers.ActiveVersion.JedTimestamp = Timestamp;

    // update the ActiveVersion structure
    Status = L4_UpdateFlashActiveVersion();
    return (BLOB_STATUS_OK != Status) ? true : false;
}

/******************************************************************************/
/*                    Bootloader Helper Functions                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Upgrade Handle App From SD Card Blob
 *
 * \details Erases the area of flash dedicated to the main app, and reprograms it using
 *          data from the blob file.  Updates the ActiveVersion area of flash upon success.
 *
 * \param   < None >
 *
 * \return  BLOB_HANDLER_STATUS - Blob Handler return status
 * \retval  See L4_BlobHandler.h
 *
 * ========================================================================== */
FLASH_PROGRAM_STATUS L4_UpgradeHandleMainApp(void)
{
    bool FirstRead;
    uint32_t BytesRead;
    uint32_t BytesToRead;
    uint32_t MaxBytesToRead;
    uint32_t DestPtr;
    uint32_t DataOffset;
    uint32_t EndDestPtr;
    uint32_t DataSize;
    uint32_t BlockIndex;
    uint32_t DataReadOffset;
    uint8_t FirstReadOffset;
    int8_t DataoffsetAdjust;
    uint8_t DecryptAlignPad;
    int32_t ReadNegativeOffsetBytes;
    int8_t UnalignedProgramBlockInfoBytes;
    FLASH_PROGRAM_STATUS Status;
    PROGRAM_BLOCK_INFO  NextBlockInfo;

    Status = FLASH_PROGRAM_OK;
    FirstRead = true;
    DestPtr = 0;
    DataOffset = 0;
    BlockIndex = 0;
    UnalignedProgramBlockInfoBytes = 0;
    ReadNegativeOffsetBytes = 0;
    DataReadOffset =0;
    memset(TempBuffer,0x00,sizeof(TempBuffer));

    do
    {
        // Set up to erase all of flash between ProgramLowAddress and PROGRAM1_AREA_END
        DataSize = PROGRAM1_AREA_END - BlobPointers.StoredHandleHeader.ProgramLowAddress;

        // FlashEraseSector requires multiples of 0x1000 for DataSize
        while (DataSize % K20_FLASH_SECTOR_SIZE)
        {
           DataSize += 1;
        }

        if ( FLASH_STATUS_OK != L2_FlashEraseSector( BlobPointers.StoredHandleHeader.ProgramLowAddress, DataSize ))
        {
           Status = FLASH_PROGRAM_ERROR_ERASE;
           break;
        }

        // Set up to erase all of flash between PROGRAM2_AREA_START and PROGRAM2_AREA_END
        DataSize = PROGRAM2_AREA_END - PROGRAM2_AREA_START;

        if ( FLASH_STATUS_OK != L2_FlashEraseSector( PROGRAM2_AREA_START, DataSize ))
        {
           Status = FLASH_PROGRAM_ERROR_ERASE;
           break;
        }

        MaxBytesToRead = FILE_DATA_MAX_SIZE;

        while ( MaxBytesToRead % 8 )
        {
           MaxBytesToRead -= 1;    // FlashProgramPhrase requires multiples of 8 for DataSize
        }

        while ( (DestPtr < BlobPointers.StoredHandleHeader.ProgramHighAddress) && (FLASH_PROGRAM_OK == Status) )
        {
            memset (TempBuffer, 0, sizeof (TempBuffer));

            DataoffsetAdjust = DataOffset % AES_BLOCKLEN;

            if ( DataoffsetAdjust > 8 )
            {
                DataReadOffset = IV_OFFSET;
            }
            else
            {
                DataReadOffset = 0;
            }

            // always reads multiples of 16 bytes, if read doesn't fall in 16 byte boundary then adjust the read address
            // this is done to decrypt data in 16 bytes blocks
            // for CBC decryption need previous cipher block for IV
            L4_BlobRead (BLOB_SECTION_HANDLE_MAIN, TempBuffer, (DataOffset - DataoffsetAdjust - IV_OFFSET),(AES_BLOCKLEN + IV_OFFSET + DataReadOffset), (uint32_t*)&BytesRead);

            if ( BlobPointers.StoredBlobHeader.Encryption.bit.HandleEncrypted )
            {
                //decrypt read data
                DecryptBinaryBuffer (TempBuffer, AES_BLOCKLEN + DataReadOffset, true);
            }

            memcpy ((void*)&NextBlockInfo, (void*)(&TempBuffer[IV_OFFSET] + DataoffsetAdjust), sizeof(PROGRAM_BLOCK_INFO));

            // PROGRAM_BLOCK_INFO falls in 16 bye boundary, ignore first 8 bytes in data
            if ( !DataoffsetAdjust )
            {
                FirstRead = true;
            }
            else if ( DataoffsetAdjust && (DataoffsetAdjust < sizeof(PROGRAM_BLOCK_INFO)))
            {   // use this to discard the first unaligned program block Info bytes from being written to flash
                UnalignedProgramBlockInfoBytes = sizeof (PROGRAM_BLOCK_INFO) - DataoffsetAdjust;
                // for 8 byte boundary alignment
                DataoffsetAdjust +=  (2 * UnalignedProgramBlockInfoBytes);
            }
            else if ( DataoffsetAdjust && (DataoffsetAdjust > sizeof(PROGRAM_BLOCK_INFO)))
            {   // use this to discard the first unaligned program block Info bytes from being written to flash
                UnalignedProgramBlockInfoBytes = sizeof (PROGRAM_BLOCK_INFO) - DataoffsetAdjust;
                DataoffsetAdjust = sizeof (PROGRAM_BLOCK_INFO) + UnalignedProgramBlockInfoBytes;
            }

            DataOffset += DataoffsetAdjust;

            // DestPtr and EndDestPtr will have been automatically aligned properly by the linker.
            DestPtr = NextBlockInfo.AbsoluteAddress;
            EndDestPtr = DestPtr + NextBlockInfo.Length;

            if ( BlockIndex == 0 )
            {
                BlobPointers.ActiveVersion.HandleLowAddress1 = DestPtr;
                BlobPointers.ActiveVersion.HandleHighAddress1 = EndDestPtr;
            }
            else if ( BlockIndex == 1 )
            {
                BlobPointers.ActiveVersion.HandleLowAddress2 = DestPtr;
                BlobPointers.ActiveVersion.HandleHighAddress2 = EndDestPtr;
            }
            else
            {
                // ERROR: should be a max of 2 blocks!
                Status = FLASH_PROGRAM_ERROR_BLOCK_INDEX;
                break;
            }

            while (DestPtr < EndDestPtr)
            {
                memset (fileData, 0, sizeof (fileData));

                BytesToRead = MIN(EndDestPtr - DestPtr, MaxBytesToRead);

                DecryptAlignPad = BytesToRead  % AES_BLOCKLEN;

                if ( DecryptAlignPad ) //don't write alignment bytes to memory
                {
                    DecryptAlignPad = AES_BLOCKLEN - DecryptAlignPad;
                }

                if ( 4 == UnalignedProgramBlockInfoBytes )
                {   // in case of 4 byte boundary misalignment read extra bytes to decrypt data
                    DataReadOffset = ( 2 * IV_OFFSET );
                }
                else if ( -4 == UnalignedProgramBlockInfoBytes )
                {
                    ReadNegativeOffsetBytes = IV_OFFSET;
                    DataReadOffset = IV_OFFSET;
                }
                else
                {
                    DataReadOffset = IV_OFFSET;
                }

                L4_BlobRead (BLOB_SECTION_HANDLE_MAIN, fileData, ( DataOffset - DataReadOffset ), (BytesToRead + DecryptAlignPad + DataReadOffset + ReadNegativeOffsetBytes), (uint32_t*)&BytesRead);

                if ( BlobPointers.StoredBlobHeader.Encryption.bit.HandleEncrypted )
                {
                    // decrypt read data
                    DecryptBinaryBuffer ( fileData, (BytesToRead + DecryptAlignPad + DataReadOffset + ReadNegativeOffsetBytes), true );
                }

                if ( BytesRead > BytesToRead ) //don't write alignment bytes to memory
                {
                    BytesRead = BytesToRead;
                }

                DataOffset += BytesRead;

                FirstReadOffset = 0;

                if ( FirstRead )
                {   //for first time data read from a new linear address block, ignore the initial 8 bytes for block meta data
                    //meta data may exist either at 16 byte boundary or at 8 byte boundary
                    //this if takes care of 16 byte boundary case
                    FirstRead       =  false;
                    FirstReadOffset = sizeof (PROGRAM_BLOCK_INFO);
                    BytesRead       = BytesRead - FirstReadOffset;
                }

                if (0 == BytesRead)
                {
                    Status = FLASH_PROGRAM_ERROR_READ;
                    break;
                }

                BytesRead = ROUNDUPPOWEROF2 (BytesRead, 8); // FlashProgramPhrase requires multiples of 8 for DataSize

                if ( FLASH_STATUS_OK != L2_FlashWrite (DestPtr, BytesRead, (uint32_t)&fileData[DataReadOffset - UnalignedProgramBlockInfoBytes] + FirstReadOffset))
                {
                    Status = FLASH_PROGRAM_ERROR_WRITE;
                    break;
                }

                DestPtr += BytesRead;
            }

            UnalignedProgramBlockInfoBytes = 0;
            BlockIndex += 1;
        }

        if ( FLASH_PROGRAM_OK == Status )
        {
            // Update ActiveVersion and put it back in place at ACTIVE_METADATA_ADDRESS
            BlobPointers.ActiveVersion.HandleTimestamp    = BlobPointers.StoredBlobHeader.HandleTimestamp;
            BlobPointers.ActiveVersion.HandleDataSize     = BlobPointers.StoredBlobHeader.HandleDataSize;
            BlobPointers.ActiveVersion.HandleChecksum     = BlobPointers.StoredHandleHeader.ProgramChecksum;
            BlobPointers.ActiveVersion.HandleEntryAddress = BlobPointers.StoredHandleHeader.ProgramEntryAddress;
            BlobPointers.ActiveVersion.StructChecksum     = CRC32 (0, (uint8_t *)&BlobPointers.ActiveVersion + sizeof (uint32_t), sizeof (ACTIVEVERSION) - sizeof (uint32_t));

            Status = (BLOB_STATUS_OK == L4_UpdateFlashActiveVersion()) ? FLASH_PROGRAM_OK : FLASH_PROGRAM_ERROR;
        }

    } while ( false );

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Validates Program flash Main App
 *
 * \details Calculates CRC for Program Flash Main App
 *
 *
 * \param   pCrcHandle: pointer to CRC Info of Flash
 *
 * \return CRC validation status
 *
 * ========================================================================== */
FLASH_CRCVALIDATION_STATUS L4_ValidateMainAppFromFlash(CRC_INFO *pCrcHandle)
{
    ACTIVEVERSION_2            *pActiveVersions;
    FLASH_CRCVALIDATION_STATUS  Status;
    bool                        IsFinalCrcCalc;
    uint32_t                    TotalFlashCodesize;

    /* Initialize variables */
    pActiveVersions = (ACTIVEVERSION_2*)(ACTIVE_METADATA_NEW_START);
    IsFinalCrcCalc = false;

    do
    {
        /* Check Metadata is having valid size of program flash */
        if ( !(pActiveVersions->HandleDataSize > 0) )
        {
            /* CRC validation has not started */
            Status =  FLASH_CRCVALIDATION_UNKNOWN;
            /* Log Error */
            Log (ERR, "Metadata Handle Program data size is zero");
            break;
        }

        /* Check Metadata High address 2 is within FLASH PROGRAM AREA2 END */
        if ( !( pActiveVersions->HandleHighAddress2 < FLASHPROGRAM_AREA2_END ) )
        {
            /* CRC validation has not started */
            Status =  FLASH_CRCVALIDATION_UNKNOWN;
            /* Log Error */
            Log(ERR, "Metadata Area2 address: %x is greater than FLASH Program Area2", pActiveVersions->HandleHighAddress2);
            break;
        }

        /* Check Metadata Low Address1 is less than High Address1*/
        if ( !( pActiveVersions->HandleLowAddress1 < pActiveVersions->HandleHighAddress1 ) )
        {
            /* CRC validation has not started */
            Status =  FLASH_CRCVALIDATION_UNKNOWN;
            /* Log Error */
            Log (ERR, "Metadata Area1 Low address: %x is greater than Metadata High Address1: %x", pActiveVersions->HandleLowAddress1, pActiveVersions->HandleHighAddress1 );
            break;
        }

        /* Update status as VALIDATING */
        Status = FLASH_CRCVALIDATION_INPROGRESS;

        /* Main App is only at PROGRAM AREA 1*/
        if ( 0 == pActiveVersions->HandleHighAddress2 )
        {
            /* calculate actual flash code size. Remove Header, ProgramBlock size from total size */
            TotalFlashCodesize = pActiveVersions->HandleDataSize - sizeof (BINARY_HEADER) - sizeof (PROGRAM_BLOCK_INFO);
            /* Calculate CRC for chunk by parsing calculated CRC */
            IsFinalCrcCalc = FlashFinalCrcCalculation (TotalFlashCodesize,pActiveVersions->HandleLowAddress1, pCrcHandle);
        }
        else
        {
            IsFinalCrcCalc = ValidateFlashForTwoSections (pCrcHandle, pActiveVersions);
        }

        /* Is final CRC calculated */
        if( IsFinalCrcCalc )
        {
            /* Reset Area1 CRC check */
            pCrcHandle->Area1CrcDone = false;
            TM_Hook(HOOK_FLASH_INTEGRITY_SIMULATE, (void *)(&pCrcHandle));
            /* Compare calculated CRC with stored CRC */
            Status = ( pCrcHandle->CrcCalculated == pActiveVersions->HandleChecksum ) ? FLASH_CRC_VALIDATED_GOOD:FLASH_CRC_VALIDATED_BAD;
            /* Clear calculated CRC */
            pCrcHandle->CrcCalculated        = 0;
            /* Clear memory size of calculated area */
            pCrcHandle->CrcCalculatedMemSize = 0;
        }
    } while ( false );

    /* Return CRC validation status */
    return Status;
}

/* ========================================================================== */
/**
 * \brief   validates handle app
 *
 * \details This function reads the Handle app from flash and validates it. Returns
 *          the address of the entry to the main app if executable is valid, NULL otherwise.
 *
 * \param   < None >
 *
 * \return  MAIN_FUNC - a pointer to the entry address of the main app (__startup).
 *
 * ========================================================================== */
MAIN_FUNC L4_ValidateHandleMainApp(void)
{
    uint32_t CalculatedChecksum;
    MAIN_FUNC pMainEntry;

    pMainEntry = NULL;

    if ( ACTIVE_VERSION_1 == ActiveVersionsStructVer )
    {
        ACTIVEVERSION_1 *ActiveVersion;
        ActiveVersion = (ACTIVEVERSION_1 *)ACTIVE_METADATA_NEW_START;

        if ((ActiveVersion->HandleDataSize > 0) && (ActiveVersion->HandleLowAddress + ActiveVersion->HandleDataSize < PROGRAM1_AREA_END))
        {
            // Now check the program in main memory
            CalculatedChecksum = CRC32 (0, (uint8_t *)ActiveVersion->HandleLowAddress, ActiveVersion->HandleDataSize - sizeof (BINARY_HEADER) - sizeof (PROGRAM_BLOCK_INFO));
            if (CalculatedChecksum == ActiveVersion->HandleChecksum)
            {
                pMainEntry = (MAIN_FUNC)ActiveVersion->HandleEntryAddress;   // Validated executable in handle
            }
        }
    }
    else if (ACTIVE_VERSION_2 == ActiveVersionsStructVer)
    {
        ACTIVEVERSION_2 *ActiveVersion;
        ActiveVersion = (ACTIVEVERSION_2 *)ACTIVE_METADATA_NEW_START;

        if ((ActiveVersion->HandleDataSize > 0) &&
            (ActiveVersion->HandleHighAddress2 < PROGRAM2_AREA_END) &&
            (ActiveVersion->HandleLowAddress1 < ActiveVersion->HandleHighAddress1))
        {
            // Now check the program in main memory
            if ( 0 == ActiveVersion->HandleHighAddress2 )
            {
                CalculatedChecksum = CRC32 (0, (uint8_t *)ActiveVersion->HandleLowAddress1, ActiveVersion->HandleDataSize - sizeof (BINARY_HEADER) - sizeof (PROGRAM_BLOCK_INFO));
            }
            else
            {
                CalculatedChecksum = CRC32 (0,                  (uint8_t *)ActiveVersion->HandleLowAddress1, ActiveVersion->HandleHighAddress1 - ActiveVersion->HandleLowAddress1);
                CalculatedChecksum = CRC32 (CalculatedChecksum, (uint8_t *)ActiveVersion->HandleLowAddress2, ActiveVersion->HandleHighAddress2 - ActiveVersion->HandleLowAddress2);
            }

            if (CalculatedChecksum == ActiveVersion->HandleChecksum)
            {
                pMainEntry = (MAIN_FUNC)ActiveVersion->HandleEntryAddress;   // Validated executable in handle
            }
        }
    }

    return pMainEntry;  // Invalid executable in handle!
}

/* ========================================================================== */
/**
 * \brief  uint32_t GetActiveHandleTimestamp(void)
 * ...
 * \details - This function returns the timestamp of the handle's active main app:
 *                blobPointers.activeVersions.handleTimeStamp
 * ...
 * \param   < None >
 * ...
 *
 * \return  uint32_t - the timestamp of the handle's active main app.
 *
 * ========================================================================== */
uint32_t L4_GetActiveHandleTimestamp(void)
{
    return BlobPointers.ActiveVersion.HandleTimestamp;
}

/* ========================================================================== */
/**
 * \brief    uint32_t GetBlobHandleTimestamp(void)
 * ...
 * \details  This function retrieves the blob Handle app timestamp.
 * ...
 * \param   < None >
 * ...
 * \return  uint32_t - Returns blob Handle timestamp
 *
 * ========================================================================== */
uint32_t L4_GetBlobHandleTimestamp(void)
{
    return BlobPointers.StoredBlobHeader.HandleTimestamp;
}

/* ========================================================================== */
/**
 * \brief   BlobClose(void)
 * ...
 * \details Closes the blob file and file system.
 * ...
 * \param   < None >
 * ...
 * \return  None
 *
 * ========================================================================== */
void L4_BlobClose(void)
{
    FS_ERR fsError;
    FSVol_Close ("sdcard:0:", &fsError);
    FSDev_Close ("sdcard:0:", &fsError);
}

/* ========================================================================== */
/**
 * \brief   L4_CheckFPGA(void)
 * ...
 * \details Check if the FPGA needs to update, and if so, update it.
 * ...
 * \param   < None >
 * ...
 * \return  FPGA_MGR_STATUS
 *
 * ========================================================================== */
FPGA_MGR_STATUS L4_CheckFPGA(void)
{
    BLOB_HANDLER_STATUS Status;

    Status = ReadBlobFilePointers();

    if ( BLOB_STATUS_OK == Status )
    {
        if ( BlobPointers.StoredBlobHeader.JedTimestamp > BlobPointers.ActiveVersion.JedTimestamp )
        {
            Log (DBG, "FPGA data is older than Blob data: update FPGA");
        }
    }

    return FPGA_MGR_OK;
}


/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* Header compatible with C++ project */
}
#endif

