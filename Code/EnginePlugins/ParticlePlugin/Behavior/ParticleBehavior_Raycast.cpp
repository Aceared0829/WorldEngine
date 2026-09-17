#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Raycast.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_Raycast, 1, WRTTIDefaultAllocator<WParticleBehaviorFactory_Raycast>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Reaction", WParticleRaycastHitReaction, m_Reaction),
    W_MEMBER_PROPERTY("BounceFactor", m_fBounceFactor)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("SlideFactor", m_fSlideFactor)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("SizeFactor", m_fSizeFactor)->AddAttributes(new WDefaultValueAttribute(0.1f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_MEMBER_PROPERTY("OnCollideEvent", m_sOnCollideEvent)->AddAttributes(new WDynamicStringEnumAttribute("ParticleEventNamesEnum")),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_Raycast, 1, WRTTIDefaultAllocator<WParticleBehavior_Raycast>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WParticleRaycastHitReaction, 1)
  W_ENUM_CONSTANTS(WParticleRaycastHitReaction::Bounce, WParticleRaycastHitReaction::Die, WParticleRaycastHitReaction::Stop)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WParticleBehaviorFactory_Raycast::WParticleBehaviorFactory_Raycast() = default;
WParticleBehaviorFactory_Raycast::~WParticleBehaviorFactory_Raycast() = default;

const WRTTI* WParticleBehaviorFactory_Raycast::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_Raycast>();
}

void WParticleBehaviorFactory_Raycast::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_Raycast* pBehavior = static_cast<WParticleBehavior_Raycast*>(pObject);

  pBehavior->m_Reaction = m_Reaction;
  pBehavior->m_uiCollisionLayer = m_uiCollisionLayer;
  pBehavior->m_sOnCollideEvent = WTempHashedString(m_sOnCollideEvent.GetData());
  pBehavior->m_fBounceFactor = m_fBounceFactor;
  pBehavior->m_fSlideFactor = m_fSlideFactor;
  pBehavior->m_fSizeFactor = m_fSizeFactor;

  pBehavior->m_pPhysicsModule = (WPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(WGetStaticRTTI<WPhysicsWorldModuleInterface>());
}

enum class BehaviorRaycastVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added event
  Version_3, // added bounce factor
  Version_4, // added slide and size factor

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void WParticleBehaviorFactory_Raycast::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)BehaviorRaycastVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_uiCollisionLayer;
  inout_stream << m_sOnCollideEvent;
  inout_stream << m_Reaction;
  inout_stream << m_fBounceFactor;
  inout_stream << m_fSlideFactor;
  inout_stream << m_fSizeFactor;
}

void WParticleBehaviorFactory_Raycast::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)BehaviorRaycastVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 2)
  {
    inout_stream >> m_uiCollisionLayer;
    inout_stream >> m_sOnCollideEvent;
    inout_stream >> m_Reaction;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_fBounceFactor;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_fSlideFactor;
    inout_stream >> m_fSizeFactor;
  }
}

void WParticleBehaviorFactory_Raycast::QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(WGetStaticRTTI<WParticleFinalizerFactory_ApplyVelocity>());
  inout_finalizerDeps.Insert(WGetStaticRTTI<WParticleFinalizerFactory_LastPosition>());
}

//////////////////////////////////////////////////////////////////////////

WParticleBehavior_Raycast::WParticleBehavior_Raycast()
{
  // do this right after WParticleFinalizer_ApplyVelocity has run
  m_fPriority = 526.0f;
}

void WParticleBehavior_Raycast::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("LastPosition", WProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
  CreateStream("Velocity", WProcessingStream::DataType::Half4, &m_pStreamVelocity, false);
}

void WParticleBehavior_Raycast::QueryOptionalStreams()
{
  m_pStreamSize = GetOwnerSystem()->QueryStream("Size", WProcessingStream::DataType::Half);
}

