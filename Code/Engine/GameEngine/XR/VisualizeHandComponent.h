#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using WVisualizeHandComponentManager = WComponentManagerSimple<class WVisualizeHandComponent, WComponentUpdateType::WhenSimulating>;

class W_GAMEENGINE_DLL WVisualizeHandComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WVisualizeHandComponent, WComponent, WVisualizeHandComponentManager);

public:
  WVisualizeHandComponent();
  ~WVisualizeHandComponent();

protected:
  void Update();
};
