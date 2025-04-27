#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup FileSys
 * \{
 * \brief      FILE_SYS
 *
 * \details    This module wraps the File System Support from Micrium
 *
 * \copyright  2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    FileSys.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Common.h"
#include "Board.h"
#include "FileSys.h"
#include "Logger.h"
#include "FaultHandler.h"  
#include <fs.h>
#include <fs_file.h>
#include <fs_vol.h>
#include <fs_entry.h>
#include <fs_type.h>
#include <fs_dev_sd_card.h>
#include <fs_fat.h>
#include "TestManager.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_FILE_SYS)    ///< Log Group Identifier
#define SD_CARD_INSTALLED_MASK  (0x400u)                ///< SD Card GPIO Mask
#define BYTES_PER_KB            (1024u)                 ///< Bytes per KiloBytes
#define FS_SW_VER_CON_FACTOR    (10000u)                ///< uC/FS SW Ver Conversion Factor [Refer fs.h]
#define NEWLINE_CHAR            (0x0Au)                 ///< New Line character '\n'
#define SD_CARD_DEVICE          ("sdcard:0:")           ///< SD Card Device
#define PERCENT_100             (100.0f)                ///< 100 percent
#define PERCENT_25              (25.0f)                 ///< 25 percent
#define PERCENT_10              (10.0f)                 ///< 10 percent
#define FS_LOWLIMIT             (5.0f)                  ///< File system low limit, 5 percent
#define CLEANUP_TASK_STACK_SIZE (256u)                  ///< cleanup task size
#define MIN_STRING_OUT_SIZE     (3u)                    ///< Minimum length of output string

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
OS_STK CleanupTaskStack[CLEANUP_TASK_STACK_SIZE + MEMORY_FENCE_SIZE_DWORDS];   ///< Cleanup Task Stack

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef struct                  /// Fs Error String Entry
{
    FS_ERR      fsErr;          ///< Fs Error Value
    uint8_t     *pFsErrString;  ///< Fs Error String
} FS_ERR_STR_T;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static bool mbFsInitialized;    ///< Filesystem Init Status Variable

//#pragma location=".sram"


static FS_FILE_SIZE RdfCleanupSize;                        ///< Calculated size required to free memory

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
static FS_ERR MonitorSDCardFreeSpace(void);

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Initialize the file system. Check if a SD card is present.
 *
 * \details This function initializes the File System.
 *
 * \param   < None >
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, refer fs_err.h for full list
 *
 * ========================================================================== */
FS_ERR FsInit(void)
{
    FS_ERR fsErr;                       /* FS Status Return */
    const FS_CFG App_FS_Cfg =
    {
        APP_CFG_FS_DEV_CNT,             // DevCnt
        APP_CFG_FS_VOL_CNT,             // VolCnt
        APP_CFG_FS_FILE_CNT,            // FileCnt
        APP_CFG_FS_DIR_CNT,             // DirCnt
        APP_CFG_FS_BUF_CNT,             // BufCnt
        APP_CFG_FS_DEV_DRV_CNT,         // DevDrvCnt
        APP_CFG_FS_MAX_SEC_SIZE         // MaxSecSize
    };

    do
    {   /* Is Filesystem already initialized ? */
        if (mbFsInitialized)
        {
            /* Return Error. */
            fsErr = FS_ERR_OS_INIT;
            break;
        }

        /* Initializes µC/FS and must be called prior
        to calling any other µC/FS API functions */
        fsErr = FS_Init ((FS_CFG*)&App_FS_Cfg);

        /* Check the Error. */
        if (FS_ERR_NONE != fsErr)
        {
            Log (REQ, "FileSys: uC/FS Init Failed");
            FsLogErr (fsErr);
            break;
        }
        else
        {
            Log (REQ, "FileSys: uC/FS Init Done");
        }

        /*  Add SD/MMC (Card) device driver to the file system*/
        FS_DevDrvAdd ((FS_DEV_API*)&FSDev_SD_Card, &fsErr);

        /* Check the Error. */
        if (FS_ERR_NONE != fsErr)
        {
            Log (REQ, "FileSys: Error! SD Card Device Driver");
            FsLogErr (fsErr);
            break;
        }
        else
        {
            Log (REQ, "FileSys: Added SD Card Device Driver");
        }
        fsErr = FsIsSDCardPresent();
        /* Only add card if present */
        if (FS_ERR_NONE != fsErr)
        {
            Log (REQ, "FileSys: Error! SD Card Not Present");
            FsLogErr (fsErr);
            break;
        }
        else
        {
            Log (REQ, "FileSys: SD Card Detected");
        }

        /* Open device SD_CARD_DEVICE  */
        FSDev_Open ((CPU_CHAR *)SD_CARD_DEVICE, (void*)0, &fsErr);

        /* Check the Error. */
        if (FS_ERR_NONE != fsErr)
        {
            Log (REQ, "FileSys: Error! Device Open, \"sdcard:0:\"");
            FsLogErr (fsErr);
            break;
        }
        else
        {
            Log (REQ, "FileSys: Device Opened, \"sdcard:0:\"");
        }

        /* Volume Open device SD_CARD_DEVICE  */
        FSVol_Open ((CPU_CHAR *)SD_CARD_DEVICE, (CPU_CHAR *)SD_CARD_DEVICE, 0, &fsErr);

        /* Check the Error. */
        if (FS_ERR_NONE != fsErr)
        {
            Log (REQ, "FileSys: Error! Volume Open, \"sdcard:0:\"");
            FsLogErr (fsErr);
            break;
        }
        else
        {
            Log (REQ, "FileSys: Volume Opened, \"sdcard:0:\"");
        }

        fsErr = MonitorSDCardFreeSpace();
      
        Log (REQ, "FileSys: uC/FS Software Version = %.4f", ((float32_t)FS_VersionGet()/FS_SW_VER_CON_FACTOR));
        Log (REQ, "FileSys: Initialized");

        /* Yes, All Good, now set the global variable. */
        mbFsInitialized = true;

    } while (false);
    /* SD CARD NOT PRESENT */
    if (FS_ERR_DEV_NONE_AVAIL == fsErr )
    {
        FaultHandlerSetFault(SDCARD_NOTPRESENT, SET_ERROR);
    }

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Test FsInit status.
 *
 * \details Check if Filesystem is Initialized or not.
 *
 * \param   < None >
 *
 * \return  Init status: true, if Initialized else false.
 *
 * ========================================================================== */
bool FsIsInitialized(void)
{
    return mbFsInitialized;
}

/* ========================================================================== */
/**
 * \brief   Open a file.
 *
 * \param   *pFile    - Pointer to the file.
 * \param   pFileName - File Name
 * \param   u8Mode    - File access Mode
 *                      FS_MODE_R  “r” or “rb”
 *                      FS_MODE_W  “w” or “wb”
 *                      FS_MODE_A  “a” or “ab”
 *                      FS_MODE_RP “r+” or “rb+” or “r+b”
 *                      FS_MODE_WP “w+” or “wb+” or “w+b”
 *                      FS_MODE_AP “a+” or “ab+” or “a+b”
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                        File opened successfully.
 *          FS_ERR_NAME_NULL                   Argument 'name_full' passed a NULL ptr.
 *          FS_ERR_FILE_NONE_AVAIL             No file available.
 *          FS_ERR_FILE_INVALID_ACCESS_MODE    Access u8Mode is specified invalid.
 *
 *                                             RETURNED BY FSFile_NameParseChk()
 *          FS_ERR_NAME_INVALID                Entry name could not be parsed, lacks an
 *                                             initial path separator character or
 *                                             includes an invalid volume name.
 *          FS_ERR_NAME_PATH_TOO_LONG          Entry path is too long.
 *          FS_ERR_VOL_NOT_OPEN                Volume not opened.
 *          FS_ERR_VOL_NOT_MOUNTED             Volume not mounted.
 *
 *                                             RETURNED BY FSSys_FileOpen()
 *          FS_ERR_BUF_NONE_AVAIL              Buffer not available.
 *          FS_ERR_DEV                         Device access error.
 *          FS_ERR_DEV_FULL                    Device is full (space could not be allocated).
 *          FS_ERR_DIR_FULL                    Directory is full (space could not be allocated).
 *          FS_ERR_ENTRY_CORRUPT               File system entry is corrupt.
 *          FS_ERR_ENTRY_EXISTS                File system entry exists.
 *          FS_ERR_ENTRY_NOT_FILE              File system entry NOT a file.
 *          FS_ERR_ENTRY_NOT_FOUND             File system entry NOT found
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND      Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR        Entry parent NOT a directory.
 *          FS_ERR_ENTRY_RD_ONLY               File system entry marked read-only.
 *          FS_ERR_NAME_INVALID                Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsOpen(FS_FILE **pFile, int8_t *pFileName, uint8_t u8Mode)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input File Pointer Valid ? */
        if (NULL == pFile)
        {
            fsErr = FS_ERR_NULL_PTR;
            break;
        }

        /* Is Input Filename Valid ? */
        if (NULL == pFileName)
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Call the corresponding µC/FS API */
        *pFile = FSFile_Open ((CPU_CHAR *)pFileName, u8Mode, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Read from a file.
 *
 * \param   pFile - Pointer to a file.
 * \param   pBuffer - Pointer to source buffer.
 * \param   u32BytesToRead - Number of bytes to Read.
 * \param   pBytesRead - Number of bytes to Read.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                   File read successfully.
 *          FS_ERR_NULL_PTR               Argument 'pFile'/'pBuffer' passed a NULL ptr.
 *          FS_ERR_INVALID_TYPE           Argument 'pFile's TYPE is invalid or unknown.
 *          FS_ERR_FILE_ERR               File has error.
 *          FS_ERR_FILE_INVALID_OP        Invalid operation on file.
 *          FS_ERR_FILE_INVALID_OP_SEQ    Invalid operation sequence on file.
 *
 *                                        RETURNED BY FSFile_AcquireLockChk()
 *          FS_ERR_DEV_CHNGD              Device has changed.
 *          FS_ERR_FILE_NOT_OPEN          File NOT open.
 *
 *                                        RETURNED BY FSSys_FileRd()
 *          FS_ERR_BUF_NONE_AVAIL         No buffer available.
 *          FS_ERR_DEV                    Device access error.
 *          FS_ERR_ENTRY_CORRUPT          File system entry corrupt.
 *
 *
 * ========================================================================== */
FS_ERR FsRead(FS_FILE *pFile, uint8_t *pBuffer, uint32_t u32BytesToRead, uint32_t *pBytesRead)
{
    FS_ERR   fsErr;         /* FS Status Return */
    uint32_t u32BytesRead;  /* Bytes Read */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input File Pointer Valid? */
        if ((NULL == pFile) || (NULL == pBuffer))
        {
            fsErr = FS_ERR_NULL_PTR;
            break;
        }

        TM_Hook(HOOK_SDRDSTART, NULL);
        /* Call the corresponding µC/FS API */
        u32BytesRead = FSFile_Rd (pFile, pBuffer, u32BytesToRead, &fsErr);
        (FS_ERR_NONE == fsErr) ? TM_Hook (HOOK_SDRDEND, &u32BytesRead) : TM_Hook (HOOK_SDRDSTART, NULL);

        TM_Hook (HOOK_FILESYSTEMERROR, &fsErr);

        /* Return Bytes Read Count */
        if (NULL != pBytesRead)
        {
            *pBytesRead = u32BytesRead;
        }

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Write to a file.
 *
 * \param   pFile - Pointer to a file.
 * \param   pBuffer - Pointer to source buffer.
 * \param   u32BytesToWrite - Number of bytes to write.
 * \param   pBytesWritten - Number of bytes to Written.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                   File written successfully.
 *          FS_ERR_NULL_PTR               Argument 'pFile'/'pBuffer' passed a NULL ptr.
 *          FS_ERR_INVALID_TYPE           Argument 'pFile's TYPE is invalid or
 *                                        unknown.
 *          FS_ERR_FILE_ERR               File has error.
 *          FS_ERR_FILE_INVALID_OP        Invalid operation on file.
 *          FS_ERR_FILE_INVALID_OP_SEQ    Invalid operation sequence on file.
 *          FS_ERR_FILE_OVF               File size/position would be overflowed if
 *                                                                 write were executed.
 *
 *                                        RETURNED BY FSFile_AcquireLockChk()
 *          FS_ERR_DEV_CHNGD              Device has changed.
 *          FS_ERR_FILE_NOT_OPEN          File NOT open.
 *
 *                                        RETURNED BY FSSys_FileWr()
 *                                        RETURNED BY FSSys_PosSet()
 *          FS_ERR_BUF_NONE_AVAIL         No buffer available.
 *          FS_ERR_DEV                    Device access error.
 *          FS_ERR_DEV_FULL               Device is full (no space could be allocated).
 *          FS_ERR_ENTRY_CORRUPT          File system entry is corrupt.
 *
 * ========================================================================== */
FS_ERR FsWrite(FS_FILE *pFile, uint8_t *pBuffer, uint32_t u32BytesToWrite, uint32_t *pBytesWritten)
{
    FS_ERR   fsErr;             /* FS Status Return */
    uint32_t u32BytesWritten;   /* Bytes Written */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input File Pointer Valid? */
        if ((NULL == pFile) || (NULL == pBuffer))
        {
            fsErr = FS_ERR_NULL_PTR;
            break;
        }

        TM_Hook(HOOK_SDWRSTART, NULL);
        /* Call the corresponding µC/FS API */
        u32BytesWritten = FSFile_Wr (pFile, pBuffer, u32BytesToWrite, &fsErr);
        (FS_ERR_NONE == fsErr) ? TM_Hook (HOOK_SDWREND, &u32BytesWritten) : TM_Hook (HOOK_SDWRSTART, NULL);

        /* Return Bytes Written Count */
        if (NULL != pBytesWritten)
        {
            *pBytesWritten = u32BytesWritten;
        }

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Close a file.
 *
 * \param   pFile - Pointer to a file.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE             File closed.
 *          FS_ERR_NULL_PTR         Argument 'pFile' passed a NULL pointer.
 *          FS_ERR_INVALID_TYPE     Argument 'pFile's TYPE is invalid or unknown.
 *
 *                                  RETURNED BY FSFile_AcquireLockChk()
 *          FS_ERR_FILE_NOT_OPEN    File NOT open.
 *
 *                                  RETURNED BY FSFile_BufEmpty()
 *          FS_ERR_BUF_NONE_AVAIL   No buffer available.
 *          FS_ERR_DEV              Device access error.
 *          FS_ERR_DEV_FULL         Device is full (no space could be allocated).
 *          FS_ERR_ENTRY_CORRUPT    File system entry is corrupt.
 *
 * ========================================================================== */
FS_ERR FsClose(FS_FILE *pFile)
{
    FS_ERR fsErr;  /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input File Pointer Valid? */
        if (NULL == pFile)
        {
            fsErr = FS_ERR_NULL_PTR;
            break;
        }

        /* Call the corresponding µC/FS API */
        FSFile_Close (pFile, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Reads a single character from the current position in the file
 *
 * \param   pFile - Pointer to a file.
 * \param   pChar - Character read.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, refer fs_err.h for full list
 *
 *          FS_ERR_NULL_PTR  Argument 'pFile'/'pChar' passed a NULL ptr.
 *
 * ========================================================================== */
FS_ERR FsGetChar(FS_FILE *pFile, uint8_t *pChar)
{
    FS_ERR fsErr;  /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input File Pointer Valid? */
        if ((NULL == pFile) || (NULL == pChar))
        {
            fsErr = FS_ERR_NULL_PTR;
            break;
        }

        /* Call the corresponding µC/FS API */
        (void) FSFile_Rd (pFile, pChar, 1, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Writes a single character from the current position in the file.
 *
 * \param   pFile - Pointer to a file.
 * \param   u8Char - Character to write.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, refer fs_err.h for full list
 *
 *          FS_ERR_NULL_PTR  Argument 'pFile'/'pChar' passed a NULL ptr.
 *
 * ========================================================================== */
FS_ERR FsPutChar(FS_FILE *pFile, uint8_t u8Char)
{
    FS_ERR fsErr;  /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input File Pointer Valid? */
        if (NULL == pFile)
        {
            fsErr = FS_ERR_NULL_PTR;
            break;
        }

        /* Call the corresponding µC/FS API */
        (void) FSFile_Wr (pFile, &u8Char, 1, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Reads a line from the specified file.
 *
 * \details This function reads from the current position in the target file
 *          and copies it into the buffer pointed by the pStr. It will stop
 *          reading from the file when (u32MaxLen - 1) characters have been
 *          read, a new line is read, or an end-of-file is reached (whichever
 *          comes first).
 *
 * \n       A Null character is always appended to the end of the output string.
 *
 * \param   pFile - Pointer to a file.
 * \param   pStr - Pointer to a String.
 * \param   u32MaxLen - Max String Length.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, refer fs_err.h for full list
 *
 * ========================================================================== */
FS_ERR FsGetLine(FS_FILE *pFile, uint8_t *pStr, uint32_t u32MaxLen)
{
    FS_ERR fsErr;       /* FS Status Return */
    uint32_t u32Index;  /* Index */

    fsErr = FS_ERR_NONE; // Initialize variable

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input File Pointer Valid? */
        if ((NULL == pFile) || (NULL == pStr))
        {
            fsErr = FS_ERR_NULL_PTR;
            break;
        }

        /* Terminate the buffer. */
        pStr[u32MaxLen] = NUL;

        /* Iterate for as long as the buffer or we get EOF. */
        for (u32Index = 0; u32Index < u32MaxLen; u32Index++)
        {
            /* Grab the character. */
            fsErr = FsGetChar (pFile, &pStr[u32Index]);

            /* Check the FS error return or Null character. */
            if ((FS_ERR_NONE != fsErr) || (NUL == pStr[u32Index]))
            {
                /* Yes. Return. */
                break;
            }

            /* Check EOF */
            if (FSFile_IsEOF (pFile, &fsErr))
            {
                /* Terminate the buffer. */
                pStr[u32Index] = NUL;
                break;
            }

            /* Check for the new line character. */
            if (NEWLINE_CHAR == pStr[u32Index])
            {
                /* Yes. Finish the string and return. */
                pStr[u32Index + 1] = NUL;
                break;
            }
        }

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Set file position indicator.
 *
 * \param   pFile - Pointer to a file.
 * \param   s32Offset - Offset from file position specified by 'whence'.
 * \param   u16Whence - Reference position for offset :
 *                      FS_SEEK_SET - Offset is from the beginning of the file.
 *                      FS_SEEK_CUR - Offset is from current file position.
 *                      FS_SEEK_END - Offset is from the end of the file.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, refer fs_err.h for full list
 *
 * ========================================================================== */
FS_ERR FsSeek(FS_FILE *pFile, int32_t s32Offset, uint16_t u16Whence)
{
    FS_ERR fsErr;  /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input File Pointer Valid? */
        if (NULL == pFile)
        {
            fsErr = FS_ERR_NULL_PTR;
            break;
        }

        /* Call the corresponding µC/FS API */
        FSFile_PosSet (pFile, s32Offset, u16Whence, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Get file position indicator.
 *
 * \param   pFile - Pointer to a file.
 * \param   pCurrPosition - Return position.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE             File position gotten successfully.
 *          FS_ERR_NULL_PTR         Argument 'pFile' passed a NULL pointer.
 *          FS_ERR_INVALID_TYPE     Argument 'pFile's TYPE is invalid or unknown.
 *          FS_ERR_FILE_ERR         File has error.
 *
 *                                  RETURNED BY FSFile_AcquireLockChk()
 *          FS_ERR_DEV_CHNGD        Device has changed.
 *          FS_ERR_FILE_NOT_OPEN    File NOT open.
 *
 * ========================================================================== */
FS_ERR FsTell(FS_FILE *pFile, uint32_t *pCurrPosition)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input File Pointer Valid? */
        if ((NULL == pFile) || (NULL == pCurrPosition))
        {
            fsErr = FS_ERR_NULL_PTR;
            break;
        }

        /* Call the corresponding µC/FS API */
        *pCurrPosition = FSFile_PosGet (pFile, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Test EOF indicator on a file.
 *
 * \param   pFile - Pointer to a file.
 * \param   pIsEOF - Return boolean, true if EOF, else false.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE             EOF indicator obtained.
 *          FS_ERR_NULL_PTR         Argument 'pFile' passed a NULL ptr.
 *          FS_ERR_INVALID_TYPE     Argument 'pFile's TYPE is invalid or unknown.
 *
 *                                  RETURNED BY FSFile_AcquireLockChk()
 *          FS_ERR_DEV_CHNGD        Device has changed.
 *          FS_ERR_FILE_NOT_OPEN    File NOT open.
 *
 * ========================================================================== */
FS_ERR FsIsEOF(FS_FILE *pFile, bool *pIsEOF)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input File Pointer Valid? */
        if ((NULL == pFile) || (NULL == pIsEOF))
        {
            fsErr = FS_ERR_NULL_PTR;
            break;
        }

        /* Call the corresponding µC/FS API */
        *pIsEOF = FSFile_IsEOF (pFile, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Get Attrib information about a file or directory.
 *
 * \param   pName - Name of the entry.
 * \param   pAttrib - Pointer to structure that will receive the Attrib information.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                      Entry information gotten successfully.
 *          FS_ERR_NAME_NULL                 Argument 'pName' passed a NULL ptr.
 *          FS_ERR_NULL_PTR                  Argument 'pAttrib' passed a NULL ptr.
 *
 *                                           RETURNED BY FSEntry_NameParseChk() -
 *          FS_ERR_NAME_INVALID              Entry name specified invalid OR volume
 *                                                                    could not be found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Entry name specified too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume was not open.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume was not mounted.
 *
 *                                           RETURNED BY FSSys_EntryQuery()
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRY_NOT_FOUND           File system entry NOT found
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsGetAttrib(int8_t *pName, uint32_t *pAttrib)
{
    FS_ERR          fsErr; /* FS Status Return */
    FS_ENTRY_INFO   xInfo; /* FS Entry Info */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if ((NULL == pName) || (NULL == pAttrib))
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Call the corresponding µC/FS API */
        FSEntry_Query ((CPU_CHAR *)pName, &xInfo, &fsErr);

        if (FS_ERR_NONE != fsErr)
        {
            break;
        }

        /* Copy the File Attribs */
        *pAttrib = xInfo.Attrib;

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Set a file or directory's attributes.
 *
 * \param   *pName - Name of the entry.
 * \param   u32SetAttrib - Entry attributes to set.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                      Entry attributes set successfully.
 *          FS_ERR_FILE_INVALID_ATTRIB       Entry attributes specified invalid.
 *          FS_ERR_NAME_NULL                 Argument 'pName' passed a NULL ptr.
 *
 *          FS_ERR_ENTRY_ROOT_DIR            File system entry is a root directory.
 *          FS_ERR_VOL_INVALID_OP            Invalid operation on volume.
 *
 *                                           RETURNED BY FSEntry_NameParseChk()
 *          FS_ERR_NAME_INVALID              Entry name specified invalid OR volume
 *                                           could not be found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Entry name specified too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume was not open.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume was not mounted.
 *
 *                                           RETURNED BY FSSys_EntryAttribSet()
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRY_NOT_FOUND           File system entry NOT found
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsSetAttrib(int8_t *pName, uint32_t u32SetAttrib)
{
    FS_ERR      fsErr;      /* FS Status Return */
    uint32_t    u32Attrib;  /* Local Attrib */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if (NULL == pName)
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Get the current Attributes */
        u32Attrib = 0;
        fsErr = FsGetAttrib (pName, &u32Attrib);
        if (FS_ERR_NONE != fsErr)
        {
            break;
        }

        /* Set the Attrib.*/
        u32Attrib |= u32SetAttrib;

        /* Call the corresponding µC/FS API */
        FSEntry_AttribSet ((CPU_CHAR *)pName, u32Attrib, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Clear a file or directory's attributes.
 *
 * \param   *pName - Name of the entry.
 * \param   u32ClearAttrib - Entry attributes to Clear.
 *
 * \note    An attribute will be cleared if its flag is not OR’d into attrib.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                      Entry attributes clear successfully.
 *          FS_ERR_FILE_INVALID_ATTRIB       Entry attributes specified invalid.
 *          FS_ERR_NAME_NULL                 Argument 'pName' passed a NULL ptr.
 *
 *          FS_ERR_ENTRY_ROOT_DIR            File system entry is a root directory.
 *          FS_ERR_VOL_INVALID_OP            Invalid operation on volume.
 *
 *                                           RETURNED BY FSEntry_NameParseChk()
 *          FS_ERR_NAME_INVALID              Entry name specified invalid OR volume
 *                                           could not be found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Entry name specified too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume was not open.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume was not mounted.
 *
 *                                           RETURNED BY FSSys_EntryAttribSet()
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRY_NOT_FOUND           File system entry NOT found
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsClearAttrib(int8_t *pName, uint32_t u32ClearAttrib)
{
    FS_ERR      fsErr;      /* FS Status Return */
    uint32_t    u32Attrib;  /* Local Attrib */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if (NULL == pName)
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Get the current Attributes */
        u32Attrib = 0;
        fsErr = FsGetAttrib (pName, &u32Attrib);
        if (FS_ERR_NONE != fsErr)
        {
            break;
        }

        /* An attribute will be cleared if its flag is not OR’d into attrib.*/
        u32Attrib &= (~u32ClearAttrib);

        /* Call the corresponding µC/FS API */
        FSEntry_AttribSet ((CPU_CHAR *)pName, u32Attrib, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Copy a file.
 *
 * \param   pSrcFile - Name of the source file.
 * \param   pDestFile - Name of the destination file.
 *
 * \note    The file shall be copied even if 'pDestFile' does exist.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                      File copied successfully.
 *          FS_ERR_NAME_NULL                 Argument 'pSrcFile'/'pDestFile'
 *                                           passed a NULL pointer.
 *          FS_ERR_VOL_INVALID_OP            Invalid operation on volume.
 *
 *                                           RETURNED BY FSEntry_NameParseChkEx()
 *                                           RETURNED BY FS_NameParse()
 *          FS_ERR_NAME_INVALID              Source or destination name specified
 *                                           invalid OR volume could not be
 *                                           found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Source or destination name specified too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume was not open.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume was not mounted.
 *
 *                                           RETURNED BY FSFile_Open()
 *                                           RETURNED BY FSFile_Rd()
 *                                           RETURNED BY FSFile_Wr(
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRY_EXISTS              File system entry exists.
 *          FS_ERR_ENTRY_NOT_FILE            File system entry NOT a file.
 *          FS_ERR_ENTRY_NOT_FOUND           File system entry NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_ENTRY_RD_ONLY             File system entry marked read-only.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsCopy(int8_t *pSrcFile, int8_t *pDestFile)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if ((NULL == pSrcFile) || (NULL == pDestFile))
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Call the corresponding µC/FS API */
       FSEntry_Copy ((CPU_CHAR *)pSrcFile, (CPU_CHAR *)pDestFile, DEF_NO, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Move a file.
 *
 * \param   pSrcFile - Name of the source file.
 * \param   pDestFile - Name of the destination file.
 *
 * \note    The file shall be moved even if 'pDestFile' does exist.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                      File moved successfully.
 *          FS_ERR_NAME_NULL                 Argument 'pSrcFile'/'pDestFile'
 *                                           passed a NULL pointer.
 *          FS_ERR_VOL_INVALID_OP            Invalid operation on volume.
 *
 *                                           RETURNED BY FSEntry_NameParseChkEx()
 *                                           RETURNED BY FS_NameParse()
 *          FS_ERR_NAME_INVALID              Source or destination name specified
 *                                           invalid OR volume could not be found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Source or destination name specified
 *                                           too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume was not open.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume was not mounted.
 *
 *                                           RETURNED BY FSFile_Open()
 *                                           RETURNED BY FSFile_Rd()
 *                                           RETURNED BY FSFile_Wr()
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRY_EXISTS              File system entry exists.
 *          FS_ERR_ENTRY_NOT_FILE            File system entry NOT a file.
 *          FS_ERR_ENTRY_NOT_FOUND           File system entry NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_ENTRY_RD_ONLY             File system entry marked read-only.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsMove(int8_t *pSrcFile, int8_t *pDestFile)
{
    /* Since there are no direct µC/FS API calls, use alternate one. */
    return (FsRename (pSrcFile, pDestFile));
}

/* ========================================================================== */
/**
 * \brief   Rename a file or directory.
 *
 * \param   pOldName - Old path of the entry.
 * \param   pNewName - New path of the entry.
 *
 * \note    The file shall be renamed even if 'pNewName' does exist.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                      Entry renamed successfully.
 *          FS_ERR_NAME_NULL                 Argument 'pOldName'/'pNewName'
 *                                           passed a NULL pointer.
 *          FS_ERR_ENTRIES_VOLS_DIFF         Paths specify file system entries
 *                                           on different vols.
 *          FS_ERR_ENTRY_ROOT_DIR            File system entry is a root directory.
 *          FS_ERR_VOL_INVALID_OP            Invalid operation on volume.
 *
 *                                           RETURNED BY FSEntry_NameParseChkEx()
 *          FS_ERR_NAME_INVALID              Entry name specified invalid OR
 *                                           volume could not be found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Entry name specified too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume was not open.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume was not mounted.
 *
 *                                           RETURNED BY FSSys_EntryRename()
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRIES_TYPE_DIFF         Paths do not both specify files
 *                                           OR directories
 *          FS_ERR_ENTRY_EXISTS              File system entry exists.
 *          FS_ERR_ENTRY_NOT_EMPTY           File system entry NOT empty.
 *          FS_ERR_ENTRY_NOT_FOUND           File system entry NOT found
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_ENTRY_RD_ONLY             File system entry marked read-only.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsRename(int8_t *pOldName, int8_t *pNewName)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if ((NULL == pOldName) || (NULL == pNewName))
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Call the corresponding µC/FS API */
       FSEntry_Rename ((CPU_CHAR *)pOldName, (CPU_CHAR *)pNewName, DEF_NO, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Delete a file.
 *
 * \param   pFileName - Name of the File.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                     Entry deleted successfully.
 *          FS_ERR_NAME_NULL                Argument 'name_full' passed a NULL ptr.
 *          FS_ERR_ENTRY_ROOT_DIR           File system entry is a root directory.
 *          FS_ERR_VOL_INVALID_OP           Invalid operation on volume.
 *          FS_ERR_ENTRY_OPEN               Open entry cannot be deleted.
 *
 *                                          RETURNED BY FSFile_IsOpen()
 *                                          AND BY FSDir_IsOpen()
 *          FS_ERR_NONE                     File/Dir opened successfully.
 *          FS_ERR_NULL_PTR                 Argument 'name_full' passed a NULL ptr.
 *          FS_ERR_BUF_NONE_AVAIL           No buffer available.
 *          FS_ERR_ENTRY_NOT_FILE           Entry not a file or dir.
 *          FS_ERR_NAME_INVALID             Invalid name or path.
 *          FS_ERR_VOL_INVALID_SEC_NBR      Invalid sector number found in directory
 *
 *                                           RETURNED BY FSEntry_NameParseChk()
 *          FS_ERR_NAME_INVALID              Entry name specified invalid OR volume
 *                                           could not be found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Entry name specified too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume was not open.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume was not mounted.
 *
 *                                           RETURNED BY FSSys_EntryDel()
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRY_NOT_FILE            File system entry NOT a file.
 *          FS_ERR_ENTRY_NOT_DIR             File system entry NOT a directory.
 *          FS_ERR_ENTRY_NOT_EMPTY           File system entry NOT empty.
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsDelete(int8_t *pFileName)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if (NULL == pFileName)
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

         /* Del Source file */
        FSEntry_Del ((CPU_CHAR *)pFileName, FS_ENTRY_TYPE_FILE, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Open a directory.
 *
 * \param   *pDirName - Name of the directory.
 * \param   **pDir - Return directory pointer
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                      Directory opened successfully.
 *          FS_ERR_NAME_NULL                 Argument 'pDirName' passed a NULL ptr.
 *          FS_ERR_DIR_DIS                   Directory module disabled.
 *          FS_ERR_DIR_NONE_AVAIL            No directory available.
 *
 *                                           RETURNED BY FSDir_NameParseChk()
 *          FS_ERR_NAME_INVALID              Entry name specified invalid or volume
 *                                           could not be found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Entry name is too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume not opened.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume not mounted.
 *
 *                                           RETURNED BY FSSys_DirOpen()
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRY_NOT_DIR             File system entry NOT a directory.
 *          FS_ERR_ENTRY_NOT_FOUND           File system entry NOT found
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsOpenDir(int8_t *pDirName, FS_DIR **pDir)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input File Pointer Valid ? */
        if ((NULL == pDir) || (NULL == pDirName))
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Call the corresponding µC/FS API */
        *pDir = FSDir_Open ((CPU_CHAR *)pDirName, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Test if the dir is already open.
 *
 * \param   pDirName - Name of the directory.
 * \param   pIsOpen - Return Boolean, true - if dir is open, else false.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                     No errors.
 *          FS_ERR_NAME_NULL                Argument 'pDirName' passed a NULL ptr.
 *
 *                                          RETURNED BY FS_FAT_LowFileFirstClusGet()
 *          FS_ERR_BUF_NONE_AVAIL           No buffer available.
 *          FS_ERR_ENTRY_NOT_FILE           Entry not a file nor a directory.
 *          FS_ERR_VOL_INVALID_SEC_NBR      Cluster address invalid.
 *          FS_ERR_ENTRY_NOT_FOUND          File system entry NOT found
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND   Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR     Entry parent NOT a directory.
 *          FS_ERR_NAME_INVALID             Invalid name or path.
 *          FS_ERR_VOL_INVALID_SEC_NBR      Invalid sector num found in dir entry.
 *
 * ========================================================================== */
FS_ERR FsIsOpenDir(int8_t *pDirName, bool *pIsOpen)
{
    FS_ERR fsErr;  /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input File Pointer Valid? */
        if ((NULL == pDirName) || (NULL == pIsOpen))
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Call the corresponding µC/FS API */
        if (DEF_YES == FSDir_IsOpen ((CPU_CHAR *)pDirName, &fsErr))
        {
            *pIsOpen = true;
        }
        else
        {
            *pIsOpen = false;
        }

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Close a directory.
 *
 * \param   pDir - Pointer to a directory.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE            Directory closed.
 *          FS_ERR_NULL_PTR        Argument 'pDir' passed a NULL pointer.
 *          FS_ERR_INVALID_TYPE    Argument 'pDir's TYPE is invalid or unknown.
 *          FS_ERR_DIR_DIS         Directory module disabled.
 *          FS_ERR_DIR_NOT_OPEN    Directory NOT open.
 *
 * ========================================================================== */
FS_ERR FsCloseDir(FS_DIR *pDir)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if (NULL == pDir)
        {
            fsErr = FS_ERR_NULL_PTR;
            break;
        }

        /* Call the corresponding µC/FS API */
        FSDir_Close (pDir, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Read a directory entry from a directory.
 *
 * \param   pDir - Pointer to a directory.
 * \param   pDirEntryInfo - Pointer to variable that will receive dir entry info.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE              Directory read successfully.
 *          FS_ERR_NULL_PTR          Argument 'pDir'/'pDirEntryInfo'passed a NULL ptr.
 *          FS_ERR_INVALID_TYPE      Argument 'pDir's TYPE is invalid or unknown.
 *          FS_ERR_DIR_DIS           Directory module disabled.
 *
 *                                   RETURNED BY FSDir_AcquireLockChk()
 *          FS_ERR_DEV_CHNGD         Device has changed.
 *          FS_ERR_DIR_NOT_OPEN      Directory NOT open.
 *
 *                                   RETURNED BY FSSys_DirRd()
 *          FS_ERR_EOF               End of directory reached.
 *          FS_ERR_DEV               Device access error.
 *          FS_ERR_BUF_NONE_AVAIL    Buffer not available.
 *
 * ========================================================================== */
FS_ERR FsReadDir(FS_DIR *pDir, FS_DIR_ENTRY *pDirEntryInfo)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if ((NULL == pDir) || (NULL == pDirEntryInfo))
        {
            fsErr = FS_ERR_NULL_PTR;
            break;
        }

        /* Call the corresponding µC/FS API */
        FSDir_Rd (pDir, pDirEntryInfo, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Change the directory.
 *
 * \param   pDirName - String that specifies EITHER
 *         (a) the absolute working directory path to set;
 *         (b) a relative path that will be applied to the curr working dir.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                   Working directory set.
 *          FS_ERR_NAME_INVALID           Name invalid.
 *          FS_ERR_NAME_PATH_TOO_LONG     Path too long.
 *          FS_ERR_NAME_NULL              Argument 'path_dir' passed a NULL ptr.
 *          FS_ERR_VOL_NONE_EXIST         No volumes exist.
 *          FS_ERR_WORKING_DIR_NONE_AVAIL No working directories available.
 *          FS_ERR_WORKING_DIR_INVALID    Arg 'path_dir' passed an invalid dir.
 *
 * ========================================================================== */
FS_ERR FsChangeDir(int8_t *pDirName)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if (NULL == pDirName)
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Call the corresponding µC/FS API */
        FS_WorkingDirSet ((CPU_CHAR *)pDirName, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Get the working directory for the current task.
 *
 * \param   pCwdPath - String buffer that will receive the working directory path.
 * \param   u32PathSize - Size of string buffer.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                  Working directory obtained.
 *          FS_ERR_NULL_PTR              Argument 'path_dir' passed a NULL pointer.
 *          FS_ERR_NULL_ARG              Argument 'size' passed a NULL value.
 *          FS_ERR_NAME_BUF_TOO_SHORT    Argument 'size' less than length of path.
 *          FS_ERR_VOL_NONE_EXIST        No volumes exist.
 *
 * ========================================================================== */
FS_ERR FsGetCWDir(int8_t *pCwdPath, uint32_t u32PathSize)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if (NULL == pCwdPath)
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Call the corresponding µC/FS API */
        FS_WorkingDirGet ((CPU_CHAR *)pCwdPath, u32PathSize, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Create a directory.
 *
 * \param   pDirName - Name of the directory.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                      Entry created successfully.
 *          FS_ERR_NAME_NULL                 Argument 'pDirName' passed a NULL pointer.
 *          FS_ERR_ENTRY_TYPE_INVALID        Argument 'entry_type' is invalid.
 *
 *          FS_ERR_VOL_INVALID_OP            Invalid operation on volume.
 *
 *                                           RETURNED BY FSEntry_NameParseChk()
 *          FS_ERR_NAME_INVALID              Entry name specified invalid OR volume
 *                                           could not be found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Entry name specified too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume was not open.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume was not mounted.
 *
 *                                           RETURNED BY FSSys_EntryCreate()
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRY_EXISTS              File system entry exists.
 *          FS_ERR_ENTRY_NOT_DIR             File system entry NOT a directory.
 *          FS_ERR_ENTRY_NOT_EMPTY           File system entry NOT empty.
 *          FS_ERR_ENTRY_NOT_FILE            File system entry NOT a file.
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_ENTRY_RD_ONLY             File system entry marked read-only.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsMakeDir(int8_t *pDirName)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if (NULL == pDirName)
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Call the corresponding µC/FS API */
        FSEntry_Create ((CPU_CHAR *)pDirName, FS_ENTRY_TYPE_DIR, DEF_YES, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Rename a directory.
 *
 * \param   pOldName - Old path of the entry.
 * \param   pNewName - New path of the entry.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                      Entry renamed successfully.
 *          FS_ERR_NAME_NULL                 Argument 'pOldName'/'pNewName'
 *                                           passed a NULL pointer.
 *          FS_ERR_ENTRIES_VOLS_DIFF         Paths specify file system entries
 *                                           on different vols.
 *          FS_ERR_ENTRY_ROOT_DIR            File system entry is a root directory.
 *          FS_ERR_VOL_INVALID_OP            Invalid operation on volume.
 *
 *                                           RETURNED BY FSEntry_NameParseChkEx()
 *          FS_ERR_NAME_INVALID              Entry name specified invalid OR
 *                                           volume could not be found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Entry name specified too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume was not open.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume was not mounted.
 *
 *                                           RETURNED BY FSSys_EntryRename()
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRIES_TYPE_DIFF         Paths do not both specify files
 *                                           OR directories
 *          FS_ERR_ENTRY_EXISTS              File system entry exists.
 *          FS_ERR_ENTRY_NOT_EMPTY           File system entry NOT empty.
 *          FS_ERR_ENTRY_NOT_FOUND           File system entry NOT found
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_ENTRY_RD_ONLY             File system entry marked read-only.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsRenameDir(int8_t *pOldName, int8_t *pNewName)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if ((NULL == pOldName) || (NULL == pNewName))
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Call the corresponding µC/FS API */
        FSEntry_Rename ((CPU_CHAR *)pOldName, (CPU_CHAR *)pNewName, DEF_NO, &fsErr);

    } while (false);

    return fsErr;
}
/* ========================================================================== */
/**
 * \brief   Delete a directory.
 *
 * \param   pDirName - Name of the directory.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                     Entry deleted successfully.
 *          FS_ERR_NAME_NULL                Argument 'pDirName' passed a NULL pointer.
 *          FS_ERR_ENTRY_ROOT_DIR           File system entry is a root directory.
 *          FS_ERR_VOL_INVALID_OP           Invalid operation on volume.
 *          FS_ERR_ENTRY_OPEN               Open entry cannot be deleted.
 *
 *                                          RETURNED BY FSFile_IsOpen()
 *                                          AND BY FSDir_IsOpen()
 *          FS_ERR_NONE                     File/Dir opened successfully.
 *          FS_ERR_NULL_PTR                 Argument 'pDirName' passed a NULL ptr.
 *          FS_ERR_BUF_NONE_AVAIL           No buffer available.
 *          FS_ERR_ENTRY_NOT_FILE           Entry not a file or dir.
 *          FS_ERR_NAME_INVALID             Invalid name or path.
 *          FS_ERR_VOL_INVALID_SEC_NBR      Invalid sector number found in directory
 *
 *                                           RETURNED BY FSEntry_NameParseChk()
 *          FS_ERR_NAME_INVALID              Entry name specified invalid OR volume
 *                                           could not be found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Entry name specified too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume was not open.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume was not mounted.
 *
 *                                           RETURNED BY FSSys_EntryDel()
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRY_NOT_FILE            File system entry NOT a file.
 *          FS_ERR_ENTRY_NOT_DIR             File system entry NOT a directory.
 *          FS_ERR_ENTRY_NOT_EMPTY           File system entry NOT empty.
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsRemoveDir(int8_t *pDirName)
{
   FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if (NULL == pDirName)
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

         /* Del Dir */
        FSEntry_Del ((CPU_CHAR *)pDirName, FS_ENTRY_TYPE_DIR, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Get information about a directory.
 *
 * \param   pDirName - Name of the entry.
 * \param   pFindInfo - Pointer to structure that will receive the entry information.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *          also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                      Entry information gotten successfully.
 *          FS_ERR_NAME_NULL                 Argument 'pDirName' passed a NULL pointer.
 *          FS_ERR_NULL_PTR                  Argument 'p_info' passed a NULL pointer.
 *
 *                                           RETURNED BY FSEntry_NameParseChk()
 *          FS_ERR_NAME_INVALID              Entry name specified invalid OR volume
 *                                           could not be found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Entry name specified too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume was not open.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume was not mounted.
 *
 *                                           RETURNED BY FSSys_EntryQuery()
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRY_NOT_FOUND           File system entry NOT found
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsQueryDir(int8_t *pDirName, FS_ENTRY_INFO *pFindInfo)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if ((NULL == pDirName) || (NULL == pFindInfo))
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Call the corresponding µC/FS API */
        FSEntry_Query ((CPU_CHAR *)pDirName, pFindInfo, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Get information about a file or directory.
 *
 * \param   pFileName - Name of the entry.
 * \param   pInfo - Pointer to structure that will receive the entry information.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                      Entry information gotten successfully.
 *          FS_ERR_NAME_NULL                 Argument 'pFileName' passed a NULL pointer.
 *          FS_ERR_NULL_PTR                  Argument 'p_info' passed a NULL pointer.
 *
 *                                           RETURNED BY FSEntry_NameParseChk()
 *          FS_ERR_NAME_INVALID              Entry name specified invalid OR volume
 *                                           could not be found.
 *          FS_ERR_NAME_PATH_TOO_LONG        Entry name specified too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume was not open.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume was not mounted.
 *
 *                                           RETURNED BY FSSys_EntryQuery()
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_ENTRY_NOT_FOUND           File system entry NOT found
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *
 * ========================================================================== */
FS_ERR FsGetInfo(int8_t *pFileName, FS_ENTRY_INFO *pInfo)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Entry Name Valid? */
        if ((NULL == pFileName) || (NULL == pInfo))
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Call the corresponding µC/FS API */
        FSEntry_Query ((CPU_CHAR *)pFileName, pInfo, &fsErr);

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Checks if SD card is present.
 *
 * \details This function Checks if SD card is present via GPIO PTD10.
 *
 * \todo    09/25/2020 GK Update with GPIO calls, issue is then
 *          L3_GPIO_Ctrl_Init() shall be called before FsInit(). In that case
 *          we might lose some valuable Init Log messages [especially when
 *          Init fails]. Have to find an alternate approach.
 *
 * \param   < None >
 *
 * \return  FS_ERR_NONE - If SD card is present.
 *          FS_ERR_DEV_NONE_AVAIL - If SD card NOT present
 *
 * ========================================================================== */
FS_ERR FsIsSDCardPresent(void)
{
    FS_ERR fsErr; /* FS Status Return */

    fsErr = FS_ERR_NONE;

    /* Check the SD card presence */
    if (GPIOD_PDIR & SD_CARD_INSTALLED_MASK)
    {
        fsErr = FS_ERR_DEV_NONE_AVAIL;
    }
    
    TM_Hook (HOOK_SIMULATEXPMODEERROR, &fsErr);
    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Get information about the SD Card
 *
 * \param   pSDInfo  Pointer to structure that will receive the SD Card information
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, refer fs_err.h for full list
 *
 * ========================================================================== */
FS_ERR FsGetInfoSDCard(FS_SDCARD_INFO *pSDInfo)
{
    FS_ERR          fsErr;      /* FS Status Return */
    FS_DEV_SD_INFO  xSDInfo;    /* SD Dev Info */
    FS_VOL_INFO     xVolInfo;   /* SD Volumne Info */

    do
    {   /* Do basic sanity check, detailed check happens inside the µC/FS API calls */

        /* Is Input Valid? */
        if (NULL == pSDInfo)
        {
            fsErr = FS_ERR_NAME_NULL;
            break;
        }

        /* Get SD Card Info */
        FSDev_SD_Card_QuerySD ((CPU_CHAR *)SD_CARD_DEVICE, &xSDInfo, &fsErr);

        if (FS_ERR_NONE != fsErr)
        {
            break;
        }

        /* Get Volumne Info */
        FSVol_Query ((CPU_CHAR *)SD_CARD_DEVICE, &xVolInfo, &fsErr);

        if (FS_ERR_NONE != fsErr)
        {
            break;
        }

        /* Copy the FSDev_SD_Card_QuerySD info to SD card info */
        pSDInfo->u8CardType = xSDInfo.CardType;
        pSDInfo->u8HighCap  = xSDInfo.HighCapacity;
        pSDInfo->u8ManufID  = xSDInfo.ManufID;
        pSDInfo->u16OEM_ID  = xSDInfo.OEM_ID;
        pSDInfo->u32ProdSN  = xSDInfo.ProdSN;
        pSDInfo->u32ProdRev = xSDInfo.ProdRev;

        /* Copy the FSVol_Query info to SD card info */
        pSDInfo->u32DevSecSize = xVolInfo.DevSecSize;
        pSDInfo->u32TotSecCnt  = xVolInfo.VolTotSecCnt;
        pSDInfo->u32BadSecCnt  = xVolInfo.VolBadSecCnt;
        pSDInfo->u32TotalSpace = ( xVolInfo.DevSecSize * (xVolInfo.VolTotSecCnt  / BYTES_PER_KB)); /* In KBs*/
        pSDInfo->u32FreeSpace  = ( xVolInfo.DevSecSize * (xVolInfo.VolFreeSecCnt / BYTES_PER_KB)); /* In KBs*/
        pSDInfo->u32UsedSpace  = ( xVolInfo.DevSecSize * (xVolInfo.VolUsedSecCnt / BYTES_PER_KB)); /* In KBs*/

    } while (false);

    return fsErr;
}

 /* ========================================================================== */
/**
 * \brief  This function reads the SD card free space and takes required actions on Handle startup.
 *
 * \details This function reads the available free space on the SD card. if the free space
 *          is found to be < 10% a cleanup task is started to clean the rdf files and attempts to
 *          increase the free space to 25%. This check is done only on startup.
 *
 *
 * \param   < None >
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, refer fs_err.h for full list
 *
 * ========================================================================== */
static FS_ERR MonitorSDCardFreeSpace(void)
{
    FS_SDCARD_INFO  SDInfo;
    FS_ERR          fsErr;
    float32_t       FreeSpacePercent;
    FS_FILE_SIZE    DesiredSpace;
    uint8_t         OsError;           /* OS errors */

    do
    {
        fsErr = FsGetInfoSDCard (&SDInfo);
        if (FS_ERR_NONE != fsErr)
        {
          
            Log (DBG, " FsGetInfoSDCard: FSErr = %x", fsErr);

            break;
        }
        /* calculate free space available based on the SD Card Info */
        FreeSpacePercent = (FS_FILE_SIZE)((PERCENT_100 * (float32_t)(SDInfo.u32FreeSpace) / (float32_t)SDInfo.u32TotalSpace));
        /* The HANDLE shall delete the oldest RDF files on the SD_CARD to ensure that the available free space is >= 25% of its total capacity,
        if the free space on the micro SD_CARD is < 10% of its total capacity. */
        if (FreeSpacePercent < PERCENT_10)
        {
            /* calculate required bytes to be cleaned up */
            DesiredSpace = (FS_FILE_SIZE)((PERCENT_25 * (float32_t)(SDInfo.u32TotalSpace)) / PERCENT_100);
            RdfCleanupSize = DesiredSpace - SDInfo.u32FreeSpace;

            Log (DBG, " Free Data Space Percent  = %3.2f", FreeSpacePercent);
            Log (DBG, " to-be-deleted %ld  Desired size  %ld  ", RdfCleanupSize, DesiredSpace);

            /* run the cleanup task in background to avoid delaying handle startup. The cleanup task deletes itself after completion of the cleanup */
            OsError = SigTaskCreate (&CleanupOldRdfFiles,
                                    (void *)&RdfCleanupSize,
                                    &CleanupTaskStack[0],
                                    TASK_PRIORITY_CLEANUP,
                                    CLEANUP_TASK_STACK_SIZE,
                                    "MemCleanup");

            if (OsError != OS_ERR_NONE)
            {
                Log (ERR, "FileSys: Cleanup Task Create Error - %d", OsError);

                break;
            }
        }

        if (FreeSpacePercent < FS_LOWLIMIT)
        {
            fsErr = FS_ERR_DEV_FULL;
            Log (DBG, " FileSys: Free Data Space Percent  = %3.2f", FreeSpacePercent );
            break;
        }
        fsErr = FS_ERR_NONE;
    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Formats SD card.
 *
 * \param   < None >
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                     Volume formatted.
 *          FS_ERR_DEV                      Device error.
 *          FS_ERR_DEV_INVALID_SIZE         Invalid device size.
 *          FS_ERR_VOL_DIRS_OPEN            Directories open on volume.
 *          FS_ERR_VOL_FILES_OPEN           Files open on volume.
 *          FS_ERR_VOL_INVALID_SYS          Invalid file system parameters.
 *          FS_ERR_VOL_NOT_OPEN             Volume not open.
 *
 * ========================================================================== */
FS_ERR FsFormatSDCard(void)
{
    FS_ERR fsErr; /* FS Status Return */

    do
    {   /* Close the Volume before the format. */
        FSVol_Close (SD_CARD_DEVICE, &fsErr);

        if (FS_ERR_NONE != fsErr)
        {
            break;
        }

        /* Open the Volume Again */
        FSVol_Open (SD_CARD_DEVICE, SD_CARD_DEVICE, 0, &fsErr);

         if (FS_ERR_NONE != fsErr)
        {
            break;
        }

        Log (REQ, "FileSys: Formatting SD card...");

        /* Format the Volume */
        FSVol_Fmt(SD_CARD_DEVICE, NULL, &fsErr);

         if (FS_ERR_NONE != fsErr)
        {
            Log (REQ, "FileSys: Formatting Failed");
            FsLogErr (fsErr);
            break;
        }

         Log (REQ, "FileSys: Formatting Success");

    } while (false);

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Performs SD card Check disk operation.
 *
 * \param   < None >
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                     Volume checked without errors.
 *          FS_ERR_DEV                      Device access error.
 *          FS_ERR_VOL_NOT_OPEN             Volume not open.
 *          FS_ERR_BUF_NONE_AVAIL           No buffers available.
 *          FS_ERR_INVALID_CFG              Invalid config, Vol Check Disabled.
 *
 * ========================================================================== */
FS_ERR FsChkDskSDCard(void)
{
    FS_ERR fsErr; /* FS Status Return */

    /* Check if Volume Check is enabled in Micrium fs_cfg.h */
#ifdef FS_FAT_CFG_VOL_CHK_EN
    do
    {   /* Close the Volume before the format. */
        FSVol_Close (SD_CARD_DEVICE, &fsErr);

        if (FS_ERR_NONE != fsErr)
        {
            break;
        }

        /* Open the Volume Again */
        FSVol_Open (SD_CARD_DEVICE, SD_CARD_DEVICE, 0, &fsErr);

         if (FS_ERR_NONE != fsErr)
        {
            break;
        }

        Log (REQ, "FileSys: Performing ChkDsk on SD card...");

        /* Call the Vol Chk function */
        FS_FAT_VolChk (SD_CARD_DEVICE, &fsErr);

         if (FS_ERR_NONE != fsErr)
        {
            Log (REQ, "FileSys: ChkDsk Failed");
            FsLogErr (fsErr);
            break;
        }

         Log (REQ, "FileSys: ChkDsk Success");

    } while (false);

#else
    /* Volume Check is Not enabled, return FS_ERR_INVALID_CFG */
    fsErr = FS_ERR_INVALID_CFG;

#endif // FS_FAT_CFG_VOL_CHK_EN

    return fsErr;
}

/* ========================================================================== */
/**
 * \brief   Test if the directory is empty.
 *
 * \param   pDirName - Name of the directory.
 * \param   pIsEmpty - Return Boolean, true - if dir is empty, else false.
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also come, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                      No error.
 *          FS_ERR_NAME_NULL                 Argument 'pDirName' passed a NULL ptr.
 *          FS_ERR_DIR_DIS                   Directory module disabled.
 *          FS_ERR_DIR_NONE_AVAIL            No directory available.
 *          FS_ERR_NAME_INVALID              Entry name specified invalid or volume
 *          FS_ERR_NAME_PATH_TOO_LONG        Entry name is too long.
 *          FS_ERR_VOL_NOT_OPEN              Volume not opened.
 *          FS_ERR_VOL_NOT_MOUNTED           Volume not mounted.
 *          FS_ERR_BUF_NONE_AVAIL            Buffer not available.
 *          FS_ERR_DEV                       Device access error.
 *          FS_ERR_EOF                       End of directory reached.
 *          FS_ERR_ENTRY_NOT_DIR             File system entry NOT a directory.
 *          FS_ERR_ENTRY_NOT_FOUND           File system entry NOT found
 *          FS_ERR_ENTRY_PARENT_NOT_FOUND    Entry parent NOT found.
 *          FS_ERR_ENTRY_PARENT_NOT_DIR      Entry parent NOT a directory.
 *          FS_ERR_NAME_INVALID              Invalid file name or path.
 *          FS_ERR_DIR_NOT_OPEN              Directory NOT open.
 *
 * ========================================================================== */
FS_ERR FsDirIsEmpty(uint8_t *pDirName, bool *pIsEmpty)
{
    FS_DIR *        pTempDir;
    FS_ERR          FsErr;
    FS_DIR_ENTRY    NextDirEntry;
    bool            IsEntryRead;

    IsEntryRead = false;
    *pIsEmpty = false;

    do
    {
        if (NULL == pDirName)
        {
            FsErr = FS_ERR_NAME_NULL;
            break;
        }

        pTempDir = FSDir_Open((CPU_CHAR *)pDirName, &FsErr);
        if (NULL == pTempDir)
        {
            break;
        }
        while (FS_ERR_NONE == FsErr)
        {
            FSDir_Rd(pTempDir, &NextDirEntry, &FsErr);
            if ((FsErr == FS_ERR_NONE) &&
                !((NextDirEntry.Name[0] == '.') && (NextDirEntry.Name[1] == 0)) &&
                !((NextDirEntry.Name[0] == '.') && (NextDirEntry.Name[1] == '.') && (NextDirEntry.Name[2] == 0)))
            {
                FSDir_Close(pTempDir, &FsErr);
                *pIsEmpty = false;
                IsEntryRead = true;
                break;
            }
        }
        if (true == IsEntryRead)
        {
            break;
        }

        FSDir_Close (pTempDir, &FsErr);
        *pIsEmpty = true;

    } while (false);

   return FsErr;
}

/* ========================================================================== */
/**
 * \brief   Logs the File System Error
 *
 * \details This function logs the Files System Error.
 *
 * \todo    09/25/2020 GK Remove xFsErrStringTable[] from production build.
 *          [Evaluate size constraints]
 *
 * \param   fsErr - File System Error Code In.
 *
 * \return  None
 *
 * ========================================================================== */
void FsLogErr(FS_ERR fsErr)
{
    /* Below table is very good for Initial Debugging, later, remove it entirely
       or limit it for the Debug build */

#ifdef DEBUG_CODE

    const FS_ERR_STR_T xFsErrStringTable[] = {   /* Fs Error String [fs_err.h] */

    { FS_ERR_NONE,                            "NONE"                           },

    { FS_ERR_INVALID_ARG,                     "INVALID_ARG"                    }, /* Invalid argument. */
    { FS_ERR_INVALID_CFG,                     "INVALID_CFG"                    }, /* Invalid configuration. */
    { FS_ERR_INVALID_CHKSUM,                  "INVALID_CHKSUM"                 }, /* Invalid checksum. */
    { FS_ERR_INVALID_LEN,                     "INVALID_LEN"                    }, /* Invalid length. */
    { FS_ERR_INVALID_TIME,                    "INVALID_TIME"                   }, /* Invalid date/time. */
    { FS_ERR_INVALID_TIMESTAMP,               "INVALID_TIMESTAMP"              }, /* Invalid timestamp. */
    { FS_ERR_INVALID_TYPE,                    "INVALID_TYPE"                   }, /* Invalid object type. */
    { FS_ERR_MEM_ALLOC,                       "MEM_ALLOC"                      }, /* Mem could not be alloc'd. */
    { FS_ERR_NULL_ARG,                        "NULL_ARG"                       }, /* Arg(s) passed NULL val(s). */
    { FS_ERR_NULL_PTR,                        "NULL_PTR"                       }, /* Ptr arg(s) passed NULL ptr(s). */
    { FS_ERR_OS,                              "OS"                             }, /* OS err. */
    { FS_ERR_OVF,                             "OVF"                            }, /* Value too large to be stored in type. */
    { FS_ERR_EOF,                             "EOF"                            }, /* EOF reached. */

    { FS_ERR_WORKING_DIR_NONE_AVAIL,          "WORKING_DIR_NONE_AVAIL"         }, /* No working dir avail. */
    { FS_ERR_WORKING_DIR_INVALID,             "WORKING_DIR_INVALID"            }, /* Working dir invalid. */

    { FS_ERR_BUF_NONE_AVAIL,                  "BUF_NONE_AVAIL"                 }, /* No buf avail. */

    { FS_ERR_CACHE_INVALID_MODE,              "CACHE_INVALID_MODE"             }, /* Mode specified invalid. */
    { FS_ERR_CACHE_INVALID_SEC_TYPE,          "CACHE_INVALID_SEC_TYPE"         }, /* Sector type specified invalid. */
    { FS_ERR_CACHE_TOO_SMALL,                 "CACHE_TOO_SMALL"                }, /* Cache specified too small. */

    { FS_ERR_DEV,                             "DEV"                            }, /* Device access error. */
    { FS_ERR_DEV_ALREADY_OPEN,                "DEV_ALREADY_OPEN"               }, /* Device already open. */
    { FS_ERR_DEV_CHNGD,                       "DEV_CHNGD"                      }, /* Device has changed. */
    { FS_ERR_DEV_FIXED,                       "DEV_FIXED"                      }, /* Device is fixed (cannot be closed). */
    { FS_ERR_DEV_FULL,                        "DEV_FULL"                       }, /* Device is full (no space could be allocated). */
    { FS_ERR_DEV_INVALID,                     "DEV_INVALID"                    }, /* Invalid device. */
    { FS_ERR_DEV_INVALID_CFG,                 "DEV_INVALID_CFG"                }, /* Invalid dev cfg. */
    { FS_ERR_DEV_INVALID_ECC,                 "DEV_INVALID_ECC"                }, /* Invalid ECC. */
    { FS_ERR_DEV_INVALID_IO_CTRL,             "DEV_INVALID_IO_CTRL"            }, /* I/O control invalid. */
    { FS_ERR_DEV_INVALID_LOW_FMT,             "DEV_INVALID_LOW_FMT"            }, /* Low format invalid. */
    { FS_ERR_DEV_INVALID_LOW_PARAMS,          "DEV_INVALID_LOW_PARAMS"         }, /* Invalid low-level device parameters. */
    { FS_ERR_DEV_INVALID_MARK,                "DEV_INVALID_MARK"               }, /* Invalid mark. */
    { FS_ERR_DEV_INVALID_NAME,                "DEV_INVALID_NAME"               }, /* Invalid device name. */
    { FS_ERR_DEV_INVALID_OP,                  "DEV_INVALID_OP"                 }, /* Invalid operation. */
    { FS_ERR_DEV_INVALID_SEC_NBR,             "DEV_INVALID_SEC_NBR"            }, /* Invalid device sec nbr. */
    { FS_ERR_DEV_INVALID_SEC_SIZE,            "DEV_INVALID_SEC_SIZE"           }, /* Invalid device sec size. */
    { FS_ERR_DEV_INVALID_SIZE,                "DEV_INVALID_SIZE"               }, /* Invalid device size. */
    { FS_ERR_DEV_INVALID_UNIT_NBR,            "DEV_INVALID_UNIT_NBR"           }, /* Invalid device unit nbr. */
    { FS_ERR_DEV_IO,                          "DEV_IO"                         }, /* Device I/O error. */
    { FS_ERR_DEV_NONE_AVAIL,                  "DEV_NONE_AVAIL"                 }, /* No device avail. */
    { FS_ERR_DEV_NOT_OPEN,                    "DEV_NOT_OPEN"                   }, /* Device not open. */
    { FS_ERR_DEV_NOT_PRESENT,                 "DEV_NOT_PRESENT"                }, /* Device not present. */
    { FS_ERR_DEV_TIMEOUT,                     "DEV_TIMEOUT"                    }, /* Device timeout. */
    { FS_ERR_DEV_UNIT_NONE_AVAIL,             "DEV_UNIT_NONE_AVAIL"            }, /* No unit avail. */
    { FS_ERR_DEV_UNIT_ALREADY_EXIST,          "DEV_UNIT_ALREADY_EXIST"         }, /* Unit already exists. */
    { FS_ERR_DEV_UNKNOWN,                     "DEV_UNKNOWN"                    }, /* Unknown. */
    { FS_ERR_DEV_VOL_OPEN,                    "DEV_VOL_OPEN"                   }, /* Vol open on dev. */
    { FS_ERR_DEV_INCOMPATIBLE_LOW_PARAMS,     "DEV_INCOMPATIBLE_LOW_PARAMS"    }, /* Incompatible low-level device parameters. */
    { FS_ERR_DEV_INVALID_METADATA,            "DEV_INVALID_METADATA"           }, /* Device driver metadata is invalid. */
    { FS_ERR_DEV_OP_ABORTED,                  "DEV_OP_ABORTED"                 }, /* Operation aborted. */
    { FS_ERR_DEV_CORRUPT_LOW_FMT,             "DEV_CORRUPT_LOW_FMT"            }, /* Corrupted low-level fmt. */
    { FS_ERR_DEV_INVALID_SEC_DATA,            "DEV_INVALID_SEC_DATA"           }, /* Retrieved sec data is invalid. */
    { FS_ERR_DEV_WR_PROT,                     "DEV_WR_PROT"                    }, /* Device is write protected. */
    { FS_ERR_DEV_OP_FAILED,                   "DEV_OP_FAILED"                  }, /* Operation failed. */

    { FS_ERR_DEV_NAND_NO_AVAIL_BLK,           "DEV_NAND_NO_AVAIL_BLK"          }, /* No blk avail. */
    { FS_ERR_DEV_NAND_NO_SUCH_SEC,            "DEV_NAND_NO_SUCH_SEC"           }, /* This sector is not available. */
    { FS_ERR_DEV_NAND_ECC_NOT_SUPPORTED,      "DEV_NAND_ECC_NOT_SUPPORTED"     }, /* The needed ECC scheme is not supported. */

    { FS_ERR_DEV_NAND_ONFI_EXT_PARAM_PAGE,    "DEV_NAND_ONFI_EXT_PARAM_PAGE"   }, /* NAND device extended parameter page must be read. */

    { FS_ERR_DEV_DRV_ALREADY_ADDED,           "DEV_DRV_ALREADY_ADDED"          }, /* Dev drv already added. */
    { FS_ERR_DEV_DRV_INVALID_NAME,            "DEV_DRV_INVALID_NAME"           }, /* Invalid dev drv name. */
    { FS_ERR_DEV_DRV_NONE_AVAIL,              "DEV_DRV_NONE_AVAIL"             }, /* No driver available. */

    { FS_ERR_DIR_ALREADY_OPEN,                "DIR_ALREADY_OPEN"               }, /* Directory already open. */
    { FS_ERR_DIR_DIS,                         "DIR_DIS"                        }, /* Directory module disabled. */
    { FS_ERR_DIR_FULL,                        "DIR_FULL"                       }, /* Directory is full. */
    { FS_ERR_DIR_NONE_AVAIL,                  "DIR_NONE_AVAIL"                 }, /* No directory  avail. */
    { FS_ERR_DIR_NOT_OPEN,                    "DIR_NOT_OPEN"                   }, /* Directory not open. */

    { FS_ERR_ECC_CORR,                        "ECC_CORR"                       }, /* Correctable ECC error. */
    { FS_ERR_ECC_CRITICAL_CORR,               "ECC_CRITICAL_CORR"              }, /* Critical correctable ECC error (should refresh data). */
    { FS_ERR_ECC_UNCORR,                      "ECC_UNCORR"                     }, /* Uncorrectable ECC error. */

    { FS_ERR_ENTRIES_SAME,                    "ENTRIES_SAME"                   }, /* Paths specify same file system entry. */
    { FS_ERR_ENTRIES_TYPE_DIFF,               "ENTRIES_TYPE_DIFF"              }, /* Paths do not both specify files OR directories. */
    { FS_ERR_ENTRIES_VOLS_DIFF,               "ENTRIES_VOLS_DIFF"              }, /* Paths specify file system entries on different vols. */
    { FS_ERR_ENTRY_CORRUPT,                   "ENTRY_CORRUPT"                  }, /* File system entry is corrupt. */
    { FS_ERR_ENTRY_EXISTS,                    "ENTRY_EXISTS"                   }, /* File system entry exists. */
    { FS_ERR_ENTRY_INVALID,                   "ENTRY_INVALID"                  }, /* File system entry invalid. */
    { FS_ERR_ENTRY_NOT_DIR,                   "ENTRY_NOT_DIR"                  }, /* File system entry NOT a directory. */
    { FS_ERR_ENTRY_NOT_EMPTY,                 "ENTRY_NOT_EMPTY"                }, /* File system entry NOT empty. */
    { FS_ERR_ENTRY_NOT_FILE,                  "ENTRY_NOT_FILE"                 }, /* File system entry NOT a file. */
    { FS_ERR_ENTRY_NOT_FOUND,                 "ENTRY_NOT_FOUND"                }, /* File system entry NOT found. */
    { FS_ERR_ENTRY_PARENT_NOT_FOUND,          "ENTRY_PARENT_NOT_FOUND"         }, /* Entry parent NOT found. */
    { FS_ERR_ENTRY_PARENT_NOT_DIR,            "ENTRY_PARENT_NOT_DIR"           }, /* Entry parent NOT a directory. */
    { FS_ERR_ENTRY_RD_ONLY,                   "ENTRY_RD_ONLY"                  }, /* File system entry marked read-only. */
    { FS_ERR_ENTRY_ROOT_DIR,                  "ENTRY_ROOT_DIR"                 }, /* File system entry is a root directory. */
    { FS_ERR_ENTRY_TYPE_INVALID,              "ENTRY_TYPE_INVALID"             }, /* File system entry type is invalid. */
    { FS_ERR_ENTRY_OPEN,                      "ENTRY_OPEN"                     }, /* Operation not allowed on already open entry. */
    { FS_ERR_ENTRY_CLUS,                      "ENTRY_CLUS"                     }, /* No clus allocated to a directory entry. */

    { FS_ERR_FILE_ALREADY_OPEN,               "FILE_ALREADY_OPEN"              }, /* File already open. */
    { FS_ERR_FILE_BUF_ALREADY_ASSIGNED,       "FILE_BUF_ALREADY_ASSIGNED"      }, /* Buf already assigned. */
    { FS_ERR_FILE_ERR,                        "FILE_ERR"                       }, /* Error indicator set on file. */
    { FS_ERR_FILE_INVALID_ACCESS_MODE,        "FILE_INVALID_ACCESS_MODE"       }, /* Access mode is specified invalid. */
    { FS_ERR_FILE_INVALID_ATTRIB,             "FILE_INVALID_ATTRIB"            }, /* Attributes are specified invalid. */
    { FS_ERR_FILE_INVALID_BUF_MODE,           "FILE_INVALID_BUF_MODE"          }, /* Buf mode is specified invalid or unknown. */
    { FS_ERR_FILE_INVALID_BUF_SIZE,           "FILE_INVALID_BUF_SIZE"          }, /* Buf size is specified invalid. */
    { FS_ERR_FILE_INVALID_DATE_TIME,          "FILE_INVALID_DATE_TIME"         }, /* Date/time is specified invalid. */
    { FS_ERR_FILE_INVALID_DATE_TIME_TYPE,     "FILE_INVALID_DATE_TIME_TYPE"    }, /* Date/time type flag is specified invalid. */
    { FS_ERR_FILE_INVALID_NAME,               "FILE_INVALID_NAME"              }, /* Name is specified invalid. */
    { FS_ERR_FILE_INVALID_ORIGIN,             "FILE_INVALID_ORIGIN"            }, /* Origin is specified invalid or unknown. */
    { FS_ERR_FILE_INVALID_OFFSET,             "FILE_INVALID_OFFSET"            }, /* Offset is specified invalid. */
    { FS_ERR_FILE_INVALID_FILES,              "FILE_INVALID_FILES"             }, /* Invalid file arguments. */
    { FS_ERR_FILE_INVALID_OP,                 "FILE_INVALID_OP"                }, /* File operation invalid. */
    { FS_ERR_FILE_INVALID_OP_SEQ,             "FILE_INVALID_OP_SEQ"            }, /* File operation sequence invalid. */
    { FS_ERR_FILE_INVALID_POS,                "FILE_INVALID_POS"               }, /* File position invalid. */
    { FS_ERR_FILE_LOCKED,                     "FILE_LOCKED"                    }, /* File locked. */
    { FS_ERR_FILE_NONE_AVAIL,                 "FILE_NONE_AVAIL"                }, /* No file available. */
    { FS_ERR_FILE_NOT_OPEN,                   "FILE_NOT_OPEN"                  }, /* File NOT open. */
    { FS_ERR_FILE_NOT_LOCKED,                 "FILE_NOT_LOCKED"                }, /* File NOT locked. */
    { FS_ERR_FILE_OVF,                        "FILE_OVF"                       }, /* File size overflowed max file size. */
    { FS_ERR_FILE_OVF_OFFSET,                 "FILE_OVF_OFFSET"                }, /* File offset overflowed max file offset. */

    { FS_ERR_NAME_BASE_TOO_LONG,              "NAME_BASE_TOO_LONG"             }, /* Base name too long. */
    { FS_ERR_NAME_EMPTY,                      "NAME_EMPTY"                     }, /* Name empty. */
    { FS_ERR_NAME_EXT_TOO_LONG,               "NAME_EXT_TOO_LONG"              }, /* Extension too long. */
    { FS_ERR_NAME_INVALID,                    "NAME_INVALID"                   }, /* Invalid file name or path. */
    { FS_ERR_NAME_MIXED_CASE,                 "NAME_MIXED_CASE"                }, /* Name is mixed case. */
    { FS_ERR_NAME_NULL,                       "NAME_NULL"                      }, /* Name ptr arg(s) passed NULL ptr(s). */
    { FS_ERR_NAME_PATH_TOO_LONG,              "NAME_PATH_TOO_LONG"             }, /* Entry path is too long. */
    { FS_ERR_NAME_BUF_TOO_SHORT,              "NAME_BUF_TOO_SHORT"             }, /* Buffer for name is too short. */
    { FS_ERR_NAME_TOO_LONG,                   "NAME_TOO_LONG"                  }, /* Full name is too long. */

    { FS_ERR_PARTITION_INVALID,               "PARTITION_INVALID"              }, /* Partition invalid. */
    { FS_ERR_PARTITION_INVALID_NBR,           "PARTITION_INVALID_NBR"          }, /* Partition nbr specified invalid. */
    { FS_ERR_PARTITION_INVALID_SIG,           "PARTITION_INVALID_SIG"          }, /* Partition sig invalid. */
    { FS_ERR_PARTITION_INVALID_SIZE,          "PARTITION_INVALID_SIZE"         }, /* Partition size invalid. */
    { FS_ERR_PARTITION_MAX,                   "PARTITION_MAX"                  }, /* Max nbr partitions have been created in MBR. */
    { FS_ERR_PARTITION_NOT_FINAL,             "PARTITION_NOT_FINAL"            }, /* Prev partition is not final partition. */
    { FS_ERR_PARTITION_NOT_FOUND,             "PARTITION_NOT_FOUND"            }, /* Partition NOT found. */
    { FS_ERR_PARTITION_ZERO,                  "PARTITION_ZERO"                 }, /* Partition zero. */

    { FS_ERR_POOL_EMPTY,                      "POOL_EMPTY"                     }, /* Pool is empty. */
    { FS_ERR_POOL_FULL,                       "POOL_FULL"                      }, /* Pool is full. */
    { FS_ERR_POOL_INVALID_BLK_ADDR,           "POOL_INVALID_BLK_ADDR"          }, /* Block not found in used pool pointers. */
    { FS_ERR_POOL_INVALID_BLK_IN_POOL,        "POOL_INVALID_BLK_IN_POOL"       }, /* Block found in free pool pointers. */
    { FS_ERR_POOL_INVALID_BLK_IX,             "POOL_INVALID_BLK_IX"            }, /* Block index invalid. */
    { FS_ERR_POOL_INVALID_BLK_NBR,            "POOL_INVALID_BLK_NBR"           }, /* Number blocks specified invalid. */
    { FS_ERR_POOL_INVALID_BLK_SIZE,           "POOL_INVALID_BLK_SIZE"          }, /* Block size specified invalid. */

    { FS_ERR_SYS_TYPE_NOT_SUPPORTED,          "SYS_TYPE_NOT_SUPPORTED"         }, /* File sys type not supported. */
    { FS_ERR_SYS_INVALID_SIG,                 "SYS_INVALID_SIG"                }, /* Sec has invalid OR illegal sig. */
    { FS_ERR_SYS_DIR_ENTRY_PLACE,             "SYS_DIR_ENTRY_PLACE"            }, /* Dir entry could not be placed. */
    { FS_ERR_SYS_DIR_ENTRY_NOT_FOUND,         "SYS_DIR_ENTRY_NOT_FOUND"        }, /* Dir entry not found. */
    { FS_ERR_SYS_DIR_ENTRY_NOT_FOUND_YET,     "SYS_DIR_ENTRY_NOT_FOUND_YET"    }, /* Dir entry not found (yet). */
    { FS_ERR_SYS_SEC_NOT_FOUND,               "SYS_SEC_NOT_FOUND"              }, /* Sec not found. */
    { FS_ERR_SYS_CLUS_CHAIN_END,              "SYS_CLUS_CHAIN_END"             }, /* Cluster chain ended. */
    { FS_ERR_SYS_CLUS_CHAIN_END_EARLY,        "SYS_CLUS_CHAIN_END_EARLY"       }, /* Cluster chain ended before number clusters traversed. */
    { FS_ERR_SYS_CLUS_INVALID,                "SYS_CLUS_INVALID"               }, /* Cluster invalid. */
    { FS_ERR_SYS_CLUS_NOT_AVAIL,              "SYS_CLUS_NOT_AVAIL"             }, /* Cluster not avail. */
    { FS_ERR_SYS_SFN_NOT_AVAIL,               "SYS_SFN_NOT_AVAIL"              }, /* SFN is not avail. */
    { FS_ERR_SYS_LFN_ORPHANED,                "SYS_LFN_ORPHANED"               }, /* LFN entry orphaned. */

    { FS_ERR_VOL_INVALID_NAME,                "VOL_INVALID_NAME"               }, /* Invalid volume name. */
    { FS_ERR_VOL_INVALID_SIZE,                "VOL_INVALID_SIZE"               }, /* Invalid volume size. */
    { FS_ERR_VOL_INVALID_SEC_SIZE,            "VOL_INVALID_SEC_SIZE"           }, /* Invalid volume sector size. */
    { FS_ERR_VOL_INVALID_CLUS_SIZE,           "VOL_INVALID_CLUS_SIZE"          }, /* Invalid volume cluster size. */
    { FS_ERR_VOL_INVALID_OP,                  "VOL_INVALID_OP"                 }, /* Volume operation invalid. */
    { FS_ERR_VOL_INVALID_SEC_NBR,             "VOL_INVALID_SEC_NBR"            }, /* Invalid volume sector number. */
    { FS_ERR_VOL_INVALID_SYS,                 "VOL_INVALID_SYS"                }, /* Invalid file system on volume. */
    { FS_ERR_VOL_NO_CACHE,                    "VOL_NO_CACHE"                   }, /* No cache assigned to volume. */

    { FS_ERR_VOL_NONE_AVAIL,                  "VOL_NONE_AVAIL"                 }, /* No vol  avail. */
    { FS_ERR_VOL_NONE_EXIST,                  "VOL_NONE_EXIST"                 }, /* No vols exist. */
    { FS_ERR_VOL_NOT_OPEN,                    "VOL_NOT_OPEN"                   }, /* Vol NOT open. */
    { FS_ERR_VOL_NOT_MOUNTED,                 "VOL_NOT_MOUNTED"                }, /* Vol NOT mounted. */
    { FS_ERR_VOL_ALREADY_OPEN,                "VOL_ALREADY_OPEN"               }, /* Vol already open. */
    { FS_ERR_VOL_FILES_OPEN,                  "VOL_FILES_OPEN"                 }, /* Files open on vol. */
    { FS_ERR_VOL_DIRS_OPEN,                   "VOL_DIRS_OPEN"                  }, /* Dirs open on vol. */

    { FS_ERR_VOL_JOURNAL_ALREADY_OPEN,        "VOL_JOURNAL_ALREADY_OPEN"       }, /* Journal already open. */
    { FS_ERR_VOL_JOURNAL_CFG_CHNGD,           "VOL_JOURNAL_CFG_CHNGD"          }, /* File system suite cfg changed since log created. */
    { FS_ERR_VOL_JOURNAL_FILE_INVALID,        "VOL_JOURNAL_FILE_INVALID"       }, /* Journal file invalid. */
    { FS_ERR_VOL_JOURNAL_FULL,                "VOL_JOURNAL_FULL"               }, /* Journal full. */
    { FS_ERR_VOL_JOURNAL_LOG_INVALID_ARG,     "VOL_JOURNAL_LOG_INVALID_ARG"    }, /* Invalid arg read from journal log. */
    { FS_ERR_VOL_JOURNAL_LOG_INCOMPLETE,      "VOL_JOURNAL_LOG_INCOMPLETE"     }, /* Log not completely entered in journal. */
    { FS_ERR_VOL_JOURNAL_LOG_NOT_PRESENT,     "VOL_JOURNAL_LOG_NOT_PRESENT"    }, /* Log not present in journal. */
    { FS_ERR_VOL_JOURNAL_NOT_OPEN,            "VOL_JOURNAL_NOT_OPEN"           }, /* Journal not open. */
    { FS_ERR_VOL_JOURNAL_NOT_REPLAYING,       "VOL_JOURNAL_NOT_REPLAYING"      }, /* Journal not being replayed. */
    { FS_ERR_VOL_JOURNAL_NOT_STARTED,         "VOL_JOURNAL_NOT_STARTED"        }, /* Journaling not started. */
    { FS_ERR_VOL_JOURNAL_NOT_STOPPED,         "VOL_JOURNAL_NOT_STOPPED"        }, /* Journaling not stopped. */
    { FS_ERR_VOL_JOURNAL_REPLAYING,           "VOL_JOURNAL_REPLAYING"          }, /* Journal being replayed. */
    { FS_ERR_VOL_JOURNAL_MARKER_NBR_MISMATCH, "VOL_JOURNAL_MARKER_NBR_MISMATCH"}, /* Marker nbr mismatch. */

    { FS_ERR_VOL_LABEL_INVALID,               "VOL_LABEL_INVALID"              }, /* Volume label is invalid. */
    { FS_ERR_VOL_LABEL_NOT_FOUND,             "VOL_LABEL_NOT_FOUND"            }, /* Volume label was not found. */
    { FS_ERR_VOL_LABEL_TOO_LONG,              "VOL_LABEL_TOO_LONG"             }, /* Volume label is too long. */

    { FS_ERR_OS_LOCK,                         "OS_LOCK"                        },
    { FS_ERR_OS_LOCK_TIMEOUT,                 "OS_LOCK_TIMEOUT"                },
    { FS_ERR_OS_INIT,                         "OS_INIT"                        },
    { FS_ERR_OS_INIT_LOCK,                    "OS_INIT_LOCK"                   },
    { FS_ERR_OS_INIT_LOCK_NAME,               "OS_INIT_LOCK_NAME"              }
    };

    uint16_t Count;                    /* Iteration Count */
    uint8_t *pErrStr = "Not Listed";   /* Default String */

    /* Iterate over Fs Error String Table */
    for (Count = 0; Count < (sizeof(xFsErrStringTable) / sizeof(xFsErrStringTable[0])); Count++)
    {
        /* Compare */
        if (xFsErrStringTable[Count].fsErr == fsErr)
        {
            /* Found, copy and break */
            pErrStr = xFsErrStringTable[Count].pFsErrString;
            break;
        }
    }

    /* Log the Error */
    Log (REQ, "FileSys: Error Code = %d, Reason = %s", fsErr, pErrStr);

#else // DEBUG_CODE

     /* Log the Error */
    Log (REQ, "FileSys: Error Code = %d", fsErr);

#endif // DEBUG_CODE

}

/* ========================================================================== */
/**
 * \brief   Force array To Ascii
 *
 * \details This functions sets ASCII non-alphanumeric characters to NULL (0).
 *
 * \param   *SourceBuff - Pointer to the input array of characters
 * \param   *AsciiBuff - Pointer to the output array, which has been forced
 *          to ascii
 * \param   ArraySize - Size of the input array in bytes
 *
 * \return  None
 *
 * ========================================================================== */
bool ForceArrayToAscii(uint8_t *SourceBuff, uint8_t *AsciiBuff, uint16_t ArraySize)
{
    uint8_t    NextChar;
    uint16_t   CharIndex;
    bool       RetVal;

    RetVal =  true;
    do
    {
        if ((NULL == SourceBuff) || (NULL == AsciiBuff))
        {
            RetVal =  false;

            break;
        }

        for (CharIndex = 0; CharIndex < ArraySize; CharIndex++)
        {
            NextChar = SourceBuff[CharIndex];
            if ((NextChar < ' ') || (NextChar > '~'))
            {
                AsciiBuff[CharIndex] = 0;
            }
            else
            {
                AsciiBuff[CharIndex] = NextChar;
            }
        }

        if (AsciiBuff[0] == 0 )
        {
            RetVal =  false;
        }
        else
        {
            AsciiBuff[ArraySize] = 0;
        }

    } while (false);

    return RetVal;
}

/* ========================================================================== */
/**
 * \brief   BinaryArrayToHexString
 *
 * \details This function converts an input array of binary values (interpreted
 *          to be an integer) to ascii characters that represent the numbers
 *          expressed in hexadecimal format.
 *
 * \param   *DataIn - Pointer to the input binary array
 * \param   DataLength - Length of the incoming array
 * \param   *StrOut - Pointer to the output string containing the characters
 *                   representing the value in hexadecimal format.
 * \param   MaxStrLength - Maximum length of the output string.
 *          Since this function converts BCD values to HEX ASCII values,Minimum converted
 *          ascii data length should be 3, two ASCII characters and one null terminator.
 *          For e.g. BCD Byte value 'CA' gets converted to hex ascii C  and A.
 * \param   GetLeastSignificant - TRUE if 1st input byte is least significant,
 *                               (little endian), FALSE otherwise.
 * \param   ReverseOrder - TRUE if result is to be output least significant
 *                        byte 1st (little endian), FALSE otherwise.
 * \return  uint16_t - Size of the resulting output string in bytes.
 *
 * ========================================================================== */
uint16_t BinaryArrayToHexString(uint8_t *DataIn, uint16_t DataLength, char *StrOut, uint16_t MaxStrLength, bool GetLeastSignificant, bool ReverseOrder)
{
    uint16_t   DataOutIndex;
    uint16_t   DataOutIndexOffset;
    uint16_t   DataOutCount;  // How many bytes from DataIn will be copied to StrOut

    DataOutCount = 0;

    do
    {
        if ((NULL == DataIn) || (NULL == StrOut))
        {
            break;
        }

        if (MaxStrLength < MIN_STRING_OUT_SIZE)
        {
            if (MaxStrLength > 0)
            {
                StrOut[0] = 0;
            }

            break;
        }

        if (MaxStrLength < ((DataLength * 2) + 1))
        { // check max string output length is within permissible data length
          //i.e. twice of input data length(BCD to ASCII) and a null terminator
            DataOutCount = (MaxStrLength - 1) / 2;

            if (GetLeastSignificant)
            {
                DataOutIndexOffset = DataLength - DataOutCount;
            }
            else
            {
                DataOutIndexOffset = 0;
            }
        }
        else
        {
            DataOutCount = DataLength;
            DataOutIndexOffset = 0;
        }

        if (ReverseOrder)
        {
            for (DataOutIndex = 0; DataOutIndex < DataOutCount; DataOutIndex++)
            {
                BinaryToHexAscii(DataIn[(DataOutCount - DataOutIndex) + DataOutIndexOffset - 1], &StrOut[DataOutIndex * 2]);
            }
        }
        else
        {
            for (DataOutIndex = 0; DataOutIndex < DataOutCount; DataOutIndex++)
            {
                BinaryToHexAscii(DataIn[DataOutIndex + DataOutIndexOffset], &StrOut[DataOutIndex * 2]);
            }
        }

        StrOut[DataOutCount * 2] = 0;  // NULL terminate the string

    } while (false);

    return DataOutCount;
}

/* ========================================================================== */
/**
 * \brief   Binary To Hex Ascii
 *
 * \details This function converts an input binary value (interpreted to be
 *           an integer) to ascii characters that represent the number expressed
 *           in hexadecimal format.
 *
 * \param   Val - The input binary value (1 byte)
 * \param   *StrOut - Pointer to the output string containing the characters
 *                   representing the value in hexadecimal format.
 *
 * \return  None
 *
 * ========================================================================== */
void BinaryToHexAscii(uint8_t Val, char *StrOut)
{
    if (((Val >> 4) >= 0) && ((Val >> 4) <= 9))
    {
      StrOut[0] = '0' + (Val >> 4);
    }
    else
    {
        StrOut[0] = 'A' + (Val >> 4) - 10;
    }

    if (((Val & 0xF) >= 0) && ((Val & 0xF) <= 9))
    {
        StrOut[1] = '0' + (Val & 0xF);
    }
    else
    {
        StrOut[1] = 'A' + (Val & 0xF) - 10;
    }
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
