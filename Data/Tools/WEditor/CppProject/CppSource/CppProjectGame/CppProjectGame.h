#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class CppProjectGame : public WGameApplication
{
public:
  using SUPER = WGameApplication;

  CppProjectGame();

protected:
  virtual WResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual WUniquePtr<WGameStateBase> CreateGameState() override;

private:
  WResult TryProjectFolder(WStringView sPath);
  void DetermineProjectPath();
};
