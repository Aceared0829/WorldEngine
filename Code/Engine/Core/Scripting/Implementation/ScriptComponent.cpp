#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WScriptComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("UpdateInterval", GetUpdateInterval, SetUpdateInterval)->AddAttributes(new WClampValueAttribute(WTime::MakeZero(), WVariant())),
    W_ACCESSOR_PROPERTY("UpdateOnlyWhenSimulating", GetUpdateOnlyWhenSimulating, SetUpdateOnlyWhenSimulating)->AddAttributes(new WDefaultValueAttribute(true)),
    W_RESOURCE_ACCESSOR_PROPERTY("ScriptClass", GetScriptClass, SetScriptClass)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_ScriptClass", WDependencyFlags::Package), new WRequiredAttribute()),
    W_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new WExposedParametersAttribute("ScriptClass")),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SetScriptVariable, In, "Name", In, "Value"),
    W_SCRIPT_FUNCTION_PROPERTY(GetScriptVariable, In, "Name"),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Scripting"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WScriptComponent::WScriptComponent() = default;
WScriptComponent::~WScriptComponent() = default;

void WScriptComponent::SerializeComponent(WWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hScriptClass;
  s << m_UpdateInterval;
  s << m_bUpdateOnlyWhenSimulating;

  WUInt16 uiNumParams = static_cast<WUInt16>(m_Parameters.GetCount());
  s << uiNumParams;

  for (WUInt32 p = 0; p < uiNumParams; ++p)
  {
    s << m_Parameters.GetKey(p);
    s << m_Parameters.GetValue(p);
  }
}

void WScriptComponent::DeserializeComponent(WWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hScriptClass;
  s >> m_UpdateInterval;

  if (uiVersion >= 2)
  {
    s >> m_bUpdateOnlyWhenSimulating;
  }

  WUInt16 uiNumParams = 0;
  s >> uiNumParams;
  m_Parameters.Reserve(uiNumParams);

  WHashedString key;
  WVariant value;
  for (WUInt32 p = 0; p < uiNumParams; ++p)
  {
    s >> key;
    s >> value;

    m_Parameters.Insert(key, value);
  }
}

void WScriptComponent::Initialize()
{
  SUPER::Initialize();

  if (m_hScriptClass.IsValid())
  {
    InstantiateScript(false);
  }
}

void WScriptComponent::Deinitialize()
{
  SUPER::Deinitialize();

  ClearInstance(false);
}

void WScriptComponent::OnActivated()
{
  SUPER::OnActivated();

  CallScriptFunction(WComponent_ScriptBaseClassFunctions::OnActivated);

  AddUpdateFunctionToSchedule();
}

void WScriptComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  CallScriptFunction(WComponent_ScriptBaseClassFunctions::OnDeactivated);

  RemoveUpdateFunctionToSchedule();
}

void WScriptComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  CallScriptFunction(WComponent_ScriptBaseClassFunctions::OnSimulationStarted);
}

void WScriptComponent::SetScriptVariable(const WHashedString& sName, const WVariant& value)
{
  if (m_pInstance != nullptr)
  {
    m_pInstance->SetInstanceVariable(sName, value);
  }
}

WVariant WScriptComponent::GetScriptVariable(const WHashedString& sName) const
{
  if (m_pInstance != nullptr)
  {
    return m_pInstance->GetInstanceVariable(sName);
  }

  return WVariant();
}

void WScriptComponent::SetScriptClass(const WScriptClassResourceHandle& hScript)
{
  if (m_hScriptClass == hScript)
    return;

  if (IsInitialized())
  {
    ClearInstance(IsActiveAndInitialized());
  }

  m_hScriptClass = hScript;

  if (IsInitialized() && m_hScriptClass.IsValid())
  {
    InstantiateScript(IsActiveAndInitialized());
  }
}

void WScriptComponent::SetUpdateInterval(WTime interval)
{
  if (m_UpdateInterval == interval)
    return;

  m_UpdateInterval = interval;

  RemoveUpdateFunctionToSchedule();
  AddUpdateFunctionToSchedule();
}

void WScriptComponent::SetUpdateOnlyWhenSimulating(bool bUpdate)
{
  if (m_bUpdateOnlyWhenSimulating == bUpdate)
    return;

  m_bUpdateOnlyWhenSimulating = bUpdate;

  RemoveUpdateFunctionToSchedule();
  AddUpdateFunctionToSchedule();
}

void WScriptComponent::BroadcastEventMsg(WMessage& ref_msg)
{
  const WRTTI* pType = ref_msg.GetDynamicRTTI();

  for (auto& sender : m_EventSenders)
  {
    if (sender.m_pMsgType == pType)
    {
      sender.m_Sender.SendEventMessage(ref_msg, this, GetOwner()->GetParent());
      return;
    }
  }

  auto& sender = m_EventSenders.ExpandAndGetRef();
  sender.m_pMsgType = pType;
  sender.m_Sender.SendEventMessage(ref_msg, this, GetOwner()->GetParent());
}

