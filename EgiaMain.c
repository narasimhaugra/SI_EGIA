#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Egia
 * \{
 *
 * \brief   EGIA Main application entry function
 *
 * \details This file contians main entry function initializing EGIA Application
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    EgiaMain.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Signia.h"

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#undef  LOG_GROUP_IDENTIFIER                  /// \todo 08/01/2022 DAZ - Remove definition of this symbol from .h files then remove this.
#define LOG_GROUP_IDENTIFIER  LOG_GROUP_EGIA  ///< Log group identifier

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   EGIA Application entry function
 *
 * \details Initializes necessary EGIA application entities
 *
 * \note    Must be defined respective application module.
 *          Perform any additional application module initialization here.
 *          The function must return.
 *
 * \param   < None >
 *
 * \b Returns void
 *
 * ========================================================================== */
void SigniaAppInitEgia(void)
{
    Log(TRC, "EGIA Main: EGIA Application Init called");
}


/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
