#include <GameEngine/GameEnginePCH.h>

#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/StateMachineState_Script.h>

namespace
{
  struct ScriptInstanceData
  {
    WReflectedClass* m_pOwner = nullptr;

    WStateMachineInstance* m_pStateMachineInstance = nullptr;
    const WArrayMap<WHashedString, WVariant>* m_pParameters = nullptr;
    WScriptClassResourceHandle m_hScriptClass;

    WSharedPtr<WScriptRTTI> m_pScriptType;
    WUniquePtr<WScriptInstance> m_pInstance;

    ~ScriptInstanceData()
    {
      ClearInstance();
    }

    void InstantiateScript(const WStateMachineState* pFromState = nullptr)
    {
      ClearInstance();

      WResourceLock<WScriptClassResource> pScript(m_hScriptClass, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pScript.GetAcquireResult() != WResourceAcquireResult::Final)
      {
        WLog::Error("Failed to load script '{}'", m_hScriptClass.GetResourceID());
        return;
      }

      auto pScriptType = pScript->GetType();
      if (pScriptType == nullptr || pScriptType->IsDerivedFrom(WGetStaticRTTI<WStateMachineState>()) == false)
      {
        WLog::Error("Script type '{}' is not a state machine state", pScriptType != nullptr ? pScriptType->GetTypeName() : "NULL");
        return;
      }

      m_pScriptType = pScriptType;

      m_pInstance = pScript->Instantiate(*m_pOwner, m_pStateMachineInstance->GetOwnerWorld());
      if (m_pInstance != nullptr)
      {
        m_pInstance->SetInstanceVariables(*m_pParameters);
      }

      if (WWorld* pWorld = m_pStateMachineInstance->GetOwnerWorld())
      {
        pWorld->AddResourceReloadFunction(m_hScriptClass, WComponentHandle(), this,
          [](WWorld::ResourceReloadContext& context)
          {
            static_cast<ScriptInstanceData*>(context.m_pUserData)->ReloadScript();
          });
      }

      CallOnEnter(pFromState);
    }

    void ClearInstance()
    {
      CallOnExit(nullptr);

      if (m_pStateMachineInstance != nullptr)
      {
        if (WWorld* pWorld = m_pStateMachineInstance->GetOwnerWorld())
        {
          auto pModule = pWorld->GetOrCreateModule<WScriptWorldModule>();
          pModule->StopAndDeleteAllCoroutines(m_pInstance.Borrow());

          pWorld->RemoveResourceReloadFunction(m_hScriptClass, WComponentHandle(), this);
        }
      }

      m_pInstance = nullptr;
      m_pScriptType = nullptr;
    }

    void ReloadScript()
    {
      InstantiateScript();
    }

    const WAbstractFunctionProperty* GetScriptFunction(WUInt32 uiFunctionIndex)
    {
      if (m_pScriptType != nullptr && m_pInstance != nullptr)
      {
        return m_pScriptType->GetFunctionByIndex(uiFunctionIndex);
      }

      return nullptr;
    }

    void CallOnEnter(const WStateMachineState* pFromState)
    {
      if (auto pFunction = GetScriptFunction(WStateMachineState_ScriptBaseClassFunctions::OnEnter))
      {
        WVariant args[] = {m_pStateMachineInstance, pFromState};
        WVariant returnValue;
        pFunction->Execute(m_pInstance.Borrow(), WMakeArrayPtr(args), returnValue);
      }
    }

    void CallOnExit(const WStateMachineState* pToState)
    {
      if (auto pFunction = GetScriptFunction(WStateMachineState_ScriptBaseClassFunctions::OnExit))
      {
        WVariant args[] = {m_pStateMachineInstance, pToState};
        WVariant returnValue;
        pFunction->Execute(m_pInstance.Borrow(), WMakeArrayPtr(args), returnValue);
      }
    }

