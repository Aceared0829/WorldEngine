#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRInputDevice.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WXRInputDevice, 1, WRTTINoAllocator);
// no properties or message handlers
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

W_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRInputDevice);
