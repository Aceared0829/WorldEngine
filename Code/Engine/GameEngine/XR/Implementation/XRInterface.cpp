#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRInterface.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WXRStageSpace, 1)
  W_BITFLAGS_CONSTANTS(WXRStageSpace::Seated, WXRStageSpace::Standing)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

W_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRInterface);
