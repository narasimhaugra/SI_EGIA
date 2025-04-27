#ifndef L4_BLOBHANDLER_H
#define L4_BLOBHANDLER_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif  
/* ========================================================================== */
/**
 * \addtogroup L4_BlobHandler
 * \{
 * \brief   Public interface for Blob Handler.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_BlobHandler.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"               ///< Import common definitions such as types, etc
#include "L3_FpgaMgr.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define MAX_REV_STRING_LENGTH    (20u)

// These addresses must be sector aligned (0x1000) for FlashEraseSector to work.
#define PROGRAM1_AREA_START         (0x00011000u)    //
#define PROGRAM1_AREA_END           (0x0007D000u)    //
#define PROGRAM2_AREA_START         (0x10000000u)    // High section of program area.  Overlaps Blob image (can't have both)
#define PROGRAM2_AREA_END           (0x1007D000u)
#define SYSTEM_AREA_START           (0x0007D000u)    // Reserved for system data: usage counters...
#define SYSTEM_AREA_END             (0x0007F000u)    // Reserved for system data: usage counters...
#define ACTIVE_METADATA_NEW_START   (0x1007F000u)    // Metadata regarding the active programs - ACTIVEVERSION struct
#define ACTIVE_METADATA_NEW_END     (0x10080000u)
#define DEVICE_PROPERTIES_MASK_BLOB_VALID             (1u) ///< Blob is valid
#define DEVICE_PROPERTIES_MASK_ACTIVE_VERSIONS_VALID  (2u) ///< Active version is valid

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef void (*MAIN_FUNC)(void);
  
typedef enum                              /// Blob Header Function call status
{
    BLOB_STATUS_OK,                       ///<  Status Ok
    BLOB_STATUS_ERROR,                    ///<  Status Error
    BLOB_STATUS_INVALID_PARAM,            ///<  Status Invalid Parameter
    BLOB_STATUS_OVERFLOW,                 ///<  Status Overflow
    BLOB_VALIDATION_STATUS_UNKNOWN,       ///<  Blob Validation Status unknown 
    BLOB_STATUS_VALIDATED,                ///<  Blob Validation Passed
    BLOB_STATUS_BAD,                      ///<  Blob Validation Failed
} BLOB_HANDLER_STATUS;

typedef enum                                     /// Get Param Commands
{
    GETINFO_PARAM_BLOB_INVALID,                  ///<  Invalid command number
    GETINFO_PARAM_BLOB_ACTIVE_PP_TIMESTAMP,      ///<  Get Active Timestamps 
    GETINFO_PARAM_BLOB_ACTIVE_PP_BL_TIMESTAMP,   ///<  Get Active powerpack bootloader Timestamps 
    GETINFO_PARAM_BLOB_ACTIVE_FPGA_TIMESTAMP,    ///<  Get Active FPGA Timestamps
    GETINFO_PARAM_BLOB_AGILE_NUMBER,             ///<  Get Agile number
    GETINFO_PARAM_BLOB_TIMESTAMP,                ///<  Get Blob Timestamp
    GETINFO_PARAM_BLOB_FLAGS,                    ///<  Get Blob flage
    GETINFO_PARAM_BLOB_PP_REVISION,              ///<  Get Blob powerpack program revision
    GETINFO_PARAM_BLOB_PP_TIMESTAMP,             ///<  Get Blob powerpack program Timestamp
    GETINFO_PARAM_BLOB_PP_BL_REVISION,           ///<  Get Blob powerpack bootloader Revision
    GETINFO_PARAM_BLOB_PP_BL_TIMESTAMP,          ///<  Get Blob powerpack bootloader Timestamp
    GETINFO_PARAM_BLOB_FPGA_REVISION,            ///<  Get Blob FPGA Revision
    GETINFO_PARAM_BLOB_FPGA_TIMESTAMP,           ///<  Get Blob FPGA Timestamp
    GETINFO_PARAM_BLOB_ADAPT_BOOT_REVISION,      ///<  Get Blob Adapter Bootloader Revision
    GETINFO_PARAM_BLOB_ADAPT_BOOT_TIMESTAMP,     ///<  Get Blob Adapter Bootloader Timestamp
    GETINFO_PARAM_BLOB_ADAPT_EGIA_REVISION,      ///<  Get Blob EGIA program Revision
    GETINFO_PARAM_BLOB_ADAPT_EGIA_TIMESTAMP,     ///<  Get Blob EGIA program Timestamp
    GETINFO_PARAM_BLOB_ADAPT_EEA_REVISION,       ///<  Get Blob EEA program Revision
    GETINFO_PARAM_BLOB_ADAPT_EEA_TIMESTAMP,      ///<  Get Blob EEA program Timestamp
    GETINFO_PARAM_PP_USE_COUNTS,                 ///<  Get Total use counts
    GETINFO_PARAM_BLOB_SYS_VERSION,              ///<  Get System Version
    GETINFO_PARAM_BLOB_LAST
} BLOB_GETINFO_PARAM;

typedef enum                                         /// Blob Section
{
    BLOB_SECTION_UNKNOWN,
    BLOB_SECTION_HANDLE_BL,                          ///< Handle Bootloader Section
    BLOB_SECTION_HANDLE_MAIN,                        ///< Handle Main Section
    BLOB_SECTION_JED_CONFIG,                         ///< JED Configuration Section
    BLOB_SECTION_JED_UFM,                            ///< JED UFM Section
    BLOB_SECTION_ADAPT_BL,                           ///< Adapter Bootloader Section
    BLOB_SECTION_EGIA_MAIN,                          ///< EGIA Main Section
    BLOB_SECTION_EEA_MAIN,                           ///< EEa Main Section
    BLOB_SECTION_COUNT,
} BLOB_SECTION;

typedef enum                                     ///< FLASH CRC STATUS
{
  FLASH_CRCVALIDATION_UNKNOWN,                   ///< FLASH CRC Validation idle
  FLASH_CRCVALIDATION_INPROGRESS,                ///< FLASH CRC Validation In Progress
  FLASH_CRC_VALIDATED_GOOD,                      ///< FLASH CRC Validated Status GOOD
  FLASH_CRC_VALIDATED_BAD,                       ///< FLASH CRC Validated Status BAD
} FLASH_CRCVALIDATION_STATUS;

typedef enum
{
    FLASH_PROGRAM_OK = 0,
    FLASH_PROGRAM_ERROR,
    FLASH_PROGRAM_ERROR_READ,
    FLASH_PROGRAM_ERROR_WRITE,
    FLASH_PROGRAM_ERROR_ERASE,
    FLASH_PROGRAM_ERROR_BLOCK_INDEX   
} FLASH_PROGRAM_STATUS;

typedef union
{
    struct
    {
        unsigned int BlobValid : 1;                ///< SD Card blob is valid
        unsigned int BlobNewerTimestamp : 1;       ///< SD Card blob has newer timestamp than handle flash
        unsigned int HandleMainCorrupt : 1;        ///< Handle flash Main app is corrupted
        unsigned int HandleUpdate : 1;             ///< Handle flash was updated from SD Card blob
        unsigned int HandleUpdateSuccess : 1;      ///< Handle flash update successful
        unsigned int BlobEncrypted : 1;            ///< SD Card blob is encrypted
        unsigned int ReservedB6 : 1;               ///< Reserved for future use
        unsigned int ReservedB7 : 1;               ///< Reserved for future use
    } bit;
    uint8_t Status;
} BOOT_FLAGS;                                   ///< Boot status

typedef union
{
    struct
    {
        unsigned int HandleEncrypted : 1;          ///< Handle binary is encrypted
        unsigned int HandleBLEncrypted : 1;        ///< Handle bootloader binary is encrypted
        unsigned int AdapterBLEncrypted : 1;       ///< adapter bootloader binary encrypted
        unsigned int EGIAEncrypted : 1;            ///< EGIA binary encrypted
        unsigned int EEAEncrypted : 1;             ///< EEA binary encrypted
        unsigned int FPGAEncrypted : 1;            ///< FPGA binary encrypted
        unsigned int Reserved1 : 1;                ///< Reserved for future use
        unsigned int Reserved2 : 1;                ///< Reserved for future use
    } bit;
    uint8_t Status;
} BLOB_ENCRYPTION;                             ///< Blob encryption status data   

typedef enum
{
    ACTIVE_VERSION_INVALID = 0,
    ACTIVE_VERSION_1 = 1,
    ACTIVE_VERSION_2 = 2,
    LATEST_ACTIVE_VERSION_STRUCT = 2
} ACTIVE_VERSION;

/// \todo 03/12/2021 CPK Blob strucutes should be limited to L4 device manager. Ensure these are not exposed to Applications
typedef struct                                   /// Active Version Ver-1 Struct
{
    uint32_t StructChecksum;                     ///< Overall Checksum of this structure
    uint32_t HandleTimestamp;                    ///<Handle program Timestamp
    uint32_t HandleChecksum;                     ///<Handle program Checksum
    uint32_t HandleDataSize;                     ///<Data Size -  Includes BINARY_HEADER structure and program data
    uint32_t HandleEntryAddress;                 ///<Handle Entry Address in Flash
    uint32_t HandleLowAddress;                   ///<Handle Low Address in Flash
    uint32_t HandleBLTimestamp;                  ///<Handle Bootloader Timestamp
    uint32_t JedTimestamp;                       ///<FPGA  binary Timestamp
    uint32_t AdaptBLTimestamp;                   ///<Adapter bootloader Timestamp
    uint32_t EgiaTimestamp;                      ///<EGIA  binary Timestamp
    uint32_t EeaTimestamp;                       ///<EEA   binary Timestamp
} ACTIVEVERSION_1;

typedef struct                                   ///Active Version Ver-2 Struct
{
    uint32_t StructChecksum;                     ///<Overall Checksum of this structure
    ACTIVE_VERSION StructVersion;                ///< Version
    uint32_t StructSize;                         ///<Total size of this structure
    uint32_t HandleTimestamp;                    ///<Handle program Timestamp
    uint32_t HandleChecksum;                     ///<Handle program Checksum
    uint32_t HandleDataSize;                     ///< total Program size - Includes BINARY_HEADER and program
    uint32_t HandleEntryAddress;                 ///<Handle Entry Address in Flash
    uint32_t HandleLowAddress1;                  ///<Handle Low Address in Flash Block-1
    uint32_t HandleHighAddress1;                 ///<Handle High Address in Flash Block-1
    uint32_t HandleLowAddress2;                  ///<Handle Low Address in Flash Block-2
    uint32_t HandleHighAddress2;                 ///<Handle High Address in Flash Block-2
    uint32_t HandleBLTimestamp;                  ///<Handle Bootloader Timestamp
    uint32_t JedTimestamp;                       ///<FPGA  binary Timestamp
    uint32_t AdaptBLTimestamp;                   ///<Adapter bootloader Timestamp
    uint32_t EgiaTimestamp;                      ///<EGIA  binary Timestamp
    uint32_t EeaTimestamp;                       ///<EEA   binary Timestamp
} ACTIVEVERSION_2;

typedef ACTIVEVERSION_2 ACTIVEVERSION;           ///< This configures the build to use ACTIVEVERSION_2 format

typedef struct                                    ///Blob Header
{
    uint32_t BlobChecksum;                       ///< Checksum of entire blob
    uint32_t BlobVersion;                        ///< Blob version
    uint32_t BlobHeaderSize;                     ///< Size of this structure
    uint32_t BlobTimestamp;                      ///< Blob Timestamp
    uint32_t BlobFlags;                          ///< Blob Flage
    uint32_t HandleTimestamp;                    ///< Blob Timestamp
    uint32_t HandleDataSize;                     ///< Blob Data size - BINARY_HEADER structure as well as the program data
    uint32_t HandleBLTimestamp;                  ///< Handle Bootloader Timestamp
    uint32_t HandleBLDataSize;                   ///< Bootloader Datasize - BINARY_HEADER structure as well as the program data
    uint32_t JedTimestamp;                       ///< FPGA  binary Timestamp
    uint32_t JedDataSize;                        ///< FPGA Datasize - MACHX02 structure as well as all the data
    uint32_t AdaptBLTimestamp;                   ///< Adapter bootloader Timestamp
    uint32_t AdaptBLDataSize;                    ///< Adapter Bootloader Data size - BINARY_HEADER structure as well as the program data
    uint32_t EgiaTimestamp;                      ///< EGIA  binary Timestamp
    uint32_t EgiaDataSize;                       ///< EGIA  binary Data size - BINARY_HEADER structure as well as the program data
    uint32_t EeaTimestamp;                       ///< EEA   binary Timestamp
    uint32_t EeaDataSize;                        ///< EEA Datasize - BINARY_HEADER structure as well as the program data

    uint8_t    		BlobAgileNumber[MAX_REV_STRING_LENGTH];   	///< Agile Number
    uint8_t    		BlobPowerPackRev[MAX_REV_STRING_LENGTH];  	///< Powerpack Revision
    uint8_t    		BlobPowerPackBLRev[MAX_REV_STRING_LENGTH];	///< Powerpack Bootloader Revision
    uint8_t    		BlobJedRev[MAX_REV_STRING_LENGTH];        	///< FPGA JED Revision
    uint8_t    		BlobAdapterBLRev[MAX_REV_STRING_LENGTH];  	///< Adapter  Revision
    uint8_t    		BlobAdapterEGIARev[MAX_REV_STRING_LENGTH];	///< EGIA Revision  
    uint8_t    		BlobAdapterEEARev[MAX_REV_STRING_LENGTH]; 	///< EEA Revision
    uint8_t    		BlobSystemVersion[MAX_REV_STRING_LENGTH]; 	///< System Version
    BLOB_ENCRYPTION	Encryption;                           		///< blob encryption status data
} BLOB_HEADER;

typedef struct                                           ///Binary Header
{
    uint32_t HeaderVersion;                              ///< Header Version of the Binary
    uint32_t HeaderSize;                                 ///< Size of this structure
    uint32_t ProgramChecksum;                            ///< Checksum of ONLY the program 
    uint32_t TotalDataSize;                              ///< Size of Program -  Includes the program itself, the BINARY_HEADER struct, and all the ProgramBlockInfo structs.
    uint32_t ProgramEntryAddress;                        ///< Program Entry address of Binary in Flash
    uint32_t ProgramLowAddress;                          ///< Program Low address of Binary in Flash
    uint32_t ProgramHighAddress;                         ///< Program High address of Binary in Flash
    uint32_t BlockCount;                                 ///< Block count of the binary
} BINARY_HEADER;

typedef struct                                          ///Blob pointers structure
{
    ACTIVEVERSION  ActiveVersion;                       ///< Active Version Data of Blob
    BLOB_HEADER    StoredBlobHeader;                    ///< Blob Header information
    BINARY_HEADER  StoredHandleHeader;                  ///< Handle Program Header
    BINARY_HEADER  StoredHandleBLHeader;                ///< Handle Bootloader Header
    MACHX02        JedInfo;                             ///< FPGA - JED Info 
    uint8_t *      JedData;                             ///< FPGA  Data
    BINARY_HEADER  StoredAdaptBLHeader;                 ///< Adapter Bootloader Header
    BINARY_HEADER  StoredEgiaHeader;                    ///< EGIA Header
    BINARY_HEADER  StoredEeaHeader;                     ///< EEA Header
    uint32_t       HandleMainOffset;                    ///< Handle main app Offset
    uint32_t       HandleBootloaderOffset;              ///< Handle Bootloader Offser
    uint32_t       JedDataOffset;                       ///< FPGA's jed data Offset
    uint32_t       AdapterBootloaderOffset;             ///< adapter ootloader Offset
    uint32_t       EgiaMainOffset;                      ///< EGIA main app Offset
    uint32_t       EeaMainOffset;                       ///< EEA main app Offset
} BLOB_POINTERS;

typedef struct             /// Block information
{
    INT32U AbsoluteAddress;    ///< Absolute adress of the program
    INT32U Length;             ///< Size of the block
} PROGRAM_BLOCK_INFO;

typedef struct                           ///< Crc Info
{
  uint32_t CrcCalculated;               ///< CRC calculated for Section
  uint32_t CrcCalculatedMemSize;        ///< CRC calculated for Memory size
  bool     Area1CrcDone;                ///< Area1 CRC is calculated
} CRC_INFO;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
BLOB_HANDLER_STATUS 	   L4_CheckHandleBootloader(void);
BLOB_HANDLER_STATUS 	   L4_BlobHandlerInit(void);
BLOB_HANDLER_STATUS 	   L4_BlobErase(uint16_t *pFsError);
BLOB_HANDLER_STATUS 	   L4_BlobWrite (uint8_t *pData, uint32_t Offset ,uint32_t NumOfBytes);
BLOB_HANDLER_STATUS 	   L4_BlobRead (BLOB_SECTION BlobSection, uint8_t *pData, uint32_t Offset ,uint32_t DataSize, uint32_t *pBytesRead);
BLOB_HANDLER_STATUS 	   L4_BlobValidate (bool ForceCheck);
BLOB_HANDLER_STATUS 	   L4_BlobGetInfo (BLOB_GETINFO_PARAM ParamId, uint8_t *pData, uint16_t *pResponseSize);
BLOB_HANDLER_STATUS 	   L4_GetBlobPointers (BLOB_POINTERS *pBlobPointers);
bool                       EraseHandleTimestamp(void);
bool                       EraseHandleBLTimestamp(void);
bool                       FPGA_EraseTimestamp(void);
uint32_t                   FPGA_GetTimestamp(void);
bool                       FPGA_SetTimestamp(uint32_t Timestamp);
void                       L4_BlobClose(void);
uint32_t                   L4_GetActiveHandleTimestamp(void);
uint32_t                   L4_GetBlobHandleTimestamp(void);
MAIN_FUNC                  L4_ValidateHandleMainApp(void);
FLASH_PROGRAM_STATUS 	   L4_UpgradeHandleMainApp(void);
FLASH_CRCVALIDATION_STATUS L4_ValidateMainAppFromFlash(CRC_INFO *pCrcHandle);
BLOB_HANDLER_STATUS        L4_ValidateFlashActiveVersionStruct(void);
FPGA_MGR_STATUS            L4_CheckFPGA(void);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif  /* L4_BLOBHANDLER_H */

