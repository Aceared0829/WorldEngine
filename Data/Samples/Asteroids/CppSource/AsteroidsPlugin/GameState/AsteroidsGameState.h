#pragma once

#include <AsteroidsPlugin/AsteroidsPluginDLL.h>
#include <AsteroidsPlugin/GameState/Level.h>
#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>

// the WFallbackGameState adds a free flying camera and a scene switching menu, so can be useful in the very beginning
// but generally it's better to use WGameState instead
// using AsteroidsGameStateBase = WFallbackGameState;
using AsteroidsGameStateBase = WGameState;

class AsteroidsGameState : public AsteroidsGameStateBase
{
  W_ADD_DYNAMIC_REFLECTION(AsteroidsGameState, AsteroidsGameStateBase);

public:
  AsteroidsGameState();
  ~AsteroidsGameState();

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureInputActions() override;
  virtual void OnChangedMainWorld(WWorld* pPrevWorld, WWorld* pNewWorld, WStringView sStartPosition, const WTransform& startPositionOffset) override;
  virtual void GetStartupOptions(WString& out_sScene, WString& out_sPreloadCollection) override;

private:
  virtual void OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;

  void CreateGameLevel();
  void DestroyLevel();

  WUniquePtr<Level> m_pLevel;
};
