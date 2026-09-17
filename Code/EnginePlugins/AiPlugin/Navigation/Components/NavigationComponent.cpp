#include <AiPlugin/AiPluginPCH.h>
#include <AiPlugin/Navigation/Components/NavigationComponent.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_BITFLAGS(WAiNavigationDebugFlags, 1)
  W_BITFLAGS_CONSTANTS(WAiNavigationDebugFlags::PrintState, WAiNavigationDebugFlags::VisPathCorridor, WAiNavigationDebugFlags::VisPathLine, WAiNavigationDebugFlags::VisTarget)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_STATIC_REFLECTED_ENUM(WAiNavigationComponentState, 1)
  W_ENUM_CONSTANTS(WAiNavigationComponentState::Idle, WAiNavigationComponentState::Moving, WAiNavigationComponentState::Turning, WAiNavigationComponentState::Falling, WAiNavigationComponentState::Fallen, WAiNavigationComponentState::Failed)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_COMPONENT_TYPE(WAiNavigationComponent, 2, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("NavmeshConfig", m_sNavmeshConfig)->AddAttributes(new WDynamicStringEnumAttribute("AiNavmeshConfig")),
    W_MEMBER_PROPERTY("PathSearchConfig", m_sPathSearchConfig)->AddAttributes(new WDynamicStringEnumAttribute("AiPathSearchConfig")),
    W_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new WDefaultValueAttribute(5.0f)),
    W_MEMBER_PROPERTY("Acceleration", m_fAcceleration)->AddAttributes(new WDefaultValueAttribute(3.0f)),
    W_MEMBER_PROPERTY("Deceleration", m_fDecceleration)->AddAttributes(new WDefaultValueAttribute(8.0f)),
    W_MEMBER_PROPERTY("FootRadius", m_fFootRadius)->AddAttributes(new WDefaultValueAttribute(0.15f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("ReachedDistance", m_fReachedDistance)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 10.0f)),
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_MEMBER_PROPERTY("FallHeight", m_fFallHeight)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_BITFLAGS_MEMBER_PROPERTY("DebugFlags", WAiNavigationDebugFlags , m_DebugFlags),
    W_MEMBER_PROPERTY("ApplySteering", m_bApplySteering)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("AI/Navigation"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SetDestination, In, "Destination", In, "AllowPartialPaths"),
    W_SCRIPT_FUNCTION_PROPERTY(CancelNavigation),
    W_SCRIPT_FUNCTION_PROPERTY(GetState),
    W_SCRIPT_FUNCTION_PROPERTY(StopWalking, In, "WithinDistance"),
    W_SCRIPT_FUNCTION_PROPERTY(TurnTowards, In, "TargetPosition"),
    W_SCRIPT_FUNCTION_PROPERTY(GetTurnAngleTowards, In, "TargetPosition"),
    W_SCRIPT_FUNCTION_PROPERTY(EnsureNavMeshSectorAvailable, In, "vCenter", In, "fRadius"),
    W_SCRIPT_FUNCTION_PROPERTY(FindRandomPointAroundCircle, In, "vCenter", In, "fRadius", Out, "out_vPoint"),
    W_SCRIPT_FUNCTION_PROPERTY(RaycastNavMesh, In, "vStart", In, "vDirection", In, "fDistance", Out, "out_vPoint", Out, "out_fDistance"),
    W_SCRIPT_FUNCTION_PROPERTY(GetSteeringPosition),
    W_SCRIPT_FUNCTION_PROPERTY(GetSteeringRotation),
  }
  W_END_FUNCTIONS;
}
W_END_COMPONENT_TYPE
// clang-format on

WAiNavigationComponent::WAiNavigationComponent() = default;
WAiNavigationComponent::~WAiNavigationComponent() = default;

void WAiNavigationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_uiSkipNextFrames = 3; // 2 are needed to have colliders set up at the start of the scene simulation, 3 just to be save
  m_Steering.m_vPosition = GetOwner()->GetGlobalPosition();
  m_Steering.m_qRotation = GetOwner()->GetGlobalRotation();
}

