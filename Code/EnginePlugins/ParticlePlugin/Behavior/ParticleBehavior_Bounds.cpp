#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Bounds.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_Bounds, 1, WRTTIDefaultAllocator<WParticleBehaviorFactory_Bounds>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    W_MEMBER_PROPERTY("BoxExtents", m_vBoxExtents)->AddAttributes(new WDefaultValueAttribute(WVec3(2, 2, 2))),
    W_ENUM_MEMBER_PROPERTY("OutOfBoundsMode", WParticleOutOfBoundsMode, m_OutOfBoundsMode),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WBoxVisualizerAttribute("BoxExtents", 1.0f, WColor::LightGreen, nullptr, WVisualizerAnchor::Center, WVec3(1.0f), "PositionOffset")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_Bounds, 1, WRTTIDefaultAllocator<WParticleBehavior_Bounds>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleBehaviorFactory_Bounds::WParticleBehaviorFactory_Bounds() = default;

const WRTTI* WParticleBehaviorFactory_Bounds::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_Bounds>();
}

void WParticleBehaviorFactory_Bounds::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_Bounds* pBehavior = static_cast<WParticleBehavior_Bounds*>(pObject);

  pBehavior->m_vPositionOffset = m_vPositionOffset;
  pBehavior->m_vBoxExtents = m_vBoxExtents;
  pBehavior->m_OutOfBoundsMode = m_OutOfBoundsMode;
}

enum class BehaviorBoundsVersion
{
  Version_0 = 0,
  Version_1, // added out of bounds mode

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleBehaviorFactory_Bounds::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)BehaviorBoundsVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_vPositionOffset;
  inout_stream << m_vBoxExtents;

  // version 1
  inout_stream << m_OutOfBoundsMode;
}

void WParticleBehaviorFactory_Bounds::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)BehaviorBoundsVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_vPositionOffset;
  inout_stream >> m_vBoxExtents;

  if (uiVersion >= 1)
  {
    inout_stream >> m_OutOfBoundsMode;
  }
}

void WParticleBehavior_Bounds::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
}

void WParticleBehavior_Bounds::QueryOptionalStreams()
{
  m_pStreamLastPosition = GetOwnerSystem()->QueryStream("LastPosition", WProcessingStream::DataType::Float3);
}

void WParticleBehavior_Bounds::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Bounds");

  const WSimdTransform trans = WSimdConversion::ToTransform(GetOwnerSystem()->GetTransform());
  const WSimdTransform invTrans = trans.GetInverse();

  const WSimdVec4f boxCenter = WSimdConversion::ToVec3(m_vPositionOffset);
  const WSimdVec4f boxExt = WSimdConversion::ToVec3(m_vBoxExtents);
  const WSimdVec4f halfExtPos = WSimdConversion::ToVec3(m_vBoxExtents) * 0.5f;
  const WSimdVec4f halfExtNeg = -halfExtPos;

  WProcessingStreamIterator<WSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);

  if (m_OutOfBoundsMode == WParticleOutOfBoundsMode::Teleport)
  {
    WVec3* pLastPosition = nullptr;

    if (m_pStreamLastPosition)
    {
      pLastPosition = m_pStreamLastPosition->GetWritableData<WVec3>();
    }

    while (!itPosition.HasReachedEnd())
    {
      const WSimdVec4f globalPosCur = itPosition.Current();
      const WSimdVec4f localPosCur = invTrans.TransformPosition(globalPosCur) - boxCenter;

      const WSimdVec4f localPosAdd = localPosCur + boxExt;
      const WSimdVec4f localPosSub = localPosCur - boxExt;

      WSimdVec4f localPosNew;
      localPosNew = WSimdVec4f::Select(localPosCur > halfExtPos, localPosSub, localPosCur);
      localPosNew = WSimdVec4f::Select(localPosCur < halfExtNeg, localPosAdd, localPosNew);

      localPosNew += boxCenter;
      const WSimdVec4f globalPosNew = trans.TransformPosition(localPosNew);

      if (m_pStreamLastPosition)
      {
        const WSimdVec4f posDiff = globalPosNew - globalPosCur;
        *pLastPosition += WSimdConversion::ToVec3(posDiff);
        ++pLastPosition;
      }

      itPosition.Current() = globalPosNew;
      itPosition.Advance();
    }
  }
  else
  {
    WUInt32 idx = 0;

    while (!itPosition.HasReachedEnd())
    {
      const WSimdVec4f globalPosCur = itPosition.Current();
      const WSimdVec4f localPosCur = invTrans.TransformPosition(globalPosCur) - boxCenter;

      if ((localPosCur > halfExtPos).AnySet() || (localPosCur < halfExtNeg).AnySet())
      {
        m_pStreamGroup->RemoveElement(idx);
      }

      ++idx;
      itPosition.Advance();
    }
  }
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Bounds);
