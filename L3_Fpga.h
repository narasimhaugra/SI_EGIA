#ifndef L3_FPGA_H
#define L3_FPGA_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_FPGA
 * \{
 * \brief   Public interface for the FPGA module.
 *
 * \details This file contains the enumerations as well as the function prototypes
 *          for the FPGA module
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_Fpga.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef enum                    /*! FPGA Register Index list */
{
  /* The registers in this block are in the FPGA Register block starting at offset 0x00 */
  FPGA_REG_SW_VERSION,          /*! Reg. 00 - FPGA software version */
  FPGA_REG_HW_VERSION,          /*! Reg. 01 - Board version   (LS 4 bits. MS 4 bits 0) */
  FPGA_REG_PIEZO_PWM,           /*! Reg. 02 - Piezo frequency */
  FPGA_REG_CONTROL,             /*! Reg. 03 - FPGA control */

  /* The registers below are divided into 3 Motor Register blocks, starting at 0x10, 0x20, 0x30. */
  FPGA_REG_MOT0_CONTROL,        /*! Reg. 04 - Motor 0 control */
  FPGA_REG_MOT0_CURR_PWM,       /*! Reg. 05 - Motor 0 current limit PWM */
  FPGA_REG_MOT0_VEL_PWM,        /*! Reg. 06 - Motor 0 drive PWM */
  FPGA_REG_MOT0_STATUS,         /*! Reg. 07 - Motor 0 status */
  FPGA_REG_MOT0_POSITION,       /*! Reg. 08 - Motor 0 position */
  FPGA_REG_MOT0_PERIOD,         /*! Reg. 09 - Motor 0 period (19.5nS counts between position ticks - speed) */
  FPGA_REG_MOT0_DELTA_COUNT,    /*! Reg. 22 - Motor 0 delta count (Number of ticks moved since last read) */

  FPGA_REG_MOT1_CONTROL,        /*! Reg. 10 - Motor 1 control */
  FPGA_REG_MOT1_CURR_PWM,       /*! Reg. 11 - Motor 1 current limit PWM */
  FPGA_REG_MOT1_VEL_PWM,        /*! Reg. 12 - Motor 1 drive PWM */
  FPGA_REG_MOT1_STATUS,         /*! Reg. 13 - Motor 1 status */
  FPGA_REG_MOT1_POSITION,       /*! Reg. 14 - Motor 1 position */
  FPGA_REG_MOT1_PERIOD,         /*! Reg. 15 - Motor 1 period (19.5nS counts between position ticks - speed) */
  FPGA_REG_MOT1_DELTA_COUNT,    /*! Reg. 23 - Motor 1 delta count (Number of ticks moved since last read) */

  FPGA_REG_MOT2_CONTROL,        /*! Reg. 16 - Motor 2 control */
  FPGA_REG_MOT2_CURR_PWM,       /*! Reg. 17 - Motor 2 current limit PWM */
  FPGA_REG_MOT2_VEL_PWM,        /*! Reg. 18 - Motor 2 drive PWM */
  FPGA_REG_MOT2_STATUS,         /*! Reg. 19 - Motor 2 status */
  FPGA_REG_MOT2_POSITION,       /*! Reg. 20 - Motor 2 position */
  FPGA_REG_MOT2_PERIOD,         /*! Reg. 21 - Motor 2 period (19.5nS counts between position ticks - speed) */
  FPGA_REG_MOT2_DELTA_COUNT,    /*! Reg. 24 - Motor 2 delta count (Number of ticks moved since last read) */

  /* The 5 regs in this block are in the FPGA MISC section starting at offset 0x40 */
  // NOTE: THe 3 delta count registers (reg. 22 - 24) in this block are grouped with
  // the rest of their respective motor registers for consistency, even though the
  // physical registers reside in this group.
  FPGA_REG_OK,                  /*! Reg. 25 - FPGA OK (Set to 1 during init. 0 if FPGA reset) */
  FPGA_REG_BAD_CRC_COUNT,       /*! Reg. 26 - Bad CRC count (Number of bad CRCs received) */
  FPGA_REG_COUNT                /*! 27 - Number of registers in FPGA */
} FPGA_REG;
/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/

#define FPGA_SLAVE_ADDRESS          (0x40U)     ///< FPGA I2C Slave address
#define FPGA_SYNC_PERIOD            (   1u)     ///< FPGA Controller Task period
#define FPGA_PERIOD_TIME            (19.56E-9)  ///< Time per FPGA period count
#define FPGA_BUFFER_MAX             ((FPGA_REG_COUNT * 7) + 67)   ///< Maximum possible buffer size; possibly read all registers at once

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern bool L3_FpgaInit(void);
extern bool L3_FpgaSync(void);
extern bool L3_FpgaReadReg(FPGA_REG Reg, uint32_t *pRegVal);
extern bool L3_FpgaWriteReg(FPGA_REG Reg, uint32_t RegVal);
extern void L3_FpgaErrorLogClear(void);
extern void L3_FpgaErrorLogDump(void);
extern bool L3_FpgaIsRefreshPending(void);
extern void L3_FpgaRequestRefresh(bool Request);
extern void L3_FpgaReload(void);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L3_FPGA_H */

