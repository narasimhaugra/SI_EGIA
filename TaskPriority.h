#ifndef TASKPRIORITY_H
#define TASKPRIORITY_H

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
 * \details This file has contains all signia platoform symbolic constants and
 *          function prototypes.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    TaskPriority.h
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
/*  !! Lower the number, higher the priority. !!
    Entries into the table are arranged as per the design. */

typedef enum
{
    TASK_PRIORITY_RESERVED,            /* 0 - Reserved Priority */
    TASK_PRIORITY_RESERVE_1,           /* Reserved/Conflicts */
    TASK_PRIORITY_RESERVE_2,           /* Reserved/Conflicts */
    TASK_PRIORITY_RESERVE_3,           /* Reserved/Conflicts */
    TASK_PRIORITY_RESERVE_4,           /* Reserved/Conflicts */
    TASK_PRIORITY_FPGA_CNTLR,          /* FPGA Controller task */
    TASK_PRIORITY_TASK_MONITOR,        /* Task Monitor task */           /// \todo 10/04/2021 DAZ - Should this be higher than FPGA?
    TASK_PRIORITY_RESERVE_5,           /* Reserved/Conflicts */
    TASK_PRIORITY_RESERVE_6,           /* Reserved/Conflicts */
    TASK_PRIORITY_RESERVE_7,           /* Reserved/Conflicts */
    TASK_PRIORITY_RESERVE_8,           /* Reserved/Conflicts */
    TASK_PRIORITY_RESERVE_9,           /* Reserved/Conflicts */
    TASK_PRIORITY_RESERVE_10,          /* Reserved/Conflicts */
    TASK_PRIORITY_STARTUP,             /* Startup task */
    TASK_PRIORITY_L3_ONEWIRE,          /* Onewire task */
    TASK_PRIORITY_RESERVE_11,          /* Reserved/Conflicts */
    TASK_PRIORITY_L4_COMM_MANAGER,     /* Communication Manager task */
    TASK_PRIORITY_L3_IRRECEIVER,       /* IR Receiver Task */
    TASK_PRIORITY_L4_KEYPAD_HANDLER,   /* Keypad Handler task */
    TASK_PRIORITY_L4_SOUNDMGR,         /* Sound Manager Task */
    TASK_PRIORITY_L4_ADAPTER_MANAGER,  /* Adapter Manager task */
    TASK_PRIORITY_HANDLE,              /* Application */ /// \todo 05/24/2021 DAZ - Needs to be adjusted
    TASK_PRIORITY_L4_DISPMANAGER,      /* Display Manager task */
    TASK_PRIORITY_L4_SENSOR,           /* Sensor Manager Task */
    TASK_PRIORITY_L4_CONSOLE_MANAGER,  /* Console Manager Task */
    TASK_PRIORITY_L4_CONSOLE_STATUS,   /* Console Manager Status Task */
    TASK_PRIORITY_L4_CHARGER_MANAGER,  /* Charger Manager Task */
    TASK_PRIORITY_L4_ACCEL,            /* Accelerometer Task */
    TASK_PRIORITY_CLEANUP,                  /* Rdf file cleanup task */
    TASK_PRIORITY_LOG,                 /* Log task */
    TASK_PRIORITY_AO_QPC_TEST,         /* QPC test stub AO Task */
    TASK_PRIORITY_APP_TEST_STUB,       /* Top level application test stub */
    TASK_PRIORITY_AO_MOTOR_DEV,        /* Motor test task */
    TASK_PRIORITY_AO_KEYPAD_DEV,       /* Signia Keypad test stub */
    TASK_PRIORITY_TM,                  /* Test Manager task*/
    TASK_PRIORITY_BACKGROUND_DIAGTASK, /* Background Diagnostic task */
    TASK_PRIORITY_LAST
} SIGNIA_TASK_PRIORITY;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* TASKPRIORITY_H */

