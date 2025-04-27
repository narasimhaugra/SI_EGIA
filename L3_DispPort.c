#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_Disp
 * \{
 * \brief   Level 3 Display Controller Module
 *
 * \details This Module handles the initialization of Display interface:
 *          The functions contained in this module are wrappers to Micrium provided
 *          functions which are used to draw on the OLED display.
 *
 * \sa      K20 SubFamily Reference Manual.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_DispPort.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_DispPort.h"
#include "L3_GpioCtrl.h"
#include "Logger.h"
#include "FaultHandler.h"
#include "L3_GuiWidgets.h"

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER        (LOG_GROUP_DISPLAY) ///< Log Group Identifier
   
///< Defines for values of address used for write and read of data and command.
#define DISP_CMD_DEFAULT_ADDRESS             (0xC0100000u)
#define DISP_CMD_ADDRESS                     ((volatile uint8_t *)(DISP_CMD_DEFAULT_ADDRESS))
#define DISP_DATA_DEFAULT_ADDRESS            (0xC0100001u)
#define DISP_DATA_ADDRESS                     ((volatile uint8_t *)(DISP_DATA_DEFAULT_ADDRESS))
  
///< defines for reading and writing of command from oled controller.
#define DISP_CMD_IN(c)                       (c) = *(DISP_CMD_ADDRESS);
#define DISP_CMD_OUT(c)                      *(DISP_CMD_ADDRESS) = (c);
   
///< defines for reading and writing of data from oled controller.
#define DISP_DATA_IN(c)                      (c) = *(DISP_DATA_ADDRESS);
#define DISP_DATA_OUT(c)                     *(DISP_DATA_ADDRESS) = (c);
   
#define DISP_BASE_DEFAULT_ADDRESS            (0xC0100000u)
#define DISP_BASE_ADDRESS                    ((volatile uint8_t *)(DISP_BASE_DEFAULT_ADDRESS))
#define DISP_SIZE                            (0x7fu)      ///< Size of the display 128x128 
#define DISP_MAX_BUF_SIZE                    (0xc000u)   ///< Maximum Display Buffer Size 128*128*3 Bytes    

#define DISP_VOLT_SET_DELAY                   (25u)     ///< time for internal 3V to settle.
#define DISP_SET_LCD_RESET_DELAY              (10u)     ///< Delay for clearing the LCD Reset signal.
#define DISP_GPIO_SET_DELAY                   (100u)    ///< Time for the GPIO subsystem time to correct any errors that might have 
                                                        ///< occurred setting the bits on the GPIO expander.
   
///< Data values used for the Commands of SSD1351. 
///< Refer to L3_DispPortInit and data sheet of SSD1351 - OLED/PLED Segment/Common Driver with Controller
#define DISP_LOCK_VAL                         (0xb1u)
#define DISP_FUNCTION_SELECTION_VAL           (0x01u)
#define DISP_PHASE_LEN_VAL                    (0x53u)
#define DISP_CLKDIV_FREQ_VAL                  (0xe1u)
#define DISP_VSL_A_VAL                        (0xa0u)
#define DISP_VSL_B_VAL                        (0xb5u)
#define DISP_VSL_C_VAL                        (0x55u)
#define DISP_PRECHARGE_VOLTAGE_VAL            (0x00u)
#define DISP_MASTER_CONTRAST_VAL              (0x0eu)
#define DISP_MUX_RATIO                        (0x5fu)
   
///< Commands for SSD1351. Refer to data sheet of SSD1351 - OLED/PLED Segment/Common Driver with Controller
#define DISP_CMD_SET_COLUMN_ADDRESS           (0x15u) 
#define DISP_CMD_SET_ROW_ADDRESS              (0x75u) 
#define DISP_CMD_WR_RAM                       (0x5Cu) 
#define DISP_CMD_SET_DISP_MODE_NORMAL         (0xA6u) 
#define DISP_CMD_SET_FUNCTION_SELECTION       (0xABu) 
#define DISP_CMD_SET_SLEEP_MODE_DISP_OFF      (0xAEu) 
#define DISP_CMD_SET_SLEEP_MODE_DISP_ON       (0xAFu) 
#define DISP_CMD_SET_PHASE_LENGTH             (0xB1u) 
#define DISP_CMD_SET_CLKDIV_FREQ              (0xB3u) 
#define DISP_CMD_SET_SEGMENT_LOW_VOLTAGE      (0xB4u)
#define DISP_CMD_PRESET_LINEAR_LUT            (0xB9u)
#define DISP_CMD_SET_PRECHARGE_VOLTAGE        (0xBBu)
#define DISP_CMD_SET_VCOMH_VOLTAGE            (0xBEu)
#define DISP_CMD_MASTER_CONTRAST              (0xC7u)
#define DISP_CMD_SET_MUX_RATIO                (0xCAu)
#define DISP_CMD_SET_COMMAND_LOCK             (0xFDu)

///< Physical display size.
#define DISP_WIDTH                           (96u)
#define DISP_HEIGHT                          (96u)

#define BITS_PER_PIXEL                       (16u)      // No of bits used for representing a pixel.
#define MIN_RW_BYTES                         (1u)       // define for 1 byte, used while read/write of 1 byte.
#define MAX_RW_BYTES                         (255u)

#define LAYER_0                              (0u)       // define for memory layer 0.
#define MAX_LAYERS                           (1u)       // Max number of layers.

#define DISP_CMD_OFF                         (0xAEu)    // command to set the display off.
#define DISP_CMD_ON                          (0xAFu)    // command to set the display on.

///< Defines used for display the battery image.
#define BAT_X_POS                             (48u)
#define BAT_Y_POS                             (6u)
#define BAT_BORDER_WIDTH                      (2u)
#define BAT_X_SIDE_LEN                        (42u)
#define BAT_Y_SIDE_LEN                        (14u)
#define BAT_GREEN_Y_LEN                       (9u)
#define BAT_TIP_X_START                       (45u)
#define BAT_TIP_X_END                         (48u)
#define BAT_TIP_Y_START                       (9u)
#define BAT_TIP_Y_END                         (15u)

///< Defines used for display the WiFi bars.
#define WIFI_X_WIDTH                          (7u)
#define WIFI_Y_WIDTH                          (2u)
#define WIFI_X1                               (8u)
#define WIFI_Y1                               (10u)
#define WIFI_X2                               (12u)
#define WIFI_Y2                               (11u)

#define ARROW_NUM_PTS                         (7u)              // Number of points used for drawing the Arrow.

///< Status values used for display driver callback routine.
#define DISP_DRV_INVALID_CMD                  (-1)
#define DISP_DRV_INIT_FAIL                    (-2)

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/
typedef int64_t   DISP_MEMDEV_HANDLE;               ///< Memory Handle used for drawing operations.

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/
static DISP_MEMDEV_HANDLE L3DispMemdevice;          ///< Variable used to hold the memory handle used for drawing on the display.
static bool DispPortInitialized = false;           ///< Flag to indicate if Display is initialized
static BITMAP *Welcome_Screen[10] =
{
    (BITMAP*) _animinwc01, 
    (BITMAP*) _animinwc02,
    (BITMAP*) _animinwc03, 
    (BITMAP*) _animinwc04, 
    (BITMAP*) _animinwc05, 
    (BITMAP*) _animinwc06, 
    (BITMAP*) _animinwc07, 
    (BITMAP*) _animinwc08, 
    (BITMAP*) _animinwc09, 
    (BITMAP*) _animinwc10
};
static GUI_WIDGET_IMAGE WelcomeImage;                     ///< Welcome Static Images to display

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/
static void DispPortClearRAM(void);
static DISP_PORT_STATUS L3_DispPortInit(DISP_INIT_MODE InitMode);

/******************************************************************************/
/*                             Local Function(s)                              */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief   Configures chip select registers for Display 
 *
 * \details This function configures the chip selct address register(CSAR)
 *          chip select control register(CSMR) and chip select mask register(CSMR)
 *          for the chip-select (5) associated with Display.
 *
 * \param   < None >
 * \return  DISP_PORT_STATUS Display Set registers status
 * \retval  DISP_PORT_STATUS_OK - Register configuration for display is successful.
 * \retval  DISP_PORT_STATUS_ERROR - Error occured while configuring the display registers.
 *
 * ==========================================================================*/
static DISP_PORT_STATUS DispPortSetRegisters(void)
{
    DISP_PORT_STATUS Status;

    Status = DISP_PORT_STATUS_OK;

    do
    {
        // Configuration of Chip Select 5 for Display interface.
        FB_CSAR5 = (INT32U)DISP_BASE_ADDRESS;
        FB_CSMR5 = FB_CSMR_BAM(0x0001) |    // Set base address mask for 64K address space.
                   FB_CSMR_V_MASK;          // Enable CS Signal.

        FB_CSCR5 = FB_CSCR_SWS(0)   |       // Secondary wait states.
                   FB_CSCR_ASET(1)  |       // Address setup.
                   FB_CSCR_RDAH(0)  |       // Read address hold.
                   FB_CSCR_WRAH(0)  |       // Write address hold.
                   FB_CSCR_WS(8)    |       // Wait States 
                   FB_CSCR_BLS_MASK |       // Byte-lane Shift 
                   FB_CSCR_AA_MASK  |       // auto-acknowledge 
                   FB_CSCR_PS(1);           // 8-bit port size 

        if (GPIO_STATUS_OK != L3_GpioCtrlSetSignal(GPIO_EN_3V))
        {
            Status = DISP_PORT_STATUS_ERROR;
            Log(ERR, "DispPortSetRegisters: Gpio Set Signal Failed");
            break;
        }  /*else not needed */

        OSTimeDly(DISP_VOLT_SET_DELAY);                      // Give time for internal 3V to settle.

        if (GPIO_STATUS_OK != L3_GpioCtrlClearSignal(GPIO_LCD_RESET))
        {
            Status = DISP_PORT_STATUS_ERROR;
            Log(ERR, "DispPortSetRegisters: Gpio Clear LCD Reset Signal Failed");
            break;
        }  /*else not needed */

        OSTimeDly(DISP_GPIO_SET_DELAY);                      // Delay for clearing the LCD Reset signal.

        if (GPIO_STATUS_OK != L3_GpioCtrlSetSignal(GPIO_LCD_RESET))
        {
            Status = DISP_PORT_STATUS_ERROR;
            Log(ERR, "DispPortSetRegisters: Gpio Set LCD Reset Signal Failed");
            break;
        }  /*else not needed */

        OSTimeDly(DISP_SET_LCD_RESET_DELAY);    // Give the GPIO subsystem time to correct any errors that might have occured setting the bits 
                                            // on the GPIO expander.
    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief     Initialize Display interface
 *
 * \details - This routine is used to initialize or re-initialize the
 *            OLED controller. When the initMode is set to DISP_INIT this function
 *            sets the port pins to their initial values.
 *
 * \param   InitMode - This param is used to control whether to initialize 
 *                     or re-initialize the Display interface.
 * \return  DISP_PORT_STATUS - Display initialization status
 * \retval  DISP_PORT_STATUS_OK - Display Controller initialization successful.
 * \retval  DISP_PORT_STATUS_INVALID_STATE - Failed to initialize because of invalid Display state
 *
 * ========================================================================== */
DISP_PORT_STATUS L3_DispPortInit(DISP_INIT_MODE InitMode)
{
    DISP_PORT_STATUS Status;

    Status = DISP_PORT_STATUS_OK;

    do
    {
        if ((DISP_REINIT == InitMode) && (!DispPortInitialized))
        {
            // Don't re-init unless init has completed
            Status = DISP_PORT_STATUS_INVALID_STATE;
            Log(ERR, "L3_DispPortInit: Set registers failed");
            break;
        }  /*else not needed */

        if ( DISP_INIT == InitMode )
        {
            if (DISP_PORT_STATUS_OK != DispPortSetRegisters())
            {
                // Error setting register values.
                Status = DISP_PORT_STATUS_ERROR;
                Log(ERR, "L3_DispPortInit: Set registers failed");
                break;
            }   /*else not needed */
        }  /*else not needed */

        // Start of Drive IC initialization.
        DISP_CMD_OUT(DISP_CMD_SET_COMMAND_LOCK);              // Set Command Lock.
        DISP_DATA_OUT(DISP_LOCK_VAL);                         // Unlock OLED driver IC.

        DISP_CMD_OUT(DISP_CMD_SET_SLEEP_MODE_DISP_OFF);       // Display off.

        DISP_CMD_OUT(DISP_CMD_SET_DISP_MODE_NORMAL);          // Normal display.

        DISP_CMD_OUT(DISP_CMD_SET_FUNCTION_SELECTION);        // Function Selection.
        DISP_DATA_OUT(DISP_FUNCTION_SELECTION_VAL);           // 16 bit data bus, 8080 interface, internal Vdd regulator.

        DISP_CMD_OUT(DISP_CMD_SET_PHASE_LENGTH);              // Set Reset (Phase 1) /Pre-charge (Phase 2) period.
        DISP_DATA_OUT(DISP_PHASE_LEN_VAL);

        DISP_CMD_OUT(DISP_CMD_SET_CLKDIV_FREQ);               // Set frame rate.
        DISP_DATA_OUT(DISP_CLKDIV_FREQ_VAL);                  // 105Hz.

        DISP_CMD_OUT(DISP_CMD_SET_SEGMENT_LOW_VOLTAGE);       // External VSL.
        DISP_DATA_OUT(DISP_VSL_A_VAL);
        DISP_DATA_OUT(DISP_VSL_B_VAL);
        DISP_DATA_OUT(DISP_VSL_C_VAL);

        DISP_CMD_OUT(DISP_CMD_PRESET_LINEAR_LUT);              // Use Built-in Linear LUT.

        DISP_CMD_OUT(DISP_CMD_SET_PRECHARGE_VOLTAGE);          // Set Pre-charge voltage.
        DISP_DATA_OUT(DISP_PRECHARGE_VOLTAGE_VAL);

        DISP_CMD_OUT(DISP_CMD_SET_VCOMH_VOLTAGE);

        DISP_CMD_OUT(DISP_CMD_MASTER_CONTRAST);                // Master current control.
        DISP_DATA_OUT(DISP_MASTER_CONTRAST_VAL);

        DISP_CMD_OUT(DISP_CMD_SET_MUX_RATIO);                  // Set MUX Ratio.
        DISP_DATA_OUT(DISP_MUX_RATIO);                                   // 96 Duty.

        if (DISP_INIT == InitMode)
        {
            DispPortClearRAM();                               // Clear the whole DDRAM.

            if (GPIO_STATUS_OK != L3_GpioCtrlSetSignal(GPIO_EN_VDISP))    // Turn on 14V rail.
            {
                // Error with GPIO Set Signal
                Status = DISP_PORT_STATUS_ERROR;
                break;
            }   /*else not needed */

            // Give the GPIO subsystem time to correct any errors that might have occurred setting the bits
            // on the GPIO expander.
            OSTimeDly(DISP_GPIO_SET_DELAY);
        } /*else not needed */

        DISP_CMD_OUT(DISP_CMD_SET_SLEEP_MODE_DISP_ON);          // Sleep mode off (display on).

        DispPortInitialized = true;

    } while (false);

    return Status;
 }

/* ========================================================================== */
/**
 * \brief   Write command to the Display interface.
 *
 * \details This is wrapper function which internally calls the display interface
 *          API to write the command to the display interface.
 *
 * \param   Command - command to write onto the display interface.
 *
 * \return  None
 *
 * ========================================================================== */
static void DispWriteCommand(uint8_t Command)
{
    if (DispPortInitialized)
    {    	
        DISP_CMD_OUT(Command);
    }
}

/* ========================================================================== */
/**
 * \brief   Write Nbytes commands to the Display interface.
 *
 * \details This is wrapper function which internally calls the display interface
 *          API to write the commands to the display interface.
 *
 * \param   pData - Pointer to command data to write.
 * \param   Nbytes - Number of commands to write
 *
 * \return  None
 *
 * ========================================================================== */
static void DispWriteMultiCommand(uint8_t *pData, int32_t Nbytes)
{
    if (DispPortInitialized && (MAX_RW_BYTES >= Nbytes ))
    {
      	while (Nbytes)
        {
           DISP_CMD_OUT(*pData);
           pData++;
           Nbytes--;
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Read command from the  Display interface.
 *
 * \details This is wrapper function which internally calls the display interface
 *          API to read the command from the display interface.
 *
 * \param   < None >
 *
 * \return  uint8_t - Command read from the display interface
 *
 * ========================================================================== */
static uint8_t DispReadCommand(void) //To do: JA 21-10-21: Based on usage this function might be required to return a failure status
{
    uint8_t Command;

    /* Initialize the variables */
    Command = 0u;

    if (DispPortInitialized)
    {
        DISP_CMD_IN(Command);
    }

    return Command;
}

/* ========================================================================== */
/**
 * \brief   Read Nbytes commands from the  Display interface.
 *
 * \details This is wrapper function which internally calls the display interface
 *          API to read the commands from the display interface.
 *
 * \param   pData - Pointer to buffer to store commands read from Display
 * \param   Nbytes - Number of commands to read
 *
 * \return  None
 *
 * ========================================================================== */
static void DispReadMultiCommand(uint8_t *pData, int32_t Nbytes) //To do: JA 21-10-21: Based on usage this function might be required to return a failure status
{
    if (DispPortInitialized && (MAX_RW_BYTES >= Nbytes))
    {
        while (Nbytes)
        {
            DISP_CMD_IN(*pData);
            pData++;
            Nbytes--;
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Write data to the Display interface.
 *
 * \details This is wrapper function which internally calls the display interface
 *          API to write the data to the display interface.
 *
 * \param   Data - Data to write to the display interface.
 *
 * \return  None
 *
 * ========================================================================== */
static void DispWriteData(uint8_t Data)
{
    if (DispPortInitialized)
    {
        DISP_DATA_OUT(Data);
    }
}

/* ========================================================================== */
/**
 * \brief   Write Nbytes data to the Display interface.
 *
 * \details This is wrapper function which internally calls the display interface
 *          API to write the data to the display interface.
 *
 * \param   pData - Pointer to data to write.
 * \param   Nbytes - Number of bytes of data to write
 *
 * \return  None
 *
 * ========================================================================== */
static void DispWriteMultiData(uint8_t *pData, int32_t Nbytes)
{
    if (DispPortInitialized && (MIN_RW_BYTES <= Nbytes))
    {
        while (Nbytes)
        {
             DISP_DATA_OUT(*pData);
             pData++;
             Nbytes--;
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Read data from the Display interface.
 *
 * \details This is wrapper function which internally calls the display interface
 *          API to read the data from the display interface.
 *
 * \param   < None >
 *
 * \return  uint8_t - data read from the display interface
 *
 * ========================================================================== */
static uint8_t DispReadData(void) //To do: JA 21-10-21: Based on usage this function might be required to return a failure status
{
    uint8_t ReadData;

    ReadData = 0u;

    if (DispPortInitialized)
    {
        DISP_DATA_IN(ReadData);  
    }

    return ReadData;
}

/* ========================================================================== */
/**
 * \brief   Read Nbytes data from the Display interface.
 *
 * \details This is wrapper function which internally calls the display interface
 *          API to read the data from the display interface.
 *
 * \param   pData - Pointer to buffer to store data read from Display
 * \param   Nbytes - Number of data to read
 *
 * \return  None
 *
 * ========================================================================== */
static void DispReadMultiData(uint8_t *pData, int32_t Nbytes) //To do: JA 21-10-21: Based on usage this function might be required to return a failure status
{
    if (DispPortInitialized && (MIN_RW_BYTES <= Nbytes))
    {
        while (Nbytes)
        {     
             DISP_DATA_IN(*pData);  
             pData++;
             Nbytes--;
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Welcome screen
 *
 * \details Setting the colour to white for 500ms and read backs the color
 *          and check with what is requested to display
 *
 * \param   ScreenNo - Welcome Static Screen id
 *
 * \return  DISP_PORT_STATUS Screen Display Status
 * \retval  DISP_PORT_STATUS_SUCCESS - Display Updated successful.
 * \retval  DISP_PORT_STATUS_ERROR - Error while Updating.
 *
 * ========================================================================== */
DISP_PORT_STATUS L3_WelcomeStaticScreen(uint8_t ScreenNo)
{
    uint32_t ColorData;

    /*Default Succes for Image Objects*/
    ColorData=DISP_COLOR_WHITE;

    /*Turn ON Display */
    L3_DisplayOn(true);

    /* Clear the display */
    L3_DispClear();

    if (0 == ScreenNo)
    {
        /* Set color white */
        L3_DispSetColor(DISP_COLOR_WHITE);

        /* Fill the entire OLED Display with WHITE color */
        L3_DispFillRect(DISPXPOS,DISPXPOS,DISPYPOS,DISPYPOS);

        /* Copy the content to LCD */
        L3_DispMemDevCopyToLCD();      

        /* Read back the color */
        ColorData = GUI_GetColor();
    }
    else
    {
        WelcomeImage = (GUI_WIDGET_IMAGE){0, 0, 96, 96,(BITMAP*)Welcome_Screen[ScreenNo]};
        L3_WidgetImageDraw(&WelcomeImage);

        /* Copy the content to LCD */
        L3_DispMemDevCopyToLCD();  
    }

    if ( DISP_COLOR_WHITE != ColorData )
    {
        /* Self test failed. Inform Fault Handler */
        FaultHandlerSetFault(PERMFAIL_OLEDSELFTEST, SET_ERROR);
    }
    return DISP_PORT_STATUS_OK;
}

/******************************************************************************/
/*                             Global Function(s)                             */
/******************************************************************************/
/* ========================================================================== */
/**
 * \brief     Initialize Display Abstraction interface
 *
 * \details - This routine initializes μC/GUI internal data structures and variables.
 *
 * \param   < None >
 * 
 * \return  DISP_PORT_STATUS - Display abstraction initialization status.
 * \retval  DISP_PORT_STATUS_SUCCESS - Display abstraction initialization successful.
 * \retval  DISP_PORT_STATUS_ERROR - Error while initializing.
 *
 * ========================================================================== */
DISP_PORT_STATUS L3_DispInit(void)
{
    DISP_PORT_STATUS Status = DISP_PORT_STATUS_ERROR;
    static bool DispPortInitialized = false; 

    do
    {
        if (DispPortInitialized)
        {
            Status = DISP_PORT_STATUS_OK;
           //  break;
        }

        if (DISP_PORT_STATUS_OK == GUI_Init())          // Calling the Micrium API to initialize the internal data structures & variables.
        {
            (void)GUI_SelectLayer(LAYER_0);             // Calling the Micrium API to Select the layer for display operations.

             // Calling the Micrium API to create the Memory device and return the handle to the memory device.
            L3DispMemdevice = GUI_MEMDEV_CreateEx(0, 0, DISP_WIDTH, DISP_HEIGHT, GUI_MEMDEV_NOTRANS);

            if (0 == L3DispMemdevice) // Failed to create the handle return with Error
            {
                break;
            }

            (void)GUI_MEMDEV_Select(L3DispMemdevice);   // Selecting the Memory device for drawing operations.
            Status = L3_WelcomeStaticScreen(0);
            Status = DISP_PORT_STATUS_OK;
            DispPortInitialized = true;
        } /*else not needed */

    } while (false);

    return Status;
}

/* ========================================================================== */
/**
 * \brief   This function Sets the font to be used for text output.
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to set the font to be used for text output.
 *
 * \param   Font - font value to be set.
 *
 * \return  FONT_TYPE - value of old font value so that it may be buffered 
 *
 * ========================================================================== */
FONT_TYPE L3_DispSetFont(FONT_TYPE Font)
{
    static FONT_TYPE OldFont;
    static FONT_TYPE NewFont;

    OldFont = NewFont;
    NewFont = Font;

    switch(Font)
    {
        case FONT_13B_1:
            GUI_SetFont(GUI_FONT_13B_1);
            break;

        case FONT_20B_1:
            GUI_SetFont(GUI_FONT_20B_1); 
            break;

        default:
            break;
    }

    return OldFont;
}

/* ========================================================================== */
/**
 * \brief   This function Sets the pen size to be used for drawing operations.
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to set the pen size to be used for further drawing operations.
 *
 * \param   PenSize - Pen size in pixels to be used.
 *
 * \return  uint8_t - value of previous pen size.
 *
 * ========================================================================== */
uint8_t L3_DispSetPenSize(uint8_t PenSize)
{
    uint8_t OldPenSize;

    OldPenSize = GUI_SetPenSize(PenSize);

    return OldPenSize;
}

/* ========================================================================== */
/**
 * \brief   This function Sets the current foreground color.
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to set the current foreground color.
 *
 * \param   Color - Color for foreground.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispSetColor(uint32_t Color)
{
    GUI_SetColor(Color);
}

/* ========================================================================== */
/**
 * \brief   This function Sets the current background color.
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to set the current background color.
 *
 * \param   Color - Color for background.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispSetBkColor(uint32_t Color)
{
    GUI_SetBkColor(Color);
}

/* ========================================================================== */
/**
 * \brief   This function Sets the current text write position.
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to set the current text write position.
 *
 * \param   X - New X-position (in pixels, 0 is left border).
 * \param   Y - New Y-position (in pixels, 0 is top border).
 *
 * \return  int8_t - Usually 0. If a value != 0 is returned, then the current text position 
 *          is outside of the window (to the right or below), so a following 
 *          write operation can be omitted. 
 *
 * ========================================================================== */
int8_t L3_DispGotoXY(int8_t X, int8_t Y)
{
    int8_t Retval;

    Retval = (int8_t) GUI_GotoXY((int32_t)X, (int32_t)Y);

    return Retval;
}

/* ========================================================================== */
/**
 * \brief   This function displays a single character at the current text 
 *          position in the current window using the current font.
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to display a single character at the current text position in the 
 *          current window using the current font
 *
 * \param   Char - Character to display
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispChar(uint8_t Char)
{
    GUI_DispChar((uint16_t)Char);
}

/* ========================================================================== */
/**
 * \brief   This function displays the string passed as parameter at the 
 *          current text position in the current window using the current font. 
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to display the string passed as parameter at the current text
 *          position in the current window using the current font
 *
 * \param   pString - pointer to String to display
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispString(int8_t const *pString)
{
    if (NULL != pString)
    {
        GUI_DispString((char const *)pString);
    }
}

/* ========================================================================== */
/**
 * \brief   This function clears the active window (or entire display if 
 *          background is the active window)
 *
 * \details This is wrapper function which internally calls the Micrium API to clear
 *          the active window (or entire display if background is the active window)
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispClear(void)
{
    L3_DispSetBkColor(DISP_COLOR_BLACK);
    GUI_Clear();
}

/* ========================================================================== */
/**
 * \brief   This function draws a line.
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to draw a line from a specified starting point to a specified
 *          endpoint in the current window (absolute coordinates).
 *
 * \param   X1 - X-starting position.
 * \param   Y1 - Y-starting position.
 * \param   X2 - X-end position.
 * \param   Y2 - Y-end position.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispDrawLine(int8_t X1, int8_t Y1, int8_t X2, int8_t Y2)
{
    GUI_DrawLine((int32_t)X1, (int32_t)Y1, (int32_t)X2, (int32_t)Y2);
}

/* ========================================================================== */
/**
 * \brief  This function draws a horizontal line.
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to draw a horizontal line from a specified starting point to a
 *          specified endpoint in the current window.
 *
 * \param   Y  - Y-position.
 * \param   X1 - X-starting position.
 * \param   X2 - X-end position
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispDrawHLine(int8_t Y, int8_t X1, int8_t X2)
{
    GUI_DrawHLine((int32_t)Y, (int32_t)X1, (int32_t)X2);
}

/* ========================================================================== */
/**
 * \brief  This function draws a vertical line.
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to draw a vertical line from a specified starting point to a
 *          specified endpoint in the current window.
 *
 * \param   X  - X-position.
 * \param   Y1 - Y-starting position.
 * \param   Y2 - Y-end position
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispDrawVLine(int8_t X, int8_t Y1,int8_t Y2)
{
    GUI_DrawVLine((int32_t)X, (int32_t)Y1, (int32_t)Y2);
}

/* ========================================================================== */
/**
 * \brief   This function draws a rectangle.
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to draw a rectangle at a specified position in the current window.
 *
 * \param   X1 - Upper left X-position.
 * \param   Y1 - Upper left Y-position.
 * \param   X2 - Lower right X-position.
 * \param   Y2 - Lower right X-position.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispDrawRect(int8_t X1, int8_t Y1, int8_t X2, int8_t Y2)
{
    GUI_DrawRect((int32_t)X1, (int32_t)Y1, (int32_t)X2, (int32_t)Y2);
}

/* ========================================================================== */
/**
 * \brief   This function draws a filled rectangle.
 *
 * \details This is wrapper function which internally calls the Micrium API to 
 *          draw a filled rectangle at a specified position in the current window.
 *
 * \param   X1 - Upper left X-position.
 * \param   Y1 - Upper left Y-position.
 * \param   X2 - Lower right X-position.
 * \param   Y2 - Lower right X-position.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispFillRect(int8_t X1,int8_t Y1,int8_t X2,int8_t Y2)
{
    GUI_FillRect((int32_t)X1, (int32_t)Y1, (int32_t)X2, (int32_t)Y2);
}

/* ========================================================================== */
/**
 * \brief   This function clears a rectangle.
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to clear a rectangle area at a specified position in the current 
 *          window by filling it with the background color.
 *
 * \param   X1 - Upper left X-position.
 * \param   Y1 - Upper left Y-position.
 * \param   X2 - Lower right X-position.
 * \param   Y2 - Lower right X-position.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispClearRect(int8_t X1, int8_t Y1, int8_t X2, int8_t Y2)
{
    GUI_ClearRect((int32_t)X1, (int32_t)Y1, (int32_t)X2, (int32_t)Y2);
}

/* ========================================================================== */
/**
 * \brief   This function draws the outline of a circle.
 *
 * \details This is wrapper function which internally calls the Micrium API
 *          to draw the outline of a circle of specified dimensions, at a 
 *          specified position in the current window.
 *
 * \param   X1 - X-position of the center of the circle in pixels of the client window.
 * \param   Y1 - Y-position of the center of the circle in pixels of the client window.
 * \param   Radius  - radius of the circle (half the diameter). Must be a positive value.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispDrawCircle(int8_t X1, int8_t Y1, int8_t Radius)
{
    GUI_DrawCircle((int32_t)X1, (int32_t)Y1, (int32_t)Radius);
}

/* ========================================================================== */
/**
 * \brief   This function draws a filled circle.
 *
 * \details This is wrapper function which internally calls the Micrium API to
 *          draw a filled circle of specified dimensions, at a specified position
 *          in the current window.
 *
 * \param   X1 - X-position of the center of the circle in pixels of the client window.
 * \param   Y1 - Y-position of the center of the circle in pixels of the client window.
 * \param   Radius  - radius of the circle (half the diameter). Must be a positive value.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispFillCircle(int8_t X1, int8_t Y1, int8_t Radius)
{
    GUI_FillCircle((int32_t)X1, (int32_t)Y1, (int32_t)Radius);
}

/* ========================================================================== */
/**
 * \brief   This function draws a bitmap image.
 *
 * \details This is wrapper function which internally calls the Micrium API to
 *          draw a bitmap image at a specified position in the current window.
 *
 * \param   pDispBmp - Pointer to the bitmap to display.
 * \param   X - X-position of the upper left corner of the bitmap in the display.
 * \param   Y - Y-position of the upper left corner of the bitmap in the display.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispDrawBitmap(Disp_Bitmap *pDispBmp, int8_t X, int8_t Y)
{
    GUI_BITMAP GuiBmp;

    if ((NULL!= pDispBmp) && (NULL!= pDispBmp->pData))
    {
        GuiBmp.XSize = pDispBmp->Width;
        GuiBmp.YSize = pDispBmp->Height;
        GuiBmp.pData = pDispBmp->pData;
        GuiBmp.BitsPerPixel = BITS_PER_PIXEL;
        GuiBmp.BytesPerLine = 2*pDispBmp->Width;
        GuiBmp.pPal = NULL;

        switch(pDispBmp->DrawMethod)
        {
            case DISP_BITMAP_DRAW_METHOD_RLE16:
                 GuiBmp.pMethods = GUI_DRAW_RLE16;
                 break;

            case DISP_BITMAP_DRAW_METHOD_RLEM16:
                 GuiBmp.pMethods = GUI_DRAW_RLEM16; 
                 break;  

            case DISP_BITMAP_DRAW_METHOD_RLE8:
                 GuiBmp.pMethods = GUI_DRAW_RLE8; 
                 break; 

            default:
                 GuiBmp.pMethods = GUI_DRAW_RLE16;
                 break;  
        }   

        GUI_DrawBitmap(&GuiBmp, (int32_t)X, (int32_t)Y);
    } /* else not needed */
}

/* ========================================================================== */
/**
 * \brief   This function draws the borders of a Rectangle defined by its sides 
 *          length with xsideLength, ysideLength.
 *
 * \param   Color - Color of border.
 * \param   X - upper left x position of square corner.
 * \param   Y - upper left y position of square corner.
 * \param   BorderWidth - width of the square border.
 * \param   XLength - Legth of the rectangle.
 * \param   YLength - Width of the rectangle.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispDrawRectBorders(uint32_t Color, int8_t X, int8_t Y, int8_t BorderWidth, int8_t XLength, int8_t YLength)
{
    uint8_t XSideLength = X + XLength - 1;
    uint8_t XBorderWidth= X + BorderWidth - 1;
    uint8_t YSideLength = Y + YLength - 1;
    uint8_t YBorderWidth= Y + BorderWidth - 1;

    L3_DispSetColor(Color);

    L3_DispFillRect(X,Y,XSideLength,YBorderWidth);
    L3_DispFillRect(X,Y,XBorderWidth,YSideLength);
    L3_DispFillRect(X, YSideLength - BorderWidth + 1, XSideLength, YSideLength); 
    L3_DispFillRect(XSideLength - BorderWidth + 1, Y, XSideLength, YSideLength); 
}

/* ========================================================================== */
/**
 * \brief   This function draws the animation from a given set of bitmap array.
 *
 * \param   pAnimation - pointer to Structure which has elements required for drawing the
 *                       sequence of bitmaps.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispDrawAnimation(Disp_Animation *pAnimation)
{
    Disp_AnimationBitmap *pNextBitmap;
    uint8_t BitmapIndex;

    if ((NULL!= pAnimation) && (NULL!= pAnimation->pBitmapArray) && (0 < pAnimation->BitmapCount))
    {
        for (BitmapIndex = 0; BitmapIndex < pAnimation->BitmapCount; BitmapIndex++)
        {
            pNextBitmap = pAnimation->pBitmapArray[BitmapIndex];

            if (NULL!=pNextBitmap)
            {
                L3_DispDrawBitmap(pAnimation->pBitmapArray[BitmapIndex]->pBitmap,
                               pNextBitmap->Location.x,
                               pNextBitmap->Location.y);
                L3_DispMemDevCopyToLCD();
                OSTimeDly(pAnimation->pFrameTimeArray[BitmapIndex]);
            }
        }
    }
}

/* ========================================================================== */
/**
 * \brief   This function draws the battery levels on the display.
 *
 * \param   Color - Color used to fill the battery level.
 * \param   Level - Battery level.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispDrawBattery(uint32_t Color, DISP_BAT_LEVEL Level)
{
    uint8_t FillWidth[]= { 42, 36, 31, 26, 22, 22, 16, 16, 11, 4 };

    if (DISP_BAT_LEVEL_LAST > Level)
    {
        L3_DispDrawRectBorders(DISP_COLOR_WHITE,BAT_X_POS,BAT_Y_POS,BAT_BORDER_WIDTH,BAT_X_SIDE_LEN,BAT_Y_SIDE_LEN);  
        L3_DispSetColor(Color);
        L3_DispFillRect(BAT_X_POS - BAT_BORDER_WIDTH + FillWidth[Level] , BAT_Y_POS + BAT_BORDER_WIDTH + 1, BAT_X_POS - BAT_BORDER_WIDTH - 2 + BAT_X_SIDE_LEN, BAT_GREEN_Y_LEN + BAT_Y_POS - 1+ BAT_BORDER_WIDTH);
        L3_DispSetColor(DISP_COLOR_WHITE);
        L3_DispFillRect(BAT_TIP_X_START, BAT_TIP_Y_START, BAT_TIP_X_END, BAT_TIP_Y_END);
    }
}

/* ========================================================================== */
/**
 * \brief   This function draws the Arrow filled with color parameter at the 
 *          position (x,y). 
 *
 * \param   Color - Color used to draw the Arrow.
 * \param   X - X-position to display string.
 * \param   Y - Y-position to display string.
 * \param   *pDispPoint - pointer to Direction of the arrow (up/down/left/right).
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispDrawArrow(uint32_t Color, int8_t X, int8_t Y, DISP_POINT *pDispPoint)
{
    if (NULL != pDispPoint)
    {
        GUI_SetColor(Color);
        GUI_FillPolygon(&pDispPoint[0], ARROW_NUM_PTS, (int32_t)X, (int32_t)Y);
    }
}

/* ========================================================================== */
/**
 * \brief   This function draws the Wifi Connection Level. 
 *          
 * \param   Color - Color used to fill the Wifi bars.
 * \param   Level - Wifi Level.
 * \param   XPos - upper left x position of Wifi bars.
 * \param   YPos - upper left y position of Wifi bars.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispDrawWifiBars(uint32_t Color, DISP_WIFI_LEVEL Level, int8_t XPos, int8_t YPos)
{
    switch(Level)
    {
        case DISP_WIFI_LEVEL_25:
            L3_DispRectFillColor(Color,XPos + WIFI_X1,YPos + WIFI_Y1,XPos + WIFI_X2,YPos + WIFI_Y2);  
            break;

        case DISP_WIFI_LEVEL_50:
            L3_DispRectFillColor(Color,XPos + WIFI_X1,YPos + WIFI_Y1,XPos + WIFI_X2,YPos + WIFI_Y2);  
            L3_DispRectFillColor(Color,XPos + WIFI_X1 + WIFI_X_WIDTH,YPos + WIFI_Y1 - WIFI_Y_WIDTH,XPos + WIFI_X2 + WIFI_X_WIDTH,YPos + WIFI_Y2);
            break;

        case DISP_WIFI_LEVEL_75:
            L3_DispRectFillColor(Color,XPos + WIFI_X1,YPos + WIFI_Y1,XPos + WIFI_X2,YPos + WIFI_Y2);  
            L3_DispRectFillColor(Color,XPos + WIFI_X1 + WIFI_X_WIDTH,YPos + WIFI_Y1 - WIFI_Y_WIDTH,XPos + WIFI_X2 + WIFI_X_WIDTH,YPos + WIFI_Y2);
            L3_DispRectFillColor(Color,XPos + WIFI_X1 + 2*WIFI_X_WIDTH,YPos + WIFI_Y1 - 2*WIFI_Y_WIDTH,XPos + WIFI_X2 + 2*WIFI_X_WIDTH,YPos + WIFI_Y2);  
            break;

        case DISP_WIFI_LEVEL_100:
            L3_DispRectFillColor(Color,XPos + WIFI_X1,YPos + WIFI_Y1,XPos + WIFI_X2,YPos + WIFI_Y2);
            L3_DispRectFillColor(Color,XPos + WIFI_X1 + WIFI_X_WIDTH,YPos + WIFI_Y1 - WIFI_Y_WIDTH,XPos + WIFI_X2 + WIFI_X_WIDTH,YPos + WIFI_Y2);
            L3_DispRectFillColor(Color,XPos + WIFI_X1 + 2*WIFI_X_WIDTH,YPos + WIFI_Y1 - 2*WIFI_Y_WIDTH,XPos + WIFI_X2 + 2*WIFI_X_WIDTH,YPos + WIFI_Y2);
            L3_DispRectFillColor(Color,XPos + WIFI_X1 + 3*WIFI_X_WIDTH,YPos + WIFI_Y1 - 3*WIFI_Y_WIDTH,XPos + WIFI_X2 + 3*WIFI_X_WIDTH,YPos + WIFI_Y2);
            break;

        default:
            break;
    }   
}

/* ========================================================================== */
/**
 * \brief   This function displays the string passed as parameter at the 
 *          position (x,y) using the font value (Font) and Color (FgColor).
 *
 * \param  Font - Font to be used for displaying the string.
 * \param  FgColor - Foreground Color to be used for displaying the string.
 * \param  BkColor - Background Color to be used 
 * \param  X - X-position to display string.
 * \param  Y - Y-position to display string.
 * \param  pString - pointer to String to display
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispStringAtXY(FONT_TYPE Font, uint32_t FgColor, uint32_t BkColor, int8_t X, int8_t Y, const int8_t *pString)
{
    if (NULL != pString)
    {
        L3_DispSetFont(Font);
        L3_DispSetColor(FgColor);
        L3_DispSetBkColor(BkColor);
        L3_DispGotoXY(X,Y);
        L3_DispString(pString);
    }
}

/* ========================================================================== */
/**
 * \brief   This function draws a filled rectangle with specified color “Color”.
 *
 * \param  Color - Color to be used for Filling the rectangle.
 * \param  X1 - Upper left X-position.
 * \param  Y1 - Upper left Y-position.
 * \param  X2 - Lower right X-position.
 * \param  Y2 - Lower right Y-position.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispRectFillColor(uint32_t Color, int8_t X1, int8_t X2, int8_t Y1, int8_t Y2)
{
    L3_DispSetColor(Color);
    L3_DispFillRect(X1,X2,Y1,Y2);
}

/* ========================================================================== */
/**
 * \brief   This function copies the content of a memory device from memory to
 *          the OLED Display.
 *
 * \details This is wrapper function which internally calls the Micrium API to
 *          copy the content of a memory device to OLED display.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DispMemDevCopyToLCD(void)
{
    GUI_MEMDEV_CopyToLCD(L3DispMemdevice);
}

/* ========================================================================== */
/**
 * \brief   This function creates and configures the display driver device. 
 *
 * \details This is a configuration routine for creating the display driver device,
 *          setting the color conversion routines and the display size. This is
 *          called from Micrium library.
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
void LCD_X_Config(void)
{
    GUI_DEVICE *pDevice;
    CONFIG_FLEXCOLOR Config = {0};
    GUI_PORT_API PortAPI = {0};

    // Using the Micrium API to set display driver and color conversion.
    pDevice = GUI_DEVICE_CreateAndLink(GUIDRV_FLEXCOLOR, GUICC_565, 0, 0);

    // Using the Micrium API to set the Display Size configuration.
    LCD_SetSizeEx(0, DISP_WIDTH,  DISP_HEIGHT);

    // Using the Micrium API to set the Orientation.
    Config.Orientation = GUI_MIRROR_X | GUI_SWAP_XY;
    GUIDRV_FlexColor_Config(pDevice, &Config);

    // Set controller and operation mode.
    // A0 = Command.
    // A1 = Data.
    PortAPI.pfWrite8_A0  = &DispWriteCommand;
    PortAPI.pfWrite8_A1  = &DispWriteData;
    PortAPI.pfWriteM8_A0 = &DispWriteMultiCommand;
    PortAPI.pfWriteM8_A1 = &DispWriteMultiData;

    PortAPI.pfRead8_A0   = &DispReadCommand;
    PortAPI.pfRead8_A1   = &DispReadData;
    PortAPI.pfReadM8_A0  = &DispReadMultiCommand;
    PortAPI.pfReadM8_A1  = &DispReadMultiData;

    // Micrium API which configures the hardware routines.
    GUIDRV_FlexColor_SetFunc(pDevice, &PortAPI, GUIDRV_FLEXCOLOR_SSD1351, GUIDRV_FLEXCOLOR_M16C0B8);
}

/* ========================================================================== */
/**
 * \brief   This function creates and configures the display driver device. 
 *
 * \details Callback routine called by the display driver during the initialization 
 *          process for putting the display into operation. This is called from 
 *          Micrium library.
 *
 * \param   LayerIndex - Layer index.
 * \param   Cmd - Command to be executed.
 * \param   pData - Pointer to data structure. 
 *
 * \return   int32_t - Status
 * \retval  -2 - Error occurred 
 * \retval  -1 - Cmd not handled 
 * \retval   0 - Cmd has been Successfully executed
 *
 * ========================================================================== */
int32_t LCD_X_DisplayDriver(uint32_t LayerIndex, uint32_t Cmd, void *pData)
{
    int32_t Status = 0;

    switch (Cmd)
    {
        case LCD_X_INITCONTROLLER:
            // Called during the initialization process in order to set up the.
            // display and put it into operation. 
            if (DISP_PORT_STATUS_OK != L3_DispPortInit(DISP_INIT))
            {
               Status = DISP_DRV_INIT_FAIL;
            } /*else not needed */
            break;

        case LCD_X_ON:
            // Required if the display should support switching on and off.
            DispWriteCommand(DISP_CMD_ON);   // Display on.
            break;

        case LCD_X_OFF:
            // Required if the display should support switching on and off.
            DispWriteCommand(DISP_CMD_OFF);   // Display off.
            break;

        default:
            Status = DISP_DRV_INVALID_CMD;
            break;
    }

    return Status;
}

/* ========================================================================== */
/**
 * \brief   This function clears the display RAM of Display. 
 *
 * \details This function is used to clear SSD1351's display RAM which is of 
 *          size 128x128. The column and Row address map to SEG(Segment Address) 
 *          and COMM (Common Address).
 *
 * \param   < None >
 *
 * \return  None
 *
 * ========================================================================== */
static void DispPortClearRAM(void)
{
    uint8_t RowIndex;   // variable to loop through while clearing the display RAM.
    uint8_t ColumnIndex;   // variable to loop through while clearing the display RAM.

    DISP_CMD_OUT(DISP_CMD_SET_COLUMN_ADDRESS);    // Column address.
    DISP_DATA_OUT(0);
    DISP_DATA_OUT(DISP_SIZE);

    DISP_CMD_OUT(DISP_CMD_SET_ROW_ADDRESS);       // Row address.
    DISP_DATA_OUT(0);
    DISP_DATA_OUT(DISP_SIZE);

    DISP_CMD_OUT(DISP_CMD_WR_RAM);                // Write RAM command.

    for (RowIndex = 0; RowIndex <= DISP_SIZE; RowIndex++)
    {
        for (ColumnIndex = 0; ColumnIndex <= DISP_SIZE; ColumnIndex++)
        {
            // Clearing the 16 bit data.
            DISP_DATA_OUT(0x00);
            DISP_DATA_OUT(0x00);
        }
    }
}

/* ========================================================================== */
/**
 * \brief   Controls display on/off
 *
 * \details Turns display on or off as requested by the user
 *
 * \param   DisplayIsOn - Display is on if 'true' else off.
 *
 * \return  None
 *
 * ========================================================================== */
void L3_DisplayOn(bool DisplayIsOn)
{

    if(DisplayIsOn)
    {
        DispWriteCommand(DISP_CMD_ON); 
        OSTimeDly(100);             // To avoid initial flicker
    }
    else
    {
        DispWriteCommand(DISP_CMD_OFF); 
    }

    return;
}

/* ========================================================================== */
/**
 * \brief   This routine is called by μC/GUI to log the debug messages. 
 *
 * \param   pString - Pointer to the string to be sent for logging.
 *
 * \return  None
 *
 * ========================================================================== */
void GUI_X_Log(const char  *pString)
{
    if (NULL != pString)
    {
        Log(DBG, "%s",pString);
    }
}

/* ========================================================================== */
/**
 * \brief   This routine is called by μC/GUI to log the warning messages. 
 *
 * \param   pString - Pointer to the string to be sent for logging.
 *
 * \return  None
 *
 * ========================================================================== */
void GUI_X_Warn(const char *pString)
{
    if (NULL != pString)
    {
        Log(WNG, "%s",pString);
    }
}

/* ========================================================================== */
/**
 * \brief   This routine is called by μC/GUI to log the error messages. 
 *
 * \param   pString - Pointer to the string to be sent for logging.
 *
 * \return  None
 *
 * ========================================================================== */
void GUI_X_ErrorOut(const char *pString)
{
    if (NULL != pString)
    {
        Log(ERR, "%s",pString);
    }
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif
