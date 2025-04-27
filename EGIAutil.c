/** \todo 06/17/2022 DQAZ - NOTE: This module / EGIA in general has a basic design problem as follows:
*       The local data store is divided into two parts - That of the Handle superstate and that of the
*       EGIA substates. Before these routines were removed from EGIA.c, the EGIA local data was accessed
*       via a static pointer, pEgia. A function has been provided in EGIA: EGIA_GetDataPtr() to retrieve
*       the pointer to the EGIA local store.
* \n \n
*       The EGIA data SHOULD appear as part of the Handle structure. This was the original design intent. Further,
*       the EGIA (and other applications) were part of a union, so that the local memory taken by each application
*       could be shared. As it is, when other applications are added, memory (already at a premium) will be
*       wasted, as EGIA memory is statically declared (as will be EEA, etc). This separate static declaration was
*       done because of the desire for debugging platform as a standalone item, and for building images with
*       or without various applications (ie EEA), without having to modify platform. (although at level 5)
* \n \n
*       This is a design deficiency which will need to be addressed at some point. For now, some of these routines
*       require two data store pointers - one to the Handle datastore, and one to the EGIA datastore. (The design
*       should be changed so these can be handled with a single pointer)
*/

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup EGIA
 * \{
 *
 * \brief   Utility and helper routines for the EGIA states.
 *
 * \details This module contains support routines for the EGIA substates of the
 *          Handle state machine. They are included here for ease of maintenance and
 *          reduce changes to the EGIA.qm model.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    EGIAutil.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Signia.h"
#include "EGIA.h"
#include "EGIAutil.h"
#include "EGIAscreens.h"
#include "TestManager.h"
#include "L4_GpioCtrl.h"
#include "HandleUtil.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define MAX_LEFT_BUTTON_STATES          (2u) ///< Maximum states for left Rotate button
#define MAX_RIGHT_BUTTON_STATES         (2u) ///< Maximum states for Right Rotate buttons
#define MAX_ROTATION_CONFIG_COUNTER     (3u) ///< Maximum value of Rotation configuration counter in seconds
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
/// Structure to hold Mulu and Cartridge compatibility
typedef struct
{
    uint16_t MuluId;                 ///< MULU Reload ID
    uint16_t CompatibleCartridgeId;  ///< Compatible Cartridge ID
} MULU_CARTRIDGE_COMPATIBLE_LIST;

typedef enum
{
    EGIA_IPROF_TABLE_INDEX_30MM_LINEAR_RELOAD,          ///< 30mm Linear Reload index
    EGIA_IPROF_TABLE_INDEX_45MM_LINEAR_RELOAD,          ///< 45mm Linear Reload index
    EGIA_IPROF_TABLE_INDEX_60MM_LINEAR_RELOAD,          ///< 60mm Linear Reload index
    EGIA_IPROF_TABLE_INDEX_72MM_RADIAL_RELOAD,          ///< 72mm Radial Reload index
    EGIA_IPROF_TABLE_INDEX_NOID_RELOAD,                 ///< Index for No id Reload
    EGIA_IPROF_TABLE_INDEX_FUTURE_RELOAD,               ///< Reserved for future use
    EGIA_IPROF_TABLE_INDEX_COUNT
} EGIA_IPROFILE_TYPE;

typedef enum
{
    ROTATION_STOPPED,                ///< Rotation stopped
    ROTATION_DEBOUNCE,               ///< Roation Debounce
    ROTATION_INPROGRESS,             ///< Rotation Inprogress
    ROTATION_CONFIG,                 ///< Rotation configuraiton
    ROTATION_DISPLAYWAIT,            ///< Rotation display wait
    ROTATION_CONFIG_STATE_COUNT      ///< Rotation configuration state count
} ROTAITON_CONFIG_STATES;

typedef enum
{
    ROTATE_REQ_CW,                  ///< Request to Rotate Cloackwise
    ROTATE_REQ_CCW,                 ///< Request to Rotate AntiClockwise
    ROTATE_DIR_COUNT,               ///< Rotate direction Count
    ROTATE_REQ_INVALID              ///< Invalid Rotate Request
} ROTAITON_REQ_DIR;

typedef enum
{
    ROTATION_SCREENTYPE_ROTATEKEY_DISSABLED,
    ROTATION_SCREENTYPE_ROTATIONCONFIG_COUNTER,
} ROTATION_SCREENTYPE;

typedef struct
{
    DEVICE_ID_ENUM      DevId;           ///< Device ID
    EGIA_IPROFILE_TYPE  Index;       ///< Index of the current profile
} ReloadDevIdToIprofileMap;

typedef void (*ScreenDef)(void);   ///< Screen typdef for Rotation screens

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

const MULU_CARTRIDGE_COMPATIBLE_LIST MuluCartridgeCompatiblity[NO_OF_RELOAD_TYPES] =
{
    /* MULU-Type            ,  Compatible Cartridge */
    { DEVICE_ID_EGIA_MULU_30, DEVICE_ID_EGIA_CART_30 },
    { DEVICE_ID_EGIA_MULU_45, DEVICE_ID_EGIA_CART_45 },
    { DEVICE_ID_EGIA_MULU_60, DEVICE_ID_EGIA_CART_60 },
};

static const int32_t RotateMotorPosition[] =
{
    ROTATE_MOTOR_ONE_EIGHTY_DEGREE_CW,        ///< Rotate motor Clockwise
    ROTATE_MOTOR_ONE_EIGHTY_DEGREE_CCW        ///< Rotate motor Counter Closckwise

};

static const ScreenDef LeftRotationConfigScreens[MAX_LEFT_BUTTON_STATES][MAX_RIGHT_BUTTON_STATES] =
{
    NULL,
    Gui_RotateDeactLeft_ScreenSet,              ///< Rotation configuration with Deactivated Left buttons screen
    Gui_RotationACtivatedLeft_ScreenSet,        ///< Rotation configuration with Activated Left buttons screen
    Gui_RotationActivatedBoth_ScreenSet         ///< Rotation with both side buttons Activate screen
};

static const ScreenDef RightRotationConfigScreens[MAX_LEFT_BUTTON_STATES][MAX_RIGHT_BUTTON_STATES] =
{
    NULL,
    Gui_RotationACtivatedRight_ScreenSet,       ///< Rotation configuration with Activated Right screen
    Gui_RotateDeactRight_ScreenSet,             ///< Rotation configuration with Deactivated Right screen
    Gui_RotationActivatedBoth_ScreenSet         ///< Rotation with both side buttons Activate screen
};




/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
#pragma location=".sram"
static FIREMODE_FORCETOSPEED ForceToSpeedTable[FIRINGSPEED_LAST] = { 0 };

//  Current Limit Profile for Pattern Matching
//  Current Limit Profile for Legacy Reloads with No ID
static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_NoID_ArticCenter =
{
    11u,                            // Number of Entries
    {   14u,  20u,   73u,   81u,  90u,   92u,  101u, 117u,  124u,  131u, 132u }, // Shaft Turns
    { 5500u,3000u,40000u,40000u, 450u,40000u,40000u, 700u,40000u,40000u, 450u }, // Current Limit or Delta
    {    0u,   1u,    0u,    0u,   1u,    0u,    0u,   1u,    0u,    0u,   1u }, // Method (0-Abs (MOT_CTPROF_METH_ABSOLUTE), 1-Delta (MOT_CTPROF_METH_DELTA), 2-Slope (MOT_CTPROF_METH_SLOPE))
    {    2u,   1u,    3u,    0u,   1u,    3u,    0u,   1u,    3u,    0u,   1u }, // Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {  0.0f, 1.0f,  0.0f,  0.0f, 1.3f,  0.0f,  0.0f, 1.0f,  0.0f,  0.0f, 0.1f }, // K Coefficient (0.0 - 2.0)
};

static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_NoID_ArticFarRight =
{
    11u,                            // Number of Entries
    {   15u,  20u,   73u,   81u,  90u,   92u,  101u, 117u,  124u,  131u, 132u }, // Shaft Turns
    { 5500u,3000u,40000u,40000u, 450u,40000u,40000u, 700u,40000u,40000u, 450u }, // Current Limit or Delta
    {    0u,   1u,    0u,    0u,   1u,    0u,    0u,   1u,    0u,    0u,   1u }, // Method (0-Abs, 1-Delta, 2-Slope)
    {    2u,   1u,    3u,    0u,   1u,    3u,    0u,   1u,    3u,    0u,   1u }, // Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {  0.0f, 1.0f,  0.0f,  0.0f, 1.3f,  0.0f,  0.0f, 1.0f,  0.0f,  0.0f, 0.1f }, // K Coefficient (0.0 - 2.0)
};

static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_NoID_ArticFarLeft =
{
    11u,                            // Number of Entries
    {   13u,  21u,   73u,   81u,  90u,   92u,  101u, 117u,  124u,  131u, 132u }, //  Shaft Turns
    { 5500u,3000u,40000u,40000u, 450u,40000u,40000u, 700u,40000u,40000u, 450u }, //  Current Limit or Delta
    {    0u,   1u,    0u,    0u,   1u,    0u,    0u,   1u,    0u,    0u,   1u }, //  Method (0-Abs, 1-Delta, 2-Slope)
    {    2u,   1u,    3u,    0u,   1u,    3u,    0u,   1u,    3u,    0u,   1u }, //  Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {  0.0f, 1.0f,  0.0f,  0.0f, 1.3f,  0.0f,  0.0f, 1.0f,  0.0f,  0.0f, 0.1f }, //  K Coefficient (0.0 - 2.0)
};

//   *  Current Limit Profile for 30mm Reloads with ID
static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_30ID_ArticCenter =
{
    5u,                                   // Number of Entries
    {    14u,   20u,    73u,    81u,   82u }, //  Shaft Turns
    {  5500u, 3000u, 40000u, 40000u,  100u }, //  Current Limit or Delta
    {     0u,    1u,     0u,     0u,    1u }, //  Method (0-Abs, 1-Delta, 2-Slope)
    {     2u,    1u,     3u,     0u,    1u }, //  Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {   0.0f,  1.0f,   0.0f,   0.0f,  0.1f }, //  K Coefficient (0.0 - 2.0)
};
static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_30ID_ArticFarRight =
{
    5u,                                   // Number of Entries
    {    15u,   20u,    73u,    81u,   82u }, //  Shaft Turns
    {  5500u, 3000u, 40000u, 40000u,  100u }, //  Current Limit or Delta
    {     0u,    1u,     0u,     0u,    1u }, //  Method (0-Abs, 1-Delta, 2-Slope)
    {     2u,    1u,     3u,     0u,    1u }, //  Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {   0.0f,  1.0f,   0.0f,   0.0f,  0.1f }, //  K Coefficient (0.0 - 2.0)
};
static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_30ID_ArticFarLeft =
{
    5u,                                   // Number of Entries
    {    13u,   21u,    73u,    81u,   82u }, //  Shaft Turns
    {  5500u, 3000u, 40000u, 40000u,  100u }, //  Current Limit or Delta
    {     0u,    1u,     0u,     0u,    1u }, //  Method (0-Abs, 1-Delta, 2-Slope)
    {     2u,    1u,     3u,     0u,    1u }, //  Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {   0.0f,  1.0f,   0.0f,   0.0f,  0.1f }, //  K Coefficient (0.0 - 2.0)
};
static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_Future_Artic =
{
    5u,                                   // Number of Entries
    {    14u,   21u,    73u,    74u,   75u }, //  Shaft Turns
    {  5500u, 3000u, 40000u, 40000u,  100u }, //  Current Limit or Delta
    {     0u,    1u,     0u,     0u,    1u }, //  Method (0-Abs, 1-Delta, 2-Slope)
    {     2u,    1u,     3u,     0u,    1u }, //  Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {   0.0f,  1.0f,   0.0f,   0.0f,  0.1f }, //  K Coefficient (0.0 - 2.0)
};


//   *  Current Limit Profile for 45mm Reloads with ID
static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_45ID_ArticCenter =
{
    5u,                                   // Number of Entries
    {    14u,   20u,    92u,   101u,  102u }, //  Shaft Turns
    {  5500u, 3000u, 40000u, 40000u,  100u }, //  Current Limit or Delta
    {     0u,    1u,     0u,     0u,    1u }, //  Method (0-Abs, 1-Delta, 2-Slope)
    {     2u,    1u,     3u,     0u,    1u }, //  Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {   0.0f,  1.0f,   0.0f,   0.0f,  0.1f }, //  K Coefficient (0.0 - 2.0)
};
static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_45ID_ArticFarRight =
{
    5,                                   // Number of Entries
    {    15,   20,    92,   101,  102 }, //  Shaft Turns
    {  5500, 3000, 40000, 40000,  100 }, //  Current Limit or Delta
    {     0,    1,     0,     0,    1 }, //  Method (0-Abs, 1-Delta, 2-Slope)
    {     2,    1,     3,     0,    1 }, //  Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {   0.0,  1.0,   0.0,   0.0,  0.1 }, //  K Coefficient (0.0 - 2.0)
};
static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_45ID_ArticFarLeft =
{
    5u,                                   // Number of Entries
    {    13u,   21u,    92u,   101u,  102u }, //  Shaft Turns
    {  5500u, 3000u, 40000u, 40000u,  100u }, //  Current Limit or Delta
    {     0u,    1u,     0u,     0u,    1u }, //  Method (0-Abs, 1-Delta, 2-Slope)
    {     2u,    1u,     3u,     0u,    1u }, //  Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {   0.0f,  1.0f,   0.0f,   0.0f,  0.1f }, //  K Coefficient (0.0 - 2.0)
};

//   *  Current Limit Profile for 60mm Reloads with ID
static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_60ID_ArticCenter =
{
    5u,                                   // Number of Entries
    {    14u,   20u,   124u,   131u,  132u }, //  Shaft Turns
    {  5500u, 3000u, 40000u, 40000u,  100u }, //  Current Limit or Delta
    {     0u,    1u,     0u,     0u,    1u }, //  Method (0-Abs, 1-Delta, 2-Slope)
    {     2u,    1u,     3u,     0u,    1u }, //  Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {   0.0f,  1.0f,   0.0f,   0.0f,  0.1f }, //  K Coefficient (0.0 - 2.0)
};
static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_60ID_ArticFarRight =
{
    5u,                                   // Number of Entries
    {    15u,   20u,   124u,   131u,  132u }, //  Shaft Turns
    {  5500u, 3000u, 40000u, 40000u,  100u }, //  Current Limit or Delta
    {     0u,    1u,     0u,     0u,    1u }, //  Method (0-Abs, 1-Delta, 2-Slope)
    {     2u,    1u,     3u,     0u,    1u }, //  Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {   0.0f,  1.0f,   0.0f,   0.0f,  0.1f }, //  K Coefficient (0.0 - 2.0)
};
static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_60id_ArticFarLeft =
{
    5u,                                   // Number of Entries
    {    13u,   21u,   124u,   131u,  132u }, //  Shaft Turns
    {  5500u, 3000u, 40000u, 40000u,  100u }, //  Current Limit or Delta
    {     0u,    1u,     0u,     0u,    1u }, //  Method (0-Abs, 1-Delta, 2-Slope)
    {     2u,    1u,     3u,     0u,    1u }, //  Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {   0.0f,  1.0f,   0.0f,   0.0f,  0.1f }, //  K Coefficient (0.0 - 2.0)
};

//   *  Current Limit Profile for 72mm Reloads with ID
static const MOT_CURTRIP_PROFILE  EGIA_ILimitProf_72ID_NoArtic =
{
    5u,                                   // Number of Entries
    {    14u,   19u,   131u,   140u,  141u }, //  Shaft Turns
    {  5500u, 3000u, 40000u, 40000u,  100u }, //  Current Limit or Delta
    {     0u,    1u,     0u,     0u,    1u }, //  Method (0-Abs, 1-Delta, 2-Slope)
    {     2u,    1u,     3u,     0u,    1u }, //  Zone ID (0-Learn, 1-Endstop, 2-Interlock(Learn), 3-Norm, 4-Unused
    {   0.0f,  1.0f,   0.0f,   0.0f,  0.1f }, //  K Coefficient (0.0 - 2.0)
};

static const EgiaCLProfArticTable  EGIA_72ID_IProf_ArticTable =
{
    2u,                                     // Number of Entries
    {   1.0f, 20.0f },                      //  Artic Position in Turns
    //  Current Limit Prof Table for each Artic Position
    {
        &EGIA_ILimitProf_72ID_NoArtic,
        &EGIA_ILimitProf_72ID_NoArtic
    },
};

static const EgiaCLProfArticTable  EGIA_60ID_IProf_ArticTable =
{
    4u,                                     // Number of Entries
    {   1.0f, 5.5f, 15.5f, 20.0f },         //  Artic Position in Turns
    //  Current Limit Prof Table for each Artic Position
    {
        &EGIA_ILimitProf_60id_ArticFarLeft,
        &EGIA_ILimitProf_60id_ArticFarLeft,
        &EGIA_ILimitProf_60ID_ArticCenter,
        &EGIA_ILimitProf_60ID_ArticFarRight
    },
};

static const EgiaCLProfArticTable  EGIA_30ID_IProf_ArticTable =
{
    4u,                                     // Number of Entries
    {   1.0f, 5.5f, 15.5f, 20.0f },         //  Artic Position in Turns
    //  Current Limit Prof Table for each Artic Position
    {
        &EGIA_ILimitProf_30ID_ArticFarLeft,
        &EGIA_ILimitProf_30ID_ArticFarLeft,
        &EGIA_ILimitProf_30ID_ArticCenter,
        &EGIA_ILimitProf_30ID_ArticFarRight
    },
};

static const EgiaCLProfArticTable  EGIA_45ID_IProf_ArticTable =
{
    4u,                                     // Number of Entries
    {   1.0f, 5.5f, 15.5f, 20.0f },         //  Artic Position in Turns
    //  Current Limit Prof Table for each Artic Position
    {
        &EGIA_ILimitProf_45ID_ArticFarLeft,
        &EGIA_ILimitProf_45ID_ArticFarLeft,
        &EGIA_ILimitProf_45ID_ArticCenter,
        &EGIA_ILimitProf_45ID_ArticFarRight
    },
};

static const EgiaCLProfArticTable  EGIA_Future_IProf_ArticTable =
{
    4u,                                     // Number of Entries
    {   1.0f, 5.5f, 15.5f, 20.0f },         //  Artic Position in Turns
    //  Current Limit Prof Table for each Artic Position
    {
        &EGIA_ILimitProf_Future_Artic,
        &EGIA_ILimitProf_Future_Artic,
        &EGIA_ILimitProf_Future_Artic,
        &EGIA_ILimitProf_Future_Artic
    },
};
static const EgiaCLProfArticTable  EGIA_NoID_IProf_ArticTable =
{
    4u,                                     // Number of Entries
    {   1.0f, 5.5f, 15.5f, 20.0f },         //  Artic Position in Turns
    //  Current Limit Prof Table for each Artic Position
    {
        &EGIA_ILimitProf_NoID_ArticFarLeft,
        &EGIA_ILimitProf_NoID_ArticFarLeft,
        &EGIA_ILimitProf_NoID_ArticCenter,
        &EGIA_ILimitProf_NoID_ArticFarRight
    },
};

static ReloadDevIdToIprofileMap ReloadMap[] =        // Table used to Map the Reload ID to Reload current limit table index
{
    { DEVICE_ID_EGIA_SULU_30,    EGIA_IPROF_TABLE_INDEX_30MM_LINEAR_RELOAD },
    { DEVICE_ID_EGIA_RADIAL_30,  EGIA_IPROF_TABLE_INDEX_30MM_LINEAR_RELOAD },   // currently 30mm Raidal Egia does't exist, so mapping it to Linear
    { DEVICE_ID_EGIA_MULU_30,    EGIA_IPROF_TABLE_INDEX_30MM_LINEAR_RELOAD },
    { DEVICE_ID_EGIA_SULU_45,    EGIA_IPROF_TABLE_INDEX_45MM_LINEAR_RELOAD },
    { DEVICE_ID_EGIA_RADIAL_45,  EGIA_IPROF_TABLE_INDEX_45MM_LINEAR_RELOAD },
    { DEVICE_ID_EGIA_MULU_45,    EGIA_IPROF_TABLE_INDEX_45MM_LINEAR_RELOAD },
    { DEVICE_ID_EGIA_SULU_60,    EGIA_IPROF_TABLE_INDEX_60MM_LINEAR_RELOAD },
    { DEVICE_ID_EGIA_RADIAL_60,  EGIA_IPROF_TABLE_INDEX_72MM_RADIAL_RELOAD },
    { DEVICE_ID_EGIA_MULU_60,    EGIA_IPROF_TABLE_INDEX_60MM_LINEAR_RELOAD }
};

static EgiaReloadTable ReloadIProfileTable[] =
{
    &EGIA_30ID_IProf_ArticTable,   // Current Limit Prof Artic Table
    &EGIA_45ID_IProf_ArticTable,   // Current Limit Prof Artic Table
    &EGIA_60ID_IProf_ArticTable,   // Current Limit Prof Artic Table
    &EGIA_72ID_IProf_ArticTable,   // Current Limit Prof Artic Table
    &EGIA_NoID_IProf_ArticTable,   // Current Limit Prof Artic Table
    &EGIA_Future_IProf_ArticTable  // Current Limit Prof Artic Table
};

// *  Table below is indexed by Defines above
static EgiaMaxFireTurnsTable EgiaMaxFireTurns =
{
    -93.0f,        // Reload - ID, 30mm, Linear
        -119.0f,        // Reload - ID, 45mm, Linear
        -148.0f,        // Reload - ID, 60mm, Linear
        -154.0f,        // Reload - ID, 72mm, Radial
        -148.0f,        // Reload - No ID
        -248.0f,        // Reload - ID, Future
        0.0f,          // Reload - No Reload
    };

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void DisplayReloadScreens_ClampTestPass(EGIA *const me);
static void ProcessCartridge(Handle *const pMe);
static bool IsCartridgeCompatible(Handle *const pMe);
static AM_STATUS GetAsaClampForceSpeed(Handle *const pMe, uint8_t ClampLow, uint8_t ClampHigh, uint8_t ClampMax, uint16_t *pFiringSpeed, FIRINGSPEED *pFiringState);
static AM_STATUS InitializeAsaForceToSpeedTable(uint8_t AsaLow, uint8_t AsaHigh, uint8_t AsaMax);
static AM_STATUS AsaUpdateFireStateGetFiringSpeed(FIRINGSPEED FiringSpeedState, uint16_t *pFiringSpeed);
static uint16_t GetSpeedFromASATable(float32_t Force);
static EGIA_IPROFILE_TYPE EGutil_GetIprofIndex(DEVICE_ID_ENUM ReloadId);
void EGUtil_RotationConfigStop(uint8_t *pRotState, Handle *const pMe);
void EGUtil_ProcessRotationConfig(QEvt const * const e, Handle *const pMe, uint8_t *pRotState);
void EGUtil_ProcessRotationInProgress(QEvt const * const e, Handle *const pMe, uint8_t *pRotState);
void EGUtil_ProcessRotationDebounce(QEvt const * const e, Handle *const pMe, uint8_t *pRotState);
void EGUtil_ProcessRotationStopped(QEvt const * const e, Handle *const pMe, uint8_t *pRotState);
bool EGUtil_IsRotationConfigRequested( Handle *const pMe);
KEY_ID EGUtil_GetRotationKeyId( QEvt const * const e);
bool EGUtil_isRotationKeyPressed( QEvt const * const e);
bool EGUtil_IsRotationKeyEnabled(KEY_ID KeyId, Handle *const pMe);
ROTAITON_REQ_DIR EGUtil_GetRotateDirection(uint16_t KeyState);
void EGUtil_RotationTransToConfig(Handle *const pMe, uint8_t *pRotState);
bool EGUtil_IsMultiKeyPressed(uint16_t KeyState);


/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Display Screens after successful clamp test
 *
 * \details This function displays the Ready Screens based on type of RELOAD attached.
 *           NON INTELLIGENT RELOAD: Ready screen
 *           SULU RELOAD           : Ready screen
 *           MULU with Catridge    : Ready Screen
 *           MULU without catridge : Insert Cartridge Screen
 *
 * \param   me  - Pointer to local data store
 *
 * \b Returns void
 *
 * ========================================================================== */
static void DisplayReloadScreens_ClampTestPass(EGIA *const me)
{
    APP_EGIA_DATA *pEgia;
    uint8_t ReloadFireCount;

    pEgia = EGIA_GetDataPtr();
    DeviceMem_READ(ME->Reload, FireCount, ReloadFireCount);

    do
    {
        /// \todo 2/2/2022, Used clamshell status is updated with AM_DEVICE_ACCESSFAIL, if any update new status from Platform need to update Clamshell.Status condition below
        if ((pEgia->ReloadType == RELOAD_MULUINTELLIGENT & \
                ME->Reload.Status == AM_DEVICE_CONNECTED) && \
                (ME->Cartridge.Status == AM_DEVICE_CONNECTED) && \
                (ME->Clamshell.Status == AM_DEVICE_CONNECTED))
        {
            IntReloadScreenProgress(RELOAD_INT_CONNECT, pEgia->ReloadLen,pEgia->ReloadCartColor,false,false);
            /// \todo 2/2/2002, Battery Sufficient status is not available from Platform. Check Battery Sufficient to play Ready tone
            Signia_PlayTone(SNDMGR_TONE_READY);
            /// \todo 2/2/2022, Play Low Battery Tone if Batery charge is Low
            break;
        }
        /* Reload Connected is of MULU Type and Reload is communicating on OneWire, Cartridge is not connected or lost communication */
        if ((pEgia->ReloadType == RELOAD_MULUINTELLIGENT && \
                ME->Reload.Status == AM_DEVICE_CONNECTED) && \
                (ME->Cartridge.Status == AM_DEVICE_DISCONNECTED))
        {
            Log(DBG, "ClampTest: Insert Cartridge");
            Gui_Insert_Cartridge_Screen();
            break;
        }

        /* Reload is Non-Intelligent */

        /// \todo 2/2/2022, Used clamshell status is updated with AM_DEVICE_ACCESSFAIL, if anyupdate new status from Platform need to update Clamshell.Status condition below

        if ((pEgia->ReloadType == RELOAD_NONINTELLIGENT) && \
                (ME->Clamshell.Status == AM_DEVICE_CONNECTED))
        {
            //Gui_Ready_Non_Int_Reload_Screen();
            NonIntReloadScreenProgress(RELOAD_NON_INT_CONNECT,NOT_USED,false,false);
            /// \todo 2/2/2002, Battery Sufficient status is not available from Platform. Check Battery Sufficient to play Ready tone
            Signia_PlayTone(SNDMGR_TONE_READY);
            break;
        }

        /* Reload is SULU */

        /// \todo 2/2/2022, Used clamshell status is updated with AM_DEVICE_ACCESSFAIL, if anyupdate new status from Platform need to update Clamshell.Status condition below
        if ((pEgia->ReloadType == RELOAD_SULUINTELLIGENT) && \
                (ME->Clamshell.Status == AM_DEVICE_CONNECTED) &&\
                (!ME->Clamshell.ClamshellEOL) && (ReloadFireCount == 0))
        {
            IntReloadScreenProgress(RELOAD_INT_CONNECT, pEgia->ReloadLen,pEgia->ReloadCartColor,false,false);
            /// \todo 2/2/2002, Battery Sufficient status is not available from Platform. Check Battery Sufficient to play Ready tone
            Signia_PlayTone(SNDMGR_TONE_READY);
            break;
        }
    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Updates the Cartridge connection status and Cartridge type
 *
 * \details This function processes Cartridge connection events, updating
 *          local data and displays as appropriate. This function also checks the
 *          Cartridge Compatiblity with the connected MULU Reload.
 *
 * \param   pMe  - Pointer to local data store
 *
 * \b Returns void
 *
 * ========================================================================== */
static void ProcessCartridge(Handle *const pMe)
{
    uint8_t CartridgeFireCount;  /* Cartridge used status */
    uint8_t MuluFireCount;       /* MULU Reload Fire count */
    APP_EGIA_DATA *pEgia;

    /* initialize */

    pEgia = EGIA_GetDataPtr();
    pMe->Cartridge.Status = AM_DEVICE_ACCESS_FAIL;
    pEgia->CartridgeLen = DEVICE_ID_UNKNOWN;

    do
    {
        /* check Handle/Adapter/reload/Clamshell status */
        if ( !( ( AM_DEVICE_CONNECTED == pMe->Handle.Status )    &&
                ( AM_DEVICE_CONNECTED == pMe->Clamshell.Status ) &&
                ( AM_DEVICE_CONNECTED == pMe->Adapter.Status )   &&
                ( AM_DEVICE_CONNECTED == pMe->Reload.Status )
              ) )
        {
            Log(DBG,"Cartridge: Error in one of the device");
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            break;
        }

        /* check Cartridge authentication */
        /// \todo 04/17/2022 CPK - Move all Authenticated variables to respective structure (Handle,Adapter,Reload etc..)
        if ( (AUTHENTICATION_SUCCESS != pEgia->CartridgeAuthenticated) )
        {
            Log(DBG,"Cartridge: Failed Authentication ");
            Gui_MULUCartridgeErrorWarning_Screen(CartridgeFireCount);
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            break;
        }

        /* check if Cartridge already used */
        DeviceMem_READ(pMe->Cartridge, FireCount, CartridgeFireCount);
        if( CARTRIDGE_USED == CartridgeFireCount)
        {
            Log(DBG,"EGIAUtil: Used Cartridge");
            /* Display used cartridge screen and play caution tone */
            Gui_Used_Cartridge_Screen(CartridgeFireCount);
            Signia_PlayTone(SNDMGR_TONE_CAUTION);
            break;
        }

        /* check cartridge compatibility for MULU reload only */
        if ((RELOAD_MULUINTELLIGENT == pEgia->ReloadType) && (!IsCartridgeCompatible(pMe)))
        {
            /* Cartridge not compatible - Display warning screen and play fault tone Req ID:326462*/
            Gui_MULUCartridgeErrorWarning_Screen(CartridgeFireCount);
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            pEgia->IsCartridgeCompatible  = false;
            break;
        }

        if (RELOAD_MULUINTELLIGENT == pEgia->ReloadType)
        {
            DeviceMem_READ(pMe->Cartridge, ReloadColor, pEgia->CartridgeColor);

            /* We have compatible Cartridge - Save the received Cartridge length */
            pEgia->CartridgeLen = pMe->Cartridge.DevID;
            /* Read Mulu fire counts */
            DeviceMem_READ(pMe->Reload, FireCount, MuluFireCount);
            Signia_PlayTone(SNDMGR_TONE_READY);
            /* update Cartridge connection status to connected */
            pMe->Cartridge.Status = AM_DEVICE_CONNECTED;
            /* Cartridge is Compatible */
            pEgia->IsCartridgeCompatible  = true;
        }
    } while ( false ) ;
}

/* ========================================================================== */
/**
 * \brief   Perform the MULU Reload - Cartridge compatiblity test
 *
 * \details This function Performs the compatiblity of the connected cartridge
 *          with the MULU Reload and returns the compatibility status.
 *
 * \param   pMe  - Pointer to local data store
 *
 * \return  bool - True for success and False for Failed compatibility test
 *
 * ========================================================================== */
static bool IsCartridgeCompatible(Handle *const pMe)
{
    uint8_t index;
    bool CartridgeCompatible;

    CartridgeCompatible = false;
    /* scan the compatibility list to check if we have a compatible cartridge */
    for (index = 0x0; index < NO_OF_RELOAD_TYPES; index++)
    {
        if (MuluCartridgeCompatiblity[index].MuluId == pMe->Reload.DevID)
        {
            if (MuluCartridgeCompatiblity[index].CompatibleCartridgeId == pMe->Cartridge.DevID)
            {
                /* we have a compatible cartridge - return true for success*/
                CartridgeCompatible = true;
            }
            /* break after checking compatibility */
            break;
        }
    }
    return CartridgeCompatible;
}

/* ========================================================================== */
/**
 * \brief    Get the Clamp Force and Speed before starting the firing i.e. initial firing speed calculation
 *
 * \details  This function calculates the ClampForce speed either from ClampLow, ClampHigh, ClampMax(from SULU or MULU Cartridge) or from pre-defined default specification Force to Speed Table
 *           Provides FireSpeed and Fire Speed State
 *
 * \param    pMe     - Me pointer
 * \param    ClampLow     - ClampLow value read from SULU or Mulu Cartridge
 * \param    ClampHigh    - ClampHigh value read from SULU or Mulu Cartridge
 * \param    ClampMax     - ClampMax value read from SULU or Mulu Cartridge
 * \param    pFiringSpeed - pointer to Firing Speed
 * \param    pFiringState - pointer to Firing Speed State
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK - No error
 *
 * ============================================================================== */
static AM_STATUS GetAsaClampForceSpeed(Handle *const pMe, uint8_t ClampLow, uint8_t ClampHigh, uint8_t ClampMax, uint16_t *pFiringSpeed, FIRINGSPEED *pFiringState)
{
    uint16_t FiringSpeedRPM;
    FIRINGSPEED FiringState;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    if ((ClampLow != 0) && (ClampHigh != 0) && (ClampMax  != 0))
    {
        /* Update Firing Speed Range from Cartridge EEPROM */
        ForceToSpeedTable[FIRINGSPEED_FAST].FiringForce = ClampLow;
        ForceToSpeedTable[FIRINGSPEED_MEDIUM].FiringForce = ClampHigh;
        ForceToSpeedTable[FIRINGSPEED_SLOW].FiringForce = ClampMax;
    }
    else
    {
        ForceToSpeedTable[FIRINGSPEED_FAST].FiringForce = CLAMPINGFORCE_RANGE_1;
        ForceToSpeedTable[FIRINGSPEED_MEDIUM].FiringForce = CLAMPINGFORCE_RANGE_2;
        ForceToSpeedTable[FIRINGSPEED_SLOW].FiringForce = CLAMPINGFORCE_RANGE_3;
    }

    // Update the initial firing speed

    if (pEgia->MaxClampForce <=  CLAMPINGFORCE_RANGE_1)
    {
        FiringSpeedRPM = FIRING_SPEED_FAST_VALUE;
        FiringState = FIRINGSPEED_FAST;
    }
    else if (pEgia->MaxClampForce <=  CLAMPINGFORCE_RANGE_2)
    {
        FiringSpeedRPM = FIRING_SPEED_MEDIUM_VALUE;
        FiringState = FIRINGSPEED_MEDIUM;
    }
    else if (pEgia->MaxClampForce <= CLAMPINGFORCE_RANGE_3)
    {
        FiringSpeedRPM = FIRING_SPEED_SLOW_VALUE;
        FiringState = FIRINGSPEED_SLOW;
    }
    else
    {
        /* Excessive Load, Do not start Firing */
        FiringSpeedRPM = 0;
        FiringState = FIRINGSPEED_LAST;
    }
    Log(DBG, "EGIAUtil: Fire Motor Speed= %d Firing Mode = %d, MaxClampForce= %3.2f lbs, CurrentADC= %u Counts, ForceLBS=%3.2f lbs",
        FiringSpeedRPM, FiringState, pEgia->MaxClampForce, pEgia->SGForce.Current, pEgia->SGForce.ForceInLBS);
    *pFiringSpeed = FiringSpeedRPM;
    *pFiringState =  FiringState;

    return AM_STATUS_OK;
}

/* ========================================================================== */
/**
 * \brief    Initialize Asa Force to Speed Table
 *
 * \details  This function updates the ASA Force To Speed table either from SULU/MULU cartridge read values or uses the default values
 *
 * \param    AsaLow     - AsaLow value read from SULU or Mulu Cartridge
 * \param    AsaHigh    - AsaHigh value read from SULU or Mulu Cartridge
 * \param    AsaMax     - AsaMax value read from SULU or Mulu Cartridge
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK - No error
 *
 * ========================================================================== */
static AM_STATUS InitializeAsaForceToSpeedTable(uint8_t AsaLow, uint8_t AsaHigh, uint8_t AsaMax)
{
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    /*Load Firing Speed from Default table */
    if ((AsaLow != 0) && (AsaHigh != 0) && (AsaMax != 0))
    {
        ForceToSpeedTable[FIRINGSPEED_FAST].FiringForce = AsaLow;
        ForceToSpeedTable[FIRINGSPEED_MEDIUM].FiringForce = AsaHigh;
        ForceToSpeedTable[FIRINGSPEED_SLOW].FiringForce = AsaMax;
    }
    else
    {
        ForceToSpeedTable[FIRINGSPEED_FAST].FiringForce = FIRINGFORCE_RANGE_1;
        ForceToSpeedTable[FIRINGSPEED_MEDIUM].FiringForce = FIRINGFORCE_RANGE_2;
        ForceToSpeedTable[FIRINGSPEED_SLOW].FiringForce = FIRINGFORCE_RANGE_3;
        pEgia->FiringMaxForceRead = MAX_FORCE_SG;
    }
    /* Update the Firing Speed */
    ForceToSpeedTable[FIRINGSPEED_FAST].FiringSpeed = FIRING_SPEED_FAST_VALUE;
    ForceToSpeedTable[FIRINGSPEED_MEDIUM].FiringSpeed = FIRING_SPEED_MEDIUM_VALUE;
    ForceToSpeedTable[FIRINGSPEED_SLOW].FiringSpeed = FIRING_SPEED_SLOW_VALUE;

    return AM_STATUS_OK;
}

/* ========================================================================== */
/**
 * \brief    Update the Firing Speed State and gives the FiringSpeed
 *
 * \details  Update the Firing Speed State and gives the FiringSpeed
 *
 * \param    FiringSpeedState - Firing Speed State
 * \param    pFiringSpeed     - pointer to Firing Started
 *
 * \return  AM_STATUS - Function return Status
 * \retval  AM_STATUS_OK - No error
 * \retval  AM_STATUS_ERROR - Error
 *
 * ========================================================================== */
static AM_STATUS AsaUpdateFireStateGetFiringSpeed(FIRINGSPEED FiringSpeedState, uint16_t *pFiringSpeed)
{
    AM_STATUS Status;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();
    Status = AM_STATUS_ERROR;

    do
    {
        if ((NULL == pFiringSpeed) || (FIRINGSPEED_LAST == FiringSpeedState))
        {
            break;
        }

        *pFiringSpeed = ForceToSpeedTable[FiringSpeedState].FiringSpeed;
        pEgia->AsaInfo.FiringState = FiringSpeedState;
        pEgia->AsaInfo.FiringRpm = ForceToSpeedTable[FiringSpeedState].FiringSpeed;;
        Status = AM_STATUS_OK;
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Function to Get the Speed and Firing state from Force Range Change
 * \details Function to get the Firing Speed and update the Firing State FAST/MED/SLOW
 *
 * \param   Force - Force Value
 *
 * \return  Speed
 *
 * ========================================================================== */
static uint16_t GetSpeedFromASATable(float32_t Force)
{
    uint16_t Speed;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    // Check 0-65Lbs for FAST Speed
    if ((Force >= FIRINGFORCE_RANGE_0) && (Force <= FIRINGFORCE_RANGE_1))
    {
        Speed = ForceToSpeedTable[FIRINGSPEED_FAST].FiringSpeed;
        pEgia->AsaInfo.FiringState = FIRINGSPEED_FAST;
    }
    // Check 65-80Lbs for MEDIUM Speed
    else  if ((Force > FIRINGFORCE_RANGE_1) && (Force <= FIRINGFORCE_RANGE_2))
    {
        Speed = ForceToSpeedTable[FIRINGSPEED_MEDIUM].FiringSpeed;
        pEgia->AsaInfo.FiringState = FIRINGSPEED_MEDIUM;
    }
    // Check 81-133Lbs for SLOW Speed
    else  if ((Force > FIRINGFORCE_RANGE_2) && (Force <= pEgia->FiringMaxForceRead))
    {
        Speed = ForceToSpeedTable[FIRINGSPEED_SLOW].FiringSpeed;
        pEgia->AsaInfo.FiringState = FIRINGSPEED_SLOW;
    }
    else
    {
        ; //Do Nothing
    }
    pEgia->AsaInfo.FiringRpm = Speed;
    return Speed;
}

/* ========================================================================== */
/**
 * \brief   Function to Get the Current Profile based on Reload ID
 * \details function returns the Current profile from the Reload to current profile
 *              map based on connected Reload ID
 *
 * \param   ReloadId - Connected Reload ID
 *
 * \return  Current Profile Index
 *
 * ========================================================================== */
static EGIA_IPROFILE_TYPE EGutil_GetIprofIndex(DEVICE_ID_ENUM ReloadId)
{
    uint8_t Index;
    EGIA_IPROFILE_TYPE MapIndex;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();
    do
    {
        // check if the Reload type is NonIntelligent
        if ( RELOAD_NONINTELLIGENT == pEgia->ReloadType)
        {
            MapIndex = EGIA_IPROF_TABLE_INDEX_NOID_RELOAD;
            break;
        }

        for(Index = 0; Index < sizeof(ReloadMap); Index++)
        {
            // loop through the Map table for Reload match. If Match read the index from table
            if ( ReloadMap[Index].DevId == ReloadId)
            {
                MapIndex = ReloadMap[Index].Index;
                break;
            }
            else
            {
                // defensive coding. if no Reload matched in Table assign index to Reload_NOID as default
                MapIndex = EGIA_IPROF_TABLE_INDEX_NOID_RELOAD;
            }
        }
    } while (false);
    return MapIndex;
}


/* ========================================================================== */
/**
 * \brief  Checks if any rotation key is pressed
 *
 * \details checks the signal received is a Rotation key press signal
 *
 * \param   e - pointer to signal
 *
 * \returns Rotation key press status. True if pressed else false
 *
* ========================================================================== */
bool EGUtil_isRotationKeyPressed( QEvt const * const e)
{
    bool Retval;
    Retval = false;
    if ( P_LATERAL_LEFT_DOWN_PRESS_SIG == e->sig || P_LATERAL_RIGHT_UP_PRESS_SIG == e->sig || \
            P_LATERAL_LEFT_UP_PRESS_SIG == e->sig || P_LATERAL_RIGHT_DOWN_PRESS_SIG == e->sig)
    {
        // signal is a rotation key press
        Retval = true;
    }
    return Retval;
}

/* ========================================================================== */
/**
 * \brief Get rotation key id based on the Signal
 *
 * \details This function returns the KeyId based on the Rotation key signal received
 *
 * \param   e - pointer to signal
 *
 * \returns Key Id
 *
* ========================================================================== */
KEY_ID EGUtil_GetRotationKeyId( QEvt const * const e)
{
    KEY_ID KeyId;

    if ( P_LATERAL_LEFT_UP_PRESS_SIG == e->sig || P_TOGGLE_UP_RELEASE_SIG == e->sig )
    {
        KeyId = LATERAL_LEFT_UP;
    }
    else if ( P_LATERAL_RIGHT_DOWN_PRESS_SIG == e->sig || P_LATERAL_RIGHT_DOWN_RELEASE_SIG == e->sig)
    {
        KeyId = LATERAL_RIGHT_DOWN;
    }
    else if ( P_LATERAL_RIGHT_UP_PRESS_SIG == e->sig || P_LATERAL_RIGHT_UP_RELEASE_SIG == e->sig)
    {
        KeyId = LATERAL_RIGHT_UP;
    }
    else if ( P_LATERAL_LEFT_DOWN_PRESS_SIG == e->sig || P_LATERAL_LEFT_DOWN_RELEASE_SIG == e->sig)
    {
        KeyId = LATERAL_LEFT_DOWN;
    }
    else
    {
        KeyId = KEY_NONE;
    }
    return KeyId;
}

/* ========================================================================== */
/**
 * \brief   Checks if the Rotation configuration is requested
 *
 * \details The functions checks the key state if the Rotation configuration key pattern is present
 *
 * \param   pMe: Handle Data store
 *
 * \returns Rotation configuration request status.
 * \retval True  - if rotation configuration requested
 * \retval False - if rotation configuration not requested
 *
* ========================================================================== */
bool EGUtil_IsRotationConfigRequested( Handle *const pMe)
{
    bool Retval;
    Retval = false;
    if ( ( LEFT_ROTATION_CONFIG_KEYSEQ == pMe->KeyState ) || ( RIGHT_ROTATION_CONFIG_KEYSEQ == pMe->KeyState ) )
    {
        Retval = true;
    }
    return Retval;
}

/* ========================================================================== */
/**
 * \brief   Checks if the requested Roation keys are enabled
 *
 * \details the function checks in the existing rotation configuration the requested
 *              rotation key function is enabled
 *
 * \param   KeyId   - Key ID
 * \param   pMe - Individual Key State
 *
 * \b Returns Rotation enable status of the requested rotation key
 *
* ========================================================================== */
bool EGUtil_IsRotationKeyEnabled(KEY_ID KeyId, Handle *const pMe)
{

    bool Retval;
    Retval = false;
    if( LATERAL_LEFT_UP == KeyId || LATERAL_LEFT_DOWN == KeyId)
    {
        Retval = pMe->RotationConfig.LeftRotateEnabled;
    }
    else if ( LATERAL_RIGHT_UP == KeyId || LATERAL_RIGHT_DOWN == KeyId)
    {
        Retval = pMe->RotationConfig.RightRotateEnabled;
    }
    return Retval;
}

/* ========================================================================== */
/**
 * \brief  gets the requested rotation direction
 *
 * \details the function determines the direction of rotation based on the key pressed
 *
 * \param   KeyState - Individual Key State
 *
 * \b Returns void
 *
* ========================================================================== */
ROTAITON_REQ_DIR EGUtil_GetRotateDirection(uint16_t KeyState)
{
    ROTAITON_REQ_DIR RotateDir;

    RotateDir = ROTATE_REQ_INVALID;
    do
    {
        if ( (KeyState & ROTATE_CW_KEYMASK) && !(KeyState & (~ROTATE_CW_KEYMASK)) )
        {
            RotateDir = ROTATE_REQ_CW;
        }
        else if (KeyState & ROTATE_CCW_KEYMASK && !(KeyState & (~ROTATE_CCW_KEYMASK)))
        {
            RotateDir = ROTATE_REQ_CCW;
        }
    } while (false);
    return RotateDir;
}

/* ========================================================================== */
/**
 * \brief  Checks if multiple keys are pressed
 *
 * \details the function checks if multiple keys are pressed based on the key status
 *
 * \param   KeyState - current key states
 *
 * \b Returns Multiple key press status
 * \retval  True if multiple keys pressed
 * \retval  False if single key is pressed
 *
* ========================================================================== */
bool EGUtil_IsMultiKeyPressed(uint16_t KeyState)
{
    uint8_t KeyPressCount;
    uint8_t i;
    bool Retval;

    KeyPressCount = 0;
    for( i=0; i<KEY_COUNT; i++)
    {
        if ( KeyState & 1<<i )
        {
            KeyPressCount++;
        }
    }
    Retval = (KeyPressCount > 1)? true : false ;
    return Retval;
}

/* ========================================================================== */
/**
 * \brief Dispays the Rotation configuration screens
 *
 * \details based on the input the function displays the appropriate rotation configuration screens
 *
 * \param   pMe - pointer to Handle Data Store
 * \param   ScreenType - Indicates either countdown screen or Key disabled screens
 *
 * \b Returns void
 *
* ========================================================================== */
void EGUtil_Display_RotateConfigScreens(Handle *const pMe, ROTATION_SCREENTYPE ScreenType)
{
    uint8_t RotConfigCounter;

    switch(ScreenType)
    {
        // Screen type is "Rotate key dissabled"
        case ROTATION_SCREENTYPE_ROTATEKEY_DISSABLED:
            // check if both key dissabled
            if ( !pMe->RotationConfig.LeftRotateEnabled && !pMe->RotationConfig.RightRotateEnabled)
            {
                // Requested Left Rotation configuration
                if ( pMe->KeyState & LEFT_ROTATION_CONFIG_KEYSEQ )
                {
                    // Display Acitvate left with Deactivated right screen
                    Gui_RotateActivateRight_ScreenSet();
                }
                else // Requested right rotation configuration
                {
                    // Display Activate Right with Deactivated left screen
                    Gui_RotateActivateLeft_ScreenSet();

                }
            }
            // if left rotation keys are dissbled and requested key press is on left side
            else if ( !pMe->RotationConfig.LeftRotateEnabled && (pMe->KeyState & LEFT_ROTATION_CONFIG_KEYSEQ) )
            {
                // Display Activated Right Activate Left screen
                Gui_RotateActivateRightActive_ScreenSet();
            }
            // if right rotation keys are dissbaled and requested key press is on right side
            else if( !pMe->RotationConfig.RightRotateEnabled && (pMe->KeyState & RIGHT_ROTATION_CONFIG_KEYSEQ) )
            {
                // Display Activated Left Activate Right screen
                Gui_RotateActivateLeftActive_ScreenSet();
            }
            else
            {
                /*Do nothing*/
                Log(DBG, "EGIA: Rotation Config screen display failed, ScreenType: KEY_DISSABLED");
            }

            break;

        case ROTATION_SCREENTYPE_ROTATIONCONFIG_COUNTER:

            RotConfigCounter = pMe->RotationConfig.RotationConfigCounter;

            if(RotConfigCounter)
            {
                if ( pMe->KeyState & LEFT_ROTATION_CONFIG_KEYSEQ )
                {
                    Gui_RotationScreen(RotConfigCounter,0,pMe->RotationConfig.LeftRotateEnabled,pMe->RotationConfig.RightRotateEnabled);
                }
                else
                {
                    Gui_RotationScreen(0,RotConfigCounter,pMe->RotationConfig.LeftRotateEnabled,pMe->RotationConfig.RightRotateEnabled);
                }
            }
            else
            {
                if(!pMe->RotationConfig.LeftRotateEnabled && !pMe->RotationConfig.RightRotateEnabled)
                {
                    Gui_RotationScreen(0,0,pMe->RotationConfig.LeftRotateEnabled,pMe->RotationConfig.RightRotateEnabled);
                }
                else
                {
                    if ( pMe->KeyState & LEFT_ROTATION_CONFIG_KEYSEQ )
                    {
                        LeftRotationConfigScreens[pMe->RotationConfig.LeftRotateEnabled][pMe->RotationConfig.RightRotateEnabled]();
                    }
                    else
                    {
                        RightRotationConfigScreens[pMe->RotationConfig.LeftRotateEnabled][pMe->RotationConfig.RightRotateEnabled]();
                    }
                }
            }

            break;

        default:
            break;
    }
}

/* ========================================================================== */
/**
 * \brief Transition to Rotation configuration state
 *
 * \details Allows the entry to Rotation configuration state if there is
 *              No Clamshell Error
 *              No Clamshell EOL
 *              No Handle EOL
 *
 * \param   pMe - pointer to Handle Data Store
 * \param   pRotState - pointer to the Rotation State
 *
 * \b Returns void
 *
* ========================================================================== */
void EGUtil_RotationTransToConfig(Handle *const pMe, uint8_t *pRotState)
{
    do
    {
        if(pMe->ActiveFaultsInfo.IsErrShell || pMe->Clamshell.ClamshellEOL || pMe->Handle.HandleEOL)
        {
            Log(DEV, "Handle - Ignore Rotation Configuration req");
            Log(DEV, "IsErrShell = %d, IsUsedClamshell = %d, IsHandleEOL = %d", pMe->ActiveFaultsInfo.IsErrShell, pMe->Clamshell.ClamshellEOL, pMe->Handle.HandleEOL);
            EGUtil_RotationConfigStop(pRotState,pMe);
            break;
        }
        // Arm Rotation Timer for 1sec and go to Rotation configuration
        AO_TimerArm(&pMe->RotationConfigTimer, ROTATION_CONFIG_SCREEN_COUNTDOWNTIME, NULL);
        pMe->RotationConfig.RotationConfigCounter = MAX_ROTATION_CONFIG_COUNTER;
        *pRotState = ROTATION_CONFIG;
    } while ( false );

}

/* ========================================================================== */
/**
 * \brief   Rotation Stopped state
 *
 * \details This state checks for the input signal and trnasistion to either
 *              100ms Debounce state or Roation configuration state or no action
 *              based on Keystate and stored rotation configuration
 *
 *
 * \param   e - pointer to the event
 * \param   pRotState - pointer to the Rotation State
 * \param   pMe - pointer to Handle Data Store
 *
 * \b Returns void
 *
* ========================================================================== */
void EGUtil_ProcessRotationStopped(QEvt const * const e, Handle *const pMe, uint8_t *pRotState)
{
    KEY_ID KeyId;
    do
    {
        //check for signal- should be one of rotation key or a Roation timer timeout signal
        BREAK_IF(!EGUtil_isRotationKeyPressed(e) && ROTATION_TIMER_TIMEOUT_SIG != e->sig);
        if ( ROTATION_TIMER_TIMEOUT_SIG == e->sig)
        {
            if ( EGUtil_IsRotationConfigRequested(pMe) )
            {
                // Rotation timer timeout and if a rotation config request is detected.
                EGUtil_RotationTransToConfig(pMe,pRotState);
            }
            else
            {
                // Stop Rotation configuration
                EGUtil_RotationConfigStop(pRotState,pMe);
            }
            break;
        }

        KeyId = EGUtil_GetRotationKeyId(e);

        if(!EGUtil_IsRotationKeyEnabled( KeyId, pMe))
        {
            // Display Activate Roation key
            EGUtil_Display_RotateConfigScreens(pMe, ROTATION_SCREENTYPE_ROTATEKEY_DISSABLED);
            // Start Timer 2sec
            AO_TimerArm(&pMe->RotationConfigTimer, ROTATION_CONFIG_2SEC_TIMEOUT, NULL);
            Log(DBG,"RotationStopped:Rotate key dissabled. Screen updated TimerArmed for 2sec");
            break;
        }
        // Arm Rotation Timer for 100ms
        AO_TimerArm(&pMe->RotationConfigTimer, ROTATION_DEBOUNCE_TIME, NULL);
        Log(DBG,"EGIA: Rotation State: Stopped Transition to: 100ms_Debounce, KeyState: 0x%x",pMe->KeyState);
        *pRotState = ROTATION_DEBOUNCE;
    } while ( false );
}



/* ========================================================================== */
/**
 * \brief   Debounce state
 *
 * \details This states waits for 100ms and checks the keystate. Proceeds to either
 *              Roation In progress state or Rotation configuration state or Stopped state
 *              based on the current keystate
 *
 * \param   e - pointer to the event
 * \param   pRotState - pointer to the Rotation State
 * \param   pMe - pointer to Handle Data Store
 *
 * \b Returns void
 *
* ========================================================================== */
void EGUtil_ProcessRotationDebounce(QEvt const * const e, Handle *const pMe, uint8_t *pRotState)
{
    ROTAITON_REQ_DIR RoationReq;
    APP_EGIA_DATA *pEgia;

    do
    {
        if ( ROTATION_TIMER_TIMEOUT_SIG == e->sig )
        {
            RoationReq = EGUtil_GetRotateDirection(pMe->KeyState);
            if ( ROTATE_REQ_INVALID == RoationReq )
            {
                if ( EGUtil_IsRotationConfigRequested(pMe) )
                {

                    Log(DBG,"EGIA: Rotation State: Debounce Transition to: RotationConfiguration, KeyState: 0x%x",pMe->KeyState);
                    EGUtil_RotationTransToConfig(pMe,pRotState);
                    break;
                }
                Log(DBG,"EGIA: Rotation State: Debounce Transition to: Stopped, KeyState: 0x%x",pMe->KeyState);
                *pRotState = ROTATION_STOPPED;
            }
            else
            {
                pEgia = EGIA_GetDataPtr();
                if ( pEgia->RotateAllowed )
                {
                    Log(DBG,"EGIA: Rotation State: Debounce, Transition to: RotationInProgress, KeyState: 0x%x",pMe->KeyState);
                    *pRotState = ROTATION_INPROGRESS;
                    /// \todo 03/21/2022 KA: revisit how to set the speed based on fully clamped condition
                    EGutil_UpdateRotation(MOTOR_START,
                                          (int32_t)RotateMotorPosition[RoationReq],
                                          ROTATE_MOTOR_SHAFT_RPM);
                }
                else
                {
                    Log(DBG,"EGIA: Rotation State: Debounce Transition to: Stopped, KeyState: 0x%x",pMe->KeyState);
                    *pRotState = ROTATION_STOPPED;
                }
            }
        }
    } while ( false );
}

/* ========================================================================== */
/**
 * \brief  Rotation in progress state
 *
 * \details Adapter is in Rotation state. based on the input stops rotation and goes to either
 *              Stopped state or rotation configuration state
 *
 * \param   e - pointer to the event
 * \param   pRotState - pointer to the Rotation State
 * \param   pMe - pointer to Handle Data Store
 *
 * \b Returns void
 *
* ========================================================================== */
void EGUtil_ProcessRotationInProgress(QEvt const * const e, Handle *const pMe, uint8_t *pRotState)
{
    KEY_ID KeyId;
    ROTAITON_REQ_DIR RoationReq;
    APP_EGIA_DATA *pEgia;
	
    do
    {
        // Process any Rotation key press signal in Rotation in progress state
        if ( P_LATERAL_LEFT_DOWN_PRESS_SIG == e->sig || P_LATERAL_LEFT_UP_PRESS_SIG== e->sig || \
                P_LATERAL_RIGHT_UP_PRESS_SIG == e->sig  || P_LATERAL_RIGHT_DOWN_PRESS_SIG == e->sig)
        {
            KeyId = EGUtil_GetRotationKeyId(e);
            if((pMe->KeyState & (~ROTATE_CW_KEYMASK)) && (pMe->KeyState & (~ROTATE_CCW_KEYMASK)))
            {
                // Multiple key press detected but not the same direction rotation request
                EGUtil_StopRotArtOnMultiKey( KeyId, pMe->KeyState );
            }
        }
        if ( EGUtil_IsRotationConfigRequested(pMe) )
        {
            Log(DBG,"EGIA: Rotation State: RotationInProgress Transition to: RotationConfiguration, KeyState: 0x%x",pMe->KeyState);
            EGUtil_RotationTransToConfig(pMe,pRotState);
            break;
        }
        if(P_ROTATE_STOP_SIG == e->sig)
        {
            if(EGUtil_IsMultiKeyPressed(pMe->KeyState))
            {
                Log(DBG,"EGIA: Rotation State: RotationInProgress Transition to: Stopped, KeyState: 0x%x",pMe->KeyState);
                *pRotState = ROTATION_STOPPED;
            }
            else
            {
                pEgia = EGIA_GetDataPtr();
                RoationReq = EGUtil_GetRotateDirection(pMe->KeyState);
                if ( ROTATE_REQ_INVALID == RoationReq )
                {
                    Log(DBG,"EGIA: Rotation State: RotationInProgress Transition to: Stopped, KeyState: 0x%x",pMe->KeyState);
                    *pRotState = ROTATION_STOPPED;
                }
                else
                {
                    // Arm Rotation Timer for 100ms
                    AO_TimerArm(&pMe->RotationConfigTimer, ROTATION_DEBOUNCE_TIME, NULL);
                    *pRotState = ROTATION_DEBOUNCE;
                    Log(DBG,"EGIA: Rotation State: RotationInProgress Transition to: 100ms_Debounce, KeyState: 0x%x",pMe->KeyState);
                    pEgia->RotateAllowed = false;
                }
            }
        }
    } while ( false );
}

/* ========================================================================== */
/**
 * \brief Process Roation Configuration state
 *
 * \details checks for the Rotation configuraiton key sequence, Enables or dissables the Rotation Configuration for selected sides
 *
 * \param   e - pointer to the event
 * \param   pRotState - pointer to the Rotation State
 * \param   pMe - pointer to Handle Data Store
 *
 * \b Returns void
 *
* ========================================================================== */
void EGUtil_ProcessRotationConfig(QEvt const * const e, Handle *const pMe, uint8_t *pRotState)
{
    do
    {
        if ( ROTATION_TIMER_TIMEOUT_SIG == e->sig )
        {
            AO_TimerDisarm(&pMe->RotationConfigTimer);
            if ( (LEFT_ROTATION_CONFIG_KEYSEQ == pMe->KeyState) || (RIGHT_ROTATION_CONFIG_KEYSEQ == pMe->KeyState) )
            {
                if ( pMe->RotationConfig.RotationConfigCounter )
                {
                    //Display Rotation config screens with RotaionConfigConter value
                    AO_TimerArm( &pMe->RotationConfigTimer, ROTATION_CONFIG_SCREEN_COUNTDOWNTIME, NULL );
                    EGUtil_Display_RotateConfigScreens( pMe, ROTATION_SCREENTYPE_ROTATIONCONFIG_COUNTER );
                    pMe->RotationConfig.RotationConfigCounter--;
                }
                else
                {
                    //update rotation config structure
                    if (LEFT_ROTATION_CONFIG_KEYSEQ == pMe->KeyState)
                    {
                        pMe->RotationConfig.LeftRotateEnabled = !pMe->RotationConfig.LeftRotateEnabled;
                    }
                    else
                    {
                        pMe->RotationConfig.RightRotateEnabled = !pMe->RotationConfig.RightRotateEnabled;
                    }
                    EGUtil_Display_RotateConfigScreens( pMe, ROTATION_SCREENTYPE_ROTATIONCONFIG_COUNTER );

                    Log(DBG,"EGIA: Rotation State: RotationConfiguration Transition to: DisplayWait, KeyState: 0x%x",pMe->KeyState);
                    Signia_PlayTone(SNDMGR_TONE_READY);
                    *pRotState = ROTATION_DISPLAYWAIT;
                    // Start Timer 2sec
                    AO_TimerArm(&pMe->RotationConfigTimer, ROTATION_CONFIG_2SEC_TIMEOUT, NULL);
                }
                break;
            }
        }
        if(P_ROTATE_STOP_SIG == e->sig)
        {
            break;
        }
        Log(DBG,"EGIA: Rotation State: RotationConfiguration Transition to: Stopped, KeyState: 0x%x",pMe->KeyState);
        EGUtil_RotationConfigStop(pRotState,pMe);
    } while ( false );
}

/* ========================================================================== */
/**
 * \brief Stop Roation Configuration
 *
 * \details Set the Rotation state to Stopped, Restore the previous handle mode of operaiton screen,
 *          Stop any motor movement
 *
 * \param   pRotState - pointer to the Rotation State
 * \param   pMe - pointer to Handle Data Store
 *
 * \b Returns none
 *
* ========================================================================== */
void EGUtil_RotationConfigStop(uint8_t *pRotState, Handle *const pMe)
{
    *pRotState = ROTATION_STOPPED;
    //Normalize display
    L4_RestoreCopiedScreen();
    pMe->RotationConfig.IsScreenCaptured = false;
    //Stop Timer
    AO_TimerDisarm(&pMe->RotationConfigTimer);
}


/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Function to check the Clamshell, Adapter and Handle status during Reload recognition
 *
 * \details Checks for Clamshell, Adapter and Handle error during reload recognition and play the tone
 *
 * \param   me  - Pointer to local data
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_ReloadRecognitionStatus(EGIA *const me)
{
    uint8_t ClamshellStatusFlags;       /* Clamshell Status Read from Clamshell */
    uint16_t AdapterFireCount;          /* Adapter Fire Count Read from Adapter */
    uint16_t AdapterFireLimit;          /* Adapter Fire Limit Read from Adapter */
    uint16_t HandleProcedureCount;      /* Handle Procedure Count Read from Handle */
    uint16_t HandleProcedureLimit;      /* Handle Procedure Limit Read from Handle */
    uint16_t HandleFireCount;           /* Handle Fire Count Read from Handle */
    uint16_t HandleFireLimit;           /* Handle Fire Limit Read from Handle */
    uint8_t  ReloadFireCount;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    /* Read the Clamshell Status */
    DeviceMem_READ(ME->Clamshell, StatusFlags, ClamshellStatusFlags);

    /* Read the Adapter Fire Count &  Fire Limit */
    DeviceMem_READ(ME->Adapter, FireCount, AdapterFireCount);
    DeviceMem_READ(ME->Adapter, FireLimit, AdapterFireLimit);

    DeviceMem_READ(ME->Handle, FireLimit, pEgia->AdapterProcedureLimit);

    /* Read the Handle Procedure Count & Limit */
    DeviceMem_READ(ME->Handle, ProcedureCount, HandleProcedureCount);
    DeviceMem_READ(ME->Handle, ProcedureLimit, HandleProcedureLimit);

    /* Read the Handle Fire Count & Fire Limit */
    DeviceMem_READ(ME->Handle, FireCount, HandleFireCount);
    DeviceMem_READ(ME->Handle, FireLimit, HandleFireLimit);

    DeviceMem_READ(ME->Reload, FireCount, ReloadFireCount);

    do
    {
        /* play the Fault Tone if the HANDLE, CLAMSHELL, or ADAPTER is in an ERROR condition */
        if ((ME->Clamshell.Status == AM_DEVICE_ACCESS_FAIL) || (ME->Clamshell.ClamshellEOL) || ((!HNutil_IsAdapterPresentInDeviceList(pEgia->AdapterDeviceID)) &&
                (pEgia->AdapterProcedureLimit == 0)) || (HandleProcedureCount >= HandleProcedureLimit) || (HandleFireCount >= HandleFireLimit))
        {
            Log(DBG, "EGIAUtil: EGIA Clameshell or Adapter or Handle in Error state");
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            break;
        }
        /* play the Caution Tone, if a RELOAD is attached to an END_OF_LIFE HANDLE or Used CLAMSHELL
           and neither is in an ERROR  */
        /// \todo 18/03/2022 AR The Handle and Adapter Error Flag need to be Implement
        else if ((ME->ActiveFaultsInfo.IsFileSysErr || ME->ActiveFaultsInfo.IsAccelErr) || (ME->Clamshell.ClamshellEOL) ||
                 (ME->ActiveFaultsInfo.IsErrShell))
        {
            Log(DBG, "EGIAUtil: HANDLE or Used CLAMSHELL and neither is in an ERROR");
            Signia_PlayTone(SNDMGR_TONE_CAUTION);
            break;
        }
        else if((ReloadFireCount == 1) && ((RELOAD_SULUINTELLIGENT == pEgia->ReloadType)))
        {
            Log(DBG, "EGIAUtil: Used SULU Reload");
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            break;
        }
        // Add MULU Reload and Cartridge Check for EOL here
        else
        {
            Log(DBG, "No Error found during Reload recognition");
            break;
        }
    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Evaluates EGIA Firing Calibration based on MOTOR STOP signal and EGIA Adapter Tare Force
 *
 * \details This function checks the pre-condition to enter next state of EGIA adapter Calibration.
 *          Pre-Conditions:
 *              - Motor stop in position
 *              - EGIA Force Tare
 *              .
 *
 * \param   me  - Pointer to local storage
 * \param   e - Pointer to Event
 *
 * \return  bool - status
 * \retval  false - Pre-Condition failed
 * \retval  true - All Pre-Conditions Passed
 *
 * ========================================================================== */
bool EGutil_CalibrationNextStatePreCond(EGIA *const me, QEvt const *const e)
{
    bool Status;
    uint16_t ProcedureLimit;
    APP_EGIA_DATA *pEgia;
    QEVENT_MOTOR_STOP_INFO   *pStopInfo; // Pointer to motor stop info signal

    pEgia = EGIA_GetDataPtr();
    Status = false;
    pStopInfo = (QEVENT_MOTOR_STOP_INFO *)e;

    do
    {
        if ((pStopInfo->StopStatus & MOT_STOP_STATUS_IN_POS) && (!(ME->EgiaReload.Status == AM_DEVICE_CONNECTED)))
        {
            /// \todo 08/25/2021 NP - Signia_PlayTone with proper API call (Signia_xxxxx)
            pEgia->CalibrationStatus = CALIBRATION_SUCCESS;
            
            // Save the Calibration Status to the Handle ( for status variables data read )
            ME->Adapter.IsCalibrated = pEgia->CalibrationStatus;
            
            Log(DBG, "EGIAUtil: EGIA Firing Calibration Success");
        }
        else
        {
            /* Update Calbration Failed due to Reload or not */
            pEgia->CalibrationStatus = (ME->EgiaReload.Status == AM_DEVICE_CONNECTED) ? CALIBRATION_FAILED_RELOADCONNECTED : CALIBRATION_FAILED_MOTOR;
            
            // Save the Calibration Status to the Handle 
            ME->Adapter.IsCalibrated = pEgia->CalibrationStatus;
            
            /// \todo 07/02/2021 CPK Error handling to be done based on Stop return status
            Log(DBG, "EGIA Firing Calibration Failed");
            Log(REQ, "EGIA Calibration: Fire - Home failed stop status 0x%04X", pStopInfo->StopStatus);
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            break;
        }

        if ((AM_STATUS_OK == ME->Adapter.pHandle->pForceTare()))
        {
            /// \todo 09/24/2021 BS - Need to check for Handle EOL - API/Handle EEPROM data needed
            Log(DBG, "EGIAUtil: EGIA Adapter Tare Success");
            /// \todo 08/25/2021 NP - Signia_PlayTone with proper API call (Signia_xxxxx)

            do
            {
                // Check for Clamshell EOL
                if (ME->Clamshell.ClamshellEOL)
                {
                    // Play Caution Tone if Clamshell is Used Clamshell
                    Signia_PlayTone(SNDMGR_TONE_CAUTION);
                    break;
                }

                // Check for Error Shell
                if (ME->ActiveFaultsInfo.IsErrShell)
                {
                    // Play Fault Tone if Clamshell Error is SET
                    Signia_PlayTone(SNDMGR_TONE_FAULT);
                    break;
                }

                if (pEgia->BatteryLevel > BATTERY_RSOC_LOW)
                {
                    Signia_PlayTone(SNDMGR_TONE_ALL_GOOD);
                    break;
                }

                if (pEgia->BatteryLevel > BATTERY_RSOC_INSUFFICIENT)
                {
                    Signia_PlayTone(SNDMGR_TONE_LOW_BATTERY);
                    break;
                }

                Signia_PlayTone(SNDMGR_TONE_INSUFFICIENT_BATTERY);
            } while (false);

            ScreenAdapterCalibShowProgress(ADAPTER_CALIB_STATE_COMPLETED);

            /* Check for Clamshell EOL and show Request Reload Screen accordingly */
            if (ME->Clamshell.ClamshellEOL)
            {
                Gui_Request_Reload_Screen(true, pEgia->HandleProcRemaining, pEgia->AdapterProcRemaining);
            }
            else
            {
                Gui_Request_Reload_Screen(false, pEgia->HandleProcRemaining, pEgia->AdapterProcRemaining);
            }
        }
        else
        {
            // Set Procedure limit to zero
            ProcedureLimit = 0;
            DeviceMem_WRITE(ME->Adapter, ProcedureLimit, ProcedureLimit);
            Log(DBG, "EGIAUtil: EGIA Adapter Tare Fail");
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            Gui_AdapterErrorScreen();
            break;
        }

        Status = true;

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Process Clamp Test Full Open, Evaluate Strain Gauge Force
 *
 * \details This function checks the pre-condition to enter next state of EGIA RELOAD CLAMP TEST .
 *          Pre-Conditions:
 *              - Motor stop in position and Clamp Test is Fully Open
 *              - Read Strain Guage Force measured when Clamp is Fully Open
 *              - Check Fully Open Strain Guage force is less than Fully Closed Strain Guage
 *              .
 *
 * \param   me - Pointer to local storage
 * \param   e - Pointer to Event
 *
 * \return  status
 * \retval  false - Pre-Condition failed
 * \retval  true - All Pre-Conditions Passed
 *
 * ========================================================================== */
bool EGutil_ReloadClampTestNextStatePreCond(EGIA *const me, QEvt const *const e)
{
    bool Status;
    uint16_t ProcedureLimit;
    int16_t SGval;
    APP_EGIA_DATA *pEgia;
    QEVENT_MOTOR_STOP_INFO   *pStopInfo; // Pointer to motor stop info signal

    pEgia = EGIA_GetDataPtr();
    Status = false;
    pStopInfo = (QEVENT_MOTOR_STOP_INFO *)e;
    do
    {
        /// \todo 07/20/2021 NP Actual Motor Stop Status Bits needs to be added
        pStopInfo = (QEVENT_MOTOR_STOP_INFO *)e;
        Log(REQ, "EGIAUtil: EGIA Clamping, Clamping stop status = 0x%04X  pos = %d ", pStopInfo->StopStatus, pStopInfo->Position);
        if ((pStopInfo->StopStatus & MOT_STOP_STATUS_IN_POS) && (pStopInfo->Position > FIRE_FULLOPEN_CLOSE) && (pEgia->BatCommState != BAT_COMM_FAULT))
        {
            /// \todo 08/25/2021 NP - Signia_PlayTone with proper API call (Signia_xxxxx)
            //Signia_PlayTone(SNDMGR_TONE_CAUTION);    //Fully Open - Play Caution Tone
        }
        else
        {
            /// \todo 08/11/2021 NP Error handling to be done
            Log(REQ, "EGIAUtil: EGIA Clamptest, Clamptest Not Full Open status = 0x%04X ", pStopInfo->StopStatus);
            pEgia->FullyUnClamped = false;
            // Save the Reload Fully Opened Status to the Handle 
            ME->Reload.ReloadFullyOpened = pEgia->FullyUnClamped;
            break;
        }

        pEgia->FullyUnClamped = true;

        // Save the Reload Fully Opened Status to the Handle ( for status variables data read )
        ME->Reload.ReloadFullyOpened = pEgia->FullyUnClamped;

        /* Read Strain Gauge Force, Reload Clamp Test Fully Open */
        memcpy(&pEgia->SGForceClampFullOpen,  &pEgia->SGForce,  sizeof(SG_FORCE));
        Log(REQ,"Strain Gauge ForceInOpen: %3.2f lbs, Current: %u Counts",pEgia->SGForce.ForceInLBS,pEgia->SGForce.Current );
        Log(REQ,"SG: ForceInOpen : %3.2f lbs, Current : %u Counts",pEgia->SGForce.ForceInLBS,pEgia->SGForce.Current );
        Log(REQ,"EGIAUtil: SG, ForceInOpen =%3.2f lbs, Current= %u Counts",pEgia->SGForce.ForceInLBS,pEgia->SGForce.Current );

        if ((pEgia->SGForceClampFullOpen.Status != SG_STATUS_GOOD_DATA) && (!pEgia->SGForceClampFullOpen.NewDataFlag))
        {
            Log(REQ, "EGIAUtil: Clamp Test FullOpen, Strain Gauge Error");
            /// \todo 2/2/2022 Strain Guage data is Error. Currently no input what to do.
            break;
        }

        SGval = (int16_t)(pEgia->SGForceClampFullClose.Current - pEgia->SGForceClampFullOpen.Current);
        pEgia->SGForce.NewDataFlag = false;
        Log(REQ, "Difference in SG Value %d Counts, FullClose: %d Counts, FullOpen: %d Counts", SGval, pEgia->SGForceClampFullClose.Current, pEgia->SGForceClampFullOpen.Current);
        Log(REQ, "Difference SG Value %d Counts, FullClose : %d Counts, FullOpen : %d Counts", SGval, pEgia->SGForceClampFullClose.Current, pEgia->SGForceClampFullOpen.Current);
        Log(REQ, "EGIAUtil: Difference SG Value = %d Counts, FullClose = %d Counts, FullOpen = %d Counts", SGval, pEgia->SGForceClampFullClose.Current, pEgia->SGForceClampFullOpen.Current);
        /* Is Full Open force is less than 5 A/D counts of Full Close i.e., */
        if (!(SGval   >=  (int16_t)(CLAMP_TEST_MIN_DELTA_COUNTS)))
        {
            /* Clamp Test is not allowed, Do not allow any Motor movement */
            pEgia->ClampCycle = EGIA_CLAMPCYCLE_CLAMPTEST_FAIL;
            Log(REQ, "EGIAUtil: Clamp Test Fail, Strain Guage Open Force is greater than 5 A/D counts of Full Close Force");
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            /*Display Adapter Error Screen */
            Gui_AdapterErrorScreen();
            /* Allow all operations except entry into FIRE MODE  */
            /* Use IsAdapterErr status before entering fire mode */
            /* Write Adapter Procedure Limit to zero */
            ProcedureLimit = 0;
            DeviceMem_WRITE(ME->Adapter, ProcedureLimit, ProcedureLimit);
            break;
        }
        /* Clamp Test is Success*/
        Status = true;
        /* Display required Screen before moving to next state */
        DisplayReloadScreens_ClampTestPass(me);

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Based on Reload DevId gets Reload Type and Length
 *
 * \note    This API should only be called at most once per Reload attachment
 *
 * \param   pMe  - Me pointer
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_GetReloadDeviceTypeLength(Handle *const pMe)
{
    uint8_t ReloadLen;
    AM_STATUS Status;
    RELOADTYPE ReloadType;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    switch (pMe->Reload.DevID)
    {
        case DEVICE_ID_EGIA_SULU_30:
        case DEVICE_ID_EGIA_RADIAL_30:
            ReloadLen   = RELOAD_TYPE_30;
            ReloadType = RELOAD_SULUINTELLIGENT;
            DeviceMem_READSULU(ReloadColor, pEgia->ReloadCartColor);
            Log(REQ, "Reload Type: SULU, Reload Length = 30mm, Reload Cart Color = %d",pEgia->ReloadCartColor);
            break;

        case DEVICE_ID_EGIA_SULU_45:
        case DEVICE_ID_EGIA_RADIAL_45:
            ReloadLen   = RELOAD_TYPE_45;
            ReloadType = RELOAD_SULUINTELLIGENT;
            DeviceMem_READSULU(ReloadColor, pEgia->ReloadCartColor);
            Log(REQ, "Reload Type: SULU, Reload Length = 45mm, Reload Cart Color = %d",pEgia->ReloadCartColor);
            break;

        case DEVICE_ID_EGIA_SULU_60:
        case DEVICE_ID_EGIA_RADIAL_60:
            ReloadLen   = RELOAD_TYPE_60;
            ReloadType = RELOAD_SULUINTELLIGENT;
            DeviceMem_READSULU(ReloadColor, pEgia->ReloadCartColor);
            Log(REQ, "Reload Type: SULU, Reload Length = 60mm, Reload Cart Color = %d",pEgia->ReloadCartColor);
            break;

        case DEVICE_ID_EGIA_MULU_30:
            ReloadLen   = RELOAD_TYPE_30;
            ReloadType = RELOAD_MULUINTELLIGENT;
            Log(REQ, "Reload Type: MULU, Reload Length = 30mm");
            break;

        case DEVICE_ID_EGIA_MULU_45:
            ReloadLen   = RELOAD_TYPE_45;
            ReloadType = RELOAD_MULUINTELLIGENT;
            Log(REQ, "Reload Type: MULU, Reload Length = 45mm");
            break;

        case DEVICE_ID_EGIA_MULU_60:
            ReloadLen   = RELOAD_TYPE_60;
            ReloadType = RELOAD_MULUINTELLIGENT;
            Log(REQ, "Reload Type: MULU, Reload Length = 60mm");
            break;

        default:
            DeviceMem_READStatus(pMe->Reload, Status);
            if (AM_STATUS_OK == Status)
            {
                ReloadType = RELOAD_UNSUPPORTED;
                Log(REQ, "Reload Type: Intelligent with Unsupported ID");
            }
            else
            {
                ReloadType = RELOAD_NONINTELLIGENT;
                Log(REQ, "Reload Type: Non-Intelligent");
            }
            break;
    }
    /* Update Reload Length, Reload Type */
    pEgia->ReloadType =  ReloadType;
    pEgia->ReloadLen  =  ReloadLen;
}

/* ========================================================================== */
/**
 * \brief   Performs MULU Reload Firing Count Test
 *
 * \details This function performs MULU Reload Firing Count test if Firing Count is less
 *          than Firing Limit.
 *          Increments FireCount and write's to Reload and Read's back the latest value.
 *          Checks whether Firing Count is increment or not.
 *          On successful test, provides status and writes back the original FireCount to Reload.
 *          On Failure provides status, play caution tone and display MULU Error Screen
 *
 * \param   me  - Pointer to local data store
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_ReloadMULUFireCountTest(EGIA *const me)
{
    uint8_t FireCount;
    uint8_t FireCountIncremented;
    uint8_t FireLimit;
    uint8_t FireCountDecremented;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();
    FireCount = 0;
    FireLimit = 0;
    FireCountIncremented = 0;
    do
    {
        /*Read Reload Fire Counter */
        DeviceMem_READ(ME->Reload, FireCount, FireCount);
        DeviceMem_READ(ME->Reload, FireLimit, FireLimit);
        ///\todo 04/01/2022 Add Reload authenticate condition
        /* Uses Remaining */
        if (FireCount >= FireLimit)
        {
            Log(REQ, "EGIAUtil: MULU Reload No Firings Left, Firing Count Test not performed");
            break;
        }
        /*Increment FireCount*/
        FireCount++;
        /* Write Incremented Firecounter to Reload EEPROM */
        DeviceMem_WRITE(ME->Reload, FireCount, FireCount);
        /* Read the data from Reload */
        DeviceMem_EEPROMRead(ME->Reload, FireCount, FireCountIncremented);
        /* Set MULU Firing Counter test status to fail */
        pEgia->MuluFireCountTestPass = false;
        /* Is FireCount Incremented and updated to EEPROM*/
        if (FireCountIncremented == FireCount)
        {
            /* FIRE COUNT test passed, write back the original FIRECOUNT value*/
            FireCount--;
            DeviceMem_WRITE(ME->Reload, FireCount, FireCount);
            /* Read the data from Reload */
            DeviceMem_EEPROMRead(ME->Reload, FireCount, FireCountDecremented);
            /* Is written and read back data to Reload EEPROM equal */
            pEgia->MuluFireCountTestPass = (FireCount == FireCountDecremented);
        }

        if (!pEgia->MuluFireCountTestPass)
        {
            /* FIRE COUNT test failed */
            /* Play Error Tone */
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            Gui_MULUErrorWarning_Screen();
            Log(REQ, "EGIAUtil: MULU Reload Firing Count Test Failed");
        }
        else
        {
            Log(REQ, "EGIAUtil: MULU Reload Firing Count Test Passed");
        }

    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Performs Used Cartridge Test
 *
 * \details This function performs Cartridge Used test if Firing Count is 0. Increments FireCount and write's to Reload and Read's back the latest value. Checks whether Firing Count is increment or not.
 *          On successful test, provides status and writes back the original FireCount to Reload.
 *          On Failure provides status, play caution tone and display MULU Cartridge Error Screen
 *
 * \param   me  - Pointer to local data store
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_UsedCartridgeTest(EGIA *const me)
{
    uint8_t FireCount;
    uint8_t FireCountReadBack;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();
    FireCount = 0;

    do
    {
        /*Read Cartridge Fire Counter */
        DeviceMem_READ(ME->Cartridge, FireCount, FireCount);
        if (CARTRIDGE_USED == FireCount)
        {
            Log(REQ, "EGIAUtil: Used Cartridge Connected, Used Cartridge test not performed");
            /* Used Cartridge*/
            break;
        }
        FireCount = CARTRIDGE_USED;
        DeviceMem_WRITE(ME->Cartridge, FireCount, FireCount);
        /* Read the data from Cartridge */
        DeviceMem_EEPROMRead(ME->Cartridge, FireCount, FireCount);
        /* Set status to test not passed */
        pEgia->UsedCartridgeTestPass = false;
        ///\todo 04/01/2022 Add Cartridge authenticate condition
        /* Is cartridge Used */
        if (CARTRIDGE_USED == FireCount)
        {
            /* Write back original FireCount to Cartridge */
            FireCount = CARTRIDGE_NOT_USED;
            DeviceMem_WRITE(ME->Cartridge, FireCount, FireCount);
            /* Read the data from Cartridge */
            DeviceMem_EEPROMRead(ME->Cartridge, FireCount, FireCountReadBack);
            pEgia->UsedCartridgeTestPass = (CARTRIDGE_NOT_USED == FireCountReadBack);
        }

        if (!pEgia->UsedCartridgeTestPass)
        {
            Gui_MULUCartridgeErrorWarning_Screen(FireCount);
            /* Play Error Tone */
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            Log(REQ, "EGIAUtil: Used Cartridge Test Failed");
        }
        else
        {
            Log(REQ, "EGIAUtil: Used Cartridge Test Passed");
        }

    } while (false);
}
/* ========================================================================== */
/**
 * \brief   Check Reload End Of Life
 *
 * \details Checks End of Life for SULU, MULU Intelligent Reloads. Display SULU/MULU based EOL screen and Plays Caution Tone
 *
 * \param   me  - me Pointer
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_ProcessReloadEOL(EGIA *const me)
{
    bool         CautionStatus;
    uint8_t      FireCount;
    uint8_t      FireLimit;
    bool         ErrorCondition;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();
    CautionStatus = false;
    CautionStatus = (ME->Adapter.AdapterEOL || ME->Clamshell.ClamshellEOL || ME->Handle.HandleEOL);

    /*Is Handle, Adaptr or Clamshell are in Error Condition */
    ErrorCondition = (ME->ActiveFaultsInfo.IsFileSysErr || ME->ActiveFaultsInfo.IsAccelErr || ME->ActiveFaultsInfo.IsPermFailWop || ME->ActiveFaultsInfo.IsErrShell);

    switch (pEgia->ReloadType)
    {
        case RELOAD_SULUINTELLIGENT:
            DeviceMem_READ(ME->Reload, FireCount, FireCount);
            /* Is Reload Used */
            if ( FireCount > 0 )
            {
                if (ME->Clamshell.ClamshellEOL)
                {
                    Gui_UsedReload_ScreenAndLock(true);
                }
                else
                {
                    Gui_UsedReload_ScreenAndLock(false);
                }
                // Save the SULUReloadEOL Status to the Handle 
                ME->Reload.ReloadSuluEOL = true;
                Log(REQ, "EGIAUtil: SULU reload End Of Life");
            }
            break;

        case RELOAD_MULUINTELLIGENT:
            /* Read the Reload Fire Count, Fire Limit */
            DeviceMem_READ(ME->Reload, FireCount, FireCount);
            DeviceMem_READ(ME->Reload, FireLimit, FireLimit);
            if ( FireCount >= FireLimit )
            {
                /* MULU EOL screen */
                Gui_EndOfLifeMULU_ScreenAndLock();
                // Save the MULUReloadEOL Status to the Handle 
                ME->Reload.ReloadMuluEOL = true;                
                Log(REQ, "EGIAUtil: MULU reload End Of Life");
            }
            break;

        default:
            /* Non-Intelligent Reload */
            break;
    }

    // If any CautionStatus or Error Condition are exists play Caution tone
    if ( CautionStatus || ErrorCondition)
    {
        Signia_PlayTone(SNDMGR_TONE_CAUTION);
    }
}

/* ========================================================================== */
/**
 * \brief   Process Reload and Cartridge events
 *
 * \details This function processes Egia specific event Reload(Mechanical Switch),Cartridge connection events, updating
 *          local data and displays as appropriate.
 *          CARTRIDGE event only handles EGIA Specific processing. Other processing of CARTRIDGE is part of HNutil_ProcessDeviceConnEvents(Handle.c)
 *
 * \param   pMe  - Pointer to local data store
 * \param   pSig - Pointer to signal to process
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_ProcessDeviceConnEvents(Handle *const pMe, QEvt const *const pSig)
{
    QEVENT_ADAPTER_MANAGER *pEvent;
    APP_EGIA_DATA *pEgia;

    QEVENT_FAULT *pSignalEvent;
    SIGNAL CurrentSig;

    pEgia = EGIA_GetDataPtr();
    pEvent = (QEVENT_ADAPTER_MANAGER *)pSig;

    switch (pEvent->Event.sig)
    {
        case P_EGIA_RELOAD_CONNECTED_SIG:
            Log(DBG, "EGIAUtil: P_EGIA_RELOAD_CONNECTED_SIG received");
            pMe->EgiaReload.Status = AM_DEVICE_CONNECTED;
            if (pMe->Reload.Status == AM_DEVICE_CONNECTED)
            {
                pMe->EgiaReload.pHandle = pMe->Reload.pHandle;
                DeviceMem_READ(pMe->EgiaReload, DeviceType, pMe->Reload.DevID);
            }
            break;

//        case P_CARTRIDGE_CONNECTED_SIG:
        /* update the 1-wire authentication status */
//            pEgia->CartridgeAuthenticated = pEvent->Authentic;
//            ProcessCartridge(pMe);
//            break;

        case P_EGIA_RELOAD_REMOVED_SIG:
            Log(DBG, "EGIAUtil: P_EGIA_RELOAD_REMOVED_SIG received");
            pMe->EgiaReload.Status = AM_DEVICE_DISCONNECTED;
            /* Clear Reload Type */
            pEgia->ReloadType =  RELOAD_NONE;
            if (pMe->Adapter.ConnectorBusShort)
            {
                CurrentSig = P_ADAPTER_ERROR_SIG;
                pSignalEvent = AO_EvtNew(CurrentSig, sizeof(QEVENT_FAULT));
                pSignalEvent->ErrorCause = ADAPTER_ONEWIRE_SHORT;
                pSignalEvent->ErrorStatus = true;
                AO_Publish(pSignalEvent, NULL);
            }
            break;

        case P_CARTRIDGE_CONNECTED_SIG:
            /* Add only EGIA Specific Cartridge Connection processing */
            /* update the 1-wire authentication status */
            pEgia->CartridgeAuthenticated = pEvent->Authentic;
            /* Get the MULU Reload Cart Color */
            DeviceMem_READ(pMe->Cartridge, ReloadColor, pEgia->ReloadCartColor);
            ProcessCartridge(pMe);
            break;

        case P_CARTRIDGE_REMOVED_SIG:
            pEgia->CartridgeAuthenticated = pEvent->Authentic;
            break;

        default:
            break;
    }
}

/* ========================================================================== */
/**
 * \brief   Updates the Rotation
 *
 * \details This function will Start, Pause, Resume, Stop the Rotationa
 *                           Rotation State:
 *                                   i. MOTOR START   : Starts the Rotation
 *                                 iii. MOTOR STOPPED : Stops the Rotation
 *
 * \todo    Need 100msec delay between button press and starting move.
 *          Implement outside of this function.
 *
 * \param   Command -
 * \param   Position -
 * \param   Speed -
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_UpdateRotation(MOTOR_COMMAND Command, int32_t Position, uint16_t Speed)
{
    switch (Command)
    {
        case MOTOR_START:
            /* Set Position to Home */
            Signia_MotorSetPos(ROTATE_MOTOR, 0);

            Signia_MotorStart(
                ROTATE_MOTOR,
                Position,
                Speed,
                TIME_DELAY_200,
                ROTATE_MOTOR_ROTATION_TIMEOUT,
                ROTATE_MOTOR_CURRTRIP,
                ROTATE_MOTOR_ROTATION_CURRENTLIMIT,
                true,
                MOTOR_VOLT_15,
                REPORT_INTERVAL,
                NULL
            );

            Log(REQ, "EGIAUtil: EGIA Rotation Started");
            break;

        case MOTOR_STOP:
            /* Stop Rotation Motor */
            Signia_MotorStop(ROTATE_MOTOR);
            Log(REQ, "EGIAUtil: EGIA Rotation Stopped");
            break;

        default:
            /* Do nothing */
            break;
    }
}

/* ========================================================================== */
/**
 * \brief   Articulate left/right and Stop Articulation
 *
 * \details This function will Start/Stop the Articulation
 *                           Articulate State:
 *                                   i. MOTOR_START      : Left/Right Articulation
 *                                  ii. MOTOR STOPPED    : Stops the Articulation
 *
 * \param   Command -
 * \param   Position -
 * \param   Speed -
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_UpdateArticulation(MOTOR_COMMAND Command, int32_t Position, uint16_t Speed)
{
    switch (Command)
    {
        case MOTOR_START:
            /* Start the Motor */
            Signia_MotorStart(
                ARTIC_MOTOR,
                Position,
                Speed,
                TIME_DELAY_200,
                ARTIC_TIMEOUT,
                ARTIC_CURRENTTRIP,
                ARTIC_CURRENTLIMIT,
                true,
                MOTOR_VOLT_15,
                REPORT_INTERVAL,
                NULL
            );
            Log(REQ, "EGIAUtil: EGIA Articulation Started");
            break;

        case MOTOR_STOP:
            /* Stop Articulation Motor */
            Signia_MotorStop(ARTIC_MOTOR);
            Log(REQ, "EGIAUtil: EGIA Articulation Stopped");
            break;

        default:
            /* Do nothing */
            break;
    }
}

/* ========================================================================== */
/**
 * \brief   Updates the Reload type/length and Handles Reload related Error
 *
 * \note    This API should only be called on Reload attachment
 *
 * \param   pMe  - Pointer to local data store
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_CheckForReloadErrors(Handle *const pMe)
{
    uint8_t FireCount;   //temporary placeholder
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    do
    {
        if ((RELOAD_MULUINTELLIGENT == pEgia->ReloadType) && (AM_STATUS_OK != pMe->Reload.pHandle->Status))
        {
            DeviceMem_READ(pMe->Reload, FireCount, FireCount);
            Log(ERR, "EGIAUtil: Connected MULU Reload and 1_WIRE communication to a MULU fails  ****");
            Gui_FluidIngressIndicator_Screen(FireCount, pEgia->ReloadLen);
            break;
        }

        // Check for Reload One wire EEPROM errors
        if ((AM_STATUS_OK != pMe->Reload.pHandle->Status) && (RELOAD_NONINTELLIGENT != pEgia->ReloadType))
        {
            Log(ERR, "EGIAUtil: Reload 1-Wire EEPROM has error ");
            //1-Wire EEPROM Error
            pEgia->ReloadType = RELOAD_NONINTELLIGENT;
            /* Play Fault Tone */
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            NonIntReloadScreenProgress(RELOAD_NON_INT_START, NOT_USED,false,false);
            break;
        }

        //check unsupported ID
        if (RELOAD_UNSUPPORTED == pEgia->ReloadType)
        {
            Log(ERR, "EGIAUtil: INTELLIGENT RELOAD with an unsupported ID is detected.");
            /* Play Fault Tone */
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            /// \todo 04/01/2022 NP- Recheck the Screen Function and uncomment
            Gui_ReloadErrorWarning_Screen();
            break;
        }

        //Authentication fail for MULU Intelligent Reload
        if ((AUTHENTICATION_SUCCESS != pEgia->ReloadAuthenticated) && (RELOAD_MULUINTELLIGENT == pEgia->ReloadType))
        {
            Log(REQ, "Authentication failure for MULU Intelligent Reload");
            Log(REQ, "**** Authentication fail for MULU Intelligent Reload  ****");
            Log(REQ, "EGIAUtil: Authentication fail for MULU Intelligent Reload ");
            /* Play Fault Tone */
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            //Display the MULU Reload Error Screen
            /// \todo 04/01/2022 NP- Recheck the Screen Function and uncomment
            Gui_MULUErrorWarning_Screen();
            break;
        }

        //Authentication fail for SULU Intelligent Reload
        if ((AUTHENTICATION_SUCCESS != pEgia->ReloadAuthenticated) && (RELOAD_SULUINTELLIGENT == pEgia->ReloadType))
        {
            Log(REQ, "Authentication failure for SULU Intelligent Reload");
            Log(REQ, "**** Authentication fail for SULU Intelligent Reload  ****");
            Log(REQ, "EGIAUtil: Authentication fail for SULU Intelligent Reload ");
            /* Play Fault Tone */
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            //Display the MULU Reload Error Screen
            Gui_ReloadErrorWarning_Screen();
            break;
        }

        if (!(pEgia->ReloadDeviceWriteStatus) && (RELOAD_SULUINTELLIGENT == pEgia->ReloadType))
        {
            Signia_PlayTone(SNDMGR_TONE_FAULT);
            /// \todo 04/01/2022 NP- Recheck the Screen Function and uncomment
            Gui_ReloadErrorWarning_Screen();
            break;
        }
    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Handle the Fire counter update for Handle and Adapter
 *
 * \details This function will Do the Handle and Adapter Fire counter update as well as Adapter Autoclave Counter
 *
 * \note    Single Timer is used for retry i.e."RetryFireCountUpdateTimer"
 *
 * \param   pMe  - Pointer to Handle data store
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_FireModeHandling(Handle *const pMe)
{
    uint16_t NewHandleFireCounter;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    // handle fire counter is updated here
    // Read the Fire Counter Value
    DeviceMem_READ(pMe->Handle, FireCount, pEgia->PrevHandleFireCounter);

    // Increment the Fire Counter Value
    NewHandleFireCounter = pEgia->PrevHandleFireCounter;
    NewHandleFireCounter++;

    // Write the Fire Counter Value
    DeviceMem_WRITE(pMe->Handle, FireCount, NewHandleFireCounter);

    //Check 1-Wire Write is failed for handle fire counter update
    if (AM_STATUS_ERROR == pMe->Handle.pHandle->Status)
    {
        Log(REQ, "Handle Fire Count Update Failed");
        Log(REQ, "**** Handle Fire Count Update Failed  ****");
        Log(REQ, "EGIAUtil: Handle Fire Count Update Failed ");

        // Update failed - Update the flag(Enum)
        pEgia->HandleFireCountUpdated = FIRE_COUNT_UPDATE_FAILED;

        // Retry timer is started for update retry for handle fire count
        AO_TimerArm(&pMe->RetryFireCountUpdateTimer, FIRE_COUNT_UPDATE_TIME, 0);
    }
    else
    {
        // Update Successful - Update the flag(Enum)
        Log(REQ, "Handle Fire Count Update Success");
        Log(REQ, "**** Handle Fire Count Update Success  ****");
        Log(REQ, "EGIAUtil: Handle Fire Count Update Success");
        pEgia->HandleFireCountUpdated = FIRE_COUNT_UPDATE_SUCCESS;

        // go to retract
        Signia_PlayTone(SNDMGR_TONE_EXIT_FIRE_MODE);

        /* clear LED when UP key pressed and transition to retract state*/
        L4_GpioCtrlClearSignal(GPIO_GN_LED);
    }

    // Set the system flag to increment the Procedure count when the
    // handle goes on charger and at least one firing completed
    /// \todo KIA 12/04/2022 - what if handle is soft reset? Is this flag in noinit RAM?
    SetSystemStatus(SYSTEM_STATUS_PROCEDURE_HAS_FIRED_FLAG);
}

/* ========================================================================== */
/**
 * \brief   Adapter Procedure and Firng counter update
 *
 * \details This function will Do the Adapter Fire counter update as well as Adapter Autoclave Counter
 *                    as well as MULU reload fire counter
 *
 * \note    Single Timer is used for retry i.e."RetryFireCountUpdateTimer"
 *
 * \param   pMe  - Pointer to Handle data store
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_FireModeOpenPress(Handle *const pMe)
{
    uint16_t NewTempCounter;
    uint8_t MULUFiringCountValue;   //temporary placeholder
    uint8_t FireCount;
    uint8_t CartridgeFiringCountValue;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();
    MULUFiringCountValue = 0;
    NewTempCounter = 0;

    /* Increment the Firing Counter for Adapter */
    // Read the Adapter Firing Counter Value
    DeviceMem_READ(pMe->Adapter, FireCount, pEgia->PrevAdapterFireCounter);

    // Increment the Firing Counter Value
    pEgia->PrevAdapterFireCounter++;

    // Write the Firing Counter Value
    DeviceMem_WRITE(pMe->Adapter, FireCount, pEgia->PrevAdapterFireCounter);

    //Check 1-Wire Write is failed for Adapter fire counter update
    if (AM_STATUS_ERROR == pMe->Adapter.pHandle->Status)
    {
        Log(REQ, "EGIAUtil: Adapter Fire Count Update Failed");

        // Update failed - Update the flag(Enum)
        pEgia->AdapterFireCountUpdated = FIRE_COUNT_UPDATE_FAILED;
    }
    else
    {
        // Update Successful - Update the flag(Enum)
        Log(REQ, "EGIAUtil: Adapter Fire Count Update Success");
        pEgia->AdapterFireCountUpdated = FIRE_COUNT_UPDATE_SUCCESS;
    }

    /* Adapter Procedure counter is updated here */
    // Check if Adapter is connected for the first time in this procedure
    if (!HNutil_AddAdapterDeviceToProcedureList(pMe->Adapter.DevID))  // Update list
    {
        // This is a new Adapter
        // Read the Adapter Procedure Counter Value
        DeviceMem_READ(pMe->Adapter, ProcedureCount, pEgia->PrevAdapterProcCounter);
        // Read the Adapter Procedure Limit Value
        DeviceMem_READ(pMe->Adapter, ProcedureLimit, pEgia->AdapterProcedureLimit);

        // Increment the Adapter Procedure Counter Value
        NewTempCounter = pEgia->PrevAdapterProcCounter;
        NewTempCounter++;

        pEgia->AdapterProcRemaining = pEgia->AdapterProcedureLimit - NewTempCounter;

        // Write the Adapter Procedure Counter Value
        DeviceMem_WRITE(pMe->Adapter, ProcedureCount, NewTempCounter);

        // Adapter First time added and its failed to write
        if (AM_STATUS_ERROR == pMe->Adapter.pHandle->Status)
        {
            Log(REQ, "EGIAUtil: Adapter Procedure Counter Update Failed");

            // Update failed - Update the flag(Enum)
            pEgia->AdapterProcCountUpdated = FIRE_COUNT_UPDATE_FAILED;
        }
        else
        {
            // Update Successful - Update the flag(Enum)
            Log(REQ, "EGIAUtil: Adapter Procedure Counter Update Success");
            pEgia->AdapterProcCountUpdated = FIRE_COUNT_UPDATE_SUCCESS;
        }
    }
    else
    {
        // Adapter is already in the list, don't update the Adapter Procedure Counter
        Log(REQ, "EGIAUtil: Adapter = 0x%04X is already in the previously connected Adapter list", pMe->Adapter.DevID);
        pEgia->AdapterProcCountUpdated = FIRE_COUNT_UPDATE_SUCCESS;
    }

    /* Update successful for both Adapter Fire counter and Adapter Procedure Counter (Autoclave) */
    if ((FIRE_COUNT_UPDATE_SUCCESS == pEgia->AdapterProcCountUpdated) && (FIRE_COUNT_UPDATE_SUCCESS == pEgia->AdapterFireCountUpdated))
    {
        Signia_PlayTone(SNDMGR_TONE_EXIT_FIRE_MODE);
        L4_GpioCtrlClearSignal(GPIO_GN_LED);
    }
    else if ((FIRE_COUNT_UPDATE_FAILED == pEgia->AdapterProcCountUpdated) || (FIRE_COUNT_UPDATE_FAILED == pEgia->AdapterFireCountUpdated))
    {
        // Retry timer is started for update retry for adapter fire count and adapter procedure counter (Autoclave)
        AO_TimerArm(&pMe->RetryFireCountUpdateTimer, FIRE_COUNT_UPDATE_TIME, 0);
    }

    // Increment the Firing Counter for MULU Reload
    if (RELOAD_MULUINTELLIGENT == pEgia->ReloadType)
    {
        // Read the Firing Counter Value
        DeviceMem_READ(pMe->Reload, FireCount, MULUFiringCountValue);

        // Increment the Firing Counter Value
        MULUFiringCountValue++;
        CartridgeFiringCountValue = CARTRIDGE_USED;

        // Write the Firing Counter Value
        DeviceMem_WRITE(pMe->Reload, FireCount, MULUFiringCountValue);

        /*update USED status to cartridge */
        DeviceMem_WRITE(pMe->Cartridge, FireCount, CartridgeFiringCountValue);
    }
    else if (RELOAD_SULUINTELLIGENT == pEgia->ReloadType)
    {
        Log(DBG, "EGIAUtil: Post firing SULU Reload set to USED ");
        FireCount = 1;
        DeviceMem_WRITE(pMe->Reload, FireCount, FireCount);
    }
    else
    {
        /* Do Nothing */
    }
    /* Firing Completion Flag for smart reload. This is used to check FPGA Reset */
    /* during fire mode and indicate FPGA error on reload is fully open after fire mode exit i.e. in clamp test */
    pEgia->FiringComplete = true;
}

/* ========================================================================== */
/**
 * \brief   Test Adapter Firing and Autoclave Counter update possible test
 *
 * \details Test the ability to increment the ADAPTER Firing and Autoclave Counters, when an ADAPTER passes validation and there are uses remaining
 *
 * \param   pMe - Pointer to Handle Data
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_AdapterFiringAutoclaveCounterTest(Handle *const pMe)
{
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    pEgia->AdapterTestPass = false;
    /* Read Adapter Fire Counter */
    DeviceMem_READ(pMe->Adapter, FireCount, pEgia->AdapterFiringCounter);
    DeviceMem_READ(pMe->Adapter, FireLimit, pEgia->AdapterFiringLimit);

    DeviceMem_READ(pMe->Adapter, ProcedureCount, pEgia->AdapterProcedureCounter);
    DeviceMem_READ(pMe->Adapter, ProcedureLimit, pEgia->AdapterProcedureLimit);

    /* Test Adapter Uses remaining  */
    /* Check adapter is validated */
    if (AM_DEVICE_CONNECTED == pMe->Adapter.Status)
    {
        if ((pEgia->AdapterFiringLimit >= pEgia->AdapterFiringCounter) || (pEgia->AdapterProcedureLimit >= pEgia->AdapterProcedureCounter))
        {
            pEgia->AdapterTestPass = true;
        }
        else
        {
            /* Adapter Error Screen */
            /// \todo 04/11/2022 NP - Add the adapter error screen
            /* Play Fault Tone */
            Signia_PlayTone(SNDMGR_TONE_FAULT);
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Requests to update the Force To Speed Table
 *
 * \details This function reads the AsaLow, AsaHigh, AsaMax from SULU RELOAD or MULU Cartridge and requests Signia layer API to update the Force to Speed Table
 *
 * \param   pMe - Me Pointer
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_AsaUpdateForceToSpeedTable(Handle *const pMe)
{
    uint8_t AsaLow;
    uint8_t AsaHigh;
    uint8_t AsaMax;
    AM_STATUS Status;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();
    AsaLow  = 0;
    AsaHigh = 0;
    AsaMax  = 0;

    switch (pEgia->ReloadType)
    {
        case RELOAD_SULUINTELLIGENT:
            DeviceMem_READStatus(pMe->Reload, Status);

            /* Is Reload EEPROM Status OK */
            if (AM_STATUS_OK == Status)
            {
                DeviceMem_READSULU(asaLow, AsaLow);
                DeviceMem_READSULU(asaHigh, AsaHigh);
                DeviceMem_READSULU(asaMax, AsaMax);
                DeviceMem_READ(pMe->Reload, FireForceMax, pEgia->FiringMaxForceRead);
            }
            break;

        case RELOAD_MULUINTELLIGENT:
            DeviceMem_READStatus(pMe->Cartridge, Status);

            /* Is Cartridge EEPROM Status OK, else use default specificatons defined in above table */
            if (AM_STATUS_OK == Status)
            {
                DeviceMem_READ(pMe->Cartridge, asaLow, AsaLow);
                DeviceMem_READ(pMe->Cartridge, asaHigh, AsaHigh);
                DeviceMem_READ(pMe->Cartridge, asaMax, AsaMax);
                DeviceMem_READ(pMe->Reload, FireForceMax, pEgia->FiringMaxForceRead);
            }
            break;

        default:
            break;
    }
    InitializeAsaForceToSpeedTable(AsaLow, AsaHigh, AsaMax);
}

/* ========================================================================== */
/**
 * \brief   Get the Fire Speed to Start Motor based on ClampForce
 *
 * \details This function reads the ClamLow, ClampHigh, ClampMax force levels from SULU RELOAD or MULU CARTRIDGE and gets the Fire Speed and Fire Speed State
 *
 *
 * \param   pMe - Me Pointer
 *
 * \return  FireSpeed
 *
 * ========================================================================== */
uint16_t EGutil_GetFireSpeedFromClampForce(Handle *const pMe)
{
    AM_STATUS  Status;
    uint8_t    ClampLow;
    uint8_t    ClampHigh;
    uint8_t    ClampMax;
    uint16_t   FiringSpeedRPM;
    FIRINGSPEED FiringSpeedState;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();
    ClampLow  = 0;
    ClampHigh = 0;
    ClampMax  = 0;

    switch (pEgia->ReloadType)
    {
        case RELOAD_SULUINTELLIGENT:
            DeviceMem_READStatus(pMe->Reload, Status);

            /* Is Reload EEPROM Status OK */
            if (AM_STATUS_OK == Status)
            {
                DeviceMem_READSULU(ClampLow, ClampLow);
                DeviceMem_READSULU(ClampHigh, ClampHigh);
                DeviceMem_READSULU(ClampMax, ClampMax);
            }
            break;

        case RELOAD_MULUINTELLIGENT:
            DeviceMem_READStatus(pMe->Cartridge, Status);

            /* Is Cartridge EEPROM Status OK, else use default specifications defined in above table */
            if (AM_STATUS_OK == Status)
            {
                DeviceMem_READ(pMe->Cartridge, ClampLow, ClampLow);
                DeviceMem_READ(pMe->Cartridge, ClampHigh, ClampHigh);
                DeviceMem_READ(pMe->Cartridge, ClampMax, ClampMax);
            }
            break;

        default:
            /*Non Intelligent Reload, use default specification from above defined table "Fire_ForceToSpeedTable" */
            break;
    }

    GetAsaClampForceSpeed(pMe, ClampLow, ClampHigh, ClampMax, &FiringSpeedRPM, &FiringSpeedState);

    pEgia->FiringInfo.FiringSpeedState = FiringSpeedState;

    return FiringSpeedRPM;
}

/* ========================================================================== */
/**
 * \brief   When any of two bottom rotation keys pressed twice within 0.5 sec, reduces the Motor Speed to next level
 *
 * \details This function evaluates any of two bottom rotation keys pressed twice within 0.5 secs and reduces the Motor Speed
 *
 * \param   me - Me pointer
 * \param   Key - Key Instance
 *
 * \return  true if Bottom Key pressed twice in 0.5 secs, false otherwise
 *
 * ========================================================================== */
bool EGutil_GetBottomKeyPressedTwiceInHalfSecond(EGIA *const me, KEY_ID Key)
{
    int32_t CheckTimeExpired;
    bool    Status;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();
    Status = false;

    do
    {
        /* Is Firing In Progress */
        if (!pEgia->FiringInfo.FiringInProgress)
        {
            break;
        }
        /* Is this the first key press in Firing */
        if (pEgia->FiringInfo.SameKeyCount == 0)
        {
            /* Hold the Key data */
            pEgia->FiringInfo.Key = Key;

            /* Get Current time stamp */
            pEgia->FiringInfo.KeyPressTime = SigTime();

            /* Increment Key Press count */
            pEgia->FiringInfo.SameKeyCount += 1;
            break;
        }
        /* Other Rotation Bottom Key is Pressed, Previous key press is other Bottom Key */
        if ((pEgia->FiringInfo.SameKeyCount == 1) && (pEgia->FiringInfo.Key != Key))
        {
            /* Hold the key */
            pEgia->FiringInfo.Key = Key;

            /* Hold the time stamp */
            pEgia->FiringInfo.KeyPressTime = SigTime();

            /*Set KeyCount to One */
            pEgia->FiringInfo.SameKeyCount = 1;
            break;
        }
        /* Same Key has been Pressed Twice */
        if ((pEgia->FiringInfo.SameKeyCount == 1) && (pEgia->FiringInfo.Key == Key))
        {
            CheckTimeExpired = SigTime() - pEgia->FiringInfo.KeyPressTime;
            Status = (CheckTimeExpired <= MSEC_500) ? true : false;
            pEgia->FiringInfo.Key          = KEY_COUNT;
            pEgia->FiringInfo.SameKeyCount = 0;
        }
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Updates the Motor Speed based on Bottom Key press twice within 0.5 seconds
 *
 * \details This function updates the motor speed based on Bottom key press twice
 *
 * \param   < None >
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_UpdateSpeedBasedOnKeyPress(void)
{
    FIRINGSPEED FiringSpeedState;
    MM_STATUS MotorStatus;
    uint16_t FiringSpeedRPM;
    SNDMGR_TONE Tone;
    char SpeedString[10];
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    switch (pEgia->FiringInfo.FiringSpeedState)
    {
        case FIRINGSPEED_FAST:
            FiringSpeedState = FIRINGSPEED_MEDIUM;
            Tone = SNDMGR_TONE_MEDIUM_SPEED;
            snprintf(SpeedString, sizeof(SpeedString), "%s", "Medium");
            break;

        case FIRINGSPEED_MEDIUM:
            FiringSpeedState = FIRINGSPEED_SLOW;
            Tone = SNDMGR_TONE_SLOW_SPEED;
            snprintf(SpeedString, sizeof(SpeedString), "%s", "Slow");
            break;

        case FIRINGSPEED_SLOW:
            FiringSpeedState = FIRINGSPEED_SLOW;
            Tone = SNDMGR_TONE_SLOW_SPEED;
            snprintf(SpeedString, sizeof(SpeedString), "%s", "Slow");
            break;

        default:
            break;
    }

    AsaUpdateFireStateGetFiringSpeed(FiringSpeedState, &FiringSpeedRPM);

    MotorStatus =  Signia_MotorUpdateSpeed(FIRE_MOTOR, FiringSpeedRPM, MOTOR_VOLT_15);
    if (MotorStatus == MM_STATUS_OK)
    {
        pEgia->FiringInfo.FiringSpeedState = FiringSpeedState;
        Log(DBG, "EGIAUtil: Fire Motor Speed= %s Reduced due to Bottom Key Clicks Within 500ms", SpeedString);
        Signia_PlayTone(Tone);
    }
}
/* ========================================================================== */
/**
 * \brief   Process Egia StrainGauge Raw Data
 *
 * \details This function processes the raw strain gauge ADC value with EGIA
 *          specific strain gauge coefficients to pound force. This function is
 *          registered as a callback in AdapterDefn module to be called every ms.
 *
 * \param   pTemp - void pointer, typecast to SG_FORCE to access the Strain Guage value
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_ProcessEgiaStrainGaugeRawData(void *pTemp)
{
    float32_t     SGCount;  /* Used to measure force in Pound */
    APP_EGIA_DATA *pEgia;
    SG_FORCE *pSgForce;
    pSgForce = (SG_FORCE *)pTemp;
    pEgia = EGIA_GetDataPtr();

    do
    {
        if (NULL == pSgForce)
        {
            break;
        }

        pSgForce->Status = SG_STATUS_GOOD_DATA;

        SGCount = (float32_t)(pSgForce->Current) - pEgia->CalibrationTareCounts;
        pSgForce->ForceInLBS = ((SGCount * pEgia->CalibParam.StrainGauge.Multiplier) + pEgia->CalibParam.StrainGauge.Offset);

        if (pEgia->CalibParam.StrainGauge.SecondOrder)
        {
            pSgForce->ForceInLBS += (pEgia->CalibParam.StrainGauge.SecondOrder * (SGCount * SGCount));
        }

        if (!pEgia->CoefficientsStatus || !pEgia->CalibParam.StrainGauge.Multiplier)
        {
            pSgForce->Status |= SG_STATUS_UNCALIBRATED_DATA;
        }

        if (pSgForce->Current >  EGIA_ADC_MAX_COUNT)
        {
            pSgForce->Status |= SG_STATUS_OVER_MAX_ADC_DATA;
        }

        if (!pSgForce->Current)
        {
            pSgForce->Status |= SG_STATUS_ZERO_ADC_DATA;
        }

        // Get the force into EGIA internal use
        pEgia->SGForce.ForceInLBS = pSgForce->ForceInLBS;
        pEgia->SGForce.Current = pSgForce->Current;
        pEgia->SGForce.NewDataFlag = pSgForce->NewDataFlag;
        pEgia->SGForce.Status = pSgForce->Status;

    } while (false);
}
/* ========================================================================== */
/**
 * \brief   Process Egia Reload Switch Data
 *
 * \details This function processes the Adapter Reload switch, publishes Egia Reload Connected
 *          or Disconnected upon receiving Open or closed state. This function is
 *          registered as a callback in AdapterDefn module to be called every ms.
 *
 * \param   pTemp - void pointer, typecast to SWITCH_DATA to access the Adapter Reload Switch Data
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_ProcessEgiaReloadSwitchData(void *pTemp)
{
    SWITCH_DATA  *pSwitchState;
    SIGNAL ReloadSignal;
    QEVENT_ADAPTER_MANAGER *pEvent;
    APP_EGIA_DATA *pEgia;

    /* Get EGIA pointer */
    pEgia = EGIA_GetDataPtr();
    do
    {
        /* NULL Check */
        if ( NULL == pTemp )
        {
            break;
        }

        /* Typecast to Switch Data */
        pSwitchState = ( SWITCH_DATA  *)pTemp;

        /* Process only if Closed or Open*/
        if ( ( SWITCH_STATE_OPEN != pSwitchState->State ) && ( SWITCH_STATE_CLOSED != pSwitchState->State ) )
        {
            break;
        }

        /* Is there a change in Switch Data */
        if ( pSwitchState->State != pEgia->PrevSwitch.State )
        {
            /* Get Switch State */
            pEgia->PrevSwitch.State = pSwitchState->State;

            /* Get Switch TimeStamp */
            pEgia->PrevSwitch.TimeStamp = pSwitchState->TimeStamp;

            /* Identify Connected or Removed */
            ReloadSignal = (SWITCH_STATE_CLOSED == pSwitchState->State) ? P_EGIA_RELOAD_CONNECTED_SIG : P_EGIA_RELOAD_REMOVED_SIG;

            /* Create New event */
            pEvent = AO_EvtNew(ReloadSignal, sizeof(QEVENT_ADAPTER_MANAGER));
            if (pEvent)
            {
                /* Publish Reload Connect/Remove event */
                AO_Publish(pEvent, NULL);
            }
        }
    } while (false);
}
/*==============================================================================*/
/**
 * \brief    Set Calibration Tare
 *
 * \details  This function is to be called after the adapter has been calibrated.
 *           (zero load at home position) It sets the internal tare in lbs based
 *           on the ADCCOunt tare.
 *
 *
 * \param    Tare -  The raw Strain Gauge ADC count reading at the adapter's "home" position.
 *                   The difference between this value and zero lb count will be saved.
 *
 * \return   status
 *           false if the tare value is too large, it was not used.
 *           true if valid tare value found
 *
 * ========================================================================== */
bool EGutil_SetCalibrationTareLbs(uint16_t Tare)
{
    bool  RetVal;
    bool  ValidCoef;  //coef validated when read from adapter, but check again
    float32_t ZeroCount;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();
    RetVal = false;

    //check the present coefficients and get root which is the zero count at factory calibration
    ValidCoef = EGutil_ValidateCalibCoefficients(&ZeroCount, Tare);
    Log(REQ,"Strain Guage: ADC Value at Tare :%u", Tare);
    Log(REQ,"SG:ADC Value at Tare :%u", Tare);
    Log(REQ,"EGIAUtil: SG, ADC Value at Tare :%u", Tare);

    //check raw strain gauge tare value for floor and ceiling
    if ((Tare < pEgia->CalibParam.BoardParam.ZBCountCeiling) && \
            (Tare > pEgia->CalibParam.BoardParam.ZBCountFloor) && \
            (ValidCoef == true))
    {
        pEgia->CalibrationTareCounts = (float32_t)Tare - ZeroCount;

        if ((pEgia->CalibrationTareCounts < pEgia->CalibParam.BoardParam.TareDriftHigh) && \
                (pEgia->CalibrationTareCounts > pEgia->CalibParam.BoardParam.TareDriftLow))
        {
            RetVal = true;
        }
    }
    return RetVal;
}

/* ========================================================================== */
/**
 * \brief   Validate Strain gauge calibration coefficients
 *
 * \details This function validates the strain gauge coefficients that are
 *          loaded to egia data. The coefficients are used to convert
 *          raw strain gauge values (adc counts streamed from adapter)
 *          to pounds force.
 *
 * \note    Coefficients test:
 *              - Cannot be default value of 2nd Order = 0, Multiplier = 1, offset = 0
 *              .
 *
 *          Linear Coefficients:
 *              - The multiplier cannot be 0
 *              - The root must be > 0 as the zero adc counts cannot be neg and 0 is invalid
 *              .
 *
 *          Quadratic Coefficient tests:
 *              - The roots must be real numbers (discriminant cannot be < 0)
 *              - For two roots (discriminant >0)
 *                  - x1 > 0  & x2 <= 0  The positive root is the actual root as the adc count cannot be negative
 *                  - x1 > 0  & x2 > 0   The Root closer to the zero counts measured is the real root
 *                  - x1 <= 0 & x2 <= 0 No valid root the actual root as the adc count cannot be negative
 *                  .
 *
 *              - One root (discriminant = 0) root must be > 0 (adc counts do not go negative, 0 is invalid)
 *              .
 *
 * \note        This does not test for limits on ADC factory zero, the actual zero is tested at tare
 *              (firing rod calibrate).
 * \n \n
 *              In tare the positive root will be used for two root quadradic fit.
 *
 * \param   pRoot   - Pointer to write the root of the coefficients to, the root would be the zero counts
 *                    at zero pounds force. If the coefficients are illegal no value is written.
 * \param   Tare    - The actual x value read at y = 0, counts at zero pounds
 *
 * \return  ValidCoeff
 *              - false if the coefficients cannot be used to convert adapterData to force
 *              - true  if valid coefficients
 *
 * ========================================================================== */
bool EGutil_ValidateCalibCoefficients(float32_t *pRoot, uint16_t Tare)
{
    float32_t Discriminant;
    float32_t Root1;
    float32_t Root2;
    float32_t SqrtDiscriminant;
    bool ValidCoeff;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();
    ValidCoeff = false;

    do
    {
        if (NULL == pRoot)
        {
            break;
        }

        //Test that there are coeficients present
        if (((pEgia->CalibParam.StrainGauge.SecondOrder == 0) && (pEgia->CalibParam.StrainGauge.Multiplier == 1) && (pEgia->CalibParam.StrainGauge.Offset == 0)) || \
                ((pEgia->CalibParam.StrainGauge.Multiplier  == 0) && (pEgia->CalibParam.StrainGauge.SecondOrder == 0)))
        {
            break; //default adc counts not allowed, multiplier of zero is illegal
        }

        if (isnan(pEgia->CalibParam.StrainGauge.Multiplier) || \
                isnan(pEgia->CalibParam.StrainGauge.Offset) || \
                isnan(pEgia->CalibParam.StrainGauge.SecondOrder))
        {
            break;
        }

        if (pEgia->CalibParam.StrainGauge.SecondOrder != 0)
        {
            // 2nd Order Coef Test
            Discriminant =  (pEgia->CalibParam.StrainGauge.Multiplier * pEgia->CalibParam.StrainGauge.Multiplier) - \
                            (4.0 * pEgia->CalibParam.StrainGauge.SecondOrder * pEgia->CalibParam.StrainGauge.Offset);

            if (Discriminant == 0)
            {
                Root1 = (-1.0 * pEgia->CalibParam.StrainGauge.Multiplier) / (2.0 * pEgia->CalibParam.StrainGauge.SecondOrder);

                // the Root is the adc counts at zero force, it can never be  < 0 and is not allowed to be 0
                if (Root1 > 0)
                {
                    *pRoot = Root1;
                    ValidCoeff = true;    //the coefficients are valid
                }
            }
            else if (Discriminant > 0)
            {
                // there are two real roots
                SqrtDiscriminant = sqrt(Discriminant);
                Root1 = ((-1.0 * pEgia->CalibParam.StrainGauge.Multiplier) + SqrtDiscriminant) / (2.0 * pEgia->CalibParam.StrainGauge.SecondOrder);
                Root2 = ((-1.0 * pEgia->CalibParam.StrainGauge.Multiplier) - SqrtDiscriminant) / (2.0 * pEgia->CalibParam.StrainGauge.SecondOrder);

                if ((Root1 > 0) && (Root2 > 0))
                {
                    *pRoot = (fabs((float32_t)Tare - Root1) > fabs((float32_t)Tare - Root2)) ? Root2 : Root1;
                    ValidCoeff = true;    //the coefficients are valid
                }
                else
                {
                    *pRoot = (Root1 > 0) ? Root1 : Root2;
                    ValidCoeff = true;    //the coefficients are valid
                }
            }
            else
            {
                //Discriminant < 0
                //there are no real number roots, not allowed
                //nothing to do
            }
        }
        else
        {
            // Linear Order Coef Test
            Root1 = (-1.0 * pEgia->CalibParam.StrainGauge.Offset) / pEgia->CalibParam.StrainGauge.Multiplier;
            Log(REQ,"EGIAUtil: SG,Offset= %3.4f, Multiplier= %3.4f, ZerCnt(Root1)= %3.4f", pEgia->CalibParam.StrainGauge.Offset,
                pEgia->CalibParam.StrainGauge.Multiplier, Root1);
            // the root is the adc counts at zero force, it can never be  <0 and is not allowed to be 0
            if (Root1 > 0)
            {
                *pRoot = Root1;
                ValidCoeff = true;    //the linear coefficients are valid
            }
        }
    } while (false);

    return ValidCoeff;
}

/* ========================================================================== */
/**
 * \brief   Evaluates EGIA Firing pre-conditions
 *
 * \details This function checks the pre-condition to enter Firing.
 *          Dissallow firing if the pre-condition fails
 *
 * \note    ///\todo 07/10/2022 - NP All Pre Condition to Fire needs to be relooked
 *
 * \param   me  - me Pointer
 *
 * \return  PrecondToFire
 * \retval  false - Pre-Condition failed
 * \retval  true - All Pre-Conditions Passed
 *
 * ========================================================================== */
bool EGutil_IsOKToFire(EGIA *const me)
{
    bool PrecondToFire;
    uint16_t AdapterFireCount;          /* Adapter Fire Count Read from Adapter */
    uint16_t AdapterFireLimit;          /* Adapter Fire Limit Read from Adapter */
    uint16_t AdapterProcedureCount;     /* Adapter Procedure Limit Read from Adapter */
    uint16_t AdapterProcedureLimit;     /* Adapter Procedure Limit Read from Adapter */
    uint16_t HandleProcedureCount;      /* Handle Procedure Count Read from Handle */
    uint16_t HandleProcedureLimit;      /* Handle Procedure Limit Read from Handle */
    uint8_t ClamshellStatusFlags;       /* Clamshell Status Read from Clamshell */
    uint8_t ReloadFireCount;            /* Reload fire count */
    uint8_t ReloadFireLimit;            /* Reload Fire Limit */
    uint8_t CartridgeFireCount;         /* Cartridge Fire Count */
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    /* Read the Adapter Fire Count &  Fire Limit */
    DeviceMem_READ(ME->Adapter, FireCount, AdapterFireCount);
    DeviceMem_READ(ME->Adapter, FireLimit, AdapterFireLimit);
    DeviceMem_READ(ME->Adapter, ProcedureCount, AdapterProcedureCount);
    DeviceMem_READ(ME->Adapter, ProcedureLimit, AdapterProcedureLimit);

    /* Read the Handle Procedure Count & Limit */
    DeviceMem_READ(ME->Handle, ProcedureCount, HandleProcedureCount);
    DeviceMem_READ(ME->Handle, ProcedureLimit, HandleProcedureLimit);

    /* Read the Clamshell Status */
    DeviceMem_READ(ME->Clamshell, StatusFlags, ClamshellStatusFlags);
    DeviceMem_READ(ME->Reload, FireCount, ReloadFireCount);
    DeviceMem_READ(ME->Reload, FireLimit, ReloadFireLimit);
    pEgia->BatteryLevel = ME->pChargerInfo->BatteryLevel;

    PrecondToFire = false;

    do
    {
        //Handle 1W Bus Shorted.
        if(true == ME->Handle.HandleBusShort)
        {
            // Play Caution Tone
            Signia_PlayTone(SNDMGR_TONE_CAUTION);
            Log(DBG,"EGIAutil: Fire Mode Denied: Handle Main 1W Bus Short");
            break;
        }

        // Check if clamshell is already used or not
        if (true == ME->Clamshell.ClamshellEOL)
        {
            // Caution Tone
            Signia_PlayTone(SNDMGR_TONE_CAUTION);
            Log(DBG,"EGIAutil: Fire Mode Denied, used clamshell");
            break;
        }

        if (ME->ActiveFaultsInfo.IsPermFailWop)
        {
            Log(DBG,"EGIAutil: Fire Mode Denied, Handle Permanent Failure");
            break;
        }

        if (ME->ActiveFaultsInfo.IsErrShell)
        {
            Log(DBG, "Fire Mode Denied: Clamshell Error");
            break;
        }

        /* Check for Battery Level before Fire Mode Entry - Req-ID 327468 */
        if (EGIA_BAT_INSUFFICIENT >= pEgia->BatteryLevel)
        {
            // Screen Insufficient Battery Screen
            Gui_InsufficientBattery_Screen(ReloadFireCount, pEgia->ReloadLen);
            Log(DBG,"EGIAutil: Fire Mode Denied, Battery Insufficient");
            break;
        }

        // Non-Intelligent SULU and Clamshell Used don't allow
        if ((ME->Clamshell.ClamshellEOL) && (RELOAD_NONINTELLIGENT == pEgia->ReloadType))
        {
            //Gui_UsedReload_ScreenAndLock(pEgia->ReloadLen,true);
            NonIntReloadScreenProgress(RELOAD_NON_INT_CONNECT,NOT_USED, true, false);
            Log(DBG,"EGIAutil: Fire Mode Denied, Dumb SULU with Used Clamshell");
            break;
        }

        // Fresh SULU and Clamshell Used don't allow
        if ((ME->Clamshell.ClamshellEOL) && (RELOAD_SULUINTELLIGENT == pEgia->ReloadType))
        {
            //Gui_UsedReload_ScreenAndLock(pEgia->ReloadLen,true);
            IntReloadScreenProgress(RELOAD_INT_CONNECT, pEgia->ReloadLen, pEgia->ReloadCartColor, true, false);
            Log(DBG,"EGIAutil: Fire Mode Denied, SULU with Used Clamshell");
            break;
        }

        if ((RELOAD_SULUINTELLIGENT == pEgia->ReloadType) && (ReloadFireCount > 0))
        {
            /* Don't Allow firing if SULU Reload is used */
            // Screen Used Reload and Lock until Reload is removed
            Gui_UsedReload_ScreenAndLock(false);
            Log(DBG,"EGIAutil: Fire Mode Denied, Used SULU");
            break;
        }

        if ( (RELOAD_NONINTELLIGENT == pEgia->ReloadType) && (pEgia->NonIntReloadEOL) )
        {
            Gui_UsedReload_ScreenAndLock(false);
            Log(DBG,"EGIAutil: Fire Mode Denied, Used Dumb SULU");
            break;
        }

        // Disallow entry into Fire mode if the SULU 1-WIRE write test fails
        if ( (RELOAD_SULUINTELLIGENT == pEgia->ReloadType) && (!pEgia->ReloadDeviceWriteStatus) )
        {
            Log(DBG,"EGIAutil: Fire Mode Denied, SULU failed 1-Wire write test");
            break;
        }

        /* If Adapter EOL, Do not enter FIRE MODE */
        if (ME->Adapter.AdapterEOL)
        {
            Log(DBG,"EGIAutil: Fire Mode Denied, Adapter EOL");
            break;
        }

        /* Check Handle Procedure and Fire Count Status before entering into firing */
        /* Req : 349978, 349980 */
        if (ME->HandleProcFireCountTestFailed)
        {
            Log(DBG,"EGIAutil: Fire Mode Denied, Handle Procedure or Fire Count test failed");
            break;
        }

        if (RELOAD_MULUINTELLIGENT == pEgia->ReloadType)
        {
            /* Is MULU Fire Count test failed */
            if (!pEgia->MuluFireCountTestPass)
            {
                Log(DBG,"EGIAutil: Fire Mode Denied, MULU fire count test failed");
                break;
            }

            /* Is Used Cartridge test failed */
            if (!pEgia->UsedCartridgeTestPass)
            {
                Log(DBG,"EGIAutil: Fire Mode Denied, Used cartridge test failed");
                break;
            }

            /* Allow firing if MULU Fire count is within Fire limit */
            if (ReloadFireCount >= ReloadFireLimit)
            {
                Log(DBG,"EGIAutil: Fire Mode Denied, MULU fire count exceeded");
                break;
            }

            /* Read Cartridge FireCount */
            if (ReloadFireCount >= ReloadFireLimit)
            {
                Log(DBG,"EGIAutil: Fire Mode Denied, MULU fire count exceeded");
                break;
            }

            DeviceMem_READ(ME->Cartridge, FireCount, CartridgeFireCount);
            /* Allow firing if Cartridge is not used */
            if (CartridgeFireCount > 0)
            {
                Log(DBG,"EGIAutil: Fire Mode Denied, MULU Cartridge Used");
                break;
            }
        }

        /// \ todo 05/12/2022 AN For now updating AdapterTestPass to true. AdapterTestPass gets updated from AdapterFiringAutoclaveCounterTest api.
        /// \ todo Currently no call is made to this API. Once AdapterFiringAutoClave API is called remove this
        pEgia->AdapterTestPass = true;
        if (!pEgia->AdapterTestPass)
        {
            Log(DBG,"EGIAutil: Fire Mode Denied, Adapter Procedure or Fire count test failed");
            break;
        }

        //[ID:EGIA-SRS 028]: do not allow entry to fire mode if adapter rx comms lost.
        if ( (pEgia->SGForce.Status != SG_STATUS_GOOD_DATA) || (!pEgia->SGForce.NewDataFlag) )
        {
            Log(REQ,"EGIAutil: Fire Mode Denied, Adapter UART Rx error");
            break;
        }

        if (FIRE_COUNT_UPDATE_SUCCESS < pEgia->HandleFireCountUpdated)
        {
            Log(DBG,"EGIAutil: Fire Mode Denied, Handle Fire Count update failed");
            break;
        }

        if (FIRE_COUNT_UPDATE_SUCCESS < pEgia->AdapterProcCountUpdated) /// \todo do not use FIRE_COUNT for Procedure Count
        {
            Log(DBG,"EGIAutil: Fire Mode Denied, Adapter Procedure Count update failed");
            break;
        }

        if (FIRE_COUNT_UPDATE_SUCCESS < pEgia->AdapterFireCountUpdated)
        {
            Log(DBG,"EGIAutil: Fire Mode Denied, Adapter Fire Count update failed");
            break;
        }

        /* Do not enter fire mode if Battery Temperature Error is Set */
        if (ME->ActiveFaultsInfo.IsBattTempError)
        {
            Log(DBG,"EGIAutil: Fire Mode Denied, Battery Temperature Error");
            break;
        }

        if(ME->ActiveFaultsInfo.IsAccelErr)
        {
            // 253484: HANDLE software shall allow all operations except entry into FIRE_MODE while in the MOO_ERR_ACCEL condition.
            Log(DBG,"EGIAutil: Fire Mode Denied, Accelerometer Error");
            break;
        }

        PrecondToFire = true;

    } while (false);

    return PrecondToFire;
}

/* ========================================================================== */
/**
 * \brief   Load default strain gauge parameters
 *
 * \details This function loads the default EGIA adapter calib coefficients
 *
 * \param   < None >
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_LoadDefaultCalibParams(void)
{
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    pEgia->CalibParam.StrainGauge.Multiplier = STRAIN_GAUGE_GAIN_DEFAULT;
    pEgia->CalibParam.StrainGauge.Offset = STRAIN_GAUGE_OFFSET_DEFAULT;
    pEgia->CalibParam.StrainGauge.SecondOrder = STRAIN_GAUGE_2ND_ORDER_DEFAULT;

    pEgia->CalibParam.CalibParam.ArticCalTurns = EGIA_ARTIC_CAL_TURNS;
    pEgia->CalibParam.CalibParam.ClampTurns = EGIA_CLAMP_TURNS;
    pEgia->CalibParam.CalibParam.FirerodBacklashTurns = EGIA_FIRE_BL_CAL_TURNS;
    pEgia->CalibParam.CalibParam.FirerodCalTurns = EGIA_FIRE_CAL_TURNS;
    pEgia->CalibParam.CalibParam.MaxLeftTurns = EGIA_ARTIC_LEFT_TURNS;
    pEgia->CalibParam.CalibParam.MaxRightTurns = EGIA_ARTIC_RIGHT_TURNS;
    pEgia->CalibParam.CalibParam.MaxRotateTurns = EGIA_ROTATE_TURNS;

    memset(pEgia->CalibParam.Lot_number, 0x00, ADAPTER_LOT_CHARS);
    pEgia->CalibParam.BoardParam.TareDriftHigh = STRAIN_GAUGE_TARE_HIGH_COUNT_DRIFT; // maximum positive drift allowable to tare off
    pEgia->CalibParam.BoardParam.TareDriftLow = STRAIN_GAUGE_TARE_LOW_COUNT_DRIFT;   // max negative drift allowable to tare off
    pEgia->CalibParam.BoardParam.ZBCountCeiling = TARE_COUNT_CEILING;                // maximum value for zero pound count (before tare at rod calibration)
    pEgia->CalibParam.BoardParam.ZBCountFloor = TARE_COUNT_FLOOR;                    // maximum value for zero pound count (before tare at rod ca

    pEgia->CalibrationTareCounts = 0;

    pEgia->CoefficientsStatus = false;
}

/* ========================================================================== */
/**
 * \brief   Republish the Deferred Signals
 *
 * \details This function re-publishes the Signal which are deferred
 *
 * \param   pMe - Me Pointer
 *
 * \b Returns void
 *
 * ========================================================================== */
/// \todo David Zeichner on 2022-05-13 Why bother with a function? Just put call to AO_Recall() in appropriate place.
void EGutil_RepublishDeferredSig(Handle *const pMe)
{
    /* Re-Publish signals which are placed in Defer Queue*/
    bool Status;

    Status = false;

    do
    {
        /* Republishes the Signals which are deferred */
        Status = AO_Recall(&pMe->super, &pMe->DeferQueue);
    } while (Status);
}

/* ========================================================================== */
/**
 * \brief   Determine Adapter End of Life
 *
 * \details This function Display Adapter EOL and Plays Caution Tone.
 *          Adapter EOL Conditions:
 *              - Fire Count >= Fire Limit or
 *              - Adapter is not one of 10 adapters used,
 *                Procedure Count >= Procedure Limit,
 *                Procedure limit != 0
 *
 * \param   pMe - Me Pointer to Handle data
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_ProcessAdapterEOL(Handle *const pMe)
{
    uint16_t AdapterFireCount;
    uint16_t AdapterFireLimit;
    uint16_t AdapterProcedureCount;
    uint16_t AdapterProcedureLimit;
    uint16_t HandleProcedureCount;
    uint16_t HandleProcedureLimit;
    uint8_t  ClamshellStatusFlags;
    bool     ErrorCondition;
    bool     EOLCondition;

    /* Read the Adapter Fire Count &  Fire Limit */
    DeviceMem_READ(pMe->Adapter, FireCount, AdapterFireCount);
    DeviceMem_READ(pMe->Adapter, FireLimit, AdapterFireLimit);
    DeviceMem_READ(pMe->Adapter, ProcedureCount, AdapterProcedureCount);
    DeviceMem_READ(pMe->Adapter, ProcedureLimit, AdapterProcedureLimit);

    /* Read the Handle Procedure Count & Limit */
    DeviceMem_READ(pMe->Handle, ProcedureCount, HandleProcedureCount);
    DeviceMem_READ(pMe->Handle, ProcedureLimit, HandleProcedureLimit);

    /* Read the Clamshell Status */
    DeviceMem_READ(pMe->Clamshell, StatusFlags, ClamshellStatusFlags);

    /* Check the Adapter Fire count with in limits, Check if the adapter is not is not one of the last 10 adapters detected
    in the current procedure and the ADAPTER has a procedure limit not equal to zero */
    if ((AdapterFireCount >= AdapterFireLimit) || ((!HNutil_IsAdapterPresentInDeviceList(pMe->Adapter.DevID)) && (AdapterProcedureCount >= AdapterProcedureLimit && AdapterProcedureLimit != 0)))
    {
        pMe->Adapter.AdapterEOL = true;

        //Display the End Of Life Adapter screen.
        Gui_EndOfLifeAdapter_Screen(AdapterFireCount);
        Log(REQ, "EGIAUtil: Adapter End of Life");

        /* Handle Clamshell Error Conditions */
        ErrorCondition = (pMe->ActiveFaultsInfo.IsFileSysErr || pMe->ActiveFaultsInfo.IsAccelErr || pMe->ActiveFaultsInfo.IsPermFailWop || pMe->ActiveFaultsInfo.IsErrShell);

        /* Handle, Clamshll EOL condition */
        EOLCondition   = (pMe->Clamshell.ClamshellEOL || pMe->Handle.HandleEOL);

        // Play caution ton if Handle Clamshell are not in Error Condition, not in EOL condition
        if (!ErrorCondition && !EOLCondition)
        {
            //Play the Caution Tone, if the HANDLE or CLAMSHELL and nther is NOT in ERROR or END_OF_LIFE condition.
            Log(DBG, "EGIAUtil: HANDLE or CLAMSHELL are not in ERROR or END_OF_LIFE condition");
            Signia_PlayTone(SNDMGR_TONE_CAUTION);
        }
    }
}

/* ========================================================================== */
/**
 * \brief   This function checks for a used cartridge.
 *
 * \details If a cartridge is found to be used, the Used Cartridge Screen is
 *          displayed, and a caution tone is issued.
 *
 * \param   pMe - Me Pointer to Handle data
 *
 * \return  true if cartridge is used, false if unused
 *
 * ========================================================================== */
bool EGutil_CheckUsedCartridge(Handle *const pMe)
{
    uint8_t CartridgeFireCount;
    uint8_t ReloadFireCount;
    uint8_t ReloadFireLimit;
    bool Status;

    Status = false;
    CartridgeFireCount = 0;

    /* check if Cartridge already used */
    DeviceMem_READ(pMe->Cartridge, FireCount, CartridgeFireCount);
    if ((CARTRIDGE_NOT_USED != CartridgeFireCount) && (pMe->Cartridge.Status == AM_DEVICE_CONNECTED))
    {
        Log(DBG, "EGIAUtil: Used Cartridge ");

        /* Display used cartridge screen and play caution tone */
        DeviceMem_READ(pMe->Reload, FireCount, ReloadFireCount);
        DeviceMem_READ(pMe->Reload, FireLimit, ReloadFireLimit);
        Gui_Used_Cartridge_ScreenLock(ReloadFireLimit - ReloadFireCount);
        Signia_PlayTone(SNDMGR_TONE_CAUTION);
        Status = true;
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Callback Function for ASA handling, invoked on Motor tick depending upon Force
 *
 * \details Callback Function for ASA handling, Function to calculate Tick Moved for firing progression indication,
 *
 * \note    pEgia->FiringMaxForceRead is read during UpdateAsaForceToSpeedTable
 *
 * \param   pMotor - pointer to Motor Info
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_ASAUpdateCallBack(MOTOR_CTRL_PARAM *pMotor)
{
    uint16_t Speed;
    float32_t TicksMoved;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    do
    {
        /* REQ ID ; 334035 - Adapter UART Receive Comm Error while in Fire Mode, then Allow Firing */
        // Calculate the distance travelled by the Knife for Reload Recognition Bar During Firing Req ID:327483/327481
        TicksMoved = (abs(pMotor->MotorPosition) - pEgia->AsaInfo.StartTicks);
        TicksMoved = TicksMoved / pEgia->AsaInfo.TotalTicks;
        pEgia->AsaInfo.FiringPercentageComplete = (uint8_t)(TicksMoved * VALUE_100);

        // Strain Gauge ADC count is out of range and Req ID :327613(UART Error in between)
        if ((pEgia->AsaInfo.SGOutOfRangeSet) || (pEgia->AsaInfo.SGLost))
        {
            break;
        }

        // If Force received is greater than Max Force Read from Reload/Cartridge - 318719/318722/327605
        if (pEgia->SGForce.ForceInLBS >= pEgia->FiringMaxForceRead)
        {
            // Stop the Motor - ExternalProcess will De-Registered in Motor Manager
            pMotor->StopStatus |= MOT_STOP_STATUS_STRAINGAGE;
            break;
        }

        // Get the Speed and check if it needs to reduced (Valid Range)
        Speed = GetSpeedFromASATable(pEgia->SGForce.ForceInLBS);

        // Strain Gauge Data Out Of Range during  Firing - 318723
        if (pEgia->SGForce.Current > MAX_SG_COUNT)
        {
            // Run the motor with SLOW speed
            Speed = ForceToSpeedTable[FIRINGSPEED_SLOW].FiringSpeed;
            pEgia->AsaInfo.FiringRpm = Speed;
            pEgia->AsaInfo.FiringState = FIRINGSPEED_SLOW;
            pEgia->AsaInfo.SGOutOfRangeSet = true;
        }

        if (Speed < pMotor->TargetShaftRpm)
        {
            Log(DBG, "EGIAUtil: Fire Motor Speed Varied From %d to %d", pMotor->TargetShaftRpm, Speed);
            Signia_MotorUpdateSpeed(FIRE_MOTOR, Speed, MOTOR_VOLT_15);
            FiringProgress(pEgia, pEgia->AsaInfo.FiringState);
        }
    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Callback Function for Maximum Clamp force update while full close, invoked on Motor tick
 * \details Callback Function for Maximum Clamp force, Function to store maximum clamp force for the initial firing speed
 *
 * \param   pMotor - pointer to Motor Info
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGutil_UpdateMaxClampForceCallBack(MOTOR_CTRL_PARAM *pMotor)
{
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();

    if (pEgia->MaxClampForce < pEgia->SGForce.ForceInLBS)
    {
        pEgia->MaxClampForce = pEgia->SGForce.ForceInLBS;
    }
}
/* ========================================================================== */
/**
 * \brief   Function to Stop Rotation or Articulation on Multi Key Press
 * \details Upon any second or multi Key press, if Articulation or Rotation or Clamp are in progress then stops the Artic and Rotation Motor
 *
 * \param   KeyId
 * \param   KeyState
 *
 * \return  Status
 * \retval  true  - Multi Key Press, stop Articulation, Rotation
 * \retval  false - Single Key Press, allow Articulation or Rotation
 *
 * ========================================================================== */
bool EGUtil_StopRotArtOnMultiKey( KEY_ID KeyId, uint16_t KeyState )
{
    bool Status;
    uint16_t KeyPos;

    KeyPos = ( 1<<KeyId );

    /* true - Multi Key Press, false- One Key Pressed */
    Status = ( KeyState  != KeyPos );

    //Rotation or Articulation or Clamping already in progress
    if ( Status )
    {
        Signia_MotorStop(ARTIC_MOTOR);
        Signia_MotorStop(ROTATE_MOTOR);
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Function to Start Articulation
 * \details This function start Articulation when Toggle left or right key is pressed. Any multiple key press will not start the Articulation
 * \details Allows articulation from Left to Center or Right to Center
 *
 * \param   KeyId   - Key ID
 * \param   KeyState - Individual Key State
 *
 * \b Returns void
 *
 * ========================================================================== */
void EGUtil_StartArticulation( KEY_ID KeyId, uint16_t KeyState )
{
    int32_t ArtPos;
    APP_EGIA_DATA *pEgia;

    pEgia = EGIA_GetDataPtr();
    do
    {
        /* Check multi Key press*/
        BREAK_IF(EGUtil_StopRotArtOnMultiKey(KeyId, KeyState));
        Signia_MotorGetPos(ARTIC_MOTOR, &ArtPos );

        if ( KeyId == TOGGLE_LEFT )
        {
            /* When Toggle Left Pressed, and Articulation Position is less than ARTIC Error, move to center position else move to FULL LEFT POS */
            ArtPos = ( ArtPos < (-ARTIC_ERROR) )? (int32_t)ARTIC_CENTER: (int32_t)ARTIC_FULL_LEFT_POS;
        }
        else
        {
            /* When Toggle Right Pressed, and Articulation Position is greater ARTIC Error, move to center position else move to FULL RIGHT POS */
            ArtPos = ( ArtPos > ARTIC_ERROR )? (int32_t)ARTIC_CENTER: (int32_t)ARTIC_FULL_RIGHT_POS;
        }
        /// \todo 03/21/2022 KA: revisit how to set the speed based on fully clamped condition
        if ( pEgia->ArticAllowed )
        {
            EGutil_UpdateArticulation(MOTOR_START,
                                      (int32_t)ArtPos,
                                      (EGIA_CLAMPCYCLE_CLAMPING_CLOSE == pEgia->ClampCycle) ? ARTIC_SHAFT_RPM_CLAMPED : ARTIC_SHAFT_RPM);
        }
    } while ( false );
}

/* ========================================================================== */
/**
 * \brief   Process Stop Status for FPGA Reset Check
 *
 * \details This function checks FPGA Reset Status while Opening/Closing/Articulating
 *
 * \note    pEgia->FiringComplete is set once the handle completes the firing
 *          i.e. open button pressed during fire mode
 * \param   e - Pointer to event
 *
 * \return  status - false for FPGA Error/True if No FPGA Error
 *
 * ========================================================================== */
bool EGutil_IsFPGAReset(QEvt const * const e)
{
    bool Status;
    int32_t TempPos;
    APP_EGIA_DATA *pEgia;
    QEVENT_MOTOR_STOP_INFO   *pStopInfo; // Pointer to motor stop info signal

    pEgia = EGIA_GetDataPtr();
    Status = false;
    pStopInfo = (QEVENT_MOTOR_STOP_INFO *)e;

    /* Not in Fire Mode - Req ID: 344204 */
    if (!pEgia->FiringComplete)
    {
        if (pStopInfo->StopStatus & MOT_STOP_STATUS_FPGA_SPI)
        {
            //Gui_FPGAErrorScreen();
            Signia_PlayTone(SNDMGR_TONE_CAUTION);    //Play Caution Tone
            Status = true;
        }
    }
    else
    {
        /* In Fire Mode - Req ID: 344206 */
        pEgia->FiringComplete = false;
        Signia_MotorGetPos(FIRE_MOTOR, &TempPos);
        if ((pEgia->FPGAResetFireMode) && (abs(FIRE_FULL_OPEN_POS) > (abs(TempPos) - (MOT_POSITION_TOLERANCE * MULTIPLY_TWO))))
        {
            pEgia->FPGAResetFireMode = false;

            //Gui_FPGAErrorScreen();
            Signia_PlayTone(SNDMGR_TONE_CAUTION);    //Play Caution Tone
            Status = true;
        }
    }
    return Status;
}

/* ========================================================================== */
/**
 * \brief Get Current limit profile for the reload
 *
 * \details Gets the Current limit profile based on the Reload type and Articulation angle
 *
 * \param   pMe - Me Pointer
 * \param   MotorITripProf - pointer for Currnet profile
 *
 * \return  true if profile found, false if profile not found
 *
 * ========================================================================== */
bool EGutil_GetCurrentLimitProfile(Handle *const pMe, MOT_CURTRIP_PROFILE *MotorITripProf)
{
    uint8_t Index;
    uint8_t LoopIndex;
    int32_t ArticPosition;
    float Turns;
    bool Status;


    Status = false;
    // get Index from current profile table based on Reload ID
    Index = EGutil_GetIprofIndex(pMe->Reload.DevID);

    do
    {
        // Check if Index is in valid range
        BREAK_IF( EGIA_IPROF_TABLE_INDEX_COUNT <= Index);

        // Get Articulation postion of the motor in motor turns from Hardstop
        Signia_MotorGetPos(ARTIC_MOTOR, &ArticPosition);
        ArticPosition -= ARTIC_CAL_HARDSTOP_POS;
        Turns = fabs((float)ArticPosition /(HANDLE_PARAM_GEAR_RATIO * HANDLE_PARAM_TICKS_PER_REV));
        for ( LoopIndex = 0; LoopIndex < ReloadIProfileTable[Index].clprof_artic->num_entries; LoopIndex++ )
        {
            // get the current profle table for the Reload and the Articulation angle.
            if ( Turns <= ReloadIProfileTable[Index].clprof_artic->artic_position[LoopIndex])
            {
                MotorITripProf->NumEntries = ReloadIProfileTable[Index].clprof_artic->clprof_tables[LoopIndex]->NumEntries;
                memcpy(MotorITripProf->CurrentTrip, ReloadIProfileTable[Index].clprof_artic->clprof_tables[LoopIndex]->CurrentTrip, MOT_MAX_CURLIMIT_ENTRIES * sizeof(ReloadIProfileTable[Index].clprof_artic->clprof_tables[LoopIndex]->CurrentTrip[0]) );
                memcpy(MotorITripProf->Kcoeff, ReloadIProfileTable[Index].clprof_artic->clprof_tables[LoopIndex]->Kcoeff, MOT_MAX_CURLIMIT_ENTRIES * sizeof(ReloadIProfileTable[Index].clprof_artic->clprof_tables[LoopIndex]->Kcoeff[0]) );
                memcpy(MotorITripProf->Method, ReloadIProfileTable[Index].clprof_artic->clprof_tables[LoopIndex]->Method, MOT_MAX_CURLIMIT_ENTRIES * sizeof(ReloadIProfileTable[Index].clprof_artic->clprof_tables[LoopIndex]->Method[0]) );
                memcpy(MotorITripProf->TurnsPosition, ReloadIProfileTable[Index].clprof_artic->clprof_tables[LoopIndex]->TurnsPosition, MOT_MAX_CURLIMIT_ENTRIES * sizeof(ReloadIProfileTable[Index].clprof_artic->clprof_tables[LoopIndex]->TurnsPosition[0]) );
                memcpy(MotorITripProf->ZoneID, ReloadIProfileTable[Index].clprof_artic->clprof_tables[LoopIndex]->ZoneID, MOT_MAX_CURLIMIT_ENTRIES * sizeof(ReloadIProfileTable[Index].clprof_artic->clprof_tables[LoopIndex]->ZoneID[0]) );

                Status = true;
                break;
            }
        }
    } while (false);
    return Status;
}

/* ========================================================================== */
/**
 * \brief  Returns the Max fire turns
 *
 * \details Returns the Max fire turns based on the Reload type
 *
 *
 * \param   pMe - Me Pointer
 *
 * \return  FireSpeed
 *
 * ========================================================================== */
int32_t EGutil_GetMaxFireTurns(Handle *const pMe)
{
    uint8_t Index;
    int32_t TurnsInTick;
    Index = EGutil_GetIprofIndex(pMe->Reload.DevID);
    TurnsInTick = (int32_t)(EgiaMaxFireTurns.maxturns[Index] * TICKS_PER_TURN);
    return TurnsInTick;
}


/* ========================================================================== */
/**
 * \brief  Process the Rotation Request
 *
 * \details This fucntion is called when ever a Rotation button is pressed.
 *          Function Checks and executes the Rotation of EGIA adapter or enters the
 *          Rotation configuraiton based on the pressed keys
 *
 * \param   e   - pointer to event
 * \param   pMe - pointer to Handle structure
 *
 * \b Returns none
 *
 * ========================================================================== */
void EGUtil_ProcessRotationRequest( QEvt const * const e, Handle *const pMe )
{
    do
    {
        if ( P_LATERAL_LEFT_UP_RELEASE_SIG == e->sig || P_LATERAL_RIGHT_DOWN_RELEASE_SIG == e->sig ||
                P_LATERAL_LEFT_DOWN_RELEASE_SIG == e->sig || P_LATERAL_RIGHT_UP_RELEASE_SIG == e->sig )
        {
            // Stop the motor if any of the Rotation key is released
            EGutil_UpdateRotation(MOTOR_STOP, NULL, NULL);
        }
        // Rotation configuration state machine
        switch ( pMe->RotationConfig.RotationConfigState )
        {
            case ROTATION_STOPPED:

                if(!pMe->RotationConfig.IsScreenCaptured)
                {
                    // Check if the active screen is captures or not, if not capture the active screen
                    // This is needed to normalize the screen after rotation configuration
                    pMe->RotationConfig.IsScreenCaptured = true;
                    L4_CopyCurrentScreen();
                }
                EGUtil_ProcessRotationStopped(e,pMe,&pMe->RotationConfig.RotationConfigState);
                break;

            case ROTATION_DEBOUNCE:
                // 100ms debounce time after a rotation key press detected and before taking any action
                EGUtil_ProcessRotationDebounce(e,pMe,&pMe->RotationConfig.RotationConfigState);
                break;

            case ROTATION_CONFIG:
                EGUtil_ProcessRotationConfig(e,pMe,&pMe->RotationConfig.RotationConfigState);
                break;

            case ROTATION_DISPLAYWAIT:
                // Wait time to finish the screen display before returning from Rotation configuration
                EGUtil_RotationConfigStop( &pMe->RotationConfig.RotationConfigState, pMe );
                break;

            case ROTATION_INPROGRESS:
                EGUtil_ProcessRotationInProgress(e,pMe,&pMe->RotationConfig.RotationConfigState);
                break;

            default:
                break;

        }

    } while (false);

}

/* ========================================================================== */
/**
 * \brief  Initiliazie Roation configuraiton state
 * \details This Api clears the boolean flag in rotaiton configuraiton and initilizes the
 *          the Rotation state to Stopped
 *
 * \param   pMe - Constant pointer to Handle structure
 *
 * \b Returns none
 *
 * ========================================================================== */
void EGUtil_InitRotationConfig(Handle *const pMe)
{
    pMe->RotationConfig.IsScreenCaptured = false;
    pMe->RotationConfig.RotationConfigState = ROTATION_STOPPED;
}




/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
