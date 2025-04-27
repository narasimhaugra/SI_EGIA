#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ============================================================================================== */
/** \addtogroup BackgroundDiagTask
 * \{
 * \brief   This module validates Internal, External SRAM and Program Code flash
 *
 * \details Test External, Internal SRAM & Flash code memory.
 *
 * \copyright 2019 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file BackgroundDiagTask.c
 *
 * Notes: New memory section "ramdyndata" is created
 *        All the DMA rx buffers, Background diag task stack are moved to memory section "ramdyndata"
 *        Internal RAM start address will depend on "ramdyndata"  end address. Refer 1MB_Pflash.icf file
 *
 * ============================================================================================== */

/**************************************************************************************************/
/*                                         Includes                                               */
/**************************************************************************************************/
#include <common.h>
#include "Logger.h"
#include "L4_BlobHandler.h"
#include "Crc.h"
#include "BackgroundDiagTask.h"
#include "FaultHandler.h"
#include "L2_OnchipRtc.h"

/**************************************************************************************************/
/*                                         Global Constant Definitions(s)                         */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                         Local Define(s) (Macros)                               */
/**************************************************************************************************/
#define     LOG_GROUP_IDENTIFIER             (LOG_GROUP_TESTS)      ///< Log Group Identifier
#define     BACKGROUNDDIAG_TASK_PERIOD       (MSEC_100)             ///< Task period every 100 milli sec
#define     SRAMPATTERN                      (0xAAAAAAAAu)          ///< Pattern
#define     SRAMANTIPATTERN                  (0x55555555u)          ///< Anti-Pattern
#define     MEMORY_TEST_INT_RAM_SIZE         (256u)                 ///< Memory chunk size in bytes
#define     MEMORY_TEST_EXT_RAM_SIZE         (64u)                  ///< Memory chunk size in bytes
#define     BACKGROUNDTASK_STACK_SIZE        (512u)                 ///< Stack size of Background Diagnostic task
#define     RAM_VALIDATIONTIME               (MIN_10)               ///< RAM memory test validation time
#define     FLASHCODE_VALIDATIONTIME         (MIN_60)               ///< Flash Validation Time
#define     MEMORY_FENCE_NAME_LENGTH         (100u)                 ///< Memory Fence Name Length
#define     MEMORY_FENCE_PATTERN             (0xA5)                 ///< Memory Fence Pattern for Byte
   
/**************************************************************************************************/
/*                                         Global Variable Definitions(s)                         */
/**************************************************************************************************/
#pragma location=".ramdyndata"
static OS_STK BackgroundDiagTaskStack[BACKGROUNDTASK_STACK_SIZE + MEMORY_FENCE_SIZE_DWORDS];  ///< Background Diag Thread stack
extern OS_STK OSTaskIdleStk[];             /* Idle task stack                */
extern OS_STK OSTaskStatStk[];             /* Statistics task stack          */
extern OS_STK OS_CPU_ExceptStk[];          /* CPU Exception Stack            */
extern OS_STK OSTmrTaskStk[];              /* OS Timer Task                  */

extern OS_STK FpgaControllerTaskStack[];   /* Stack for fpga controller task */
extern OS_STK TaskMonitorStack[];          /* Stack for Task Monitor Task    */
extern OS_STK StartupTaskTask[];           /* Stack for Startup              */
extern OS_STK OWTaskStack[];          	   /* 1-Wire task stack              */
extern OS_STK KeypadTaskStack[];           /* Keypad Task Stack              */
extern OS_STK SoundMgrStack[];             /* Stack for SoundManager         */
extern OS_STK AdapterMngrTaskStack[];      /* Adapter Manager Task Stack     */
extern OS_STK HandleStack[];               /* Handle Stack for AO            */
extern OS_STK DmTaskStack[];               /* Display Manager task stack     */
extern OS_STK ConsoleMgrTaskStack[];       /* Communication Manager task stack*/
extern OS_STK ChgMgrTaskStack[];           /* Charger Manager task stack     */
extern OS_STK AccelTaskStack[];            /* Stack for accelrometer task    */
extern OS_STK CleanupTaskStack[];          /* CleanUp Task Stack             */
extern OS_STK LoggerStack[];               /* Logger Stack for AO            */
extern OS_STK TMStack[];                   /* Stack for TestManager AO       */

