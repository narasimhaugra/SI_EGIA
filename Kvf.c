#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Utils
 * \{
 *
 * \brief   Key Value File Utilities (KVF)
 *
 * \details This module provides functions for the handling of Key Value Files.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Kvf.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Kvf.h"
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER  (LOG_GROUP_FILE_SYS)
#define SIZE_TYPE_BOOL        (2u)
#define SIZE_TYPE_INT8        (4u)
#define SIZE_TYPE_INT16       (8u)
#define SIZE_TYPE_INT32       (16u)
#define SIZE_TYPE_INT64       (32u)
#define SIZE_TYPE_STRING      (KVF_STRING_VALUE_LEN*2u) 
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
static CRC_MODEL_16 KvfCrcModel =
{
  0x8005u,              //Polynomial to use in calculation.
  0xFFFFu,              //initial value
  DEF_NO,               // Reflect 16 bit value
  0x0000u,              // xor value
  &CRC_TblCRC16_8005[0] //Crc table pointer
};
/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
// The full path to the active data directory
CPU_CHAR DataFilePath[CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS_LEN + 25];
/******************************************************************************/
/*                             Local Function ProtoType(s)                    */
/******************************************************************************/
static FS_FILE *GetFileForKey( const char *pKeyName, VAR_TYPE type, char *pFileName, KVF_ERROR *pError );
static bool CompareKeyWithFile( FS_FILE *pFile, const char *pKeyName, uint16_t CrcKey );
static void RewriteKeyValueFile( const KVF_PARAM *pParam, char *pFileName, KVF_ERROR *pError );
static void CreateDefaultKeyValueFile( char *pFileName, const KVF_PARAM *pParam );
static uint8_t GetObjectSize( VAR_TYPE type );
static uint16_t GetEnumSize( KVF_ENUM_ITEM const *pEnumTypeArray );
static void KvfBackup(char *pFileName);
/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**  
 * \brief    Compare Key With File
 *
 * \details  Compare passed in key to one in the file after verifying
 *           passed in CRC matches the CRC in the file.
 * 
 * \param   *pFile - string containing file name to search for key ..
 * \param   *pKeyName - string containing key to compare ..
 * \param   *CrcKey - CRC of key in memory
 * 
 * \return  bool - true on Success. FAIL on error.
 *
 * ========================================================================== */
static bool CompareKeyWithFile( FS_FILE *pFile, const char *pKeyName, uint16_t CrcKey )
{
    uint8_t StrLen;
    uint8_t Indx;
    uint8_t TempChar;
    uint16_t CrcFile;
    bool RetVal;

    do
    {
        RetVal = true;
        // get Crc  
        FsFileRdWord(pFile, &CrcFile);
        if (CrcFile != CrcKey) 
        {
            RetVal = false;
            break;
        }

        // get string length  
        FsFileRdByte(pFile, &StrLen);
        if (StrLen != Str_Len(pKeyName)) 
        {
            RetVal = false;
            break;
        }

        for (Indx=0; Indx < StrLen; Indx++) 
        {   
            FsFileRdByte(pFile, &TempChar);
            if (TempChar != pKeyName[Indx]) 
            {
              RetVal = false;
              break;
            }
        }
        
    } while (false);
    // if we make it here, string passes
    return RetVal;
}
/** ========================================================================== */
/** 
 * \brief   Get File For Key
 *
 * \details This function will search the specified file for the
 *          specified key of the specified VarType.
 *
 * \param   *pKeyName - string containing Key to match ..
 * \param   VarType - VarType to match key Name string ..
 * \param   *pFileName - string containing file name to search for key ..
 * \param   *pError - KVF_ERR_NONE = if return Value is valid,
 *                    others defined in KVF_ERROR in kvf.h
 *
 * \return  FS_FILE* - Return a pointer to the file if there are no errors.
 *                     otherwise return a null pointer to a FS_FILE object.
 * ========================================================================== */ 
static FS_FILE *GetFileForKey( const char *pKeyName, VAR_TYPE VarType, char *pFileName, KVF_ERROR *pError )
{
    FS_FILE *pFile;
    FS_ERR FsError;
    uint8_t DescriptionLen;
    VAR_TYPE FileType;
    uint16_t CrcKey;
    uint16_t ObjectSize;
    EDC_ERR CrcErr;
    uint32_t ObjectPos;
    uint8_t RdByte;
    
    do
    {
        // guard against null error pointer
        if (pError == (KVF_ERROR*)0) 
        {
            pFile = NULL;
            break;
        }

        // calculate Crc of key
        CrcKey = CRC_ChkSumCalc_16Bit(&KvfCrcModel, (void*)pKeyName, sizeof(char) * Str_Len(pKeyName), &CrcErr);

        // try to open file
        pFile = FSFile_Open(pFileName, FS_FILE_ACCESS_MODE_RD, &FsError);
        if (FsError != FS_ERR_NONE) 
        {
            *pError = KVF_ERR_FILE_DOES_NOT_EXIST;
            pFile = NULL;
            break;
        }

        // move past header
        FSFile_PosSet(pFile, 4, FS_FILE_ORIGIN_START, &FsError);


        // move past description
        FsFileRdByte(pFile, &DescriptionLen);
        FSFile_PosSet(pFile, DescriptionLen, FS_FILE_ORIGIN_CUR, &FsError);

        // search for Key Name in file
        while (FSFile_IsEOF(pFile, &FsError) == false) 
        {
            // get object size
            FsFileRdWord(pFile, &ObjectSize);
            // get object position
            ObjectPos = FSFile_PosGet(pFile, &FsError);

            if (CompareKeyWithFile(pFile, pKeyName, CrcKey) == true) 
            {
                // key has been found, move past description
                FsFileRdByte(pFile, &DescriptionLen);
                FSFile_PosSet(pFile, DescriptionLen, FS_FILE_ORIGIN_CUR, &FsError);

                // get VarType field and validate against map
                FsFileRdByte(pFile, &RdByte);
                FileType = (VAR_TYPE)RdByte;
                if (FileType != VarType) 
                {
                    FSFile_Close(pFile, &FsError);
                    *pError = KVF_ERR_KEY_TYPE_DOES_NOT_MATCH;                    
                    break;
                } 
                else 
                {
                    *pError = KVF_ERR_NONE;
                    break;
                }
            }

            // skip over object data to next object
            FSFile_PosSet(pFile, ObjectPos+ObjectSize, FS_FILE_ORIGIN_START, &FsError);      
        }
        if (KVF_ERR_NONE != *pError)
        {
            // if we get here, then end of file is reached, search failed
            FSFile_Close(pFile, &FsError);
            *pError = KVF_ERR_KEY_DOES_NOT_EXIST;
            pFile = NULL;
        }
        
    } while(false);
    
    return pFile;
}
/* ========================================================================== */
/**  
 * \brief   Rewrite Key Value File
 * 
 * \details - Rewrite the Key Value to the specified file.

 * \param   *pParam - kvf parameters to write ..
 * \param   *pFileName -  name of key file.  ..
 * \param   *pError - FS_ERR_NONE on Success. KVF_ERR_FILE_SYSTEM error on file open error.
 * 
 * \return  None
 *
 * ========================================================================== */

static void RewriteKeyValueFile( const KVF_PARAM *pParam, char *pFileName, KVF_ERROR *pError )
{
    const KVF_MAP *pMap = pParam->pMap;
    FS_FILE *pSourceFile;
    FS_FILE *pTempFile;
    FS_ERR FsError;
    uint8_t DescriptionLen;
    static uint8_t TempNum=0;
    char TempFileName[7]; // enough room for 'tmpXXX'
    uint8_t KeyNameLen;
    uint16_t ObjectSize;
    uint16_t Crc;
    EDC_ERR CrcErr;
    uint8_t ByteValue;
    uint8_t BoolValue;
    uint16_t WordValue;
    uint32_t LongValue;
    char Value[KVF_STRING_VALUE_LEN];
    uint8_t ItemCount;

    const KVF_BOOL *pDataBool;
    const KVFInt8u *pDataByte;
    const KVFInt16u *pDataWord;
    const KVFInt32u *pDataLong;
    const KVF_STRING *pDataStr;

    const KVFEnum *pEnumData;
    KVF_ENUM_ITEM const *pNextEnumItem;

    do
    {
        // create temp file
        snprintf(TempFileName, sizeof(TempFileName) - 1, "tmp%d", TempNum++);
        pTempFile = FSFile_Open(TempFileName, FS_FILE_ACCESS_MODE_WR | FS_FILE_ACCESS_MODE_CREATE, &FsError);
        if (FsError != FS_ERR_NONE) 
        {
          *pError = KVF_ERR_FILE_SYSTEM;
          break;
        }

        // Write header
        FsFileWrWord(pTempFile, FILE_TYPE_ID_KVF);
        FsFileWrByte(pTempFile, KVF_MAJOR_REV);
        FsFileWrByte(pTempFile, KVF_MINOR_REV);

        // just write description from new, no need to check if different
        DescriptionLen = Str_Len_N(pParam->pDescriptionStr, 0xFF);
        FsFileWrByte(pTempFile, DescriptionLen);
        FSFile_Wr(pTempFile, (void*)pParam->pDescriptionStr, sizeof(char)*DescriptionLen, &FsError);

        // go through each map entry and write object parameters (check old file for each key name)
        while (pMap->VarType != VAR_TYPE_UNKNOWN) 
        {
            KeyNameLen = Str_Len_N(pMap->pKeyStr, 0xFF);
            DescriptionLen = Str_Len_N(pMap->pDescriptionStr, 0xFF);
            ObjectSize = 5 + KeyNameLen + DescriptionLen;
            
            if (pMap->VarType == VAR_TYPE_ENUM)
            {
               ObjectSize += GetEnumSize(((KVFEnum *)pMap->pValueObject)->Items);
            }
            else
            {
               ObjectSize += GetObjectSize(pMap->VarType);
            }
            // write object size
            FsFileWrWord(pTempFile, ObjectSize);

            // write out key's CRC
            Crc = CRC_ChkSumCalc_16Bit(&KvfCrcModel, (void*)pMap->pKeyStr, sizeof(char) * KeyNameLen, &CrcErr);
            FsFileWrWord(pTempFile, Crc);

            // write out key name
            FsFileWrByte(pTempFile, KeyNameLen);
            FSFile_Wr(pTempFile, (void*)pMap->pKeyStr, KeyNameLen, &FsError);

            // write out description
            FsFileWrByte(pTempFile, DescriptionLen);
            FSFile_Wr(pTempFile, (void*)pMap->pDescriptionStr, DescriptionLen, &FsError);

            // Write out 'VarType'
            FsFileWrByte(pTempFile, (uint8_t)pMap->VarType);

            // search to see if key is in old file
            pSourceFile = GetFileForKey(pMap->pKeyStr, pMap->VarType, pFileName, pError);

            switch (pMap->VarType) 
            {
                case VAR_TYPE_BOOL: 
                    pDataBool = pMap->pValueObject;
                    BoolValue = pDataBool->DefaultValue;
                    if(*pError == KVF_ERR_NONE)
                    {
                        FsFileRdByte(pSourceFile, &BoolValue);
                    }
                    FsFileWrByte(pTempFile, BoolValue);
                    FsFileWrByte(pTempFile, pDataBool->DefaultValue);
                  
                    break;
                    
                case VAR_TYPE_INT8U:
                case VAR_TYPE_INT8S:              
                    pDataByte = pMap->pValueObject;
                    ByteValue = pDataByte->DefaultValue;
                    
                    if(*pError == KVF_ERR_NONE)
                    {
                      FsFileRdByte(pSourceFile, &ByteValue);
                    }
                    
                    FsFileWrByte(pTempFile, ByteValue);
                    FsFileWrByte(pTempFile, pDataByte->DefaultValue);
                    FsFileWrByte(pTempFile, pDataByte->MinVal);
                    FsFileWrByte(pTempFile, pDataByte->MaxVal);            
                    break;
                    
                case VAR_TYPE_INT16U:
                case VAR_TYPE_INT16S: 
                    pDataWord = pMap->pValueObject;              
                    WordValue = pDataWord->DefaultValue;
                    if(*pError == KVF_ERR_NONE)
                    {
                      FsFileRdWord(pSourceFile, &WordValue);
                    }
                    FsFileWrWord(pTempFile, WordValue);
                    FsFileWrWord(pTempFile, pDataWord->DefaultValue);
                    FsFileWrWord(pTempFile, pDataWord->MinVal);
                    FsFileWrWord(pTempFile, pDataWord->MaxVal);                  
                    break;
                    
                case VAR_TYPE_INT32U:
                case VAR_TYPE_INT32S:
                case VAR_TYPE_FP32: 
                    pDataLong = pMap->pValueObject;
                    LongValue = pDataLong->DefaultValue;
                    if(*pError == KVF_ERR_NONE)
                    {
                        FsFileRdLong(pSourceFile, &LongValue);
                    }
                    FsFileWrLong(pTempFile, LongValue);
                    FsFileWrLong(pTempFile, pDataLong->DefaultValue);
                    FsFileWrLong(pTempFile, pDataLong->MinVal);
                    FsFileWrLong(pTempFile, pDataLong->MaxVal);            
                    break;
                case VAR_TYPE_STRING:
                    pDataStr = pMap->pValueObject;
                    if (*pError == KVF_ERR_NONE)
                    {
                       FSFile_Rd(pTempFile, Value, KVF_STRING_VALUE_LEN, &FsError);
                    }
                    else
                    {
                       memcpy(Value, pDataStr->DefaultValue, KVF_STRING_VALUE_LEN);
                    }

                    FSFile_Wr(pTempFile, (void *)Value, KVF_STRING_VALUE_LEN, &FsError);
                    FSFile_Wr(pTempFile, (void *)(pDataStr->DefaultValue), KVF_STRING_VALUE_LEN, &FsError);            
                    break;

                case VAR_TYPE_ENUM:
                    pEnumData = pMap->pValueObject;
                    ItemCount = 0;
                    pEnumData = pMap->pValueObject;  
                    pNextEnumItem = pEnumData->Items;
                    LongValue = pEnumData->DefaultValue;
                    
                    if(*pError == KVF_ERR_NONE)
                    {
                      FsFileRdLong(pSourceFile, &LongValue);
                    }
                    
                    // Current Value
                    FsFileWrLong(pTempFile, LongValue);
                    // Default Value
                    FsFileWrLong(pTempFile, pEnumData->DefaultValue);

                    // Count the number of items
                    while (pNextEnumItem->Name != NULL)
                    {
                       ItemCount++;
                       pNextEnumItem++;
                    }
                    FsFileWrByte(pTempFile, ItemCount);

                    pNextEnumItem = pEnumData->Items;
                    while (pNextEnumItem->Name != NULL)
                    {
                        // Write the enum's name length and string
                        DescriptionLen = Str_Len_N(pNextEnumItem->Name, 0xFF);
                        FsFileWrByte(pTempFile, DescriptionLen);
                        FSFile_Wr(pTempFile, (void*)pNextEnumItem->Name, sizeof(char)*DescriptionLen, &FsError);

                        // Write the enum's Value
                        FsFileWrLong(pTempFile, pNextEnumItem->Value);
                        pNextEnumItem++;
                    }                  
                    break;
            }

            if (*pError == KVF_ERR_NONE) 
            {
              FSFile_Close(pSourceFile, &FsError);
            }
           
            pMap++;
        }

        FSFile_Close(pTempFile, &FsError);

        // rename temp file to to new file
        FSEntry_Rename(TempFileName, pFileName, DEF_NO, &FsError);
        
    } while (false);
}
/** ========================================================================== */
 /** 
 * \brief   Get Object Size
 *
 * \details This function returns the size of specified objects.
 *
 * \param   VarType - Type of object to find.
 *
 * \return  uint8_t - return the size of the specified object. Return 0 if
 *          an unknown object was passed in.
 *
 * ========================================================================== */ 
static uint8_t GetObjectSize( VAR_TYPE VarType )
{ 
    uint8_t Size;
    
    Size = 0;
    switch (VarType) 
    {
        case VAR_TYPE_BOOL:
            Size = SIZE_TYPE_BOOL;
            break;
        case VAR_TYPE_INT8U:
        case VAR_TYPE_INT8S:
            Size = SIZE_TYPE_INT8;
            break;
        case VAR_TYPE_INT16U:
        case VAR_TYPE_INT16S:
            Size = SIZE_TYPE_INT16;
            break;
        case VAR_TYPE_INT32U:
        case VAR_TYPE_INT32S:
        case VAR_TYPE_FP32:
            Size = SIZE_TYPE_INT32;       // Active Value, Default Value, Min Value, Max Value
            break;
        case VAR_TYPE_INT64U:
        case VAR_TYPE_INT64S:
        case VAR_TYPE_FP64:
            Size = SIZE_TYPE_INT64;
            break;
        case VAR_TYPE_STRING:
            Size = SIZE_TYPE_STRING;
            break;
            
        default:
        case VAR_TYPE_ENUM:   // must use GetEnumSize for VAR_TYPE_ENUM
        case VAR_TYPE_UNKNOWN:
            Size = 0;
            break;
    }
    return Size;
}

/** ========================================================================== */
 /** 
 * \brief   Get Enum Size
 *
 * \details Calculates and returns the total size of an enum array (KVF_ENUM_ITEM[])
 *          The size includes the lengths of the name strings, not the size of the 
 *          pointers.
 *
 * \param   *pEnumTypeArray - pointer to an array of KVF_ENUM_ITEM
 * \param   *pParam - Key File Parameters.
 *
 * \return  uint16_t - size of the ENUM array
 *
 * ========================================================================== */ 
static uint16_t GetEnumSize( KVF_ENUM_ITEM const *pEnumTypeArray )
{
    KVF_ENUM_ITEM const *pNextEnumItem = pEnumTypeArray;
    
    // Current Value + Default Value + Item Count
    uint16_t TotalSize = sizeof(pNextEnumItem->Value) + sizeof(pNextEnumItem->Value) + 1;

    while( pNextEnumItem->Name != NULL )
    {
        TotalSize += 1;   // string length of name
        TotalSize += Str_Len_N(pNextEnumItem->Name, 0xFF);
        TotalSize += sizeof(pNextEnumItem->Value);
        pNextEnumItem++;
    }
    return TotalSize;
}
/** ========================================================================== */
 /** 
 * \brief   Create Default Key Value File
 *
 * \details  This function creates a default key Value file in absense of a
 *           particular Kvf.
 * \param   pFileName    - Name of Default Key File.
 * \param   *pParam - Key File Parameters.
 *
 * \return  None
 *
 * ========================================================================== */  
static void CreateDefaultKeyValueFile( char *pFileName, const KVF_PARAM *pParam )
{
  const KVF_MAP *pMap = pParam->pMap;
  FS_FILE *pFile;
  FS_ERR FsErr;
  EDC_ERR CrcErr;
  uint8_t KeyNameLen;
  uint8_t DescriptionLen;
  uint16_t ObjectSize;
  uint16_t Crc=0;

  do
  {
      // Create file
      pFile = FSFile_Open(pFileName, FS_FILE_ACCESS_MODE_WR | FS_FILE_ACCESS_MODE_CREATE, &FsErr);
      if (FsErr != FS_ERR_NONE) 
      {
        break;
      }

      // file header info
      FsErr = FsFileWrWord(pFile, FILE_TYPE_ID_KVF);
      FsErr = FsFileWrByte(pFile, KVF_MAJOR_REV);
      FsErr = FsFileWrByte(pFile, KVF_MINOR_REV);

      // File Description
      DescriptionLen = Str_Len_N(pParam->pDescriptionStr, 0xFF);
      FsErr = FsFileWrByte(pFile, DescriptionLen);
      FSFile_Wr(pFile, (void*)pParam->pDescriptionStr, sizeof(char)*DescriptionLen, &FsErr);
      

      // Go through each map entry and write the value object parameters to the file
      while (pMap->VarType != VAR_TYPE_UNKNOWN) 
      {
        KeyNameLen = Str_Len_N(pMap->pKeyStr, 0xFF);
        DescriptionLen = Str_Len_N(pMap->pDescriptionStr, 0xFF);

        // the uint8_t is pMap->VarType typecast into 8 bits
        ObjectSize = sizeof(ObjectSize) + sizeof(Crc) + KeyNameLen + DescriptionLen + sizeof(uint8_t);
        if (pMap->VarType == VAR_TYPE_ENUM)
        {
           ObjectSize += GetEnumSize(((KVFEnum *)pMap->pValueObject)->Items);
        }
        else
        {
           ObjectSize += GetObjectSize(pMap->VarType);
        }

        // write size
        FsErr = FsFileWrWord(pFile, ObjectSize);

        // write out Crc
        Crc = CRC_ChkSumCalc_16Bit(&KvfCrcModel, (void*)pMap->pKeyStr, sizeof(char) * KeyNameLen, &CrcErr);
        FsErr = FsFileWrWord(pFile, Crc);

        // Key Name
        FsErr = FsFileWrByte(pFile, KeyNameLen);
        FSFile_Wr(pFile, (void*)pMap->pKeyStr, KeyNameLen, &FsErr);

        // description
        FsErr = FsFileWrByte(pFile, DescriptionLen);
        FSFile_Wr(pFile, (void*)pMap->pDescriptionStr, DescriptionLen, &FsErr);

        // Type
        FsErr = FsFileWrByte(pFile, (uint8_t)pMap->VarType);

        switch (pMap->VarType) 
        {
          case VAR_TYPE_BOOL: {
              const KVF_BOOL *pBool = pMap->pValueObject;
              FsErr = FsFileWrByte(pFile, pBool->DefaultValue);
              FsErr = FsFileWrByte(pFile, pBool->DefaultValue);
            }
            break;
          case VAR_TYPE_INT8U:
          case VAR_TYPE_INT8S: {
              const KVFInt8u *pInt8u = pMap->pValueObject;
              FsErr = FsFileWrByte(pFile, pInt8u->DefaultValue);
              FsErr = FsFileWrByte(pFile, pInt8u->DefaultValue);
              FsErr = FsFileWrByte(pFile, pInt8u->MinVal);
              FsErr = FsFileWrByte(pFile, pInt8u->MaxVal);
            }
            break;
          case VAR_TYPE_INT16U:
          case VAR_TYPE_INT16S: {
              const KVFInt16u *pInt16u = pMap->pValueObject;
              FsErr = FsFileWrWord(pFile, pInt16u->DefaultValue);
              FsErr = FsFileWrWord(pFile, pInt16u->DefaultValue);
              FsErr = FsFileWrWord(pFile, pInt16u->MinVal);
              FsErr = FsFileWrWord(pFile, pInt16u->MaxVal);
            }
            break;
          case VAR_TYPE_INT32U:
          case VAR_TYPE_INT32S:
          case VAR_TYPE_FP32: {
              const KVFInt32u *pInt32u = pMap->pValueObject;
              FsErr = FsFileWrLong(pFile, pInt32u->DefaultValue);
              FsErr = FsFileWrLong(pFile, pInt32u->DefaultValue);
              FsErr = FsFileWrLong(pFile, pInt32u->MinVal);
              FsErr = FsFileWrLong(pFile, pInt32u->MaxVal);
            }
            break;
          case VAR_TYPE_STRING: {
              const KVF_STRING *pString = pMap->pValueObject;
              // Current value
              FSFile_Wr(pFile, (void *)(pString->DefaultValue), KVF_STRING_VALUE_LEN, &FsErr);
              // Default value
              FSFile_Wr(pFile, (void *)(pString->DefaultValue), KVF_STRING_VALUE_LEN, &FsErr);
            }
            break;

          case VAR_TYPE_ENUM: {
              uint8_t ItemCount = 0;
              const KVFEnum *pEnum = pMap->pValueObject;
              KVF_ENUM_ITEM const *NextEnumItem = pEnum->Items;

              // Current value
              FsErr = FsFileWrLong(pFile, pEnum->DefaultValue);
              // Default value
              FsErr = FsFileWrLong(pFile, pEnum->DefaultValue);

              // Count the number of items
              while (NextEnumItem->Name != NULL)
              {
                  ItemCount++;
                  NextEnumItem++;
              }
              FsErr = FsFileWrByte(pFile, ItemCount);

              NextEnumItem = pEnum->Items;
              while (NextEnumItem->Name != NULL)
              {
                  // Write the enum's name length and string
                  DescriptionLen = Str_Len_N(NextEnumItem->Name, 0xFF);
                  FsErr = FsFileWrByte(pFile, DescriptionLen);
                  FSFile_Wr(pFile, (void*)NextEnumItem->Name, sizeof(char)*DescriptionLen, &FsErr);

                  // Write the enum's value
                  FsErr = FsFileWrLong(pFile, NextEnumItem->Value);
                  NextEnumItem++;
              }
            }
            break;
        }
        
        pMap++;
      }

      FSFile_Close(pFile, &FsErr);
  
  } while (false);
}
/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   kvf backup
 * 
 * \details Create a kvf backup file.
 * 
 * \param   *pFileName - File name of the kvf backup file. 
 *           
 * \return  None
 * ========================================================================== */
void KvfBackup(char *pFileName)
{
   FS_ERR FsError;
   char * BackslashLocation;
   CPU_CHAR SettingFileDest[CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS_LEN + 25 + 20];

   BackslashLocation = strrchr(pFileName, '\\');
   
   if (BackslashLocation != NULL)
   {
      snprintf(SettingFileDest, sizeof(SettingFileDest), "%s\\%s", DataFilePath, BackslashLocation + 1);
   }
   else
   {
      snprintf(SettingFileDest, sizeof(SettingFileDest), "%s\\%s", DataFilePath, pFileName);
   }
   FSEntry_Copy(pFileName, SettingFileDest, true, &FsError);   
}
/* ========================================================================== */
/**
 * \brief   kvf validate
 *
 * \details validate device kvf. Create a new kvf if it doesnt exist.
 *          Check file parameters and update in case of a mismatch.    
 * \param   pParam    - pointer to KVF parameter to validate.
 * \param   pFileName - string containing file name to search.
 * \param   pError    - KVF_ERR_NONE = if return Value is valid,
 *                      others are defined in KVF_ERROR in kvf.h 
 *
 * \return  None
 *
 * ========================================================================== */  
void KvfValidate( const KVF_PARAM *pParam, char *pFileName, KVF_ERROR *pError )
{
    const KVF_MAP *pMap = pParam->pMap;
    FS_FILE *pFile;
    FS_ERR FsError;
    uint8_t KeyNameLen;
    uint8_t DescriptionLen;
    uint16_t ObjectSize;
    uint32_t ObjectPos;
    uint16_t Crc;
    uint16_t Indx;
    VAR_TYPE VarType;
    EDC_ERR CrcErr;
    uint32_t RdLong;
    uint16_t RdWord;
    uint8_t RdByte;
    bool ParameterErr;    
    bool rewrite;    
    
    ParameterErr = false;
    rewrite = false;
    
    do
    {
        pFile = FSFile_Open(pFileName, FS_FILE_ACCESS_MODE_RD, &FsError);
        
        if (FsError == FS_ERR_ENTRY_NOT_FOUND) 
        {
            // if file is not there, create it, then we are done
            CreateDefaultKeyValueFile(pFileName, pParam);
            /// \todo 10/13/2022 JA:  Save in correct directory path. 
            //KvfBackup(pFileName);  
            *pError = KVF_ERR_NONE;
            break; 
        } 
        else if (FsError != FS_ERR_NONE) 
        {
            *pError = KVF_ERR_FILE_SYSTEM;
            break; 
        }
        /// \todo 10/13/2022 JA: Save in correct directory path.
        //KvfBackup(pFileName);

        // from this point on there should be no errors unless re-writing causes them
        *pError = KVF_ERR_NONE;

        // move past header
        FSFile_PosSet(pFile, 4, FS_FILE_ORIGIN_START, &FsError);
        
        // check description length
        FsFileRdByte(pFile, &DescriptionLen);
        if (DescriptionLen != Str_Len(pParam->pDescriptionStr)) 
        {
            FSFile_Close(pFile, &FsError);
            RewriteKeyValueFile(pParam, pFileName, pError);
            break;        
        }
        // check description string
        for (Indx=0; Indx < DescriptionLen; Indx++) 
        {
            FsFileRdByte(pFile, &RdByte);
            if (pParam->pDescriptionStr[Indx] != RdByte)
            {
                FSFile_Close(pFile, &FsError);
                RewriteKeyValueFile(pParam, pFileName, pError);
                ParameterErr = true;
                break;
            }
        }
        if (ParameterErr == true)
        {
            break;
        }
        
        // Go through each map entry and check against the file
        while (pMap->VarType != VAR_TYPE_UNKNOWN)
        {            
            // get object size
             FsFileRdWord(pFile, &ObjectSize);
            // get object pos
            ObjectPos = FSFile_PosGet(pFile, &FsError);

            // check Crc in file against key in map
            Crc = CRC_ChkSumCalc_16Bit(&KvfCrcModel, (void*)pMap->pKeyStr, sizeof(char) * Str_Len(pMap->pKeyStr), &CrcErr);

            FsFileRdWord(pFile, &RdWord);
            if (Crc != RdWord) 
            {
                FSFile_Close(pFile, &FsError);
                RewriteKeyValueFile(pParam, pFileName, pError);  
                ParameterErr = true;  
                break;
            }

            // check key name length
            FsFileRdByte(pFile, &KeyNameLen);
            if (KeyNameLen != Str_Len(pMap->pKeyStr)) 
            {
                FSFile_Close(pFile, &FsError);
                RewriteKeyValueFile(pParam, pFileName, pError); 
                ParameterErr = true;
                break;
            }

            // check key name
            for (Indx=0; Indx < KeyNameLen; Indx++) 
            {
                FsFileRdByte(pFile, &RdByte);
                if (pMap->pKeyStr[Indx] != RdByte) 
                {
                    FSFile_Close(pFile, &FsError);
                    RewriteKeyValueFile(pParam, pFileName, pError);                
                    ParameterErr = true;
                    break;
                }
            }
            // check description length
            FsFileRdByte(pFile, &DescriptionLen);
            if (DescriptionLen != Str_Len(pMap->pDescriptionStr)) 
            {
                FSFile_Close(pFile, &FsError);
                RewriteKeyValueFile(pParam, pFileName, pError); 
                ParameterErr = true;
                break;
            }
            // check description string
            for (Indx=0; Indx < DescriptionLen; Indx++) 
            {
                FsFileRdByte(pFile, &RdByte);
                if (pMap->pDescriptionStr[Indx] != RdByte) 
                {
                    FSFile_Close(pFile, &FsError);
                    RewriteKeyValueFile(pParam, pFileName, pError); 
                    ParameterErr = true;
                    break;
                }
            }
            
            if(ParameterErr == true)
            {
                break;//breaks out of while loop 
            }
            
            // check VarType
            FsFileRdByte(pFile, &RdByte);
            VarType = (VAR_TYPE)RdByte;
            
            if (pMap->VarType != VarType) 
            {
                FSFile_Close(pFile, &FsError);
                RewriteKeyValueFile(pParam, pFileName, pError);            
                break;
            }

            
            // check min/max values for data
            switch (VarType) 
            {
                case VAR_TYPE_BOOL: 
                  {
                    const KVF_BOOL *pData = pMap->pValueObject;
                    FSFile_PosSet(pFile, 1, FS_FILE_ORIGIN_CUR, &FsError);
                    FsFileRdByte(pFile, &RdByte);
                    
                    if (pData->DefaultValue != RdByte) 
                    {
                        rewrite = true;
                    }
                  }
                  break;
                case VAR_TYPE_INT8U:
                case VAR_TYPE_INT8S: 
                  {
                    const KVFInt8u *pData = pMap->pValueObject;
                    FSFile_PosSet(pFile, 1, FS_FILE_ORIGIN_CUR, &FsError);
                    
                    FsFileRdByte(pFile, &RdByte);
                    if (pData->DefaultValue != RdByte) 
                    {
                      rewrite = true;
                    }
                    FsFileRdByte(pFile, &RdByte);
                    if (pData->MinVal != RdByte) 
                    {
                      rewrite = true;
                    }
                    FsFileRdByte(pFile, &RdByte);
                    if (pData->MaxVal != RdByte) 
                    {
                      rewrite = true;
                    }
                  }
                  break;
                case VAR_TYPE_INT16U:
                case VAR_TYPE_INT16S: 
                  {
                    const KVFInt16u *pData = pMap->pValueObject;
                    FSFile_PosSet(pFile, 2, FS_FILE_ORIGIN_CUR, &FsError);
                    
                    FsFileRdWord(pFile, &RdWord);
                    if (pData->DefaultValue != RdWord) 
                    {
                      rewrite = true;
                    }
                    FsFileRdWord(pFile, &RdWord);
                    if (pData->MinVal != RdWord) 
                    {
                      rewrite = true;
                    }
                    FsFileRdWord(pFile, &RdWord);
                    if (pData->MaxVal != RdWord) 
                    {
                      rewrite = true;
                    }
                  }
                  break;
                case VAR_TYPE_INT32U:
                case VAR_TYPE_INT32S:
                case VAR_TYPE_FP32:   
                  {
                    const KVFInt32u *pData = pMap->pValueObject;
                    FSFile_PosSet(pFile, 4, FS_FILE_ORIGIN_CUR, &FsError);
                    
                    FsFileRdLong(pFile, &RdLong);                
                    if (pData->DefaultValue != RdLong) 
                    {
                        rewrite = true;
                    }
                    FsFileRdLong(pFile, &RdLong);
                    if (pData->MinVal != RdLong) 
                    {
                        rewrite = true;
                    }
                    FsFileRdLong(pFile, &RdLong);
                    if (pData->MaxVal != RdLong) 
                    {
                        rewrite = true;
                    }
                  }
                  break;

                case VAR_TYPE_STRING:   
                  {
                    const KVF_STRING *pData = pMap->pValueObject;
                    char defaultValue[KVF_STRING_VALUE_LEN];

                    // Skip over the current Value
                    FSFile_PosSet(pFile, KVF_STRING_VALUE_LEN, FS_FILE_ORIGIN_CUR, &FsError);
                    // Read the default Value
                    FSFile_Rd(pFile, defaultValue, KVF_STRING_VALUE_LEN, &FsError);
                    if (FsError != FS_ERR_NONE)
                    {
                        rewrite = true;
                    }
                    if (memcmp(pData->DefaultValue, defaultValue, KVF_STRING_VALUE_LEN) != 0)
                    {
                        rewrite = true;
                    }
                  }
                  break;


                case VAR_TYPE_ENUM:   
                  {
                    const KVFEnum *pData = pMap->pValueObject;
                    uint8_t  itemCount;
                    uint8_t  itemIndex;
                    uint8_t  nameLength;
                    uint32_t defaultValue;

                    // Skip over the current Value
                    FSFile_PosSet(pFile, sizeof(pData->DefaultValue), FS_FILE_ORIGIN_CUR, &FsError);
                    // Read the default Value
                    FSFile_Rd(pFile, &defaultValue, sizeof(pData->DefaultValue), &FsError);
                    if (FsError != FS_ERR_NONE)
                    {
                        rewrite = true;
                    }
                    if (memcmp(&pData->DefaultValue, &defaultValue, sizeof(pData->DefaultValue)) != 0)
                    {
                        rewrite = true;
                    }

                    FsFileRdByte(pFile, &itemCount);
                    for (itemIndex = 0 ; itemIndex < itemCount; itemIndex++)
                    {
                         // Read the next enum's name length
                         FSFile_Rd(pFile, &nameLength, sizeof(nameLength), &FsError);
                         // Skip over the name's Value
                         FSFile_PosSet(pFile, nameLength, FS_FILE_ORIGIN_CUR, &FsError);

                         // Skip over the Value
                         FSFile_PosSet(pFile, sizeof(pData->DefaultValue), FS_FILE_ORIGIN_CUR, &FsError);
                    }
                  }
                  break;
            }
            if (rewrite == true) 
            {
                FSFile_Close(pFile, &FsError);
                RewriteKeyValueFile(pParam, pFileName, pError);            
                break;
            }

            // move file pointer to next object
            FSFile_PosSet(pFile, ObjectPos+ObjectSize, FS_FILE_ORIGIN_START, &FsError);

            // increment to next map entry
            pMap++;
        }// while loop end 

        // if we get here, all is well, close file and return
        if ((rewrite == false) && (ParameterErr == false)) 
        {
            FSFile_Close(pFile, &FsError);
        }
        
    } while (false);
}


/* ========================================================================== */
/**
 * \brief   Kvf Get Description
 *
 * \details This function will copy the description of the kvf file into the buffer
 *          and file name provided.
 *
 * \param   pFileName - string containing file name to search for key 
 * \param   pBuf - buffer to copy data to.
 * \param   MaxChars - maximum number of characters to copy.
 * \param   pError - KVF_ERR_NONE = if return Value is valid,
 *                    others defined in KVF_ERROR in kvf.h
 *
 * \return  None
 *
 * ========================================================================== */  
uint8_t KvfGetDescription( char *pFileName, char *pBuf, uint8_t MaxChars, KVF_ERROR *pError )
{
    FS_FILE *pFile;
    FS_ERR FsError;
    uint8_t DescriptionLen;

    pFile = FSFile_Open(pFileName, FS_FILE_ACCESS_MODE_RD, &FsError);
    
    DescriptionLen = 0;
    
    do
    {
        if (FsError == FS_ERR_ENTRY_NOT_FOUND) 
        {
            *pError = KVF_ERR_FILE_DOES_NOT_EXIST;
            break;
        } 
        else if (FsError != FS_ERR_NONE) 
        {
            *pError = KVF_ERR_FILE_SYSTEM;
            break;
        }

        // move past header
        FSFile_PosSet(pFile, 4, FS_FILE_ORIGIN_START, &FsError);
        
        // get description
        FsFileRdByte(pFile, &DescriptionLen);
        
        if (DescriptionLen > MaxChars) 
        {
            DescriptionLen = MaxChars;
        }

        // Copy description into buffer
        FSFile_Rd(pFile, pBuf, sizeof(char)*DescriptionLen, &FsError);

        FSFile_Close(pFile, &FsError);

        *pError = KVF_ERR_NONE;
        
    } while ( false );

    return DescriptionLen;
}
/* ========================================================================== */
/**  
 * \brief Kvf Bool For Key
 * 
 * \details This function returns a bool Value for a key in
 *          the provided file name.
 * 
 * \param   pKeyName - string containing Key to match ..
 * \param   pFileName - string containing file name to search for key ..
 * \param   pError - KVF_ERR_NONE = if return Value is valid,
 *                    others defined in KVF_ERROR in kvf.h
 * 
 * \return  bool - boolean Value
 *
 * ========================================================================== */
bool KvfBoolForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError )
{
  FS_FILE *pFile;
  FS_ERR FsError;
  bool Value;
  uint8_t RdByte;

  Value = false;
  pFile = GetFileForKey(pKeyName, VAR_TYPE_BOOL, pFileName, pError);
  FsFileRdByte(pFile, &RdByte);
  Value = RdByte;
  FSFile_Close(pFile, &FsError);
  
  return (pFile == (FS_FILE*)0) ? false : Value;
}
/* ========================================================================== */
/**  
 * \brief Kvf Int8u For Key
 *
 * \details This function will return a uint8_t Value for a key in
 *          the provided file name.
 * 
 * \param   pKeyName - string containing Key to match ..
 * \param   pFileName - string containing file name to search for key ..
 * \param   pError - KVF_ERR_NONE = if return Value is valid,
 *                    others defined in KVF_ERROR in kvf.h
 * 
 * \return  uint8_t - 8-bit Value
 *
 * ========================================================================== */
