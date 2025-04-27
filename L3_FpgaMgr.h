#ifndef L3_FPGAMGR_H
#define L3_FPGAMGR_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_FPGA
 * \{
 * \brief   Public interface for the FPGA configuration.
 *
 * \details This file contains the enumerations as well as the function prototypes
 *          for the FPGA configuration module
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_FpgaMgr.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"         ///< Import common definitions such as types, etc.

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define SIGNIA_FPGA_FEATURE_BITS (0x0200u) /// FPGA feature bits for Signia - SPI persistence disable (Makes SPI useable by FPGA code)

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/*  The feature Row update is not part of this list [Not part of regular
    programming], as any programming mistake could permanently disable the
    FPGA from future Programming. Refer to “L3_FpgaMgrUpdateFeatureBits()”
    API for feature Row Update. */
typedef enum                        /// Update Fpga - Memory Area Map [Reference: TN1204]
{
  FPGA_SRAM             = (0x01u),  ///< Update SRAM only
  FPGA_CONFIG           = (0x04u),  ///< Update Config only
  FPGA_UFM              = (0x08u),  ///< Update UFM only
  FPGA_SRAM_UFM         = (0x09u),  ///< Update SRAM and UFM
  FPGA_SRAM_CONFIG      = (0x05u),  ///< Update SRAM and Config
  FPGA_UFM_CONFIG       = (0x0Cu),  ///< Update UFM and Config
  FPGA_SRAM_UFM_CONFIG  = (0x0Du)   ///< Update SRAM, UFM and Config
} FPGA_MEM_AREA;

typedef enum                        /// Update Fpga - Memory Area Map [Reference: TN1204]
{
  FPGA_MGR_OK,                      ///< No Error
  FPGA_MGR_INVALID_PARAM,           ///< Invalid Parameter passed.
  FPGA_MGR_REFRESH_FAILED,          ///< Fpga Refresh Failed
  FPGA_MGR_PROGRAM_FAILED,          ///< Fpga Programming Failed
  FPGA_MGR_ERROR                    ///< Fpga Generic Error.
} FPGA_MGR_STATUS;

typedef struct                                          ///Mach02 Structure (FPGA)
{
    uint32_t FileTimestamp;                             ///< JED time stamp
    uint32_t TotalDataSize;                             ///< Total size of JED data 
    uint32_t FuseDataSize;                              ///< Fuse Data size
    uint32_t UfmDataSize;                               ///< UFM datasize
    uint32_t FeatureDataSize;                           ///< Data Size
    uint32_t FeaBitsDataSize;                           ///< Data Size
} MACHX02;

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern FPGA_MGR_STATUS L3_FpgaMgrInit(void);
extern FPGA_MGR_STATUS L3_FpgaMgrUpdateFPGA(MACHX02 *pJedInfo, FPGA_MEM_AREA eMemArea);
extern FPGA_MGR_STATUS L3_FpgaMgrUpdateFeatureBits(uint16_t u16FeatureBits);
extern FPGA_MGR_STATUS L3_FpgaMgrCheckStatus(void);
extern FPGA_MGR_STATUS L3_FpgaMgrSleepEnable(bool bEnable);

extern bool L3_FpgaMgrRefresh(void);
extern bool L3_FpgaMgrReset(void);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_FPGAMGR_H */

