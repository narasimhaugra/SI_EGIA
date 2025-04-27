#include "L3_GuiWidgets.h"
#include "Signia_ChargerManager.h"

static bool sb_DefaultParametersAreCreated = false;
extern bool L4_DmIsScreenLocked_New(void);

#define Y_POS_0_ANVIL 39
#define Y_POS_0_TISSUE (Y_POS_0_ANVIL+8)
#define Y_POS_100_ANVIL 59
#define Y_POS_100_TISSUE (Y_POS_100_ANVIL+8)

extern const unsigned char _acBattery_100[];
extern const unsigned char _acBattery_10[];
extern const unsigned char _acBattery_0[];
extern const unsigned char _acBattery_Error[];
extern const unsigned char _acRequestClamp2[];
extern const unsigned char EGIA_ForceDial1Pic[];
extern const unsigned char EGIA_ForceDial2Pic[];
extern const unsigned char EGIA_ForceDial3Pic[];
extern const unsigned char _acHandleBM[];
extern const unsigned char _acHandleBM90[];
extern const unsigned char _acPowerPack[];
extern const unsigned char _acAdapterBM[];
extern const unsigned char _acReload_EGIA_BM[];
//extern const unsigned char _acEEA_ReloadBar[];
//extern const unsigned char _acEEA_ReloadBarRealPurple[];
//extern const unsigned char _acEEA_AnvilAssembly[];
//extern const unsigned char _acEEA_MaxForceMeterBlack[];
//extern const unsigned char _acEEA_MaxForceMeter_0[];
//extern const unsigned char _acEEA_MaxForceMeter_25[];
//extern const unsigned char _acEEA_MaxForceMeter_50[];
//extern const unsigned char _acEEA_MaxForceMeter_75[];
//extern const unsigned char _acEEA_MaxForceMeter_100[];
//extern const unsigned char _acEEA_normalTissue[];
//extern const unsigned char _acEEA_purpleStapleLoad[];
//extern const unsigned char _acEEA_ReloadShell[];
extern const unsigned char _acAlert_Error[];
//extern const unsigned char _acWaterDrop[];
extern const unsigned char _acGreenLoading_1[];
extern const unsigned char _acGreenLoading_2[];
extern const unsigned char _acGreenLoading_3[];
extern const unsigned char _acGreenLoading_4[];
extern const unsigned char _acGreenLoading_5[];
extern const unsigned char _acGreenLoading_6[];
extern const unsigned char _acEEA_HandleCloseupsafetylightgreen[];
extern const unsigned char _acSafety_Yellow_ArrowCircle_Left[];
extern const unsigned char _acSafety_Yellow_ArrowCircle_Right[];
extern const unsigned char _acEEA_HandleCloseupsafetylightgreenMirrored[];
extern const unsigned char _acHOLD10s[];
extern const unsigned char _acTopview_Handle[];
extern const unsigned char _acYellowArrowLeft[];
extern const unsigned char _acYellowArrowRight[];
extern const unsigned char GUI_PLTFM_AW_EEAReloadPic[];
extern const unsigned char GUI_PLTFM_AW_EGIAReloadPic[];
extern const unsigned char GUI_PLTFM_AW_SmallGreenCirclePic[];
extern const unsigned char _acGreenArrowLeft[];
extern const unsigned char _acYellowWrench[];
extern const unsigned char _acErrorCircle[];
extern const unsigned char _acRotationConfigHandle[];
extern const unsigned char _acRotationConfigLeftButtonsGreen[];
extern const unsigned char _acRotationConfigRightButtonsGreen[];
extern const unsigned char _acRotationConfigLeftButtonsWhite[];
extern const unsigned char _acRotationConfigRightButtonsWhite[];
extern const unsigned char _acGreenSphere[];
extern const unsigned char _acWhiteArrowLeft[];
extern const unsigned char _acWhiteArrowRight[];
extern const unsigned char _acRequestReload[];
extern const unsigned char _acRequestClamp1[];
extern const unsigned char _acGreenArrowDown[];
extern const unsigned char _acRequestClamp2[];
extern const unsigned char _acGreenArrowUp[];
extern const unsigned char _acEGIA_HighFireForce[];
extern const unsigned char _acUnsupportedAdapter[];
//extern const unsigned char _acEGIA_HighClampForce[];

bool GetBatteryLevelValue(uint8_t ScreenId);
bool GetBatteryLevelImage(uint8_t ScreenId);
bool GetFiringLevelValue(uint8_t ScreenId);
bool MoveAnvil(uint8_t ScreenId);
bool MoveTissue(uint8_t ScreenId);
bool ChooseBars(uint8_t ScreenId);
bool AdjustXpositionOfTextOnLeftPanelBottom(uint8_t ScreenId);
bool AdjustXpositionOfTextOnCenterPanelBottom_Bold(uint8_t ScreenId);

uint8_t 		g_uiPercentage;
uint8_t 		g_uiCurrentFiringLevel;
uint16_t 		g_uiNumberForTextOnLeftPanelBottom = 300;
uint16_t 		g_uiNumberForTextOnCenterPanelBottom = 300;
bool 			UIDefaultParamCreated = false;
static uint8_t 	YSHIFT = 6;

//#######################################################################################
// Memory to keep default parameters:
//UI_OBJECT UI_DefaultObjects[HOW_MANY_UI_OBJECTS];

//############################
// Some reusable UI objects:
//############################
UI_OBJECT HighForceImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  5, 40, 58, 37,
  NULL//(BITMAP*)&_acEGIA_HighClampForce
  }
};

UI_OBJECT HighForceImage_Copy;

UI_OBJECT GreenArrowUpImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  67, 54, 15, 15,
  (BITMAP*)&_acGreenArrowUp
  }
};

UI_OBJECT GreenArrowUpImage_Copy;

UI_OBJECT GreenArrowDownImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  67, 59, 15, 15,
  (BITMAP*)&_acGreenArrowDown
  }
};

UI_OBJECT GreenArrowDownImage_Copy;

UI_OBJECT ClampingClosedImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  7, 52, 55, 20,
  (BITMAP*)&_acRequestClamp1
  }
};

UI_OBJECT ClampingClosedImage_Copy;

UI_OBJECT ClampingOpenedImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  7, 52, 55, 20,
  (BITMAP*)&_acRequestClamp2
  }
};

UI_OBJECT ClampingOpenedImage_Copy;

UI_OBJECT RequestReloadArrow =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  63, 38, 28, 13,
  (BITMAP*)&_acRequestReload
  }
};

UI_OBJECT RequestReloadArrow_Copy;

UI_OBJECT LeftUpperWhiteArrow =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  20, // uiXpos
  59, // uiYpos
  9, // uiXsize
  11, // uiYsize
  (BITMAP*)&_acWhiteArrowRight
  }
};

UI_OBJECT LeftUpperWhiteArrow_Copy;

UI_OBJECT LeftLowerWhiteArrow =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  20, // uiXpos
  71, // uiYpos
  9, // uiXsize
  11, // uiYsize
  (BITMAP*)&_acWhiteArrowRight
  }
};

UI_OBJECT LeftLowerWhiteArrow_Copy;

