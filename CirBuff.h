#ifndef CIR_BUFF_H
#define CIR_BUFF_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif
 
/* ========================================================================== */
/**
 * \addtogroup CirBuff
 * \{
 * \brief   Public interface for the CircularBuffer routines.
 *
 * \details This header file provides the public interface for the CircularBuffer 
 *          module, including type definitions and function prototypes.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    CirBuff.h
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
 
/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
typedef struct
{
    bool       bIsEmpty;    ///< Empty flag
    uint16_t   u16HeadPos;  ///< Head position
    uint16_t   u16TailPos;  ///< Tail position
    uint16_t   u16BuffSize; ///< Buffer Size
    uint8_t   *pDataBuff;   ///< Pointer to the buffer
} CIR_BUFF;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

extern bool     CirBuffInit(CIR_BUFF *pCB, uint8_t *pDataBuffIn, uint16_t u16BuffSize);
extern uint16_t CirBuffPush(CIR_BUFF *pCB, uint8_t *pDataIn,   uint16_t u16DataCountIn); 
extern uint16_t CirBuffPeek(CIR_BUFF *pCB, uint8_t *pDataOut,  uint16_t u16DataCountIn); 
extern uint16_t CirBuffPop(CIR_BUFF *pCB, uint16_t u16DataCountIn);                     
extern uint16_t CirBuffCount(CIR_BUFF *pCB);
extern uint16_t CirBuffFreeSpace(CIR_BUFF *pCB);
extern void     CirBuffClear(CIR_BUFF *pCB);

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* CIR_BUFF_H */

/* End of file */

