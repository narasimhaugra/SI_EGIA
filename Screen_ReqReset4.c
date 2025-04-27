#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   Request Reset Screen template (image #4)
 *
 * \details Shows Request Reset Screen (image #4) 
 *
 *
 * \copyright 2021 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_ReqReset4.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_ReqReset4.h"
#include "UI_externals.h"
   
static UI_SCREEN Screen_ReqReset4_Screen[] =
{
  &YellowBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BigHandle_2,
  NULL
};

static UI_SEQUENCE Sequence_ReqReset4_Sequence[] =
  {
    Screen_ReqReset4_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
 /**
 * \brief   Shows Request Reset Screen (image #4)  
 *  
 * \details Shows Request Reset Screen (image #4)   
 *  
 * \param   < None >
 *  
 * \return  None
* ========================================================================== */
void Gui_ReqReset4_Screen(void)
{
  if(UI_ReturnToDefaultParameters())
  {
    L4_DmShowScreen_New(SCREEN_ID_REQ_RESET_4, 150, Sequence_ReqReset4_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