UI_OBJECT RightUpperWhiteArrow =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  70, // uiXpos
  59, // uiYpos
  9, // uiXsize
  11, // uiYsize
  (BITMAP*)&_acWhiteArrowLeft
  }
};

UI_OBJECT RightUpperWhiteArrow_Copy;

UI_OBJECT RightLowerWhiteArrow =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  70, // uiXpos
  71, // uiYpos
  9, // uiXsize
  11, // uiYsize
  (BITMAP*)&_acWhiteArrowLeft
  }
};

UI_OBJECT RightLowerWhiteArrow_Copy;

UI_OBJECT TextInLeftRing =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  13,
  61,
  20,
  20,
  SIG_COLOR_WHITE,
  SIG_COLOR_TRANSPARENT,
  0,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  "1"
  }
};

UI_OBJECT TextInLeftRing_Copy;

UI_OBJECT TextInRightRing =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  72,
  61,
  20,
  20,
  SIG_COLOR_WHITE,
  SIG_COLOR_TRANSPARENT,
  0,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  "1"
  }
};

UI_OBJECT TextInRightRing_Copy;

UI_OBJECT LeftRing =
{
  UI_TYPE_CIRCLE, // OurObjectType
  NULL,
  .ObjCircle=
  {
    19,  // X
    70, //Y
    12, // Radius
    false, // bFill
    SIG_COLOR_TRANSPARENT, // Background color
    1, // BorderSize
    SIG_COLOR_GREEN // BorderColor
  }
};

UI_OBJECT LeftRing_Copy;

UI_OBJECT RightRing =
{
  UI_TYPE_CIRCLE, // OurObjectType
  NULL,
  .ObjCircle=
  {
    78,  // X
    70, //Y
    12, // Radius
    false, // bFill
    SIG_COLOR_TRANSPARENT, // Background color
    1, // BorderSize
    SIG_COLOR_GREEN // BorderColor
  }
};

UI_OBJECT RightRing_Copy;

UI_OBJECT LeftGreenCircle =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  9, // uiXpos
  61, // uiYpos
  21, // uiXsize
  19, // uiYsize
  (BITMAP*)&_acGreenSphere
  }
};

UI_OBJECT LeftGreenCircle_Copy;

UI_OBJECT RightGreenCircle =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  68, // uiXpos
  61, // uiYpos
  21, // uiXsize
  19, // uiYsize
  (BITMAP*)&_acGreenSphere
  }
};

UI_OBJECT RightGreenCircle_Copy;

UI_OBJECT LeftGreenButton =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  34, // uiXpos
  62, // uiYpos
  4, // uiXsize
  17, // uiYsize
  (BITMAP*)&_acRotationConfigLeftButtonsGreen
  }
};

UI_OBJECT LeftGreenButton_Copy;

UI_OBJECT RightGreenButton =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  60, // uiXpos
  62, // uiYpos
  4, // uiXsize
  17, // uiYsize
  (BITMAP*)&_acRotationConfigRightButtonsGreen
  }
};

UI_OBJECT RightGreenButton_Copy;

UI_OBJECT LeftWhiteButton =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  34, // uiXpos
  62, // uiYpos
  4, // uiXsize
  17, // uiYsize
  (BITMAP*)&_acRotationConfigLeftButtonsWhite
  }
};

UI_OBJECT LeftWhiteButton_Copy;

UI_OBJECT RightWhiteButton =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  60, // uiXpos
  62, // uiYpos
  4, // uiXsize
  17, // uiYsize
  (BITMAP*)&_acRotationConfigRightButtonsWhite
  }
};

UI_OBJECT RightWhiteButton_Copy;

UI_OBJECT RotateHandleImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  29, // uiXpos
  23, // uiYpos
  40, // uiXsize
  69, // uiYsize
  (BITMAP*)&_acRotationConfigHandle
  }
};

UI_OBJECT RotateHandleImage_Copy;

UI_OBJECT BigReloadImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  33, // uiXpos
  43, // uiYpos
  55, // uiXsize
  20, // uiYsize
  (BITMAP*)&_acRequestClamp2
  }
};

UI_OBJECT BigReloadImage_Copy;

UI_OBJECT ErrorCircleImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  22, // uiXpos
  33, // uiYpos
  52, // uiXsize
  48, // uiYsize
  (BITMAP*)&_acErrorCircle
  }
};

UI_OBJECT ErrorCircleImage_Copy;

UI_OBJECT YellowWrenchImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  68, // uiXpos
  8, // uiYpos
  21, // uiXsize
  20, // uiYsize
  (BITMAP*)&_acYellowWrench
  }
};

UI_OBJECT YellowWrenchImage_Copy;

UI_OBJECT GreenArrowLeftImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  46, // uiXpos
  39, // uiYpos
  15, // uiXsize
  15, // uiYsize
  (BITMAP*)&_acGreenArrowLeft
  }
};

UI_OBJECT GreenArrowLeftImage_Copy;

UI_OBJECT GreenCircleImage_1 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  10, // uiXpos
  38, // uiYpos
  10, // uiXsize
  12, // uiYsize
  (BITMAP*)&GUI_PLTFM_AW_SmallGreenCirclePic
  }
};

UI_OBJECT GreenCircleImage_1_Copy;

UI_OBJECT GreenCircleImage_2 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  10, // uiXpos
  55, // uiYpos
  10, // uiXsize
  12, // uiYsize
  (BITMAP*)&GUI_PLTFM_AW_SmallGreenCirclePic
  }
};

UI_OBJECT GreenCircleImage_2_Copy;

UI_OBJECT EEAreloadImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  25, // uiXpos
  55, // uiYpos
  26, // uiXsize
  14, // uiYsize
  (BITMAP*)&GUI_PLTFM_AW_EEAReloadPic
  }
};

UI_OBJECT EEAreloadImage_Copy;

UI_OBJECT BigHandle_1 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  10, // uiXpos
  14, // uiYpos
  79, // uiXsize
  66, // uiYsize
  (BITMAP*)&_acEEA_HandleCloseupsafetylightgreen
  }
};

UI_OBJECT BigHandle_1_Copy;

UI_OBJECT BigHandle_2 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  9, // uiXpos
  14, // uiYpos
  79, // uiXsize
  66, // uiYsize
  (BITMAP*)&_acEEA_HandleCloseupsafetylightgreenMirrored
  }
};

UI_OBJECT BigHandle_2_Copy;

UI_OBJECT BigHandleTop =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  37, // uiXpos
  21, // uiYpos
  22, // uiXsize
  63, // uiYsize
  (BITMAP*)&_acTopview_Handle
  }
};

UI_OBJECT BigHandleTop_Copy;

UI_OBJECT Hold10sImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  13, // uiXpos
  7, // uiYpos
  70, // uiXsize
  8, // uiYsize
  (BITMAP*)&_acHOLD10s
  }
};

UI_OBJECT Hold10sImage_Copy;

UI_OBJECT YellowArrowLeft =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  72, // uiXpos
  50, // uiYpos
  15, // uiXsize
  15, // uiYsize
  (BITMAP*)&_acYellowArrowLeft
  }
};

UI_OBJECT YellowArrowLeft_Copy;

UI_OBJECT YellowArrowRight =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  9, // uiXpos
  50, // uiYpos
  15, // uiXsize
  15, // uiYsize
  (BITMAP*)&_acYellowArrowRight
  }
};

