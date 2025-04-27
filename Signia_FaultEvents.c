#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_FaultEvents
 * \{
 *
 * \brief   Signia functions to publish Fault Events
 *
 * \details The FaultEvents  is responsible for handling all Event published between
             the Signia Handle and the Adapter.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_FaultEvents.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Signia_FaultEvents.h"
#include "ActiveObject.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
extern FAULTINFO_STARTUP FaultInfoFromStartup;

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER             (LOG_GROUP_AO)

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
static bool UpdateErrorSignalToPublish(SIGNAL *pErrorSig, ERROR_CAUSE Cause, uint8_t *pLoopIndex, bool *pPublish, bool *pLog );

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   This API provides whether any of the requested Cause, Error Signal is already published.
 *
 * \details This function provides whether any of the startup fault signal is already published or not. If requested Cause is already published, publish is not allowed and Log is allowed.
 *
 * \param   pErrorSig  - Pointer to array of Signals
 * \param   Cause      - Cause of Fault
 * \param   pLoopIndex - Index of Array of Signals
 * \param   pPublish   - Pointer to bool. true- Publish signal, false- do not publish signal
 * \param  pLog        - Pointer to bool. true- Log the Cause, false- do not Log the Cause
 *
 * \return  Status
 * \return  true  - valid
 * \return  false - Invalid inputs
 *
 * ========================================================================== */
static bool UpdateErrorSignalToPublish(SIGNAL *pErrorSig, ERROR_CAUSE Cause, uint8_t *pLoopIndex, bool *pPublish, bool *pLog )
{
    uint8_t Index;
    bool SigAlreadyPublished;
    bool Status;

    SigAlreadyPublished = false;
    Index = 0;

    do
    {
        /* Checks for NULL */
        if ( NULL == pLoopIndex || NULL == pErrorSig || NULL == pPublish || NULL == pLog )
        {
            Status = false;
            break;
        }
        /*Is cause within range */
        if ( LAST_ERROR_CAUSE <= Cause )
        {
            Status = false;
            break;
        }
        /* Check if Already signal available to publish */
        for( Index = 0; Index < *pLoopIndex ; Index++ )
        {
            /* Is signal already Published */
            if ( CauseToSig_Table[Cause].Sig == pErrorSig[Index] )
            {
               SigAlreadyPublished = true;
            }
        }
        if ( SigAlreadyPublished )
        {
           /*Signal already Published, Log Error message */
            *pPublish = false;
            *pLog     = true ;
        }
        else
        {
            pErrorSig[*pLoopIndex] = CauseToSig_Table[Cause].Sig;
            /* Increment by 1 */
            *pLoopIndex += 1;
            /* Publish signal and Log Error */
            *pPublish = true;
            *pLog     = true;
        }
        Status = true;

    }while( false );

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Publish Error Signals
 *
 * \details This function publishes the Handle Errors from APP STARTUP
 *
 * \param   < None >
 *
 * \return  < None >
 *
 * ========================================================================== */
void Signia_StartupErrorEventPublish(void)
{
    uint8_t LoopIndex;
    ERROR_CAUSE Cause;
    SIGNAL ErrorSig[LAST_ERROR_CAUSE];
    bool  IsError;
    bool PublishSig;
    bool LogCause;

    LoopIndex = 0;
    IsError   = false;

    /* Clear Error Sig array */
    memset(ErrorSig, LAST_SIG , sizeof(ErrorSig));

    for( Cause = REQRST_FPGA_SELFTEST; Cause < LAST_ERROR_CAUSE; Cause++)
    {
        PublishSig = false;
        LogCause   = false;
        IsError = ( ((uint64_t)1 << Cause) & (FaultInfoFromStartup.ErrorStatus) ) && true ;

        IsError = (IsError)? UpdateErrorSignalToPublish(ErrorSig, Cause, &LoopIndex, &PublishSig, &LogCause): (IsError);

        if ( PublishSig )
        {
          Signia_ErrorEventPublish(Cause, SET_ERROR);
        }

        if ( LogCause )
        {
          Log(REQ,"ERROR CAUSE: %s",CauseToSig_Table[Cause].ErrorCauseStrings);
        }
    }
    /* App is ready to handle Error Signals */
    FaultInfoFromStartup.FaultHandlerAppInit = true;
}

/* ========================================================================== */
/**
 * \brief   Publish Error Signals
 *
 * \param   Cause       - Cause of the error
 * \param   ErrorStatus - Set or Clear Error
 *
 * \return  bool - event status
 * \retval  True - Event Published
 * \retval  FALSE - Not Published
 * ========================================================================== */
bool Signia_ErrorEventPublish(ERROR_CAUSE Cause, bool ErrorStatus )
{
    bool Status;
    QEVENT_FAULT *pSignalEvent;
    SIGNAL CurrentSig;

    CurrentSig = CauseToSig_Table[Cause].Sig;
    pSignalEvent = AO_EvtNew(CurrentSig, sizeof(QEVENT_FAULT));
    if ( pSignalEvent )
    {
        pSignalEvent->ErrorCause = Cause;
        pSignalEvent->ErrorStatus = ErrorStatus;
        AO_Publish(pSignalEvent, NULL);
        Status = true;
    }
    else
    {
        Log(DBG, "Signia Event allocation error in Fault Handler Runtime");
        Status = false;
    }
    return Status;
}

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

