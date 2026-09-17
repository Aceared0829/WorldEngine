#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <Core/Scripting/ScriptComponent.h>

static WGameObject* GetAngelScriptOwnerObject(asIScriptObject* pSelf)
{
  if (pSelf)
  {
    WAngelScriptInstance* pInstance = (WAngelScriptInstance*)pSelf->GetUserData(WAsUserData::ScriptInstancePtr);
    pSelf->Release();

    return pInstance->GetOwnerComponent()->GetOwner();
  }

  return nullptr;
}

static WWorld* GetAngelScriptOwnerWorld(asIScriptObject* pSelf)
{
  if (pSelf)
  {
    WAngelScriptInstance* pInstance = (WAngelScriptInstance*)pSelf->GetUserData(WAsUserData::ScriptInstancePtr);
    pSelf->Release();

    return pInstance->GetOwnerComponent()->GetWorld();
  }

  return nullptr;
}

static WScriptComponent* GetAngelScriptOwnerComponent(asIScriptObject* pSelf)
{
  if (pSelf)
  {
    WAngelScriptInstance* pInstance = (WAngelScriptInstance*)pSelf->GetUserData(WAsUserData::ScriptInstancePtr);
    pSelf->Release();

    return pInstance->GetOwnerComponent();
  }

  return nullptr;
}


void WAngelScriptEngineSingleton::Register_WAngelScriptClass()
{
  AS_CHECK(m_pEngine->RegisterInterface("WIAngelScriptClass"));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("WGameObject@ GetScriptOwnerObject(WIAngelScriptClass@ self)", asFUNCTION(GetAngelScriptOwnerObject), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WScriptComponent@ GetScriptOwnerComponent(WIAngelScriptClass@ self)", asFUNCTION(GetAngelScriptOwnerComponent), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("WWorld@ GetScriptOwnerWorld(WIAngelScriptClass@ self)", asFUNCTION(GetAngelScriptOwnerWorld), asCALL_CDECL));

  const char* szClassCode = R"(
shared class WAngelScriptClass : WIAngelScriptClass
{
    WScriptComponent@ GetOwnerComponent()
    {
        return GetScriptOwnerComponent(@this);
    }

    WGameObject@ GetOwner()
    {
        return GetScriptOwnerObject(@this);
    }

    WWorld@ GetWorld()
    {
        return GetScriptOwnerWorld(@this);
    }

    void SetUpdateInterval(WTime interval)
    {
      GetScriptOwnerComponent(@this).UpdateInterval = interval;
    }
}
    )";

  if (SetModuleCode("Builtin_AngelScriptClass", szClassCode, false) == nullptr)
  {
    W_REPORT_FAILURE("Failed to register WAngelScriptClass class");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("WScriptComponent", "void BroadcastEventMsg(const WMessage& in msg)", asMETHOD(WScriptComponent, BroadcastEventMsg), asCALL_THISCALL));
}
