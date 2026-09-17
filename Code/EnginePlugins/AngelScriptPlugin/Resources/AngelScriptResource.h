#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/ScriptClassResource.h>

class asIScriptModule;
class asITypeInfo;

using WAngelScriptResourceHandle = WTypedResourceHandle<class WAngelScriptResource>;

class W_ANGELSCRIPTPLUGIN_DLL WAngelScriptResource : public WScriptClassResource
{
  W_ADD_DYNAMIC_REFLECTION(WAngelScriptResource, WScriptClassResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WAngelScriptResource);

public:
  WAngelScriptResource();
  ~WAngelScriptResource();

  WStringView GetScriptContent() const { return m_sScriptContent; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  virtual WUniquePtr<WScriptInstance> Instantiate(WReflectedClass& inout_owner, WWorld* pWorld) const override;

  void FindMessageHandlers(const asITypeInfo* pClassType, WScriptRTTI::MessageHandlerList& inout_Handlers);

  WString m_sClassName;
  WString m_sScriptContent;
  asIScriptModule* m_pModule = nullptr;
};