/* Buffer Reference*/
extern uint8_t EventMsgBuf1[];
extern uint8_t EventMsgBuf2[];
extern uint8_t mpSDHCSDCardBuffer[];
extern uint8_t FpgaTxBuffer[];                         /* SPI buffer for transmit to FPGA   */
extern uint8_t FpgaRxBuffer[];                         /* SPI Buffer for Receive to FPGA    */
extern MOTOR_CTRL_PARAM MotorCtrlParam[MOTOR_COUNT];   /* Motor control data for each motor */
extern uint8_t oneWireTempData[];                      /* Memory bank to store 1-wire temporary data    */
extern uint8_t slaveMAC[];                             /* Memory bank to store 1-wire Slave MAC data    */
extern uint8_t slaveEEPROMPage[];                      /* Memory bank to store 1-wire Slave EEPROM data */
extern uint8_t challengeData[];                        /* Memory bank to store 1-wire Challenge data to be used for MAC computation */
extern uint8_t masterMAC[];                            /* Memory bank to store 1-wire Master MAC data   */
extern ADAPTER_RESPONSE PartialResponse;                /* Adapter Uart Partial Response Buffer */
extern uint8_t  adapterOutgoingData[];                 /* Adapter Tx Buffer */
extern PARTIALDATA PartData;                           /* Console Manager Partial Data Buffer */
extern uint8_t  UsbRxDataBuffer[];                     /* Usb Rx data buffer */
extern uint8_t  UsbTxDataBuffer[];                     /* Usb Tx data buffer */
extern uint8_t  WlanRxDataBuffer[];                    /* Wlan Rx Data Buffer*/
extern uint8_t  WlanTxDataBuffer[];                    /* Tx data buffer     */
extern uint8_t  Uart0RxDataBuffer[];                   /* Rx data buffer     */
extern uint8_t  Uart0TxDataBuffer[];                   /* Tx data buffer     */
extern CPU_INT08U   Mem_Heap[];                        /* Micrium Mem heap.  */
extern RDF_OBJECT RdfObject[];                         /* Storage for RDF objects */
/**************************************************************************************************/
/*                                         Local Type Definition(s)                               */
/**************************************************************************************************/
typedef struct                                 ///< Struct to hold RAM info
{
    uint32_t RamStartAddress;                  ///< RAM start address
    uint32_t RamEndAddress;                    ///< RAM End address
    uint32_t RamSize;                          ///< RAM size to test
    uint32_t MemTested;                        ///< Memory Tested
    bool     IsEntireRamTested;
} BackgroundRamInfo;

typedef struct                                 ///< Struct to hold Timers Info
{
    int32_t RamTestTimer;                      ///< Time instance to evaluate RAM 10 min timer
    int32_t FlashTestTimer;                    ///< Time instance to evaluate FLASH 60 min timer
    bool ValidateRamMem;                       ///< Flag to set RAM meomory validation
    bool ValidateFlash;
} BackgroundTimers;

typedef struct                                                       ///< Memory Fence Definition
{
    int8_t *const   MemoryFencePtr;                                  ///< Pointer to Memory Fence array
    const uint8_t MemoryFenceErrorString[MEMORY_FENCE_NAME_LENGTH];  ///< Memory Fence Error String
} BackgroundMemoryFenceDetails;

typedef enum                                  ///< Background Task Status
{
   BACKGROUND_TASKSTATUS_RUN,                 ///< Background Thread is validating Memory
   BACKGROUND_TASKSTATUS_STOP,                ///< Background Thread has Stopped Validating Memory
} BACKGROUND_TASKSTATUS;