UI_OBJECT YellowArrowRight_Copy;

UI_OBJECT YellowArrowCircle_1 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  20, // uiXpos
  18, // uiYpos
  32, // uiXsize
  32, // uiYsize
  (BITMAP*)&_acSafety_Yellow_ArrowCircle_Left
  }
};

UI_OBJECT YellowArrowCircle_1_Copy;

UI_OBJECT YellowArrowCircle_2 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  46, // uiXpos
  18, // uiYpos
  32, // uiXsize
  32, // uiYsize
  (BITMAP*)&_acSafety_Yellow_ArrowCircle_Right
  }
};

UI_OBJECT YellowArrowCircle_2_Copy;

UI_OBJECT GreenLoading_1 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  25, // uiXpos
  32, // uiYpos
  50, // uiXsize
  50, // uiYsize
  (BITMAP*)&_acGreenLoading_1
  }
};

UI_OBJECT GreenLoading_1_Copy;

UI_OBJECT GreenLoading_2 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  25, // uiXpos
  32, // uiYpos
  50, // uiXsize
  50, // uiYsize
  (BITMAP*)&_acGreenLoading_2
  }
};

UI_OBJECT GreenLoading_2_Copy;

UI_OBJECT GreenLoading_3 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  25, // uiXpos
  32, // uiYpos
  50, // uiXsize
  50, // uiYsize
  (BITMAP*)&_acGreenLoading_3
  }
};

UI_OBJECT GreenLoading_3_Copy;

UI_OBJECT GreenLoading_4 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  25, // uiXpos
  32, // uiYpos
  50, // uiXsize
  50, // uiYsize
  (BITMAP*)&_acGreenLoading_4
  }
};

UI_OBJECT GreenLoading_4_Copy;

UI_OBJECT GreenLoading_5 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  25, // uiXpos
  32, // uiYpos
  50, // uiXsize
  50, // uiYsize
  (BITMAP*)&_acGreenLoading_5
  }
};

UI_OBJECT GreenLoading_5_Copy;

UI_OBJECT GreenLoading_6 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  25, // uiXpos
  32, // uiYpos
  50, // uiXsize
  50, // uiYsize
  (BITMAP*)&_acGreenLoading_6
  }
};

UI_OBJECT GreenLoading_6_Copy;

UI_OBJECT WaterDropImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  70, // uiXpos
  58, // uiYpos
  15, // uiXsize
  22, // uiYsize
  NULL//(BITMAP*)&_acWaterDrop
  }
};

UI_OBJECT WaterDropImage_Copy;

UI_OBJECT BatteryImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  &GetBatteryLevelImage,
  .ObjBitmap=
  {
  42, // uiXpos
  5, // uiYpos
  48, // uiXsize
  16, // uiYsize
  (BITMAP*)&_acBattery_100
  }
};

UI_OBJECT BatteryImage_Copy;

UI_OBJECT BatteryImage_10 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  42, // uiXpos
  5, // uiYpos
  48, // uiXsize
  16, // uiYsize
  (BITMAP*)&_acBattery_10
  }
};

UI_OBJECT BatteryImage_10_Copy;

UI_OBJECT BatteryImage_0 =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  42, // uiXpos
  5, // uiYpos
  48, // uiXsize
  16, // uiYsize
  (BITMAP*)&_acBattery_0
  }
};

UI_OBJECT BatteryImage_0_Copy;

UI_OBJECT BatteryErrorImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  52, // uiXpos
  6, // uiYpos
  37, // uiXsize
  18, // uiYsize
  (BITMAP*)&_acBattery_Error
  }
};

UI_OBJECT BatteryErrorImage_Copy;

UI_OBJECT BatteryProgressBar =
{
  UI_TYPE_PROGRESS,
  &GetBatteryLevelValue,
  .ObjProgress=
  {
    true,
    49, // x
    8,  // y
    38, // width
    8,  // height
    SIG_COLOR_BLACK,
    SIG_COLOR_GREEN,
    0,
    100,
    100
  }
};

UI_OBJECT BatteryProgressBar_Copy;

UI_OBJECT FiringProgressBar =
{
  UI_TYPE_PROGRESS,
  &GetFiringLevelValue,
  .ObjProgress=
  {
    true,
    6, // x
    24,  // y
    82, // width
    14,  // height
    SIG_COLOR_GREEN,
    SIG_COLOR_PURPLE,
    0,
    100,
    100
  }
};

UI_OBJECT FiringProgressBar_Copy;

UI_OBJECT HandleWithGreenButtonImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  6, // uiXpos
  47, // uiYpos
  28, // uiXsize
  38, // uiYsize
  (BITMAP*)&_acHandleBM
  //(BITMAP*)&_acPowerPack
  }
};

UI_OBJECT HandleWithGreenButtonImage_Copy;

UI_OBJECT HandleWithoutGreenButtonImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  31, // uiXpos
  43, // uiYpos
  28, // uiXsize
  38, // uiYsize
  //(BITMAP*)&_acHandleBM90
  (BITMAP*)&_acPowerPack
  }
};

UI_OBJECT HandleWithoutGreenButtonImage_Copy;

UI_OBJECT AdapterImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  33, // uiXpos
  47, // uiYpos
  27, // uiXsize
  16, // uiYsize
  (BITMAP*)&_acAdapterBM
  }
};

UI_OBJECT AdapterImage_Copy;

UI_OBJECT EGIAReloadImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  61, // uiXpos
  46, // uiYpos
  26, // uiXsize
  14, // uiYsize
  //(BITMAP*)&_acReload_EGIA_BM
  (BITMAP*)&GUI_PLTFM_AW_EGIAReloadPic
  }
};

UI_OBJECT EGIAReloadImage_Copy;

UI_OBJECT EGIAReloadOtherImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  61, // uiXpos
  46, // uiYpos
  27, // uiXsize
  10, // uiYsize
  (BITMAP*)&_acReload_EGIA_BM
  //(BITMAP*)&GUI_PLTFM_AW_EGIAReloadPic
  }
};

UI_OBJECT EGIAReloadOtherImage_Copy;

UI_OBJECT EGIA_ForceDial1Image =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  10, // uiXpos
  43, // uiYpos
  75, // uiXsize
  45, // uiYsize
  (BITMAP*)&EGIA_ForceDial1Pic
  }
};

UI_OBJECT EGIA_ForceDial1Image_Copy;

UI_OBJECT EGIA_ForceDial2Image =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  10, // uiXpos
  43, // uiYpos
  75, // uiXsize
  45, // uiYsize
  (BITMAP*)&EGIA_ForceDial2Pic
  }
};

UI_OBJECT EGIA_ForceDial2Image_Copy;

UI_OBJECT EGIA_ForceDial3Image =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  10, // uiXpos
  43, // uiYpos
  75, // uiXsize
  45, // uiYsize
  (BITMAP*)&EGIA_ForceDial3Pic
  }
};

UI_OBJECT EGIA_ForceDial3Image_Copy;

UI_OBJECT WhiteBoxAround =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  0,
  0,
  96,
  96,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_WHITE,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT WhiteBoxAround_Copy;

UI_OBJECT BlackBoxAround =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  0,
  0,
  96,
  96,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_BLACK,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT BlackBoxAround_Copy;

