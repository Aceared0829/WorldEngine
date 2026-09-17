#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class AsteroidsGame : public WGameApplication
{
public:
  using SUPER = WGameApplication;

  AsteroidsGame();

protected:
  virtual WResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual WUniquePtr<WGameStateBase> CreateGameState() override;

private:
  WResult TryProjectFolder(WStringView sPath);
  void DetermineProjectPath();
};