/**************************************************************************************************/
/*                                         Local Constant Definition(s)                           */
/**************************************************************************************************/
const static BackgroundMemoryFenceDetails BackgroundMemoryFenceArray [] =
{
    // Stacks grow from high memory to low memory, so put our stack fences at the start of each stack (low memory).
    /* Stack of threads created by uCOS-II  */
    {(int8_t *)&OSTaskIdleStk[0]                             ,"OS Idle Task       Stack Overflow"},
    {(int8_t *)&OSTaskStatStk[0]                             ,"OS Statistic Task  Stack Overflow"},
    {(int8_t *)&OS_CPU_ExceptStk[0]                          ,"OS CPU Exception   Stack Overflow"},
    {(int8_t *)&OSTmrTaskStk[0]                              ,"OS Timer Task      Stack Overflow"},
    /* Stacks of threads created from Singia and following the order as per priority given in TaskPriority.h */
    {(int8_t *)&FpgaControllerTaskStack[0]                   ,"FpgaControllerTask Stack Overflow"},
    {(int8_t *)&TaskMonitorStack[0]                          ,"TaskMonitor        Stack Overflow"},
    {(int8_t *)&StartupTaskTask[0]                           ,"StartupTask        Stack Overflow"},
    {(int8_t *)&OWTaskStack[0]                               ,"OwTask             Stack Overflow"},
    {(int8_t *)&KeypadTaskStack[0]                           ,"KeypadTask         Stack Overflow"},
    {(int8_t *)&SoundMgrStack[0]                             ,"SoundMgrTask       Stack Overflow"},
    {(int8_t *)&AdapterMngrTaskStack[0]                      ,"AdapterMgrTask     Stack Overflow"},
    {(int8_t *)&HandleStack[0]                               ,"Handle App         Stack Overflow"},
    {(int8_t *)&DmTaskStack[0]                               ,"DisplayManager     Stack Overflow"},
    {(int8_t *)&ConsoleMgrTaskStack[0]                       ,"ConsoleMgrTask     Stack Overflow"},
    {(int8_t *)&ChgMgrTaskStack[0]                           ,"ChargerManagerTask Stack Overflow"},
    {(int8_t *)&AccelTaskStack[0]                            ,"AccelTask          Stack Overflow"},
    {(int8_t *)&CleanupTaskStack[0]                          ,"CleanUp Task       Stack Overflow"},
    {(int8_t *)&LoggerStack[0]                               ,"Logger             Stack Overflow"},
    {(int8_t *)&TMStack[0]                                   ,"Test Manager       Stack Overflow"},
    {(int8_t *)&BackgroundDiagTaskStack[0]                   ,"BackgroundDiagTask Stack Overflow"}, 
    /* Normal buffers grow from low memory to high memory, so put their fences at the end of their areas.  */
    {(int8_t *)&EventMsgBuf1[EVENT_MSG_BUF1_TOTAL_SIZE]      , "EventMsgBuf1 Buffer"             },  /* AO Active Object*/
    {(int8_t *)&EventMsgBuf2[EVENT_MSG_BUF2_TOTAL_SIZE]      , "EventMsgBuf2 Buffer"             },  /* AO Active Object*/
    {(int8_t *)&mpSDHCSDCardBuffer[SDHC_BUFFER_SIZE]         , "mpSDHCSDCardBuffer"              },  /* L2/Board/fs_dev_sd_card_bsp */
    {(int8_t *)&FpgaTxBuffer[FPGA_BUFFER_MAX]                , "FpgaTxBuffer"                    },  /* L3/Fpga                     */
    {(int8_t *)&FpgaRxBuffer[FPGA_BUFFER_MAX]                , "FpgaRxBuffer"                    },  /* L3/Fpga                     */
    {(int8_t *)&MotorCtrlParam[MOTOR_ID0].MemoryFence        , "MotorCtrlParam[MOTOR_ID0]"       },  /* L3/Motor                    */
    {(int8_t *)&MotorCtrlParam[MOTOR_ID1].MemoryFence        , "MotorCtrlParam[MOTOR_ID1]"       },  /* L3/Motor                    */
    {(int8_t *)&MotorCtrlParam[MOTOR_ID2].MemoryFence        , "MotorCtrlParam[MOTOR_ID2]"       },  /* L3/Motor                    */    
    {(int8_t *)&oneWireTempData[ONEWIRE_MEMORY_TEMPDATA_SIZE], "oneWireTempData"                 },  /* L3/OneWire                  */
    {(int8_t *)&slaveMAC[ONEWIRE_MEMORY_BANK_SIZE]           , "slaveMAC"                        },  /* L3/OneWire                  */
    {(int8_t *)&slaveEEPROMPage[ONEWIRE_MEMORY_BANK_SIZE]    , "slaveEEPROMPage"                 },  /* L3/OneWire                  */
    {(int8_t *)&challengeData[ONEWIRE_MEMORY_BANK_SIZE]      , "challengeData"                   },  /* L3/OneWire                  */
    {(int8_t *)&masterMAC[ONEWIRE_MEMORY_BANK_SIZE]          , "masterMAC"                       },  /* L3/OneWire                  */ 
    {(int8_t *)&PartialResponse.Buffer[ADAPTER_RX_BUFF_SIZE] , "PartialResponse.Buffer"          },  /* L4/AdapterDefn              */
    {(int8_t *)&adapterOutgoingData[ADAPTER_TX_BUFF_SIZE]    , "adapterOutgoingData"             },  /* L4/AdapterDefn              */
    {(int8_t *)&PartData.Data[PARTIAL_DATA_BUFF_SIZE]        , "PartData.Data"                   },  /* L4/ConsoleManager           */
    {(int8_t *)&UsbRxDataBuffer[MAX_DATA_BYTES]              , "UsbRxDataBuffer"                 },  /* L4/Comm Manager             */
    {(int8_t *)&UsbTxDataBuffer[MAX_DATA_BYTES]              , "UsbTxDataBuffer"                 },  /* L4/Comm Manager             */
    {(int8_t *)&WlanRxDataBuffer[MAX_DATA_BYTES]             , "WlanRxDataBuffer"                },  /* L4/Comm Manager             */
    {(int8_t *)&WlanTxDataBuffer[MAX_DATA_BYTES]             , "WlanTxDataBuffer"                },  /* L4/Comm Manager             */
    {(int8_t *)&Uart0RxDataBuffer[MAX_DATA_BYTES]            , "Uart0RxDataBuffer"               },  /* L4/Comm Manager             */
    {(int8_t *)&Uart0TxDataBuffer[MAX_DATA_BYTES]            , "Uart0TxDataBuffer"               },  /* L4/Comm Manager             */
    {(int8_t *)&RdfObject[MOTOR_ID0].FileBuf[RDF_FILEBUF_SIZE],"RdfObject[0]"                    },  /*Utils/Rdf                    */
    {(int8_t *)&RdfObject[MOTOR_ID1].FileBuf[RDF_FILEBUF_SIZE],"RdfObject[1]"                    },  /*Utils/Rdf                    */
    {(int8_t *)&RdfObject[MOTOR_ID2].FileBuf[RDF_FILEBUF_SIZE],"RdfObject[2]"                    },  /*Utils/Rdf                    */       
    {(int8_t *)&Mem_Heap[LIB_MEM_CFG_HEAP_SIZE]               ,"Mem_Heap"                        },  /*Micrium/uCLib/lib_mem        */
};

