#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Opacity.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_Opacity, 2, WRTTIDefaultAllocator<WParticleBehaviorFactory_Opacity>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("ChangeOpacityWith", WCurveSource, m_CurveSource),
    W_MEMBER_PROPERTY("OpacityCurve", m_Curve),
    W_RESOURCE_MEMBER_PROPERTY("SharedOpacityCurve", m_hSharedCurve)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_Opacity, 1, WRTTIDefaultAllocator<WParticleBehavior_Opacity>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const WRTTI* WParticleBehaviorFactory_Opacity::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_Opacity>();
}

void WParticleBehaviorFactory_Opacity::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_Opacity* pBehavior = static_cast<WParticleBehavior_Opacity*>(pObject);

  pBehavior->m_pCurve = &m_RuntimeCurve;
}

void WParticleBehaviorFactory_Opacity::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_CurveSource;
  inout_stream << m_hSharedCurve;

  m_Curve.ConvertToRuntimeData(m_RuntimeCurve);
  m_RuntimeCurve.SortControlPoints();
  m_RuntimeCurve.Save(inout_stream);
}

void WParticleBehaviorFactory_Opacity::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_CurveSource;
  inout_stream >> m_hSharedCurve;

  m_RuntimeCurve.Load(inout_stream);
  m_RuntimeCurve.SortControlPoints(); // also updates the aabb
  m_RuntimeCurve.CreateLinearApproximation();

  if (m_CurveSource == WCurveSource::SharedCurve && m_hSharedCurve.IsValid())
  {
    WResourceLock<WCurve1DResource> pCurveResource(m_hSharedCurve, WResourceAcquireMode::BlockTillLoaded);
    if (pCurveResource.GetAcquireResult() == WResourceAcquireResult::Final && !pCurveResource->GetDescriptor().m_Curves.IsEmpty())
    {
      m_RuntimeCurve = pCurveResource->GetDescriptor().m_Curves[0];
    }
  }
}

void WParticleBehavior_Opacity::CreateRequiredStreams()
{
  CreateStream("LifeTime", WProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Color", WProcessingStream::DataType::Half4, &m_pStreamColor, false);
}

void WParticleBehavior_Opacity::Process(WUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // When invisible, don't update at all. Set the interval to 1 so that once
    // the effect becomes visible, all particles get fully updated on the next frame.
    m_uiCurrentUpdateInterval = 1;
    m_uiFirstToUpdate = 0;
    return;
  }

  if (m_pCurve == nullptr || m_pCurve->IsEmpty())
    return;

  W_PROFILE_SCOPE("PFX: Opacity");

  WProcessingStreamIterator<WFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  WProcessingStreamIterator<WColorLinear16f> itColor(m_pStreamColor, uiNumElements, 0);

  double fMinX, fMaxX;
  m_pCurve->QueryExtents(fMinX, fMaxX);

  // make sure the curve has a length of at least 1
  fMinX = WMath::Min(fMinX, 0.0);
  fMaxX = WMath::Max(fMaxX, 1.0);

  // skip the first n particles
  itLifeTime.Advance(m_uiFirstToUpdate);
  itColor.Advance(m_uiFirstToUpdate);

  while (!itLifeTime.HasReachedEnd())
  {
    // particle age: 0 at birth, increases to 1 at death
    const float fInvParticleAge = itLifeTime.Current().x * itLifeTime.Current().y;

    const double evalPos = WMath::Lerp(fMaxX, fMinX, fInvParticleAge);
    const float fOpacity = (float)m_pCurve->Evaluate(evalPos);

    itColor.Current().a = WMath::Clamp(fOpacity, 0.0f, 1.0f);

    // skip the next n items
    // this is to reduce the number of particles that need to be fully evaluated,
    // since sampling the curve is expensive
    itLifeTime.Advance(m_uiCurrentUpdateInterval);
    itColor.Advance(m_uiCurrentUpdateInterval);
  }

  // adjust which index is the first to update
  {
    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }

  // reset the update interval to the default
  m_uiCurrentUpdateInterval = 2;
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Opacity);
