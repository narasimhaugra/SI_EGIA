#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   Request Reset Screen template (image #2)
 *
 * \details Shows Request Reset Screen (image #2) 
 *
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_ReqReset2.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_ReqReset2.h"
#include "UI_externals.h"
   
static UI_SCREEN Screen_ReqReset2_Screen[] =
{
  &YellowBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BigHandle_1,
  NULL
};

static UI_SEQUENCE Sequence_ReqReset2_Sequence[] =
  {
    Screen_ReqReset2_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
 /**
 * \brief   Shows Request Reset Screen (image #2)  
 *  
 * \details Shows Request Reset Screen (image #2)   
 *  
 * \param   < None >
 *  
 * \return  None
* ========================================================================== */
void Gui_ReqReset2_Screen(void)
{
  if(UI_ReturnToDefaultParameters())
  {
    L4_DmShowScreen_New(SCREEN_ID_REQ_RESET_2, 150, Sequence_ReqReset2_Sequence );
}
}

/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