void WParticleBehavior_Raycast::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Raycast");

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  WProcessingStreamIterator<WVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  WProcessingStreamIterator<const WVec3> itLastPosition(m_pStreamLastPosition, uiNumElements, 0);
  WProcessingStreamIterator<WFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  WFloat16 fDummySize = 0.0f;
  const WFloat16* pSize = m_pStreamSize != nullptr ? m_pStreamSize->GetData<WFloat16>() : &fDummySize;

  WPhysicsCastResult hitResult;

  WUInt32 i = 0;
  while (!itPosition.HasReachedEnd())
  {
    const WVec3 vLastPos = itLastPosition.Current();
    const WVec3 vCurPos = itPosition.Current().GetAsVec3();

    if (!vLastPos.IsZero())
    {
      const WVec3 vChange = vCurPos - vLastPos;

      if (!vChange.IsZero(WMath::DefaultEpsilon<float>()))
      {
        WVec3 vDirection = vChange;

        const float fSize = WMath::Max(*pSize * m_fSizeFactor, 0.01f);
        const float fMaxLen = vDirection.GetLengthAndNormalize();

        WPhysicsQueryParameters query(m_uiCollisionLayer);
        query.m_ShapeTypes = WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic;

        if (m_pPhysicsModule != nullptr && m_pPhysicsModule->Raycast(hitResult, vLastPos, vDirection, fMaxLen + fSize, query))
        {
          hitResult.m_vPosition -= vDirection * fSize;
          const float fRemainingLen = (vCurPos - hitResult.m_vPosition).GetLength();
          const float fRemainder = fRemainingLen / fMaxLen;

          if (m_Reaction == WParticleRaycastHitReaction::Bounce)
          {
            const WVec3 vTangentDir = vChange - hitResult.m_vNormal * hitResult.m_vNormal.Dot(vChange);
            const WVec3 vNormalDir = vTangentDir - vChange;

            const WVec3 vNewDir = vNormalDir * m_fBounceFactor + vTangentDir * m_fSlideFactor;

            if (vNewDir.GetLengthSquared() < WMath::Square(0.01f))
            {
              itPosition.Current() = hitResult.m_vPosition.GetAsPositionVec4();
              itVelocity.Current() = WVec4(0, 0, 1, 0);
            }
            else
            {
              itPosition.Current() = (hitResult.m_vPosition + vNewDir * fRemainder).GetAsVec4(0);

              const WVec3 newVel = vNewDir / tDiff;
              const float newSpeed = newVel.GetLength();
              const WVec3 newDir = newSpeed > 0.0f ? newVel / newSpeed : WVec3(0, 0, 1);

              itVelocity.Current() = WVec4(newDir.x, newDir.y, newDir.z, newSpeed);
            }
          }
          else if (m_Reaction == WParticleRaycastHitReaction::Die)
          {
            /// \todo Get current element index from iterator ?
            m_pStreamGroup->RemoveElement(i);
          }
          else if (m_Reaction == WParticleRaycastHitReaction::Stop)
          {
            itPosition.Current() = hitResult.m_vPosition.GetAsPositionVec4();
            itVelocity.Current() = WVec4(0, 0, 1, 0);
          }

          if (!m_sOnCollideEvent.IsEmpty())
          {
            WParticleEvent e;
            e.m_EventType = m_sOnCollideEvent;
            e.m_vPosition = hitResult.m_vPosition;
            e.m_vNormal = hitResult.m_vNormal;
            e.m_vDirection = vDirection;

            GetOwnerEffect()->AddParticleEvent(e);
          }
        }

        if constexpr (false)
        {
          WDebugRenderer::DrawLineSphere(m_pPhysicsModule->GetWorld(), WBoundingSphere::MakeFromCenterAndRadius(itPosition.Current().GetAsVec3(), fSize), WColor::Red);
        }
      }
    }

    itPosition.Advance();
    itLastPosition.Advance();
    itVelocity.Advance();

    if (m_pStreamSize != nullptr)
      ++pSize;

    ++i;
  }
}

void WParticleBehavior_Raycast::RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<WPhysicsWorldModuleInterface>();
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Raycast);