UI_OBJECT YellowBoxAround =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  0,
  0,
  96,
  96,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_YELLOW,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT YellowBoxAround_Copy;

UI_OBJECT GreenBoxAround =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  1,
  1,
  94,
  94,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_GREEN,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT GreenBoxAround_Copy;

UI_OBJECT ThinGreenBoxAround =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  2,
  2,
  91,
  91,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_GREEN,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT ThinGreenBoxAround_Copy;

UI_OBJECT BlackBoxInsideWhiteBox =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  1,
  1,
  93,
  93,
  SIG_COLOR_BLACK,
  SIG_COLOR_BLACK,
  0,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT BlackBoxInsideWhiteBox_Copy;

UI_OBJECT BlackBoxInsideGreenBox =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  5,
  5,
  87,
  87,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_BLACK,
  0,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT BlackBoxInsideGreenBox_Copy;

UI_OBJECT BlackBoxInsideGreenBox_2 =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  3,
  3,
  89,
  89,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_BLACK,
  0,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT BlackBoxInsideGreenBox_2_Copy;

UI_OBJECT LeftGreenBoxOfThree =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  4,
  44,
  27,
  47,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_GREEN,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT LeftGreenBoxOfThree_Copy;

UI_OBJECT CenterGreenBoxOfThree =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  33,
  44,
  26,
  47,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_GREEN,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT CenterGreenBoxOfThree_Copy;

UI_OBJECT RightGreenBoxOfThree =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  61,
  44,
  30,
  47,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_GREEN,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT RightGreenBoxOfThree_Copy;

UI_OBJECT BlackBoxInsideCenterGreenBoxOfThree =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  34,
  45,
  24,
  45,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_BLACK,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT BlackBoxInsideCenterGreenBoxOfThree_Copy;

UI_OBJECT UpperWhiteBar =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  4,
  22,
  86,
  18,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_WHITE,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  ""
  }
};

UI_OBJECT UpperWhiteBar_Copy;

UI_OBJECT UpperMagentaBar =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  6,
  24,
  82,
  14,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_PINK,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  ""
  }
};

UI_OBJECT UpperMagentaBar_Copy;

UI_OBJECT TextOnUpperMagentaBar =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  68,
  24,
  6,
  4,
  SIG_COLOR_BLACK,
  SIG_COLOR_TRANSPARENT,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  "60"
  }
};

UI_OBJECT TextOnUpperMagentaBar_Copy;

UI_OBJECT TextLeftMiddle =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  6,
  46,
  20,
  12,
  SIG_COLOR_WHITE,
  SIG_COLOR_TRANSPARENT,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  "2x"
  }
};

UI_OBJECT TextLeftMiddle_Copy;

UI_OBJECT TextForX =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  20,
  48,
  20,
  12,
  SIG_COLOR_WHITE,
  SIG_COLOR_TRANSPARENT,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  "x"
  }
};

UI_OBJECT TextForX_Copy;

UI_OBJECT UpperLeftTransparentText =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  2,
  3,
  10,
  8,
  SIG_COLOR_WHITE,
  SIG_COLOR_TRANSPARENT,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  "2x"
  }
};

UI_OBJECT UpperLeftTransparentText_Copy;

UI_OBJECT CircleOnRightPanel =
{
  UI_TYPE_CIRCLE, // OurObjectType
  NULL,
  .ObjCircle=
  {
    77,  // X
    68, //Y
    10, // Radius
    true, // bFill
    SIG_COLOR_WHITE, // Background color
    1, // BorderSize
    SIG_COLOR_BLACK // BorderColor
  }
};

UI_OBJECT CircleOnRightPanel_Copy;

UI_OBJECT TextInCircle =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  73,
  61,
  6,
  2,
  SIG_COLOR_RED,
  SIG_COLOR_TRANSPARENT,
  0,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  "0"
  }
};

UI_OBJECT TextInCircle_Copy;

UI_OBJECT TextOnRightPanelBottom =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  69,
  78,
  6,
  2,
  SIG_COLOR_BLACK,
  SIG_COLOR_TRANSPARENT,
  0,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  "1"
  }
};

UI_OBJECT TextOnRightPanelBottom_Copy;

UI_OBJECT TextOnLeftPanelBottom =
{
  UI_TYPE_TEXT, // OurObjectType
  &AdjustXpositionOfTextOnLeftPanelBottom,
  .ObjText=
  {
  5,
  78,
  6,
  2,
  SIG_COLOR_BLACK,
  SIG_COLOR_TRANSPARENT,
  0,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  ""
  }
};

UI_OBJECT TextOnLeftPanelBottom_Copy;

UI_OBJECT TextOnCenterPanelBottom =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  42,
  78,
  6,
  2,
  SIG_COLOR_RED,
  SIG_COLOR_TRANSPARENT,
  0,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  "0"
  }
};

UI_OBJECT TextOnCenterPanelBottom_Copy;

UI_OBJECT TextOnCenterPanelBottom_Bold =
{
  UI_TYPE_TEXT, // OurObjectType
  &AdjustXpositionOfTextOnCenterPanelBottom_Bold,
  .ObjText=
  {
  30,
  70,
  20,
  10,
  SIG_COLOR_BLACK,
  SIG_COLOR_TRANSPARENT,
  0,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_20B_1,
  "100"
  }
};

UI_OBJECT TextOnCenterPanelBottom_Bold_Copy;

UI_OBJECT TextRemCount =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  60,
  75,
  80,
  10,
  SIG_COLOR_BLACK,
  SIG_COLOR_TRANSPARENT,
  0,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  "300"
  }
};

UI_OBJECT TextRemCount_Copy;

UI_OBJECT TriangleAboveRightPanel =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  64, // uiXpos
  25, // uiYpos
  25, // uiXsize
  14, // uiYsize
  (BITMAP*)&_acAlert_Error
  }
};

UI_OBJECT TriangleAboveRightPanel_Copy;

//EEA related objects - commented for now
/*
  UI_OBJECT ReloadBarImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  7, // uiXpos
  22, // uiYpos
  84, // uiXsize
  15, // uiYsize
  (BITMAP*)&_acEEA_ReloadBarRealPurple
  }
};

UI_OBJECT ReloadBarImage_Copy;

  UI_OBJECT StapleLoadImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  43, // uiXpos
  68, // uiYpos
  48, // uiXsize
  11, // uiYsize
  (BITMAP*)&_acEEA_purpleStapleLoad
  }
};

UI_OBJECT StapleLoadImage_Copy;

  UI_OBJECT ReloadShellImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  43, // uiXpos
  77, // uiYpos
  49, // uiXsize
  14, // uiYsize
  (BITMAP*)&_acEEA_ReloadShell
  }
};

UI_OBJECT ReloadShellImage_Copy;

UI_OBJECT NormalTissueImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  &MoveTissue,
  .ObjBitmap=
  {
  43, // uiXpos
  Y_POS_100_TISSUE, // uiYpos
  49, // uiXsize
  23, // uiYsize
  (BITMAP*)&_acEEA_normalTissue
  }
};

UI_OBJECT NormalTissueImage_Copy;

  UI_OBJECT AnvilAssemblyImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  &MoveAnvil,
  .ObjBitmap=
  {
  46, // uiXpos
  Y_POS_100_ANVIL, // uiYpos
  43, // uiXsize
  32, // uiYsize
  (BITMAP*)&_acEEA_AnvilAssembly
  }
};

UI_OBJECT AnvilAssemblyImage_Copy;

 UI_OBJECT ForceMeterImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  &ChooseBars,
  .ObjBitmap=
  {
  20, // uiXpos
  41, // uiYpos
  22, // uiXsize
  49, // uiYsize
  (BITMAP*)&_acEEA_MaxForceMeter_0
  }
};

UI_OBJECT ForceMeterImage_Copy;
*/

