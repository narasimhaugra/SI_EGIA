#ifndef L2_FLASH_H
#define L2_FLASH_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif
  
/* ========================================================================== */
/**
 * \addtogroup L2_Flash
 * \{
 * \brief   This Header File exposes public information to callers of L2 K20 
 *          flash routines
 *
 * \details 
 *
 * \copyright  Copyright 2020 Covidien - Surgical Innovations.  All Rights Reserved.
 * 
 * \file  L2_Flash.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
/* Word size */
#define FLASH_WORD_SIZE               (0x0002u)     /* 2 bytes     */

/* Longword size */
#define FLASH_LONGWORD_SIZE           (0x0004u)     /* 4 bytes     */

/* Phrase size */
#define FLASH_PHRASE_SIZE             (0x0008u)     /* 8 bytes     */

/* Double-phrase size */
#define FLASH_DPHRASE_SIZE            (0x0010u)     /* 16 bytes    */


/* PFlash sector size */
#define FLASH_PSECTOR_SIZE            (0x00001000u) /* 4 KB size   */
/* DFlash sector size */
#define FLASH_DSECTOR_SIZE            (0x00001000u) /* 4 KB size   */
#define FLASH_DEBLOCK_SIZE            (0x00080000u) /* 512 KB size */

/*destination to read Dflash IFR area*/
#define FLASH_DFLASH_IFR_READRESOURCE_ADDRESS       (0x8003F8u)

/*-------------- Read/Write/Set/Clear Operation Macros -----------------*/
#define FLASH_REG_BIT_SET(Address, Mask)   (*(vuint8_t*)(Address) |= (Mask))
#define FLASH_REG_BIT_CLEAR(Address, Mask) (*(vuint8_t*)(Address) &= ~(Mask))
#define FLASH_REG_BIT_TEST(Address, Mask)  (*(vuint8_t*)(Address) & (Mask))
#define FLASH_REG_WRITE(Address, Value)    (*(vuint8_t*)(Address) = (Value))
#define FLASH_REG_READ(Address)            ((uint8_t)(*(vuint8_t*)(Address)))
#define FLASH_REG_WRITE32(Address, Value)  (*(vuint32_t*)(Address) = (Value))
#define FLASH_REG_READ32(Address)          ((uint32_t)(*(vuint32_t*)(Address)))

#define FLASH_WRITE8(Address, Value)       (*(vuint8_t*)(Address) = (Value))
#define FLASH_READ8(Address)               ((uint8_t)(*(vuint8_t*)(Address)))
#define FLASH_SET8(Address, Value)         (*(vuint8_t*)(Address) |= (Value))
#define FLASH_CLEAR8(Address, Value)       (*(vuint8_t*)(Address) &= ~(Value))
#define FLASH_TEST8(Address, Value)        (*(vuint8_t*)(Address) & (Value))

#define FLASH_WRITE16(Address, Value)      (*(vuint16_t*)(Address) = (Value))
#define FLASH_READ16(Address)              ((uint16_t)(*(vuint16_t*)(Address)))
#define FLASH_SET16(Address, Value)        (*(vuint16_t*)(Address) |= (Value))
#define FLASH_CLEAR16(Address, Value)      (*(vuint16_t*)(Address) &= ~(Value))
#define FLASH_TEST16(Address, Value)       (*(vuint16_t*)(Address) & (Value))

#define FLASH_WRITE32(Address, Value)      (*(vuint32_t*)(Address) = (Value))
#define FLASH_READ32(Address)              ((uint32_t)(*(vuint32_t*)(Address)))
#define FLASH_SET32(Address, Value)        (*(vuint32_t*)(Address) |= (Value))
#define FLASH_CLEAR32(Address, Value)      (*(vuint32_t*)(Address) &= ~(Value))
#define FLASH_TEST32(Address, Value)       (*(vuint32_t*)(Address) & (Value))

/*------------- Flash hardware algorithm operation commands -------------*/
/* Refer to Flash commands section of K20 data sheet for more details  */
#define FLASH_VERIFY_BLOCK               (0x00u)
#define FLASH_VERIFY_SECTION             (0x01u)
#define FLASH_PROGRAM_CHECK              (0x02u)
#define FLASH_READ_RESOURCE              (0x03u)
#define FLASH_PROGRAM_LONGWORD           (0x06u)
#define FLASH_PROGRAM_PHRASE             (0x07u)
#define FLASH_ERASE_BLOCK                (0x08u)
#define FLASH_ERASE_SECTOR               (0x09u)
#define FLASH_PROGRAM_SECTION            (0x0Bu)
#define FLASH_VERIFY_ALL_BLOCK           (0x40u)
#define FLASH_READ_ONCE                  (0x41u)
#define FLASH_PROGRAM_ONCE               (0x43u)
#define FLASH_ERASE_ALL_BLOCK            (0x44u)
#define FLASH_SECURITY_BY_PASS           (0x45u)
#define FLASH_PFLASH_SWAP                (0x46u)
#define FLASH_PROGRAM_PARTITION          (0x80u)
#define FLASH_SET_EERAM                  (0x81u)

