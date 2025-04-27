#ifndef FILE_SYS_H
#define FILE_SYS_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup FileSys
 * \{
 * \brief Public interface for File System
 *
 * \details This module defines symbolic constants as well as function prototypes
 *
 * \copyright  Copyright 2020 Covidien - Surgical Innovations.  All Rights Reserved.
 *
 * \file  FileSys.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include <fs.h>
#include <fs_file.h>
#include <fs_dev.h>
#include <fs_dir.h>
#include <fs_vol.h>
#include <fs_entry.h>
#include <fs_type.h>
#include <fs_dev_sd_card.h>
#include <fs_fat.h>

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define FS_SEEK_SET      FS_FILE_ORIGIN_START               ///< Origin is beginning of file.
#define FS_SEEK_CUR      FS_FILE_ORIGIN_CUR                 ///< Origin is current file position.
#define FS_SEEK_END      FS_FILE_ORIGIN_END                 ///< Origin is end of file.

#define FS_MODE_R        FS_FILE_ACCESS_MODE_RD             ///< “r” or “rb”
#define FS_MODE_W        FS_FILE_ACCESS_MODE_WR         |\
                         FS_FILE_ACCESS_MODE_CREATE     |\
                         FS_FILE_ACCESS_MODE_TRUNCATE       ///< “w” or “wb”
#define FS_MODE_A        FS_FILE_ACCESS_MODE_WR         |\
                         FS_FILE_ACCESS_MODE_CREATE     |\
                         FS_FILE_ACCESS_MODE_APPEND         ///< “a” or “ab”
#define FS_MODE_RP       FS_FILE_ACCESS_MODE_RD         |\
                         FS_FILE_ACCESS_MODE_WR             ///< “r+” or “rb+” or “r+b”
#define FS_MODE_WP       FS_FILE_ACCESS_MODE_RD         |\
                         FS_FILE_ACCESS_MODE_WR         |\
                         FS_FILE_ACCESS_MODE_CREATE     |\
                         FS_FILE_ACCESS_MODE_TRUNCATE       ///< “w+” or “wb+” or “w+b”
#define FS_MODE_AP       FS_FILE_ACCESS_MODE_RD         |\
                         FS_FILE_ACCESS_MODE_WR         |\
                         FS_FILE_ACCESS_MODE_CREATE     |\
                         FS_FILE_ACCESS_MODE_APPEND         ///< “a+” or “ab+” or “a+b”

#define FS_ATTRIB_RD     FS_ENTRY_ATTRIB_RD            ///< Entry is readable.
#define FS_ATTRIB_WR     FS_ENTRY_ATTRIB_WR            ///< Entry is writeable.
#define FS_ATTRIB_HIDDEN FS_ENTRY_ATTRIB_HIDDEN        ///< Entry is hidden from user-level processes.

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef struct
{
    uint8_t  u8CardType;       /* Card type.                      */
    uint8_t  u8HighCap;        /* Standard capacity/high capacity.*/
    uint8_t  u8ManufID;        /* Manufacturer ID.                */
    uint16_t u16OEM_ID;        /* OEM/Application ID.             */
    uint32_t u32ProdSN;        /* Product serial number.          */
    uint32_t u32ProdRev;       /* Product revision.               */

    uint32_t u32DevSecSize;    /* Sector size in Bytes.           */
    uint32_t u32TotSecCnt;     /* Number of total data sectors.   */
    uint32_t u32BadSecCnt;     /* Number of bad data sectors.     */
    uint32_t u32TotalSpace;    /* Total Data Space in Kilo Bytes. */
    uint32_t u32FreeSpace;     /* Free Data Space in Kilo Bytes.  */
    uint32_t u32UsedSpace;     /* Used Data Space in Kilo Bytes.  */
} FS_SDCARD_INFO;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
/* General Functions */
extern FS_ERR FsInit(void);
extern bool FsIsInitialized(void);
extern void FsLogErr(FS_ERR fsErr);

/* File Related Functions */
extern FS_ERR FsOpen(FS_FILE **pFile, int8_t *pFileName, uint8_t u8Mode);
extern FS_ERR FsRead(FS_FILE *pFile, uint8_t *pBuffer, uint32_t u32BytesToRead, uint32_t *pBytesRead);
extern FS_ERR FsWrite(FS_FILE *pFile, uint8_t *pBuffer, uint32_t u32BytesToWrite, uint32_t *pBytesWritten);
extern FS_ERR FsClose(FS_FILE *pFile);
extern FS_ERR FsGetChar(FS_FILE *pFile, uint8_t *pChar);
extern FS_ERR FsPutChar(FS_FILE *pFile, uint8_t u8Char);
extern FS_ERR FsGetLine(FS_FILE *pFile, uint8_t *pStr, uint32_t u32MaxLen);
extern FS_ERR FsSeek(FS_FILE *pFile, int32_t s32Offset, uint16_t u16Whence);
extern FS_ERR FsTell(FS_FILE *pFile, uint32_t *pCurrPosition);
extern FS_ERR FsIsEOF(FS_FILE *pFile, bool *pIsEOF);
extern FS_ERR FsSetAttrib(int8_t *pName, uint32_t u32SetAttrib);
extern FS_ERR FsClearAttrib(int8_t *pName, uint32_t u32ClearAttrib);
extern FS_ERR FsGetAttrib(int8_t *pName, uint32_t *pAttrib);
extern FS_ERR FsCopy(int8_t *pSrcFile, int8_t *pDestFile);
extern FS_ERR FsMove(int8_t *pSrcFile, int8_t *pDestFile);
extern FS_ERR FsRename(int8_t *pOldName, int8_t *pNewName);
extern FS_ERR FsDelete(int8_t *pFileName);
extern FS_ERR FsGetInfo(int8_t *pFileName, FS_ENTRY_INFO *pInfo);

/* Directory Related Functions */
extern FS_ERR FsOpenDir(int8_t *pDirName, FS_DIR **pDir);
extern FS_ERR FsIsOpenDir(int8_t *pDirName, bool *pIsOpen);
extern FS_ERR FsChangeDir(int8_t *pDirName);
extern FS_ERR FsMakeDir(int8_t *pDirName);
extern FS_ERR FsRemoveDir(int8_t *pDirName);
extern FS_ERR FsQueryDir(int8_t *pDirName, FS_ENTRY_INFO *pFindInfo);
extern FS_ERR FsRenameDir(int8_t *pOldName, int8_t *pNewName);
extern FS_ERR FsGetCWDir(int8_t *pCwdPath, uint32_t u32PathSize);
extern FS_ERR FsCloseDir(FS_DIR *pDir);
extern FS_ERR FsReadDir(FS_DIR *pDir, FS_DIR_ENTRY *pDirEntryInfo);
extern FS_ERR FsDirIsEmpty(uint8_t *pDirName, bool *pIsEmpty);

/* SD Card Related Functions */
extern FS_ERR FsIsSDCardPresent(void);
extern FS_ERR FsGetInfoSDCard(FS_SDCARD_INFO *pSDInfo);
extern FS_ERR FsFormatSDCard(void);
extern FS_ERR FsChkDskSDCard(void);

/* generic helper functions */
extern bool ForceArrayToAscii(uint8_t *SourceBuff, uint8_t *AsciiBuff, uint16_t ArraySize);
extern uint16_t BinaryArrayToHexString(uint8_t *DataIn, uint16_t DataLength, char *StrOut, uint16_t MaxStrLength, bool GetLeastSignificant, bool ReverseOrder);
extern void BinaryToHexAscii(uint8_t Val, char *StrOut);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* FILE_SYS_H */

