#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_I2c
 * \{
 * \brief   Layer 3 I2C control routines
 *
 * \details This file contains IPC protected I2C interfaces implementation.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_I2c.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Includes                                       */
/******************************************************************************/
#include "L3_I2c.h"
#include "Logger.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/
/* None */

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/
/* None */

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_I2C)       /* Log Group Identifier */
#define MAX_I2C_DEVICES             (MAX_I2C_SLAVE)       /* Maximum I2C devices */
#define I2C_DEVICE_EMPTY            (0xFFFFu)             /* Empty device configuration slot indicator */

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef enum                /// I2C transaction types
{
    I2C_TXN_READ,           ///< I2C read transaction
    I2C_TXN_WRITE,          ///< I2C write transaction
    I2C_TXN_READ_BURST,     ///< I2C Burst read transaction
    I2C_TXN_COUNT           ///< I2C transaction range indicator
} I2C_TXN;

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static OS_EVENT *pMutexI2c = NULL;              /* I2C access mutex, synchronizes L2 I2C calls */
static I2CControl ConfigList[MAX_I2C_DEVICES];  /* I2C configuration registry  */
static uint16_t ActiveDevice;                   /* I2C slave device address, supports 10 bit address as well */
static uint16_t ActiveTimeout;                  /* Active I2C request timeout */

static OS_TCB   *pCurrentUser = NULL;       // Pointer to TCB of current I2C user
static uint8_t   UseCount = 0;              // User nesting count

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static I2CControl *I2cGetConfig(uint16_t Device);
static I2C_STATUS  I2cAddConfig(I2CControl *pPacket);
static I2C_STATUS  I2cActivateConfig(uint16_t Device);
static I2C_STATUS  I2cTransfer(I2CDataPacket *pPacket, I2C_TXN Transfer);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   Layer 3 I2C Transfer
 *
 * \details This function is a wrapper function around layer 2 I2C read and write functions
 *
 * \param   pPacket    - Pointer to data packet.
 * \param   Transfer    - Transfer type indicating read or write transaction.
 *
 * \return  I2C_STATUS - function execution status
 * \retval      I2C_STATUS_SUCCESS               - Configuration Accepted
 * \retval      I2C_STATUS_IDLE                  - Bus is Idle
 * \retval      I2C_STATUS_FAIL                  - Status Fail
 * \retval      I2C_STATUS_BUSY     -            - Bus is busy
 * \retval      I2C_STATUS_FAIL_CONFIG           - Configuration Failed
 * \retval      I2C_STATUS_FAIL_INVALID_PARAM    - Failed due to invalid parameter
 * \retval      I2C_STATUS_FAIL_NO_RESPONSE      - No response from device
 * \retval      I2C_STATUS_FAIL_TIMEOUT          - Request timed out
 *
 * ========================================================================== */
