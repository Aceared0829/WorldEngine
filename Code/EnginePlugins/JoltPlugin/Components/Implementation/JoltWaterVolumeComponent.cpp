#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Components/JoltWaterVolumeComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltWorldModule.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltWaterVolumeComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Extents", m_vExtents)->AddAttributes(new WDefaultValueAttribute(WVec3(10.0f)), new WClampValueAttribute(WVec3(0.0f), WVariant())),
    W_MEMBER_PROPERTY("Flow", m_vFlow),
    W_MEMBER_PROPERTY("NoiseStrength", m_fNoiseStrength)->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_RESOURCE_MEMBER_PROPERTY("Surface", m_hSurface)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),
    W_MEMBER_PROPERTY("Interaction", m_sInteraction)->AddAttributes(new WDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgTriggerTriggered, OnMsgTriggerTriggered),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Physics/Jolt/Effects"),
    new WBoxManipulatorAttribute("Extents", 1.0f, true),
    new WBoxVisualizerAttribute("Extents", 1.0f, WColorScheme::GetCategoryColor("Physics", WColorScheme::CategoryColorUsage::ViewportIcon)),
    new WDirectionVisualizerAttribute("Flow", 1.0f, WColorScheme::GetCategoryColor("Physics", WColorScheme::CategoryColorUsage::ViewportIcon)),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

WJoltWaterVolumeComponentManager::WJoltWaterVolumeComponentManager(WWorld* pWorld)
  : WComponentManager<WJoltWaterVolumeComponent, WBlockStorageType::FreeList>(pWorld)
  , m_Noise(12345)
{
}

WJoltWaterVolumeComponentManager::~WJoltWaterVolumeComponentManager() = default;

void WJoltWaterVolumeComponentManager::UpdateWaterVolumes(WTime deltaTime)
{
  W_PROFILE_SCOPE("UpdateWaterVolumes");

  auto pJoltSystem = GetWorld()->GetModule<WJoltWorldModule>()->GetJoltSystem();

  for (auto it = GetComponents(0); it.IsValid(); it.Next())
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update(*pJoltSystem, deltaTime);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

WJoltWaterVolumeComponent::WJoltWaterVolumeComponent() = default;
WJoltWaterVolumeComponent::~WJoltWaterVolumeComponent() = default;

void WJoltWaterVolumeComponent::OnSimulationStarted()
{
  WJoltTriggerComponent* pTriggerComponent = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType(pTriggerComponent) == false)
  {
    WLog::Warning("WJoltWaterVolumeComponent requires an WJoltTriggerComponent on the same game object.");
  }

  auto pJoltSystem = GetWorld()->GetOrCreateModule<WJoltWorldModule>()->GetJoltSystem();
  UpdateWaterPlane(WJoltConversionUtils::ToVec3(pJoltSystem->GetGravity()));
}

void WJoltWaterVolumeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vFlow;
  s << m_fNoiseStrength;
  s << m_hSurface;
  s << m_sInteraction;
}

void WJoltWaterVolumeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  /*const WUInt32 uiVersion =*/inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vFlow;
  s >> m_fNoiseStrength;
  s >> m_hSurface;
  s >> m_sInteraction;
}

void WJoltWaterVolumeComponent::OnMsgTriggerTriggered(WMsgTriggerTriggered& msg)
{
  if (msg.m_TriggerState != WTriggerState::Activated && msg.m_TriggerState != WTriggerState::Deactivated)
    return;

  WGameObject* pSubmergedObject = nullptr;
  if (!GetWorld()->TryGetObject(msg.m_hTriggeringObject, pSubmergedObject))
    return;

  WJoltDynamicActorComponent* pActorComponent = nullptr;
  if (!pSubmergedObject->TryGetComponentOfBaseType(pActorComponent) || pActorComponent->GetKinematic())
    return;

  if (msg.m_TriggerState == WTriggerState::Activated)
  {
    if (m_hSurface.IsValid() && m_sInteraction.IsEmpty() == false)
    {
      WResourceLock<WSurfaceResource> pSurface(m_hSurface, WResourceAcquireMode::BlockTillLoaded);

      const WVec3 vPos = m_SurfacePlane.ProjectOntoPlane(pSubmergedObject->GetGlobalPosition());
      const WVec3 vNormal = m_SurfacePlane.m_vNormal;
      const WVec3 vDirection = pSubmergedObject->GetLinearVelocity();

      pSurface->InteractWithSurface(GetWorld(), WGameObjectHandle(), vPos, vNormal, vDirection, m_sInteraction, &GetOwner()->GetTeamID());
    }

    m_SubmergedActors.Insert(pActorComponent->GetHandle());
  }
  else if (msg.m_TriggerState == WTriggerState::Deactivated)
  {
    m_SubmergedActors.Remove(pActorComponent->GetHandle());
  }
}

