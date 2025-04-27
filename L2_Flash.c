#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L2_Flash
 * \{
 * \brief    K20 Flash control routines
 *
 * \details  This file contians K20 Flash functionality implementation.
 *
 * \sa      K20 SubFamily Reference Manual
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L2_Flash.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L2_Flash.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
/*------------- Number of commands in Command Sequence Array  ----------*/
#define FLASH_CMDARRAY_SIZE       (12u)              ///< Command Array Size
#define FLASH_NCMDS_ERASESECTOR   (3u)               ///< Num Commands - Erase Sector
#define FLASH_NCMDS_PROGRAM       (11u)              ///< Num Commands - Program

/* Flash Internal Memory Offset */
#define FLASH_INTERNAL_MEMORY_OFFSET     (0x800000u)

/* Time out for setting and Resetting CCIF Flag */
#define FLASH_CONFIG_DEFAULT_TIMEOUT     (0xFFu)

/* Default Values for Flash Config */
#define FLASH_REG_BASE        (0x40020000u)          ///< Flash control register base
#define FLASH_PFLASH_BASE     (0x00u)                ///< Base address of PFlash block
#define FLASH_PFLASH_SIZE     (0x80000u)             ///< Size of PFlash block
#define FLASH_DFLASH_BASE     (0x10000000u)          ///< Base address of DFlash block
#define FLASH_DFLASH_SIZE     (0x80000u)             ///< Size of DFlash block
#define FLASH_EEPROM_BASE     (0x00u)                ///< Base address of EERAM block
#define FLASH_EEPROM_SIZE     (0x00u)                ///< Size of EERAM block
#define FLASH_EEPROM_EEESPLIT (0x03u)                ///< EEPROM EEESPLIT Code
#define FLASH_EEPROM_EEESIZE  (0xffffu)              ///< EEPROM EEESIZE Code

/* Flash Status Register (FSTAT)*/
#define FLASH_SSD_FSTAT_OFFSET       (0x00000000u)
#define FLASH_SSD_FSTAT_CCIF         (0x80u)         ///< Command complete interrupt flag.
#define FLASH_SSD_FSTAT_RDCOLERR     (0x40u)         ///< Read collision error flag.
#define FLASH_SSD_FSTAT_ACCERR       (0x20u)         ///< Flash access error flag.
#define FLASH_SSD_FSTAT_FPVIOL       (0x10u)         ///< Flash protection voilation flag.
#define FLASH_SSD_FSTAT_MGSTAT0      (0x01u)         ///< Memory controller command completion status flag.

/* The Flash common command object (FCCOB) regiser group provides 12 bytes for command codes */
/* and parameters. Below are the list of Flash common command object registers (FCCOB0-B) */
#define FLASH_SSD_FCCOB0_OFFSET      (0x00000007u)
#define FLASH_SSD_FCCOB1_OFFSET      (0x00000006u)
#define FLASH_SSD_FCCOB2_OFFSET      (0x00000005u)
#define FLASH_SSD_FCCOB3_OFFSET      (0x00000004u)
#define FLASH_SSD_FCCOB4_OFFSET      (0x0000000Bu)
#define FLASH_SSD_FCCOB5_OFFSET      (0x0000000Au)
#define FLASH_SSD_FCCOB6_OFFSET      (0x00000009u)
#define FLASH_SSD_FCCOB7_OFFSET      (0x00000008u)
#define FLASH_SSD_FCCOB8_OFFSET      (0x0000000Fu)
#define FLASH_SSD_FCCOB9_OFFSET      (0x0000000Eu)
#define FLASH_SSD_FCCOBA_OFFSET      (0x0000000Du)
#define FLASH_SSD_FCCOBB_OFFSET      (0x0000000Cu)

#define SHIFT_BITS_16               (16u)   /* Shift width size 16 bits */
#define SHIFT_BITS_8                (8u)    /* Shift width size 8  bits */

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static FLASH_DATA FlashData;

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static FLASH_STATUS L2_FlashWriteCommandSequence(uint8_t NumCommands, uint8_t *pCommands);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   This Method initializes the Flash in accordance with either default
 *          Partition definitions or in accordance with passed parameter 
 *          definitions.
 *
 * \param   *pFlashConfig - Pointer to FLASH_CONFIG struct to pass optional
 *                         Configuration values
 *
 * \return  None
 *
 * ========================================================================== */
void L2_FlashInit(FLASH_CONFIG *pFlashConfig)
{
    FLASH_CONFIG *pConfig;
    uint32_t FlashRegBase;
    int16_t CcifTimeout;

    pConfig = &FlashData.Config;
    FlashData.Status.Initialized = false;

    // Validate the pointer and also the base addresses, sizes
    if ((NULL != pFlashConfig) 
      && (FLASH_REG_BASE == pFlashConfig->FlashRegBase)
      && (FLASH_DFLASH_BASE == pFlashConfig->DFlashBase) 
      && (FLASH_PFLASH_SIZE >= pFlashConfig->PFlashSize) 
      && (FLASH_DFLASH_SIZE >= pFlashConfig->DFlashSize))
    {                          // Yes, Copy config into database
        memcpy((char*)pConfig, (char*)pFlashConfig, sizeof(FLASH_CONFIG));
    }
    else
    {                          // No, Load Defaults
        pConfig->FlashRegBase = FLASH_REG_BASE;          // Flash control register base
        pConfig->PFlashBase = FLASH_PFLASH_BASE;         // Base address of PFlash block
        pConfig->PFlashSize = FLASH_PFLASH_SIZE;         // Size of PFlash block
        pConfig->DFlashBase = FLASH_DFLASH_BASE;         // Base address of DFlash block
        pConfig->DFlashSize = FLASH_DFLASH_SIZE;         // Size of DFlash block
        pConfig->EepromBase = FLASH_EEPROM_BASE;         // Base address of EERAM block
        pConfig->EepromSize = FLASH_EEPROM_SIZE;         // Size of EERAM block
        pConfig->EepromEeeSplit = FLASH_EEPROM_EEESPLIT; // EEPROM EEESPLIT Code
        pConfig->EepromEeeSize = FLASH_EEPROM_EEESIZE;   // EEPROM EEESIZE Code
        pConfig->CcifTimeout = FLASH_CONFIG_DEFAULT_TIMEOUT; // Config Default Timeout
    }

    FlashRegBase = pConfig->FlashRegBase;

    CcifTimeout = pConfig->CcifTimeout;

    /* check CCIF bit of the flash status register */
    while ((!(FLASH_REG_BIT_TEST((FlashRegBase + FLASH_SSD_FSTAT_OFFSET), FLASH_SSD_FSTAT_CCIF))) && 
        (0 < CcifTimeout))
    {
        CcifTimeout -= 1;  /* wait till CCIF bit is set */
    }

    /* clear RDCOLERR & ACCERR & FPVIOL flag in flash status register */
    FLASH_REG_WRITE((FlashRegBase + FLASH_SSD_FSTAT_OFFSET),
                      (FLASH_SSD_FSTAT_RDCOLERR | FLASH_SSD_FSTAT_ACCERR | FLASH_SSD_FSTAT_FPVIOL));

    /* Write Command Code to FCCOB0 */
    FLASH_REG_WRITE((FlashRegBase + FLASH_SSD_FCCOB0_OFFSET), FLASH_READ_RESOURCE);
    /* Write address to FCCOB1/2/3 */
    FLASH_REG_WRITE((FlashRegBase + FLASH_SSD_FCCOB1_OFFSET), ((uint8_t)(FLASH_DFLASH_IFR_READRESOURCE_ADDRESS >> SHIFT_BITS_16)));
    FLASH_REG_WRITE((FlashRegBase + FLASH_SSD_FCCOB2_OFFSET), ((uint8_t)((FLASH_DFLASH_IFR_READRESOURCE_ADDRESS >> SHIFT_BITS_8) & 0xFF)));
    FLASH_REG_WRITE((FlashRegBase + FLASH_SSD_FCCOB3_OFFSET), ((uint8_t)(FLASH_DFLASH_IFR_READRESOURCE_ADDRESS & 0xFF)));

    /* Write Resource Select Code of 0 to FCCOB8 to select IFR. Without this, */
    /* an access error may occur if the register contains data from a previous command. */
    FLASH_REG_WRITE((FlashRegBase + FLASH_SSD_FCCOB4_OFFSET), 0);
    /* clear CCIF bit */
    FLASH_REG_WRITE((FlashRegBase + FLASH_SSD_FSTAT_OFFSET), FLASH_SSD_FSTAT_CCIF);

    /* Load actual Config timeout */
    CcifTimeout = pConfig->CcifTimeout;

    /* check CCIF bit */
    while ((!(FLASH_REG_BIT_TEST((FlashRegBase + FLASH_SSD_FSTAT_OFFSET), FLASH_SSD_FSTAT_CCIF))) && 
        (0 < CcifTimeout))
    {
        CcifTimeout -= 1; /* wait till CCIF bit is set */
    }

    FlashData.Status.Initialized = true;
    FlashData.Status.ErrorCode = FLASH_STATUS_OK;
}

/* ========================================================================== */
/**
 * \brief   This Method Erases a Sector in the Partition specified by the passed
 *          parameters of Destination (address) and Nbytes (size).
 *
 * \param   Destination - Start Address of sector to Erase
 * \param   Nbytes      - Number of bytes to Erase
 *
 * \return  FLASH_STATUS
 * \retval      FLASH_STATUS_OK - Erase Successful
 * \retval      FLASH_STATUS_ERR_RANGE - Parameter Range Error
 * \retval      FLASH_STATUS_ERR_ADDR - Parameter Address Error
 * \retval      FLASH_STATUS_ERR_SIZE - Sector Alignment Error
 * \retval      FLASH_STATUS_ERR_ACCERR - Access Error
 * \retval      FLASH_STATUS_ERR_PVIOL - Protection Error
 * \retval      FLASH_STATUS_ERR_MGSTAT0 - Non-Correctable Error
 * \retval      FLASH_STATUS_ERR_PARAM - Passed Parameter Error
 *
 * ========================================================================== */
FLASH_STATUS L2_FlashEraseSector(uint32_t Destination, uint32_t Nbytes)
{
    FLASH_CONFIG  *pConfig;
    uint8_t  Command[FLASH_CMDARRAY_SIZE];  /* command sequence array */
    FLASH_STATUS  Ret;                      /* return code variable */
    uint32_t  EndAddress;                   /* storing end address */
    uint32_t  SectorSize;                   /* size of one sector */

    pConfig = &FlashData.Config;

    /* set the default return code as FTFx_OK */
    Ret = FLASH_STATUS_OK;

    /* calculating Flash end address */
    EndAddress = Destination + Nbytes;

    do
    {
        /* check for valid range of the target addresses */
        if ((Destination < pConfig->PFlashBase) ||
            (EndAddress > (pConfig->PFlashBase + pConfig->PFlashSize)))
        {
            if ((Destination < pConfig->DFlashBase) ||
                (EndAddress > (pConfig->DFlashBase + pConfig->DFlashSize)))
            {
                /* return an error code FTFx_ERR_RANGE */
                Ret = FLASH_STATUS_ERR_RANGE;
                break;
            }
            else
            {
                /* Convert System memory address to FTFx internal memory address */
                Destination = (Destination - pConfig->DFlashBase) + FLASH_INTERNAL_MEMORY_OFFSET;
                SectorSize = FLASH_DSECTOR_SIZE;
            }
        }
        else
        {
            /* Convert System memory address to FTFx internal memory address */
            Destination -= pConfig->PFlashBase;
            SectorSize = FLASH_PSECTOR_SIZE;
        }

        /* check if the destination is sector aligned or not */
        if ( NULL != (Destination % SectorSize) )
        {
            Ret = FLASH_STATUS_ERR_ADDR;
            break;
        }   /* else not needed */

        /* check if the size is sector alignment or not */
        if (NULL != (Nbytes % SectorSize) )
        {
            Ret = FLASH_STATUS_ERR_SIZE;
            break;
        }   /* else not needed */

        while ( NULL < Nbytes )
        {
            /* preparing passing parameter to erase a flash block */
            Command[0] = FLASH_ERASE_SECTOR;
            Command[1] = (uint8_t)(Destination >> SHIFT_BITS_16);
            Command[2] = (uint8_t)((Destination >> SHIFT_BITS_8) & 0xFF);
            Command[3] = (uint8_t)(Destination & 0xFF);

            /* calling flash command sequence function to execute the command */
            Ret = L2_FlashWriteCommandSequence(FLASH_NCMDS_ERASESECTOR, Command);

            /* checking the success of command execution */
            if (FLASH_STATUS_OK != Ret)
            {
                break;
            }
            else
            {
                /* update size and destination address */
                Nbytes -= SectorSize;
                Destination += SectorSize;
            }
        }
    } while (false);

    FlashData.Status.ErrorCode = Ret;
    return (Ret);
}

/* ========================================================================== */
/**
 * \brief   This Method returns the current Status of the Flash Object in the
 *          contents of the passed pointer parameter.
 *
 * \param   *pFlashCurrentStatus - Pointer to FLASH_STATUS.
 *
 * \return  None
 *
 * ========================================================================== */
void L2_FlashGetStatus(FLASH_CURRENT_STATUS *pFlashCurrentStatus)
{
    // Check validity of passed parameter
    if (NULL != pFlashCurrentStatus)
    {
        memcpy((char*)pFlashCurrentStatus, (char*)&FlashData.Status, sizeof(FLASH_STATUS));
    }
}

/* ========================================================================== */
/**
 * \brief   This Method Writes Data to the specified Memory Partition specified
 *          by the passed parameters of Destination (address) and Nbytes (size)
 *          from the address specified by the passed parameter Source.
 *
 * \param   Destination - Start Address of sector to Write
 * \param   Nbytes      - Number of bytes to Write
 * \param   Source      - Start Address of Data Source
 *
 * \return  FLASH_STATUS
 * \retval      FLASH_STATUS_OK - Write Successful
 * \retval      FLASH_STATUS_ERR_RANGE - Parameter Range Error
 * \retval      FLASH_STATUS_ERR_ADDR - Parameter Address Error
 * \retval      FLASH_STATUS_ERR_SIZE - Size Alignment Error
 * \retval      FLASH_STATUS_ERR_ACCERR - Access Error
 * \retval      FLASH_STATUS_ERR_PVIOL - Protection Error
 * \retval      FLASH_STATUS_ERR_MGSTAT0 - Non-Correctable Error
 * \retval      FLASH_STATUS_ERR_PARAM - Passed Parameter Error
 *
 * ========================================================================== */
FLASH_STATUS L2_FlashWrite(uint32_t Destination, uint32_t Nbytes, uint32_t Source)
{
    FLASH_CONFIG  *pConfig;
    uint8_t           Command[FLASH_CMDARRAY_SIZE]; // command sequence array
    FLASH_STATUS          Ret;                      // return code variable
    uint32_t          EndAddress;                   // storing end address

    pConfig = &FlashData.Config;

    /* set the default return code as FTFE_OK */
    Ret = FLASH_STATUS_OK;

    /* calculating Flash end address */
    EndAddress = Destination + Nbytes;

    do
    {
        /* check if the destination is Longword aligned or not */
        if (NULL != (Destination % FLASH_PHRASE_SIZE))
        {
            Ret = FLASH_STATUS_ERR_ADDR;
            break;
        }   /* else not needed */

        /* check if the size is Longword alignment or not */
        if (NULL != (Nbytes % FLASH_PHRASE_SIZE) )
        {
            Ret = FLASH_STATUS_ERR_SIZE;
            break;
        }   /* else not needed */

        /* check for valid range of the target addresses */
        if ((Destination < pConfig->PFlashBase) ||
            (EndAddress > (pConfig->PFlashBase + pConfig->PFlashSize)))
        {
            if ((Destination < pConfig->DFlashBase) ||
                (EndAddress > (pConfig->DFlashBase + pConfig->DFlashSize)))
            {
                Ret = FLASH_STATUS_ERR_RANGE;
                break;
            }
            else
            {
                /* Convert System memory address to FTFx internal memory address */
                Destination = Destination - pConfig->DFlashBase + FLASH_INTERNAL_MEMORY_OFFSET;
            }
        }
        else
        {
            /* Convert System memory address to FTFx internal memory address */
            Destination -= pConfig->PFlashBase;
        }

        while (0 < Nbytes)
        {
            /* preparing passing parameter to program the flash block */
            Command[0] = FLASH_PROGRAM_PHRASE;
            Command[1] = (uint8_t)(Destination >> SHIFT_BITS_16);
            Command[2] = (uint8_t)((Destination >> SHIFT_BITS_8) & 0xFF);
            Command[3] = (uint8_t)(Destination & 0xFF);

            /* Reading the data in bytes from the source */
            Command[4] = FLASH_READ8(Source + 3);
            Command[5] = FLASH_READ8(Source + 2);
            Command[6] = FLASH_READ8(Source + 1);
            Command[7] = FLASH_READ8(Source);
            Command[8] = FLASH_READ8(Source + 7);
            Command[9] = FLASH_READ8(Source + 6);
            Command[10] = FLASH_READ8(Source + 5);
            Command[11] = FLASH_READ8(Source + 4);

            /* calling flash command sequence function to execute the command */
            Ret = L2_FlashWriteCommandSequence(FLASH_NCMDS_PROGRAM, Command);

            /* checking for the success of command execution */
            if (FLASH_STATUS_OK != Ret)
            {
                break;
            }
            else
            {
                /* update destination address for next iteration */
                Destination += FLASH_PHRASE_SIZE;
                /* update size for next iteration */
                Nbytes -= FLASH_PHRASE_SIZE;
                /* increment the source address by 1 */
                Source += FLASH_PHRASE_SIZE;
            }
        }
      } while (false);

    FlashData.Status.ErrorCode = Ret;
    return (Ret);
}

/* ========================================================================== */
/**
 * \brief   This Subroutine Writes a command sequence to the Flash Hardware
 *
 * \param   NumCommands - Number of commands to transmit
 * \param   *pCommands - Pointer to an array of byte command values
 *
 * \return  FLASH_STATUS
 * \retval      FLASH_STATUS_ERR_ACCERR - Access Error
 * \retval      FLASH_STATUS_ERR_PVIOL - Protection Error
 * \retval      FLASH_STATUS_ERR_MGSTAT0 - Non-Correctable Error
 * \retval      FLASH_STATUS_ERR_PARAM - Passed Parameter Error
 *
 * ========================================================================== */
static FLASH_STATUS L2_FlashWriteCommandSequence(uint8_t NumCommands,uint8_t *pCommands)
{
    uint32_t  FlashRegBase;
    uint8_t   Counter;          // for loop counter variable
    uint8_t   RegisterValue;    // store data read from flash register
    FLASH_STATUS  Ret;          // return code variable
    OS_CPU_SR cpu_sr;           // Required for OS_ENTER/EXIT_CRITICAL

    static const uint32_t FlashCommandOffset[FLASH_CMDARRAY_SIZE] =
    {
        FLASH_SSD_FCCOB0_OFFSET,
        FLASH_SSD_FCCOB1_OFFSET,
        FLASH_SSD_FCCOB2_OFFSET,
        FLASH_SSD_FCCOB3_OFFSET,
        FLASH_SSD_FCCOB4_OFFSET,
        FLASH_SSD_FCCOB5_OFFSET,
        FLASH_SSD_FCCOB6_OFFSET,
        FLASH_SSD_FCCOB7_OFFSET,
        FLASH_SSD_FCCOB8_OFFSET,
        FLASH_SSD_FCCOB9_OFFSET,
        FLASH_SSD_FCCOBA_OFFSET,
        FLASH_SSD_FCCOBB_OFFSET,
    };

    FlashRegBase = FlashData.Config.FlashRegBase;

    /* set the default return as FTFE_OK */
    Ret = FLASH_STATUS_OK;

    // Check validity of passed parameters
    if ((NULL == pCommands) || (FLASH_CMDARRAY_SIZE < NumCommands))
    {
        Ret = FLASH_STATUS_ERR_PARAM;
    }
    else
    {
        /* check CCIF bit of the flash status register */
        OS_ENTER_CRITICAL();
        while ((!(FLASH_REG_BIT_TEST(FlashRegBase + FLASH_SSD_FSTAT_OFFSET, FLASH_SSD_FSTAT_CCIF))))
        {
            /* wait till CCIF bit is set */
        }

        /* clear RDCOLERR & ACCERR & FPVIOL flag in flash status register */
        FLASH_REG_WRITE((FlashRegBase + FLASH_SSD_FSTAT_OFFSET),
              (FLASH_SSD_FSTAT_RDCOLERR | FLASH_SSD_FSTAT_ACCERR | FLASH_SSD_FSTAT_FPVIOL));

        /* load FCCOB registers */
        for (Counter=0; Counter <= NumCommands; Counter++)
        {
            FLASH_REG_WRITE((FlashRegBase + FlashCommandOffset[Counter]), pCommands[Counter]);
        }

        /* clear CCIF bit */
        FLASH_REG_WRITE((FlashRegBase + FLASH_SSD_FSTAT_OFFSET), FLASH_SSD_FSTAT_CCIF);

        /* check CCIF bit */
        while ((!(FLASH_REG_BIT_TEST(FlashRegBase + FLASH_SSD_FSTAT_OFFSET, FLASH_SSD_FSTAT_CCIF))))
        {
             /* wait till CCIF bit is set */
        }

        /* Check error bits */
        /* Get flash status register value */
        RegisterValue = FLASH_REG_READ(FlashRegBase + FLASH_SSD_FSTAT_OFFSET);

        OS_EXIT_CRITICAL();

        /* checking access error */
        if (NULL != (RegisterValue & FLASH_SSD_FSTAT_ACCERR) )
        {
            Ret = FLASH_STATUS_ERR_ACCERR;
        }
        /* checking protection error */
        else if (NULL != (RegisterValue & FLASH_SSD_FSTAT_FPVIOL) )
        {
            Ret = FLASH_STATUS_ERR_PVIOL;
        }
        /* checking MGSTAT0 non-correctable error */
        else if (NULL != (RegisterValue & FLASH_SSD_FSTAT_MGSTAT0) )
        {
            Ret = FLASH_STATUS_ERR_MGSTAT0;
        }
        else
        {
            //do nothing.
        }
    }
    return (Ret);
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

