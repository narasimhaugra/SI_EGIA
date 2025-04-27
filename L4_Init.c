#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \brief   Layer 4 initialization function
 *
 * \details This file contains layer 4 initialization function which invokes
 *          all relevant module initializaiton functions.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_Init.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_Init.h"
#include "L4_DisplayManager.h"
#include "Signia_SoundManager.h"
#include "Signia_CommManager.h"
#include "Signia_Keypad.h"
#include "Signia_AdapterManager.h"
#include "L4_BlobHandler.h"
#include "L4_ConsoleManager.h"
#include "Signia_ChargerManager.h"
#include "Signia_Accelerometer.h"
#include "Signia_Motor.h"
#include "FaultHandler.h"
#include "L4_HandleKvf.h"
/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/

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
 * \brief   Layer 4 initialization function
 *
 * \details Initializes Layer 4 functions and module
 *
 * \param   < None >
 *
 * \return  bool - status
 *
 * ========================================================================== */
 bool L4_Init(void)
{
    bool Status;
    
#if USE_KVF_VALUES    
    /* initialise the handle Kvf*/
    HandleKvfInit();
#endif  

    /* Initialize L4 Module in desired order */     
    Status = (ACCEL_STATUS_OK != L4_AccelInit());
    /* Initialize the Sound Manager module */
    L4_SoundManagerCtor();
    /* Initialize the Communication Manager module */
    Status = (COMM_MGR_STATUS_OK != L4_CommManagerInit())     || Status;
    /* Initialize the Keypad Handler module */
    Status = (KEYPAD_STATUS_OK != L4_KeypadInit())            || Status;
    /* Initialize the Adapter Manager module */
    Status = (AM_STATUS_OK != L4_AdapterManagerInit())        || Status;
    /* Initialize the Blob Handler module */
    Status = (BLOB_STATUS_OK != L4_BlobHandlerInit())         || Status;
    /* Initialize the Console Manager module */
    Status = (CONS_MGR_STATUS_OK != L4_ConsoleMgrInit())      || Status;
    /* Initialize the Charger Manager module */
    Status = (CHRG_MNGR_STATUS_OK != L4_ChargerManagerInit()) || Status;
    /* Initialize the Display Manager module */
    Status = (DM_STATUS_OK != L4_DmInit())                    || Status;

    return Status;
}

#ifdef __cplusplus  /* header compatible with C++ project */
 }
#endif

