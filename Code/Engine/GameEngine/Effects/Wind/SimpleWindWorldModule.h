#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <GameEngine/GameEngineDLL.h>

class W_GAMEENGINE_DLL WSimpleWindWorldModule : public WWindWorldModuleInterface
{
  W_DECLARE_WORLD_MODULE();
  W_ADD_DYNAMIC_REFLECTION(WSimpleWindWorldModule, WWindWorldModuleInterface);

public:
  WSimpleWindWorldModule(WWorld* pWorld);
  ~WSimpleWindWorldModule();

  virtual WVec3 GetWindAt(const WVec3& vPosition) const override;

  void SetFallbackWind(const WVec3& vWind);

private:
  WVec3 m_vFallbackWind;
};