void WAiNavigationComponent::SetDestination(const WVec3& vGlobalPos, bool bAllowPartialPath)
{
  m_fStopWalkDistance = WMath::HighValue<float>();
  m_bAllowPartialPath = bAllowPartialPath;
  m_Navigation.SetTargetPosition(vGlobalPos);
  m_State = WAiNavigationComponentState::Moving;
}

void WAiNavigationComponent::CancelNavigation()
{
  m_Navigation.CancelNavigation();

  if (m_State != WAiNavigationComponentState::Falling)
  {
    // if it is still falling, don't reset the state
    m_State = WAiNavigationComponentState::Idle;
  }
}

void WAiNavigationComponent::StopWalking(float fWithinDistance)
{
  m_fStopWalkDistance = WMath::Min(m_fStopWalkDistance, fWithinDistance);
}

void WAiNavigationComponent::TurnTowards(const WVec2& vGlobalPos)
{
  if (m_State == WAiNavigationComponentState::Idle)
  {
    m_State = WAiNavigationComponentState::Turning;

    m_vTurnTowardsPos = vGlobalPos;
  }
}

WAngle WAiNavigationComponent::GetTurnAngleTowards(const WVec2& vGlobalPos) const
{
  WVec3 vOwnPos2D = GetOwner()->GetGlobalPosition();
  vOwnPos2D.z = 0.0f;

  WVec3 vTargetDir = (vGlobalPos.GetAsVec3(0) - vOwnPos2D);
  if (vTargetDir.NormalizeIfNotZero(WVec3::MakeZero()).Failed())
    return WAngle::MakeZero();

  WVec3 vLookDir = GetOwner()->GetGlobalDirForwards();
  vLookDir.z = 0.0f;
  vLookDir.Normalize();

  return vLookDir.GetAngleBetween(vTargetDir, WVec3::MakeAxisZ());
}

bool WAiNavigationComponent::PrepareQueryObject()
{
  if (m_Query.GetNavmesh() == nullptr)
  {
    WAiNavMeshWorldModule* pNavMeshModule = GetWorld()->GetOrCreateModule<WAiNavMeshWorldModule>();
    if (pNavMeshModule == nullptr)
      return false;

    m_Query.SetNavmesh(pNavMeshModule->GetNavMesh(m_sNavmeshConfig));
    m_Query.SetQueryFilter(pNavMeshModule->GetPathSearchFilter(m_sPathSearchConfig));
  }

  return true;
}

bool WAiNavigationComponent::EnsureNavMeshSectorAvailable(const WVec3& vCenter, float fRadius)
{
  if (!PrepareQueryObject())
    return false;

  return m_Query.GetNavmesh()->RequestSector(vCenter.GetAsVec2(), WVec2(fRadius));
}

bool WAiNavigationComponent::FindRandomPointAroundCircle(const WVec3& vCenter, float fRadius, WVec3& out_vPoint)
{
  if (!PrepareQueryObject())
    return false;

  if (!m_Query.PrepareQueryArea(vCenter, fRadius))
    return false;

  return m_Query.FindRandomPointAroundCircle(vCenter, fRadius, GetWorld()->GetRandomNumberGenerator(), out_vPoint);
}

bool WAiNavigationComponent::RaycastNavMesh(const WVec3& vStart, const WVec3& vDirection, float fDistance, WVec3& out_vPoint, float& out_fDistance)
{
  if (!PrepareQueryObject())
    return false;

  // ignore result, even if not everything is loaded, the raycast may still hit an obstacle
  m_Query.PrepareQueryArea(vStart, fDistance);

  WAiNavmeshRaycastHit hit;
  if (!m_Query.Raycast(vStart, vDirection, fDistance, hit))
    return false;

  out_vPoint = hit.m_vHitPosition;
  out_fDistance = hit.m_fHitDistance;
  return true;
}

WVec3 WAiNavigationComponent::GetSteeringPosition() const
{
  return m_vSteerPosition;
}

