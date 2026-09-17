#pragma once

#include <RTSPlugin/GameMode/GameMode.h>
#include <RTSPlugin/GameState/RTSGameState.h>
#include <RmlUiPlugin\Components\RmlUiCanvas2DComponent.h>

class RtsMainMenuMode : public RtsGameMode
{
public:
  RtsMainMenuMode();
  ~RtsMainMenuMode();

protected:
  virtual void OnActivateMode() override;
  virtual void OnDeactivateMode() override;
  virtual void OnProcessInput(const RtsMouseInputState& MouseInput, bool bUiWantsInput) override;
  virtual void OnBeforeWorldUpdate() override;

  WTypedComponentHandle<WRmlUiCanvas2DComponent> m_hMainMenu;
};
