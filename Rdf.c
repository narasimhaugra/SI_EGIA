#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Rdf
 * \{
 *
 * \brief   Raw Data File Utilities (RDF)
 *
 * \details This module provides functions for the handling of Raw Data Files (rdf)
 *          which are used to collect motor data during a move.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Rdf.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include <Common.h>
#include <string.h>
#include "Logger.h"
#include "Rdf.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_LOGGER)

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
/// Types & names for RDF parameters. The titles are numbered to insure proper
/// ordering by MCP when creating excel spreadsheets
static const RDF_ITEM_DEFINITION RdfItemDefinitions[] =
{
    { VAR_TYPE_INT32U,  "00 Time"        }, // MOT_STREAM_TIME
    { VAR_TYPE_INT32U,  "01 Speed Setpt" }, // MOT_STREAM_SPEED_SETPOINT
    { VAR_TYPE_INT32U,  "02 Avg Speed"   }, // MOT_STREAM_SPEED
    { VAR_TYPE_INT32U,  "03 Inst Speed"  }, // MOT_STREAM_INST_SPEED
    { VAR_TYPE_INT32S,  "04 Motor Pos"   }, // MOT_STREAM_POSITION
    { VAR_TYPE_INT16U,  "05 Filter Curr" }, // MOT_STREAM_FILTER_CURRENT
    { VAR_TYPE_INT16U,  "06 Avg Curr"    }, // MOT_STREAM_AVG_CURRENT
    { VAR_TYPE_FP32,    "07 PID Err"     }, // MOT_STREAM_PID_ERROR
    { VAR_TYPE_FP32,    "08 PID ErrSum"  }, // MOT_STREAM_PID_ERRORSUM
    { VAR_TYPE_FP32,    "09 PID Output"  }, // MOT_STREAM_PID_OUTPUT
    { VAR_TYPE_INT16U,  "10 PWM Output"  }, // MOT_STREAM_PWM_OUTPUT
    { VAR_TYPE_INT16U,  "11 Raw Strain"  }, // MOT_STREAM_RAW_SG
    { VAR_TYPE_INT16U,  "12 Scl Strain"  }, // MOT_STREAM_SCALED_SG
    { VAR_TYPE_INT16U,  "13 Inst Curr"   }  // MOT_STREAM_INST_CURRENT /// \todo 05/11/2022 DAZ - In case we wish to add
};

// A compile-time check to ensure that RdfItemDefinitions is the same size as MOT_STREAM_MAX_VARS.
static_assert((sizeof(RdfItemDefinitions) / sizeof(RdfItemDefinitions[0])) == MOT_STREAM_MAX_VARS,
              "RdfItemDefinitions is wrong size.  Should have MOT_STREAM_MAX_VARS entries.");

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
RDF_OBJECT RdfObject[RDF_OBJECT_MAX];           // Storage for RDF objects
static uint32_t  TotalDroppedRDFPackets;        // Dropped RDF packet count

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
 * \brief   Create a Raw Data File Object.
 *
 * \details Fills in the RDF block corresponding to the specified motor & fills
 *          in filename, motor number, sample rate, and bit map of variables to
 *          be streamed, and returns a pointer to the RDF block.
 *
 * \param   pName       - Pointer to Name of RDF file
 * \param   MotorNum    - Number of motor
 * \param   SampleRate  - Sample rate of data in milliseconds
 * \param   StreamVars  - Bit map of variables to be streamed
 *
 * \return  Pointer to RDF object selected. NULL if error.
 *
 * ========================================================================== */
RDF_OBJECT* RdfCreate(char *pName, uint8_t MotorNum, uint32_t SampleRate, uint16_t StreamVars)
{
    RDF_OBJECT *pRdf;

    do
    {
        pRdf = NULL;                    // Return null if no filename given

        BREAK_IF(NULL == pName);

        pRdf = &RdfObject[MotorNum];    // Point to RDF object
        pRdf->pName = pName;
        pRdf->MotorNum = MotorNum;
        pRdf->NumVars = 0;
        pRdf->StreamVars = StreamVars;
        pRdf->SampleRate = SampleRate;
        pRdf->pFile = NULL;

        // Count the number of 1 bits (NumVars) in the StreamVars bitmap:
        while (StreamVars != 0)
        {
            pRdf->NumVars++;
            StreamVars = StreamVars >> 1;
        }

    } while (false);

    return pRdf;
}

/* ========================================================================== */
/**
 * \brief   Open an RDF file
 *
 * \details Creates an RDF_OPEN_SIG signal and posts it to the logger. This
 *          tells the logger to open the file. (The logger task owns the file)
 *
 * \param   pRdf - Pointer to the RDF object.
 *
 * \return  None
 *
 * ========================================================================== */
void RdfOpen(RDF_OBJECT *pRdf)
{
#if !REFORMAT_BAD_SD_CARDS
    QEVENT_RDF_OPEN_CLOSE *pEvent;

    if (NULL != pRdf)
    {
        pEvent = AO_EvtNew(RDF_OPEN_SIG, sizeof(QEVENT_RDF_OPEN_CLOSE));
        if (pEvent)
        {
            pEvent->pRdf = pRdf;

/// \todo 11/15/2021 DAZ - This is a violation of QPC abstraction. We don't have an abstraction for the following
///                        condition yet:
///                        QACTIVE_POST_X is called here directly, because we are using a non-zero margin value
///                        which AO_Post does not support. We have the margin so that POST returns an error
///                        instead of throwing an assert. Change the way AO_Post works? Have asserts that don't
///                        hang? (Current Q_ASSERT does.)

            if (!QACTIVE_POST_X(AO_Logger, &pEvent->Event, 1, NULL))    // If Logger event queue is full, bump dropped RDF packets count
            {
                TotalDroppedRDFPackets += 1;
            }
        }
    }
#endif
}

/* ========================================================================== */
/**
 * \brief   Close an RDF file
 *
 * \details Creates an RDF_CLOSE_SIG signal and posts ito to the logger. This
 *          tells the logger to close the file. (The logger task owns the file)
 *
 * \param   pRdf - Pointer to the RDF object.
 *
 * \return  None
 *
 * ========================================================================== */
void RdfClose(RDF_OBJECT *pRdf)
{
#if !REFORMAT_BAD_SD_CARDS
    QEVENT_RDF_OPEN_CLOSE *pEvent;

    if (pRdf != NULL)
    {
        pEvent = AO_EvtNew(RDF_CLOSE_SIG, sizeof(QEVENT_RDF_OPEN_CLOSE));
        if (pEvent)
        {
            pEvent->pRdf = pRdf;
            /// \todo 11/15/2021 DAZ - See RdfOpen
            if (!QACTIVE_POST_X(AO_Logger, &pEvent->Event, 1, NULL))    // If Logger event queue is full, bump dropped RDF packets count
            {
                TotalDroppedRDFPackets += 1;
            }
        }
    }
#endif
}

/* ========================================================================== */
/**
 * \brief   Write a variable value to the specified RDF object.
 *
 * \details This updates the RDF variable value prior to the RDF object being
 *          written to disk.
 *
 * \param   pRdf   - Pointer to the RDF object to update variable in
 * \param   VarIdx - Index of variable to update
 * \param   pVoid  - Pointer to value.
 *
 * \return  None
 *
 * ========================================================================== */
void RdfVariableWrite(RDF_OBJECT *pRdf, MOT_STREAM_PARAMS VarIdx, void *pVoid)
{
    RDF_VAR *pVar;

    if (VarIdx < MOT_STREAM_MAX_VARS)
    {
        pVar = &pRdf->Var[VarIdx];

        switch (RdfItemDefinitions[VarIdx].VarType)     // Get variable type from item definitions
        {
            case VAR_TYPE_INT8U:
            case VAR_TYPE_INT8S:
                pVar->Int8uVal = *(uint8_t *)pVoid;
                break;
            case VAR_TYPE_INT16U:
            case VAR_TYPE_INT16S:
                pVar->Int16uVal = *(uint16_t *)pVoid;
                break;
            case VAR_TYPE_INT32U:
            case VAR_TYPE_INT32S:
                pVar->Int32uVal = *(uint32_t *)pVoid;
                break;
            case VAR_TYPE_FP32:
                pVar->Fp32Val = *(float32_t *)pVoid;
                break;
            default:
                break;
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Request an RDF object be written to the file.
 *
 * \details Create an RDF event, add the appropriate data and post the event
 *          to the Logger module.
 *
 * \param   pRdf - Pointer to the RDF object
 *
 * \return  None
 *
 * ========================================================================== */
void RdfWriteData(RDF_OBJECT *pRdf)
{
    QEVENT_RDF_DATA *pEvent;        // Pointer to data event
    RDF_VAR         *pVar;          // Pointer to RDF variable
    uint16_t        StreamVars;     // List of variables to stream
    uint8_t         Idx;            // Index for processing variable list
    uint8_t         *pData;         // Pointer to data to retrieve from RDF variable
    bool            Status;         // Operation status - True if error

    do
    {
        Status = true;                                          // Default to error
        BREAK_IF(!FsIsInitialized());                           // Quit if file system not initialized
        BREAK_IF(NULL == pRdf);                                 // Quit if null RDF pointer

        pEvent = AO_EvtNew(RDF_DATA_SIG, sizeof(QEVENT_RDF_DATA));  // Create new event
        BREAK_IF(NULL == pEvent);                                   // Quit if new event creation failed

        StreamVars = pRdf->StreamVars;  // Get list of variables to stream
        pEvent->pRdf = pRdf;            // Set pointer to RDF object
        pEvent->Count = 0;              // Initialize data byte count

        // Go through list & copy selected parameters

        pData = pEvent->Data;                                   // Set destination pointer to data area of event

        for (Idx = 0; Idx < pRdf->NumVars; Idx++)
        {
            if (StreamVars & 1)                                 // Check bit to see if corresponding parameter is selected
            {
                pVar = &pRdf->Var[Idx];                         // Point to selected variable in object

                switch (RdfItemDefinitions[Idx].VarType)
                {
                    case VAR_TYPE_INT8U:
                    case VAR_TYPE_INT8S:
                        Mem_Copy(pData, (const void *)pVar->Int8uVal, 1);   // Copy data BYTE
                        pData += 1;                                         // Increment destination pointer
                        pEvent->Count += 1;                                 // Increment data byte count
                        break;

                    case VAR_TYPE_INT16U:
                    case VAR_TYPE_INT16S:
                        Mem_Copy(pData, (const void *)&pVar->Int8uVal, 2);  // Copy data WORD (2 bytes, 16 bit)
                        pData += 2;
                        pEvent->Count += 2;
                        break;

                    case VAR_TYPE_INT32U:
                    case VAR_TYPE_INT32S:
                    case VAR_TYPE_FP32:
                        Mem_Copy(pData, (const void *)&pVar->Int8uVal, 4);  // Copy data WORD (4 bytes, 32 bit)
                        pData += 4;
                        pEvent->Count += 4;
                        break;

                    default:
                        break;
                }
            }

            StreamVars = StreamVars >> 1;   // Shift bitmap to next variable
        }

        /// \todo 11/15/2021 DAZ - See RdfOpen concerning unabstracted post call.
        BREAK_IF(!QACTIVE_POST_X(AO_Logger, &pEvent->Event, 1, NULL));      // If Logger event queue is full, exit bad
        Status = false;                                                     // No errors

    } while (false);

    if (Status)
    {
        TotalDroppedRDFPackets += 1;        // Error occurred - Increment dropped packet count. /// \todo 12/10/2021 DAZ - Keep this per RDF object?
    }
}

/* ========================================================================== */
/**
 * \brief   Process the RDF_OPEN signal.
 *
 * \details Opens the specified file & writes the RDF parameters to it.
 *
 * \param   pEvent - Pointer to event signal received (RDF_OPEN_SIG)
 *
 * \return  None
 *
 * ========================================================================== */
void RdfProcessOpenSignal(QEvt const *const pEvent)
{
    RDF_OBJECT  *pRdf;
    uint16_t    StreamVars;         // Bitmap of variables to stream
    uint8_t     Idx;
    uint8_t     Len;
    uint32_t    BytesWritten;
    FS_ERR      FsErr;

    do
    {
        BREAK_IF(NULL == pEvent);                   // Quit if null parameter
        pRdf = ((QEVENT_RDF_OPEN_CLOSE *)pEvent)->pRdf;
        BREAK_IF(NULL == pRdf);                     // Quit if null RDF object
        BREAK_IF(!FsIsInitialized());               // Quit if error during file system initialization

        if (pRdf->pFile != NULL)
        {
            Log(FLT, "Error opening file '%s' in RdfProcessOpenSignal: Previous file not closed", pRdf->pName);
            return;
        }

        FsErr = FsOpen(&pRdf->pFile, (int8_t *)pRdf->pName, FS_FILE_ACCESS_MODE_WR | FS_FILE_ACCESS_MODE_CREATE | FS_FILE_ACCESS_MODE_TRUNCATE);
        if (FsErr != FS_ERR_NONE)
        {
            Log(ERR, "Error opening file '%s' in RdfProcessOpenSignal: %d", pRdf->pName, FsErr);
            pRdf->pFile = NULL;
            break;
        }

        /// \todo 11/15/2021 DAZ - There is no abstraction for this function yet.
        /// \todo 12/10/2021 DAZ - Why do we need a bufassign here, but not for event logging?
        FSFile_BufAssign(pRdf->pFile, pRdf->FileBuf, FS_FILE_BUF_MODE_WR, 512, &FsErr);

        // write out file header
        FsFileWrWord(pRdf->pFile, FILE_TYPE_ID_RDF);
        FsFileWrByte(pRdf->pFile, RDF_MAJOR_REV);
        FsFileWrByte(pRdf->pFile, RDF_MINOR_REV);

        // rdf file name
        Len = (uint8_t)Str_Len(pRdf->pName);
        FsFileWrByte(pRdf->pFile, Len);
        FsWrite(pRdf->pFile, (uint8_t *)pRdf->pName, sizeof(char) * Len, &BytesWritten);

        // motor number
        FsFileWrByte(pRdf->pFile, pRdf->MotorNum);

        // sample rate
        FsFileWrLong(pRdf->pFile, pRdf->SampleRate);

        // num vars
        FsFileWrByte(pRdf->pFile, pRdf->NumVars);

        // Write out variable headers from RdfItemDefinitions

        StreamVars = pRdf->StreamVars;      // Get list of variables to stream

        for (Idx = 0; Idx < pRdf->NumVars; Idx++)
        {
            if (StreamVars & 1)
            {
                // var name
                Len = (uint8_t)Str_Len(RdfItemDefinitions[Idx].ItemTypeName) * sizeof(char);
                FsFileWrByte(pRdf->pFile, Len);
                FsWrite(pRdf->pFile, (uint8_t *)RdfItemDefinitions[Idx].ItemTypeName, Len, &BytesWritten);

                // var type
                FsFileWrByte(pRdf->pFile, (uint8_t)RdfItemDefinitions[Idx].VarType);

                // set compression to 0 (reserved for future use)
                FsFileWrByte(pRdf->pFile, 0);
            }

            StreamVars = StreamVars >> 1;   // Shift bit map to next variable
        }
    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Write the RDF event data to the SD file.
 *
 * \details Writes the data attached to the specified event to the file spedified
 *          in the event.
 *
 * \param   pEvent - Pointer to event to process
 *
 * \return  None
 *
 * ========================================================================== */
void RdfProcessDataSignal(QEvt const *const pEvent)
{
    RDF_OBJECT *pRdf;
    uint8_t Count;
    uint32_t BytesWritten;

    if (FsIsInitialized())  // File system OK - continue
    {
        pRdf = ((QEVENT_RDF_DATA *)pEvent)->pRdf;       // Get pointer to RDF object

        if (pRdf->pFile != NULL)                        /// \todo 05/18/2022 DAZ - Need to think about this. pRdf can NEVER be null.
        {
            Count = ((QEVENT_RDF_DATA *)pEvent)->Count;
            FsWrite(pRdf->pFile, ((QEVENT_RDF_DATA *)pEvent)->Data, Count, &BytesWritten);
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Close the SD RDF file.
 *
 * \details The file to be closed is obtained from the event passed.
 *
 * \param   pEvent - Pointer to RDF close event.
 *
 * \return  None
 *
 * ========================================================================== */
void RdfProcessCloseSignal(QEvt const *const pEvent)
{
    RDF_OBJECT *pRdf;

    pRdf = ((QEVENT_RDF_OPEN_CLOSE *)pEvent)->pRdf;

    if (pRdf)
    {
        if ((pRdf->pFile != NULL) && FsIsInitialized())
        {
            FsClose(pRdf->pFile);
            pRdf->pFile = NULL;
        }
    }
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
