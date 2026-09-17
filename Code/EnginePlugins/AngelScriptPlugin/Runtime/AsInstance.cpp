#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/Scripting/ScriptComponent.h>

WAngelScriptInstance::WAngelScriptInstance(WReflectedClass& inout_owner, WWorld* pWorld, asIScriptModule* pModule, const char* szObjectTypeName)
  : WScriptInstance(inout_owner, pWorld)
{
  W_ASSERT_DEBUG(inout_owner.GetDynamicRTTI()->IsDerivedFrom<WComponent>(), "Invalid owner");

  m_pOwnerComponent = static_cast<WScriptComponent*>(&inout_owner);

  auto pAsEngine = WAngelScriptEngineSingleton::GetSingleton();

  m_pContext = pAsEngine->GetEngine()->CreateContext();
  AS_CHECK(m_pContext->SetExceptionCallback(asMETHOD(WAngelScriptInstance, ExceptionCallback), this, asCALL_THISCALL));

  if (asITypeInfo* pClassType = pModule->GetTypeInfoByName(szObjectTypeName))
  {
    if (auto pFactory = pClassType->GetFactoryByIndex(0))
    {
      AS_CHECK(m_pContext->Prepare(pFactory));
      AS_CHECK(m_pContext->Execute());

      m_pObject = (asIScriptObject*)m_pContext->GetReturnObject();

      if (m_pObject)
      {
        m_pObject->AddRef();
        m_pObject->SetUserData(this, WAsUserData::ScriptInstancePtr);
        return;
      }
    }
  }

  WLog::Error("Failed to create AngelScript object of type '{}'", szObjectTypeName);
}

WAngelScriptInstance::~WAngelScriptInstance()
{
  if (m_pObject)
  {
    m_pObject->Release();
    m_pObject = nullptr;
  }

  if (m_pContext)
  {
    W_ASSERT_DEBUG(!m_pContext->IsNested(), "Invalid time to release context!");

    m_pContext->Release();
    m_pContext = nullptr;
  }
}

void WAngelScriptInstance::SetInstanceVariable(const WHashedString& sName, const WVariant& value)
{
  if (!m_pObject)
    return;

  // TODO AngelScript: this could be a more efficient lookup instead of a search

  for (WUInt32 i = 0; i < m_pObject->GetPropertyCount(); ++i)
  {
    if (sName == m_pObject->GetPropertyName(i))
    {
      const int typeId = m_pObject->GetPropertyTypeId(i);
      void* pProp = m_pObject->GetAddressOfProperty(i);

      WAngelScriptUtils::WriteToAsTypeAtLocation(m_pObject->GetEngine(), typeId, pProp, value).AssertSuccess();
      return;
    }
  }

  WLog::Error("The variable '{}' doesn't exist in the Angel Script.", sName);
}

WVariant WAngelScriptInstance::GetInstanceVariable(const WHashedString& sName)
{
  W_ASSERT_NOT_IMPLEMENTED;
  return {};
}

void WAngelScriptInstance::ExceptionCallback(asIScriptContext* pContext)
{
  W_LOG_BLOCK("AS Exception", m_pOwnerComponent->GetScriptClass().GetResourceIdOrDescription());

  WLog::Error("AS Exception '{}' - in '{}'", pContext->GetExceptionString(), m_pOwnerComponent->GetScriptClass().GetResourceIdOrDescription());

  const WUInt32 uiNumLevels = pContext->GetCallstackSize();

  for (WUInt32 i = 0; i < uiNumLevels; ++i)
  {
    if (asIScriptFunction* pFunc = pContext->GetFunction(i))
    {
      WStringBuilder line("  ");

      if (!WStringUtils::IsNullOrEmpty(pFunc->GetNamespace()))
      {
        line.Append(pFunc->GetNamespace(), "::");
      }

      if (!WStringUtils::IsNullOrEmpty(pFunc->GetObjectName()))
      {
        line.Append(pFunc->GetObjectName(), "::");
      }

      const char* szSection = nullptr;
      int lineNbr = pContext->GetLineNumber(i, nullptr, &szSection);

      line.AppendFormat("{}() - Line {} in '{}'", pFunc->GetName(), lineNbr, szSection);

      WLog::Error(line);
    }
    else
    {
      WLog::Error("  <nested call>");
    }
  }
}
