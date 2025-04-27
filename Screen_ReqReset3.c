#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   Request Reset Screen template (image #3)
 *
 * \details Shows Request Reset Screen (image #3) 
 *
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_ReqReset3.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_ReqReset3.h"
#include "UI_externals.h"
   
static UI_SCREEN Screen_ReqReset3_Screen[] =
{
  &YellowBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BigHandle_2,
  &YellowArrowCircle_2,
  NULL
};

static UI_SEQUENCE Sequence_ReqReset3_Sequence[] =
  {
    Screen_ReqReset3_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
/**
 * \brief   Shows Request Reset Screen (image #3)  
 *  
 * \details Shows Request Reset Screen (image #3)   
 *  
 * \param   < None >
 *  
 * \return  None
* ========================================================================== */
void Gui_ReqReset3_Screen(void)
{
  if(UI_ReturnToDefaultParameters())
  {
    L4_DmShowScreen_New(SCREEN_ID_REQ_RESET_3, 150, Sequence_ReqReset3_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

