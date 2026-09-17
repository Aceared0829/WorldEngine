#include <AiPlugin/AiPluginPCH.h>
#include <AiPlugin/Navigation/Components/NavMeshPathTestComponent.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WAiNavMeshPathTestComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("VisualizePathCorridor", m_bVisualizePathCorridor)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("VisualizePathLine", m_bVisualizePathLine)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("VisualizePathState", m_bVisualizePathState)->AddAttributes(new WDefaultValueAttribute(true)),

    W_ACCESSOR_PROPERTY("PathEnd", DummyGetter, SetPathEndReference)->AddAttributes(new WGameObjectReferenceAttribute()),
    W_MEMBER_PROPERTY("NavmeshConfig", m_sNavmeshConfig)->AddAttributes(new WDynamicStringEnumAttribute("AiNavmeshConfig")),
    W_MEMBER_PROPERTY("PathSearchConfig", m_sPathSearchConfig)->AddAttributes(new WDynamicStringEnumAttribute("AiPathSearchConfig")),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("AI/Navigation"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WAiNavMeshPathTestComponent::WAiNavMeshPathTestComponent() = default;
WAiNavMeshPathTestComponent::~WAiNavMeshPathTestComponent() = default;

void WAiNavMeshPathTestComponent::SetPathEndReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetPathEnd(resolver(szReference, GetHandle(), "PathEnd"));
}

void WAiNavMeshPathTestComponent::SetPathEnd(WGameObjectHandle hObject)
{
  m_hPathEnd = hObject;
}

void WAiNavMeshPathTestComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hPathEnd);
  s << m_sPathSearchConfig;
  s << m_sNavmeshConfig;
  s << m_bVisualizePathCorridor;
  s << m_bVisualizePathLine;
  s << m_bVisualizePathState;
}

void WAiNavMeshPathTestComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  WStreamReader& s = inout_stream.GetStream();
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  m_hPathEnd = inout_stream.ReadGameObjectHandle();
  s >> m_sPathSearchConfig;
  s >> m_sNavmeshConfig;
  s >> m_bVisualizePathCorridor;
  s >> m_bVisualizePathLine;
  s >> m_bVisualizePathState;
}

void WAiNavMeshPathTestComponent::Update()
{
  if (m_hPathEnd.IsInvalidated())
    return;

  WGameObject* pEnd = nullptr;
  if (!GetWorld()->TryGetObject(m_hPathEnd, pEnd))
    return;

  m_Navigation.SetCurrentPosition(GetOwner()->GetGlobalPosition());
  m_Navigation.SetTargetPosition(pEnd->GetGlobalPosition());

  if (WAiNavMeshWorldModule* pNavMeshModule = GetWorld()->GetOrCreateModule<WAiNavMeshWorldModule>())
  {
    m_Navigation.SetNavmesh(pNavMeshModule->GetNavMesh(m_sNavmeshConfig));
    m_Navigation.SetQueryFilter(pNavMeshModule->GetPathSearchFilter(m_sPathSearchConfig));
  }

  m_Navigation.Update();

  if (m_bVisualizePathCorridor)
  {
    m_Navigation.DebugDrawPathCorridor(GetWorld(), WColor::Aquamarine.WithAlpha(0.2f));
  }

  if (m_bVisualizePathLine)
  {
    m_Navigation.DebugDrawPathLine(GetWorld(), WColor::Lime);
  }

  if (m_bVisualizePathState)
  {
    m_Navigation.DebugDrawState(GetWorld(), GetOwner()->GetGlobalPosition());
  }

  // if (m_fSpeed <= 0)
  //   return;

  // WVec2 vForwardDir = GetOwner()->GetGlobalDirForwards().GetAsVec2();
  // vForwardDir.NormalizeIfNotZero(WVec2(1, 0)).IgnoreResult();

  // m_Steering.m_vPosition = GetOwner()->GetGlobalPosition();
  // m_Steering.m_qRotation = GetOwner()->GetGlobalRotation();
  // m_Steering.m_vVelocity = GetOwner()->GetLinearVelocity();
  // m_Steering.m_fMaxSpeed = m_fSpeed;
  // m_Steering.m_MinTurnSpeed = m_TurnSpeed;
  // m_Steering.m_fAcceleration = m_fAcceleration;
  // m_Steering.m_fDecceleration = m_fDecceleration;

  // const float fBrakingDistance = 1.2f * (WMath::Square(m_Steering.m_fMaxSpeed) / (2.0f * m_Steering.m_fDecceleration));

  // m_Navigation.ComputeSteeringInfo(m_Steering.m_Info, vForwardDir, fBrakingDistance);
  // m_Steering.Calculate(GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds(), GetWorld());

  // m_Steering.m_vPosition.z = m_Navigation.GetCurrentElevation();

  // GetOwner()->SetGlobalPosition(m_Steering.m_vPosition);
  // GetOwner()->SetGlobalRotation(m_Steering.m_qRotation);
}


W_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Components_NavMeshPathTestComponent);
