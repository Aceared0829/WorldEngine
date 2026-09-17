#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/BlackboardComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RmlUiPlugin/Components/RmlUiCanvasComponentBase.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WRmlUiCanvasComponentBase, 2)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("RmlFile", GetRmlResource, SetRmlResource)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Rml_UI"), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("AutobindBlackboards", GetAutobindBlackboards, SetAutobindBlackboards),
    W_ACCESSOR_PROPERTY("SendEventMessage", GetSendEventMessage, SetSendEventMessage),
    W_ACCESSOR_PROPERTY("OnDemandUpdate", GetOnDemandUpdate, SetOnDemandUpdate)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgRmlUiReload, OnMsgReload)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Input/RmlUi"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

static WAtomicInteger32 s_RmlContextIdCounter;

WRmlUiCanvasComponentBase::WRmlUiCanvasComponentBase() = default;
WRmlUiCanvasComponentBase::~WRmlUiCanvasComponentBase() = default;
WRmlUiCanvasComponentBase& WRmlUiCanvasComponentBase::operator=(WRmlUiCanvasComponentBase&& rhs) = default;

void WRmlUiCanvasComponentBase::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s << m_bAutobindBlackboards;
  s << m_bSendEventMessage;
  s << m_bOnDemandUpdate;
}

void WRmlUiCanvasComponentBase::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  s >> m_bAutobindBlackboards;

  if (uiVersion >= 2)
  {
    s >> m_bSendEventMessage;
  }

  s >> m_bOnDemandUpdate;
}

void WRmlUiCanvasComponentBase::Initialize()
{
  SUPER::Initialize();

  UpdateAutobinding();
}

void WRmlUiCanvasComponentBase::Deinitialize()
{
  SUPER::Deinitialize();

  if (m_pContext != nullptr)
  {
    WRmlUi::GetSingleton()->DeleteContext(m_pContext);
    m_pContext = nullptr;
  }

  m_DataBindings.Clear();
}

void WRmlUiCanvasComponentBase::OnDeactivated()
{
  m_pContext->HideDocument();

  SUPER::OnDeactivated();
}

void WRmlUiCanvasComponentBase::Update()
{
  if (m_pContext == nullptr)
    return;

  const WTime tDiff = WClock::GetGlobalClock()->GetTimeDiff();
  m_bNeedsUpdate |= m_pContext->GetNextUpdateDelay() < WMath::Max(tDiff.GetSeconds(), 1.0 / 240.0);

  for (auto& pDataBinding : m_DataBindings)
  {
    if (pDataBinding != nullptr)
    {
      m_bNeedsUpdate |= pDataBinding->Update();
    }
  }

  if (m_bNeedsUpdate || m_bOnDemandUpdate == false)
  {
    m_pContext->Update();

    m_bNeedsUpdate = false;
  }
}

bool WRmlUiCanvasComponentBase::ReceiveInput(const WVec2& vMousePosInsideCanvas, WRmlUiInputSnapshot input)
{
  if (m_pContext == nullptr)
    return false;

  m_InputProvider.Update(input);
  m_bNeedsUpdate |= m_pContext->UpdateInput(vMousePosInsideCanvas, m_InputProvider);

  return true;
}

void WRmlUiCanvasComponentBase::SetRmlResource(const WRmlUiResourceHandle& hResource)
{
  if (m_hResource != hResource)
  {
    m_hResource = hResource;

    if (m_pContext != nullptr)
    {
      if (m_pContext->LoadDocumentFromResource(m_hResource).Succeeded() && IsActive())
      {
        m_pContext->ShowDocument();
      }

      UpdateCachedValues();
    }
  }
}

void WRmlUiCanvasComponentBase::SetAutobindBlackboards(bool bAutobind)
{
  if (m_bAutobindBlackboards != bAutobind)
  {
    m_bAutobindBlackboards = bAutobind;

    UpdateAutobinding();
  }
}

void WRmlUiCanvasComponentBase::SetSendEventMessage(bool bSendEventMessage)
{
  if (m_bSendEventMessage != bSendEventMessage)
  {
    m_bSendEventMessage = bSendEventMessage;

    UpdateEventHandler();
  }
}

void WRmlUiCanvasComponentBase::SetOnDemandUpdate(bool bOnDemandUpdate)
{
  m_bOnDemandUpdate = bOnDemandUpdate;
  m_bNeedsUpdate = true;
}

WUInt32 WRmlUiCanvasComponentBase::AddDataBinding(WUniquePtr<WRmlUiDataBinding>&& pDataBinding)
{
  // Document needs to be loaded again since data bindings have to be set before document load
  if (m_pContext != nullptr)
  {
    if (pDataBinding->Initialize(*m_pContext).Succeeded())
    {
      if (m_pContext->LoadDocumentFromResource(m_hResource).Succeeded() && IsActive())
      {
        m_pContext->ShowDocument();
      }
    }
  }

  for (WUInt32 i = 0; i < m_DataBindings.GetCount(); ++i)
  {
    if (pDataBinding == nullptr)
    {
      m_DataBindings[i] = std::move(pDataBinding);
      return i;
    }
  }

  WUInt32 uiDataBindingIndex = m_DataBindings.GetCount();
  m_DataBindings.PushBack(std::move(pDataBinding));
  return uiDataBindingIndex;
}

