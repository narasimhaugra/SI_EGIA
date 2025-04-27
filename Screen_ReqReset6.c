#ifdef __cplusplus  /* header compatible with C++ project */
extern "C"
{
#endif

/* ========================================================================== */
/**
 * \addtogroup Screens
 * \{
 *
 * \brief   Request Reset Screen template (image #6)
 *
 * \details Shows Request Reset Screen (image #6) 
 *
 *
 * \copyright 2022 Covidien - Surgical Innovations. All Rights Reserved.
 *
 * \file    Screen_ReqReset6.c
 *
 * ========================================================================== */
/******************************************************************************/
/*                             Include                                        */
/******************************************************************************/
#include "L4_DisplayManager.h"
#include "Screen_ReqReset6.h"
#include "UI_externals.h"

#define X_ARROW_SHIFT 9  
extern UI_SCREEN Screen_ReqReset5_Screen[];  

static UI_SEQUENCE Sequence_ReqReset6_Sequence[] =
  {
    Screen_ReqReset5_Screen,
    NULL
  };

/******************************************************************************/
/*                                 Local Functions                            */
/******************************************************************************/
/* ==========================================================================*/
 /**
 * \brief   Shows Request Reset Screen (image #6)  
 *  
 * \details Shows Request Reset Screen (image #6)   
 *  
 * \param   < None >
 *  
 * \return  None
* ========================================================================== */
void Gui_ReqReset6_Screen(void)
{
  if(UI_ReturnToDefaultParameters())
  {
    YellowArrowLeft.ObjBitmap.X = YellowArrowLeft.ObjBitmap.X - X_ARROW_SHIFT;
    YellowArrowRight.ObjBitmap.X = YellowArrowRight.ObjBitmap.X + X_ARROW_SHIFT;
    L4_DmShowScreen_New(SCREEN_ID_REQ_RESET_6, 150, Sequence_ReqReset6_Sequence );
}
}
/**
*\} Doxygen group end tag
*/

#ifdef __cplusplus  /* header compatible with C++ project */
}
#endif

