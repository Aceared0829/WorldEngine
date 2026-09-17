#pragma once

#include <Foundation/Math/Vec2.h>
#include <GameEngine/GameEngineDLL.h>

class WDebugRendererContext;
class WVirtualThumbStick;

namespace WInputDebugVis
{
  /// Renders a debug visualization of the given thumbstick using the 2D screen space debug render functions.
  W_GAMEENGINE_DLL void DebugRender(const WDebugRendererContext& context, const WVec2& vResolution, const WVirtualThumbStick& stick);

}; // namespace WInputDebugVis
