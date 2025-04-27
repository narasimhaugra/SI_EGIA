#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \brief   Keypad module
 *
 * \details This module provides task, functions and APIs for keypad initilization, 
 *          keypad scan, key state notifications, key pattern recognition. 
 *          A total of ten keys are scanned and their states notified. APIs are provided 
 *          for applications to register their own handlers for multilple key state 
 *          change notification, watch and  unwatch key patterns.
 *        
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Signia_Keypad.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "Signia_Keypad.h"
#include "L3_GpioCtrl.h"
#include "Signia_KeypadEvents.h"
#include "FaultHandler.h"
#include "L3_OneWireController.h"
#include "L3_DispPort.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER            (LOG_GROUP_KEYPAD)  ///< Log Group Identifier
#define KEYPAD_TASK_STACK               (512u)              ///< Task stack size 
#define DEBOUNCE_DELAY                  (MSEC_10)
#define MAX_PATTERN_SEQUENCE            (10u)                ///< count of keysets a pattern sequence can have
#define MAX_KEY_PATTERN_COUNT           (10u)               ///< maximum key patterns that can be registered
#define KEY_IDLE_DURATION               (SEC_2)
#define SIG_MASK(SIG_ID)                (1 << SIG_ID)
#define MAX_KEY_ROTATIONCONFIG          (2u)               ///< Maximum Key patterns per Rotation configuration
#define KEY_ROTATIONCONFIG_COUNT        (2u)               ///< Maximum Key Count for Rotation Configuration
#define MAX_ROTATION_COUNTDOWN_SCREENS  (3u)               ///< Maximum Rotation Count down Screens
#define MAX_KEY_SHIPMODE_SEQ            (9u)               ///< Maximum Key patterns per ShipMode key sequence           
#define KEY_SHIPMODE_SEQ_COUNT          (4u)               ///< Maximum Key Sequence for ShipMode
#define END_OF_PATTERN                  (0x00)             ///< End of pattern indication
#define GET_KEY(KeyID)                  (1 << KeyID)
#define DEBOUNCE_SAMPLE_COUNT           (3u)
#define DEFAULT_MAX_PATTERN_TIMEOUT     (3000u)
#define KEY_SOFTRESET_PATTERN_TYPE_COUNT (1u)              ///< Various key patterns to invoke Soft Reset
#define KEY_SOFTRESET_PATTERN_COUNT     (2)
#define DELAY_BEFORE_RESET              (SEC_2)            ///< Delay before reset
/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
OS_EVENT *pSemaKeyISR;    /*!< Semaphore to pend during the request processing */
OS_STK KeypadTaskStack[KEYPAD_TASK_STACK+MEMORY_FENCE_SIZE_DWORDS];  /* Keypad task stack */
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
 /*! \enum KEYPAD_PROC_STATE
 *   Request key handler state-machine State.
 */
typedef enum
{
    KEY_PROC_STATE_IDLE,   /*!< The key processor is not executing any requests */
    KEY_PROC_STATE_SCAN,   /*!< Performing scan */
    KEY_PROC_STATE_LAST    /*!< State range indicator */
} KEYPAD_PROC_STATE;

