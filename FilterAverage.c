#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup ALGORITHM
 * \{
 *
 * \brief   This module provides filters a set of input data samples and outputs
 *          the average value.
 *
 * \details This module is responsible for initializing and running the filter.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "FilterAverage.h"

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

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Initialize the filter structure.
 *
 * \details Clears average buffer and resets all persistent variables in the
 *          specified structure.
 *
 * \param   pFilter - Pointer to the filter structure
 * \param   pData - pointer to filter data
 * \param   Length  - Filter length
 *
 * \return  None
 *
 * ========================================================================== */
void FilterAverageInit(FILTER_AVERAGE *pFilter, int16_t  *pData, uint8_t Length)
{
    do
    {
        if (NULL == pFilter)
        {
            break;
        }

        /* Initialize the filter structure */
        pFilter->Output = 0;
        pFilter->Index = 0;
        pFilter->SumData = 0;
        pFilter->Length = Length;


        /* Initialize the filter data */
        pFilter->pData = pData;
        memset(pFilter->pData, 0x0, pFilter->Length * 2);   /// \todo 10/07/2021 DAZ - Not correct. Should be length * sizeof(int16_t)

    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Removes the minimum and maximum value
 *
 * \details Removes the sample minimum and maximum values from the FilterAverage()
 *          average calculation, outputs the average value.
 *
 * \param   pFilter - Pointer to the filter structure
 *
 * \return  None
 *
 * ========================================================================== */
void FilterAverageExcludeMinMax(FILTER_AVERAGE *pFilter)
{
    uint8_t IndexMin;       /* Index of min value */
    uint8_t IndexMax;       /* Index of max value */
    uint8_t Index;          /* Filter data index */

    IndexMin = 0;
    IndexMax = 0;

    do
    {
        if ((NULL == pFilter) || (NULL == pFilter->pData))
        {
            break;
        }

        for (Index = 1; Index < pFilter->Length; Index++)
        {
            IndexMin = (pFilter->pData[IndexMin] > pFilter->pData[Index]) ? Index : IndexMin;
            IndexMax = (pFilter->pData[IndexMax] < pFilter->pData[Index]) ? Index : IndexMax;
        }

        pFilter->Output = (pFilter->SumData - (pFilter->pData[IndexMin] + pFilter->pData[IndexMax])) / (pFilter->Length - 2);

    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Filters a set of input data samples
 *
 * \details Filters a set of input samples, outputs the average value.
 *
 * \param   pFilter - Pointer to the filter structure
 * \param   Sample  - Filter input
 *
 * \return  None
 *
 * ========================================================================== */
void FilterAverage(FILTER_AVERAGE *pFilter, int16_t Sample)
{
    do
    {
        if ((NULL == pFilter) || (NULL == pFilter->pData))
        {
            break;
        }

        pFilter->Index = (pFilter->Index >= (pFilter->Length)) ? 0 : pFilter->Index;

        pFilter->SumData -= pFilter->pData[pFilter->Index];
        pFilter->SumData += Sample;

        /* Overwrite oldest sample and point to new oldest sample */
        pFilter->pData[pFilter->Index++] = Sample;

        pFilter->Output = pFilter->SumData/pFilter->Length;

    } while (false);
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
