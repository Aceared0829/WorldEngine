#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/Type/Effect/ParticleTypeEffect.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypeEffectFactory, 1, WRTTIDefaultAllocator<WParticleTypeEffectFactory>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Effect", m_sEffect)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Particle_Effect"), new WRequiredAttribute()),
    // W_MEMBER_PROPERTY("Shared Instance Name", m_sSharedInstanceName), // there is currently no way (I can think of) to uniquely identify each sub-system for the 'shared owner'
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypeEffect, 1, WRTTIDefaultAllocator<WParticleTypeEffect>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleTypeEffectFactory::WParticleTypeEffectFactory() = default;
WParticleTypeEffectFactory::~WParticleTypeEffectFactory() = default;

const WRTTI* WParticleTypeEffectFactory::GetTypeType() const
{
  return WGetStaticRTTI<WParticleTypeEffect>();
}

void WParticleTypeEffectFactory::CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const
{
  WParticleTypeEffect* pType = static_cast<WParticleTypeEffect*>(pObject);

  pType->m_hEffect = m_hEffect;

  // pType->m_sSharedInstanceName = m_sSharedInstanceName;


  if (bFirstTime)
  {
    pType->GetOwnerSystem()->AddParticleDeathEventHandler(WMakeDelegate(&WParticleTypeEffect::OnParticleDeath, pType));
  }
}

enum class TypeEffectVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleTypeEffectFactory::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)TypeEffectVersion::Version_Current;
  inout_stream << uiVersion;

  WUInt64 m_uiRandomSeed = 0;

  inout_stream << m_sEffect;
  inout_stream << m_uiRandomSeed;
  inout_stream << m_sSharedInstanceName;
}

void WParticleTypeEffectFactory::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)TypeEffectVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_sEffect;

  if (uiVersion >= 2)
  {
    WUInt64 m_uiRandomSeed = 0;

    inout_stream >> m_uiRandomSeed;
    inout_stream >> m_sSharedInstanceName;
  }

  if (!m_sEffect.IsEmpty())
  {
    m_hEffect = WResourceManager::LoadResource<WParticleEffectResource>(m_sEffect);
  }
}


WParticleTypeEffect::WParticleTypeEffect() = default;

WParticleTypeEffect::~WParticleTypeEffect()
{
  if (m_pStreamPosition != nullptr)
  {
    GetOwnerSystem()->RemoveParticleDeathEventHandler(WMakeDelegate(&WParticleTypeEffect::OnParticleDeath, this));

    ClearEffects(true);
  }
}

void WParticleTypeEffect::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("EffectID", WProcessingStream::DataType::Int, &m_pStreamEffectID, false);
}

void WParticleTypeEffect::ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const
{
  W_PROFILE_SCOPE("PFX: Effect");

  const WUInt32 numParticles = (WUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  const WUInt32* pEffectID = m_pStreamEffectID->GetData<WUInt32>();

  const WParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  for (WUInt32 i = 0; i < numParticles; ++i)
  {
    WParticleEffectHandle hInstance = WParticleEffectHandle(WParticleEffectId(pEffectID[i]));

    const WParticleEffectInstance* pEffect = nullptr;
    if (pWorldModule->TryGetEffectInstance(hInstance, pEffect))
    {
      pWorldModule->ExtractEffectRenderData(pEffect, ref_msg, pEffect->GetTransform());
    }
  }
}

void WParticleTypeEffect::OnReset()
{
  ClearEffects(true);
}

void WParticleTypeEffect::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Effect");

  if (!m_hEffect.IsValid())
    return;

  const WVec4* pPosition = m_pStreamPosition->GetData<WVec4>();
  WUInt32* pEffectID = m_pStreamEffectID->GetWritableData<WUInt32>();

  WParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  m_fMaxEffectRadius = 0.0f;

  const WUInt64 uiRandomSeed = GetOwnerEffect()->GetRandomSeed();

  for (WUInt32 i = 0; i < uiNumElements; ++i)
  {
    if (pEffectID[i] == 0) // always an invalid ID
    {
      const void* pDummy = nullptr;
      WParticleEffectHandle hInstance = pWorldModule->CreateEffectInstance(m_hEffect, uiRandomSeed, /*m_sSharedInstanceName*/ nullptr, pDummy, WArrayPtr<WParticleEffectFloatParam>(), WArrayPtr<WParticleEffectColorParam>());

      pEffectID[i] = hInstance.GetInternalID().m_Data;
    }

    WParticleEffectHandle hInstance = WParticleEffectHandle(WParticleEffectId(pEffectID[i]));

    WParticleEffectInstance* pEffect = nullptr;
    if (pWorldModule->TryGetEffectInstance(hInstance, pEffect))
    {
      WTransform t;
      t.m_qRotation.SetIdentity();
      t.m_vScale.Set(1.0f);
      t.m_vPosition = pPosition[i].GetAsVec3();

      // TODO: pass through velocity
      pEffect->SetVisibleIf(GetOwnerEffect());
      pEffect->SetTransformForNextFrame(t, WVec3::MakeZero());

      WBoundingBoxSphere bounds;
      pEffect->GetBoundingVolume(bounds);

      m_fMaxEffectRadius = WMath::Max(m_fMaxEffectRadius, bounds.m_fSphereRadius);
    }
  }
}

void WParticleTypeEffect::OnParticleDeath(const WStreamGroupElementRemovedEvent& e)
{
  WParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  const WUInt32* pEffectID = m_pStreamEffectID->GetData<WUInt32>();

  WParticleEffectHandle hInstance = WParticleEffectHandle(WParticleEffectId(pEffectID[e.m_uiElementIndex]));

  pWorldModule->DestroyEffectInstance(hInstance, false, nullptr);
}

void WParticleTypeEffect::ClearEffects(bool bInterruptImmediately)
{
  // delete all effects that are still in the processing group

  WParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();
  const WUInt64 uiNumParticles = GetOwnerSystem()->GetNumActiveParticles();

  if (uiNumParticles == 0 || m_pStreamEffectID == nullptr)
    return;

  WUInt32* pEffectID = m_pStreamEffectID->GetWritableData<WUInt32>();

  for (WUInt32 elemIdx = 0; elemIdx < uiNumParticles; ++elemIdx)
  {
    WParticleEffectHandle hInstance = WParticleEffectHandle(WParticleEffectId(pEffectID[elemIdx]));
    pEffectID[elemIdx] = 0;

    pWorldModule->DestroyEffectInstance(hInstance, bInterruptImmediately, nullptr);
  }
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Effect_ParticleTypeEffect);
