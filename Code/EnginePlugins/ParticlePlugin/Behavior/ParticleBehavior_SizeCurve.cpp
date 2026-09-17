#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_SizeCurve.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_SizeCurve, 2, WRTTIDefaultAllocator<WParticleBehaviorFactory_SizeCurve>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("ChangeSizeWith", WCurveSource, m_CurveSource),
    W_MEMBER_PROPERTY("SizeCurve", m_Curve),
    W_RESOURCE_MEMBER_PROPERTY("SharedSizeCurve", m_hSharedCurve)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    W_MEMBER_PROPERTY("SizeCurveOffset", m_fSizeCurveOffset)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("SizeCurveScale", m_fSizeCurveScale)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, WVariant())),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_SizeCurve, 1, WRTTIDefaultAllocator<WParticleBehavior_SizeCurve>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const WRTTI* WParticleBehaviorFactory_SizeCurve::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_SizeCurve>();
}

void WParticleBehaviorFactory_SizeCurve::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_SizeCurve* pBehavior = static_cast<WParticleBehavior_SizeCurve*>(pObject);

  pBehavior->m_fSizeCurveOffset = m_fSizeCurveOffset;
  pBehavior->m_fSizeCurveScale = m_fSizeCurveScale;
  pBehavior->m_pCurve = &m_RuntimeCurve;
}

void WParticleBehaviorFactory_SizeCurve::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 2;
  inout_stream << uiVersion;

  inout_stream << m_CurveSource;
  inout_stream << m_hSharedCurve;
  inout_stream << m_fSizeCurveOffset;
  inout_stream << m_fSizeCurveScale;

  m_Curve.ConvertToRuntimeData(m_RuntimeCurve);
  m_RuntimeCurve.SortControlPoints();
  m_RuntimeCurve.Save(inout_stream);
}

void WParticleBehaviorFactory_SizeCurve::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  if (uiVersion >= 2)
  {
    inout_stream >> m_CurveSource;
    inout_stream >> m_hSharedCurve;
  }
  else
  {
    // Version 1: had m_hCurve as the shared curve resource
    WCurve1DResourceHandle hOldCurve;
    inout_stream >> hOldCurve;

    // Migrate to version 2: default to Shared mode with the old curve
    m_CurveSource = WCurveSource::SharedCurve;
    m_hSharedCurve = hOldCurve;
  }

  inout_stream >> m_fSizeCurveOffset;
  inout_stream >> m_fSizeCurveScale;

  if (uiVersion >= 2)
  {
    m_RuntimeCurve.Load(inout_stream);
    m_RuntimeCurve.SortControlPoints(); // also updates the aabb
    m_RuntimeCurve.CreateLinearApproximation();
  }

  if (m_CurveSource == WCurveSource::SharedCurve && m_hSharedCurve.IsValid())
  {
    WResourceLock<WCurve1DResource> pCurveResource(m_hSharedCurve, WResourceAcquireMode::BlockTillLoaded);
    if (pCurveResource.GetAcquireResult() == WResourceAcquireResult::Final && !pCurveResource->GetDescriptor().m_Curves.IsEmpty())
    {
      m_RuntimeCurve = pCurveResource->GetDescriptor().m_Curves[0];
    }
  }
}

void WParticleBehavior_SizeCurve::CreateRequiredStreams()
{
  CreateStream("LifeTime", WProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Size", WProcessingStream::DataType::Half, &m_pStreamSize, false);
}


void WParticleBehavior_SizeCurve::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  WProcessingStreamIterator<WFloat16> itSize(m_pStreamSize, uiNumElements, uiStartIndex);
  while (!itSize.HasReachedEnd())
  {
    itSize.Current() = m_fSizeCurveOffset;
    itSize.Advance();
  }
}

void WParticleBehavior_SizeCurve::Process(WUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // When the effect is not visible, drastically reduce the update frequency
    // to save CPU, but still update occasionally since size changes can affect
    // visibility culling.
    m_uiCurrentUpdateInterval = 32;
  }
  else
  {
    m_uiCurrentUpdateInterval = 2;
  }

  if (m_pCurve == nullptr || m_pCurve->IsEmpty())
    return;

  W_PROFILE_SCOPE("PFX: Size Curve");

  WProcessingStreamIterator<WFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  WProcessingStreamIterator<WFloat16> itSize(m_pStreamSize, uiNumElements, 0);

  double fMinX, fMaxX;
  m_pCurve->QueryExtents(fMinX, fMaxX);

  // make sure the curve has a length of at least 1
  fMinX = WMath::Min(fMinX, 0.0);
  fMaxX = WMath::Max(fMaxX, 1.0);

  // skip the first n particles
  itLifeTime.Advance(m_uiFirstToUpdate);
  itSize.Advance(m_uiFirstToUpdate);

  while (!itLifeTime.HasReachedEnd())
  {
    // if (itLifeTime.Current().y > 0)
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;

      const double evalPos = WMath::Lerp(fMaxX, fMinX, fLifeTimeFraction);
      const float val = (float)m_pCurve->Evaluate(evalPos);

      itSize.Current() = m_fSizeCurveOffset + val * m_fSizeCurveScale;
    }

    // skip the next n items
    // this is to reduce the number of particles that need to be fully evaluated,
    // since sampling the curve is expensive
    itLifeTime.Advance(m_uiCurrentUpdateInterval);
    itSize.Advance(m_uiCurrentUpdateInterval);
  }

  // adjust which index is the first to update
  {
    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_SizeCurve);
