#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup CRC
 * \{
 * \brief   CRC
 *
 * \details The CRC Module provides functions, which compute an 8-,16-, or
 *          32-bit Cyclic Redundancy Check (CRC) value for input data. The
 *          input data may be either a single value or an array of values.
 *          The following functions are provided:
 *               CRC8        - Computes the 8-bit CRC for a buffer of
 *                             unsigned bytes. ...
 *               CRC16       - Computes the 16-bit CRC for a buffer of
 *                             unsigned bytes. ...
 *               CRC32       - Computes the 32-bit CRC for a buffer of
 *                             unsigned bytes. ...
 *               DoCRC8      - Computes the 8-bit CRC for a single
 *                             unsigned byte. ...
 * ...
 *               DoCRC16     - Computes the 16-bit CRC for a single
 *                             unsigned 16-bit integer
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file Crc.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Common.h"
#include "Crc.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
/* 8 bit CRC for SMBus with Polynomial = X8+x2+X1+1, init value = 0*/
const uint8_t SMBusCrcTable[] =
{

    0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
    0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
    0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
    0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
    0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5,
    0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
    0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85,
    0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
    0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
    0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
    0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2,
    0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
    0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32,
    0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
    0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
    0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
    0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c,
    0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
    0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
    0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
    0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
    0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
    0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c,
    0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
    0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b,
    0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
    0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
    0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
    0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
    0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
    0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb,
    0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3

};

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define CRC16_POLY          (0xC001u)            ///< Polynomial used for CRC16 calculation
#define SHIFT_LEFT_BY_ONE   (1u)
#define MSBIT_BYTE          (0x80u)
#define SET_LSBIT           (1u)
#define CRC16_MSB           (0x8000u)
#define CRC16_CCITT_POLY    (0x11021u)
#define BYTE_MASK           (0xFFu)

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/* This CRC table is used for calculating One Wire 8 bit CRCs: */
static const uint8_t dscrc_table[] =    /// \bug Need source for this table. What polynomial used to calculate it? Reference?
{
    0,   94,  188, 226,  97,  63, 221, 131, 194, 156, 126,  32, 163, 253,  31,  65,
    157, 195,  33, 127, 252, 162,  64,  30,  95,   1, 227, 189,  62,  96, 130, 220,
    35, 125, 159, 193,  66,  28, 254, 160, 225, 191,  93,   3, 128, 222,  60,  98,
    190, 224,   2,  92, 223, 129,  99,  61, 124,  34, 192, 158,  29,  67, 161, 255,
    70,  24, 250, 164,  39, 121, 155, 197, 132, 218,  56, 102, 229, 187,  89,   7,
    219, 133, 103,  57, 186, 228,   6,  88,  25,  71, 165, 251, 120,  38, 196, 154,
    101,  59, 217, 135,   4,  90, 184, 230, 167, 249,  27,  69, 198, 152, 122,  36,
    248, 166,  68,  26, 153, 199,  37, 123,  58, 100, 134, 216,  91,   5, 231, 185,
    140, 210,  48, 110, 237, 179,  81,  15,  78,  16, 242, 172,  47, 113, 147, 205,
    17,  79, 173, 243, 112,  46, 204, 146, 211, 141, 111,  49, 178, 236,  14,  80,
    175, 241,  19,  77, 206, 144, 114,  44, 109,  51, 209, 143,  12,  82, 176, 238,
    50, 108, 142, 208,  83,  13, 239, 177, 240, 174,  76,  18, 145, 207,  45, 115,
    202, 148, 118,  40, 171, 245,  23,  73,   8,  86, 180, 234, 105,  55, 213, 139,
    87,   9, 235, 181,  54, 104, 138, 212, 149, 203,  41, 119, 244, 170,  72,  22,
    233, 183,  85,  11, 136, 214,  52, 106,  43, 117, 151, 201,  74,  20, 246, 168,
    116,  42, 200, 150,  21,  75, 169, 247, 182, 232,  10,  84, 215, 137, 107,  53
};

/* Array index:  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* Entry = 0 if # of bits of index is even, 1 if odd. (This is really EVEN parity) */
static const uint16_t oddparity[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

static const uint32_t crc32Table[] =  /// \bug Need source for this table. What polynomial used to calculate it? Standard CRC32 table 0x04C11DB7 (?)
{
    0x00000000u, 0x77073096u, 0xee0e612cu, 0x990951bau, 0x076dc419u, 0x706af48fu,
    0xe963a535u, 0x9e6495a3u, 0x0edb8832u, 0x79dcb8a4u, 0xe0d5e91eu, 0x97d2d988u,
    0x09b64c2bu, 0x7eb17cbdu, 0xe7b82d07u, 0x90bf1d91u, 0x1db71064u, 0x6ab020f2u,
    0xf3b97148u, 0x84be41deu, 0x1adad47du, 0x6ddde4ebu, 0xf4d4b551u, 0x83d385c7u,
    0x136c9856u, 0x646ba8c0u, 0xfd62f97au, 0x8a65c9ecu, 0x14015c4fu, 0x63066cd9u,
    0xfa0f3d63u, 0x8d080df5u, 0x3b6e20c8u, 0x4c69105eu, 0xd56041e4u, 0xa2677172u,
    0x3c03e4d1u, 0x4b04d447u, 0xd20d85fdu, 0xa50ab56bu, 0x35b5a8fau, 0x42b2986cu,
    0xdbbbc9d6u, 0xacbcf940u, 0x32d86ce3u, 0x45df5c75u, 0xdcd60dcfu, 0xabd13d59u,
    0x26d930acu, 0x51de003au, 0xc8d75180u, 0xbfd06116u, 0x21b4f4b5u, 0x56b3c423u,
    0xcfba9599u, 0xb8bda50fu, 0x2802b89eu, 0x5f058808u, 0xc60cd9b2u, 0xb10be924u,
    0x2f6f7c87u, 0x58684c11u, 0xc1611dabu, 0xb6662d3du, 0x76dc4190u, 0x01db7106u,
    0x98d220bcu, 0xefd5102au, 0x71b18589u, 0x06b6b51fu, 0x9fbfe4a5u, 0xe8b8d433u,
    0x7807c9a2u, 0x0f00f934u, 0x9609a88eu, 0xe10e9818u, 0x7f6a0dbbu, 0x086d3d2du,
    0x91646c97u, 0xe6635c01u, 0x6b6b51f4u, 0x1c6c6162u, 0x856530d8u, 0xf262004eu,
    0x6c0695edu, 0x1b01a57bu, 0x8208f4c1u, 0xf50fc457u, 0x65b0d9c6u, 0x12b7e950u,
    0x8bbeb8eau, 0xfcb9887cu, 0x62dd1ddfu, 0x15da2d49u, 0x8cd37cf3u, 0xfbd44c65u,
    0x4db26158u, 0x3ab551ceu, 0xa3bc0074u, 0xd4bb30e2u, 0x4adfa541u, 0x3dd895d7u,
    0xa4d1c46du, 0xd3d6f4fbu, 0x4369e96au, 0x346ed9fcu, 0xad678846u, 0xda60b8d0u,
    0x44042d73u, 0x33031de5u, 0xaa0a4c5fu, 0xdd0d7cc9u, 0x5005713cu, 0x270241aau,
    0xbe0b1010u, 0xc90c2086u, 0x5768b525u, 0x206f85b3u, 0xb966d409u, 0xce61e49fu,
    0x5edef90eu, 0x29d9c998u, 0xb0d09822u, 0xc7d7a8b4u, 0x59b33d17u, 0x2eb40d81u,
    0xb7bd5c3bu, 0xc0ba6cadu, 0xedb88320u, 0x9abfb3b6u, 0x03b6e20cu, 0x74b1d29au,
    0xead54739u, 0x9dd277afu, 0x04db2615u, 0x73dc1683u, 0xe3630b12u, 0x94643b84u,
    0x0d6d6a3eu, 0x7a6a5aa8u, 0xe40ecf0bu, 0x9309ff9du, 0x0a00ae27u, 0x7d079eb1u,
    0xf00f9344u, 0x8708a3d2u, 0x1e01f268u, 0x6906c2feu, 0xf762575du, 0x806567cbu,
    0x196c3671u, 0x6e6b06e7u, 0xfed41b76u, 0x89d32be0u, 0x10da7a5au, 0x67dd4accu,
    0xf9b9df6fu, 0x8ebeeff9u, 0x17b7be43u, 0x60b08ed5u, 0xd6d6a3e8u, 0xa1d1937eu,
    0x38d8c2c4u, 0x4fdff252u, 0xd1bb67f1u, 0xa6bc5767u, 0x3fb506ddu, 0x48b2364bu,
    0xd80d2bdau, 0xaf0a1b4cu, 0x36034af6u, 0x41047a60u, 0xdf60efc3u, 0xa867df55u,
    0x316e8eefu, 0x4669be79u, 0xcb61b38cu, 0xbc66831au, 0x256fd2a0u, 0x5268e236u,
    0xcc0c7795u, 0xbb0b4703u, 0x220216b9u, 0x5505262fu, 0xc5ba3bbeu, 0xb2bd0b28u,
    0x2bb45a92u, 0x5cb36a04u, 0xc2d7ffa7u, 0xb5d0cf31u, 0x2cd99e8bu, 0x5bdeae1du,
    0x9b64c2b0u, 0xec63f226u, 0x756aa39cu, 0x026d930au, 0x9c0906a9u, 0xeb0e363fu,
    0x72076785u, 0x05005713u, 0x95bf4a82u, 0xe2b87a14u, 0x7bb12baeu, 0x0cb61b38u,
    0x92d28e9bu, 0xe5d5be0du, 0x7cdcefb7u, 0x0bdbdf21u, 0x86d3d2d4u, 0xf1d4e242u,
    0x68ddb3f8u, 0x1fda836eu, 0x81be16cdu, 0xf6b9265bu, 0x6fb077e1u, 0x18b74777u,
    0x88085ae6u, 0xff0f6a70u, 0x66063bcau, 0x11010b5cu, 0x8f659effu, 0xf862ae69u,
    0x616bffd3u, 0x166ccf45u, 0xa00ae278u, 0xd70dd2eeu, 0x4e048354u, 0x3903b3c2u,
    0xa7672661u, 0xd06016f7u, 0x4969474du, 0x3e6e77dbu, 0xaed16a4au, 0xd9d65adcu,
    0x40df0b66u, 0x37d83bf0u, 0xa9bcae53u, 0xdebb9ec5u, 0x47b2cf7fu, 0x30b5ffe9u,
    0xbdbdf21cu, 0xcabac28au, 0x53b39330u, 0x24b4a3a6u, 0xbad03605u, 0xcdd70693u,
    0x54de5729u, 0x23d967bfu, 0xb3667a2eu, 0xc4614ab8u, 0x5d681b02u, 0x2a6f2b94u,
    0xb40bbe37u, 0xc30c8ea1u, 0x5a05df1bu, 0x2d02ef8du
};

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
 * \brief   Calculate the 8 bit CRC of the specified buffer
 *
 * \details This function is used to calculate CRC's for One Wire ID verification.
 *          It uses a table of pre-calculated CRCs to eliminate the need for shifting,
 *          testing, and xoring a bit at a time.
 * \n \n
 *          If the buffer pointer is NULL, or the size is 0, then the
 *          incoming CRC is returned.
 *
 * \param   crc -    Starting CRC value
 * \param   pBuf -   Pointer to buffer to calculate CRC of.
 * \param   size -   Number of bytes in buffer
 *
 * \return  Updated (if possible) CRC
 *
 * ========================================================================== */
uint8_t CRC8(uint8_t crc, const uint8_t *pBuf, size_t size)
{
    const uint8_t *pByte;

    do
    {
        if ((NULL == pBuf) || (0 == size))
        {
            break;             // No buffer / size - just return incoming CRC
        }

        pByte = pBuf;

        while (size)
        {
            crc = dscrc_table[crc ^ *pByte];
            pByte++;
            size--;
        }

    } while (false);

    return crc;
}

/* ========================================================================== */
/**
 * \brief   Calculate the 16-bit CRC of the specified buffer
 *
 * \details The incoming 16-bit CRC is updated with the data from the specified
 *          buffer. If the buffer pointer is NULL, or the size is 0, then the
 *          incoming CRC is returned.
 *
 * \todo    Operation of this function needs to be explained.
 *
 * \param   crc16In -   16-bit CRC to update
 * \param   pBuf -      Pointer to data buffer
 * \param   size -      Number of bytes to include in CRC
 *
 * \return  Updated (if possible) CRC
 *
 * ========================================================================== */
uint16_t CRC16(uint16_t crc16In, uint8_t *pBuf, size_t size)
{
    uint16_t data;
    const uint8_t *pByte;

    do
    {
        if ((NULL == pBuf) || (0 == size))      // No buffer or size - return incoming crc
        {
            break;
        }

        pByte = pBuf;

        while (size)
        {
            data = (*pByte ^ (crc16In & 0xff)) & 0xff;
            crc16In >>= 8;

            if (oddparity[data & 0x0f] ^ oddparity[data >> 4])  ///  \todo  05/05/20 DAZ - HOW DOES THIS WORK?
            {
                crc16In ^= CRC16_POLY;
            }

            data <<= 6;
            crc16In ^= data;
            data <<= 1;
            crc16In ^= data;

            pByte++;
            size--;
        }

    } while (false);

    return crc16In;
}

/* ========================================================================== */
/**
 * \brief   Calculate the 32-bit CRC of the specified buffer
 *
 * \details The incoming 32-bit CRC is updated with the data from the specified
 *          buffer. It uses a table of pre-calculated CRCs to eliminate the need
 *          for shifting, testing, and xoring a bit at a time.
 * \n \n
 *          If the buffer pointer is NULL, or the size is 0, then the
 *          1's complement of the incoming CRC is returned.
 *
 *          \bug I think returning the 1's complement of the incoming CRC is wrong. This needs research.
 *
 * \todo    Operation of this function needs to be explained.
 *
 * \param   crc -   Incoming 32-bit CRC
 * \param   pBuf -  Pointer to data buffer
 * \param   size -  Number of bytes to include in CRC
 *
 * \return  Updated (if possible) CRC
 *
 * ========================================================================== */
uint32_t CRC32(uint32_t crc, const uint8_t *pBuf, size_t size)
{
    const uint8_t *pByte;

    pByte = pBuf;

    do
    {
        crc = crc ^ ~0U;

        if ((NULL == pBuf) || (0 == size))    /// \bug This is the way it was in the original code, but I think it's wrong. Shouldn't we return UNMODIFIED crc?
        {
            break;
        }

        while (size)
        {
            crc = crc32Table[(crc ^ *pByte) & BYTE_MASK] ^ (crc >> 8);   // Use table
            pByte++;
            size--;
        }

        crc = crc ^ ~0U;                    // Invert the final result

    } while (false);

    return crc;
}


/* ========================================================================== */
/**
 * \brief   Update 8-bit crc with one byte
 *
 * \param   crc8    - Incoming CRC
 * \param   value   - Value to add to CRC
 *
 * \return  Updated CRC
 *
 * ========================================================================== */
uint8_t DoCRC8(uint8_t crc8, uint8_t value)
{
    uint8_t crc8New;

    crc8New = dscrc_table[crc8 ^ value];
    return crc8New;
}


/* ========================================================================== */
/**
 * \brief   Update 8-bit crc with one byte used for SMBUS CRC calculation
 *
 * \details 8 bit CRC calculation for SMBUS PEC
 *              polynomial = X8+x2+x1+1
 *
 * \param   crc8    - Incoming CRC
 * \param   value   - Value to add to CRC
 *
 * \return  Updated CRC
 *
 * ========================================================================== */
/*
uint8_t DoSMBusCRC8(uint8_t crc8, uint8_t value)
{
    uint8_t crc8New;

    crc8New = SMBusCrcTable[crc8 ^ value];
    return crc8New;
}
*/
/* ========================================================================== */
/**
 * \brief   Update 16-bit CRC with a single 16 bit input
 *
 * \param   crc16In - Incoming CRC
 * \param   data    - Value to add to CRC
 *
 * \return  Updated CRC
 *
 * ========================================================================== */
uint16_t DoCRC16(uint16_t crc16In, uint16_t data)
{
    data = (data ^ (crc16In & BYTE_MASK)) & BYTE_MASK;            ///  \todo  05/05/20 DAZ - This needs to be documented. Reference? Polynomial used?
    crc16In >>= 8;

    if (oddparity[data & LOW_NIBBLE_MASK] ^ oddparity[data >> 4])
    {
        crc16In ^= CRC16_POLY;
    }

    data <<= 6;
    crc16In ^= data;
    data <<= 1;
    crc16In ^= data;

    return crc16In;
}
/* ========================================================================== */
/**
 * \brief   Slow (shift & xor) CRC16 calculation
 *
 * \details This function uses the CCITT CRC16 polynomial, and calculates the CRC
 *          by shifting and xoring a bit at a time, rather than using a table of
 *          pre-calculated CRCs.
 * \n \n
 *          If the buffer pointer is NULL, or the size is 0, then the
 *          incoming CRC is returned.
 *
 * \bug WHY IS THIS NOT TABLE DRIVEN?
 *
 * \param   sum     - Incoming CRC
 * \param   pBuf    - Pointer to buffer to add to CRC
 * \param   len     - Number of bytes to add to CRC
 *
 * \return  Updated (if possible) CRC.
 *
 * ========================================================================== */
 uint16_t SlowCRC16(uint16_t sum, uint8_t *pBuf, uint16_t len)
{
    int32_t     i;          // Byte loop counter
    uint8_t     byte;       // Temporary byte to process
    uint32_t    osum;       // Temporary CRC accumulator

    do
    {
        if ((NULL == pBuf) || (0 == len))
        {
            break;
        }

        while (len)                             // Number of bytes in the array to calculate the 16 bit checksum for
        {
            // Add a byte to the checksum

            byte = *pBuf;                       // Point to the next byte start of the byte array

            for (i = 0; i < BITS_PER_BYTE; ++i) // Shift bits through check sum polynomial
            {
                osum = sum;                     // Save the initial (previous) crc value

                // Shift MS bit of data byte into incoming CRC

                sum <<= SHIFT_LEFT_BY_ONE;

                if (byte & MSBIT_BYTE)          // Is the MS bit of this byte set?
                {
                    sum |= SET_LSBIT;           // Yes - set LS bit (else is 0 from the shift above)
                }

                // We use osum to see if the MSB of the sum shift would have shifted into the carry...

                if (osum & CRC16_MSB)           // Is the MS bit of the 16 bit word set?
                {
                    sum ^= CRC16_CCITT_POLY;    // Yes - xor with polynomial
                }
                byte <<= SHIFT_LEFT_BY_ONE;     // Shift to next bit in byte
            }

            pBuf++;         // Increment buffer pointer to next data byte
            len--;          // Decrement byte count

        }
    } while (false);

    return sum;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

