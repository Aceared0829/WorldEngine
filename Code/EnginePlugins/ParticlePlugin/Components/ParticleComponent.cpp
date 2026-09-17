#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/WorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

WParticleComponentManager::WParticleComponentManager(WWorld* pWorld)
  : SUPER(pWorld)
{
}

void WParticleComponentManager::Initialize()
{
  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WParticleComponentManager::Update, this);
    desc.m_bOnlyUpdateWhenSimulating = true;
    RegisterUpdateFunction(desc);
  }
}

void WParticleComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

void WParticleComponentManager::UpdatePfxTransformsAndBounds()
{
  for (auto it = this->m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->UpdatePfxTransformAndBounds();

      // This function is called in the post-transform phase so the global bounds and transform have already been calculated at this point.
      // Therefore we need to manually update the global bounds again to ensure correct bounds for culling and rendering.
      pComponent->GetOwner()->UpdateLocalBounds();
      pComponent->GetOwner()->UpdateGlobalBounds();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WParticleComponent, 5, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Particle_Effect", WDependencyFlags::Package), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("SpawnAtStart", m_bSpawnAtStart)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ENUM_MEMBER_PROPERTY("OnFinishedAction", WOnComponentFinishedAction2, m_OnFinishedAction),
    W_MEMBER_PROPERTY("MinRestartDelay", m_MinRestartDelay),
    W_MEMBER_PROPERTY("RestartDelayRange", m_RestartDelayRange),
    W_MEMBER_PROPERTY("RandomSeed", m_uiRandomSeed),
    W_ENUM_MEMBER_PROPERTY("SpawnDirection", WBasisAxis, m_SpawnDirection)->AddAttributes(new WDefaultValueAttribute((WInt32)WBasisAxis::PositiveZ)),
    W_MEMBER_PROPERTY("IgnoreOwnerRotation", m_bIgnoreOwnerRotation),
    W_MEMBER_PROPERTY("SharedInstanceName", m_sSharedInstanceName),
    W_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new WExposedParametersAttribute("Effect"), new WExposeColorAlphaAttribute),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgSetPlaying, OnMsgSetPlaying),
    W_MESSAGE_HANDLER(WMsgInterruptPlaying, OnMsgInterruptPlaying),
    W_MESSAGE_HANDLER(WMsgSetFloatParameter, OnMsgSetFloatParameter),
    W_MESSAGE_HANDLER(WMsgSetColorParameter, OnMsgSetColorParameter),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(StartEffect),
    W_SCRIPT_FUNCTION_PROPERTY(StopEffect),
    W_SCRIPT_FUNCTION_PROPERTY(InterruptEffect),
    W_SCRIPT_FUNCTION_PROPERTY(IsEffectActive),
  }
  W_END_FUNCTIONS;
}
W_END_COMPONENT_TYPE
// clang-format on

WParticleComponent::WParticleComponent() = default;
WParticleComponent::~WParticleComponent() = default;

void WParticleComponent::OnDeactivated()
{
  m_EffectController.Invalidate();
  SetUserFlag(0, false);

  WRenderComponent::OnDeactivated();
}

void WParticleComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  auto& s = inout_stream.GetStream();

  s << m_hEffectResource;
  s << m_bSpawnAtStart;

  // Version 1
  {
    bool bAutoRestart = false;
    s << bAutoRestart;
  }

  s << m_MinRestartDelay;
  s << m_RestartDelayRange;
  s << m_RestartTime;
  s << m_uiRandomSeed;
  s << m_sSharedInstanceName;

  // Version 2
  s << m_FloatParams.GetCount();
  for (WUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
  {
    s << m_FloatParams[i].m_sName;
    s << m_FloatParams[i].m_Value;
  }
  s << m_ColorParams.GetCount();
  for (WUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
  {
    s << m_ColorParams[i].m_sName;
    s << m_ColorParams[i].m_Value;
  }

  // Version 3
  s << m_OnFinishedAction;

  // version 4
  s << m_bIgnoreOwnerRotation;

  // version 5
  s << m_SpawnDirection;

  /// \todo store effect state
}

void WParticleComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  auto& s = inout_stream.GetStream();
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_hEffectResource;
  s >> m_bSpawnAtStart;

  // Version 1
  {
    bool bAutoRestart = false;
    s >> bAutoRestart;
  }

  s >> m_MinRestartDelay;
  s >> m_RestartDelayRange;
  s >> m_RestartTime;
  s >> m_uiRandomSeed;
  s >> m_sSharedInstanceName;

  if (uiVersion >= 2)
  {
    WUInt32 numFloats, numColors;

    s >> numFloats;
    m_FloatParams.SetCountUninitialized(numFloats);

    for (WUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
    {
      s >> m_FloatParams[i].m_sName;
      s >> m_FloatParams[i].m_Value;
    }

    m_bFloatParamsChanged = numFloats > 0;

    s >> numColors;
    m_ColorParams.SetCountUninitialized(numColors);

    for (WUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
    {
      s >> m_ColorParams[i].m_sName;
      s >> m_ColorParams[i].m_Value;
    }

    m_bColorParamsChanged = numColors > 0;
  }

  if (uiVersion >= 3)
  {
    s >> m_OnFinishedAction;
  }

  if (uiVersion >= 4)
  {
    s >> m_bIgnoreOwnerRotation;
  }

  if (uiVersion >= 5)
  {
    s >> m_SpawnDirection;
  }
}

bool WParticleComponent::StartEffect()
{
  // stop any previous effect
  m_EffectController.Invalidate();

  if (m_hEffectResource.IsValid())
  {
    WParticleWorldModule* pModule = GetWorld()->GetOrCreateModule<WParticleWorldModule>();

    m_EffectController.Create(m_hEffectResource, pModule, m_uiRandomSeed, m_sSharedInstanceName, this, m_FloatParams, m_ColorParams);

    UpdatePfxTransformAndBounds();

    m_bFloatParamsChanged = false;
    m_bColorParamsChanged = false;

    return true;
  }

  return false;
}

void WParticleComponent::StopEffect()
{
  m_EffectController.Invalidate();
}

void WParticleComponent::InterruptEffect()
{
  m_EffectController.StopImmediate();
}

bool WParticleComponent::IsEffectActive() const
{
  return m_EffectController.IsAlive();
}

void WParticleComponent::OnMsgSetPlaying(WMsgSetPlaying& ref_msg)
{
  if (ref_msg.m_bPlay)
  {
    StartEffect();
  }
  else
  {
    StopEffect();
  }
}

void WParticleComponent::OnMsgSetFloatParameter(WMsgSetFloatParameter& ref_msg)
{
  SetFloatParameter(ref_msg.m_sParameterName, ref_msg.m_fValue);
}

void WParticleComponent::OnMsgSetColorParameter(WMsgSetColorParameter& ref_msg)
{
  SetColorParameter(ref_msg.m_sParameterName, ref_msg.m_Value);
}

void WParticleComponent::OnMsgInterruptPlaying(WMsgInterruptPlaying& ref_msg)
{
  InterruptEffect();
}

void WParticleComponent::SetParticleEffect(const WParticleEffectResourceHandle& hEffect)
{
  m_EffectController.Invalidate();

  m_hEffectResource = hEffect;

  TriggerLocalBoundsUpdate();
}


void WParticleComponent::SetParticleEffectFile(WStringView sFile)
{
  WParticleEffectResourceHandle hEffect;

  if (!sFile.IsEmpty())
  {
    hEffect = WResourceManager::LoadResource<WParticleEffectResource>(sFile);
  }

  SetParticleEffect(hEffect);
}


WStringView WParticleComponent::GetParticleEffectFile() const
{
  if (!m_hEffectResource.IsValid())
    return "";

  return m_hEffectResource.GetResourceID();
}


WResult WParticleComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  if (m_EffectController.IsAlive())
  {
    WBoundingBoxSphere volume = WBoundingBoxSphere::MakeInvalid();

    m_EffectController.GetBoundingVolume(volume);

    if (volume.IsValid())
    {
      if (m_SpawnDirection != WBasisAxis::PositiveZ)
      {
        const WQuat qRot = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveZ, m_SpawnDirection);
        volume.Transform(qRot.GetAsMat4());
      }

      if (m_bIgnoreOwnerRotation)
      {
        volume.Transform((GetOwner()->GetGlobalRotation().GetInverse()).GetAsMat4());
      }

      ref_bounds = volume;
      return W_SUCCESS;
    }
  }

  return W_FAILURE;
}


void WParticleComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  switch (msg.m_pView->GetCameraUsageHint())
  {
    case WCameraUsageHint::Shadow:
    case WCameraUsageHint::Reflection:
      return;

    default:
      break;
  }

  m_EffectController.ExtractRenderData(msg, GetPfxTransform());
}

