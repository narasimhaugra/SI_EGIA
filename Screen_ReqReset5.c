#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   Request Reset Screen template (image #5)
 *
 * \details Shows Request Reset Screen (image #5) 
 *
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_ReqReset5.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_ReqReset5.h"
#include "UI_externals.h"
   
UI_SCREEN Screen_ReqReset5_Screen[] =
{
  &YellowBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BigHandleTop,
  &Hold10sImage,
  &YellowArrowLeft,
  &YellowArrowRight,
  NULL
};

static UI_SEQUENCE Sequence_ReqReset5_Sequence[] =
  {
    Screen_ReqReset5_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
 /**
 * \brief   Shows Request Reset Screen (image #5)  
 *  
 * \details Shows Request Reset Screen (image #5)   
 *  
 * \param   < None >
 *  
 * \return  None
* ========================================================================== */
void Gui_ReqReset5_Screen(void)
{
  if(UI_ReturnToDefaultParameters())
  {
    L4_DmShowScreen_New(SCREEN_ID_REQ_RESET_5, 150, Sequence_ReqReset5_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