    void CallUpdate(WTime deltaTime)
    {
      if (auto pFunction = GetScriptFunction(WStateMachineState_ScriptBaseClassFunctions::Update))
      {
        WVariant args[] = {m_pStateMachineInstance, deltaTime};
        WVariant returnValue;
        pFunction->Execute(m_pInstance.Borrow(), WMakeArrayPtr(args), returnValue);
      }
    }
  };
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineState_Script, 1, WRTTIDefaultAllocator<WStateMachineState_Script>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("ScriptClass", GetScriptClassFile, SetScriptClassFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_ScriptClass"), new WRequiredAttribute()),
    W_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new WExposedParametersAttribute("ScriptClass")),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachineState_Script::WStateMachineState_Script(WStringView sName)
  : WStateMachineState(sName)
{
}

WStateMachineState_Script::~WStateMachineState_Script() = default;

void WStateMachineState_Script::OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const
{
  auto& instanceData = *static_cast<ScriptInstanceData*>(pInstanceData);

  if (instanceData.m_pInstance == nullptr)
  {
    instanceData.m_pOwner = const_cast<WStateMachineState_Script*>(this);
    instanceData.m_pStateMachineInstance = &ref_instance;
    instanceData.m_pParameters = &m_Parameters;
    instanceData.m_hScriptClass = WResourceManager::LoadResource<WScriptClassResource>(m_sScriptClassFile);

    instanceData.InstantiateScript(pFromState);
  }
  else
  {
    instanceData.CallOnEnter(pFromState);
  }
}

void WStateMachineState_Script::OnExit(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pToState) const
{
  auto& instanceData = *static_cast<ScriptInstanceData*>(pInstanceData);
  instanceData.CallOnExit(pToState);
}

void WStateMachineState_Script::Update(WStateMachineInstance& ref_instance, void* pInstanceData, WTime deltaTime) const
{
  auto& instanceData = *static_cast<ScriptInstanceData*>(pInstanceData);
  instanceData.CallUpdate(deltaTime);
}

WResult WStateMachineState_Script::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sScriptClassFile;

  WUInt16 uiNumParams = static_cast<WUInt16>(m_Parameters.GetCount());
  inout_stream << uiNumParams;

  for (WUInt32 p = 0; p < uiNumParams; ++p)
  {
    inout_stream << m_Parameters.GetKey(p);
    inout_stream << m_Parameters.GetValue(p);
  }

  return W_SUCCESS;
}

WResult WStateMachineState_Script::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_sScriptClassFile;

  WUInt16 uiNumParams = 0;
  inout_stream >> uiNumParams;
  m_Parameters.Reserve(uiNumParams);

  WHashedString key;
  WVariant value;
  for (WUInt32 p = 0; p < uiNumParams; ++p)
  {
    inout_stream >> key;
    inout_stream >> value;

    m_Parameters.Insert(key, value);
  }

  return W_SUCCESS;
}

bool WStateMachineState_Script::GetInstanceDataDesc(WInstanceDataDesc& out_desc)
{
  out_desc.FillFromType<ScriptInstanceData>();
  return true;
}

void WStateMachineState_Script::SetScriptClassFile(const char* szFile)
{
  m_sScriptClassFile = szFile;

  // Note that we can't load the resource here directly. State machine states are instantiated during
  // state machine asset transform but the script class resource overwrites are not known there so the resource load would fail.
}

const char* WStateMachineState_Script::GetScriptClassFile() const
{
  return m_sScriptClassFile;
}

const WRangeView<const char*, WUInt32> WStateMachineState_Script::GetParameters() const
{
  return WRangeView<const char*, WUInt32>([]() -> WUInt32
    { return 0; },
    [this]() -> WUInt32
    { return m_Parameters.GetCount(); },
    [](WUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const WUInt32& uiIt) -> const char*
    { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void WStateMachineState_Script::SetParameter(const char* szKey, const WVariant& value)
{
  WHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != WInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void WStateMachineState_Script::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(WTempHashedString(szKey)))
  {
  }
}

bool WStateMachineState_Script::GetParameter(const char* szKey, WVariant& out_value) const
{
  WUInt32 it = m_Parameters.Find(szKey);

  if (it == WInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}


W_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineState_Script);