const WRangeView<const char*, WUInt32> WScriptComponent::GetParameters() const
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

void WScriptComponent::SetParameter(const char* szKey, const WVariant& value)
{
  WHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != WInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  if (IsInitialized() && m_hScriptClass.IsValid())
  {
    InstantiateScript(IsActiveAndInitialized());
  }
}

void WScriptComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(WTempHashedString(szKey)))
  {
    if (IsInitialized() && m_hScriptClass.IsValid())
    {
      InstantiateScript(IsActiveAndInitialized());
    }
  }
}

bool WScriptComponent::GetParameter(const char* szKey, WVariant& out_value) const
{
  WUInt32 it = m_Parameters.Find(szKey);

  if (it == WInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

void WScriptComponent::InstantiateScript(bool bActivate)
{
  ClearInstance(IsActiveAndInitialized());

  WResourceLock<WScriptClassResource> pScript(m_hScriptClass, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pScript.GetAcquireResult() != WResourceAcquireResult::Final)
  {
    WLog::Error("Failed to load script '{}'", GetScriptClass().GetResourceIdOrDescription());
    return;
  }

  auto pScriptType = pScript->GetType();
  if (pScriptType == nullptr || pScriptType->IsDerivedFrom(WGetStaticRTTI<WComponent>()) == false)
  {
    WLog::Error("Script type '{}' is not a component", pScriptType != nullptr ? pScriptType->GetTypeName() : "NULL");
    return;
  }

  m_pScriptType = pScriptType;
  m_pMessageDispatchType = pScriptType;

  m_pInstance = pScript->Instantiate(*this, GetWorld());
  if (m_pInstance != nullptr)
  {
    m_pInstance->SetInstanceVariables(m_Parameters);
  }

  GetWorld()->AddResourceReloadFunction(m_hScriptClass, GetHandle(), nullptr,
    [](const WWorld::ResourceReloadContext& context)
    {
      WStaticCast<WScriptComponent*>(context.m_pComponent)->ReloadScript();
    });

  CallScriptFunction(WComponent_ScriptBaseClassFunctions::Initialize);
  if (bActivate)
  {
    CallScriptFunction(WComponent_ScriptBaseClassFunctions::OnActivated);

    if (GetWorld()->GetWorldSimulationEnabled())
    {
      CallScriptFunction(WComponent_ScriptBaseClassFunctions::OnSimulationStarted);
    }
  }

  AddUpdateFunctionToSchedule();
}

void WScriptComponent::ClearInstance(bool bDeactivate)
{
  if (bDeactivate)
  {
    CallScriptFunction(WComponent_ScriptBaseClassFunctions::OnDeactivated);
  }
  CallScriptFunction(WComponent_ScriptBaseClassFunctions::Deinitialize);

  RemoveUpdateFunctionToSchedule();

  auto pModule = GetWorld()->GetOrCreateModule<WScriptWorldModule>();
  pModule->StopAndDeleteAllCoroutines(m_pInstance.Borrow());

  GetWorld()->RemoveResourceReloadFunction(m_hScriptClass, GetHandle(), nullptr);

  m_pInstance = nullptr;
  m_pScriptType = nullptr;

  m_pMessageDispatchType = GetDynamicRTTI();
}

void WScriptComponent::AddUpdateFunctionToSchedule()
{
  if (IsActiveAndInitialized() == false)
    return;

  auto pModule = GetWorld()->GetOrCreateModule<WScriptWorldModule>();
  if (auto pUpdateFunction = GetScriptFunction(WComponent_ScriptBaseClassFunctions::Update))
  {
    pModule->AddUpdateFunctionToSchedule(pUpdateFunction, m_pInstance.Borrow(), m_UpdateInterval, m_bUpdateOnlyWhenSimulating);
  }
}

void WScriptComponent::RemoveUpdateFunctionToSchedule()
{
  auto pModule = GetWorld()->GetOrCreateModule<WScriptWorldModule>();
  if (auto pUpdateFunction = GetScriptFunction(WComponent_ScriptBaseClassFunctions::Update))
  {
    pModule->RemoveUpdateFunctionToSchedule(pUpdateFunction, m_pInstance.Borrow());
  }
}

const WAbstractFunctionProperty* WScriptComponent::GetScriptFunction(WUInt32 uiFunctionIndex)
{
  if (m_pScriptType != nullptr && m_pInstance != nullptr)
  {
    return m_pScriptType->GetFunctionByIndex(uiFunctionIndex);
  }

  return nullptr;
}

void WScriptComponent::CallScriptFunction(WUInt32 uiFunctionIndex)
{
  if (auto pFunction = GetScriptFunction(uiFunctionIndex))
  {
    WVariant returnValue;
    pFunction->Execute(m_pInstance.Borrow(), WArrayPtr<WVariant>(), returnValue);
  }
}

void WScriptComponent::ReloadScript()
{
  InstantiateScript(IsActiveAndInitialized());
}

W_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptComponent);