#define MEMORY_FENCE_COUNT  (sizeof(BackgroundMemoryFenceArray)  / sizeof(BackgroundMemoryFenceDetails))

static int8_t MemoryFencePattern[MEMORY_FENCE_SIZE_BYTES];  /* Array of Memory Fence Pattern */

/**************************************************************************************************/
/*                                         Local Variable Definition(s)                           */
/**************************************************************************************************/
static BackgroundTimers   ValidationTimers;
static BackgroundRamInfo  ExternalRamInfo;           ///< External RAM Information
static BackgroundRamInfo  InternalRamInfo;           ///< Internal RAM Information

/**************************************************************************************************/
/*                                         Local Function Prototype(s)                            */
/**************************************************************************************************/
static void BackgroundDiagTask(void *p_arg);
static BACKGROUND_STATUS ValidateInternalRAM(void);
static BACKGROUND_STATUS ValidateExternalRAM (void);
static BACKGROUND_STATUS ValidateRam(BackgroundRamInfo *pRaminfo, uint32_t *pFaultyAddr, uint32_t Memorysize);
static FLASH_CRCVALIDATION_STATUS ValidateFlashArea(void);
static void BackgroundInitRamFlashInfo(void);
static void BackgroundTaskValidateEntireRAM(BACKGROUND_STATUS *pIntRamStatus, BACKGROUND_STATUS *pExtRamStatus);
static void BackgroundMemoryFenceInit(void);
static BACKGROUND_STATUS BackgroundTaskMemoryFenceTest(void);

/**************************************************************************************************/
/*                                         Local Function(s)                                      */
/**************************************************************************************************/
/* ========================================================================== */
/**
 * \brief    Background Diagnostic Task.
 *
 * \details  Background Diagnostic task periodically checks Internal, External RAM Integrity, Program flash memory Integrity validation and Memory Fence Test check
 *
 * \note     When Program Flash Integrity (or) RAM Integrity (or) Memory Fence Error occured, s/w will Publish Request Reset 
 *           with Cause and STOP performing Memory Tests.
 *
 * \param   <None>
 *
 * \return  <None>
 *
 * ========================================================================== */
static void BackgroundDiagTask(void *p_arg)
{
   FLASH_CRCVALIDATION_STATUS    FlashStatus;
   BACKGROUND_STATUS             InternalRamStatus;
   BACKGROUND_STATUS             ExternalRamStatus;
   BACKGROUND_STATUS             BackgroundMemoryFenceStatus;
   BACKGROUND_TASKSTATUS         TaskStatus;
   ERROR_CAUSE                   ErrorCause;
   
   TaskStatus        = BACKGROUND_TASKSTATUS_RUN;
   InternalRamStatus = BACKGROUND_STATUS_OK;
   ExternalRamStatus = BACKGROUND_STATUS_OK;
   ErrorCause        = NO_ERROR_CAUSE;       
   /* Initialize time instance to run 60 min timer for FLASH test */
   ValidationTimers.FlashTestTimer = OSTimeGet();
   /* Initialize time instance to run 10min timer for RAM memory test */
   ValidationTimers.RamTestTimer   = OSTimeGet();
   /* Initialize RAM struct of Internal and External RAM */
   BackgroundInitRamFlashInfo();

   while (true)
   {
      if(BACKGROUND_TASKSTATUS_RUN == TaskStatus)
      {
          BackgroundTaskValidateEntireRAM(&InternalRamStatus,&ExternalRamStatus);
          /* Validate Flash area1 */
          FlashStatus = ValidateFlashArea();
          /* Memory Fence Test */
          BackgroundMemoryFenceStatus = BackgroundTaskMemoryFenceTest();
          
          /* Check For Internal or External RAM Integrity Failure */
          if ( BACKGROUND_STATUS_ERROR == BackgroundMemoryFenceStatus )
          {
              /* Set Cause to MEMORY FENCE Error */
              ErrorCause = REQRST_MEMORYFENCE_ERROR;
          }
          else if ( (BACKGROUND_STATUS_ERROR == InternalRamStatus) || (BACKGROUND_STATUS_ERROR == ExternalRamStatus) )
          {
              /* Error Cause is RAM Integrity Failure */
              ErrorCause = REQRST_RAM_INTEGRITYFAIL;
          }
          else if ( FLASH_CRC_VALIDATED_BAD == FlashStatus )
          {
              /* Error Cause Program Flash Integrity Failure */
              ErrorCause = REQRST_PROGRAMCODE_INTEGRITYFAIL; 
          }
          else
          {
             /* Do nothing */
          }
          /* Check if any memory area is faulty*/
          if( NO_ERROR_CAUSE != ErrorCause )
          {
              /* Memory failure, do not perform any memory validation */
              TaskStatus = BACKGROUND_TASKSTATUS_STOP;
              /* Clear timers */
              ValidationTimers.ValidateFlash = false;
              ValidationTimers.ValidateRamMem = false;
              /* Clear Internal, External and Flash Test Info */
              BackgroundInitRamFlashInfo();
              /* Publish Error */
              FaultHandlerSetFault(ErrorCause, SET_ERROR );
          }
      }
      /* Sleep for 100 ms */
      OSTimeDly(BACKGROUNDDIAG_TASK_PERIOD);
   }
}

