#ifndef L4_ADAPTERDEFN_H
#define L4_ADAPTERDEFN_H

#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Signia_AdapterManager
 * \{

 * \brief   Public interface for Adapter Definitions.
 *
 * \details Symbolic types and constants are included with the interface prototypes
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L4_AdapterDefn.h
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include(s)                                     */
/******************************************************************************/
#include "Common.h"                      /*! Import common definitions such as types, etc */
#include "L4_DetachableCommon.h"
#include "Signia_CommManager.h"
#include "CirBuff.h"

/******************************************************************************/
/*                             Global Define(s) (Macros)                      */
/******************************************************************************/
#define ADAPTER_BAUD_RATE                       (129032u)      /*! Adapter baud rate */
#define ADAPTER_UART                            (UART0)        /*! UART used for Adapter communication */

#define FLASH_SECTOR_SIZE                       (0x400u)

#define BOOT_FLASH_START_ADDR                   (0x0000u)      /*! Bootloader goes here */
#define BOOT_FLASH_END_ADDR                     (0x17FFu)

#define DATA_FLASH_START_ADDR                   (0x1800u)      /*! Timestamps structure goes here */
#define DATA_FLASH_END_ADDR                     (0x1BFFu)

#define BOOT_FLASH_START_SECTOR                 (0u)
#define BOOT_FLASH_END_SECTOR                   ((BOOT_FLASH_END_ADDR + 1) / FLASH_SECTOR_SIZE)
#define DATA_FLASH_START_SECTOR                 (DATA_FLASH_START_ADDR / FLASH_SECTOR_SIZE)
#define DATA_FLASH_END_SECTOR                   ((DATA_FLASH_END_ADDR + 1) / FLASH_SECTOR_SIZE)

/* number of data bytes used in Data flash excluding the time stamps */
#define DATA_FLASH_CONFIG_BYTES                 (sizeof(AdapterFlashData) - sizeof(AdapterTimeStamps))
#define DATA_FLASH_ADDRESS_WIDTH                (4u)
#define DATA_FLASH_NUMBERBYTES_WIDTH            (2u)
#define DATA_FLASH_READ_CMD_DATA                (DATA_FLASH_ADDRESS_WIDTH)
#define DATA_FLASH_ADAPTER_TIMESTAMP_ADDRESS    (DATA_FLASH_START_ADDR)
#define DATA_FLASH_STRAIN_GAUGE_ADDRESS         (DATA_FLASH_START_ADDR + sizeof(AdapterTimeStamps))
#define DATA_FLASH_ADAPTER_PARAM_ADDRESS        (DATA_FLASH_STRAIN_GAUGE_ADDRESS + sizeof(StrainGauge_Flash))
#define DATA_FLASH_LOT_ADDRESS                  (DATA_FLASH_ADAPTER_PARAM_ADDRESS + sizeof(AdapterCalParams_Flash))
#define DATA_FLASH_BOARD_PARAM_ADDRESS          (DATA_FLASH_LOT_ADDRESS+ADAPTER_LOT_CHARS + FLASH_ITEM_CHECKSUM_SIZE)
#define DATA_FLASH_ADAPTER_DATA_START           (DATA_FLASH_STRAIN_GAUGE_ADDRESS)
#define FLASH_ITEM_CHECKSUM_SIZE                (4u)

#define GTIN_CHAR_COUNT                         (20u)
#define ADAPTER_LOT_CHARS                       (16u)

#define SG_STATUS_GOOD_DATA                     (0u)  /// \todo 05062022 KA: use a bitfield?
#define SG_STATUS_STALE_DATA                    (1u)
#define SG_STATUS_UNCALIBRATED_DATA             (2u)
#define SG_STATUS_OVER_MAX_ADC_DATA             (4u)
#define SG_STATUS_INVALID_TARE                  (8u)
#define SG_STATUS_ZERO_ADC_DATA                 (16u)
#define SG_STATUS_NULL_POINTER                  (32u)

#define SG_STATUS  uint16_t

#define ADAPTER_RX_BUFF_SIZE             (512u)                /*! Receive buffer size */
#define ADAPTER_TX_BUFF_SIZE             (512u)                /*! Transmit buffer size */

/******************************************************************************/
/*                             Global Type(s)                                 */
/******************************************************************************/

/* Codes returned by adapter to indicate if there is an error in
 * processing a command and the type of error */
typedef enum
{
    ADAPT_COMM_NOERROR,
    FLASH_ERASE_FAIL,
    FLASH_WRITE_PACKET_EMPTY,
    FLASH_WRITE_PACKET_ALIGN,
    FLASH_WRITE_ERASE_FAIL,
    FLASH_WRITE_TIMESTAMP_FAIL,
    FLASH_WRITE_DATA_PROG_FAIL,
    FLASH_WRITE_OVERLAP,
    FLASH_WRITE_PROG_FAIL,
    FLASH_READ_PACKET_EMPTY,
    FLASH_READ_PACKET_SIZE,
    SET_VERSION_ERASE_FAIL,
    SET_VERSION_SCRATCH_WRITE_FAIL,
    SET_VERSION_FSET_FAIL,
    SET_VERISON_TIMESTAMP_ERASE_FAIL,
    SET_VERSION_TIMESTAMP_PROG_FAIL,
    SET_VERSION_CRC_FAIL,
    SET_VERSION_CALRESTORE_FAIL,
    BOOT_ENTER_FAILURE,
    BOOT_QUIT_ERROR,
    BOOT_INVALID_APP_SECTOR_ERROR,
    PGA_COMMAND_FMT_ERROR,
    PGA_WRITE_FAIL,
    PGA_WRITE_EE_FAIL,
    PGA_READ_FAIL,
    REC_EE_COMMAND_FMT_ERROR,
    REC_EE_READ_FAIL,
    REC_EE_WRITE_FAIL,
} ADAPT_COMM_ERRORS;

typedef enum
{
    SWITCH_STATE_OPEN = 0,     ///< Switch open
    SWITCH_STATE_CLOSED,       ///< Switch Closed
    SWITCH_STATE_INBETWEEN,    ///< Switch not determined/error
    SWITCH_STATE_UNKNOWN,      ///< Switch initial/default
    SWITCH_STATE_COUNT         ///< Switch state count

} AdapterSwitchState;


typedef enum                         ///< Enum used to call the respective registered call back
{
   ADAPTER_APP_STRAIN_GAUGE_INDEX,   ///< Index to hold the strain guage based call back in array of function pointer
   ADAPTER_APP_RELOADSWITCH_INDEX,   ///< Index to hold the ReloadSwitch based call back in array of function pointer
   ADAPTER_APP_MAX                   ///< Max Index
} ADAPTER_APP_INDEX;

typedef enum
{
    ADAPTER_NO_COMMAND,
    ADAPTER_ENTERBOOT,
    ADAPTER_ENTERMAIN,
    ADAPTER_GET_VERSION,
    ADAPTER_GET_FLASHDATA,
    ADAPTER_GET_HWVERSION,
    ADAPTER_GET_TYPE,
    ADAPTER_ENABLE_ONEWIRE,
    ADAPTER_DISBALE_ONEWIRE,
    ADAPTER_ENABLE_SWEVENTS,
    ADAPTER_DISSABLE_SWENVENTS,
    ADAPTER_GET_EGIA_SWITCHDATA,
    ADAPTER_START_SGSTREAM,
    ADAPTER_STOP_SGSTREAM,
    ADAPTER_UPDATE_MAIN,
    ADAPTER_RESTART,
    ADAPTER_CMD_COUNT
} ADAPTER_COMMANDS;

typedef struct                 /*! Programmed Adapter firmware timestamps */
{
   uint32_t Checksum;          /*! Checksum of both adapter main and boot image */
   uint32_t TimeStampBoot;     /*! Adapter boot image timestamp */
   uint32_t TimeStampMain;     /*! Adapter Main application image timestamp */
} AdapterTimeStamps;

typedef struct                 /*! Strain gauge force value */
{
   uint16_t Current;           /*! Current value in ADC counts*/
   uint16_t Min;               /*! Minimum value */
   uint16_t Max;               /*! Maximum value */
   bool    NewDataFlag;        /*! indicates if new data is present*/
   float32_t ForceInLBS ;      /*! Force in LBS or Pounds */
   uint16_t Status;
} SG_FORCE;

typedef struct                 /*! Adapter Switch data */
{
  uint32_t            TimeStamp;
  AdapterSwitchState  State;
} SWITCH_DATA;

/*! Adapter types */
typedef enum
{
   ADAPTER_TYPE_EGIA,           /*! EGIA Adapter */
   ADAPTER_TYPE_EEA,            /*! EEA Adapter */
   ADAPTER_TYPE_NGSL,           /*! NGSL Adapter */
   ADAPTER_TYPE_UNKNOWN,        /*! Unknown Adapter */
   AM_TYPE_COUNT                /*! Adapter type count */
} ADAPTER_TYPE;

typedef void (*APP_CALLBACK_HANDLER)(void *pTemp);                            /*! App callback event handler function.*/
typedef AM_STATUS (*AM_DEFN_ENABLE_IF)(bool Enable);                          /*! Enable interface function */
typedef SG_STATUS (*AM_DEFN_SG_IF)(SG_FORCE *pForce);                         /*! Strain gauge interface function */
typedef AM_STATUS (*AM_DEFN_SWITCH_IF)(SWITCH_DATA *pSwitch);                 /*! Switch Data interface function */
typedef AM_STATUS (*AM_DEFN_FLASH_PARAM)(uint8_t *pFlashParam);                 /*! Switch Data interface function */

/// Adapter Interface functions
typedef struct
{
    MEM_LAYOUT_ADAPTER      Data;                              ///< Attributes
    AM_DEFN_EEP_UPDATE      Update;                            ///< Eeprom update interface function
    AM_STATUS               Status;                            ///< General access status code
    AM_DEFN_SWITCH_IF       pAdapterGetSwitchState;            ///< Send command to get switch state from adapter
    AM_DEFN_SG_IF           pGetStrainGaugeData;               ///< Get Strain gauge data
    AM_DEFN_IF              pForceTare;                        ///< Force Tare Interface
    AM_DEFN_IF              pForceLimitsReset;                 ///< Force limits reset Interface
    AM_DEFN_FLASH_PARAM     pGetFlashCalibParam;               ///< Read Adapter Flash calibration parameters
    AM_DEFN_IF              pSupplyON;
    AM_DEFN_IF              pSupplyOFF;
    AM_DEFN_STATUS_IF       pIsAdapComInProgress;

} AM_ADAPTER_IF;

typedef struct                 /*! UART message */
{
    ADAPTER_COMMANDS       Cmd;        /*! command */
    uint32_t              DelayInMsec;     /*! Inter command delay in ms*/
} ADAPTER_COM_MSG;

typedef struct                      /*! Adapter Communication Event*/
{
    QEvt                Event;          ///< QPC event header
    ADAPTER_COMMANDS     AdapterCmd;
} QEVENT_ADAPTERCOM;

typedef struct
{
    uint8_t  Buffer[ADAPTER_RX_BUFF_SIZE + MEMORY_FENCE_SIZE_BYTES];
    uint16_t CurrentSize;
    uint16_t FrameSize;
    bool     IsFramePartial;
}ADAPTER_RESPONSE;
/******************************************************************************/
/*                             Global Function Prototype(s)                   */
/******************************************************************************/
extern AM_STATUS AdapterDefnInit(void);
extern void ProcessAdapterUartResponse(COMM_MGR_EVENT Event);
extern AM_STATUS AdapterConnected(void);
extern void AdapterSetDeviceId(DEVICE_UNIQUE_ID DeviceAddress, uint8_t *pData);
extern void AdapterComm(void);
extern void InitAppHandler(void);
extern AM_STATUS L4_RegisterAdapterAppCallback(APP_CALLBACK_HANDLER pCallbackHandler, ADAPTER_APP_INDEX AppCallBackIndex);
extern AM_STATUS L4_AdapterComPostReq(ADAPTER_COM_MSG Msg);
extern bool IsAdapterPowered(void);
extern AM_STATUS RunAdapterComSM(void);
extern void L4_AdapterComSMReset(void);
extern AM_STATUS AdapterGetType(uint16_t *pAdapterType);
extern AM_STATUS AdapterDataFlashInitialize(void);

/**
 * \}
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

#endif /* L4_ADAPTERDEFN_H */

