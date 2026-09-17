#pragma once

#include <Core/Input/Declarations.h>
#include <Core/Input/VirtualThumbStick.h>
#include <Core/World/Declarations.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/GameState.h>
#include <PacManPlugin/PacManPluginDLL.h>

// Every game can have a single 'game state' for high-level logic.
// For more details, see https://ezengine.net/pages/docs/runtime/application/game-state.html
class PacManGameState : public WGameState
{
  W_ADD_DYNAMIC_REFLECTION(PacManGameState, WGameState);

public:
  static WHashedString s_sStats;
  static WHashedString s_sCoinsEaten;
  static WHashedString s_sPacManState;

public:
  PacManGameState();
  ~PacManGameState();

  // Called at the start of each frame. The typical per-frame decisions should be done here.
  virtual void ProcessInput() override;

protected:
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;

private:
  virtual void OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset) override;
  virtual void OnDeactivation() override;
  virtual void AfterWorldUpdate() override;
  virtual WResult SpawnPlayer(WStringView sStartPosition, const WTransform& startPositionOffset) override;
  virtual void GetStartupOptions(WString& out_sScene, WString& out_sPreloadCollection) override;

  void ResetState();

  // How many coins we have in the scene, in total.
  WUInt32 m_uiNumCoinsTotal = 0;
  bool m_bTouchInput = false;
  bool m_bShowSceneExportError = false;

  WUniquePtr<WVirtualThumbStick> m_pLeftStick;
  WUniquePtr<WVirtualThumbStick> m_pRightStick;
};