static I2C_STATUS I2cTransfer(I2CDataPacket *pPacket, I2C_TXN Transfer)
{
    I2C_STATUS Status;
    uint8_t u8OsError;

    u8OsError = 0u; // Initialize variable

    do
    {
        if (NULL == pPacket)
        {
            Status = I2C_STATUS_FAIL_INVALID_PARAM;
            break;
        }

        // If the same task is trying to get in, don't pend on mutex

        u8OsError = OS_ERR_NONE;        // Default to no OS error (in case mutex pend is skipped)

        if (pCurrentUser != OSTCBCur)
        {
            OSMutexPend(pMutexI2c, /*ActiveTimeout*/ 2000, &u8OsError);    /* Mutex lock */ //26-SEP-22 /// \todo 03/23/2021 DAZ - Try long timeout
            if (OS_ERR_NONE  != u8OsError)
            {
                /* Waited too long, return for now */
                Status = I2C_STATUS_FAIL_TIMEOUT;
                Log(DEV,"Access wait timeout: 0x%x, task: %s", pPacket->Address, OSTCBCur->OSTCBTaskName);
                break;
            }
            pCurrentUser = OSTCBCur;        // Take ownership of I2C
        }

        UseCount++;                         // Increment use count

        Status = I2cActivateConfig(pPacket->Address);    /* Check and re-configure if needed */
        if (I2C_STATUS_SUCCESS != Status)
        {
            Log(DEV,"Config Failed : 0x%x", pPacket->Address);
            break;
        }

        if(I2C_TXN_WRITE == Transfer)
        {
            Status = L2_I2cWrite(pPacket);
        }
        else if (I2C_TXN_READ == Transfer)
        {
            Status = L2_I2cRead(pPacket);
        }
        else if (I2C_TXN_READ_BURST == Transfer)
        {
            Status = L2_I2cBurstRead(pPacket);
        }
        else
        {
            Status = I2C_STATUS_FAIL_INVALID_PARAM;    /* No break so the mutex is released */
        }

    } while (false);

    if (OS_ERR_NONE  == u8OsError)
    {
        UseCount--;
        if (0 == UseCount)
        {
            pCurrentUser = NULL;        // No current user of I2C
            OSMutexPost(pMutexI2c);    /* Mutex Release */
        }
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief    Get I2C Configuration from registry
 *
 * \details    This function gets the configuration from configuration registry based on
 *          device ID.
 *
 * \param    Device - Device address to fetch configuration from registry
 *
 * \return  I2CControl  - Pointer to requested configuration based on device address
 * \retval  Configuration for device specified
 * \retval  NULL - No device address entry found
 * ========================================================================== */
static I2CControl *I2cGetConfig(uint16_t Device)
{
    I2CControl* pControl;
    uint8_t Index;

    pControl = NULL;
    /* Search through the registered configuration list to see if device configuration exists */
    for (Index = 0; Index < MAX_I2C_DEVICES; Index++)
    {
        if (ConfigList[Index].Device == Device)
        {
            pControl = &ConfigList[Index];
            break;  /* Device configuration found, Stop seaching, break out */
        }
    }

    return pControl;
}

/* ========================================================================== */
/**
 * \brief   Add configuration to registry
 *
 * \details This function adds the configuration specified to the configuration registry.
 *          If configuration already exists, the updates the configuration with new values.
 *          registry.
 *
 * \param   pPacket - Pointer to configuration packet.
 *
 * \return  I2C_STATUS - function execution status
 * \retval      I2C_STATUS_SUCCESS               - Configuration Accepted
 * \retval      I2C_STATUS_FAIL                  - Configuration Failed
 *
 * ========================================================================== */
static I2C_STATUS I2cAddConfig(I2CControl *pPacket)
{

    I2C_STATUS Status;
    uint8_t Index;

    Status = I2C_STATUS_FAIL;
    /* Search for already existing entry or an unused index */
    for (Index = 0; Index < MAX_I2C_DEVICES; Index++)
    {
        if (ConfigList[Index].Device == pPacket->Device)
        {
            /* Entry already existing at 'Index', just update the same entry with new value */
            break;
        }
        else if (ConfigList[Index].Device == I2C_DEVICE_EMPTY)
        {
            /* Empty slot found to store configuration */
            break;
        }
        else
        {
            /* Empty else to handle Misra check 14.10  */
        }
    }
    /* Now Index holds either 'first occurrence of already existing entry' or 'new empty slot'
    Assumption: No intermediate empty slots */
    if (Index < MAX_I2C_DEVICES)
    {
        ConfigList[Index].Device = pPacket->Device;
        ConfigList[Index].AddrMode = pPacket->AddrMode;
        ConfigList[Index].Clock = pPacket->Clock;
        ConfigList[Index].State = pPacket->State;
        ConfigList[Index].Timeout = pPacket->Timeout;
        Status = I2C_STATUS_SUCCESS;    // Added configuration to registry successfully
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Check and activate config
 *
 * \details This function checks if the specified data transfer requires
 *          re-configuration. If yes then perform re-configuration, update active
 *          configuration.
 *
 * \note    Must be called only by L3_I2cRead() and L3_I2cWrite() functions
 *
 * \param   Device - Device ID for which Configuration to be activated
 *
 * \return  I2C_STATUS - function execution status
 * \retval      I2C_STATUS_SUCCESS               - Configuration Accepted
 * \retval      I2C_STATUS_FAIL_CONFIG           - Configuration Failed
 * \retval      I2C_STATUS_FAIL_INVALID_PARAM    - Failed due to invalid parameter
 *
 * ========================================================================== */
static I2C_STATUS I2cActivateConfig(uint16_t Device)
{
    I2C_STATUS Status;
    I2CControl *ConfigCurr;
    I2CControl *ConfigNew;

    Status = I2C_STATUS_SUCCESS;
    if (ActiveDevice != Device)         /* else, no re-configuration needed, return success */
    {
        /* Different device address, check if the configurations are also different */
        ConfigCurr = I2cGetConfig(ActiveDevice);
        ConfigNew  = I2cGetConfig(Device);
        if ((NULL != ConfigCurr) && (NULL != ConfigNew))
        {
            /* Check if required configured is different from the active configuration */
            if ((ConfigCurr->Clock != ConfigNew->Clock) || (ConfigCurr->Timeout != ConfigNew->Timeout) || \
                (ConfigCurr->AddrMode != ConfigNew->AddrMode) || (ConfigCurr->State != ConfigNew->State))
            {
                /* One of more configuration parameter is/are different, reconfigure the I2C bus.
                L2_I2cConfig() is a layer 2 call. It is safe to call as mutex is already acquired */
                Status = L2_I2cConfig(ConfigNew);
                ActiveDevice  = Device;
                ActiveTimeout = ConfigNew->Timeout;

            }
        }
        else
        {
            Status = I2C_STATUS_FAIL_INVALID_PARAM;
        }
    }

    return Status;
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/

/* NOTE TO REVIEWERS: The following two functions are only required if it is necessary
   to retain I2C ownership over multiple I2cTransfer calls. This may be necessary during
   battery communication, or during One Wire Authentication. */

/* ========================================================================== */
/**
* \brief    Claim exclusive access to I2C bus for the calling task.
*
* \details  This function establishes exclusive access to the I2C bus as soon as
*           it is available. It sets pCurrentUser to the currently running task,
*           ensuring only it has access. It also sets the UseCount to 1 to ensure
*           that I2cTransfer does not release the mutex, thus retaining ownership
*           of I2C until L3_I2cRelease is called to release the mutex. All other
*           tasks will pend on the mutex pMutexI2c.
* \n \n
*           This function returns when the mutex has been secured.
*
* \warning  Great care must be taken to insure that there is a matching L3_I2cRelease
*           call for every L3_I2cClaim call. Be extra careful to ensure that error
*           exits, etc. do not bypass the L3_I2cRelease call. Failure to do so will
*           lock out all other tasks!
*
* \note     At this writing, there is no timeout on the pend for the mutex. This
*           feature may be added later if desired.
*
* \param    < None >
*
* \return   I2C_STATUS
* \retval   I2C_STATUS_SUCCESS - Lock obtained
* \retval   I2C_STATUS_FAIL - Lock not obtained
*
* ========================================================================== */
I2C_STATUS L3_I2cClaim(void)
{
    I2C_STATUS Status;
    uint8_t u8OsError = OS_ERR_NONE;

    do
    {
        Status = I2C_STATUS_SUCCESS;

        /* If current user is seeking a lock again, exit OK */
        if (OSTCBCur == pCurrentUser)
        {
            Log(DEV, "Nested Claim error");
            break;
        }

        OSMutexPend(pMutexI2c, 0, &u8OsError);    /* Mutex lock */
        if (OS_ERR_NONE  != u8OsError)
        {
            /* Some kind of error, return for now */
            Status = I2C_STATUS_FAIL;
            Log(DEV,"I2C Claim Mutex Pend failed");
            break;
        }


        /* I2C lock has been obtained - Retain ownership for this task. */
        pCurrentUser = OSTCBCur;
        UseCount = 1;
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Release exclusive access to the I2C bus.
 *
 * \details This function releases exclusive access to the I2C bus by clearing
 *          pCurrentUser and the usage count. It also posts to pMutexI2c to release
 *          the next task pending on it. Only the locking task can release the lock.
 *
 * \note    This function returns an error if a task other than the claiming one
 *          attempts to release the lock.
 *
 * \param   < None >
 *
 * \return  I2C_STATUS
 * \retval  I2C_STATUS_SUCCESS - Lock released
 * \retval  I2C_STATUS_FAIL - Lock not released - see note
 *
 * ========================================================================== */
I2C_STATUS L3_I2cRelease(void)
{
    I2C_STATUS Status;

    do
    {
        Status = I2C_STATUS_SUCCESS;

        /* Only the locking task can release. */
        if (pCurrentUser != OSTCBCur)
        {
            Status = I2C_STATUS_FAIL;
            Log(DEV, "I2C Release attempt by task other than owner");
            break;
        }

        /* Remove ownership before posting so next pending task can claim ownership or
           invoke I2cTransfer*/
        pCurrentUser = NULL;    // Remove owner before posting
        UseCount = 0;           // Reset usage count
        OSMutexPost(pMutexI2c); // Mutex Release
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   Layer 3 I2C hardware initialization routine.
 *
 * \details Initializes I2C port
 *
 * \note    This function is intended to be called once during the system
 *          initialization to initialize all applicable I2C hardware ports.
 *          Any other I2C interface functions should be called only after
 *          calling this function.
 *
 * \param   < None >
 *
 * \return  I2C_STATUS
 *
 * ========================================================================== */
I2C_STATUS L3_I2cInit(void)
{
    uint8_t u8OsError;
    I2C_STATUS Status;
    uint8_t Index;

    Status = I2C_STATUS_SUCCESS;
    do
    {
        pMutexI2c = SigMutexCreate("L3-I2C", &u8OsError);

        if (NULL == pMutexI2c)
        {
            Status = I2C_STATUS_FAIL;
            Log(ERR, "L3_I2cInit: L3 I2c Mutex Create Error - %d", u8OsError);
            break;
        }

        /* Configure the I2C interface for the lowest speed so that any device can use */
        for (Index = 0; Index < MAX_I2C_DEVICES; Index++)
        {
            ConfigList[Index].Device   = I2C_DEVICE_EMPTY;
            ConfigList[Index].AddrMode = I2C_ADDR_MODE_7BIT;
            ConfigList[Index].Clock    = I2C_CLOCK_78K;
            ConfigList[Index].State    = I2C_STATE_ENA;
            ConfigList[Index].Timeout  = 0;
        }
    } while (false);
    ActiveDevice = 0;
    ActiveTimeout = 0;

    return Status;
}

/* ========================================================================== */
/**
 * \brief    I2C Configuration
 *
 * \details This function configures the specified I2C interface with the
 *          parameters supplied to the function. This is a blocking function,
 *          can also be used to enable, disable, activate sleep mode.
 *
 * \param   pControl    - Pointer to  configuration structure.
 *
 * \return  I2C_STATUS - Status of I2C Configuration
 * \retval      I2C_STATUS_SUCCESS               - Configuration Accepted
 * \retval      I2C_STATUS_FAIL_CONFIG           - Configuration Failed
 * \retval      I2C_STATUS_FAIL_INVALID_PARAM    - Failed due to invalid parameter
 * \retval      I2C_STATUS_FAIL_TIMEOUT          - Request timed out
 *
 * ========================================================================== */
I2C_STATUS L3_I2cConfig (I2CControl *pControl)
{
    I2C_STATUS Status;
    uint8_t u8OsError;

    Status = I2C_STATUS_FAIL_CONFIG;
    do
    {
        if (NULL == pControl)
        {
            Status = I2C_STATUS_FAIL_INVALID_PARAM;
            break;
        }
        OSMutexPend(pMutexI2c, ActiveTimeout, &u8OsError);    /*  Mutex lock */
        if (OS_ERR_NONE  != u8OsError)
        {
            Status = I2C_STATUS_FAIL_TIMEOUT;     /* Mutex Timed out */
            break;
        }
        Status = I2C_STATUS_SUCCESS ;       /* No error */
        Status = L2_I2cConfig(pControl);
        I2cAddConfig(pControl);             /* Configuration was successful, add this config to config registry */
        ActiveDevice = pControl->Device;    /* Mark new configuration */
        ActiveTimeout = pControl->Timeout;
        OSMutexPost(pMutexI2c);             /* Mutex release */
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   I2C Data Write
 *
 * \details This function is intended to be used to write to an I2C slave device.
 *          Allows the function to complete write with appropriate timeout set.
 *          If the timeout is 0, then this function blocks until write is complete.
 *
 * \param   pPacket - Pointer to data packet.
 *
 * \return  I2C_STATUS - Function execution status
 * \retval      I2C_STATUS_SUCCESS               - Configuration Accepted
 * \retval      I2C_STATUS_IDLE                  - Bus is Idle
 * \retval      I2C_STATUS_FAIL                  - Status Fail
 * \retval      I2C_STATUS_BUSY     -            - Bus is busy
 * \retval      I2C_STATUS_FAIL_CONFIG           - Configuration Failed
 * \retval      I2C_STATUS_FAIL_INVALID_PARAM    - Failed due to invalid parameter
 * \retval      I2C_STATUS_FAIL_NO_RESPONSE      - No response from device
 * \retval      I2C_STATUS_FAIL_TIMEOUT          - Request timed out
 *
 * ========================================================================== */
I2C_STATUS L3_I2cWrite (I2CDataPacket *pPacket)
{
    I2C_STATUS Status;
    do
    {
        if(NULL == pPacket)
        {
            Status = I2C_STATUS_FAIL_INVALID_PARAM;
            break;
        }
        Status = I2cTransfer(pPacket, I2C_TXN_WRITE);
        if (I2C_STATUS_SUCCESS  != Status )
        {
            Log(DEV,"Write Failed, Address: 0x%X, Status: %d, Task: %s", pPacket->Address, Status, OSTCBCur->OSTCBTaskName);
            OSTimeDly(100);
        }
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   I2C Data Read
 *
 * \details This function is intended to be used to read from an I2C slave device.
 *          Allows the function to complete read with appropriate timeout set.
 *          If the timeout is 0, then this function blocks until read is complete.
 *
 * \param   pPacket    - Pointer to data packet.
 *
 * \return  I2C_STATUS - function execution status
 * \retval      I2C_STATUS_SUCCESS               - Configuration Accepted
 * \retval      I2C_STATUS_IDLE                  - Bus is Idle
 * \retval      I2C_STATUS_FAIL                  - Status Fail
 * \retval      I2C_STATUS_BUSY     -            - Bus is busy
 * \retval      I2C_STATUS_FAIL_CONFIG           - Configuration Failed
 * \retval      I2C_STATUS_FAIL_INVALID_PARAM    - Failed due to invalid parameter
 * \retval      I2C_STATUS_FAIL_NO_RESPONSE      - No response from device
 * \retval      I2C_STATUS_FAIL_TIMEOUT          - Request timed out
 *
 * ========================================================================== */
I2C_STATUS L3_I2cRead(I2CDataPacket *pPacket)
{
    I2C_STATUS Status;

    do
    {
        if(NULL == pPacket)
        {
            Status = I2C_STATUS_FAIL_INVALID_PARAM;
            break;
        }
        Status = I2cTransfer(pPacket, I2C_TXN_READ);
        if (I2C_STATUS_SUCCESS  != Status )
        {
            Log(DEV,"Read failed, Address: 0x%X, Status: %d, Task: %s", pPacket->Address, Status, OSTCBCur->OSTCBTaskName);
        }
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   I2C Data Burst Read
 *
 * \details This function is mis-named. It is like the standard I2cRead() function,
 *          except that a repeated start is NOT sent before reading. The device
 *          address is written, immediately followed by the reading of data. (The
 *          dummy read is performed to start the process the same way as in I2cRead()).
 *
 *      Old details:
 *          This function is intended to be used to read from an I2C slave device.
 *          Allows the function to complete read with appropriate timeout set.
 *          If the timeout is 0, then this function blocks until read is complete.
 *
 * \note    This function is ONLY used when reading the computed MAC from the DS2465
 *          OneWire bus master chip. It is not required anywhere else.
 *
 * \param   pPacket    - Pointer to data packet.
 *
 * \return  I2C_STATUS - function execution status
 * \retval      I2C_STATUS_SUCCESS               - Configuration Accepted
 * \retval      I2C_STATUS_IDLE                  - Bus is Idle
 * \retval      I2C_STATUS_FAIL                  - Status Fail
 * \retval      I2C_STATUS_BUSY     -            - Bus is busy
 * \retval      I2C_STATUS_FAIL_CONFIG           - Configuration Failed
 * \retval      I2C_STATUS_FAIL_INVALID_PARAM    - Failed due to invalid parameter
 * \retval      I2C_STATUS_FAIL_NO_RESPONSE      - No response from device
 * \retval      I2C_STATUS_FAIL_TIMEOUT          - Request timed out
 *
 * ========================================================================== */
I2C_STATUS L3_I2cBurstRead(I2CDataPacket *pPacket)
{
    I2C_STATUS Status;
    do
    {
        if(NULL == pPacket)
        {
            Status = I2C_STATUS_FAIL_INVALID_PARAM;
            break;
        }
        Status = I2cTransfer(pPacket, I2C_TXN_READ_BURST);
        if (I2C_STATUS_SUCCESS  != Status )
        {
            Log(DEV,"Read failed, Address: 0x%X, Status: %d", pPacket->Address, Status);
        }
    } while (false);
    return Status;
}

/**
 * \}  <If using addtogroup above>
 */
#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

