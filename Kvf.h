#ifndef KVF_H
#define KVF_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup KVF
 * \{
 *
 * \brief   Key Value File utilities (KVF)
 *
 * \details This header file defines the public interfaces for KVF handling
 *          as well as symbols to define parameters to be logged.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Kvf.h
 *
 * ========================================================================== */
#include <Common.h>
#include <string.h>
#include "ActiveObject.h"
#include "FileTypes.h"
#include "FileSys.h"
#include "FileUtil.h"
#include "Logger.h"
#include "L4_ConsoleCommands.h"  /// \todo 09/30/2022 JA - For VAR_TYPE. Put in common?  
#include "uC-CRC/Source/edc_Crc.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define KVF_MAJOR_REV      (1u)
#define KVF_MINOR_REV      (1u)
#define KVF_STRING_VALUE_LEN  (64u)
/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum
{
  KVF_ERR_NONE,                     ///< No Error
  KVF_ERR_FILE_DOES_NOT_EXIST,      ///< KVF File doesnt exist
  KVF_ERR_FILE_SYSTEM,              ///< Filesystem Error 
  KVF_ERR_KEY_DOES_NOT_EXIST,       ///< Key Doesnt exist 
  KVF_ERR_KEY_TYPE_DOES_NOT_MATCH,  ///< Key value mismatch 

} KVF_ERROR;

typedef struct
{
  VAR_TYPE VarType;            ///< Variable type, ie: VAR_TYPE_INT8U, VAR_TYPE_INT32U...
  const char *pKeyStr;
  const void *pValueObject;
  const char *pDescriptionStr;

} KVF_MAP;

typedef struct
{
  const KVF_MAP *pMap;
  const char *pDescriptionStr;

} KVF_PARAM;


typedef struct
{
  char *Name;
  uint32_t Value;
  
} KVF_ENUM_ITEM;

typedef struct { bool DefaultValue;} KVF_BOOL;
typedef struct {char DefaultValue[KVF_STRING_VALUE_LEN]; } KVF_STRING;
typedef struct { uint8_t DefaultValue; uint8_t MinVal; uint8_t MaxVal; } KVFInt8u;
typedef struct { int8_t DefaultValue; int8_t MinVal; int8_t MaxVal; } KVFInt8s;
typedef struct { uint16_t DefaultValue; uint16_t MinVal; uint16_t MaxVal; } KVFInt16u;
typedef struct { int16_t DefaultValue; int16_t MinVal; int16_t MaxVal; } KVFInt16s;
typedef struct { uint32_t DefaultValue; uint32_t MinVal; uint32_t MaxVal; } KVFInt32u;
typedef struct { int32_t DefaultValue; int32_t MinVal; int32_t MaxVal; } KVFInt32s;
typedef struct { uint64_t DefaultValue; uint64_t MinVal; uint64_t MaxVal; } KVFInt64u;
typedef struct { int64_t DefaultValue; int64_t MinVal; int64_t maxval; } KVFInt64s;
typedef struct { float32_t DefaultValue; float32_t MinVal; float32_t MaxVal; } KVFFloat32_t;
typedef struct { float64_t DefaultValue; float64_t MinVal; float64_t MaxVal; } KVFFloat64_t;
typedef struct { uint32_t DefaultValue; KVF_ENUM_ITEM const *Items; } KVFEnum;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
void KvfValidate( const KVF_PARAM *pParam, char *pFileName, KVF_ERROR *pError );
uint8_t KvfGetDescription( char *pFileName, char *pBuf, uint8_t maxChars, KVF_ERROR *pError );
bool KvfBoolForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError );
uint8_t KvfInt8uForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError );
int8_t KvfInt8sForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError );
uint16_t KvfInt16uForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError );
int16_t KvfInt16sForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError );
uint32_t KvfInt32uForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError );
int32_t KvfInt32sForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError );
float32_t KvfFp32ForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError );
void KvfStringForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError, char *pStrValue );
uint32_t KvfEnumForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError );


/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif    /* KVF_H */