uint8_t KvfInt8uForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError )
{
  FS_FILE *pFile;
  FS_ERR FsError;
  uint8_t Value;

  Value = 0;
  pFile = GetFileForKey(pKeyName, VAR_TYPE_INT8U, pFileName, pError);
  FsFileRdByte(pFile, &Value);
  FSFile_Close(pFile, &FsError);
  
  return (pFile == (FS_FILE*)0) ? 0 : Value;
}
/* ========================================================================== */
/**  
 * \brief   Kvf Int8s For Key
 *
 * \details This function will return a int8_t Value for a key in
 *          the provided file name.
 * 
 * \param   *pKeyName - string containing Key to match ..
 * \param   *pFileName - string containing file name to search for key ..
 * \param   *pError - KVF_ERR_NONE = if return Value is valid,
 *                    others defined in KVF_ERROR in kvf.h
 * 
 * \return  int8_t - 8-bit Value
 *
 * ========================================================================== */
int8_t KvfInt8sForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError )
{
  FS_FILE *pFile;
  FS_ERR FsError;
  uint8_t Value;

  Value=0;
  pFile = GetFileForKey(pKeyName, VAR_TYPE_INT8S, pFileName, pError);
  FsFileRdByte(pFile, &Value);
  FSFile_Close(pFile, &FsError);
  
  return (pFile == (FS_FILE*)0) ? 0 : Value;
}

