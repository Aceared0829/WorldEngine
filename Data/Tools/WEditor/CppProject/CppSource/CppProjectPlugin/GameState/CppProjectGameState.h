#pragma once

#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <CppProjectPlugin/CppProjectPluginDLL.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>

// the WFallbackGameState adds a free flying camera and a scene switching menu, so can be useful in the very beginning
// but generally it's better to use WGameState instead
// using CppProjectGameStateBase = WFallbackGameState;
using CppProjectGameStateBase = WGameState;

class CppProjectGameState : public CppProjectGameStateBase
{
  W_ADD_DYNAMIC_REFLECTION(CppProjectGameState, CppProjectGameStateBase);

public:
  CppProjectGameState();
  ~CppProjectGameState();

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;
  virtual WResult SpawnPlayer(WStringView sStartPosition, const WTransform& startPositionOffset) override;
  virtual void OnChangedMainWorld(WWorld* pPrevWorld, WWorld* pNewWorld, WStringView sStartPosition, const WTransform& startPositionOffset) override;
  virtual void GetStartupOptions(WString& out_sScene, WString& out_sPreloadCollection) override;

private:
  virtual void OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset) override;
  virtual void BeforeWorldUpdate() override;
  virtual void AfterWorldUpdate() override;

  WDeque<WGameObjectHandle> m_SpawnedObjects;
};
