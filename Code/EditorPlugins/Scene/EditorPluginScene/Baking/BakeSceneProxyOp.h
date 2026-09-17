#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class WLongOpProxy_BakeScene : public WLongOpProxy
{
  W_ADD_DYNAMIC_REFLECTION(WLongOpProxy_BakeScene, WLongOpProxy);

public:
  virtual void InitializeRegistered(const WUuid& documentGuid, const WUuid& componentGuid) override;
  virtual const char* GetDisplayName() const override { return "Bake Scene"; }
  virtual void GetReplicationInfo(WStringBuilder& out_sReplicationOpType, WStreamWriter& ref_description) override;
  virtual void Finalize(WResult result, const WDataBuffer& resultData) override;

private:
  WUuid m_DocumentGuid;
  WUuid m_ComponentGuid;
};