/* ========================================================================== */
/**  
 * \brief   Kvf Int16u For Key
 * 
 * \details This function will return a uint16_t Value for a key in
 *          the provided file name.
 
 * \param   *pKeyName - string containing Key to match ..
 * \param   *pFileName - string containing file name to search for key ..
 * \param   *pError - KVF_ERR_NONE = if return Value is valid,
 *                    others defined in KVF_ERROR in kvf.h
 * 
 * \return  uint16_t - 16-bit Value
 *
 * ========================================================================== */
uint16_t KvfInt16uForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError )
{
  FS_FILE *pFile;
  FS_ERR FsError;
  uint16_t Value;
  
  Value=0;
  pFile = GetFileForKey(pKeyName, VAR_TYPE_INT16U, pFileName, pError);
  FsFileRdWord(pFile, &Value);
  FSFile_Close(pFile, &FsError);
  
  return (pFile == (FS_FILE*)0) ? 0 : Value;
}

/* ========================================================================== */
/**  
 * \brief   Kvf Int16s For Key
 *
 * \details This function will return a int16_t Value for a key in
 *          the provided file name.
 
 * \param   *pKeyName - string containing Key to match ..
 * \param   *pFileName - string containing file name to search for key ..
 * \param   *pError - KVF_ERR_NONE = if return Value is valid,
 *                    others defined in KVF_ERROR in kvf.h
 * 
 * \return  int16_t - 16-bit Value
 *
 * ========================================================================== */
