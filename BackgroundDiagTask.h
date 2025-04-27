/*==================================================================================================
 * \brief Brief Description.
 *
 * \details   This header file provides the public interface for the
 *            Background Diagnostic Task module, including symbolic constants, enumerations,
 *            type definitions, and function prototypes.
 *
 * \bug  Noe
 *
 * \copyright  Copyright 2019 Covidien - Surgical Innovations.  All Rights Reserved.
 * * \addtogroup Background Diagnostic Task
 * \{
 *
 * * \file  BackgroundDiagTask.h
 *
==================================================================================================*/

#ifndef BACKGROUNDTASK_H
#define BACKGROUNDTASK_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/**************************************************************************************************/
/*                                         Includes                                               */
/**************************************************************************************************/
#include "TestManager.h"
#include "ActiveObject.h"
#include "FileUtil.h"
#include "L3_Fpga.h"
#include "L3_MotorCommon.h"
#include "L3_Motor.h"
#include "L3_OneWireCommon.h"
#include "L4_AdapterDefn.h"
#include "L4_ConsoleManager.h"
#include "Rdf.h"

/**************************************************************************************************/
/*                                    Global Define(s) (Macros)                                   */
/**************************************************************************************************/


/**************************************************************************************************/
/*                                    Global Type(s)                                              */
/**************************************************************************************************/
typedef enum                                  ///< Status
{
   BACKGROUND_STATUS_OK,                   ///< BACKGROUND STATUS OK
   BACKGROUND_STATUS_ERROR,                ///< BACKGROUND STATUS ERROR
   BACKGROUND_STATUS_INVALIDPARAM,         ///< BACKGROUDN STATUS INVALID
}BACKGROUND_STATUS;

/**************************************************************************************************/
/*                                   Global Constant Declaration(s)                               */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                   Global Variable Declaration(s)                               */
/**************************************************************************************************/
/**************************************************************************************************/
/*                                   Global Function Prototype(s)                                 */
/**************************************************************************************************/
BACKGROUND_STATUS BackgroundDiagTaskInit(void);

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
#endif /* __BACKGROUNDTASK_H__ */

/**
 * \}  <If using addtogroup above>
 */