WQuat WAiNavigationComponent::GetSteeringRotation() const
{
  return m_qSteerRotation;
}

void WAiNavigationComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_sPathSearchConfig;
  s << m_sNavmeshConfig;
  s << m_fReachedDistance;
  s << m_fSpeed;
  s << m_fAcceleration;
  s << m_fDecceleration;
  s << m_fFootRadius;
  s << m_uiCollisionLayer;
  s << m_fFallHeight;
  s << m_DebugFlags;
  s << m_bApplySteering;
}

void WAiNavigationComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  WStreamReader& s = inout_stream.GetStream();
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_sPathSearchConfig;
  s >> m_sNavmeshConfig;
  s >> m_fReachedDistance;
  s >> m_fSpeed;
  s >> m_fAcceleration;
  s >> m_fDecceleration;
  s >> m_fFootRadius;
  s >> m_uiCollisionLayer;
  s >> m_fFallHeight;
  s >> m_DebugFlags;

  if (uiVersion >= 2)
  {
    s >> m_bApplySteering;
  }
}

void WAiNavigationComponent::Update()
{
  if (m_uiSkipNextFrames > 0)
  {
    // in the very first frame, physics may not be available yet (colliders are not yet set up)
    // so skip that frame to prevent not finding a ground and entering the 'falling' state
    m_uiSkipNextFrames--;
    return;
  }

  WTransform transform = GetOwner()->GetGlobalTransform();
  const float tDiff = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

  Steer(transform, tDiff);
  Turn(transform, tDiff);
  PlaceOnGround(transform, tDiff);

  m_vSteerPosition = transform.m_vPosition;
  m_qSteerRotation = transform.m_qRotation;

  if (m_bApplySteering)
  {
    GetOwner()->SetGlobalPosition(m_vSteerPosition);
    GetOwner()->SetGlobalRotation(m_qSteerRotation);
  }

  if (m_DebugFlags.IsAnyFlagSet())
  {
    if (m_DebugFlags.IsSet(WAiNavigationDebugFlags::VisPathCorridor))
    {
      m_Navigation.DebugDrawPathCorridor(GetWorld(), WColor::Aquamarine.WithAlpha(0.15f), 0.2f);
    }

    if (m_DebugFlags.IsSet(WAiNavigationDebugFlags::VisPathLine))
    {
      m_Navigation.DebugDrawPathLine(GetWorld(), WColor::DeepSkyBlue, 0.3f);
    }

    if (m_DebugFlags.IsSet(WAiNavigationDebugFlags::PrintState))
    {
      const WVec3 vPosition = GetOwner()->GetGlobalPosition() + WVec3(0, 0, 1.5f);

      switch (m_State)
      {
        case WAiNavigationComponentState::Idle:
          WDebugRenderer::Draw3DText(GetWorld(), "Idle", vPosition, WColor::Grey);
          break;
        case WAiNavigationComponentState::Moving:
          WDebugRenderer::Draw3DText(GetWorld(), "Moving", vPosition, WColor::Yellow);
          m_Navigation.DebugDrawState(GetWorld(), vPosition - WVec3(0, 0, 0.5f));
          break;
        case WAiNavigationComponentState::Turning:
          WDebugRenderer::Draw3DText(GetWorld(), "Turning", vPosition, WColor::Orange);
          break;
        case WAiNavigationComponentState::Falling:
          WDebugRenderer::Draw3DText(GetWorld(), "Falling...", vPosition, WColor::IndianRed);
          break;
        case WAiNavigationComponentState::Fallen:
          WDebugRenderer::Draw3DText(GetWorld(), "Fallen", vPosition, WColor::IndianRed);
          break;
        case WAiNavigationComponentState::Failed:
          WDebugRenderer::Draw3DText(GetWorld(), "Failed", vPosition, WColor::Red);
          m_Navigation.DebugDrawState(GetWorld(), vPosition - WVec3(0, 0, 0.5f));
          break;
      }
    }

    if (m_DebugFlags.IsSet(WAiNavigationDebugFlags::VisTarget))
    {
      WDebugRenderer::DrawArrow(GetWorld(), 1.0f, WColor::Lime, WTransform(m_Navigation.GetTargetPosition() + WVec3(0, 0, 1.5f)), -WVec3::MakeAxisZ());
    }
  }
}