void WRmlUiCanvasComponentBase::RemoveDataBinding(WUInt32 uiDataBindingIndex)
{
  auto& pDataBinding = m_DataBindings[uiDataBindingIndex];

  if (m_pContext != nullptr)
  {
    pDataBinding->Deinitialize(*m_pContext);
  }

  m_DataBindings[uiDataBindingIndex] = nullptr;
}

WUInt32 WRmlUiCanvasComponentBase::AddBlackboardBinding(const WSharedPtr<WBlackboard>& pBlackboard)
{
  auto pDataBinding = W_DEFAULT_NEW(WRmlUiInternal::BlackboardDataBinding, pBlackboard);
  return AddDataBinding(pDataBinding);
}

void WRmlUiCanvasComponentBase::RemoveBlackboardBinding(WUInt32 uiDataBindingIndex)
{
  RemoveDataBinding(uiDataBindingIndex);
}

WResult WRmlUiCanvasComponentBase::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return W_SUCCESS;
}

WRmlUiContext* WRmlUiCanvasComponentBase::GetOrCreateRmlContext()
{
  if (m_pContext != nullptr)
  {
    return m_pContext;
  }

  WStringBuilder sName = "RmlUi_";
  if (m_hResource.IsValid())
  {
    WResourceLock<WRmlUiResource> pResource(m_hResource, WResourceAcquireMode::BlockTillLoaded);

    WStringView sResourceID = pResource->GetResourceDescription();
    sName.Append(sResourceID.GetFileName());
  }

  if (m_uiContextID == 0)
  {
    m_uiContextID = s_RmlContextIdCounter.Increment();
  }

  sName.AppendFormat("_{}", m_uiContextID);

  m_pContext = WRmlUi::GetSingleton()->CreateContext(sName, m_vSize);
  W_ASSERT_DEV(m_pContext != nullptr, "RML UI context creation failed");

  for (auto& pDataBinding : m_DataBindings)
  {
    pDataBinding->Initialize(*m_pContext).IgnoreResult();
  }

  m_pContext->LoadDocumentFromResource(m_hResource).IgnoreResult();

  UpdateCachedValues();
  UpdateEventHandler();

  return m_pContext;
}

void WRmlUiCanvasComponentBase::OnMsgReload(WMsgRmlUiReload& msg)
{
  if (m_pContext != nullptr)
  {
    m_pContext->ReloadDocumentFromResource(m_hResource).IgnoreResult();
    m_pContext->ShowDocument();

    UpdateCachedValues();
  }
}

void WRmlUiCanvasComponentBase::UpdateCachedValues()
{
  m_ResourceEventUnsubscriber.Unsubscribe();
  m_vReferenceResolution.SetZero();

  if (m_hResource.IsValid())
  {
    WResourceLock pResource(m_hResource, WResourceAcquireMode::BlockTillLoaded);

    if (pResource->GetScaleMode() == WRmlUiScaleMode::WithScreenSize)
    {
      m_vReferenceResolution = pResource->GetReferenceResolution();
    }

    pResource->m_ResourceEvents.AddEventHandler(
      [hComponent = GetHandle(), pWorld = GetWorld()](const WResourceEvent& e)
      {
        if (e.m_Type == WResourceEvent::Type::ResourceContentUnloading)
        {
          pWorld->PostMessage(hComponent, WMsgRmlUiReload(), WTime::MakeZero());
        }
      },
      m_ResourceEventUnsubscriber);
  }
}

void WRmlUiCanvasComponentBase::UpdateAutobinding()
{
  for (WUInt32 uiIndex : m_AutoBindings)
  {
    RemoveDataBinding(uiIndex);
  }

  m_AutoBindings.Clear();

  if (m_bAutobindBlackboards)
  {
    WTempHybridArray<WBlackboardComponent*, 4> blackboardComponents;

    WGameObject* pObject = GetOwner();
    while (pObject != nullptr)
    {
      pObject->TryGetComponentsOfBaseType(blackboardComponents);

      for (auto pBlackboardComponent : blackboardComponents)
      {
        pBlackboardComponent->EnsureInitialized();

        m_AutoBindings.PushBack(AddBlackboardBinding(pBlackboardComponent->GetBoard()));
      }

      pObject = pObject->GetParent();
    }
  }
}

void WRmlUiCanvasComponentBase::UpdateEventHandler()
{
  if (m_pContext != nullptr)
  {
    if (m_bSendEventMessage)
    {
      m_pContext->RegisterFallbackEventHandler(
        [hComponent = GetHandle()](const WHashedString& sIdentifier, Rml::Event& event)
        {
          WRmlUiCanvasComponentBase* pComponent = nullptr;
          if (WWorld::GetWorld(hComponent)->TryGetComponent(hComponent, pComponent))
          {
            pComponent->EventHandler(sIdentifier, event);
          }
        });
    }
    else
    {
      m_pContext->DeregisterFallbackEventHandler();
    }
  }
}

void WRmlUiCanvasComponentBase::EventHandler(const WHashedString& sIdentifier, Rml::Event& event)
{
  WMsgRmlUiEvent msg;
  msg.m_sIdentifier = sIdentifier;
  msg.m_sType.Assign(WRmlUiConversionUtils::ToStringView(event.GetType()));

  m_EventMessageSender.SendEventMessage(msg, this, GetOwner());
}


W_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvasComponentBase);