int16_t KvfInt16sForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError )
{
  FS_FILE *pFile;
  FS_ERR FsError;
  uint16_t Value;

  Value = 0;
  pFile = GetFileForKey(pKeyName, VAR_TYPE_INT16S, pFileName, pError);
  FsFileRdWord(pFile, &Value);
  FSFile_Close(pFile, &FsError);
  
  return (pFile == (FS_FILE*)0) ? 0 : Value;
}

/* ========================================================================== */
/**  
 * \brief   Kvf Int32u For Key
 * 
 * \details This function will return a uint32_t Value for a key in
 *          the provided file name.
 *
 * \param   *pKeyName - string containing Key to match ..
 * \param   *pFileName - string containing file name to search for key ..
 * \param   *pError - KVF_ERR_NONE = if return Value is valid,
 *                    others defined in KVF_ERROR in kvf.h
 * 
 * \return  uint32_t - 32-bit Value
 *
 * ========================================================================== */
uint32_t KvfInt32uForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError )
{
  FS_FILE *pFile;
  FS_ERR FsError;
  uint32_t Value;

  Value=0;
  pFile = GetFileForKey(pKeyName, VAR_TYPE_INT32U, pFileName, pError);
  FsFileRdLong(pFile, &Value);
  FSFile_Close(pFile, &FsError);
  
  return (pFile == (FS_FILE*)0) ? 0 : Value;
}

