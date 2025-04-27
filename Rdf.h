#ifndef RDF_H
#define RDF_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup RDF
 * \{
 *
 * \brief   Raw Data File utilities (RDF)
 *
 * \details This header file defines the public interfaces for RDF file handling
 *          as well as symbols to define parameters to be logged.
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Rdf.h
 *
 * ========================================================================== */
#include "ActiveObject.h"
#include "L4_ConsoleCommands.h"   /// \todo 11/15/2021 DAZ - For VAR_TYPE. Put in common?
#include "FileTypes.h"
#include "FileSys.h"
#include "FileUtil.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define RDF_MAJOR_REV      (1u)
#define RDF_MINOR_REV      (2u)
#define RDF_OBJECT_MAX     (3u)
#define RDF_FILEBUF_SIZE   (512u)

/// \todo 03/24/2022 DAZ - Implement the following as initialized enums?

/// RDF Streaming flags:
#define MOT_STREAM_TIME_BIT                 0x0001    ///< Time of log (absolute)
#define MOT_STREAM_SPEED_SETPOINT_BIT       0x0002    ///< Speed setpoint (winding RPM)
#define MOT_STREAM_AVG_SPEED_BIT            0x0004    ///< Present average speed (winding RPM)
#define MOT_STREAM_INST_SPEED_BIT           0x0008    ///< Instantaneous speed (from FPGA)
#define MOT_STREAM_POSITION_BIT             0x0010    ///< Motor position
#define MOT_STREAM_FILTER_CURRENT_BIT       0x0020    ///< Filtered (H/W) ADC input at CPU
#define MOT_STREAM_AVG_CURRENT_BIT          0x0040    ///< Averaged filtered value
#define MOT_STREAM_PID_ERROR_BIT            0x0080    ///< PID velocity error
#define MOT_STREAM_PID_ERRORSUM_BIT         0x0100    ///< PID velocity error sum
#define MOT_STREAM_PID_OUTPUT_BIT           0x0200    ///< PID output
#define MOT_STREAM_PWM_OUTPUT_BIT           0x0400    ///< PWM output (511 full scale)
#define MOT_STREAM_RAW_SG_BIT               0x0800    ///< Unscaled raw SG ADC
#define MOT_STREAM_SCALED_SG_BIT            0x1000    ///< Scaled, Tared SG in lbs
#define MOT_STREAM_INST_CURRENT_BIT         0x2000    ///< Unfiltered - we don't use this. Do we need it?

#define MOT_RDF_NAMESIZE_ITEMDEF        (15u)       ///< Size of RDF Name, Item Definition
#define MOT_RDF_NAMESIZE_GEN            (20u)       ///< Size of RDF Name, General
#define MOT_NUM_RDF_STOPINFO_MSGS       (13u)       ///< Number of RDF Stop Msgs
#define MOT_RDF_NUM_ENTRIES             (1000u)     ///< Number of RDF Entries

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/
/// Streaming parameters. This enum MUST be in sync with RdfItemDefinitions
typedef enum
{
    MOT_STREAM_TIME,                ///< Time of record
    MOT_STREAM_SPEED_SETPOINT,      ///< Speed setpoint
    MOT_STREAM_AVG_SPEED,           ///< Average speed
    MOT_STREAM_INST_SPEED,          ///< Instantaneous speed (From FPGA period register)
    MOT_STREAM_POSITION,            ///< Motor position
    MOT_STREAM_FILTER_CURRENT,      ///< H/W filtered current input to ADC.
    MOT_STREAM_AVG_CURRENT,         ///< Average current
    MOT_STREAM_PID_ERROR,           ///< PID speed error
    MOT_STREAM_PID_ERRORSUM,        ///< PID speed error sum
    MOT_STREAM_PID_OUTPUT,          ///< PID output (from 0 to 1)
    MOT_STREAM_PWM_OUTPUT,          ///< PWM output (from 0 to 511)
    MOT_STREAM_RAW_SG,              ///< Raw (unscaled) strain gauge ADC
    MOT_STREAM_SCALED_SG,           ///< Scaled & tared strain gauge
    MOT_STREAM_INST_CURRENT,        ///< Instantaneous (unscaled) current (unimplemented)
    MOT_STREAM_MAX_VARS
} MOT_STREAM_PARAMS;

/// RDF data item definition
typedef struct
{
    VAR_TYPE VarType;                                   ///< Variable type, ie: VAR_TYPE_INT8U, VAR_TYPE_INT32U...
    char     ItemTypeName[MOT_RDF_NAMESIZE_ITEMDEF];    ///< Variable name (Including terminator)
} RDF_ITEM_DEFINITION;

/// RDF variable structure
typedef struct
{
  /// Union for variable value
    union
    {
        bool        BoolVal;      ///< Boolean variable
        uint8_t     Int8uVal;     ///< 8 bit unsigned
        int8_t      Int8sVal;     ///< 8 bit signed
        uint16_t    Int16uVal;    ///< 16 bit unsigned
        int16_t     Int16sVal;    ///< 16 bit signed
        uint32_t    Int32uVal;    ///< 32 bit unsigned
        int32_t     Int32sVal;    ///< 32 bit signed
        float32_t   Fp32Val;      ///< 32 bit floating point
    };
} RDF_VAR;

typedef struct
{
    char      *pName;                     ///< RDF file name
    uint8_t   MotorNum;                   ///< Motor Id
    uint8_t   NumVars;                    ///< Number of variables in list
    uint16_t  StreamVars;                 ///< Variables to stream
    uint32_t  SampleRate;                 ///< Rate to log variables (milliseconds)
    FS_FILE   *pFile;                     ///< RDF file pointer
    RDF_VAR   Var[MOT_STREAM_MAX_VARS];   ///< RDF variables. See RdfItemDefinitions for variable type.
    uint8_t   FileBuf[RDF_FILEBUF_SIZE + MEMORY_FENCE_SIZE_BYTES];               ///< File buffer
} RDF_OBJECT;

/// RDF file open/close event structure
typedef struct
{
    QEvt        Event;          ///< Event structure
    RDF_OBJECT  *pRdf;          ///< File pointer to open/close
} QEVENT_RDF_OPEN_CLOSE;

/// RDF file log data signal structure
typedef struct
{
    QEvt        Event;          ///< Event structure
    RDF_OBJECT  *pRdf;          ///< Pointer to RDF object to log to
    uint8_t     Count;          ///< Number of bytes to log
    uint8_t     Data[64];       ///< Data to log. Ensure array is big enough for all defined variables.
} QEVENT_RDF_DATA;

/******************************************************************************/
/*                             Global Constant Declaration(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Declaration(s)                 */
/******************************************************************************/
extern uint32_t  TotalDroppedRDFPackets;

/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
RDF_OBJECT *RdfCreate( char *pName, uint8_t MotorNum, uint32_t SampleRate, uint16_t StreamVars );
void RdfOpen( RDF_OBJECT *pRdf );
void RdfClose( RDF_OBJECT *pRdf );
void RdfVariableWrite( RDF_OBJECT *pVar, MOT_STREAM_PARAMS VarIdx, void *pVoid );
void RdfWriteData( RDF_OBJECT *pRdf );

void RdfProcessOpenSignal( QEvt const * const pEvent );
void RdfProcessDataSignal( QEvt const * const pEvent );
void RdfProcessCloseSignal( QEvt const * const pEvent );

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif    /* RDF_H */
