#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup  UTILS
 * \{
 *
 * \brief       CPU Cycle counter
 *
 * \details     This module implements CPU cycle measurement feature typically
 *              used for higher resolution time events
 *
 * \copyright  2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    CpuCycles.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Common.h"
#include "CpuCycles.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_GENERAL) /*! Log Group Identifier */
#define MAX_SAMPLE_COUNT        (100u)              /*! Max sample count */

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef struct
{
    uint32_t Time;  /* CPU counter value indicates number of micro seoconds*/
    uint32_t Value; /* Value tag to hold any user specific information */
} TIME_SAMPLE;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
#pragma location=".sram"
static TIME_SAMPLE CpuTimeData[MAX_SAMPLE_COUNT]; /* Hold time and sample value */
static uint32_t SampleCount;

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
 * \brief   Initialize the CPU cycle counter.
 *
 * \details This function initializes CPU cycle counter
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void CpuTimeLogInit(void)
{
    CPU_COUNTER_ENABLE();
    CPU_COUNTER_RESET();
    memset(CpuTimeData, 0, sizeof(CpuTimeData));
    SampleCount = 0;
}

/* ========================================================================== */
/**
 * \brief   Log time and restart time counter
 *
 * \details This function adds CPU cycle counter to log registry and restart the counter
 *
 * \note    Function call delay overhead is ignored
 *
 * \param   Value - 32 Bit value to be added along CPU Time counter
 *
 * \return  None
 *
 * ========================================================================== */
void CpuTimeLogAndRestart(uint32_t Value)
{
    if(SampleCount < MAX_SAMPLE_COUNT)
    {
        CpuTimeData[SampleCount].Time = CPU_COUNTER_READ();
        CpuTimeData[SampleCount++].Value = Value;
        CPU_COUNTER_RESET();
    }
}

/* ========================================================================== */
/**
 * \brief   Capture a CPU cycle counter snapshot
 *
 * \details This function adds CPU cycle to log registry
 *
 * \note    Function call delay overhead is ignored
 *
 * \param   Value - 32 Bit value to be added along CPU Time counter
 *
 * \return  None
 *
 * ========================================================================== */
void CpuTimeLog(uint32_t Value)
{
    if(SampleCount < MAX_SAMPLE_COUNT)
    {
        CpuTimeData[SampleCount].Time = CPU_COUNTER_READ();
        CpuTimeData[SampleCount++].Value = Value;
    }
}

/* ========================================================================== */
/**
 * \brief   Dump CPU time logs
 *
 * \details This function dumps captured CPU time values to debug terminal
 *
 * \param   DumpThreshold - Log dump threshold. If 0, dumps logs only if full capacity is reached.
 *                  Else dumps if specified threshold is reached.
 *
 * \return  None
 *
 * ========================================================================== */
void CpuTimeLogDump(uint32_t DumpThreshold)
{
    uint32_t Index;

    if (((0 == DumpThreshold ) && (SampleCount >= MAX_SAMPLE_COUNT)) || (SampleCount >= DumpThreshold ))
    {
        Log(DBG, "CPU cycle counter log.....");

        for (Index=0; Index < MAX_SAMPLE_COUNT; Index++ )
        {
            if ((CpuTimeData[Index].Time == 0) && (Index > 1))
            {
                /* End if no valid entries */
                break;
            }
            Log(DBG, "   %u,  %u", CpuTimeData[Index].Time, CpuTimeData[Index].Value);
        }

        Log(DBG, "End of CPU counter log");
        CpuTimeLogInit();
    }
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