/* ========================================================================== */
/**  
 * \brief   Kvf Int32s For Key
 * 
 * \details This function will return a int32_t Value for a key in
 *          the provided file name.
 *
 * \param   *pKeyName - string containing Key to match ..
 * \param   *pFileName - string containing file name to search for key ..
 * \param   *pError - KVF_ERR_NONE = if return Value is valid,
 *                    others defined in KVF_ERROR in kvf.h
 * 
 * \return  int32_t - 32-bit Value
 *
 * ========================================================================== */
int32_t KvfInt32sForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError )
{
  FS_FILE *pFile;
  FS_ERR FsError;
  uint32_t Value;

  Value=0;
  pFile = GetFileForKey(pKeyName, VAR_TYPE_INT32S, pFileName, pError);
  FsFileRdLong(pFile, &Value);
  FSFile_Close(pFile, &FsError);
  
  return (pFile == (FS_FILE*)0) ? 0 : Value;
}

/* ========================================================================== */
/**  
 * \brief   Kvf float32 For Key
 *
 * \details This function(s) will return a float32_t Value for a key in
 *          the provided file name.
 
 * \param   *pKeyName - string containing Key to match ..
 * \param   *pFileName - string containing file name to search for key ..
 * \param   *pError - KVF_ERR_NONE = if return Value is valid,
 *                    others defined in KVF_ERROR in kvf.h
 * 
 * \return  float32_t - 32-bit floating point Value
 *
 * ========================================================================== */
