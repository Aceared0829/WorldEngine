#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class WBakingScene;

class WLongOpWorker_BakeScene : public WLongOpWorker
{
  W_ADD_DYNAMIC_REFLECTION(WLongOpWorker_BakeScene, WLongOpWorker);

public:
  virtual WResult InitializeExecution(WStreamReader& ref_config, const WUuid& documentGuid) override;
  virtual WResult Execute(WProgress& ref_progress, WStreamWriter& ref_proxydata) override;

  WString m_sOutputPath;
  WBakingScene* m_pScene;
};