/* EEE Data Set Size Field Description */
#define FLASH_EEESIZE_0000       (0x00004000u)      /* 16386 bytes */
#define FLASH_EEESIZE_0001       (0x00002000u)      /* 8192 bytes */
#define FLASH_EEESIZE_0010       (0x00001000u)      /* 4096 bytes */
#define FLASH_EEESIZE_0011       (0x00000800u)      /* 2048 bytes */
#define FLASH_EEESIZE_0100       (0x00000400u)      /* 1024 bytes */
#define FLASH_EEESIZE_0101       (0x00000200u)      /* 512 bytes */
#define FLASH_EEESIZE_0110       (0x00000100u)      /* 256 bytes */
#define FLASH_EEESIZE_0111       (0x00000080u)      /* 128 bytes */
#define FLASH_EEESIZE_1000       (0x00000040u)      /* 64 bytes */
#define FLASH_EEESIZE_1001       (0x00000020u)      /* 32 bytes */
#define FLASH_EEESIZE_1010       (0x00000000u)      /* Reserved */
#define FLASH_EEESIZE_1011       (0x00000000u)      /* Reserved */
#define FLASH_EEESIZE_1100       (0x00000000u)      /* Reserved */
#define FLASH_EEESIZE_1101       (0x00000000u)      /* Reserved */
#define FLASH_EEESIZE_1110       (0x00000000u)      /* Reserved */
#define FLASH_EEESIZE_1111       (0x00000000u)      /* 0 byte */

/* D/E-Flash Partition Code Field Description  Refer to Flash Memory module section of K20 data sheet for more details */
#define FLASH_DEPART_0000        (0x00080000u)      /* 512 KB */
#define FLASH_DEPART_0001        (0x00080000u)      /* Reserved */
#define FLASH_DEPART_0010        (0x00080000u)      /* Reserved */
#define FLASH_DEPART_0011        (0x00080000u)      /* Reserved */
#define FLASH_DEPART_0100        (0x00070000u)      /* 448 KB */
#define FLASH_DEPART_0101        (0x00060000u)      /* 384 KB */
#define FLASH_DEPART_0110        (0x00040000u)      /* 256 KB */
#define FLASH_DEPART_0111        (0x00000000u)      /* 0 KB */
#define FLASH_DEPART_1000        (0x00000000u)      /* 0 KB */
#define FLASH_DEPART_1001        (0x00080000u)      /* Reserved */
#define FLASH_DEPART_1010        (0x00080000u)      /* Reserved */
#define FLASH_DEPART_1011        (0x00080000u)      /* Reserved */
#define FLASH_DEPART_1100        (0x00010000u)      /* 64 KB */
#define FLASH_DEPART_1101        (0x00020000u)      /* 128 KB */
#define FLASH_DEPART_1110        (0x00040000u)      /* 256 KB */
#define FLASH_DEPART_1111        (0x00080000u)      /* 512 KB */

/* Address offset (compare to start addr of P/D flash) and size of PFlash IFR and DFlash IFR */
/* Refer to Flash Memory module section of K20 data sheet for more details */
#define FLASH_PFLASH_IFR_OFFSET       (0x00000000u)
#define FLASH_PFLASH_IFR_SIZE         (0x00000400u)
#define FLASH_DFLASH_IFR_OFFSET       (0x00000000u)
#define FLASH_DFLASH_IFR_SIZE         (0x00000400u)

/* Size for checking alignment of a section */
#define FLASH_ERSBLK_ALIGN_SIZE   FLASH_DPHRASE_SIZE  /* check align of erase block function */
#define FLASH_PGMCHK_ALIGN_SIZE   FLASH_LONGWORD_SIZE /* check align of program check function */
#define FLASH_PPGMSEC_ALIGN_SIZE  FLASH_DPHRASE_SIZE  /* check align of program section function */
#define FLASH_DPGMSEC_ALIGN_SIZE  FLASH_DPHRASE_SIZE  /* check align of program section function */
#define FLASH_RD1BLK_ALIGN_SIZE   FLASH_DPHRASE_SIZE  /* check align of verify block function */
#define FLASH_PRD1SEC_ALIGN_SIZE  FLASH_DPHRASE_SIZE  /* check align of verify section function */
#define FLASH_DRD1SEC_ALIGN_SIZE  FLASH_DPHRASE_SIZE  /* check align of verify section function */
#define FLASH_SWAP_ALIGN_SIZE     FLASH_DPHRASE_SIZE  /* check align of swap function*/
#define FLASH_RDRSRC_ALIGN_SIZE   FLASH_PHRASE_SIZE   /* check align of read resource function */
#define FLASH_RDONCE_INDEX_MAX    (0x7u)              /* maximum index in read once command */