float32_t KvfFp32ForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError )
{
  FS_FILE *pFile;
  FS_ERR FsError;
  float32_t Value=0;

  pFile = GetFileForKey(pKeyName, VAR_TYPE_FP32, pFileName, pError);
  FSFile_Rd(pFile, &Value, sizeof(float32_t), &FsError);
  FSFile_Close(pFile, &FsError);
  
  return (pFile == (FS_FILE*)0) ? 0 : Value;
}

/* ========================================================================== */
/**  
 * \brief   Kvf String For Key
 *
 * \details This function will return a string Value for a key
 *          in the provided file name.
 * 
 * \param   *pKeyName - string containing Key to match ..
 * \param   *pFileName - string containing file name to search for key ..
 * \param   *pError - KVF_ERR_NONE = if return Value is valid,
 *                    others defined in KVF_ERROR in kvf.h ..
 * \param   *pStrValue - pointer to a char[KVF_STRING_VALUE_LEN]
 *                       to receive the string Value
 * 
 * \return  None
 *
 * ========================================================================== */
void KvfStringForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError, char *pStrValue )
{
  FS_FILE *pFile;
  FS_ERR FsError;

  *pStrValue = 0; // On error, return a NULL string

  do
  {
      pFile = GetFileForKey(pKeyName, VAR_TYPE_STRING, pFileName, pError);
      if (*pError != KVF_ERR_NONE)
      {
         break;
      }
      FSFile_Rd(pFile, pStrValue, KVF_STRING_VALUE_LEN, &FsError);
      if (FsError != FS_ERR_NONE)
         *pError = KVF_ERR_FILE_SYSTEM;

      FSFile_Close(pFile, &FsError);
      
  } while (false);
}

/* ========================================================================== */
/**  
 * \brief   Kvf Enum For Key
 *
 * \details This function will return an uint32_t Value for a key in
 *          the provided file name.
 * \param   *pKeyName - string containing Key to match ..
 * \param   *pFileName - string containing file name to search for key ..
 * \param   *pError - KVF_ERR_NONE = if return Value is valid,
 *                    others defined in KVF_ERROR in kvf.h
 * 
 * \return  uint32_t - 32-bit Value
 *
 * ========================================================================== */
uint32_t KvfEnumForKey( const char *pKeyName, char *pFileName, KVF_ERROR *pError )
{
  FS_FILE *pFile;
  FS_ERR FsError;
  uint32_t Value;

  Value = 0;
  pFile = GetFileForKey(pKeyName, VAR_TYPE_ENUM, pFileName, pError);
  FsFileRdLong(pFile, &Value);
  FSFile_Close(pFile, &FsError);
  
  return (pFile == (FS_FILE*)0) ? 0 : Value;
}


/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
