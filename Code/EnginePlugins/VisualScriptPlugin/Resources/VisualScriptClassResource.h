#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <VisualScriptPlugin/Runtime/VisualScriptData.h>

class W_VISUALSCRIPTPLUGIN_DLL WVisualScriptClassResource : public WScriptClassResource
{
  W_ADD_DYNAMIC_REFLECTION(WVisualScriptClassResource, WScriptClassResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WVisualScriptClassResource);

public:
  WVisualScriptClassResource();
  ~WVisualScriptClassResource();

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  virtual WUniquePtr<WScriptInstance> Instantiate(WReflectedClass& inout_owner, WWorld* pWorld) const override;

  WSharedPtr<WVisualScriptDataStorage> m_pConstantDataStorage;
  WSharedPtr<const WVisualScriptDataDescription> m_pInstanceDataDesc;
  WSharedPtr<WVisualScriptInstanceDataMapping> m_pInstanceDataMapping;
};