/* ========================================================================== */
/**
 * \brief    Background Diagnostic Interal, External Info Initialization
 *
 * \details  Intializes Start address, end addres, memory size to test, Ram tested started status, Total memory size tested
 *
 *
 * \param   <None>
 *
 * \return  <None>
 *
 * ========================================================================== */
static void BackgroundInitRamFlashInfo(void)
{
   extern uint32_t __INTRAM_start__;
   extern uint32_t __EXTRAM_start__;
   extern uint32_t __EXTRAM_end__;
   extern uint32_t __INTRAM_END__;

   /* Initialize to Internal RAM memory test starting address */
   InternalRamInfo.RamStartAddress = (uint32_t)&__INTRAM_start__;
   /* Initialize to Internal RAM memory END address */
   InternalRamInfo.RamEndAddress   = (uint32_t)&__INTRAM_END__;
   /* Total RAM size to test */
   InternalRamInfo.RamSize         = (uint32_t)&__INTRAM_END__ - (uint32_t)&__INTRAM_start__;
   /* Total memory tested */
   InternalRamInfo.MemTested       = 0;
   /* Entire Ram Tested or not */
   InternalRamInfo.IsEntireRamTested  = false;

   /* Initialize to External RAM memory test starting address */
   ExternalRamInfo.RamStartAddress = (uint32_t)&__EXTRAM_start__;
   /* Initialize to External RAM memory END address */
   ExternalRamInfo.RamEndAddress   = (uint32_t)&__EXTRAM_end__;
   /* Total External RAM size to test */
   ExternalRamInfo.RamSize         = (uint32_t)&__EXTRAM_end__ - (uint32_t)&__EXTRAM_start__;
   /* Total memory tested */
   ExternalRamInfo.MemTested       = 0;
   /* Entire Ram Tested or not */
   ExternalRamInfo.IsEntireRamTested  = false;

/// \todo Clear crc handle info
}

/* ========================================================================== */
/**
 * \brief    Validates Internal, External RAM
 *
 * \details  Perform memory test every 10min. Perform Internal RAM test followed by External RAM test
 *
 * \param   intRamStatus : Internal RAM test status
 *          extRamStatus : External RAM test status
 *
 * \return  None
 *
 * ========================================================================== */
static void BackgroundTaskValidateEntireRAM(BACKGROUND_STATUS *pIntRamStatus, BACKGROUND_STATUS *pExtRamStatus)
{
   /* Is RAM validation timer expired */
   if((OSTimeGet() - ValidationTimers.RamTestTimer) >= RAM_VALIDATIONTIME)
   {
      /* Restart 10 min timer */
      ValidationTimers.RamTestTimer = OSTimeGet();
      /* Enable RAM memory test */
      ValidationTimers.ValidateRamMem = true;
      Log(DBG, "Internal & External RAM Validation Starts: Integrity Check  ");
   }
   /* Is RAM memory test enabled */
   if( ValidationTimers.ValidateRamMem )
   {
      /* Test Internal RAM first */
      if( !InternalRamInfo.IsEntireRamTested )
      {
         /* Validate Internal RAM */
         *pIntRamStatus = ValidateInternalRAM();
      }
      else if( !ExternalRamInfo.IsEntireRamTested )  /* Continue testing External RAM */
      {
         /* Validate External RAM */
         *pExtRamStatus = ValidateExternalRAM();
      }
      else
      {
          /* Do nothing */
      }
      /* Validation is completed */
      if( InternalRamInfo.IsEntireRamTested && ExternalRamInfo.IsEntireRamTested )
      {
         /* Reset 10 min validation timer running status */
         ValidationTimers.ValidateRamMem = false;
         /* Reset Internal RAM Tested */
         InternalRamInfo.IsEntireRamTested = false;
         /* Reset External RAM Tested */
         ExternalRamInfo.IsEntireRamTested = false;
      }
   }
}

