#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   Request Reset Screen template (image #1)
 *
 * \details Shows Request Reset Screen (image #1) 
 *
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_ReqReset1.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_ReqReset1.h"
#include "UI_externals.h"
   
static UI_SCREEN Screen_ReqReset1_Screen[] =
{
  &YellowBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BigHandle_1,
  &YellowArrowCircle_1,
  NULL
};

static UI_SEQUENCE Sequence_ReqReset1_Sequence[] =
  {
    Screen_ReqReset1_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
/**
 * \brief   Shows Request Reset Screen (image #1)  
 *  
 * \details Shows Request Reset Screen (image #1)   
 *  
 * \param   < None >
 *  
 * \return  None
* ========================================================================== */
void Gui_ReqReset1_Screen(void)
{
  if(UI_ReturnToDefaultParameters())
  {
    L4_DmShowScreen_New(SCREEN_ID_REQ_RESET_1, 150, Sequence_ReqReset1_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