UI_OBJECT UpperLeftGrayBox =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  4,
  23,
  27,
  18,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_GRAY,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  ""
  }
};

UI_OBJECT UpperLeftGrayBox_Copy;

UI_OBJECT UpperCenterGrayBox =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  33,
  23,
  26,
  18,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_GRAY,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  ""
  }
};

UI_OBJECT UpperCenterGrayBox_Copy;

UI_OBJECT UpperRightGrayBox =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  61,
  23,
  30,
  18,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_GRAY,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  ""
  }
};

UI_OBJECT UpperRightGrayBox_Copy;

UI_OBJECT UpperLeftWhiteBox =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  6,
  25,
  23,
  14,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_WHITE,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  ""
  }
};

UI_OBJECT UpperLeftWhiteBox_Copy;

UI_OBJECT UpperCenterWhiteBox =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  35,
  25,
  22,
  14,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_WHITE,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  ""
  }
};

UI_OBJECT UpperCenterWhiteBox_Copy;

UI_OBJECT UpperRightWhiteBox =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  63,
  25,
  25,
  14,
  SIG_COLOR_TRANSPARENT,
  SIG_COLOR_WHITE,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  ""
  }
};

UI_OBJECT UpperRightWhiteBox_Copy;

UI_OBJECT UpperLeftTextBox =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  9,
  26,
  6,
  4,
  SIG_COLOR_BLACK,
  SIG_COLOR_TRANSPARENT,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  "30"
  }
};

UI_OBJECT UpperLeftTextBox_Copy;

UI_OBJECT UpperCenterTextBox =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  38,
  26,
  6,
  4,
  SIG_COLOR_BLACK,
  SIG_COLOR_TRANSPARENT,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  "45"
  }
};

UI_OBJECT UpperCenterTextBox_Copy;

UI_OBJECT UpperRightTextBox =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  67,
  26,
  6,
  4,
  SIG_COLOR_BLACK,
  SIG_COLOR_TRANSPARENT,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  "60"
  }
};

UI_OBJECT UpperRightTextBox_Copy;

UI_OBJECT UpperRightTextFiringBox =
{
  UI_TYPE_TEXT, // OurObjectType
  NULL,
  .ObjText=
  {
  72,
  24,
  6,
  4,
  SIG_COLOR_BLACK,
  SIG_COLOR_TRANSPARENT,
  1,
  SIG_COLOR_TRANSPARENT,
  SIG_FONT_13B_1,
  ""
  }
};

UI_OBJECT UpperRightTextFiringBox_Copy;

UI_OBJECT UnsupportedAdapterImage =
{
  UI_TYPE_BITMAP, // OurObjectType
  NULL,
  .ObjBitmap=
  {
  38, // uiXpos
  38, // uiYpos
  20, // uiXsize
  19, // uiYsize
  (BITMAP*)&_acUnsupportedAdapter
  }
};

UI_OBJECT UnsupportedAdapterImage_Copy;
//###########################################################################################################
void AdjustPannelsVerticalPositions(void)
{
    LeftGreenBoxOfThree.ObjText.Y = LeftGreenBoxOfThree.ObjText.Y - YSHIFT;
    LeftGreenBoxOfThree.ObjText.Height = LeftGreenBoxOfThree.ObjText.Height + YSHIFT;
    CenterGreenBoxOfThree.ObjText.Y = CenterGreenBoxOfThree.ObjText.Y - YSHIFT;
    CenterGreenBoxOfThree.ObjText.Height = CenterGreenBoxOfThree.ObjText.Height + YSHIFT;
    BlackBoxInsideCenterGreenBoxOfThree.ObjText.Y = BlackBoxInsideCenterGreenBoxOfThree.ObjText.Y - YSHIFT;
    BlackBoxInsideCenterGreenBoxOfThree.ObjText.Height = BlackBoxInsideCenterGreenBoxOfThree.ObjText.Height + YSHIFT;
    HandleWithGreenButtonImage.ObjBitmap.Y = HandleWithGreenButtonImage.ObjBitmap.Y - YSHIFT;
    HandleWithoutGreenButtonImage.ObjBitmap.Y = HandleWithoutGreenButtonImage.ObjBitmap.Y - YSHIFT;
    AdapterImage.ObjBitmap.Y = AdapterImage.ObjBitmap.Y - YSHIFT;
}