void WParticleComponent::OnMsgDeleteGameObject(WMsgDeleteGameObject& msg)
{
  WOnComponentFinishedAction2::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

void WParticleComponent::Update()
{
  if (!m_EffectController.IsAlive() && m_bSpawnAtStart && !GetUserFlag(0))
  {
    if (StartEffect())
    {
      SetUserFlag(0, true); // already spawned

      if (m_EffectController.IsContinuousEffect())
      {
        if (m_bIfContinuousStopRightAway)
        {
          StopEffect();
        }
        else
        {
          SetUserFlag(0, false);
        }
      }
    }
  }

  if (!m_EffectController.IsAlive() && (m_OnFinishedAction == WOnComponentFinishedAction2::Restart))
  {
    const WTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

    if (m_RestartTime == WTime())
    {
      const WTime tDiff = WTime::MakeFromSeconds(GetWorld()->GetRandomNumberGenerator().DoubleMinMax(m_MinRestartDelay.GetSeconds(), m_MinRestartDelay.GetSeconds() + m_RestartDelayRange.GetSeconds()));

      m_RestartTime = tNow + tDiff;
    }
    else if (m_RestartTime <= tNow)
    {
      m_RestartTime = WTime::MakeZero();
      StartEffect();
    }
  }

  if (m_EffectController.IsAlive())
  {
    if (m_bFloatParamsChanged)
    {
      m_bFloatParamsChanged = false;

      for (WUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
      {
        const auto& e = m_FloatParams[i];
        m_EffectController.SetParameter(e.m_sName, e.m_Value);
      }
    }

    if (m_bColorParamsChanged)
    {
      m_bColorParamsChanged = false;

      for (WUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
      {
        const auto& e = m_ColorParams[i];
        m_EffectController.SetParameter(e.m_sName, e.m_Value);
      }
    }

    const WTime tDiff = GetWorld()->GetClock().GetTimeDiff();
    m_EffectController.UpdateWindSamples(tDiff);
    m_EffectController.FindNearbyAttractors(tDiff);
  }
  else
  {
    WOnComponentFinishedAction2::HandleFinishedAction(this, m_OnFinishedAction);
  }
}

const WRangeView<const char*, WUInt32> WParticleComponent::GetParameters() const
{
  return WRangeView<const char*, WUInt32>([this]() -> WUInt32
    { return 0; },
    [this]() -> WUInt32
    { return m_FloatParams.GetCount() + m_ColorParams.GetCount(); },
    [this](WUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const WUInt32& uiIt) -> const char*
    {
      if (uiIt < m_FloatParams.GetCount())
        return m_FloatParams[uiIt].m_sName.GetData();
      else
        return m_ColorParams[uiIt - m_FloatParams.GetCount()].m_sName.GetData();
    });
}

void WParticleComponent::SetParameter(const char* szKey, const WVariant& var)
{
  if (var.CanConvertTo<float>())
  {
    SetFloatParameter(WStringView(szKey), var.ConvertTo<float>());
    return;
  }

  if (var.CanConvertTo<WColor>())
  {
    SetColorParameter(WStringView(szKey), var.ConvertTo<WColor>());
    return;
  }
}

void WParticleComponent::SetFloatParameter(WStringView sName, float fValue)
{
  const WTempHashedString tmp(sName);

  for (WUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
  {
    if (m_FloatParams[i].m_sName == tmp)
    {
      if (m_FloatParams[i].m_Value != fValue)
      {
        m_bFloatParamsChanged = true;
        m_FloatParams[i].m_Value = fValue;
      }
      return;
    }
  }

  m_bFloatParamsChanged = true;
  auto& e = m_FloatParams.ExpandAndGetRef();
  e.m_sName.Assign(sName);
  e.m_Value = fValue;
}

void WParticleComponent::SetColorParameter(WStringView sName, const WColor& value)
{
  const WTempHashedString tmp(sName);

  for (WUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
  {
    if (m_ColorParams[i].m_sName == tmp)
    {
      if (m_ColorParams[i].m_Value != value)
      {
        m_bColorParamsChanged = true;
        m_ColorParams[i].m_Value = value;
      }
      return;
    }
  }

  m_bColorParamsChanged = true;
  auto& e = m_ColorParams.ExpandAndGetRef();
  e.m_sName.Assign(sName);
  e.m_Value = value;
}

void WParticleComponent::RemoveParameter(const char* szKey)
{
  const WTempHashedString th(szKey);

  for (WUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
  {
    if (m_FloatParams[i].m_sName == th)
    {
      m_FloatParams.RemoveAtAndSwap(i);
      return;
    }
  }

  for (WUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
  {
    if (m_ColorParams[i].m_sName == th)
    {
      m_ColorParams.RemoveAtAndSwap(i);
      return;
    }
  }
}

bool WParticleComponent::GetParameter(const char* szKey, WVariant& out_value) const
{
  const WTempHashedString th(szKey);

  for (const auto& e : m_FloatParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  for (const auto& e : m_ColorParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  return false;
}

WTransform WParticleComponent::GetPfxTransform() const
{
  WTransform transform = GetOwner()->GetGlobalTransform();

  const WQuat qRot = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveZ, m_SpawnDirection);

  if (m_bIgnoreOwnerRotation)
  {
    transform.m_qRotation = qRot;
  }
  else
  {
    transform.m_qRotation = transform.m_qRotation * qRot;
  }

  return transform;
}

void WParticleComponent::UpdatePfxTransformAndBounds()
{
  m_EffectController.SetTransform(GetPfxTransform(), GetOwner()->GetLinearVelocity());
  m_EffectController.CombineSystemBoundingVolumes();
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleComponent);