typedef struct
{
   uint16_t KeyPressed;
   uint16_t KeyReleased;
   uint16_t DurationActive[KEY_COUNT];
   uint16_t DurationInActive[KEY_COUNT];
} KEY_LOG;

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/
/*\todo 08/31/2021 BS - "duration Min" field is not used in current implementation */ 
static const KEY_PATTERN shipmodepatternTable[KEY_SHIPMODE_SEQ_COUNT][MAX_KEY_SHIPMODE_SEQ] =     
{
	    /* Keypattern,            duration min,           duration max ,    Action on release?*/
    {   /* ShipMode sequence 1 with two Safety Left button press*/
        { GET_KEY(TOGGLE_DOWN),         0,                      1000,     true },
        { GET_KEY(TOGGLE_UP),           0,                      1000,     true },
        { GET_KEY(TOGGLE_DOWN),         0,                      1000,     true },
        { GET_KEY(TOGGLE_UP),           0,                      1000,     true },
        { GET_KEY(TOGGLE_DOWN),         0,                      1000,     true },
        { GET_KEY(TOGGLE_UP),           0,                      1000,     true },
        { GET_KEY(SAFETY_LEFT),         0,                      1000,     true },
        { GET_KEY(SAFETY_LEFT),         0,                      1000,     true },
        { END_OF_PATTERN,	END_OF_PATTERN,         END_OF_PATTERN,  false}
    },
    {   /* ShipMode sequence 2 with two Safety Right button press*/
        { GET_KEY(TOGGLE_DOWN),         0,                      1000,     true },
        { GET_KEY(TOGGLE_UP),           0,                      1000,     true },
        { GET_KEY(TOGGLE_DOWN),         0,                      1000,     true },
        { GET_KEY(TOGGLE_UP),           0,                      1000,     true },
        { GET_KEY(TOGGLE_DOWN),         0,                      1000,     true },
        { GET_KEY(TOGGLE_UP),           0,                      1000,     true },
        { GET_KEY(SAFETY_RIGHT),        0,                      1000,     true },
        { GET_KEY(SAFETY_RIGHT),        0,                      1000,     true },
        { END_OF_PATTERN,           END_OF_PATTERN,       END_OF_PATTERN, false }
    },
    {   /* ShipMode sequence 3 with Safety Right &  Safety Left button press*/
        { GET_KEY(TOGGLE_DOWN),         0,                      1000,     true },
        { GET_KEY(TOGGLE_UP),           0,                      1000,     true },
        { GET_KEY(TOGGLE_DOWN),         0,                      1000,     true },
        { GET_KEY(TOGGLE_UP),           0,                      1000,     true },
        { GET_KEY(TOGGLE_DOWN),         0,                      1000,     true },
        { GET_KEY(TOGGLE_UP),           0,                      1000,     true },
        { GET_KEY(SAFETY_RIGHT),        0,                      1000,     true },
        { GET_KEY(SAFETY_LEFT),         0,                      1000,     true },
        { END_OF_PATTERN,           END_OF_PATTERN,       END_OF_PATTERN, false }
    },
    {   /* ShipMode sequence 4 with Safety Left & Safety Right button press*/
        { GET_KEY(TOGGLE_DOWN),         0,                      1000,     true },
        { GET_KEY(TOGGLE_UP),           0,                      1000,     true },
        { GET_KEY(TOGGLE_DOWN),         0,                      1000,     true },
        { GET_KEY(TOGGLE_UP),           0,                      1000,     true },
        { GET_KEY(TOGGLE_DOWN),         0,                      1000,     true },
        { GET_KEY(TOGGLE_UP),           0,                      1000,     true },
        { GET_KEY(SAFETY_LEFT),         0,                      1000,     true },
        { GET_KEY(SAFETY_RIGHT),        0,                      1000,     true },
        { END_OF_PATTERN,           END_OF_PATTERN,       END_OF_PATTERN, false }
    }
};

static const KEY_PATTERN RotationConfigPatternTable[KEY_ROTATIONCONFIG_COUNT][MAX_KEY_ROTATIONCONFIG] =
{ 
       // Keypattern,                                               duration min,           duration max,  Action on release?
    {  // Rotation Config for Right Side 
        { GET_KEY(LATERAL_RIGHT_UP) | GET_KEY(LATERAL_RIGHT_DOWN),      3000,                    3000,        false },
        { END_OF_PATTERN,                                               END_OF_PATTERN,       END_OF_PATTERN, false }
    },

    {  // Rotation Config for Left Side 
        { GET_KEY(LATERAL_LEFT_UP) | GET_KEY(LATERAL_LEFT_DOWN),        3000,                    3000,         false },
        { END_OF_PATTERN,                                               END_OF_PATTERN,       END_OF_PATTERN,  false }
    }
};

// Below structure array holding Valid Soft Reset Patterns 
static const KEY_PATTERN SoftResetPatternTable[KEY_SOFTRESET_PATTERN_TYPE_COUNT][KEY_SOFTRESET_PATTERN_COUNT] =
{
  // Key pattern                                                  Duration min,         duration max,    Action on release    
  { //soft reset pattern
    { (GET_KEY(SAFETY_LEFT) | GET_KEY(SAFETY_RIGHT) ),             4000,                    0,             false },
    {  END_OF_PATTERN,                                             END_OF_PATTERN,       END_OF_PATTERN ,  false }
  }
};

// GPIO signals for keys, The order here should match with enum KEY_ID
static const uint8_t KeypadSignal[KEY_COUNT] =
{
    GPIO_CLOSE_KEYn,
    GPIO_OPEN_KEYn,
    GPIO_LEFT_ARTIC_KEYn,
    GPIO_RIGHT_ARTIC_KEYn,
    GPIO_LEFT_CW_KEYn,
    GPIO_RIGHT_CW_KEYn,
    GPIO_LEFT_CCW_KEYn,
    GPIO_RIGHT_CCW_KEYn,
    GPIO_GN_KEY1n,
    GPIO_GN_KEY2n
};

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static keyPatternWatch RegisteredKeyPattern[MAX_KEY_PATTERN_COUNT];
static KEYPAD_HANDLER KeyHandler[KEY_COUNT];
static bool KeyscanPause;

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static KEYPAD_STATUS CheckKeyPattern(uint16_t KeyStates);
static void KeypadGPIOInit(void);
static void KeypadTask(void *pArg);
static void KeypadWakeup_IntCallback(void);
static void NotifyKeyStateChange(uint16_t KeyState);
static void Key_RotationConfigHandler(void);
static void Key_ShipModeHandler(void);
static uint16_t DebounceKeypad(KEY_LOG *pKLog, uint16_t *pKeyImage);
static void ScanKeypad(uint16_t *pKeyImage);
static void ScanDebounceNotifyKeyEvents(KEYPAD_PROC_STATE*);
static KEYPAD_STATUS RegisterSoftResetKeyPatterns(void);
static void SoftResetHandler(void);


