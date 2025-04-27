#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L4_DisplayManager
 * \{
 *
 * \brief   Display Manager Functions
 *
 * \details Display manager allows applications to develop graphical content in
 *          the form of screens. A screen is a collection of graphical elements
 *          called widgets(Window Objects) and methods to interact with them.
 *          Display manager is responsible drawing these elements on screen,
 *          refresh widgets periodically or when updated
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_DisplayManager.c
 *
 * ========================================================================== */
/* \todo FUTURE ENHANCEMENT <01/15/2020> <KJ> <Add screen id is enum and force user to add screens in the enum> */
/* \todo FUTURE ENHANCEMENT <01/15/2020> <KJ> <Optimize all update functions> */
/* \todo FUTURE ENHANCEMENT <01/15/2020> <KJ> <Optimize show/hide widget implementation, currently entire screen refreshed > */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "L3_GuiWidgets.h"
#include "L3_DispPort.h"
//#include "L4_ChargerManager.h"

uint8_t GlobalCurrentSequenceIndex;
//extern void EEA_PercentageCounter(void);
static bool sb_UIthreadIsStarted = false;
static bool sb_UIthreadIsRunning = false;
static bool sb_DoNotRedrawAnyMore = false;
static bool sb_PlaySequenceOnlyOnce = false;
/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER    (LOG_GROUP_DISPLAY)         /*! Log Group Identifier */
#define DM_TASK_STACK           (512u)                     /*! Task stack size */
#define DM_REFRESH_TICKS        (DISPLAY_REFRESH_INTERVAL * 1000/OS_TICKS_PER_SEC)  /*! Timer in ms  */
#define DM_YIELD_WHEN_DEAD      (1000u)              /*! Delay used to yield during critical errors */
#define DM_BACKGROUND_COLOR     (SIG_COLOR_BLACK)    /*! Default background color used when screen cleared */
#define SCREEN_REFRESH_TIME     (MSEC_300)           /*! Screen refresh time period */

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
OS_STK DmTaskStack[DM_TASK_STACK + MEMORY_FENCE_SIZE_DWORDS];  /* Display Manager task stack */

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
/*! \struct ScreenInf
 *   Screen structure.
 */

typedef struct
{
    UI_SEQUENCE *pActiveScreen;                   /*! Pointer to active screen */
    //UI_SEQUENCE *pTempScreen;                /*! Pointer to Temp screen attributes*/
    //bool HasClip;                               /*! If clip widget is present on active screen */
    bool Refresh;                               /*! Refresh flag */
    //UI_SEQUENCE *pStaticScreen;                   /*! Pointer to active screen */
    //bool RefreshStaticScreen;                   /*! Refresh static screen flag */    
    //bool CopyToLCD;
    //uint32_t RefreshTmr;
    uint32_t RefreshRate;
    SCREEN_LOCK ScreenLock;
} SCREENINF_NEW;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static OS_EVENT *pMutexDispManager = NULL;        /*! Display manager mutex */
static OS_EVENT *pSemDisplayUpdate;               /*! Semaphore to pend */
//static ScreenInf ScreenInfo;                      /*! Screen details */
static SCREENINF_NEW ScreenInfo_New;                      /*! Screen details */
//static DM_SCREEN *pSavedScreen;                   /*! Screen pointer to the saved screen */
bool g_bUseNewUIThreadFunction = false;
/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void DmTask(void *pArg);
//static DM_STATUS DmUpdateDisplay(DM_SCREEN *pScreen, bool ForceRefresh, bool CopyToLCD);
//static DM_STATUS DmClipRefresh(DM_SCREEN *pScreen);
static DM_STATUS DmUpdateDisplay_New(UI_SEQUENCE *pSequence);
//static DM_STATUS L4_DmRefreshScreen_New(void);
static void ScreenInfoInit(void);
/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
void ScreenInfoInit(void)
{
    ScreenInfo_New.pActiveScreen = NULL;                   /*! Pointer to active screen */
    ScreenInfo_New.RefreshRate = UI_SEQUENCE_DEFAULT_REFRESH_RATE;
    ScreenInfo_New.ScreenLock = SCREEN_LOCK_OFF;
}
/* ========================================================================== */
/**
 * \brief   Display Manager task
 *
 * \details The task waits on the semaphore to refresh the screen.
 *
 * \param   pArg - Task arguments
 *
 * \return  None
 *
 * ========================================================================== */
static void DmTask(void *pArg)
{
  uint8_t OsError;
  
  ScreenInfoInit();
  
  while (true)
  {
    
     // Give other threads / tasks a chance:
      OSTimeDly(ScreenInfo_New.RefreshRate);
           
      if(NULL != ScreenInfo_New.pActiveScreen)
      {
        OSMutexPend(pMutexDispManager, OS_WAIT_FOREVER, &OsError);
        if(OS_ERR_NONE != OsError)
        {
            Log(ERR, "DmTask: OSMutexPend error");
            /* \todo MISSING <01/15/2020> <KJ> <Add exception handler here> */
            break;
        }
        DmUpdateDisplay_New(ScreenInfo_New.pActiveScreen);
        OSMutexPost(pMutexDispManager);
      }
    
      sb_UIthreadIsRunning = true; // this flad says our UI task is REALLY started and running
  }
}
/* ========================================================================== */
//#####################################################################################################
void L4_SetPlaySequenceOnlyOnce(void)
{
  uint8_t OsError;
  //  UI thread Mutex lock 
        OSMutexPend(pMutexDispManager, OS_WAIT_FOREVER, &OsError);
        if(OS_ERR_NONE != OsError)
        {
            Log(ERR, "L4_SetPlaySequenceOnlyOnce(): OSMutexPend error");
            /* \todo MISSING <01/15/2020> <KJ> <Add exception handler here> */
            return;
        }

  sb_PlaySequenceOnlyOnce = true;

  // Mutex release 
        OSMutexPost(pMutexDispManager);
}
//#####################################################################################################
/**
 * \brief   Update the display
 *
 * \details Update the widgets on the display
 *
 * \note    This function is invoked by the DmTask.
 *
 * \param   pScreen - pointer to Screen object containing all the widget details
 * \param   ForceRefresh - force refresh screen after paint window
 * \param   CopyToLCD - update screen
 *
 * \return  DM_STATUS - Status
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success.
 *
 * ========================================================================== */
//###########################################################################################################
static DM_STATUS DmUpdateDisplay_New(UI_SEQUENCE *pSequence)
{
  DM_STATUS Status;                   /* contains status */
  UI_SCREEN* pCurrentScreen;
  UI_OBJECT* pCurrentUIObject;
  uint8_t uiCurrentObjectIndex;
  
  Status = DM_STATUS_ERROR;
  
  do
  {
    if (NULL == &pSequence[0]) break; // i.e. return an error
    
    if(sb_DoNotRedrawAnyMore)
    {
      Status = DM_STATUS_OK;
      break;
    }
    
    pCurrentScreen = (&pSequence[GlobalCurrentSequenceIndex])->OurUIScreen;
    
    if (NULL == pCurrentScreen) // play the sequence from the beginning
    {
      if(!sb_PlaySequenceOnlyOnce)
      {
        GlobalCurrentSequenceIndex = 0;
        pCurrentScreen = (&pSequence[GlobalCurrentSequenceIndex])->OurUIScreen;
      }
      else
      {
        sb_PlaySequenceOnlyOnce = false; 
        sb_DoNotRedrawAnyMore = true; // to prevent further redrawings
        break;
      }
    }
    
    uiCurrentObjectIndex = 0;
    
    do
    {
      pCurrentUIObject = (&pCurrentScreen[uiCurrentObjectIndex])->OurUIObject;
      if(NULL == pCurrentUIObject) break;
      
      // Look at all items on the current screen:
      // First, we execute the function, if there is one:
      
      if(NULL != pCurrentUIObject -> pFunction)
        pCurrentUIObject -> pFunction(0);
      
      // Then, we see what object is it:
      switch(pCurrentUIObject->OurObjectType)
      {
      case UI_TYPE_TEXT:
        // draw text
        L3_WidgetTextDraw_New(&pCurrentUIObject->ObjText);    
        break;
        
      case UI_TYPE_SQUARE:
        // draw square
        break;
        
      case UI_TYPE_CIRCLE:
        // draw circle
        L3_WidgetCircleDraw(&pCurrentUIObject->ObjCircle);
        break;
        
      case UI_TYPE_BITMAP:
        // draw image
        L3_WidgetImageDraw_New(&pCurrentUIObject->ObjBitmap);
        break;
        
      case UI_TYPE_PROGRESS:
        // draw progress bar
        L3_WidgetProgressBarDraw_New(&pCurrentUIObject->ObjProgress);
        break;
      } // switch
      
      ++uiCurrentObjectIndex;
    } while(1); // do
    
    Status = DM_STATUS_OK;
    
    L3_DispMemDevCopyToLCD();
    ++GlobalCurrentSequenceIndex; // go to the next screen in the sequence
  } while (false);
  
  return Status;
}
//##############################################################################################################
 /* ========================================================================== */
/**
 * \brief   Check point is window area.
 *
 * \details This function checks if the specified point overlaps a window region
 *
 * \param   Px - X Coordinate of the point
 * \param   Py - Y Coordinate of the point
 * \param   X  - X Coordinate of the Window
 * \param   Y  - Y Coordinate of the Window
 * \param   Width  - Window width
 * \param   Height  - Window height
 *
 * \return  bool - true if the point overlaps with window
 *
 * ========================================================================== */
bool IsPointInWindow (uint8_t Px, uint8_t Py, uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height )
{
    bool Result;

    Result = ((Px >= X) && (Px <= X + Width) && (Py >= Y) && (Px <= Y - Height)) ? true : false;

    return Result;
}

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Function to initialize the Display Manager
 *
 * \details This function creates the display manager task, mutex and timer
 *
 * \param   < None >
 *
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success
 *
 * ========================================================================== */
DM_STATUS L4_DmInit(void)
{
    DM_STATUS       DmStatus;                  /* contains status */
    uint8_t         OsError;                   /* contains status of error */
    
    if(!sb_UIthreadIsStarted)
    {
    do
    {
        DmStatus = DM_STATUS_OK;     /* Default Status */
        
         /* Initialize the structures with default values*/

        /* Create the Display Manager Task */
        OsError = SigTaskCreate(&DmTask,
                                    NULL,
                                    &DmTaskStack[0],
                                    TASK_PRIORITY_L4_DISPMANAGER,
                                    //TASK_PRIORITY_TM,
                                    DM_TASK_STACK,
                                "DisplayMgr");

        if (OsError != OS_ERR_NONE)
        {
            /* Couldn't create task, exit with error */
            DmStatus = DM_STATUS_ERROR;
            Log(ERR, "L4_DM_Init: DmTask Create Error - %d", OsError);
            break;
        }

        pMutexDispManager = SigMutexCreate("L4-DISPMAN", &OsError);

        if (NULL == pMutexDispManager)
        {
            /* Couldn't create mutex, exit with error */
            DmStatus = DM_STATUS_ERROR;
            Log(ERR, "L4_DM_Init: Display Manager Mutex Create Error - %d", OsError);
            break;
        }

        pSemDisplayUpdate = SigSemCreate(0, "Dm-Sem", &OsError);    /* Semaphore to block by default */
        if (NULL == pSemDisplayUpdate)
        {
            /* Error in semaphore creation, return with error */
            DmStatus = DM_STATUS_ERROR;
            Log(ERR, "DmTask: Create Semaphore Error");
            break;
        }

        /* Initialization done */
        sb_UIthreadIsStarted = true;

    } while (false);
    } // if(!sb_UIthreadIsStarted)
    return DmStatus;
}

/* ========================================================================== */
/**
 * \brief   Function to draw specified screen on display
 *
 * \details This function copies to the screen object to an empty slot
 *
 * \param   pScreen - Screen object to display
 *
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success
 *
 * ========================================================================== */
//############################################################################################################
DM_STATUS L4_DmShowScreen_New(SCREEN_ID ScreenID, uint32_t RefreshRate, UI_SEQUENCE* pSequence)
{
    DM_STATUS DmStatus;                 /* contains the status */
    DM_STATUS UiThreadStatus;
    uint8_t OsError;                    /* contains the OS error status */
    static  SCREEN_ID PreviousScreenID = SCREEN_ID_NONE;  /* suppress screen messages for unchanged screen updates */

    DmStatus = DM_STATUS_ERROR;         /* Default Status */
    UiThreadStatus = DM_STATUS_OK;
    g_bUseNewUIThreadFunction = true;
    sb_DoNotRedrawAnyMore = false;
    //g_pScreenSequenceBeginning = pSequence->OurUIScreen; // Need to remember the beginning of our sequence
    GlobalCurrentSequenceIndex = 0;
    
    if(!sb_UIthreadIsStarted)
    {
      UiThreadStatus = L4_DmInit();
    }
    
    do
    {
        BREAK_IF (DM_STATUS_OK != UiThreadStatus);
        BREAK_IF ( SCREEN_LOCK_OFF != ScreenInfo_New.ScreenLock );
        BREAK_IF (NULL == pSequence);

        while(!sb_UIthreadIsRunning) // wait until UI task/thread is fully running
        {
           OSTimeDly(50);          
        }
        
        // checking active screen is the same as we try to switch to: 
        if ((ScreenInfo_New.pActiveScreen == pSequence) && (ScreenInfo_New.Refresh == false))
        {
            ScreenInfo_New.Refresh = true;
            DmStatus = DM_STATUS_OK;
            break;
        }
        

        /*  Mutex lock */
        OSMutexPend(pMutexDispManager, OS_WAIT_FOREVER, &OsError);
        if(OS_ERR_NONE != OsError)
        {
            Log(ERR, "L4_DmShowScreen_New: OSMutexPend error");
            /* \todo MISSING <01/15/2020> <KJ> <Add exception handler here> */
            break;
        }
        DmStatus = DM_STATUS_OK;

        ScreenInfo_New.pActiveScreen = pSequence;
        ScreenInfo_New.RefreshRate = RefreshRate;
        ///\todo 08/07/2022 SK - This would need the Screen id would be unique for the screen. 
        if (PreviousScreenID != ScreenID)
        {
            Log(REQ,"Screen Updated to ID: %d",ScreenID);
            PreviousScreenID = ScreenID;
        }
        /* Mutex release */
        OSMutexPost(pMutexDispManager);

    } while (false);

    return DmStatus;
}
//##############################################################################################################################        
/* ========================================================================== */
/**
 * \brief   Function to redraw all current screen objects
 *
 * \details This function posts the semaphore to redraw the screen
 *
 * \param   < None >
 *
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success
 *
 * ========================================================================== */
//######################################################################################################
DM_STATUS L4_DmRefreshScreen_New(void)
{
    DM_STATUS DmStatus;         /* contains the status */
    uint8_t OsError;            /* contains the OS error status */

    DmStatus = DM_STATUS_OK;

    do
    {
      OsError = OSSemPost(pSemDisplayUpdate);
      if (OS_ERR_NONE != OsError)
      {
          DmStatus = DM_STATUS_ERROR;
          /* \todo MISSING <01/15/2020> <KJ> <Add exception handler here> */
          break;
      }
      // To Refresh the Screen
      ScreenInfo_New.Refresh = true;
    } while (false);

    return DmStatus;
}
/* ========================================================================== */
/**
 * \brief   Function to lock the active screen
 *
 * \details This function locks the current active screen. Provides Temporary and Permanent Active Screen lock
 *          Temporary Lock - Once screen is Temporarily locked then other screen can be displayed if unlocked
 *          Permanent Lock - Once screen is Permanent locked no other screen can be displayed, until RESET
 *
 * \param   pScreen - Active screen object
 * \param   ScreenLock  
 *                    SCREEN_LOCK_OFF       - Active screen is not locked
 *                    SCREEN_LOCK_PERMANENT - Active screen is Permanently Locked, cannot be unlocked
 *                    SCREEN_LOCK_TEMPORARY - Active screen is Temporary Locked and can be unlocked
 *
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success
 *
 * ========================================================================== */
DM_STATUS L4_DmScreenLockUnLock_New(UI_SEQUENCE *pSequence, SCREEN_LOCK ScreenLock)
{
    DM_STATUS Status;         /* contains the status */
    uint8_t OsError;
    Status = DM_STATUS_ERROR;
    do
    {
        if ( NULL == pSequence )
        {
             break;
        }
        
        /* Is Screen Locked Permanently, then do not unlock */
        if ( SCREEN_LOCK_PERMANENT == ScreenInfo_New.ScreenLock )
        {
            break;
        }
        
        if ( ScreenInfo_New.pActiveScreen != pSequence)
        {
            break;
        }

        /*  Mutex lock */
        OSMutexPend(pMutexDispManager, OS_WAIT_FOREVER, &OsError);
        if (OS_ERR_NONE != OsError)
        {
            Log(ERR, "L4_DMPermanentScreenLock: OSMutexPend error");
            break;
        }

        ScreenInfo_New.ScreenLock = ScreenLock;
        Status = DM_STATUS_OK;
        /* Mutex release */
        OSMutexPost(pMutexDispManager);
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Function to return the lock status for permanent screen lock
 *
 * \details This function unlock the screen if temporary locked 
 *          if screen locked permanent then does not unlock the screen
 *
 * \param   ScreenLock  
 *                    SCREEN_LOCK_OFF       - Active screen is not locked
 *                    SCREEN_LOCK_PERMANENT - Active screen is Permanently Locked, cannot be unlocked
 *                    SCREEN_LOCK_TEMPORARY - Active screen is Temporary Locked and can be unlocked
 *
 * \return  DM_STATUS - DmStatus
 * \retval  DM_STATUS_ERROR - On Error
 * \retval  DM_STATUS_OK - On success
 *
 * ========================================================================== */
DM_STATUS L4_DmCurrentScreenLockUnLock_New(SCREEN_LOCK ScreenLock)
{
    DM_STATUS Status;         /* contains the status */
    uint8_t OsError;
    Status = DM_STATUS_ERROR;
    do
    {
        /* Is Screen Locked Permanently, then do not unlock */
        if ( SCREEN_LOCK_PERMANENT == ScreenInfo_New.ScreenLock )
        {
            break;
        }
        
        if ( ScreenInfo_New.pActiveScreen == NULL)
        {
            break;
        }

        /*  Mutex lock */
        OSMutexPend(pMutexDispManager, OS_WAIT_FOREVER, &OsError);
        if (OS_ERR_NONE != OsError)
        {
            Log(ERR, "L4_DMPermanentScreenLock: OSMutexPend error");
            break;
        }

        ScreenInfo_New.ScreenLock = ScreenLock;
        Status = DM_STATUS_OK;
        /* Mutex release */
        OSMutexPost(pMutexDispManager);
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Function to return the screen lock status 
 *
 * \details This function returns the screen lock status for permanent lock and
 *          clears the screen lock if screen is temporary locked 
 *          lock
 *
 * \param   < None >
 *
 * \return  True/False
 *
 * ========================================================================== */
bool L4_DmScreenUnlockTemp_New(void)
{
    bool Status;
    do
    {
        if ((SCREEN_LOCK_OFF == ScreenInfo_New.ScreenLock) || (SCREEN_LOCK_TEMPORARY == ScreenInfo_New.ScreenLock))
        {
            Status = false;
            ScreenInfo_New.ScreenLock = SCREEN_LOCK_OFF;
        }
        else if (SCREEN_LOCK_PERMANENT == ScreenInfo_New.ScreenLock)
        {
            Status = true;
        }
        else
        {
            ;//Do Nothing
        }
    }while (false);
    return Status;
}

/* ========================================================================== */
/**
 * \brief   Function to return the lock status
 *
 * \details This function returns the screen lock status for temporary/permanent
 *          lock
 *
 * \param   < None >
 *
 * \return  True/False
 *
 * ========================================================================== */
bool L4_DmIsScreenLocked_New(void)
{
    return (SCREEN_LOCK_OFF != ScreenInfo_New.ScreenLock);
}
/* ========================================================================== */
/**
 * \brief   This is a wrapper function to draw color
 *
 * \details This function select the color depending up on the parameter
 *
 * \param   Color -  color Id
 * 
 * \return  None
 *
 * ========================================================================== */
void L4_DmDrawColor(uint32_t Color)
{
    L3_DispSetColor(Color);
}

/* ========================================================================== */
/**
 * \brief   This is a wrapper function to draw a circle 
 *
 * \details This function draw the circle depending up on the selected position
 *          and radius on active screen
 *
 * \param   x -  x-coordinate
 * \param   y -  y-coordinate
 * \param   Radius - Circle Radius
 *
 * \return  None
 *
 * ========================================================================== */
void L4_DmDrawCircle(int8_t x, int8_t y, int8_t Radius)
{
    L3_DispDrawCircle(x, y, Radius);
}

/* ========================================================================== */
/**
 * \brief   This is a wrapper function to fill a rectangle 
 *
 * \details This function select the color depending up on the parameter
 *
 * \param   X1 - x-coordinate
 * \param   Y1 - y-coordinate
 * \param   X2 - x-coordinate
 * \param   Y2 - y-coordinate
 * 
 * \return  None
 *
 * ========================================================================== */
void L4_DmFillRectangle(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2)
{
    L3_DispFillRect(X1, Y1, X2, Y2);
}


/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

