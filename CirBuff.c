#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup Utils
 * \{
 *
 * \brief   Circular Buffer Implementation.
 *
 * \details This module implements a FIFO circular buffer and provides
 *          supporting functions for: 
 *          - Initializing the buffer
 *          - Pushing bytes to the buffer
 *          - Popping bytes from the buffer
 *          - Obtaining the available data bytes
 *          - Obtaining the space remaining
 *          - Adjusting the head and tail pointers
 *          - Peeking into the buffer
 *          - Clearing the buffer
 *
 * \note    NOT thread safe!
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include <Common.h>
#include "CirBuff.h"
    
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER   (LOG_GROUP_GENERAL)  ///< Identifier for log entries

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
    
/* ========================================================================== */
/**
 * \brief  Increments the head pointer. 
 *
 * \details This function increments the head pointer by number of added items.
 *
 * \param   pCB - Pointer to the CirBuffer
 * \param   u16AddedCount - Added items count to increment head pointer
 * 
 * \return  None
 *
 * ========================================================================== */
static void IncrementHead(CIR_BUFF *pCB, uint16_t u16AddedCount)
{
    do
    {   /* Check the input validity */
        if ((NULL == pCB) || (0 == u16AddedCount))
        {
            break;
        }
        
        /* Update the Head position */
        pCB->u16HeadPos += u16AddedCount;
        
        while (pCB->u16HeadPos >= pCB->u16BuffSize)
        {
            pCB->u16HeadPos -= pCB->u16BuffSize;
        }
        
        pCB->bIsEmpty = false;
        
    } while (false);
}

/* ========================================================================== */
/**
 * \brief  Increments the tail pointer. 
 *
 * \details This function increments the tail pointer by number of removed items. 
 *
 * \param   pCB - Pointer to the CirBuffer
 * \param   u16RemovedCount - Removed items count to increment tail pointer
 * 
 * \return  None
 *
* ========================================================================== */
static void IncrementTail(CIR_BUFF *pCB, uint16_t u16RemovedCount)
{
    do
    {   /* Check the input validity */
        if ((NULL == pCB) || (0 == u16RemovedCount))
        {
            break;
        }        
        
        if (u16RemovedCount >= CirBuffCount(pCB))
        {
            CirBuffClear(pCB);
            break;
        }
        
        pCB->u16TailPos += u16RemovedCount;
        
        while (pCB->u16TailPos >= pCB->u16BuffSize)
        {
            pCB->u16TailPos -= pCB->u16BuffSize;
        }
        
        if (pCB->u16TailPos == pCB->u16HeadPos)
        {
            CirBuffClear(pCB);
        }
        
    } while (false);
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Initializes the buffer
 *
 * \details This function initializes the circular buffer with a given data 
 *          buffer and size 
 *
 * \param   pCB - Pointer to the CirBuffer object.
 * \param   pDataBuffIn - Pointer to data buffer memory.
 * \param   u16BuffSizeIn - Maximum size of the data buffer memory
 * 
 * \return  bool - false, init success, true otherwise.
 *
 * ========================================================================== */
bool CirBuffInit(CIR_BUFF *pCB, uint8_t *pDataBuffIn, uint16_t u16BuffSizeIn)
{
    bool bStatus; /* Function error status. True if error. */
    
    /* Start with Error status */
    bStatus = true;
    
    if (NULL != pCB)
    {                
        pCB->pDataBuff = pDataBuffIn;
        
        pCB->u16BuffSize = u16BuffSizeIn;
        
        CirBuffClear(pCB);
        
        /* Success */
        bStatus = false;
    }
    
    return bStatus;
}

/* ========================================================================== */
/**
 * \brief   Pushes bytes to the buffer
 *
 * \details This function adds new data to the head of the buffer.
 *
 * \note    May overflow the Cirbuffer. Check return value for actual count.
 *
 * \param   pCB - Pointer to the CirBuffer object.
 * \param   pDataIn - Pointer to the incoming data buffer to be added.
 * \param   u16DataCountIn - Count of the incoming data buffer
 * 
 * \return  uint16_t - Returns the number of items actually added to the CirBuffer.
 *
 * ========================================================================== */
uint16_t CirBuffPush(CIR_BUFF *pCB, uint8_t *pDataIn, uint16_t u16DataCountIn)
{
    uint16_t u16SpaceAtEnd;
    uint16_t u16SavedCount;
    uint16_t u16SpaceRemaining;
    
    u16SavedCount = 0u; // Initialize variable
    
    do
    {   
        /* Check the pointer validity */
        if ((NULL == pCB) || (NULL == pDataIn))
        {
            break;
        }  
        
        /* Assume we can fit all the data. */
        u16SavedCount = u16DataCountIn;    
        u16SpaceRemaining = CirBuffFreeSpace(pCB);        
        
        /* Check the data count validity */
        if ((0 == u16DataCountIn) || (0 == u16SpaceRemaining))
        {
            break;
        }
        
        if (u16DataCountIn > u16SpaceRemaining)
        {
            /* Oops, can't fit all the data.  Overflow!  */
            u16SavedCount = u16SpaceRemaining;  
        }
        
        if (1 == u16SavedCount)
        {
            pCB->pDataBuff[pCB->u16HeadPos] = *pDataIn;
            IncrementHead(pCB, 1);
            break;
        }
        
        if (pCB->u16HeadPos >= pCB->u16TailPos)
        {
            if (u16SavedCount <= pCB->u16BuffSize - pCB->u16HeadPos)
            {
                /* Can fit data without wrapping  */
                memcpy(&(pCB->pDataBuff[pCB->u16HeadPos]), pDataIn, u16SavedCount);
                IncrementHead(pCB, u16SavedCount);
            }
            else
            {
                /* Data will wrap  */
                u16SpaceAtEnd = pCB->u16BuffSize - pCB->u16HeadPos;
                memcpy(&(pCB->pDataBuff[pCB->u16HeadPos]), pDataIn, u16SpaceAtEnd);
                memcpy(pCB->pDataBuff, &pDataIn[u16SpaceAtEnd], u16SavedCount - u16SpaceAtEnd);
                IncrementHead(pCB, u16SavedCount);
            }
            break;
        }
        
        /* No wrapping of data.  */
        memcpy(&(pCB->pDataBuff[pCB->u16HeadPos]), pDataIn, u16SavedCount);
        IncrementHead(pCB, u16SavedCount);
        
    } while (false);
    
    return u16SavedCount;
}

/* ========================================================================== */
/**
 * \brief   Peeks into the buffer
 *
 * \details This function get the oldest data from the tail of the buffer
 *
 * \note    Doesn't remove the data.
 *
 * \param   pCB - Pointer to the CirBuffer object.
 * \param   pDataOut - Pointer to the data buffer to get oldest data
 * \param   u16DataCountIn - Requested number of data items
 * 
 * \return  uint16_t - Returns the number of items actually read.
 *
 * ========================================================================== */
uint16_t CirBuffPeek(CIR_BUFF *pCB, uint8_t *pDataOut, uint16_t u16DataCountIn)
{
    uint16_t u16DataCountAtEnd;
    uint16_t u16ReadCount;
    uint16_t u16TotalCount;

    u16ReadCount = 0;

    do
    { 
        /* Check the pointer validity */
        if ((NULL == pCB) || (NULL == pDataOut))
        {
            break;
        } 
        
        /* Assume we have that much data.  */
        u16ReadCount = u16DataCountIn;     
        u16TotalCount = CirBuffCount(pCB);
        
        /* Check the data count validity */
        if ((0 == u16DataCountIn) || (0 == u16TotalCount))
        {
            break;
        }  
        
        if (u16DataCountIn > u16TotalCount)
        {
            /* We don't have that much data, send all we've got.*/
            u16ReadCount = u16TotalCount;      
        }
        
        if (1 == u16ReadCount)
        {
            *pDataOut = pCB->pDataBuff[pCB->u16TailPos];
            break;
        }   
        
        if ((pCB->u16HeadPos > pCB->u16TailPos) || 
            (u16ReadCount < pCB->u16BuffSize - pCB->u16TailPos))
        {
            /* Can get data without wrapping  */
            memcpy(pDataOut, &(pCB->pDataBuff[pCB->u16TailPos]), u16ReadCount);
            break;
        }   
        
        /* Data is wrapped */
        u16DataCountAtEnd = pCB->u16BuffSize - pCB->u16TailPos;
        memcpy(pDataOut, &(pCB->pDataBuff[pCB->u16TailPos]), u16DataCountAtEnd);
        memcpy(&pDataOut[u16DataCountAtEnd], pCB->pDataBuff, u16ReadCount - u16DataCountAtEnd);           
        
    } while (false);
    
    return u16ReadCount;
}

/* ========================================================================== */
/**
 * \brief   Pops bytes from the buffer
 *
 * \details This function removes items from the tail of the buffer.
 *
 * \param   pCB - Pointer to the CirBuffer object.
 * \param   u16DataCountIn - Requested number of data items to be popped.
 * 
 * \return  uint16_t - Actual number of data items been popped.
 *
 * ========================================================================== */
uint16_t CirBuffPop(CIR_BUFF *pCB, uint16_t u16DataCountIn)
{
    uint16_t u16RemovedCount;
    uint16_t u16TotalCount;

    u16RemovedCount = 0u;    

    do
    { 
        /* Check the input validity */
        if ((NULL == pCB) || (0 == u16DataCountIn))
        {
            break;
        }
        
        /* Get the counts */
        u16RemovedCount = u16DataCountIn;
        u16TotalCount = CirBuffCount(pCB);
        
        if (u16DataCountIn >= u16TotalCount)
        {
            /* We don't have that much data, remove all. */
            u16RemovedCount = u16TotalCount;   
            
            CirBuffClear(pCB);
            
            break;
        }
        
        IncrementTail(pCB, u16RemovedCount);
        
    } while (false);
    
    return u16RemovedCount;
}

/* ========================================================================== */
/**
 * \brief   Obtains the available data byte count.
 *
 * \details This function retrieves the available data count in the CirBuffer.
 *
 * \param   pCB - Pointer to the CirBuffer object.
 * 
 * \return  uint16_t - Returns the number of available data in the CirBuffer
 *
 * ========================================================================== */
uint16_t CirBuffCount(CIR_BUFF *pCB)
{
    uint16_t u16AvailableCount = 0;
    
    do
    {   /* Check the input validity */
        if (NULL == pCB)
        {
            break;
        }

        /* Is Empty? */
        if (pCB->bIsEmpty)
        {
            break;
        }
        
        if (pCB->u16HeadPos > pCB->u16TailPos)
        {
            u16AvailableCount =  pCB->u16HeadPos - pCB->u16TailPos;
            
            break;
        }
        
        /* Get the available count */
        u16AvailableCount =  pCB->u16BuffSize - (pCB->u16TailPos - pCB->u16HeadPos);        
        
    } while (false);
    
    return u16AvailableCount;
}

/* ========================================================================== */
/**
 * \brief   Obtains the space remaining
 *
 * \details This function retrieves remaining free space remaining in the 
 *          circular buffer.
 *
 * \param   pCB - Pointer to the CirBuffer object.
 * 
 * \return  uint16_t - Returns the number of free spaces in the circular buffer
 *
 * ========================================================================== */
uint16_t CirBuffFreeSpace(CIR_BUFF *pCB)
{
    uint16_t u16Count = 0;
    
    /* Check the input validity */
    if (NULL != pCB)
    {
        u16Count = pCB->u16BuffSize - CirBuffCount(pCB);
    }    
    
    return u16Count;
}

/* ========================================================================== */
/**
 * \brief   Clears the buffer
 *
 * \details This function clears the circular buffer to empty.
 *
 * \param   pCB - Pointer to the CirBuffer object.
 * 
 * \return  None
 *
 * ========================================================================== */
void CirBuffClear(CIR_BUFF *pCB)
{
    /* Check the input validity */
    if (NULL != pCB)
    {
        pCB->u16HeadPos = 0;
        pCB->u16TailPos = 0;
        pCB->bIsEmpty = true;
    }  
}

/**
 * \}   <this marks the end of the Doxygen group>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

