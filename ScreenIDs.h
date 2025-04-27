#ifndef SCREEN_IDS_H
#define SCREEN_IDS_H

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
 * \details This file has contains Screen IDs for proper UI functionality.
 *
 * \copyright 2023 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    ScreenIDs.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/

/**
 * \}  <If using addtogroup above>
 */

/* Screen IDs enumeration */
typedef enum
{
    SCREEN_ID_NONE,                                     /* */
    SCREEN_ID_WELCOME,
    SCREEN_ID_ADAPTER_CHECK,
    SCREEN_ID_ADAPTER_CALIB_PROGRESS,
    SCREEN_ID_ADAPT_COMPAT,                             /* Static Screen */
    SCREEN_ID_ADAPTER_CALIB,           
    SCREEN_ID_ADAPTER_ERROR,            
    SCREEN_ID_ADAPTER_RELOAD_ATTACHED,                  /* Animated Screen */
    SCREEN_ID_ADAPTER_REQUEST,                          /* Animated Screen */
    SCREEN_ID_ADAPTER_UNSUPPORT,
    SCREEN_ID_ADAPTER_VERIFY_ONE,
    SCREEN_ID_ADAPTER_VERIFY_TWO,
    SCREEN_ID_BATT_CONNECTION_ERR,
    SCREEN_ID_CLAMSHELL_ERROR,
    SCREEN_ID_DEPLETED_BATT,                            /* Animated Screen */
    SCREEN_ID_EGIA_RELOAD_INTELLIGENT,                  /* Animated Screen */
    SCREEN_ID_EGIA_RELOAD_NON_INTELLIGENT,              /* Animated Screen */        
    SCREEN_ID_END_OF_LIFE_ADAPTER,
    SCREEN_ID_END_OF_LIFE_MULU,
    SCREEN_ID_FLUID_INGRESS,
    SCREEN_ID_HANDLE_END_OF_LIFE,
    SCREEN_ID_HANDLE_ERROR,
    SCREEN_ID_HANDLE_STATUS,
    SCREEN_ID_HIGH_FORCE,
    SCREEN_ID_INSERT_CARTRIDGE,
    SCREEN_ID_INSERT_CLAMSHELL,                         /* Animated Screen */
    SCREEN_ID_INSUFFICIENT_COMPAT_ADAPTER,
    SCREEN_ID_INSUFFICIENT_BATT_PP1,
    SCREEN_ID_INSUFFICIENT_BATT_PP2,
    SCREEN_ID_INSUFFICIENT_BATTERY,
    SCREEN_ID_LOW_COMPAT_ADAPTER,
    SCREEN_ID_LOW_BATT_PR1,
    SCREEN_ID_LOW_BATT_PR2,
    SCREEN_ID_LOW_BATTERY,
    SCREEN_ID_MAIN,
    SCREEN_ID_PROCEDURE_REMAIN,
    SCREEN_ID_RB_ACTIVATED_BOTH,
    SCREEN_ID_RB_ACTIVATED_LEFT,
    SCREEN_ID_RB_ACTIVATED_RIGHT,
    SCREEN_ID_RB_ACTIVATION_COUNT1_LEFT,
    SCREEN_ID_RB_ACTIVATION_COUNT1_RIGHT,
    SCREEN_ID_RB_ACTIVATION_COUNT2_LEFT,
    SCREEN_ID_RB_ACTIVATION_COUNT2_RIGHT,
    SCREEN_ID_RB_ACTIVATION_COUNT3_LEFT,
    SCREEN_ID_RB_ACTIVATION_COUNT3_RIGHT,
    SCREEN_ID_RB_DEACTIVATED_BOTH,
    SCREEN_ID_RB_DEACTIVATED_LEFT,
    SCREEN_ID_RB_DEACTIVATED_RIGHT,
    SCREEN_ID_RB_DEACTIVATION_COUNT1_LEFT,
    SCREEN_ID_RB_DEACTIVATION_COUNT1_RIGHT,
    SCREEN_ID_RB_DEACTIVATION_COUNT2_LEFT,
    SCREEN_ID_RB_DEACTIVATION_COUNT2_RIGHT,
    SCREEN_ID_RB_DEACTIVATION_COUNT3_LEFT,
    SCREEN_ID_RB_DEACTIVATION_COUNT3_RIGHT,
    SCREEN_ID_RBAI_LEFT,
    SCREEN_ID_RBAI_RIGHT,
    SCREEN_ID_RBAI_OTHER_SIDE_ACTIVE_LEFT,
    SCREEN_ID_RBAI_OTHER_SIDE_ACTIVE_RIGHT,
    SCREEN_ID_READY_NON_INT_RELOAD,
    SCREEN_ID_READY_SULU_LOADED,
    SCREEN_ID_READY_MULU_LOADED,
    SCREEN_ID_RELOAD_ERROR_WARNING,
    SCREEN_ID_MULU_ERROR_WARNING,
    SCREEN_ID_MULU_CARTRIDGE_ERROR_WARNING,
    SCREEN_ID_REQ_RESET_1,
    SCREEN_ID_REQ_RESET_2,
    SCREEN_ID_REQ_RESET_3,
    SCREEN_ID_REQ_RESET_4,
    SCREEN_ID_REQ_RESET_5,
    SCREEN_ID_REQ_RESET_6,
    SCREEN_ID_REQUEST_RELOAD,                           /* Animated Screen */                                    
    SCREEN_ID_RESET_ERR,                                /* No Progress Bar */
    SCREEN_ID_SET_BATTERY_LEVEL,
    SCREEN_ID_SET_BATTERY_LEVEL_LOW,
    SCREEN_ID_SET_BATTERY_LEVEL_INSUFF,
    SCREEN_ID_USED_CLAMSHELL,
    SCREEN_ID_REQ_RESET_ERR,
    SCREEN_ID_USED_CARTRIDGE,
    SCREEN_ID_FIRING_PROGRESS,
    SCREEN_ID_LAST
} SCREEN_ID;
  
#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* SCREEN_IDS_H */