//###############################################################################################################
void UI_CreateDefaultParameters(void)
{
	if(!sb_DefaultParametersAreCreated)
 	{
		 UIDefaultParamCreated = true;

		 HighForceImage_Copy =
		 HighForceImage;

		 GreenArrowUpImage_Copy =
		 GreenArrowUpImage;

		 GreenArrowDownImage_Copy =
		 GreenArrowDownImage;

		 ClampingClosedImage_Copy =
		 ClampingClosedImage;

		 ClampingOpenedImage_Copy =
		 ClampingOpenedImage;

		 RequestReloadArrow_Copy =
		 RequestReloadArrow;

		 LeftUpperWhiteArrow_Copy =
		 LeftUpperWhiteArrow;

		 LeftLowerWhiteArrow_Copy =
		 LeftLowerWhiteArrow;

		 RightUpperWhiteArrow_Copy =
		 RightUpperWhiteArrow;

		 RightLowerWhiteArrow_Copy =
		 RightLowerWhiteArrow;

		 TextInLeftRing_Copy =
		 TextInLeftRing;

		 TextInRightRing_Copy =
		 TextInRightRing;

		 LeftRing_Copy =
		 LeftRing;

		 RightRing_Copy =
		 RightRing;

		 LeftGreenCircle_Copy =
		 LeftGreenCircle;

		 RightGreenCircle_Copy =
		 RightGreenCircle;

		 LeftGreenButton_Copy =
		 LeftGreenButton;

		 RightGreenButton_Copy =
		 RightGreenButton;

		 LeftWhiteButton_Copy =
		 LeftWhiteButton;

		 RightWhiteButton_Copy =
		 RightWhiteButton;

		 RotateHandleImage_Copy =
		 RotateHandleImage;

		 BigReloadImage_Copy =
		 BigReloadImage;

		 ErrorCircleImage_Copy =
		 ErrorCircleImage;

		 YellowWrenchImage_Copy =
		 YellowWrenchImage;

		 GreenArrowLeftImage_Copy =
		 GreenArrowLeftImage;

		 GreenCircleImage_1_Copy =
		 GreenCircleImage_1;

		 GreenCircleImage_2_Copy =
		 GreenCircleImage_2;

		 EEAreloadImage_Copy =
		 EEAreloadImage;

		 BigHandle_1_Copy =
		 BigHandle_1;

		 BigHandle_2_Copy =
		 BigHandle_2;

		 BigHandleTop_Copy =
		 BigHandleTop;

		 Hold10sImage_Copy =
		 Hold10sImage;

		 YellowArrowLeft_Copy =
		 YellowArrowLeft;

		 YellowArrowRight_Copy =
		 YellowArrowRight;

		 YellowArrowCircle_1_Copy =
		 YellowArrowCircle_1;

		 YellowArrowCircle_2_Copy =
		 YellowArrowCircle_2;

		 GreenLoading_1_Copy =
		 GreenLoading_1;

		 GreenLoading_2_Copy =
		 GreenLoading_2;

		 GreenLoading_3_Copy =
		 GreenLoading_3;

		 GreenLoading_4_Copy =
		 GreenLoading_4;

		 GreenLoading_5_Copy =
		 GreenLoading_5;

		 GreenLoading_6_Copy =
		 GreenLoading_6;

		 WaterDropImage_Copy =
		 WaterDropImage;

		 BatteryImage_Copy =
		 BatteryImage;

		 BatteryImage_10_Copy =
		 BatteryImage_10;

		 BatteryImage_0_Copy =
		 BatteryImage_0;

		 BatteryErrorImage_Copy =
		 BatteryErrorImage;

		 BatteryProgressBar_Copy =
		 BatteryProgressBar;

		 FiringProgressBar_Copy =
		 FiringProgressBar;

		 HandleWithGreenButtonImage_Copy =
		 HandleWithGreenButtonImage;

		 HandleWithoutGreenButtonImage_Copy =
		 HandleWithoutGreenButtonImage;

		 AdapterImage_Copy =
		 AdapterImage;

		 EGIAReloadImage_Copy =
		 EGIAReloadImage;

		 EGIAReloadOtherImage_Copy =
		 EGIAReloadOtherImage;

		 EGIA_ForceDial1Image_Copy =
		 EGIA_ForceDial1Image;

		 EGIA_ForceDial2Image_Copy =
		 EGIA_ForceDial2Image;

		 EGIA_ForceDial3Image_Copy =
		 EGIA_ForceDial3Image;

		 WhiteBoxAround_Copy =
		 WhiteBoxAround;

		 BlackBoxAround_Copy =
		 BlackBoxAround;

		 YellowBoxAround_Copy =
		 YellowBoxAround;

		 GreenBoxAround_Copy =
		 GreenBoxAround;

		 ThinGreenBoxAround_Copy =
		 ThinGreenBoxAround;

		 BlackBoxInsideWhiteBox_Copy =
		 BlackBoxInsideWhiteBox;

		 BlackBoxInsideGreenBox_Copy =
		 BlackBoxInsideGreenBox;

		 BlackBoxInsideGreenBox_2_Copy =
		 BlackBoxInsideGreenBox_2;

		 LeftGreenBoxOfThree_Copy =
		 LeftGreenBoxOfThree;

		 CenterGreenBoxOfThree_Copy =
		 CenterGreenBoxOfThree;

		 RightGreenBoxOfThree_Copy =
		 RightGreenBoxOfThree;

		 BlackBoxInsideCenterGreenBoxOfThree_Copy =
		 BlackBoxInsideCenterGreenBoxOfThree;

		 UpperWhiteBar_Copy =
		 UpperWhiteBar;

		 UpperMagentaBar_Copy =
		 UpperMagentaBar;

		 TextOnUpperMagentaBar_Copy =
		 TextOnUpperMagentaBar;

		 TextLeftMiddle_Copy =
		 TextLeftMiddle;

		 TextForX_Copy =
		 TextForX;

		 UpperLeftTransparentText_Copy =
		 UpperLeftTransparentText;

		 CircleOnRightPanel_Copy =
		 CircleOnRightPanel;

		 TextInCircle_Copy =
		 TextInCircle;

		 TextOnRightPanelBottom_Copy =
		 TextOnRightPanelBottom;

		 TextOnLeftPanelBottom_Copy =
		 TextOnLeftPanelBottom;

		 TextOnCenterPanelBottom_Copy =
		 TextOnCenterPanelBottom;

		 TextOnCenterPanelBottom_Bold_Copy =
		 TextOnCenterPanelBottom_Bold;

		 TextRemCount_Copy =
		 TextRemCount;

		 TriangleAboveRightPanel_Copy =
		 TriangleAboveRightPanel;

		 /*
		 ReloadBarImage_Copy =
		 ReloadBarImage;

		 StapleLoadImage_Copy =
		 StapleLoadImage;

		 ReloadShellImage_Copy =
		 ReloadShellImage;

		 NormalTissueImage_Copy =
		 NormalTissueImage;

		 AnvilAssemblyImage_Copy =
		 AnvilAssemblyImage;

		 ForceMeterImage_Copy =
		 ForceMeterImage;
		 */
		 UpperLeftGrayBox_Copy =
		 UpperLeftGrayBox;

		 UpperCenterGrayBox_Copy =
		 UpperCenterGrayBox;

		 UpperRightGrayBox_Copy =
		 UpperRightGrayBox;

		 UpperLeftWhiteBox_Copy =
		 UpperLeftWhiteBox;

		 UpperCenterWhiteBox_Copy =
		 UpperCenterWhiteBox;

		 UpperRightWhiteBox_Copy =
		 UpperRightWhiteBox;

		 UpperLeftTextBox_Copy =
		 UpperLeftTextBox;

		 UpperCenterTextBox_Copy =
		 UpperCenterTextBox;

		 UpperRightTextBox_Copy =
		 UpperRightTextBox;

		 UnsupportedAdapterImage_Copy =
		 UnsupportedAdapterImage;

		 UpperRightTextFiringBox_Copy =
		 UpperRightTextFiringBox;

		sb_DefaultParametersAreCreated = true;
	}
}
//################################################################################################################
bool UI_ReturnToDefaultParameters(void)
{
    if (L4_DmIsScreenLocked_New())
       return false;

     if(!sb_DefaultParametersAreCreated)
     {
       UI_CreateDefaultParameters();
     }

     HighForceImage =
     HighForceImage_Copy;
     GreenArrowUpImage =
     GreenArrowUpImage_Copy;
     GreenArrowDownImage =
     GreenArrowDownImage_Copy;
     ClampingClosedImage =
     ClampingClosedImage_Copy;
     ClampingOpenedImage =
     ClampingOpenedImage_Copy;
     RequestReloadArrow =
     RequestReloadArrow_Copy;
     LeftUpperWhiteArrow =
     LeftUpperWhiteArrow_Copy;
     LeftLowerWhiteArrow =
     LeftLowerWhiteArrow_Copy;
     RightUpperWhiteArrow =
     RightUpperWhiteArrow_Copy;
     RightLowerWhiteArrow =
     RightLowerWhiteArrow_Copy;
     TextInLeftRing =
     TextInLeftRing_Copy;
     TextInRightRing =
     TextInRightRing_Copy;
     LeftRing =
     LeftRing_Copy;
     RightRing =
     RightRing_Copy;
     LeftGreenCircle =
     LeftGreenCircle_Copy;
     RightGreenCircle =
     RightGreenCircle_Copy;
     LeftGreenButton =
     LeftGreenButton_Copy;
     RightGreenButton =
     RightGreenButton_Copy;
     LeftWhiteButton =
     LeftWhiteButton_Copy;
     RightWhiteButton =
     RightWhiteButton_Copy;
     RotateHandleImage =
     RotateHandleImage_Copy;
     BigReloadImage =
     BigReloadImage_Copy;
     ErrorCircleImage =
     ErrorCircleImage_Copy;
     YellowWrenchImage =
     YellowWrenchImage_Copy;
     GreenArrowLeftImage =
     GreenArrowLeftImage_Copy;
     GreenCircleImage_1 =
     GreenCircleImage_1_Copy;
     GreenCircleImage_2 =
     GreenCircleImage_2_Copy;
     EEAreloadImage =
     EEAreloadImage_Copy;
     BigHandle_1 =
     BigHandle_1_Copy;
     BigHandle_2 =
     BigHandle_2_Copy;
     BigHandleTop =
     BigHandleTop_Copy;
     Hold10sImage =
     Hold10sImage_Copy;
     YellowArrowLeft =
     YellowArrowLeft_Copy;
     YellowArrowRight =
     YellowArrowRight_Copy;
     YellowArrowCircle_1 =
     YellowArrowCircle_1_Copy;
     YellowArrowCircle_2 =
     YellowArrowCircle_2_Copy;
     GreenLoading_1 =
     GreenLoading_1_Copy;
     GreenLoading_2 =
     GreenLoading_2_Copy;
     GreenLoading_3 =
     GreenLoading_3_Copy;
     GreenLoading_4 =
     GreenLoading_4_Copy;
     GreenLoading_5 =
     GreenLoading_5_Copy;
     GreenLoading_6 =
     GreenLoading_6_Copy;
     WaterDropImage =
     WaterDropImage_Copy;
     BatteryImage =
     BatteryImage_Copy;
     BatteryImage_10 =
     BatteryImage_10_Copy;
     BatteryImage_0 =
     BatteryImage_0_Copy;
     BatteryErrorImage =
     BatteryErrorImage_Copy;
     BatteryProgressBar =
     BatteryProgressBar_Copy;
     FiringProgressBar =
     FiringProgressBar_Copy;
     HandleWithGreenButtonImage =
     HandleWithGreenButtonImage_Copy;
     HandleWithoutGreenButtonImage =
     HandleWithoutGreenButtonImage_Copy;
     AdapterImage =
     AdapterImage_Copy;
     EGIAReloadImage =
     EGIAReloadImage_Copy;
     EGIAReloadOtherImage =
     EGIAReloadOtherImage_Copy;
     EGIA_ForceDial1Image =
     EGIA_ForceDial1Image_Copy;
     EGIA_ForceDial2Image =
     EGIA_ForceDial2Image_Copy;
     EGIA_ForceDial3Image =
     EGIA_ForceDial3Image_Copy;
     WhiteBoxAround =
     WhiteBoxAround_Copy;
     BlackBoxAround =
     BlackBoxAround_Copy;
     YellowBoxAround =
     YellowBoxAround_Copy;
     GreenBoxAround =
     GreenBoxAround_Copy;
     ThinGreenBoxAround =
     ThinGreenBoxAround_Copy;
     BlackBoxInsideWhiteBox =
     BlackBoxInsideWhiteBox_Copy;
     BlackBoxInsideGreenBox =
     BlackBoxInsideGreenBox_Copy;
     BlackBoxInsideGreenBox_2 =
     BlackBoxInsideGreenBox_2_Copy;
     LeftGreenBoxOfThree =
     LeftGreenBoxOfThree_Copy;
     CenterGreenBoxOfThree =
     CenterGreenBoxOfThree_Copy;
     RightGreenBoxOfThree =
     RightGreenBoxOfThree_Copy;
     BlackBoxInsideCenterGreenBoxOfThree =
     BlackBoxInsideCenterGreenBoxOfThree_Copy;
     UpperWhiteBar =
     UpperWhiteBar_Copy;
     UpperMagentaBar =
     UpperMagentaBar_Copy;
     TextOnUpperMagentaBar =
     TextOnUpperMagentaBar_Copy;
     TextLeftMiddle =
     TextLeftMiddle_Copy;
     TextForX =
     TextForX_Copy;
     UpperLeftTransparentText =
     UpperLeftTransparentText_Copy;
     CircleOnRightPanel =
     CircleOnRightPanel_Copy;
     TextInCircle =
     TextInCircle_Copy;
     TextOnRightPanelBottom =
     TextOnRightPanelBottom_Copy;
     TextOnLeftPanelBottom =
     TextOnLeftPanelBottom_Copy;
     TextOnCenterPanelBottom =
     TextOnCenterPanelBottom_Copy;
     TextOnCenterPanelBottom_Bold =
     TextOnCenterPanelBottom_Bold_Copy;
     TextRemCount =
     TextRemCount_Copy;
     TriangleAboveRightPanel =
     TriangleAboveRightPanel_Copy;
     /*
     ReloadBarImage =
     ReloadBarImage_Copy;
     StapleLoadImage =
     StapleLoadImage_Copy;
     ReloadShellImage =
     ReloadShellImage_Copy;
     NormalTissueImage =
     NormalTissueImage_Copy;
     AnvilAssemblyImage =
     AnvilAssemblyImage_Copy;
     ForceMeterImage =
     ForceMeterImage_Copy;
     */
     UpperLeftGrayBox =
     UpperLeftGrayBox_Copy;
     UpperCenterGrayBox =
     UpperCenterGrayBox_Copy;
     UpperRightGrayBox =
     UpperRightGrayBox_Copy;
     UpperLeftWhiteBox =
     UpperLeftWhiteBox_Copy;
     UpperCenterWhiteBox =
     UpperCenterWhiteBox_Copy;
     UpperRightWhiteBox =
     UpperRightWhiteBox_Copy;
     UpperLeftTextBox =
     UpperLeftTextBox_Copy;
     UpperCenterTextBox =
     UpperCenterTextBox_Copy;
     UpperRightTextBox =
     UpperRightTextBox_Copy;
     UnsupportedAdapterImage =
     UnsupportedAdapterImage_Copy;
     UpperRightTextFiringBox =
     UpperRightTextFiringBox_Copy;

     return true;
}
//################################################################################################################
// Artificial function to show the new animation:
void EEA_PercentageCounter(void)
{
  ++g_uiPercentage;
  if(g_uiPercentage > 100)
    g_uiPercentage = 100;
}
//###########################################################################################################
// Global callback functions:

bool AdjustXpositionOfTextOnLeftPanelBottom(uint8_t ScreenId)
{
  if(g_uiNumberForTextOnLeftPanelBottom < 10)
  {
    TextOnLeftPanelBottom.ObjText.X = 13;
  }
  else
  if((g_uiNumberForTextOnLeftPanelBottom >= 10) && (g_uiNumberForTextOnLeftPanelBottom < 100))
  {
    TextOnLeftPanelBottom.ObjText.X = 10;
  }
  else
  if(g_uiNumberForTextOnLeftPanelBottom >= 100)
  {
    TextOnLeftPanelBottom.ObjText.X = 5;
  }
  return true;
}

bool AdjustXpositionOfTextOnCenterPanelBottom_Bold(uint8_t ScreenId)
{
  if(g_uiNumberForTextOnCenterPanelBottom < 10)
  {
    TextOnCenterPanelBottom_Bold.ObjText.X = 30;
  }
  else
  if((g_uiNumberForTextOnCenterPanelBottom >= 10) && (g_uiNumberForTextOnCenterPanelBottom < 100))
  {
    TextOnCenterPanelBottom_Bold.ObjText.X = 35;
  }
  else
  if(g_uiNumberForTextOnCenterPanelBottom >= 100)
  {
    TextOnCenterPanelBottom_Bold.ObjText.X = 40;
  }
  return true;
}