/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   KeypadTask
 *
 * \details This starts scanning all the keys and notfies registered handlers
 *          on receiving a key press semaphore triggered from GPIO_KEY_WAKEn ISR.
 *          after all keys goes to idle the scanning ends and task goes back to
 *          waiting on the semaphore.
 *
 * \param   pArg - pointer to arguments
 *
 * \return  None
 *
 * ========================================================================== */
static void KeypadTask(void *pArg)
{
    uint8_t Error = false;
    static KEYPAD_PROC_STATE KeyProcState;

    // in default state wait for semaphore from key ISR
    KeyProcState = KEY_PROC_STATE_IDLE;

    while ( true )
    {
        switch ( KeyProcState )
        {
            case KEY_PROC_STATE_IDLE:
                // Wait for key press. Valid key press would move through rest of the state-machine.
                // Invalid keypress is discarded and state-machine waits for a new key press again

                // Wait for the key press semaphore indefinitely
                OSSemPend(pSemaKeyISR, 0, &Error);
                
                if (OS_ERR_NONE != Error)
                {
                    Log(ERR, "KeypadTask : OSSemPend error");
                    break;
                }
                KeyProcState = KEY_PROC_STATE_SCAN;            

            case KEY_PROC_STATE_SCAN:
                //scan, debounce and notify all keys events
                if(!KeyscanPause)
                {
                    ScanDebounceNotifyKeyEvents(&KeyProcState);
                }
                break;

            default:                
                KeyProcState = KEY_PROC_STATE_IDLE;
                break;
        }
        //this provides the debounce delay
        OSTimeDly(DEBOUNCE_DELAY);
    }
}

/* ========================================================================== */
/**
 * \brief   L4_KeypadInit
 *
 * \details Initialize Keypad Handler module and internal structures. Create wake
 *          key semaphores and create the keypad task.
 *
 * \param   < None >
 *
 * \return  KEY_PAD_STATUS - Key pad init status
 * \retval  KEYPAD_STATUS_OK
 * \retval  KEYPAD_STATUS_ERROR
 *
 * ========================================================================== */
