#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \brief   Layer 5 initialization function
 *
 * \details This file contains layer 5 initialization function which invokes
 *          all relevant module initializaiton functions.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L5_Init.c
 *
 * ========================================================================== */


/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L5_Init.h"
#include "Handle.h"

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
/* None */
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
/* None */
/******************************************************************************/
/*                             Global Function Prototype(s)                    */
/******************************************************************************/
/* None */
/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
/* None */
/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Layer 5 initialization funcion
 *
 * \details Initializes Layer 5 functions and module
 *
 * \note    This function currently always returns true, as the App constructor
 *          presently does not return an error. If an error occurs, an exception
 *          is thrown.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
 bool L5_Init(void)
{
    bool Status;

    /* Initialize L5 Module in desired order */

    Status = true; /* Return status, false if no errors */

    do
    {
        /* Initialize and start the top level Active Object (App) */
        HandleCtor();
        Status = false; /* Return status, false if no errors */

    } while (false);

    return Status;
}

#ifdef __cplusplus  /* header compatible with C++ project */
 }
#endif

