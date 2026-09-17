#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Velocity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WVelocityChangeMode, 1)
  W_ENUM_CONSTANT(WVelocityChangeMode::CustomCurve),
  W_ENUM_CONSTANT(WVelocityChangeMode::SharedCurve),
  W_ENUM_CONSTANT(WVelocityChangeMode::Friction),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_Velocity, 2, WRTTIDefaultAllocator<WParticleBehaviorFactory_Velocity>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("ChangeSpeedWith", WVelocityChangeMode, m_ChangeSpeedWith),
    W_MEMBER_PROPERTY("Friction", m_fFriction)->AddAttributes(new WClampValueAttribute(0.0f, 100.0f)),
    W_MEMBER_PROPERTY("SpeedCurve", m_SpeedCurve),
    W_RESOURCE_MEMBER_PROPERTY("SharedSpeedCurve", m_hSpeedSharedCurve)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    W_MEMBER_PROPERTY("SpeedCurveOffset", m_fSpeedCurveOffset)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("SpeedCurveScale", m_fSpeedCurveScale)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, WVariant())),

  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_Velocity, 1, WRTTIDefaultAllocator<WParticleBehavior_Velocity>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleBehaviorFactory_Velocity::WParticleBehaviorFactory_Velocity() = default;
WParticleBehaviorFactory_Velocity::~WParticleBehaviorFactory_Velocity() = default;

const WRTTI* WParticleBehaviorFactory_Velocity::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_Velocity>();
}

void WParticleBehaviorFactory_Velocity::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_Velocity* pBehavior = static_cast<WParticleBehavior_Velocity*>(pObject);

  pBehavior->m_fFriction = m_fFriction;
  pBehavior->m_ChangeSpeedWith = m_ChangeSpeedWith;
  pBehavior->m_fSpeedCurveOffset = m_fSpeedCurveOffset;
  pBehavior->m_fSpeedCurveScale = m_fSpeedCurveScale;
  pBehavior->m_pCurve = &m_RuntimeSpeedCurve;
}

enum class BehaviorVelocityVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added rise speed and acceleration
  Version_3, // added wind influence
  Version_4, // added speed curves
  Version_5, // removed wind influence (migrated to Wind behavior)
  Version_6, // removed rise speed

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleBehaviorFactory_Velocity::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)BehaviorVelocityVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fFriction;

  // Version 4
  inout_stream << m_ChangeSpeedWith;
  inout_stream << m_hSpeedSharedCurve;
  inout_stream << m_fSpeedCurveOffset;
  inout_stream << m_fSpeedCurveScale;

  m_SpeedCurve.ConvertToRuntimeData(m_RuntimeSpeedCurve);
  m_RuntimeSpeedCurve.SortControlPoints();
  m_RuntimeSpeedCurve.Save(inout_stream);
}

void WParticleBehaviorFactory_Velocity::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)BehaviorVelocityVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion < 6)
  {
    float fRiseSpeed;
    inout_stream >> fRiseSpeed;
  }

  inout_stream >> m_fFriction;

  if (uiVersion >= 3 && uiVersion < 5)
  {
    // wind influence was removed in version 5, but still need to read it from old files
    float fWindInfluence = 0;
    inout_stream >> fWindInfluence;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_ChangeSpeedWith;
    inout_stream >> m_hSpeedSharedCurve;
    inout_stream >> m_fSpeedCurveOffset;
    inout_stream >> m_fSpeedCurveScale;

    m_RuntimeSpeedCurve.Load(inout_stream);
    m_RuntimeSpeedCurve.SortControlPoints(); // also updates the aabb
    m_RuntimeSpeedCurve.CreateLinearApproximation();

    if (m_ChangeSpeedWith == WVelocityChangeMode::SharedCurve && m_hSpeedSharedCurve.IsValid())
    {
      WResourceLock<WCurve1DResource> pCurveResource(m_hSpeedSharedCurve, WResourceAcquireMode::BlockTillLoaded);
      if (pCurveResource.GetAcquireResult() == WResourceAcquireResult::Final && !pCurveResource->GetDescriptor().m_Curves.IsEmpty())
      {
        m_RuntimeSpeedCurve = pCurveResource->GetDescriptor().m_Curves[0];
      }
    }
  }
}

void WParticleBehaviorFactory_Velocity::QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(WGetStaticRTTI<WParticleFinalizerFactory_ApplyVelocity>());
}

void WParticleBehavior_Velocity::CreateRequiredStreams()
{
  CreateStream("Velocity", WProcessingStream::DataType::Half4, &m_pStreamVelocity, false);

  if ((m_ChangeSpeedWith == WVelocityChangeMode::CustomCurve || m_ChangeSpeedWith == WVelocityChangeMode::SharedCurve) && m_pCurve && !m_pCurve->IsEmpty())
  {
    CreateStream("LifeTime", WProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  }
}

void WParticleBehavior_Velocity::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Velocity");

  auto pOwner = GetOwnerEffect();

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  WProcessingStreamIterator<WFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  // Handle curve-based speed changes
  if ((m_ChangeSpeedWith == WVelocityChangeMode::CustomCurve || m_ChangeSpeedWith == WVelocityChangeMode::SharedCurve) && m_pStreamLifeTime)
  {
    double fMinX, fMaxX;
    m_pCurve->QueryExtents(fMinX, fMaxX);

    // make sure the curve has a length of at least 1
    fMinX = WMath::Min(fMinX, 0.0);
    fMaxX = WMath::Max(fMaxX, 1.0);

    WProcessingStreamIterator<WFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);

    while (!itVelocity.HasReachedEnd())
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;

      const double evalPos = WMath::Lerp(fMaxX, fMinX, fLifeTimeFraction);
      const float val = (float)m_pCurve->Evaluate(evalPos);

      WVec4 vel = itVelocity.Current();
      vel.w = m_fSpeedCurveOffset + val * m_fSpeedCurveScale;
      itVelocity.Current() = vel;

      itVelocity.Advance();
      itLifeTime.Advance();
    }
  }
  // Handle friction or no speed changes
  else
  {
    const float fFriction = (m_ChangeSpeedWith == WVelocityChangeMode::Friction) ? WMath::Clamp(m_fFriction, 0.0f, 100.0f) : 0.0f;
    const float fFrictionFactor = WMath::Pow(0.5f, tDiff * fFriction);

    while (!itVelocity.HasReachedEnd())
    {
      WVec4 vel = itVelocity.Current();
      vel.w *= fFrictionFactor;
      itVelocity.Current() = vel;

      itVelocity.Advance();
    }
  }
}

void WParticleBehavior_Velocity::RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<WPhysicsWorldModuleInterface>();
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Velocity);
