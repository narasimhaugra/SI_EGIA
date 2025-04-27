#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Include
 * \{
 * \brief   Public interface for signia platform modules
 *
 * \details This header file provides definitions for various preprocessor 
 *          directives used in the project 
 * 
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Config.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define DO_MEMORY_FENCE_TEST

#ifdef DO_MEMORY_FENCE_TEST
   #define MEMORY_FENCE_SIZE_DWORDS    (8u)  // Number of INT32U to use as fence
   #define MEMORY_FENCE_SIZE_BYTES     (MEMORY_FENCE_SIZE_DWORDS * 4u)
   #define MEMORY_FENCE_STATIC
#else
   #define MEMORY_FENCE_SIZE_BYTES     (0u)
   #define MEMORY_FENCE_SIZE_DWORDS    (0u)
   #define MEMORY_FENCE_STATIC         static
#endif
    
/* If we are using a C3B, enable the below macro. In legacy, this is peformed via KVF settings. */
/// \todo 02/11/2021 GK - Revisit this once we have KVF file handling */
//#define IS_C3_BOARD

/******************************************************************************/
/*                             Global Type(s)                                 */
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

/**
 * \}  <If using addtogroup above>
 */
  
#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* CONFIG_H */

