#pragma once

#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

class W_SAMPLEGAMEPLUGIN_DLL SampleGameState : public WFallbackGameState
{
  W_ADD_DYNAMIC_REFLECTION(SampleGameState, WFallbackGameState);

public:
  SampleGameState();

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureMainWindowInputDevices(WWindow* pWindow) override;
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;

private:
  virtual void OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;
  virtual void AfterWorldUpdate() override;

  // BEGIN-DOCS-CODE-SNIPPET: confunc-decl
  void ConFunc_Print(WString sText);
  WConsoleFunction<void(WString)> m_ConFunc_Print;
  // END-DOCS-CODE-SNIPPET

  WDeque<WGameObjectHandle> m_SpawnedObjects;
};
