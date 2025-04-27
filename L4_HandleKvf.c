#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Kvf
 * \{
 *
 * \brief   Handle Definition functions
 *
 * \details The Handle Definition defines all the interfaces used for
            communication between Handle and Handle.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_HandleKvf.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_HandleKvf.h"  
#include "Kvf.h"
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_FILE_SYS) ///< Log Group Identifier

#if USE_KVF_VALUES
#define HANDLE_KVF_FILE  "\\settings\\handle.kvf"
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static const KVF_ENUM_ITEM HandleModeItems[] = {
  { "AUTO",          HANDLE_MODE_AUTO        },
  { "EGIA",          HANDLE_MODE_EGIA        },
  { "EEA",           HANDLE_MODE_EEA         },
  { "ASAP",          HANDLE_MODE_ASAP        },
  { "ASAP FLEX",     HANDLE_MODE_ASAP_FLEX   },
  { "ASAP 360",      HANDLE_MODE_ASAP_360    },
  { "ASAP PLAN",     HANDLE_MODE_ASAP_PLAN   },
  { "EEA FLEX",      HANDLE_MODE_EEA_FLEX    },
  { "Dyno",          HANDLE_MODE_DYNO        },
  { "EGIA REL",      HANDLE_MODE_EGIA_REL    },
  { "EEA REL",       HANDLE_MODE_EEA_REL     },
  { "DEBUG REL",     HANDLE_MODE_DEBUG_REL   },
  { "RDF Playback",  HANDLE_MODE_RDF_PLAYBACK},
  { "Piezo Demo",    HANDLE_MODE_PIEZEO_DEMO },
  { "ABS",           HANDLE_MODE_ABS         },
  { "ABSA",          HANDLE_MODE_ABSA        },
  { "HAND CAL",      HANDLE_MODE_HAND_CAL    },
  { "REL",           HANDLE_MODE_REL         },
  { "ES",            HANDLE_MODE_ES          },
  { "Test Fixture",  HANDLE_MODE_TEST_FIXTURE},
  { NULL,         0                     }
};

#define HANDLE_MODE_COUNT  ((sizeof(HandleModeItems) / sizeof(HandleModeItems[0])) - 1)

static const KVF_ENUM_ITEM TicksPerRevItems[] = {
  { "6",          6  },
  { "12",         12 },
  { NULL,         0  }
};

static const KVF_ENUM_ITEM GearRatioItems[] = {
  { "33.64 : 1",  1 },
  { "25.00 : 1",  2 },
  { "29.00 : 1",  3 },
  { NULL,         0 },
};

static const KVFInt32u LogPeriod               = { 10, 1,  100 };
static const KVFEnum   HandleMode              = { 1, HandleModeItems };
static const KVFEnum   TicksPerRev             = { 12, TicksPerRevItems };
static const KVFEnum   GearRatio               = { 2, GearRatioItems };
static const KVF_BOOL   PowerSave               = { false };
static const KVF_BOOL   PiezoEnable             = { true };
static const KVF_BOOL   FatalErrorEnable        = { true };
static const KVF_BOOL   IsC3BoardKVF            = { false };
static const KVF_BOOL   IsMotorlesssC3BoardKVF  = { false };
static const KVF_BOOL   DisableFireCountsKVF    = { false };
static const KVF_BOOL   DisableSleepModesKVF    = { false };

static const KVF_MAP HandleKvfMap[] = {
//  type,            *pKeyString,               *pObject,               *pDescription
  { VAR_TYPE_ENUM,   "handle Mode",             &HandleMode,            "board reset required when changing" },
  { VAR_TYPE_INT32U, "DataLog Period",          &LogPeriod,             "Data Logging Period (mSec)" },
  { VAR_TYPE_ENUM,   "Motor Ticks",             &TicksPerRev,           "number of motor ticks per revolution" },
  { VAR_TYPE_ENUM,   "Motor GearRatio",         &GearRatio,             "gear ratio in handle" },
  { VAR_TYPE_BOOL,   "Power Save Enable",       &PowerSave,             "On/Off" },
  { VAR_TYPE_BOOL,   "Piezo Enable",            &PiezoEnable,           "On/Off" },
  { VAR_TYPE_BOOL,   "Fatal Error Enable",      &FatalErrorEnable,      "Allow the FatalError handler to annunciate" },
  { VAR_TYPE_BOOL,   "Is motorless C3 Board",   &IsMotorlesssC3BoardKVF,"Is this a C3 board with no motors?" },
  { VAR_TYPE_BOOL,   "Disable fire count",      &DisableFireCountsKVF,  "Increment fire counts?" },
  { VAR_TYPE_BOOL,   "Disable sleep modes",     &DisableSleepModesKVF,  "Disables handle going to sleep" },
  { VAR_TYPE_BOOL,   "Is C3 Board",             &IsC3BoardKVF,          "Is this a C3 board?" },
  { VAR_TYPE_UNKNOWN, 0, 0, 0 }  // need this as a terminator
};

static const KVF_PARAM HandleKvfParam = {
// *pMap,        *pDescription
   HandleKvfMap, "handle Settings"
};


/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief    handle kvf Init
 *
 * \details  Validate handle kvf parameters. Create a kvf if it doesnt exist.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void HandleKvfInit( void )
{
  KVF_ERROR KvfErr;

  KvfValidate(&HandleKvfParam, HANDLE_KVF_FILE, &KvfErr);
  
  if (KvfErr != KVF_ERR_NONE)
  {
    /// \todo 10/13/2022 JA: Log appropriate error message here
  }
}
#endif
/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

