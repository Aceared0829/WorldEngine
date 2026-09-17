#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class WPlayerApplication : public WGameApplication
{
public:
  using SUPER = WGameApplication;

  WPlayerApplication();

protected:
  virtual WResult BeforeCoreSystemsStartup() override;

private:
  void DetermineProjectPath();
};
