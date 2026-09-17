#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Turbulence.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_Turbulence, 1, WRTTIDefaultAllocator<WParticleBehaviorFactory_Turbulence>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new WDefaultValueAttribute(2.0f)),
    W_MEMBER_PROPERTY("Frequency", m_fFrequency)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.01f, {})),
    W_MEMBER_PROPERTY("ChangeSpeed", m_vScrollSpeed)->AddAttributes(new WDefaultValueAttribute(WVec3(1.0f))),
    W_MEMBER_PROPERTY("Octaves", m_uiOctaves)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(1, 4)),
    W_MEMBER_PROPERTY("AffectVelocity", m_bAffectVelocity)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_Turbulence, 1, WRTTIDefaultAllocator<WParticleBehavior_Turbulence>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleBehaviorFactory_Turbulence::WParticleBehaviorFactory_Turbulence() = default;

const WRTTI* WParticleBehaviorFactory_Turbulence::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_Turbulence>();
}

void WParticleBehaviorFactory_Turbulence::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_Turbulence* pBehavior = static_cast<WParticleBehavior_Turbulence*>(pObject);

  pBehavior->m_fStrength = m_fStrength;
  pBehavior->m_fFrequency = m_fFrequency;
  pBehavior->m_vScrollSpeed = m_vScrollSpeed;
  pBehavior->m_uiOctaves = m_uiOctaves;
  pBehavior->m_bAffectVelocity = m_bAffectVelocity;
}

void WParticleBehaviorFactory_Turbulence::QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const
{
  if (m_bAffectVelocity)
  {
    inout_finalizerDeps.Insert(WGetStaticRTTI<WParticleFinalizerFactory_ApplyVelocity>());
  }
}

void WParticleBehaviorFactory_Turbulence::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_fStrength;
  inout_stream << m_fFrequency;
  inout_stream << m_vScrollSpeed;
  inout_stream << m_uiOctaves;
  inout_stream << m_bAffectVelocity;
}

void WParticleBehaviorFactory_Turbulence::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= 1, "Invalid version {0}", uiVersion);

  inout_stream >> m_fStrength;
  inout_stream >> m_fFrequency;
  inout_stream >> m_vScrollSpeed;
  inout_stream >> m_uiOctaves;
  inout_stream >> m_bAffectVelocity;
}

void WParticleBehavior_Turbulence::OnFinalize()
{
  m_Noise.Initialize(GetOwnerEffect()->GetRNG());
}

void WParticleBehavior_Turbulence::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);

  if (m_bAffectVelocity)
  {
    CreateStream("Velocity", WProcessingStream::DataType::Half4, &m_pStreamVelocity, false);
  }
}

void WParticleBehavior_Turbulence::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: NoiseForce");

  const float tDiff = m_TimeDiff.AsFloatInSeconds();
  if (tDiff <= 0.0f)
    return;

  m_TotalTime += m_TimeDiff;

  const float freq = m_fFrequency;
  const WVec3 scroll = m_vScrollSpeed * m_TotalTime.AsFloatInSeconds();

  // Small offsets for curl noise approximation: sample noise at 3 different offsets
  // to get uncorrelated X, Y, Z force components
  const float kOffset1 = 31.416f;
  const float kOffset2 = 67.123f;

  WProcessingStreamIterator<WSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);

  const WSimdVec4f constHalf(0.5f);
  const WSimdVec4f constMul(2.0f * m_fStrength * tDiff);

  // Precompute (scroll * freq) so per-particle noise coords can be computed with a single MulAdd
  const WSimdVec4f simdScrollFreq(scroll.x * freq, scroll.y * freq, scroll.z * freq, 0.0f);
  const WSimdFloat simdFreq(freq);

  if (m_bAffectVelocity)
  {
    WProcessingStreamIterator<WFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);

    while (!itPosition.HasReachedEnd())
    {
      const WSimdVec4f simPos = itPosition.Current();

      // Noise sampling coordinates: (pos + scroll) * freq, computed per-axis via SIMD
      const WSimdVec4f sPos = WSimdVec4f::MulAdd(simPos, simdFreq, simdScrollFreq);
      const float sx = sPos.GetComponent<0>();
      const float sy = sPos.GetComponent<1>();
      const float sz = sPos.GetComponent<2>();

      // Sample noise at 3 offset positions for curl-noise approximation
      // Each component uses a different spatial offset to get uncorrelated values
      const WSimdVec4f noise = m_Noise.NoiseZeroToOne(WSimdVec4f(sx, sx + kOffset1, sx + kOffset2, 0), WSimdVec4f(sy, sy + kOffset1, sy + kOffset2, 0), WSimdVec4f(sz, sz + kOffset1, sz + kOffset2, 0), m_uiOctaves);

      // Map [0,1] -> [-1,1]; velocity is stored as (dirX, dirY, dirZ, speed)
      const WSimdVec4f forceSimd = (noise - constHalf).CompMul(constMul);

      const WVec4 vel4 = itVelocity.Current();
      WSimdVec4f velSimd = WSimdConversion::ToVec4(vel4);

      // Compute dir * speed + force in SIMD (velSimd.w = speed, broadcast via w())
      WSimdVec4f newVelSimd = WSimdVec4f::MulAdd(velSimd, velSimd.w(), forceSimd);

      const WSimdFloat newSpeed = newVelSimd.GetLength<3>();
      newVelSimd.NormalizeIfNotZero<3>(WSimdVec4f(0.0f, 0.0f, 1.0f, 0.0f));
      newVelSimd.SetW(newSpeed);

      itVelocity.Current() = WSimdConversion::ToVec4(newVelSimd);

      itPosition.Advance();
      itVelocity.Advance();
    }
  }
  else
  {
    // Direct position offset mode
    while (!itPosition.HasReachedEnd())
    {
      const WSimdVec4f simPos = itPosition.Current();

      const WSimdVec4f sPos = WSimdVec4f::MulAdd(simPos, simdFreq, simdScrollFreq);
      const float sx = sPos.GetComponent<0>();
      const float sy = sPos.GetComponent<1>();
      const float sz = sPos.GetComponent<2>();

      const WSimdVec4f noise = m_Noise.NoiseZeroToOne(WSimdVec4f(sx, sx + kOffset1, sx + kOffset2, 0), WSimdVec4f(sy, sy + kOffset1, sy + kOffset2, 0), WSimdVec4f(sz, sz + kOffset1, sz + kOffset2, 0), m_uiOctaves);

      const WSimdVec4f offset = (noise - constHalf).CompMul(constMul);

      itPosition.Current() = simPos + offset;

      itPosition.Advance();
    }
  }
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Turbulence);