void WJoltWaterVolumeComponent::CreateShapes(WDynamicArray<WJoltSubShape>& out_Shapes, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial)
{
  // can't create boxes smaller than this
  WVec3 size = m_vExtents * 0.5f;
  size.x = WMath::Max(size.x, JPH::cDefaultConvexRadius);
  size.y = WMath::Max(size.y, JPH::cDefaultConvexRadius);
  size.z = WMath::Max(size.z, JPH::cDefaultConvexRadius);

  auto pNewShape = new JPH::BoxShape(WJoltConversionUtils::ToVec3(size));
  pNewShape->AddRef();
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<WUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  WJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape = pNewShape;
  sub.m_Transform = WTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
}

void WJoltWaterVolumeComponent::Update(JPH::PhysicsSystem& joltSystem, WTime deltaTime)
{
  const WTransform globalTransform = GetOwner()->GetGlobalTransform();
  const WVec3 vScaledExtents = m_vExtents.CompMul(globalTransform.m_vScale);

  if (vScaledExtents.x * vScaledExtents.y * vScaledExtents.z < WMath::DefaultEpsilon<float>())
    return;

  const WVec3 gravity = WJoltConversionUtils::ToVec3(joltSystem.GetGravity());

  if (GetOwner()->IsDynamic() || m_vGravity.IsEqual(gravity, WMath::DefaultEpsilon<float>()) == false)
  {
    UpdateWaterPlane(gravity);
  }

  const JPH::Vec3 flow = WJoltConversionUtils::ToVec3(globalTransform.TransformDirection(m_vFlow));
  const float fDeltaTime = deltaTime.AsFloatInSeconds();

  const WVec3 vSurfaceTangent = m_SurfacePlane.m_vNormal.GetOrthogonalVector().GetNormalized();
  const WVec3 vSurfaceBitangent = m_SurfacePlane.m_vNormal.CrossRH(vSurfaceTangent).GetNormalized();

  m_fNoiseTime += fDeltaTime * 0.5f;
  if (m_fNoiseTime > 1000.0f)
    m_fNoiseTime -= 1000.0f;

  auto& noise = static_cast<WJoltWaterVolumeComponentManager*>(GetOwningManager())->m_Noise;

  for (auto it : m_SubmergedActors)
  {
    WJoltDynamicActorComponent* pActorComponent = nullptr;
    if (!GetWorld()->TryGetComponent(it, pActorComponent) || pActorComponent->IsActiveAndSimulating() == false)
      continue;

    JPH::BodyLockWrite lock(joltSystem.GetBodyLockInterface(), JPH::BodyID(pActorComponent->GetJoltBodyID()));
    JPH::Body& body = lock.GetBody();
    if (body.IsActive() && body.IsDynamic())
    {
      const WVec3 pos = WJoltConversionUtils::ToVec3(body.GetCenterOfMassPosition());
      WVec3 surfacePosition = m_SurfacePlane.ProjectOntoPlane(pos);

      if (m_fNoiseStrength != 0.0f)
      {
        const float noisePosX = vSurfaceTangent.Dot(surfacePosition);
        const float noisePosY = vSurfaceBitangent.Dot(surfacePosition);

        WSimdVec4f noisePos = WSimdConversion::ToVec3(WVec3(noisePosX, noisePosY, m_fNoiseTime));
        WSimdVec4f noiseValue = noise.NoiseZeroToOne(WSimdVec4f(noisePos.x()), WSimdVec4f(noisePos.y()), WSimdVec4f(noisePos.z()));

        surfacePosition += m_SurfacePlane.m_vNormal * (float(noiseValue.x()) * 2 - 1) * m_fNoiseStrength;

        body.ResetSleepTimer();
      }

      const JPH::Vec3 surfacePositionJolt = WJoltConversionUtils::ToVec3(surfacePosition);
      const JPH::Vec3 surfaceNormal = WJoltConversionUtils::ToVec3(m_SurfacePlane.m_vNormal);
      const float fBuoyancyFactor = pActorComponent->m_fBuoyancyFactor;

      body.ApplyBuoyancyImpulse(surfacePositionJolt, surfaceNormal, fBuoyancyFactor, 0.3f, 0.05f, flow, joltSystem.GetGravity(), fDeltaTime);
    }
  }
}

void WJoltWaterVolumeComponent::UpdateWaterPlane(const WVec3& vGravity)
{
  const WTransform globalTransform = GetOwner()->GetGlobalTransform();
  const WVec3 halfExtents = m_vExtents * 0.5f;
  const WVec3 vGravityDir = vGravity.GetNormalized();
  float fMinDot = 1000.0f;

  for (WUInt32 i = 0; i < 6; ++i)
  {
    WVec3 vNormal = WBasisAxis::GetBasisVector(static_cast<WBasisAxis::Enum>(i));
    WVec3 vPoint = vNormal.CompMul(halfExtents);
    vNormal = globalTransform.TransformDirection(vNormal).GetNormalized();

    float fDot = vNormal.Dot(vGravityDir);
    if (fDot < fMinDot)
    {
      vPoint = globalTransform.TransformPosition(vPoint);
      m_SurfacePlane = WPlane::MakeFromNormalAndPoint(vNormal, vPoint);

      fMinDot = fDot;
    }
  }

  m_vGravity = vGravity;
}

W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltWaterVolumeComponent);