KEYPAD_STATUS L4_KeypadInit( void )
{
    uint8_t         Index;
    uint8_t         Error;      /* OS error return temporary*/
    KEYPAD_STATUS   Status;     /* Operation status*/
    static bool     KeypadInitDone = false;           /* to mark initialization, avoid multiple inits*/

    do
    {
        Status = KEYPAD_STATUS_OK;     /* Default Status */

        if ( true == KeypadInitDone )
        {
            /* Repeated initialization attempt, do nothing */
            break;
        }

       /* Enable all GPIOs associated with keypad inputs  */
       KeypadGPIOInit();

       /*Initialize keypattern register*/
       for ( Index=0; Index < MAX_KEY_PATTERN_COUNT; Index++ )
       {
         RegisteredKeyPattern[Index].pKeyPattern = NULL;
       }
       /*Initialize keypad notification handler register*/
       for ( Index=0; Index < KEY_COUNT; Index++ )
       {
          KeyHandler[Index] = NULL;
       }
        /* Create keypad Handler task */
       Error = SigTaskCreate( &KeypadTask,
                              NULL,
                              &KeypadTaskStack[0],
                              TASK_PRIORITY_L4_KEYPAD_HANDLER,
                              KEYPAD_TASK_STACK,
                              "KeypadMgr");

        if ( Error != OS_ERR_NONE )
        {
            /* Couldn't create task, exit with error */
            Status = KEYPAD_STATUS_ERROR;
            Log(ERR, "L4_KeypadInit: KeypadTask Create Error - %d", Error);
            break;
        }

        /* Create semaphore for key press. Used to recieve keypress signal from ISR */
        pSemaKeyISR = OSSemCreate(0);
        if ( NULL == pSemaKeyISR )
        {
           	/* Something went wrong with Semaphore creation, return with error */
            Error = true;
            Log(ERR, "L4_KeypadInit: Create Semaphore Error");
            FaultHandlerSetFault(REQRST_MOO_SYSTEM_FAULT, SET_ERROR);
            break;
        }
        Signia_KeypadEventHandlerInit();

        Signia_StartShipModePatternwatch();        
        RegisterSoftResetKeyPatterns();

        /* Mark init done to avoid repeated initialization */
        KeypadInitDone = true;

        KeyscanPause = false;       
       

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   ScanDebounceNotifyKeyEvents 
 *
 * \details This function is triggered whenever there is a key press event.   
 *          This function calls subroutines for keypad scanning, debouncing,
 *          key event notification and key pattern detection.
 *          this function runs till all the keys are released(debounced) and no 
 *          keypattern detection is in progress. 
 *
 * \param   KeyProcState - pointer to current  keypad process state
 *
 * \return  None
 *
 * ========================================================================== */
static void ScanDebounceNotifyKeyEvents(KEYPAD_PROC_STATE *KeyProcState)
{   
    KEY_LOG KLog;
    uint16_t DebouncedKeys;        //debounce sample count 
    KEYPAD_STATUS PatternStatus;   //key pattern match status 
    static uint16_t KeyImage[DEBOUNCE_SAMPLE_COUNT];      
    
  
    //scan Keypad
    ScanKeypad(KeyImage);
    
    //debounce the keys for both press and release events
    DebouncedKeys = DebounceKeypad( &KLog, KeyImage );      
        
    //Notify any key state change         
    NotifyKeyStateChange( DebouncedKeys );             
    
    //check key pattern
    PatternStatus = CheckKeyPattern( DebouncedKeys );     
    
    if ( (KEY_STATE_RELEASE == KLog.KeyReleased )  && (KEYPAD_STATUS_OK == PatternStatus) )
    { //all keys released and no pattern detection in progress
        *KeyProcState = KEY_PROC_STATE_IDLE;// go back to semaphore pend state         
    }  
}

/* ========================================================================== */
/**
 * \brief   ScanKeypad
 *
 * \details This function is Scans the keypad GPIOs and updates the keyImage sample
 *          buffers.The scan happens every 10ms untill all keys are released and 
 *          no key pattern detection is in progress. the scanned keystates are saved
 *          sample buffers. The buffer index increments every scan and rolls over to  
 *          first buffer after max sample counts are saved.This way the oldest buffer
 *          gets overwritten. debouncing is done on required samples every 10ms.
 * \param   pKeyImage - pointer to key image buffer
 *
 * \return  None
 *
 * ========================================================================== */
static void ScanKeypad(uint16_t *pKeyImage)
{
    KEY_ID KeyID;
    bool ReadValue;               //key bit value
    static uint16_t Index = 0;    //sample index 
    
    // scan all key GPIOs
    for(KeyID = TOGGLE_DOWN; KeyID < KEY_COUNT; KeyID++ )
    {   //read GPIO state and update the corresponding key bit in key image
        if ( GPIO_STATUS_OK == L3_GpioCtrlGetSignal((GPIO_SIGNAL)KeypadSignal[KeyID], &ReadValue) )
        { 
            //key press detection has inverted logic, so !ReadValue indicates a key pressed 
            if(!ReadValue)
            {
                pKeyImage[Index] |= GET_KEY(KeyID);                
            }
            else
            {
                pKeyImage[Index] &= ~GET_KEY(KeyID);            
            }
        } 
    } 
    
    if ( ++Index >= DEBOUNCE_SAMPLE_COUNT )
    { //rollover to the first sampling KeyImage buffer
        Index = 0;
    }
}
/* ========================================================================== */
/**
 * \brief   DebounceKeypad
 *
 * \details Debouncing is done every 10ms with the required number of keyImage samples.
 *          Both press and release events are debounced.
 *          Debounce logic uses AND operation on all samples to find the key pressed.
 *          OR operation on all samples tells if any key is pressed in any of the samples.
 *          
 *          Note: For key release debounce
 *          all key samples should have the key bit cleared. Untill that happens the released
 *          bit is retained in the current debounced key state.
 *          This is done by the detecting the release key bit (XOR) and retained (AND/OR).
 *              
 * \param   pKeyImage - pointer to key image sample buffer
 *          pKLog - pointer to keylog 
 *
 * \return  Uuint16_t debounced keys state 
 *
 * ========================================================================== */
static uint16_t DebounceKeypad( KEY_LOG *pKLog, uint16_t *pKeyImage )
{    
    uint16_t SampleCount;
    static uint16_t PrevKeyPressed = 0; // Keys state 
    
    //copy the first keyImage to AND with other sample Key images
    pKLog->KeyPressed = *pKeyImage;
    pKLog->KeyReleased = *pKeyImage;
    
    for( SampleCount = 1; SampleCount < DEBOUNCE_SAMPLE_COUNT; SampleCount++)
    {
        //Key pressed if all the debounce samples have the key bit set
        pKLog->KeyPressed &= pKeyImage[SampleCount];  
        //Key is released if all the debounce samples have the key bit cleared
        pKLog->KeyReleased |= pKeyImage[SampleCount]; 
    }   
    // Find the actual release keys state.
    // AND operation above for KeyPressed will filter out any key press thats not present in all samples
    // The single line code below retains the filtered key bit till all samples have it cleared        
    PrevKeyPressed = pKLog->KeyPressed | ( ( pKLog->KeyPressed ^ PrevKeyPressed ) & pKLog->KeyReleased );
    
    return PrevKeyPressed;    
}
/* ========================================================================== */
/**
 * \brief   Check Next Pattern.
 *
 * \details CheckNextPattern gets the next key patterns in a key pattern
 *          sequence. If next key patttern is NULL then we have reached the end of 
 *          the sequence and pattern match is complete. this function also updates
 *          the pattern sequence timeout variable. 
 *
 * \param   index - registered key pattern undex 
 *          pKStatus -  pointer to the pattern detetction status
 *
 * \return  < None >
 * ========================================================================== */
static void CheckNextPattern( int index, KEYPAD_STATUS *pKStatus )
{
    int16_t Duration;
    
    RegisteredKeyPattern[index].KeySetNumber++; //point index to next key pattern in sequence                 
    RegisteredKeyPattern[index].pKeyPattern++;  //point to next key pattern in sequence
    RegisteredKeyPattern[index].MinStableDurationTimerFlag = false;//reset minimum key press stable duration timer flag 
    
    if (  RegisteredKeyPattern[index].pKeyPattern->KeySet == 0 )
    { // next pattern in sequence is NULL, key pattern sequence matching complete
        RegisteredKeyPattern[index].pKeyPattern -= RegisteredKeyPattern[index].KeySetNumber;
        RegisteredKeyPattern[index].KeySetNumber = 0;
        RegisteredKeyPattern[index].DetectTimeout = 0;
        
        Log(DBG, "Keypad: pattern match found..!");
        *pKStatus = KEYPAD_STATUS_MATCH_COMPLETE;
        
        RegisteredKeyPattern[index].phandler();
    }
    else
    {  // key pattern match found.. more patterns in the sequence expected        
        Log(DBG, "Keypad: Partial pattern matched..KEYPAD_STATUS_MATCH_IN_PROGRESS!");
        *pKStatus = KEYPAD_STATUS_MATCH_IN_PROGRESS;
        
        //load default max timout,         
        RegisteredKeyPattern[index].DetectTimeout = OSTimeGet() + DEFAULT_MAX_PATTERN_TIMEOUT;
        if ( RegisteredKeyPattern[index].pKeyPattern->DurationMax )
        { //load the registered max timeout duration if available
            RegisteredKeyPattern[index].DetectTimeout = OSTimeGet() + RegisteredKeyPattern[index].pKeyPattern->DurationMax;
        }
        
        Duration = RegisteredKeyPattern[index].pKeyPattern->DurationMin - RegisteredKeyPattern[index].pKeyPattern->DurationMax;
        if ( Duration > 0)
        {
            //Max timeout cannot be smaller than min stable duration
            //add difference of max and min to default timeout
            RegisteredKeyPattern[index].DetectTimeout += Duration;
        }
    }
}
/* ========================================================================== */
/**
 * \brief   Start Min Duration Key Timer.
 *
 * \details This timer keeps track of the minimum duration the key should be held
 *          for a valid key press
 * \param   index - registered key pattern index
 *
 * \return  <None>
 * ========================================================================== */
static void StartMinDurationKeyTimer( int index )
{
    RegisteredKeyPattern[index].MinStableDurationTimerFlag = true;
    RegisteredKeyPattern[index].ValidMinTime = OSTimeGet() + RegisteredKeyPattern[index].pKeyPattern->DurationMin;
}
/* ========================================================================== */
/**
 * \brief   CheckKeyPattern.
 *
 * \details CheckKeyPattern can identify individual key patterns or a sequence of
 *          key patterns. Each pattern is associated with a min and max duration.
 *          min duartion is the minimum amount of time a keypattern should be stable.
 *          max duration is the max delay before the next pattern in the sequence 
 *          should show up. min duration is applicable to both individual key pattern 
 *          and key pattern sequence detection. max duration is only applicable to 
 *          key pattern sequence detection. 
 *
 * \param   KeyState - debounced Key state 
 *
 * \return  KEYPAD_STATUS - pattern detection status
 * \retval  KEYPAD_STATUS_MATCH_INCOMPLETE - pattern sequence detection in progress 
 * \retval  KEYPAD_STATUS_OK - a key pattern detection over
 * \retval  KEYPAD_STATUS_MATCH_COMPLETE - keypattern match found
 * ========================================================================== */
static KEYPAD_STATUS CheckKeyPattern(uint16_t KeysState)
{   
    uint8_t index;
    KEYPAD_STATUS kStatus;
    
    kStatus = KEYPAD_STATUS_OK;

    // check if any of the key pattern sequence detection timed out
    for( index = 0; index< MAX_KEY_PATTERN_COUNT; index++)
    {
        if ( RegisteredKeyPattern[index].KeySetNumber > 0 )
        {
            if ( OSTimeGet() >= RegisteredKeyPattern[index].DetectTimeout )
            { //max timeout while waiting for next pattern in sequence
                Log(DBG, "KeypadTimerHandler: Key pattern idle Timeout");
                RegisteredKeyPattern[index].pKeyPattern -= RegisteredKeyPattern[index].KeySetNumber;                
                RegisteredKeyPattern[index].KeySetNumber = 0;
                RegisteredKeyPattern[index].DetectTimeout = 0;                 
                RegisteredKeyPattern[index].PreviousKeySet = 0;
            }        
            else
            { //pattern sequence detection in progress
                kStatus = KEYPAD_STATUS_MATCH_IN_PROGRESS;
                //Log(DBG, "Keypad: KEYPAD_STATUS_MATCH_IN_PROGRESS..");
            }
        }
    }
    
    for ( index = 0; index< MAX_KEY_PATTERN_COUNT; index++ )
    {  //loop through all registered patterns
        if ( RegisteredKeyPattern[index].pKeyPattern->KeySet == KeysState )
        { //key pattern match found
            
            if ( false ==  RegisteredKeyPattern[index].MinStableDurationTimerFlag )
            { //start the timer for minimum valid duration the key pattern should be stable               
              //  Log(DBG, "Keypad: MinStableDurationTimerFlag..started!");
                StartMinDurationKeyTimer(index);
                
            }            
            if ( OSTimeGet() >= RegisteredKeyPattern[index].ValidMinTime )
            {   //key pattern was stable for min valid duration     
                if( true == RegisteredKeyPattern[index].pKeyPattern->ActOnRelease )
                {// check on next pattern only after release of previos pattern 
                    RegisteredKeyPattern[index].PreviousKeySet = RegisteredKeyPattern[index].pKeyPattern->KeySet;
                }
                else
                {
                    CheckNextPattern( index, &kStatus);
                }                                
            }
        }
        else if ( RegisteredKeyPattern[index].KeySetNumber > 0 )
        { //pattern match in progress            
            kStatus = KEYPAD_STATUS_MATCH_IN_PROGRESS;
            
            if( RegisteredKeyPattern[index].MinStableDurationTimerFlag && !KeysState)
            { //key released before min stable duration for valid pattern
                RegisteredKeyPattern[index].MinStableDurationTimerFlag = false;
                kStatus = KEYPAD_STATUS_OK; //key match not in progress
            }           
        }              
        else if( RegisteredKeyPattern[index].MinStableDurationTimerFlag )
        {   // key pattern not registered or keys released, reset min stable duration check timer
          //  Log(DBG, "Keypad: MinStableDurationTimerFlag stopped..!");
            RegisteredKeyPattern[index].MinStableDurationTimerFlag = false;            
        }        
        if ( RegisteredKeyPattern[index].PreviousKeySet && !KeysState)
        {  //check if we can start looking for the next pattern in the keypattern sequence
           //keypattern held for min valid time 
           RegisteredKeyPattern[index].PreviousKeySet = 0;
           //we can start detecting new pattern in the sequence if the previous pattern is released                     
           CheckNextPattern( index, &kStatus );      
        }
    }

    return kStatus;
}

/* ========================================================================== */
/**
 * \brief   NotifyKeyStateChange
 *
 * \details Use this function to notify applications for keypad changes
 *
 * \param   KeyState - key state
 *
 * \return  None
 *
 * ========================================================================== */
static void NotifyKeyStateChange(uint16_t KeyState)
{
    static uint16_t prevKeyState;
    uint16_t KeyChanged;
    uint8_t index= 0;
    KEY_ID keyID = TOGGLE_DOWN;

    KeyChanged = KeyState ^ prevKeyState;

    do
    {
        if ( KeyChanged & (1 << keyID) )
        {
            do
            {
                if ( KeyHandler[index] != 0 )
                {
                    /* Notify any key change to applications through a registered handler */
                    KeyHandler[index](keyID, ((KEY_STATE)((KeyState & (1 << keyID))>0? \
                     KEY_STATE_PRESS:KEY_STATE_RELEASE)), KeyState);
                }

            } while (  ++index< KEY_COUNT );

            index = 0;
        }
    } while ( ++keyID < KEY_COUNT );

    prevKeyState = KeyState;
}

/* ========================================================================== */
/**
 * \brief   RegisterSoftResetKeyPatterns
 *
 * \details Registers the key sequences to be watched by Pattern watch to 
 *          detect soft reset
 *
 * \param   < None >
 *
 * \return  KEYPAD_STATUS
 *          KEYPAD_STATUS_ERROR : null pKeypattern as argument passed
 *          KEYPAD_STATUS_OK 	: Pattern registered
 *
 * ========================================================================== */
static KEYPAD_STATUS RegisterSoftResetKeyPatterns(void)
{
    KEYPAD_STATUS status = KEYPAD_STATUS_OK;
    uint8_t index;
    
    // register pattern watch
    for(index = 0 ; index < KEY_SOFTRESET_PATTERN_TYPE_COUNT; index++)
    {
        status = L4_KeypadWatchPattern(SoftResetPatternTable[index], &SoftResetHandler);
        if(KEYPAD_STATUS_OK != status)
        {
            break;
        }
    }
  
    return status;
}
/* ========================================================================== */
/**
 * \fn      void KeypadGPIOInit (void)
 *
 * \brief   Initialize Keypad GPIOs
 *
 * \details Initializes Keypad GPIOs as inputs and configures interrupt on wake key
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void KeypadGPIOInit(void)
{
    GPIO_uP_PIN_INT_CONFIG_T GpioKeyWakeIntConfig; /*< Used to register the Wake Key ISR callback. */

    /* Registering the  callback to handle the interrupt from Wake Key */
    GpioKeyWakeIntConfig.pxInterruptCallback = &KeypadWakeup_IntCallback;
    /* Once a key press is detected, keys are polled till all goes back to idle*/
    GpioKeyWakeIntConfig.eInterruptType = GPIO_uP_INT_FALLING_EDGE;

    /* Configure GPIO_KEY_WAKEn for Interrupt. "schematic ref. KEY_WAKEn" */
    L3_GpioCtrlEnableCallBack(GPIO_KEY_WAKEn, &GpioKeyWakeIntConfig);
}

/* ========================================================================== */
/**
 * \brief   KeypadWakeup Callback function for GPIO_KEY_WAKEn interrupt.
 *
 * \details This callback handler is triggered when there is any key press.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ==========================================================================*/
static void KeypadWakeup_IntCallback(void)
{
  OSSemPost(pSemaKeyISR);   /* Signal waiting Keypad scan function. */
}

/* ========================================================================== */
/**
 * \brief    Handler for ShipMode key sequence
 *
 * \details 	This function handles the shipmode key pattern sequence.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void Key_ShipModeHandler(void)
{
    Log(DBG, "Keypad: Received ShipMode key sequence");
    Signia_ShipModeReqEvent(SIGNIA_SHIPMODE_VIA_KEYPAD);  
}

/* ========================================================================== */
/**
 * \brief   Handler for Rotation Config Mode Keys
 *
 * \details Rotation Configuration Handler
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void Key_RotationConfigHandler(void)
{
    Signia_RotationConfigReqEvent();
}

/* ========================================================================== */
/**
 * \brief   Soft Reset Key sequence handler function
 *
 * \details Perform the Soft Reset If the Soft Reset combination keys
 *          Pressed simultaneously for 4-Sec.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void SoftResetHandler(void)
{    
    Log(DBG, "Soft Reset key sequence Done!!!");
    L3_OneWireEnable(false);
    L3_DisplayOn(false);
    OSTimeDly(DELAY_BEFORE_RESET);
    SoftReset();
}

/******************************************************************************/
/*                             Global Function(s)                              */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   L4_KeypadHandlerSetup
 *
 * \details Use this function to register for keypad event notifications
 *
 * \param   pHandler - keypad event handler function. Registering NULL disables key change notification
 *
 * \return  KEYPAD_STATUS - key status
 *
 * ========================================================================== */
KEYPAD_STATUS L4_KeypadHandlerSetup(KEYPAD_HANDLER pHandler)
{
    KEYPAD_STATUS kStatus = KEYPAD_STATUS_ERROR;

    for ( uint8_t index= 0; index < KEY_COUNT; index++ )
    {
        if ( KeyHandler[index] == NULL && pHandler != NULL )
        {
            KeyHandler[index] = pHandler;
            kStatus = KEYPAD_STATUS_OK;
            break;
        }
    }

    return kStatus;
}
/* ========================================================================== */
/**
 * \brief   L4_KeypadGetKeyState().
 *
 * \details get key states for key ids passed as parameter
 *
 * \param   Key - Key ID
 *
 * \return  KEY_STATE - key state
 * \retval  KEY_STATE_PRESS
 * \retval  KEY_STATE_RELEASE
 * \retval  KEY_STATE_STUCK
 *          Note: Invalid Key, if specified would return idle state ‘KEY_STATE_RELEASE’
 *
 * ========================================================================== */
KEY_STATE L4_KeypadGetKeyState(KEY_ID Key)
{
    KEY_STATE kState = KEY_STATE_RELEASE;
    bool ReadValue;

    if ( GPIO_STATUS_OK == L3_GpioCtrlGetSignal((GPIO_SIGNAL)KeypadSignal[Key], &ReadValue) )
    {
        if ( !ReadValue )
        {
            kState = KEY_STATE_PRESS;
        }
    }

    return kState;
}

/* ========================================================================== */
/**
 * \brief   L4_KeypadWatchPattern
 *
 * \details Register pattern to be observed
 *
 * \param   pKeyPattern - pointer to Null terminated array of patterns
 * \param   pPatternHandler - Pattern notification handler
 *
 * \return  KEYPAD_STATUS
 * \retval  KEYPAD_STATUS_ERROR - null pKeypattern as argument passed
 * \retval  KEYPAD_STATUS_OK - Pattern registered
 *
 * ========================================================================== */
KEYPAD_STATUS L4_KeypadWatchPattern (KEY_PATTERN const *pKeyPattern, KEYPAD_PATTERN_HANDLER pPatternHandler)
{
    uint8_t index = 0;
    KEYPAD_STATUS kStatus = KEYPAD_STATUS_ERROR;

    do
    {
        if ( RegisteredKeyPattern[index].pKeyPattern == NULL && pKeyPattern != NULL )
        {
            RegisteredKeyPattern[index].KeySetNumber = 0;
            RegisteredKeyPattern[index].pKeyPattern = pKeyPattern;
            RegisteredKeyPattern[index].phandler = pPatternHandler;
            RegisteredKeyPattern[index].MinStableDurationTimerFlag = false;
            kStatus = KEYPAD_STATUS_OK;

            break;
        }

    } while ( ++index < MAX_KEY_PATTERN_COUNT);

    return kStatus;
}

/* ========================================================================== */
/**
 * \brief   Unregister a pattern detection.
 *
 * \details Register pattern to be observed
 *
 * \param   pKeyPattern: pointer to Already registered key pattern to be stopped from detection.
 *          If NULL, all pattern watch are removed.
 *
 * \return  KEYPAD_STATUS
 * \retval  KEYPAD_STATUS_ERROR - pKeypattern not found
 * \retval  KEYPAD_STATUS_OK -  Pattern unregistered
 *
 * ========================================================================== */
KEYPAD_STATUS L4_KeypadUnwatchPattern (KEY_PATTERN const *pKeyPattern)
{
    uint8_t index = 0;
    KEYPAD_STATUS kStatus = KEYPAD_STATUS_ERROR;

    do
    {
        if ( (RegisteredKeyPattern[index].pKeyPattern == pKeyPattern) || (pKeyPattern == NULL) )
        {
            RegisteredKeyPattern[index].pKeyPattern = NULL;
            RegisteredKeyPattern[index].phandler = NULL;

            kStatus = KEYPAD_STATUS_OK;

            break;
        }
    } while ( ++index < MAX_KEY_PATTERN_COUNT );

    return kStatus;
}


/* ========================================================================== */
/**
 * \brief   Signia_StartRotationConfigPatternWatch
 *
 * \details Registers the key sequences to be watched by Pattern watch to 
 *          detect Rotation Config key sequence
 *
 * \param   < None >
 *
 * \return  KEYPAD_STATUS
 *          KEYPAD_STATUS_ERROR : null pKeypattern as argument passed
 *          KEYPAD_STATUS_OK 	: Pattern registered
 *
 * ========================================================================== */
KEYPAD_STATUS Signia_StartRotationConfigPatternWatch(void)
{
    KEYPAD_STATUS status = KEYPAD_STATUS_OK;
    uint8_t index;
    do
    {
        /*register pattern watch*/
        for(index = 0 ; index < KEY_ROTATIONCONFIG_COUNT; index++)
        {
            status = L4_KeypadWatchPattern(RotationConfigPatternTable[index], &Key_RotationConfigHandler);
            if(KEYPAD_STATUS_OK != status)
            {
                break;
            }
        }
    } while (false);
    return status;
}

/* ========================================================================== */
/**
 * \brief   L4_StopRotationConfigPatternWatch
 *
 * \details UnRegisgers the key sequences to be watched by Pattern watch to 
 *          detect Rotation Key sequence
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_StopRotationConfigPatternWatch(void)
{
    KEYPAD_STATUS status = KEYPAD_STATUS_OK;
    uint8_t index;
    do
    {
        /*register pattern watch*/
        for(index = 0 ; index < KEY_ROTATIONCONFIG_COUNT; index++)
        {
            /*register pattern watch*/
            status = L4_KeypadUnwatchPattern(RotationConfigPatternTable[index]);
            if(KEYPAD_STATUS_OK != status)
            {
                break;
            }
        }
    } while (false);
}