/* ========================================================================== */
/**
 * \brief    Validates Internal RAM
 *
 * \details  Perform memory test by writing and reading pattern (0xAA) and similarly anti-pattern (0x55)
 *           Read's chunk of Internal RAM to local temp.
 *           Entire RAM test is performed by reading RAM memory in chunks
 *
 * \param   void
 *
 * \return  Status of Internal RAM Test
 *
 * ========================================================================== */
static BACKGROUND_STATUS ValidateInternalRAM(void)
{
   BACKGROUND_STATUS Status;
   uint32_t FaultyAddr;
   /* Update status as valid */
   Status = BACKGROUND_STATUS_OK;

   Status = ValidateRam(&InternalRamInfo,&FaultyAddr,MEMORY_TEST_INT_RAM_SIZE);

   if(BACKGROUND_STATUS_ERROR == Status)
   {
      Log(ERR,"Internal RAM memory Test Failed at address: %X",FaultyAddr);
   }
   else if(BACKGROUND_STATUS_INVALIDPARAM == Status)
   {
     Log(ERR,"Internal RAM memory Test Invalid parameter,Starting Address: %X,End address: %X, Memory size tested: %d",
         InternalRamInfo.RamStartAddress, InternalRamInfo.RamEndAddress, InternalRamInfo.MemTested);
   }
   else
   {
     /* Do nothing */
   }

   /* return status */
   return Status;
}

/* ============================================================================================= */
/**
 * \brief   Validates External RAM
 *
 * \details Perform's memory test by writing and reading pattern (0x5A), anti-pattern (0xA5)
 *          Read's chunk of Internal RAM to local temp.
 *          Entire RAM test is performed by reading RAM memory in chunks
 * \warning None
 *
 * \param   None
 *
 * \return  Status of External RAM Test
 *
 * ============================================================================================= */
static BACKGROUND_STATUS ValidateExternalRAM (void)
{
   BACKGROUND_STATUS Status;
   uint32_t FaultyAddr;

   /* Validates External RAM */
   Status = ValidateRam(&ExternalRamInfo,&FaultyAddr, MEMORY_TEST_EXT_RAM_SIZE);

   if(BACKGROUND_STATUS_ERROR == Status)
   {
      Log(ERR,"External RAM memory Test Failed at address: %X",FaultyAddr);
   }
   else if(BACKGROUND_STATUS_INVALIDPARAM == Status)
   {
      Log(ERR,"External RAM memory Test Invalid parameter,Starting Address: %X, End Address: %X, Memory tested size: %d",
          ExternalRamInfo.RamStartAddress, ExternalRamInfo.RamEndAddress, ExternalRamInfo.MemTested);
   }
   else
   {
      /* Do nothing */
   }

   /* return status */
   return Status;
}

//* ============================================================================================= */
/**
 * \brief   Validates RAM chunk
 *
 * \details Calculates the RAM offset to validate. Take backup of RAM data to local stack. 
 *          Perform bit flip on each location.
 *          Any error in reading the same pattern will be reported.
 *          Write back's original data to RAM.
 *
 * \param   raminfo: Internal or External RAM information
 *          pfaultyAddr: Holds faulty address if memory failure identified else holds 0
 *
 * \return  Status of Memory test
 *
 * ============================================================================================= */
#pragma optimize=none
static BACKGROUND_STATUS ValidateRam(BackgroundRamInfo *pRaminfo, uint32_t *pFaultyAddr, uint32_t  Memorysize)
{
    register uint8_t  ArrayIndex;                      /* Loop Index */
    register uint8_t  LoopSize;
    register BACKGROUND_STATUS Status;           /* Status of RAM memory chunk test */
    register uint32_t  *pRamWordAddr;          /* Holds next chunk starting address in word format */
    register int32_t  BackUpData;             /* Holds each word data in backup */
    uint32_t RamNextChunk;
    OS_CPU_SR cpu_sr;                         /* Required for OS_ENTER/EXIT_CRITICAL */

    Status = BACKGROUND_STATUS_OK;

    /* Last chunk of memory might be less than FIXED size */
    if( ( pRaminfo->MemTested + Memorysize) > pRaminfo->RamSize )
    {
        Memorysize = pRaminfo->RamSize - pRaminfo->MemTested;
    }
    /* Get Loop size in words */
    LoopSize   =  Memorysize/4;
    /* Get next ram chunk starting address */
    RamNextChunk = pRaminfo->RamStartAddress + pRaminfo->MemTested;

    /* Is ram next chunk is within the RAM end address */
    if( ( (RamNextChunk + Memorysize) ) <= pRaminfo->RamEndAddress )
    {
          pRamWordAddr = (uint32_t*)RamNextChunk;
          /* Enter Critical Section to perform RAM memory test */
          /* In critical section ensure to depend on local stack    */
          /* OS and Interrupts are disabled */
          CPU_CRITICAL_ENTER();

          /* Perform memory test using word    */
          /* Word is preferred over byte to decrease runtime */
          for( ArrayIndex=0;ArrayIndex<LoopSize;ArrayIndex++ )
          {
            /* Take backup */
              BackUpData = *pRamWordAddr;

              /*Set Bits high and low */
              *pRamWordAddr  = SRAMPATTERN;
               TM_Hook(HOOK_RAMPATTERNFAIL, (void *)(pRamWordAddr));
              if( *pRamWordAddr != SRAMPATTERN )
              {
                  /* Same data was not read. Update status as ERROR */
                  Status = BACKGROUND_STATUS_ERROR;
                  /* Memory failure, save failed memory location  */
                  *pFaultyAddr = (uint32_t)(pRamWordAddr);
                  break;
              }

              /* Flip bits */
              *pRamWordAddr = SRAMANTIPATTERN;
              TM_Hook(HOOK_RAMANTIPATTERNFAIL, (void *)(pRamWordAddr));
              if( *pRamWordAddr != SRAMANTIPATTERN)
              {
                  /* Same data was not read. Update status as ERROR */
                  Status = BACKGROUND_STATUS_ERROR;
                  /* Take a copy of failed memory address */
                  *pFaultyAddr = (uint32_t)(pRamWordAddr);
                  break;
              }
              /* Restore backup */
              *pRamWordAddr = BackUpData;
               pRamWordAddr++;
          }

          CPU_CRITICAL_EXIT();
          /* Update total memory tested */
          pRaminfo->MemTested += Memorysize;

          if(pRaminfo->MemTested == pRaminfo->RamSize)
          {
              /* Reset total RAM tested */
              pRaminfo->MemTested = 0;
              /* Set Flag RAM is tested */
              pRaminfo->IsEntireRamTested = true;
          }
    }
    else
    {
        /* RAM memory test not performed due to invalid parameters */
        Status = BACKGROUND_STATUS_INVALIDPARAM;
    }

    return Status;
}

//* ============================================================================================= */
/**
 * \brief   Initializes flash test every 60 min timer
 *
 * \details On 60 min timer expiry calls  enables Flash validation flag and calls the FLASH VALIDATION api.
 *          Clear's the Flash validation flag when entire FLASH is validated.
 *
 * \param   None
 *
 * \return  Status of Flash Memory test
 *
 * ============================================================================================= */
static FLASH_CRCVALIDATION_STATUS ValidateFlashArea(void)
{
   static CRC_INFO CrcHandle;                            ///< Flash CRC Information
   FLASH_CRCVALIDATION_STATUS  Status;

    /* Initial status set to unknown */
   Status = FLASH_CRCVALIDATION_UNKNOWN;

   /* Is 60min expired */
   if((OSTimeGet() - ValidationTimers.FlashTestTimer) >= FLASHCODE_VALIDATIONTIME)
   {
      /* Reset the time value */
      ValidationTimers.FlashTestTimer = OSTimeGet();
      /* Enable flash validation flag */
      ValidationTimers.ValidateFlash    = true;
      Log(REQ, "Flash Integrity Check Started ");
   }
   /* Flash validation is Enabled */
   if(ValidationTimers.ValidateFlash)
   {
        Status = L4_ValidateMainAppFromFlash(&CrcHandle);

       if(FLASH_CRCVALIDATION_INPROGRESS != Status)
       {
           /* Flash validation is not in progress, disable flash validation flag*/
           ValidationTimers.ValidateFlash = false;
       }
       if( FLASH_CRC_VALIDATED_BAD == Status )
       {
          Log(ERR,"Flash Integrity Check CRC Failed");
       }
       else if( FLASH_CRC_VALIDATED_GOOD == Status )
       {
          Log(REQ,"Flash Integrity Check CRC Matched with Flash CRC");
       }
       else
       {
          /* Do nothing */
       }
   }
   return Status;
}