// To show an actual battery level:
bool GetBatteryLevelValue(uint8_t ScreenId)
{
  uint8_t uiCurrentBatteryLevel;

  uiCurrentBatteryLevel = 100;
  Signia_ChargerManagerGetBattRsoc(&uiCurrentBatteryLevel);

  BatteryProgressBar.ObjProgress.Value = 100 - uiCurrentBatteryLevel;

  if(uiCurrentBatteryLevel <= BATTERY_LIMIT_INSUFFICIENT  )
  {
    BatteryProgressBar.ObjProgress.BackColor = SIG_COLOR_RED;
  }
  else
  if((uiCurrentBatteryLevel > BATTERY_LIMIT_INSUFFICIENT  ) && (uiCurrentBatteryLevel <= BATTERY_LIMIT_LOW))
  {
    BatteryProgressBar.ObjProgress.BackColor = SIG_COLOR_YELLOW;
  }
  /*
  else
  {
    BatteryProgressBar.ObjProgress.BackColor = SIG_COLOR_GREEN;
  }
  */
  return true;
}

// To show an actual battery image:
bool GetBatteryLevelImage(uint8_t ScreenId)
{
  uint8_t uiCurrentBatteryLevel;

  uiCurrentBatteryLevel = 100;
  Signia_ChargerManagerGetBattRsoc(&uiCurrentBatteryLevel);

  if(uiCurrentBatteryLevel <= BATTERY_LIMIT_INSUFFICIENT)
  {
    BatteryImage.ObjBitmap.pBitmap = (BITMAP*)&_acBattery_0;
  }
  else
  if((uiCurrentBatteryLevel > BATTERY_LIMIT_INSUFFICIENT) && (uiCurrentBatteryLevel <= BATTERY_LIMIT_LOW))
  {
    BatteryImage.ObjBitmap.pBitmap = (BITMAP*)&_acBattery_10;
  }
  else
  {
    BatteryImage.ObjBitmap.pBitmap = (BITMAP*)&_acBattery_100;
  }

  return true;
}

// To show an actual firing level:
bool GetFiringLevelValue(uint8_t ScreenId)
{
  FiringProgressBar.ObjProgress.Value = g_uiCurrentFiringLevel;
  return true;
}

// EEA related functions - commented for now:
/*
// To move anvil & tissue position depending on percentage:
bool MoveAnvil(uint8_t ScreenId)
{
  AnvilAssemblyImage.ObjBitmap.Y = Y_POS_0_ANVIL + ((Y_POS_100_ANVIL - Y_POS_0_ANVIL) * g_uiPercentage / 100);
  return true;
}

bool MoveTissue(uint8_t ScreenId)
{
  NormalTissueImage.ObjBitmap.Y = Y_POS_0_TISSUE + ((Y_POS_100_TISSUE - Y_POS_0_TISSUE) * g_uiPercentage / 100);
  return true;
}

// To draw different versical bars depending on percentage:
bool ChooseBars(uint8_t ScreenId)
{
  if(g_uiPercentage < 25)
      ForceMeterImage.ObjBitmap.pBitmap = (BITMAP*)&_acEEA_MaxForceMeter_0;
  else
  if((g_uiPercentage >= 25) && (g_uiPercentage < 50))
      ForceMeterImage.ObjBitmap.pBitmap = (BITMAP*)&_acEEA_MaxForceMeter_25;
  else
  if((g_uiPercentage >= 50) && (g_uiPercentage < 75))
      ForceMeterImage.ObjBitmap.pBitmap = (BITMAP*)&_acEEA_MaxForceMeter_50;
  else
  if((g_uiPercentage >= 75) && (g_uiPercentage < 100))
      ForceMeterImage.ObjBitmap.pBitmap = (BITMAP*)&_acEEA_MaxForceMeter_75;
  else
  if(g_uiPercentage == 100)
      ForceMeterImage.ObjBitmap.pBitmap = (BITMAP*)&_acEEA_MaxForceMeter_100;
  return true;
}
*/
//####################################################################################################################
// Reusable screens and sequences:

UI_SCREEN Screen_rotate_activate_left_countdown_Screen[] =
{
  &GreenBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &RotateHandleImage,
  &UpperWhiteBar,
  &UpperMagentaBar,
  &TextOnUpperMagentaBar,
  &RightGreenButton,
  &LeftWhiteButton,
  &RightGreenCircle,
  &LeftRing,
  &RightRing,
  &TextInLeftRing,
  NULL
};

UI_SEQUENCE Sequence_rotate_activate_left_countdown_Sequence[] =
  {
    Screen_rotate_activate_left_countdown_Screen,
    NULL
  };

UI_SCREEN Screen_rotate_activate_right_countdown_Screen[] =
{
  &GreenBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &RotateHandleImage,
  &UpperWhiteBar,
  &UpperMagentaBar,
  &TextOnUpperMagentaBar,
  &LeftGreenButton,
  &RightWhiteButton,
  &LeftGreenCircle,
  &LeftRing,
  &RightRing,
  &TextInRightRing,
  NULL
};

UI_SEQUENCE Sequence_rotate_activate_right_countdown_Sequence[] =
  {
    Screen_rotate_activate_right_countdown_Screen,
    NULL
  };

UI_SCREEN Screen_rotate_deactivate_left_countdown_Screen[] =
{
  &GreenBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &RotateHandleImage,
  &UpperWhiteBar,
  &UpperMagentaBar,
  &TextOnUpperMagentaBar,
  &RightGreenButton,
  &LeftGreenButton,
  &RightGreenCircle,
  &LeftRing,
  &RightRing,
  &TextInLeftRing,
  NULL
};

UI_SEQUENCE Sequence_rotate_deactivate_left_countdown_Sequence[] =
  {
    Screen_rotate_deactivate_left_countdown_Screen,
    NULL
  };

UI_SCREEN Screen_rotate_deactivate_right_countdown_Screen[] =
{
  &GreenBoxAround,
  &BlackBoxInsideGreenBox_2,
  &BatteryImage,
  &BatteryProgressBar,
  &RotateHandleImage,
  &UpperWhiteBar,
  &UpperMagentaBar,
  &TextOnUpperMagentaBar,
  &RightGreenButton,
  &LeftGreenButton,
  &LeftGreenCircle,
  &LeftRing,
  &RightRing,
  &TextInRightRing,
  NULL
};

UI_SEQUENCE Sequence_rotate_deactivate_right_countdown_Sequence[] =
  {
    Screen_rotate_deactivate_right_countdown_Screen,
    NULL
  };


//###########################################################################################################
bool UI_DefaultParametersCreated(void)
{
    return UIDefaultParamCreated;
}

//####################################################################################################################