/* PFlash swap states  Refer to Flash Memory module section of K20 data sheet for more details */
#define FLASH_SWAP_UNINIT            (0x00u)
#define FLASH_SWAP_READY             (0x01u)
#define FLASH_SWAP_INIT              (0x01u)
#define FLASH_SWAP_UPDATE            (0x02u)
#define FLASH_SWAP_UPDATE_ERASED     (0x03u)
#define FLASH_SWAP_COMPLETE          (0x04u)

/* Flash configuration register (FCNFG)*/
#define FLASH_SSD_FCNFG_OFFSET           (0x00000001u)
#define FLASH_SSD_FCNFG_CCIE             (0x80u)
#define FLASH_SSD_FCNFG_RDCOLLIE         (0x40u)
#define FLASH_SSD_FCNFG_ERSAREQ          (0x20u)
#define FLASH_FCNFG_ERSSUSP              (0x10u)
#define FLASH_FCNFG_RAMRDY               (0x02u)
#define FLASH_FCNFG_EEERDY               (0x01u)

/* Flash security register (FSEC) */
#define FLASH_SSD_FSEC_OFFSET            (0x00000002u)
#define FLASH_SSD_FSEC_KEYEN             (0xC0u)
#define FLASH_SSD_FSEC_FSLACC            (0x0Cu)
#define FLASH_SSD_FSEC_SEC               (0x03u)

/* Flash Option Register (FOPT) */
#define FLASH_SSD_FOPT_OFFSET            (0x00000003u)

/* P-Flash protection registers (FPROT0-3)  Refer to Flash Memory module section of K20 data sheet for more details */
#define FLASH_SSD_FPROT0_OFFSET    (0x00000013u)
#define FLASH_SSD_FPROT1_OFFSET    (0x00000012u)
#define FLASH_SSD_FPROT2_OFFSET    (0x00000011u)
#define FLASH_SSD_FPROT3_OFFSET    (0x00000010u)

/* D-Flash protection registers (FDPROT) */
#define FLASH_SSD_FDPROT_OFFSET    (0x00000017u)

/* EERAM Protection Register (FEPROT)  */
#define FLASH_SSD_FEPROT_OFFSET    (0x00000016u)

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum
{
    FLASH_STATUS_OK,                ///< Flash operation(Write/Erase) successful.
    FLASH_STATUS_ERR_SIZE,          ///< Flash write size longword alignement error.
    FLASH_STATUS_ERR_RANGE,         ///< Flash memory out of range error.
    FLASH_STATUS_ERR_ACCERR,        ///< Flash access error.
    FLASH_STATUS_ERR_PVIOL,         ///< Flash Protection voialtion error.
    FLASH_STATUS_ERR_MGSTAT0,       ///< Flash error detected during execution of command or during flash reset.
    FLASH_STATUS_ERR_ADDR,          ///< Flash destination address longword alignement error.
    FLASH_STATUS_ERR_PARAM          ///< Invalid Params.
} FLASH_STATUS;

/*---------------- Flash Configuration Structure -------------------*/
typedef struct 
{
    uint32_t  FlashRegBase;   ///< Flash control register base
    uint32_t  PFlashBase;     ///< Base address of PFlash block
    uint32_t  PFlashSize;     ///< Size of PFlash block
    uint32_t  DFlashBase;     ///< Base address of DFlash block
    uint32_t  DFlashSize;     ///< Size of DFlash block
    uint32_t  EepromBase;     ///< Base address of EERAM block
    uint32_t  EepromSize;     ///< Size of EERAM block
    uint32_t  EepromEeeSplit; ///< EEPROM EEESPLIT Code
    uint32_t  EepromEeeSize;  ///< EEPROM EEESIZE Code
    int16_t   CcifTimeout;    ///< CCIF  timeout
} FLASH_CONFIG;

typedef struct 
{
    bool          Initialized;            ///< Flash Module Initialized
    FLASH_STATUS  ErrorCode;              ///< Error Code
} FLASH_CURRENT_STATUS;

typedef struct 
{
    FLASH_CONFIG  Config;           ///< Flash Config Variables
    FLASH_CURRENT_STATUS  Status;   ///< Flash Current Status
} FLASH_DATA;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern void L2_FlashInit(FLASH_CONFIG *pFlashConfig);
extern void L2_FlashGetStatus(FLASH_CURRENT_STATUS *pFlashCurrentStatus);
extern FLASH_STATUS L2_FlashWrite(uint32_t Destination, uint32_t Nbytes, uint32_t Source);
extern FLASH_STATUS L2_FlashEraseSector(uint32_t Destination, uint32_t Nbytes);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L2_FLASH_H */