void WAiNavigationComponent::Steer(WTransform& transform, float tDiff)
{
  if (m_State != WAiNavigationComponentState::Moving)
    return;

  if (WAiNavMeshWorldModule* pNavMeshModule = GetWorld()->GetOrCreateModule<WAiNavMeshWorldModule>())
  {
    m_Navigation.SetNavmesh(pNavMeshModule->GetNavMesh(m_sNavmeshConfig));
    m_Navigation.SetQueryFilter(pNavMeshModule->GetPathSearchFilter(m_sPathSearchConfig));
  }

  m_Navigation.SetCurrentPosition(GetOwner()->GetGlobalPosition());

  m_Navigation.Update();

  switch (m_Navigation.GetState())
  {
    case WAiNavigation::State::Idle:
      m_State = WAiNavigationComponentState::Idle;
      return;

    case WAiNavigation::State::InvalidCurrentPosition:
    case WAiNavigation::State::InvalidTargetPosition:
    case WAiNavigation::State::NoPathFound:
      m_State = WAiNavigationComponentState::Failed;
      return;

    case WAiNavigation::State::StartNewSearch:
    case WAiNavigation::State::Searching:
      return;

    case WAiNavigation::State::FullPathFound:
      break;

    case WAiNavigation::State::PartialPathSearchLimited:
    case WAiNavigation::State::PartialPathUnreachable:
      if (m_bAllowPartialPath)
        break;

      m_State = WAiNavigationComponentState::Failed;
      return;
  }

  if (m_fSpeed <= 0)
    return;

  WVec2 vForwardDir = GetOwner()->GetGlobalDirForwards().GetAsVec2();
  vForwardDir.NormalizeIfNotZero(WVec2(1, 0)).IgnoreResult();

  m_Steering.m_fMaxSpeed = m_fSpeed;
  m_Steering.m_vPosition = GetOwner()->GetGlobalPosition();
  m_Steering.m_qRotation = GetOwner()->GetGlobalRotation();
  m_Steering.m_vVelocity = GetOwner()->GetLinearVelocity();
  m_Steering.m_fAcceleration = m_fAcceleration;
  m_Steering.m_fDecceleration = m_fDecceleration;

  // TODO: hard-coded values
  m_Steering.m_MinTurnSpeed = WAngle::MakeFromDegree(180);

  const float fBrakingDistance = 1.2f * (WMath::Square(m_Steering.m_fMaxSpeed) / (2.0f * m_Steering.m_fDecceleration));

  m_Navigation.ComputeSteeringInfo(m_Steering.m_Info, vForwardDir, fBrakingDistance);
  // m_Steering.m_Info.m_vDirectionTowardsWaypoint.Set(1, 0);

  if (m_fStopWalkDistance < WMath::HighValue<float>())
  {
    m_Steering.m_Info.m_fArrivalDistance = WMath::Min(m_fStopWalkDistance, m_Steering.m_Info.m_fArrivalDistance);
    m_Steering.m_Info.m_fDistanceToWaypoint = WMath::Min(m_fStopWalkDistance, m_Steering.m_Info.m_fDistanceToWaypoint);
  }

  m_Steering.Calculate(tDiff, GetWorld());

  const WVec2 vMove = m_Steering.m_vPosition.GetAsVec2() - transform.m_vPosition.GetAsVec2();
  const float fMoveDist = vMove.GetLength();

  if (m_fStopWalkDistance < WMath::HighValue<float>())
  {
    m_fStopWalkDistance = WMath::Max(0.0f, m_fStopWalkDistance - fMoveDist);
  }

  const float fSpeed = fMoveDist / tDiff;

  transform.m_vPosition = m_Steering.m_vPosition;
  transform.m_qRotation = m_Steering.m_qRotation;

  if (fSpeed < 0.2f && (m_fStopWalkDistance <= 0.1f || ((m_Navigation.GetTargetPosition().GetAsVec2() - m_Steering.m_vPosition.GetAsVec2()).GetLengthSquared() < WMath::Square(m_fReachedDistance))))
  {
    // reached the goal
    CancelNavigation();
    m_State = WAiNavigationComponentState::Idle;
    return;
  }
}