/* ========================================================================== */
/**
 * \brief   L4_StartShipModePatternWatch
 *
 * \details Regisgers the key sequences to be watched by Pattern watch to 
 *          detect ShipMode key sequence
 *
 * \param   < None >
 *
 * \return  KEYPAD_STATUS
 *          KEYPAD_STATUS_ERROR : null pKeypattern as argument passed
 *          KEYPAD_STATUS_OK 	: Pattern registered
 *
 * ========================================================================== */
KEYPAD_STATUS Signia_StartShipModePatternwatch(void)
{
    KEYPAD_STATUS status = KEYPAD_STATUS_OK;
    uint8_t index;
    do
    {
        /*register pattern watch*/
        for(index=0 ; index < KEY_SHIPMODE_SEQ_COUNT; index++)
        {
            status = L4_KeypadWatchPattern(shipmodepatternTable[index], &Key_ShipModeHandler);
            if(KEYPAD_STATUS_OK != status)
            {
                break;
            }
        }
    } while (false);
    return status;
}

/* ========================================================================== */
/**
 * \brief   L4_StopShipModePatternWatch
 *
 * \details UnRegisgers the key sequences to be watched by Pattern watch to 
 *          detect ShipMode key sequence
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void Signia_StopShipModePatternwatch(void)
{
    KEYPAD_STATUS status = KEYPAD_STATUS_OK;
    uint8_t index;
    do
    {
        /*register pattern watch*/
        for(index=0 ; index < KEY_SHIPMODE_SEQ_COUNT; index++)
        {
            /*register pattern watch*/
            status = L4_KeypadUnwatchPattern(shipmodepatternTable[index]);
            if(KEYPAD_STATUS_OK != status)
            {
                break;
            }
        }
    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Keypad Scan Pause
 *
 * \details Keypad scan is paused untill resume, no key events shall be generated.
 *
 * \param  < None >
 *
 * \return  None
 *
 * ==========================================================================*/
void Signia_KeypadScanPause(void)
{
    KeyscanPause = true;
}

/* ========================================================================== */
/**
 * \brief   Keypad Scan Resume
 *
 * \details Keypad scan is Resumed, key events shall be generated.
 *
 * \param  < None >
 *
 * \return  None
 *
 * ==========================================================================*/
void Signia_KeypadScanResume(void)
{
    KeyscanPause = false;
}

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
