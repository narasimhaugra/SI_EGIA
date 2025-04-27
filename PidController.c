#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup ALGORITHM
 * \{
 *
 * \brief   PID utilities.
 *
 * \details This module implements an abstract 'PID' object, which
 *          implements a Proportional, Integral, Derivative Controller.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "PidController.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void PidInterpolateValue(uint32_t DataInput, float32_t *pValue, uint32_t *pDataList, float32_t *pValArray);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   PID Interpolate value calculation.
 *
 * \details Using the supplied input data and data array, an interpolation ratio
 *          is calculated which is used to return an interpolated value
 *          from the corresponding entries in the value array.
 *
 * \param   DataInput  - Input data
 * \param   pValue     - Pointer to Interpolated value to return
 * \param   pDataList  - Pointer to DataList array
 * \param   pValArray  - Pointer to PID Value array
 *
 * \return  None
 *
 * ========================================================================== */
static void PidInterpolateValue(uint32_t DataInput, float32_t *pValue, uint32_t *pDataList, float32_t *pValArray)
{
    uint8_t        Index;             /* Array index */
    float32_t      MinPidValue;       /* Minimum array value */
    float32_t      MaxPidValue;       /* Maximum array value */
    float32_t      MinData;           /* Minimum data value in the list */
    float32_t      MaxData;           /* Maximum data value in the list */
    float32_t      Ratio;             /* Interpolation ratio */
    float32_t      Delta;             /* Difference of Max and Min value */

    do
    {
        /* Check the input parameters validity */
        if ((pValue == NULL) || (pDataList == NULL) || (pValArray == NULL))
        {
            break;
        }

        /* Check if DataInput Less than Min Table Value */
        if (DataInput <= pDataList[0])
        {
            /* Return Min Table Value */
            *pValue = pValArray[0];
            break;
        }

        /* Check if DataInput Greater than Max Table Value */
        if (DataInput >= pDataList[PID_MAX_INTERP_ENTRIES - 1])
        {
            /* Return Max Table Value */
            *pValue = pValArray[PID_MAX_INTERP_ENTRIES - 1];
            break;
        }

        for (Index = 1; Index < PID_MAX_INTERP_ENTRIES; Index++)
        {
            /* If RPM between these 2 segments */
            if (DataInput < pDataList[Index])
            {
                MinPidValue = pValArray[Index - 1];
                MaxPidValue = pValArray[Index];
                MinData = pDataList[Index - 1];
                MaxData = pDataList[Index];

                /* Determine ratio */
                Ratio = ((float32_t)DataInput - MinData) / (MaxData - MinData);

                Delta = MaxPidValue - MinPidValue;
                *pValue = MinPidValue + (Ratio * Delta);
                break;
            }
        }

    } while (false);
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Initializes the PID structure.
 *
 * \details This function initializes the PID structure with constants to be  used
 *          for the PID coefficients. This will also clear any persistent data variables.
 *
 * \param   pPid    - Pointer to PID structure to initialize
 * \param   Kp      - Proportional coefficient
 * \param   Ki      - Integral coefficient
 * \param   Kd      - Derivative coefficient
 *
 * \return  None
 *
 * ========================================================================== */
void PidInit(PID *pPid, float32_t Kp, float32_t Ki, float32_t Kd)
{
    if (NULL != pPid)
    {
        pPid->Output = 0.0f;
        pPid->OutputMax = PID_OUTPUT_MAX;
        pPid->OutputMin = PID_OUTPUT_MIN;
        pPid->Error = 0.0f;
        pPid->ErrorSum = 0.0f;
        pPid->ErrorDiff = 0.0f;
        pPid->Kp = Kp;
        pPid->Ki = Ki;
        pPid->Kd = Kd;

        /* Integral must be able to force 100% PWM */
        pPid->IntegratorHighClamp = PID_INTEGRATOR_HIGH / Ki;
        pPid->IntegratorLowClamp = PID_INTEGRATOR_LOW / Ki;
    }
}

/* ========================================================================== */
/**
 * \brief   Reset the PID structure.
 *
 * \details This function will clear any persistent data variables.
 *
 * \param   pPID                 - Pointer to PID structure to reset
 * \param   OutputMax            - Filter output high clamp
 * \param   OutputMin            - Filter output low clamp
 * \param   IntegratorHighClamp  - High clamp
 * \param   IntegratorLowClamp   - Low clamp
 *
 * \return  None
 *
 * ========================================================================== */
void PidReset(PID *pPID, float32_t OutputMax, float32_t OutputMin, float32_t IntegratorHighClamp, float32_t IntegratorLowClamp)
{
    if (NULL != pPID)
    {
        pPID->Output = 0.0f;
        pPID->OutputMax = OutputMax;
        pPID->OutputMin = OutputMin;
        pPID->Error = 0.0f;
        pPID->ErrorSum = 0.0f;
        pPID->ErrorDiff = 0.0f;
        pPID->Kp = 0.0f;
        pPID->Ki = 0.0f;
        pPID->Kd = 0.0f;
        pPID->IntegratorHighClamp = IntegratorHighClamp;
        pPID->IntegratorLowClamp = IntegratorLowClamp;
    }
}

/* ========================================================================== */
/**
 * \brief   Perform PID Calculation.
 *
 * \details Performs PID calculation and updates all local error values.
 *
 * \param   pPID    - Pointer to PID structure to run calculation on
 * \param   Error   - Error input to PID calculation
 *
 * \return  None
 *
 * ========================================================================== */
void PidController(PID *pPID, float32_t Error)
{
    /* Check the input validity and proceed */
    if (NULL != pPID)
    {
        pPID->ErrorSum += Error;
        pPID->ErrorDiff = pPID->Error - Error;
        pPID->Error = Error;

        /* Clip integral to limits (anti-windup) */
        pPID->ErrorSum = (pPID->ErrorSum > pPID->IntegratorHighClamp) ? pPID->IntegratorHighClamp : pPID->ErrorSum;
        pPID->ErrorSum = (pPID->ErrorSum < pPID->IntegratorLowClamp) ? pPID->IntegratorLowClamp : pPID->ErrorSum;

        /* Output value calculation */
        pPID->Output = (pPID->Kp * pPID->Error) + (pPID->Ki * pPID->ErrorSum) + (pPID->Kd * pPID->ErrorDiff);

        /* Clip output to limits */
        pPID->Output = (pPID->Output > pPID->OutputMax) ? pPID->OutputMax : pPID->Output;
        pPID->Output = (pPID->Output < pPID->OutputMin) ? pPID->OutputMin : pPID->Output;
    }
}

/* ========================================================================== */
/**
 * \brief   Interpolate PID gains.
 *
 * \details This function returns the PID values appropriate for
 *          the specified input value & PID table data.
 *
 * \param   TableData  - Pointer to interpolation table
 * \param   Kp      - Pointer to Proportional coefficient
 * \param   Ki      - Pointer to Integral coefficient
 * \param   Kd      - Pointer to Derivative coefficient
 *
 * \return  None
 *
 * ========================================================================== */
void PidInterpolation(PID_TABLE_DATA *TableData, float32_t *Kp, float32_t *Ki, float32_t *Kd)
{
    PID_INTERP_TABLE   *Pidtable;     /* Pointer to the PID interpolate table */

    /* Check the input validity and proceed */
    if (NULL != TableData)
    {
        Pidtable = TableData->PidInterpTables.PidDataTable[TableData->TableId];

        PidInterpolateValue(TableData->DataInput, Kp, Pidtable->DataInput, Pidtable->Proportional);
        PidInterpolateValue(TableData->DataInput, Ki, Pidtable->DataInput, Pidtable->Integral);
        PidInterpolateValue(TableData->DataInput, Kd, Pidtable->DataInput, Pidtable->Differential);
    }
}

/* ========================================================================== */
/**
 * \brief   Set number of filter taps and speed correction threshold for requested
 *          speed.
 *
 * \details Based on target speed, select the proper velocity filter size, and
 *          the minimum speed error threshold for correction.
 *
 * \param   pTableData - Pointer to PID_TABLE_DATA structure which has motor voltage ID,
 *                       target trilobe speed, & pointer to interpolation table.
 * \param   pTaps      - Pointer to filter taps storage
 * \param   pRpmThresh - Pointer to RPM threshold storage
 *
 * \return  None
 *
 * ========================================================================== */
void PidSetTapsRpmThreshold(PID_TABLE_DATA *pTableData, uint8_t *pTaps, uint32_t *pRpmThresh)
{
    uint32_t         TargetSpeed;   // Target trilobe speed
    uint32_t         Index;         // Index for stepping through interpolation table
    PID_INTERP_TABLE *pPidtable;    // Pointer to interpolation table

    do
    {
        BREAK_IF(NULL == pTableData);
        BREAK_IF(NULL == pTaps);
        BREAK_IF(NULL == pRpmThresh);

        pPidtable = pTableData->PidInterpTables.PidDataTable[pTableData->TableId];  // Point to proper interpolation table for specified voltage
        TargetSpeed = pTableData->DataInput;                                        // Retrieve the desired motor speed (trilobe)

        if (TargetSpeed <= pPidtable->DataInput[0])         // Check if RPM Less than Min Table Value
        {
            *pTaps = pPidtable->Taps[0];                    // Yes, Return Min Table Values
            *pRpmThresh = pPidtable->RpmThreshold[0];
            break;
        }

        if (TargetSpeed >= pPidtable->DataInput[PID_MAX_INTERP_ENTRIES - 1])    // Check if RPM Greater than Max Table Value
        {
            *pTaps = pPidtable->Taps[PID_MAX_INTERP_ENTRIES - 1];               // Yes, Return Max Table Values
            *pRpmThresh = pPidtable->RpmThreshold[PID_MAX_INTERP_ENTRIES - 1];
            break;
        }

        for (Index = 1; Index < PID_MAX_INTERP_ENTRIES; Index++)
        {
            if (TargetSpeed < pPidtable->DataInput[Index]) // If RPM between these 2 segments
            {
                *pTaps = pPidtable->Taps[Index];
                *pRpmThresh = pPidtable->RpmThreshold[Index];
                break;
            }
        }
    } while (false);
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
