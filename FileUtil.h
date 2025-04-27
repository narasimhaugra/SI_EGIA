#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup FSUTIL
 * \{
 *
 * \brief   This header file provides the public interface for the FileUtil.c unit,
 *          including function prototypes.
 *
 * \details This module provides byte/word/long read and write wrappers for micrium
 *          file system read & write.
 *
 * \sa      FileSys.c
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    FileUtil.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "fs_sys.h"
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/
#define SD_CARD_BUFFER_SIZE      (512u)
#define SDHC_BUFFER_SIZE         (512u)             ///< SD Card Buffer Size

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
FS_ERR FsFileWrByte( FS_FILE *pFile, uint8_t  Data );
FS_ERR FsFileWrWord( FS_FILE *pFile, uint16_t Data );
FS_ERR FsFileWrLong( FS_FILE *pFile, uint32_t Data );

FS_ERR FsFileRdByte( FS_FILE *pFile, uint8_t  *pData );
FS_ERR FsFileRdWord( FS_FILE *pFile, uint16_t *pData );
FS_ERR FsFileRdLong( FS_FILE *pFile, uint32_t *pData );

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif  /* FILE_UTIL_H */
