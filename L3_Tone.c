#ifdef __cplusplus  /* header compatible with C++ project */
extern "C" {
#endif

/* ========================================================================== */
/**
 * \addtogroup L3_Tone
 * \{
 * \brief   Layer 3 Tone
 *
 * \details The Tone Module is responsible for playing distinct tones using the
 *          API functions provided.
 *
 * \copyright 2020 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    L3_Tone.c
 *
 * ========================================================================== */

/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L3_Tone.h"

/******************************************************************************/
/*                             Global Constant Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Global Variable Definitions(s)                 */
/******************************************************************************/

/******************************************************************************/
/*                             Local Define(s) (Macros)                       */
/******************************************************************************/
#define LOG_GROUP_IDENTIFIER                (LOG_GROUP_TONE) ///< Log Group Identifier
#define TONE_FREQ_DIVISOR                   (11.9f)          ///< Tone Frequency Divisor
#define TONE_FREQ_MULTIPLIER                (2.0f)           ///< Tone Frequency Multiplier
/******************************************************************************/
/*                             Local Type Definition(s)                       */
/******************************************************************************/

/******************************************************************************/
/*                             Local Constant Definition(s)                   */
/******************************************************************************/

/******************************************************************************/
/*                             Local Variable Definition(s)                   */
/******************************************************************************/

static bool ToneActive = false;        // True if tone sequence is being played

/******************************************************************************/
/*                             Local Function Prototype(s)                    */
/******************************************************************************/

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/


/******************************************************************************/
/*                                 Global Functions                            */
/******************************************************************************/

/* ========================================================================== */
/**
 * \brief   This function plays a tone sequence
 *
 * \details It waits for the whole tone sequence to be completed before returning to
 *          the caller.
 * \param   pTone - Tone to be played
 *
 * \return  None
 *
 * ========================================================================== */
void L3_TonePlay(Tone *pTone)
{
    uint8_t index;

    do
    {
        /* Check for Valid Input */
        if ( (NULL == pTone) || (NULL == pTone->pToneNotes) )
        {
            Log(ERR, "L3_TonePlay: Invalid Tone Input");
            break;
        }

        ToneActive = true;                              // Start of tone sequence
        Log(REQ, "L3_TonePlay: %s", pTone->ToneName);

        for (index = 0; pTone->pToneNotes[index].Duration != NULL; index++)
        {
            (void) L3_FpgaWriteReg(FPGA_REG_PIEZO_PWM, (uint32_t)((pTone->pToneNotes[index].Frequency / TONE_FREQ_DIVISOR) * TONE_FREQ_MULTIPLIER));
                   OSTimeDly((uint32_t)(pTone->pToneNotes[index].Duration));
            (void) L3_FpgaWriteReg(FPGA_REG_PIEZO_PWM, 0);
        }
        ToneActive = false;                             // Tone sequence complete

    } while (false);
}

/* ========================================================================== */
/**
 * \brief   Return tone playing status
 *
 * \details This function returns the value of the boolean ToneActive, which
 *          is set true as long as a tone sequence is being played, false
 *          otherwise.
 * \n \n
 *          This function is called by L3_Motor. It is used detect that
 *          a sound is being played. This is used to inhibit FPGA reset
 *          at the start of a move. (The FPGA shall not be reset at the
 *          start of a move if a sound is being played)
 *
 * \param   < None >
 *
 * \return  true if a tone sequence is playing, false otherwise.
 *
 * ========================================================================== */
bool L3_IsToneActive(void)
{
    return ToneActive;
}

/**
 * \}  <If using addtogroup above>
 */

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