//* ============================================================================================= */
/**
 * \brief   This function initializes the memory fence area at the end of a task's stack or a memory array to
 *          the test values.
 *
 * \details The memory fence is an area at the end of each task's stack or an array.
 *          During intialization each memory fence area is written with fixed pattern (MEMORY_FENCE_STRING).
 *
 * \param   None
 *
 * \return  None
 *
 * ============================================================================================= */
static void BackgroundMemoryFenceInit(void)
{  
    uint8_t MemoryFenceCounter;

    for ( MemoryFenceCounter = 0; MemoryFenceCounter < MEMORY_FENCE_SIZE_BYTES; MemoryFenceCounter++ )
    {    
        MemoryFencePattern[MemoryFenceCounter] = MEMORY_FENCE_PATTERN;
    }
    /* Iterate throught Memory Fence Array */
    for ( MemoryFenceCounter = 0; MemoryFenceCounter < MEMORY_FENCE_COUNT; MemoryFenceCounter++ )
    {    
        /* Initialize the Memory fence with fixed pattern (MEMORY_FENCE_STRING) */
        memcpy( BackgroundMemoryFenceArray[MemoryFenceCounter].MemoryFencePtr ,MemoryFencePattern,MEMORY_FENCE_SIZE_BYTES);      
    }
}

//* ============================================================================================= */
/**
 * \brief  This function checks the memory fence of a task's stack or a memory array to determine if
 *         the stack or memory array has overflowed.
 *
 * \details The memory fence is an area at the end of each task's stack or memory array
 *          After the initialization if the memory fence for a task has data
 *          written into it, the stack or memory array has overflowed.
 *          This function Checks BackgroundMemoryFenceArray table,
 *          tests each Memory Fence array with already written fixed pattern (MEMORY_FENCE_STRING)
 *
 * \param   None
 *
 * \return  BACKGROUND_STATUS
 * \return     BACKGROUND_STATUS_OK    - No Memory Fence Error
 * \return     BACKGROUND_STATUS_ERROR - Memory Fence Error
 * ============================================================================================= */
static BACKGROUND_STATUS BackgroundTaskMemoryFenceTest(void)
{
    uint8_t MemoryFenceCounter;
    BACKGROUND_STATUS BackGroundStatus;
    
    BackGroundStatus = BACKGROUND_STATUS_OK;
    /* Iterate through the Memory Fence Array */
    for( MemoryFenceCounter = 0; MemoryFenceCounter < MEMORY_FENCE_COUNT; MemoryFenceCounter++ )
    {
        /* Compare Memory fence array with fixed pattern */
        if ( memcmp(BackgroundMemoryFenceArray[MemoryFenceCounter].MemoryFencePtr,MemoryFencePattern,MEMORY_FENCE_SIZE_BYTES) )
        {
            /* Memory Fence was overwritten, there is an overflow log Error */
            BackGroundStatus = BACKGROUND_STATUS_ERROR;
            Log(ERR,"Memory Fence Test Error: %s",BackgroundMemoryFenceArray[MemoryFenceCounter].MemoryFenceErrorString);
        }
    }
    return BackGroundStatus;
}
/**************************************************************************************************/
/*                                         Global Function(s)                                     */
/**************************************************************************************************/
//* ============================================================================================= */
/**
 * \brief   Initializes the Background Diagnostic Task.
 *
 * \details This function initializes the Backgroudn Diagnostic Info. Initializes Flash, RAM starting address, datasize,memory size to read, memory size tested
 *
 *
 * \note    Called during system initialzation from Startup
 *
 * \warning None
 *
 * \param   None
 *
 * \return  BACKGROUND_STATUS
 *
 *
 * ============================================================================================= */
BACKGROUND_STATUS BackgroundDiagTaskInit(void)
{
    uint8_t Error;
    BACKGROUND_STATUS Status;

    Status = BACKGROUND_STATUS_ERROR;
    do
    {
        /* Create thread */
         Error = SigTaskCreate(&BackgroundDiagTask,
                                    NULL,
                                    &BackgroundDiagTaskStack[0],
                                    TASK_PRIORITY_BACKGROUND_DIAGTASK,
                                    BACKGROUNDTASK_STACK_SIZE,
                                    "BackgroundDiag");
        if ( Error != OS_ERR_NONE)
        {
            /* Couldn't create task, exit with error */
            Log(ERR, "BackgroundDiagTaskInit: OSTaskCreateExt FaiLed, Error- %d", Error);
            Status = BACKGROUND_STATUS_ERROR;
            break;
        }

        /* Initialize Memory Fence */
        BackgroundMemoryFenceInit();

        //OSTaskNameSet(TASK_PRIORITY_BACKGROUND_DIAGTASK, "Background Diagnostic Task", &Error);
        /* Status is OK */
        Status = BACKGROUND_STATUS_OK;
    } while (false);
    return Status;
}

/**
 * \}   <this marks the end of the Doxygen group>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif