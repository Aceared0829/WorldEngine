#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_FadeOut.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_FadeOut, 1, WRTTIDefaultAllocator<WParticleBehaviorFactory_FadeOut>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("StartAlpha", m_fStartAlpha)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("Exponent", m_fExponent)->AddAttributes(new WDefaultValueAttribute(1.0f)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_FadeOut, 1, WRTTIDefaultAllocator<WParticleBehavior_FadeOut>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const WRTTI* WParticleBehaviorFactory_FadeOut::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_FadeOut>();
}

void WParticleBehaviorFactory_FadeOut::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_FadeOut* pBehavior = static_cast<WParticleBehavior_FadeOut*>(pObject);

  pBehavior->m_fStartAlpha = m_fStartAlpha;
  pBehavior->m_fExponent = m_fExponent;
}

void WParticleBehaviorFactory_FadeOut::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_fStartAlpha;
  inout_stream << m_fExponent;
}

void WParticleBehaviorFactory_FadeOut::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fStartAlpha;
  inout_stream >> m_fExponent;
}

void WParticleBehavior_FadeOut::CreateRequiredStreams()
{
  CreateStream("LifeTime", WProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Color", WProcessingStream::DataType::Half4, &m_pStreamColor, false);
}

void WParticleBehavior_FadeOut::Process(WUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // When invisible, don't update at all. Set the interval to 1 so that once
    // the effect becomes visible, all particles get fully updated on the next frame.
    m_uiCurrentUpdateInterval = 1;
    m_uiFirstToUpdate = 0;
    return;
  }

  W_PROFILE_SCOPE("PFX: Fade Out");

  WProcessingStreamIterator<WFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  WProcessingStreamIterator<WColorLinear16f> itColor(m_pStreamColor, uiNumElements, 0);

  // skip the first n particles
  itLifeTime.Advance(m_uiFirstToUpdate);
  itColor.Advance(m_uiFirstToUpdate);

  if (m_fStartAlpha <= 1.0f)
  {
    while (!itLifeTime.HasReachedEnd())
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
      itColor.Current().a = m_fStartAlpha * WMath::Pow(fLifeTimeFraction, m_fExponent);

      // skip the next n items
      // this is to reduce the number of particles that need to be fully evaluated
      itLifeTime.Advance(m_uiCurrentUpdateInterval);
      itColor.Advance(m_uiCurrentUpdateInterval);
    }
  }
  else
  {
    // this case has to clamp alpha to 1
    while (!itLifeTime.HasReachedEnd())
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
      itColor.Current().a = WMath::Min(1.0f, m_fStartAlpha * WMath::Pow(fLifeTimeFraction, m_fExponent));

      // skip the next n items
      // this is to reduce the number of particles that need to be fully evaluated
      itLifeTime.Advance(m_uiCurrentUpdateInterval);
      itColor.Advance(m_uiCurrentUpdateInterval);
    }
  }

  // adjust which index is the first to update
  {
    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }

  /// \todo Use level of detail to reduce the update interval further
  /// up close, with a high interval, animations appear choppy, especially when fading stuff out at the end

  // reset the update interval to the default
  m_uiCurrentUpdateInterval = 2;
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_FadeOut);
