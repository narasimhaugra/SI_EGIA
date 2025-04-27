#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L4_ConsoleManager
 * \{
 *
 * \brief   Console Manager Functions
 *
 * \details Signia Handle communicates with external software applications
 *          through various commands.The console manager facilitates in receiving
 *          these commands and transmitting responses back to the external
 *          applications through the communication manager interface.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_ConsoleManager.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER                (LOG_GROUP_CONSOLE)  /*! Log Group Identifier */
#define MIN_VALID_CMD_FRAME_LENGTH          (6u)                 /*! Min Valid cmd frame len*/
#define CONSOLE_RX_BUFF_SIZE                (1000u)              /*! Receive buffer size*/
#define CONSOLE_MGR_TASK_STACK              (1024u)              /*! Task stack size */

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
COMM_IF         *pConsInterface;
PROCESSDATA     DataProcess;
OS_STK    ConsoleMgrTaskStack[CONSOLE_MGR_TASK_STACK + MEMORY_FENCE_SIZE_DWORDS];     /* Communication Manager task stack */

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static CONS_MGR_STATE ConsEventState = CONS_MGR_STATE_WAIT_FOR_EVENT;
static bool ConsoleMgrInitDone = false;         /* to avoid multiple init */
static OS_EVENT *pSemaConsole;
/// \todo 03/22/2022 KA: uncommenting the pragma below creates a compiler warning, why?
//#pragma location=".sram"
PARTIALDATA PartData;
/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void ConsoleMgrTask(void *pArg);
static CONS_MGR_STATE GetCommEvent(void);
static CONS_MGR_STATE ReadCommMgrBuffer(void);
static uint16_t Get16BitValue(uint8_t *pData);
static CONS_MGR_STATUS SendResponse(PROCESSDATA *pData);
static CONS_MGR_STATE ValidateCommand(PROCESSDATA *pDataFrame);

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Console Manager task
 *
 * \details The function invokes interface specific read/write functions.
 *
 * \param   pArg - pointer to data passed to console mgr task
 *
 * \return  None
 *
 * ========================================================================== */
static void ConsoleMgrTask(void *pArg)
{
    uint8_t Error = false;

    static CONS_MGR_STATE ConsoleProcState = CONS_MGR_STATE_WAIT_FOR_EVENT;

    while ( true )
    {
        switch ( ConsoleProcState )
        {
            case CONS_MGR_STATE_WAIT_FOR_EVENT:
                OSSemPend(pSemaConsole, 0, &Error); /* pend for an event by default */
                if ( OS_ERR_NONE != Error )
                {
                    Log(ERR, "ConsoleMgrTask: OSSemPend time out error");
                    break;
                }
                /* get current comms event that triggered the semaphore*/
                ConsoleProcState = GetCommEvent();
                break;

            case CONS_MGR_STATE_GET_PCKT:
                ConsoleProcState = ReadCommMgrBuffer();
                break;

            case CONS_MGR_STATE_VALIDATE_PCKT:
                ConsoleProcState = ValidateCommand( &DataProcess );
                break;

            case CONS_MGR_STATE_PROCESS_COMMAND:
                ConsoleProcState = ProcessCommand( &DataProcess );
                break;

            case CONS_MGR_STATE_SEND_RESPONSE:
                SendResponse( &DataProcess );
                /* if we have more commands to process go back to ProcessCommand State */
                if ( DataProcess.ValidCommandCount > 0 )
                {
                    DataProcess.CommandCounter++;
                    DataProcess.ValidCommandCount--;
                    ConsoleProcState = CONS_MGR_STATE_PROCESS_COMMAND;
                }
                /*  Commands processed */
                else
                {
                    ConsoleProcState = CONS_MGR_STATE_WAIT_FOR_EVENT;
                }
                break;

            default:
                Log(ERR, "ConsoleMgrTask: reached default case");
                ConsoleProcState = CONS_MGR_STATE_WAIT_FOR_EVENT;
                break;
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Get function to access  comm event value
 *
 * \details comm manager intimates about various events with this handler.
 *          Post a semaphore to further process the data.
 *
 * \param   <None>
 *
 *
 * \return  CONS_MGR_STATE - Current value of Comm event
 *
 * ========================================================================== */
static CONS_MGR_STATE GetCommEvent(void)
{
  return ConsEventState;
}

/* ========================================================================== */
/**
 * \brief   process the incoming data
 *
 * \details get data from comms mgr cir buffer
 *
 * \param   < None >
 *
 * \return  CONS_MGR_STATE - return state
 * \retval  CONS_MGR_STATUS_ERROR - No data found in buffer
 * \retval  CONS_MGR_STATUS_OK -  New data received and read into local buffer
 *
 * ========================================================================== */
static CONS_MGR_STATE ReadCommMgrBuffer(void)
{
    static uint8_t  Data[CONSOLE_RX_BUFF_SIZE];     /* Data out buffer */

    CONS_MGR_STATE ConsoleTaskNextState = CONS_MGR_STATE_WAIT_FOR_EVENT;

    memset(Data, 0x00, sizeof(Data));

    DataProcess.pDataIn = Data;
    DataProcess.pDataIF = pConsInterface;

    if ( DataProcess.pDataIF != NULL )
    {
        DataProcess.pDataIF->Peek(&DataProcess.RxDataSize);/* get data count present in buffer*/

        if ( DataProcess.RxDataSize > 0 )
        {
            if(PartData.Flag)
            {/*if previous data was partial read only the remaining bytes from buffer*/
                DataProcess.RxDataSize = PartData.RemainingDataSize;
            }

            /* Get data from comm manager buffer*/
            DataProcess.pDataIF->Receive(DataProcess.pDataIn, &DataProcess.RxDataSize);

            ConsoleTaskNextState = CONS_MGR_STATE_VALIDATE_PCKT;
        }
    }

    return ConsoleTaskNextState;
}

/* ========================================================================== */
/**
 * \brief   Validate data.
 *          Todo: Re-write The Validate routine  to parse through the data in states.
 *
 * \details  This routine checkes for more than one valid command till it processes all the data it receives in
 *           data frame and adds those valid commands in the pValidCommands array. A count of valid commands is maintianed
 *           in the Data frame for next states to process and send the response to the commands.
 *
 *           Below are the details of how each of the state handles multiple commands received in a data frame.
 *
 *          ValidateCommand : adds all valid commands received (currently max commands set to MAX_VALID_COMMANDS)
 *                            Also, initializes number of valid commands (ValidCommandCount) present in the data frame.
 *          ProcessCommand  : Processes a Valid commands identified in ValidateCommand state
 *          SendResponse    : Sends the response to the command that is processed by Process command. If more commands
 *                            need to be procesed transitions to ProcessCommand else transition to "Wait for events".
 *
 * \param   pDataFrame - pointer to data frame
 *
 * \return  CONS_MGR_STATE - return state
 * \retval  CONS_MGR_STATE_WAIT_FOR_EVENT - in case of any error Wait for next data event
 * \retval  CONS_MGR_STATE_PROCESS_COMMAND - Validation sucess, process the command
 *
 * ========================================================================== */
static CONS_MGR_STATE ValidateCommand(PROCESSDATA *pDataFrame)
{
    uint16_t  PacketSize;          /* Packet size */
    uint16_t  ByteCount = 0;       /* Incoming packet data index */
    uint16_t  CalculatedCheckSum;  /* Calculated checksum */
    uint16_t  ReceivedCheckSum;    /* Checksum in the packet */
    uint16_t  PartCommandSize;     /* Size of the Part command frame */
    int16_t   ProcessedDataSize;   /* Number of Data bytes processed */
    uint8_t   *pStartAddress;      /* Start address byte (0xAA) of command received */
    uint8_t   CommandCount;        /* number of commands received in the data frame */
    uint8_t   temp;                /* temp vaiable */
    uint16_t  RemainingDataSize;   /* Data frame bytes remaining to be processed */
    CONS_MGR_STATE NxtConsTskState = CONS_MGR_STATE_WAIT_FOR_EVENT;

    /* Initialize command specific data */
    pDataFrame->CommandCounter      = 0x0;
    pDataFrame->ValidCommandCount   = 0x0;

    /* Initialize ValidCommands array at the beginning of this state */
    for (temp=0; temp< MAX_VALID_COMMANDS; temp++)
    {
        pDataFrame->pValidCommands[temp] = 0x0;
    }

    /* initalize start address to the received data frame */
    pStartAddress = pDataFrame->pDataIn;

    /* initialize local variables */
    ProcessedDataSize = 0x0;
    CommandCount = 0x0;
    PacketSize   = 0x0;

    do
    {
        if ( pDataFrame == NULL || pDataFrame->pDataIn == NULL )
        {
            break;
        }
        /* Bytes remaining to be processed in the data frame */
        RemainingDataSize = pDataFrame->RxDataSize - ProcessedDataSize;

         /* break if we reached max command limit */
        if (CommandCount >= MAX_VALID_COMMANDS)
        {
            break;
        }

        if ( PartData.Flag )
        {
            PartData.Flag = 0;

            if ( OSTimeGet() < PartData.Timeout ) /* if previous data was incomplete and current event arrived within wait timeout*/
            {
                PartData.Flag = false;
                /*Read the partial data*/
                memcpy( &PartData.Data[PartData.PacketSize - PartData.RemainingDataSize], &pDataFrame->pDataIn, RemainingDataSize);

                /* Assign partial data buffer for processing command */
                pStartAddress = PartData.Data;
                RemainingDataSize = PartData.PacketSize;
            }
        }

        ByteCount = 0x0;
        /* get the PACKET_START */
        while ((pStartAddress[ByteCount++] != PCKT_START) && (ByteCount < RemainingDataSize));

        /* In case of partial data at the least START BYTE and DATA SIZE should be received */
        /* For 16 bit msg START_BYTE + DATA_SIZE is 3 bytes*/
        if ((RemainingDataSize - (ByteCount - 1)) < MIN_PCKT_SIZE)/* If anything less than 3 is received discard the packet */
        {
            break;
        }

        /* get PACKET_START Index */
        pDataFrame->PacketStartIndex[CommandCount] = ByteCount - 1;

        /* get total Packet size*/
        PacketSize = Get16BitValue( &pStartAddress[pDataFrame->PacketStartIndex[CommandCount] + PCKT_SIZE_OFFSET]);

        /* additional check to make sure we are processing a command with atleast minimum size */
        if ( PacketSize < MIN_PCKT_SIZE )
        {
            break; // invalid packet - discard
        }

        /* check if incomplete message is received */
        if ( ( pDataFrame->PacketStartIndex[CommandCount] + PacketSize ) > RemainingDataSize )
        {
            PartData.Flag = true;

            memset( PartData.Data, 0x00, PARTIAL_DATA_BUFF_SIZE);

            PartCommandSize = RemainingDataSize - pDataFrame->PacketStartIndex[CommandCount];

            /* Save the incomplete data to be used with next data event */
            memcpy( PartData.Data, &pStartAddress[pDataFrame->PacketStartIndex[CommandCount]], PartCommandSize );

            /* This is the complete expected packet size*/
            PartData.PacketSize =  PacketSize;
            /* remaining bytes expected*/
            PartData.RemainingDataSize = PacketSize - PartCommandSize;

            /*Get OS timer value to set the partial data timeout */
            PartData.Timeout = OSTimeGet() + MAX_TIME_TO_WAIT_FOR_PACKET;

            break;
        }

        /* get data size*/
        pDataFrame->DataSize =  PacketSize - PCKT_OVERHEAD_16BIT;

        /* Calculate checksum for received data */
        CalculatedCheckSum = CRC16(0, (pStartAddress + pDataFrame->PacketStartIndex[CommandCount]), (PacketSize - sizeof(uint16_t)));
        ReceivedCheckSum = Get16BitValue(&pStartAddress[(pDataFrame->PacketStartIndex[CommandCount] + PacketSize) - sizeof(uint16_t)]);

        if (ReceivedCheckSum != CalculatedCheckSum)
        {
            Log(ERR, "ValidateCommand: checksum error received: %d, calculated checksum: ",ReceivedCheckSum,CalculatedCheckSum);
            break;
        }

        /* Found a valid command - Update Valid command count and associated addresses  */
        pDataFrame->ValidCommandCount = CommandCount;
        /* for command[n] - Update Valid command start address  */
        pDataFrame->pValidCommands[pDataFrame->ValidCommandCount] = pStartAddress;

        /* After we found a valid address - consider those bytes as processed */
        ProcessedDataSize += PacketSize;
        /* look for remaining commands in the data frame */
        pStartAddress += PacketSize;

        /*  command counter incremented before we loop around to look for more commands -
           at this stage we found valid commands and proceeding to find new commands */
        CommandCount++;
        /* debug purpose - will be removed after testing */
        #if 0
        if(CommandCount > 1)
        {

            //Log(DBG,"PKT %d  %d %d", ProcessedDataSize,RemainingDataSize,PacketSize);
            //Log(DBG,"A %d  %d cnt %d", pStartAddress,pDataFrame->pDataIn,CommandCount);
            Log(DBG,"cnt %d", CommandCount);
            Log(DBG,"-------------------------");
        }
        #endif
        NxtConsTskState = CONS_MGR_STATE_PROCESS_COMMAND;
    } while ( ProcessedDataSize < pDataFrame->RxDataSize );

    return NxtConsTskState;
}

/* ========================================================================== */
/**
 * \brief   send response data
 *
 * \details create a response frame and send  via comms manager interface
 *
 * \param   pSendData - pointer to data
 *
 * \return  CONS_MGR_STATE - return state
 * \retval  CONS_MGR_STATUS_ERROR - No data found in buffer
 * \retval  CONS_MGR_STATUS_OK -  New data received and read into local buffer
 *
 * ========================================================================== */
static CONS_MGR_STATUS SendResponse(PROCESSDATA *pSendData)
{
    uint16_t     Checksum;                         /* Checksum of the packet */
    uint16_t     PacketSize;                       /* Data packet size */
    uint16_t     DataCount;                        /* Data count */
    uint8_t      DataOut[MAX_DATA_TRANSMIT_SIZE];         /* Buffer to store the circular buffer data */
    uint16_t     Count;
    uint8_t     *pResponseData;
    uint8_t     *pDataIn;
    uint16_t    CommandStartIndex;

    CONS_MGR_STATUS Status = CONS_MGR_STATUS_ERROR;

    do
    {
        if ( pSendData == NULL || pSendData->pDataOut == NULL || pSendData->pDataIF->Send == NULL )
        {
            break;
        }

        DataCount = 0;
        Checksum  = 0;
        pResponseData = pSendData->pDataOut;
        memset(DataOut, 0x00, sizeof(DataOut));

        DataOut[DataCount++] = PCKT_START;

        PacketSize = pSendData->TxDataCount + PCKT_OVERHEAD_16BIT;
        /* get Packet size LSB */
        DataOut[DataCount++] =  PacketSize;
        /* get Packet size MSB */
        DataOut[DataCount++] =  PacketSize >> SHIFT_8_BITS;
        /* get starting address of the current command being processed */
        pDataIn              = pSendData->pValidCommands[pSendData->CommandCounter];
        /* get starting Index address  of the command being processed */
        CommandStartIndex    = pSendData->PacketStartIndex[pSendData->CommandCounter];
        /* get command offset */
        DataOut[DataCount++] = pDataIn[ CommandStartIndex + COMMAND_OFFSET_16BIT ];

        if ( pResponseData != NULL)
        {
            /* add response data to the send buffer */
            for(Count = 0; Count < pSendData->TxDataCount; Count++)
            {
                DataOut[DataCount++] = *pResponseData++;
            }
        }

        /* Calculate Checksum 16 for the packet */
        for ( Count = 0; Count < DataCount ; Count++)
        {
            Checksum = CRC16(Checksum, &DataOut[Count], 1);
        }

        DataOut[DataCount++] = Checksum;
        DataOut[DataCount++] = Checksum >> SHIFT_8_BITS;

        /* Post data to communication manager send buffer */
        pSendData->pDataIF->Send(DataOut, &DataCount);

        Status = CONS_MGR_STATUS_OK;

    } while ( false );

    return Status;
}

/* ========================================================================== */
/**
 * \brief   helper function
 *
 * \details helper function to add bytes and get a bit equivalent value.
 *
 * \param   pData - pointer to data
 *
 * \return  uint16_t - returns 16bit value
 *
 * ========================================================================== */
static uint16_t Get16BitValue(uint8_t *pData)
{
   return ( ( (uint16_t)pData[1] << SHIFT_8_BITS) | ( ( uint16_t)pData[0] ) );
}

/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Function to initialize the Console Manager
 *
 * \details This function creates the Console manager task, mutex and timer
 *
 * \param   < None >
 *
 * \return  CONS_MGR_STATUS
 * \retval  COMM_MGR_STATUS_ERROR - On Error
 * \retval  COMM_MGR_STATUS_OK - On success
 *
 * ========================================================================== */
CONS_MGR_STATUS L4_ConsoleMgrInit(void)
{
    CONS_MGR_STATUS  ConsoleMgrStatus = CONS_MGR_STATUS_ERROR;        /* contains status */
    uint8_t          OsError;                                         /* contains status of error */
    #ifdef CREATE_STATUS_TASK
    static OS_STK    ConsoleStatusTaskStack[CONSOLE_MGR_TASK_STACK];  /* Communication Manager task stack */
    #endif
    uint8_t          u8OsError;  /// \\todo 02-17-2022 KA : remove, use OsError?

    do
    {
        if ( ConsoleMgrInitDone )
        {
            /* Repeated initialization attempt, do nothing */
            break;
        }

        pSemaConsole = SigSemCreate(0, "ConsoleTaskSema", &u8OsError);    /* Semaphore to block by default */

        if (NULL == pSemaConsole)
        {
            /* Something went wrong semaphore creation, return with error */
            Log(ERR, "L4_ConsoleManagerInit: Create Semaphore Error");
            break;
        }

        /* Create the Communication Manager Task */
        OsError = SigTaskCreate(&ConsoleMgrTask,
                                NULL,
                                &ConsoleMgrTaskStack[0],
                                TASK_PRIORITY_L4_CONSOLE_MANAGER,
                                CONSOLE_MGR_TASK_STACK,
                                "ConsoleMgr");

        if (OsError != OS_ERR_NONE)
        {
            /* Couldn't create task, exit with error */
            Log(ERR, "L4_ConsoleManagerInit: ConsoleMgrTask Create Error - %d", OsError);
            break;
        }

        pConsInterface = L4_CommManagerConnOpen(COMM_CONN_ACTIVE, &CommEventHandler);

        if ( pConsInterface == NULL )
        {
            Log(ERR, "L4_ConsoleManagerInit: No active connection");
        }

        /* Initialization done */
        ConsoleMgrInitDone = true;
        /// \todo 06/01/2021 CPK Below task creation to be Integrated to streaming task - disabled with compiler switch
        #ifdef CREATE_STATUS_TASK
        OsError = SigTaskCreate(&SendStatusVars,
                                NULL,
                                &ConsoleStatusTaskStack[0],
                                TASK_PRIORITY_L4_CONSOLE_STATUS,
                                CONSOLE_MGR_TASK_STACK,
                                "ConsoleMgrSendStatus");
        #endif
        ConsoleMgrStatus = CONS_MGR_STATUS_OK;

    } while ( false );

    return ConsoleMgrStatus;
}

/* ========================================================================== */
/**
 * \brief   Function to return the status of initialization the Console Manager
 *
 * \details This function returns the initialization status of console manager
 *
 * \param   < None >
 *
 * \return  bool
 * \retval  true -  On Error
 * \retval  false - On success
 *
 * ========================================================================== */
/// \\todo 02/17/2022 CPK Below function review to be done during other MCP commands review - added to support blackbox testing
bool L4_ConsoleMgrInitDone(void)
{
    bool DoneStatus;

    if (ConsoleMgrInitDone)
    {
        DoneStatus = false; /* return false on success */
    }
    else
    {
        DoneStatus = true;
    }

    return DoneStatus;
}

/* ========================================================================== */
/**
 * \brief   Function to set the Active interface
 *
 * \details This function updates the Interface update status of console manager.
 *          Active interface is initialized on starup. This function would be required by the interfaces
 *          which come up after startup (Eg. WLAN)
 *
 * \param   pActiveInterface - pointer to acrive interface
 *
 * \return  bool
 * \retval  true -  On Error
 * \retval  false - On success
 *
 * ========================================================================== */
bool L4_ConsoleMgrUpdateInterface(COMM_IF *pActiveInterface)
{
    pConsInterface = pActiveInterface;

    return false;
}

/* ========================================================================== */
/**
 * \brief   Function to send data
 *
 * \details This function can be used to send data via console manager
 *
 * \param   ComID - communication interface to use to send data
 * \param   Cmd - serial command to send
 * \param   pData - Pointer to the data to be sent
 * \param   DataCount - Size of data to send
 *
 * \return  CONS_MGR_STATUS
 * \retval  COMM_MGR_STATUS_ERROR - On Error
 * \retval  COMM_MGR_STATUS_OK - On success
 *
 * ========================================================================== */
CONS_MGR_STATUS L4_ConsoleMgrSendRequest( COMM_CONN ComID, SERIAL_CMD Cmd, uint8_t *pData, uint16_t DataCount )
{
    PROCESSDATA Data;
    uint8_t DummyDataIn[MIN_VALID_CMD_FRAME_LENGTH];

    CONS_MGR_STATUS Status = CONS_MGR_STATUS_ERROR;

    do
    {
        if ( ( Cmd <= SERIALCMD_UNKNOWN  || Cmd >= SERIALCMD_COUNT ) || ( ComID >= COMM_CONN_COUNT ) )
        {
            Status = CONS_MGR_STATUS_INVALID_COMMAND;
            break;
        }

        if ( ComID == COMM_CONN_USB || ComID == COMM_CONN_WLAN || ComID == COMM_CONN_ACTIVE )
        {
             /* if we have a NULL Handle - get the  Active handle */
             if( NULL == pConsInterface )
             {
                 pConsInterface = L4_CommManagerConnOpen(COMM_CONN_ACTIVE, &CommEventHandler);
             }
             /* if still have a NULL handle - break */
             if( NULL == pConsInterface )
             {
                 Status = CONS_MGR_STATUS_ERROR;
                 break;
             }
             /* update the console manager handle */
             Data.pDataIF = pConsInterface;
        }

        /* add required information for creating frame format */
        Data.ValidCommandCount   = 0x0;           /* Valid command count - '0' is valid */
        Data.CommandCounter      = 0x0;           /* Current command counter to be processed */
        Data.PacketStartIndex[0] = 0x0;           /* Packet start address */
        DummyDataIn[COMMAND_OFFSET_16BIT] = Cmd;  /* Command id */
        Data.pValidCommands[0] = DummyDataIn;     /* command  */
        Data.pDataOut = pData;                    /* Data  */
        Data.TxDataCount =  DataCount;            /* Size of response*/

        /* call console manager send function*/
        Status = SendResponse( &Data );

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Handler for com manager events
 *
 * \details Com manager intimates about various events with this handler.
 *          Post a semaphore to further process the data.
 *
 * \param   Event - Com event for new data, connect and disconnect
 *
 * \return  None
 *
 * ========================================================================== */
void CommEventHandler(COMM_MGR_EVENT Event)
{
    switch( Event )
    {
        case  COMM_MGR_EVENT_CONNECT:              /*! Connect event */
        case  COMM_MGR_EVENT_DISCONNECT:           /*! Disconnect event */
        case  COMM_MGR_EVENT_SUSPEND:              /*! Suspend */
        case  COMM_MGR_EVENT_ERROR:                /*! Error */
        case  COMM_MGR_EVENT_RESUME:               /*! Resume */
        case  COMM_MGR_EVENT_RESET:                /*! Reset */
            ConsEventState = CONS_MGR_STATE_WAIT_FOR_EVENT;
            break;

        case  COMM_MGR_EVENT_NEWDATA:              /* New data */
            ConsEventState = CONS_MGR_STATE_GET_PCKT;
            break;

        default:
            break;
    }
    /* Post semaphore to enable console manager task to work on the comms event*/
    if (OS_ERR_NONE != OSSemPost(pSemaConsole))
    {
        Log(ERR, "CheckNewCommand: Error on OSSemPost");
    }
}

/**
 *\}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

