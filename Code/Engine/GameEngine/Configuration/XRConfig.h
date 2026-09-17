#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <GameEngine/GameEngineDLL.h>

class W_GAMEENGINE_DLL WXRConfig : public WProfileConfigData
{
  W_ADD_DYNAMIC_REFLECTION(WXRConfig, WProfileConfigData);

public:
  virtual void SaveRuntimeData(WChunkStreamWriter& inout_stream) const override;
  virtual void LoadRuntimeData(WChunkStreamReader& inout_stream) override;

  bool m_bEnableXR = false;
  WString m_sXRRenderPipeline;
};
