#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>
#include <Core/Scripting/ScriptClassResource.h>
#include <Core/Scripting/ScriptRTTI.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Containers/Blob.h>

class asIScriptContext;
class asIScriptModule;
class asIScriptObject;
class WComponent;
class WScriptComponent;

class W_ANGELSCRIPTPLUGIN_DLL WAngelScriptInstance : public WScriptInstance
{
public:
  WAngelScriptInstance(WReflectedClass& inout_owner, WWorld* pWorld, asIScriptModule* pModule, const char* szObjectTypeName);
  ~WAngelScriptInstance();

  virtual void SetInstanceVariable(const WHashedString& sName, const WVariant& value) override;
  virtual WVariant GetInstanceVariable(const WHashedString& sName) override;

  asIScriptContext* GetContext() const { return m_pContext; }
  asIScriptObject* GetObject() const { return m_pObject; }
  WScriptComponent* GetOwnerComponent() const { return m_pOwnerComponent; }

private:
  void ExceptionCallback(asIScriptContext* pContext);

  asIScriptObject* m_pObject = nullptr;
  asIScriptContext* m_pContext = nullptr;
  WScriptComponent* m_pOwnerComponent = nullptr;
};
