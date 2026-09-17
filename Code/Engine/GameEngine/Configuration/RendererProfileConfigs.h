#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <GameEngine/GameEngineDLL.h>

class W_GAMEENGINE_DLL WRenderPipelineProfileConfig : public WProfileConfigData
{
  W_ADD_DYNAMIC_REFLECTION(WRenderPipelineProfileConfig, WProfileConfigData);

public:
  virtual void SaveRuntimeData(WChunkStreamWriter& inout_stream) const override;
  virtual void LoadRuntimeData(WChunkStreamReader& inout_stream) override;

  WString m_sMainRenderPipeline;
  // WString m_sEditorRenderPipeline;
  // WString m_sDebugRenderPipeline;

  WMap<WString, WString> m_CameraPipelines;
};
