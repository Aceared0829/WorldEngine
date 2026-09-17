#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Containers/Blob.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class W_VISUALSCRIPTPLUGIN_DLL WVisualScriptInstance : public WScriptInstance
{
public:
  WVisualScriptInstance(WReflectedClass& inout_owner, WWorld* pWorld, const WSharedPtr<WVisualScriptDataStorage>& pConstantDataStorage, const WSharedPtr<const WVisualScriptDataDescription>& pInstanceDataDesc, const WSharedPtr<WVisualScriptInstanceDataMapping>& pInstanceDataMapping);

  virtual void SetInstanceVariable(const WHashedString& sName, const WVariant& value) override;
  virtual WVariant GetInstanceVariable(const WHashedString& sName) override;

  WVisualScriptDataStorage* GetConstantDataStorage() { return m_pConstantDataStorage.Borrow(); }
  WVisualScriptDataStorage* GetInstanceDataStorage() { return &m_InstanceDataStorage; }

private:
  WSharedPtr<WVisualScriptDataStorage> m_pConstantDataStorage;
  WSharedPtr<WVisualScriptInstanceDataMapping> m_pInstanceDataMapping;

  WVisualScriptDataStorage m_InstanceDataStorage;
};