void WAiNavigationComponent::Turn(WTransform& transform, float tDiff)
{
  if (m_State != WAiNavigationComponentState::Turning)
    return;

  WAngle turnSpeed = WAngle::MakeFromDegree(360);

  const WAngle remainingAngle = GetTurnAngleTowards(m_vTurnTowardsPos);
  const WAngle rotateNow = tDiff * turnSpeed;

  WAngle toRotate;

  if (rotateNow >= remainingAngle)
  {
    toRotate = remainingAngle;
    m_State = WAiNavigationComponentState::Idle;
  }
  else
  {
    toRotate = rotateNow;
  }

  const WQuat qRot = WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisZ(), toRotate);
  const WVec3 vNewLookDir = qRot * GetOwner()->GetGlobalDirForwards();

  transform.m_qRotation = WQuat::MakeShortestRotation(WVec3::MakeAxisX(), vNewLookDir);
}

void WAiNavigationComponent::PlaceOnGround(WTransform& transform, float tDiff)
{
  if (m_fFootRadius <= 0.0f)
    return;

  WPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<WPhysicsWorldModuleInterface>();

  if (pPhysicsInterface == nullptr)
    return;

  const WVec3 vDown = -WVec3::MakeAxisZ();
  const float fDistUp = 1.0f;
  const float fDistDown = m_fFallHeight;
  const WVec3 vStartPos = transform.m_vPosition - fDistUp * vDown;

  float fMoveUp = 0.0f;
  bool bHadCollision = false;

  WPhysicsCastResult res;
  WPhysicsQueryParameters params(m_uiCollisionLayer, WPhysicsShapeType::Static);
  if (pPhysicsInterface->SweepTestSphere(res, m_fFootRadius, vStartPos, vDown, fDistUp + fDistDown, params))
  {
    if (res.m_fDistance == 0.0f)
    {
      // ran into an obstacle
      // this shouldn't happen when walking just on the navmesh, as long as foot radius is smaller than the character radius
      // it can easily happen, once outside forces push the NPC around, but then one should really use a proper
      // physics character controller to avoid geometry
      return;
    }

    // found an intersection within the search radius
    bHadCollision = true;
    const float fFloorHeight = vStartPos.z - res.m_fDistance - m_fFootRadius;
    fMoveUp = fFloorHeight - transform.m_vPosition.z;
  }
  else
  {
    // did not find an intersection -> falling down
    fMoveUp = -fDistDown; // will be clamped by gravity
    CancelNavigation();
    m_State = WAiNavigationComponentState::Falling;
  }

  if (fMoveUp > 0.0f)
  {
    // if the character is being pushed up

    // TODO: better lerp up
    transform.m_vPosition.z += fMoveUp * WMath::Min(1.0f, 25.0f * tDiff);

    m_fFallSpeed = 0.0f;
  }
  else
  {
    m_fFallSpeed += pPhysicsInterface->GetGravity().z * tDiff;

    float fFallDist = m_fFallSpeed * tDiff;

    if (fFallDist < fMoveUp)
    {
      // clamp to the maximum speed / or to the floor
      fFallDist = fMoveUp;

      if (bHadCollision)
      {
        m_fFallSpeed = 0.0f;

        if (m_State == WAiNavigationComponentState::Falling)
        {
          // we just landed from a high fall -> starting to walk again probably makes no sense, since we obviously left the navmesh
          m_State = WAiNavigationComponentState::Fallen;
        }
      }
    }

    transform.m_vPosition.z += fFallDist;
  }
}

W_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Components_NavigationComponent);
