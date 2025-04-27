#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup FileSys
 * \{
 *
 * \brief   File system utilities
 *
 * \details This module provides file handling utilities
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    FileUtil.c
 *
 * \todo 11/15/2021 DAZ - These COULD reside in FileSys.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Common.h"
#include "FileUtil.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/

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

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Write a byte to the specified file.
 *
 * \details This is a wrapper for FSFile_Wr, hard coded to write a single byte
 *
 * \param   pFile - Pointer to FS_FILE structure
 * \param   Data  - Byte to write to the file
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also occur, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                   File written successfully.
 *          FS_ERR_NULL_PTR               Argument 'pFile'/'pBuffer' passed a NULL ptr.
 *          FS_ERR_INVALID_TYPE           Argument 'pFile's TYPE is invalid or
 *                                        unknown.
 *          FS_ERR_FILE_ERR               File has error.
 *          FS_ERR_FILE_OVF               File size/position would be overflowed if
 *                                                                 write were executed.
 *
 *          FS_ERR_FILE_NOT_OPEN          File NOT open.
 *
 *          FS_ERR_BUF_NONE_AVAIL         No buffer available.
 *          FS_ERR_DEV                    Device access error.
 *          FS_ERR_DEV_FULL               Device is full (no space could be allocated).
 *          FS_ERR_ENTRY_CORRUPT          File system entry is corrupt.
 *
 * ========================================================================== */
FS_ERR FsFileWrByte(FS_FILE *pFile, uint8_t Data)
{
    FS_ERR FsErr;

    FSFile_Wr(pFile, &Data, sizeof(uint8_t), &FsErr);
    return FsErr;
}

/* ========================================================================== */
/**
 * \brief   Write a word to the specified file.
 *
 * \details This is a wrapper for FSFile_Wr, hard coded to write a word
 *
 * \param   pFile - Pointer to FS_FILE structure
 * \param   Data  - Byte to write to the file
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also occur, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                   File written successfully.
 *          FS_ERR_NULL_PTR               Argument 'pFile'/'pBuffer' passed a NULL ptr.
 *          FS_ERR_INVALID_TYPE           Argument 'pFile's TYPE is invalid or
 *                                        unknown.
 *          FS_ERR_FILE_ERR               File has error.
 *          FS_ERR_FILE_OVF               File size/position would be overflowed if
 *                                                                 write were executed.
 *
 *          FS_ERR_FILE_NOT_OPEN          File NOT open.
 *
 *          FS_ERR_BUF_NONE_AVAIL         No buffer available.
 *          FS_ERR_DEV                    Device access error.
 *          FS_ERR_DEV_FULL               Device is full (no space could be allocated).
 *          FS_ERR_ENTRY_CORRUPT          File system entry is corrupt.
 *
 * ========================================================================== */
FS_ERR FsFileWrWord(FS_FILE *pFile, uint16_t Data)
{
    FS_ERR FsErr;

    FSFile_Wr(pFile, &Data, sizeof(uint16_t), &FsErr);
    return FsErr;
}

/* ========================================================================== */
/**
 * \brief   Write a long word to the specified file.
 *
 * \details This is a wrapper for FSFile_Wr, hard coded to write a long word
 *
 * \param   pFile - Pointer to FS_FILE structure
 * \param   Data  - Byte to write to the file
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also occur, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                   File written successfully.
 *          FS_ERR_NULL_PTR               Argument 'pFile'/'pBuffer' passed a NULL ptr.
 *          FS_ERR_INVALID_TYPE           Argument 'pFile's TYPE is invalid or
 *                                        unknown.
 *          FS_ERR_FILE_ERR               File has error.
 *          FS_ERR_FILE_OVF               File size/position would be overflowed if
 *                                                                 write were executed.
 *
 *          FS_ERR_FILE_NOT_OPEN          File NOT open.
 *
 *          FS_ERR_BUF_NONE_AVAIL         No buffer available.
 *          FS_ERR_DEV                    Device access error.
 *          FS_ERR_DEV_FULL               Device is full (no space could be allocated).
 *          FS_ERR_ENTRY_CORRUPT          File system entry is corrupt.
 *
 * ========================================================================== */
FS_ERR FsFileWrLong(FS_FILE *pFile, uint32_t Data)
{
    FS_ERR FsErr;

    FSFile_Wr(pFile, &Data, sizeof(INT32U), &FsErr);
    return FsErr;
}

#if USE_KVF_VALUES | DEBUG_CODE
/* ========================================================================== */
/**
 * \brief   Read a byte from the specified file.
 *
 * \details This is a wrapper for FSFile_Rd, hard coded to read a byte
 *
 * \param   pFile - Pointer to FS_FILE structure
 * \param   pData - Pointer to store byte
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also occur, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                   File read successfully.
 *          FS_ERR_NULL_PTR               Argument 'pFile'/'pBuffer' passed a NULL ptr.
 *          FS_ERR_INVALID_TYPE           Argument 'pFile's TYPE is invalid or
 *                                        unknown.
 *          FS_ERR_FILE_ERR               File has error.
 *          FS_ERR_FILE_NOT_OPEN          File NOT open.
 *
 *          FS_ERR_BUF_NONE_AVAIL         No buffer available.
 *          FS_ERR_DEV                    Device access error.
 *          FS_ERR_DEV_FULL               Device is full (no space could be allocated).
 *          FS_ERR_ENTRY_CORRUPT          File system entry is corrupt.
 *
 * ========================================================================== */
FS_ERR FsFileRdByte(FS_FILE *pFile, uint8_t *pData)
{
    FS_ERR FsErr;

    FSFile_Rd(pFile, pData, sizeof(uint8_t), &FsErr);
    return FsErr;
}
#endif

#if USE_KVF_VALUES | DEBUG_CODE
/* ========================================================================== */
/**
 * \brief   Read a word from the specified file.
 *
 * \details This is a wrapper for FSFile_Rd, hard coded to read a word
 *
 * \param   pFile - Pointer to FS_FILE structure
 * \param   pData - Pointer to store word
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also occur, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                   File read successfully.
 *          FS_ERR_NULL_PTR               Argument 'pFile'/'pBuffer' passed a NULL ptr.
 *          FS_ERR_INVALID_TYPE           Argument 'pFile's TYPE is invalid or
 *                                        unknown.
 *          FS_ERR_FILE_ERR               File has error.
 *          FS_ERR_FILE_NOT_OPEN          File NOT open.
 *
 *          FS_ERR_BUF_NONE_AVAIL         No buffer available.
 *          FS_ERR_DEV                    Device access error.
 *          FS_ERR_DEV_FULL               Device is full (no space could be allocated).
 *          FS_ERR_ENTRY_CORRUPT          File system entry is corrupt.
 *
 * ========================================================================== */
FS_ERR FsFileRdWord(FS_FILE *pFile, uint16_t *pData)
{
    FS_ERR FsErr;

    FSFile_Rd(pFile, pData, sizeof(INT16U), &FsErr);
    return FsErr;
}
#endif

#if USE_KVF_VALUES | DEBUG_CODE
/* ========================================================================== */
/**
 * \brief   Read a long word from the specified file.
 *
 * \details This is a wrapper for FSFile_Rd, hard coded to read a long word
 *
 * \param   pFile - Pointer to FS_FILE structure
 * \param   pData - Pointer to store long word
 *
 * \return  FS_ERR - FILE SYSTEM ERROR CODES, mainly following, others might
 *                   also occur, refer to "fs_err.h" for the full list
 *
 *          FS_ERR_NONE                   File read successfully.
 *          FS_ERR_NULL_PTR               Argument 'pFile'/'pBuffer' passed a NULL ptr.
 *          FS_ERR_INVALID_TYPE           Argument 'pFile's TYPE is invalid or
 *                                        unknown.
 *          FS_ERR_FILE_ERR               File has error.
 *          FS_ERR_FILE_NOT_OPEN          File NOT open.
 *
 *          FS_ERR_BUF_NONE_AVAIL         No buffer available.
 *          FS_ERR_DEV                    Device access error.
 *          FS_ERR_DEV_FULL               Device is full (no space could be allocated).
 *          FS_ERR_ENTRY_CORRUPT          File system entry is corrupt.
 *
 * ========================================================================== */
FS_ERR FsFileRdLong(FS_FILE *pFile, uint32_t *pData)
{
    FS_ERR FsErr;

    FSFile_Rd(pFile, pData, sizeof(uint32_t), &FsErr);
    return FsErr;
}
#endif

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
