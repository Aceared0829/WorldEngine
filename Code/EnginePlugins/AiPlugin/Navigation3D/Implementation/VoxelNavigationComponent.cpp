#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/SteeringUtils.h>
#include <AiPlugin/Navigation3D/VoxelGridComponent.h>
#include <AiPlugin/Navigation3D/VoxelNavigationComponent.h>
#include <AiPlugin/Navigation3D/VoxelWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_BITFLAGS(WAiVoxelNavigationDebugFlags, 1)
  W_BITFLAGS_CONSTANTS(WAiVoxelNavigationDebugFlags::PrintState, WAiVoxelNavigationDebugFlags::VisPath)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_STATIC_REFLECTED_ENUM(WAiVoxelNavigationComponentState, 1)
  W_ENUM_CONSTANTS(WAiVoxelNavigationComponentState::Idle, WAiVoxelNavigationComponentState::Moving, WAiVoxelNavigationComponentState::Failed)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_COMPONENT_TYPE(WAiVoxelNavigationComponent, 1, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("NavigationTarget", DummyGetter, SetNavigationTargetReference)->AddAttributes(new WGameObjectReferenceAttribute()),
    W_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new WDefaultValueAttribute(5.0f)),
    W_MEMBER_PROPERTY("Acceleration", m_fAcceleration)->AddAttributes(new WDefaultValueAttribute(3.0f)),
    W_MEMBER_PROPERTY("Deceleration", m_fDeceleration)->AddAttributes(new WDefaultValueAttribute(8.0f)),
    W_MEMBER_PROPERTY("ReachedDistance", m_fReachedDistance)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 10.0f)),
    W_MEMBER_PROPERTY("ApplySteering", m_bApplySteering)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("LookAheadDistance", m_fLookAheadDistance)->AddAttributes(new WDefaultValueAttribute(3.0f), new WClampValueAttribute(0.1f, WVariant())),
    W_MEMBER_PROPERTY("MaxPathOffset", m_fMaxPathOffset)->AddAttributes(new WDefaultValueAttribute(3.0f), new WClampValueAttribute(0.1f, WVariant())),
    W_MEMBER_PROPERTY("CorridorCorrectionRate", m_fCorridorCorrectionRate)->AddAttributes(new WDefaultValueAttribute(8.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("MaxAngularSpeed", m_MaxAngularSpeed)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(180.0f)), new WClampValueAttribute(WAngle::MakeFromDegree(0.0f), WVariant())),
    W_MEMBER_PROPERTY("BankAmount", m_fBankAmount)->AddAttributes(new WDefaultValueAttribute(3.0f)),
    W_MEMBER_PROPERTY("MaxBankAngle", m_MaxBankAngle)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(30.0f)), new WClampValueAttribute(WAngle::MakeFromDegree(0.0f), WAngle::MakeFromDegree(90.0f))),
    W_BITFLAGS_MEMBER_PROPERTY("DebugFlags", WAiVoxelNavigationDebugFlags, m_DebugFlags),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("AI/Navigation"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SetDestination, In, "Destination"),
    W_SCRIPT_FUNCTION_PROPERTY(SetDestinationDirect, In, "Destination"),
    W_SCRIPT_FUNCTION_PROPERTY(SetNavigationTarget, In, "Object"),
    W_SCRIPT_FUNCTION_PROPERTY(CancelNavigation),
    W_SCRIPT_FUNCTION_PROPERTY(GetState),
    W_SCRIPT_FUNCTION_PROPERTY(TurnTowards, In, "Direction"),
    W_SCRIPT_FUNCTION_PROPERTY(GetTurnAngleTowards, In, "Direction"),
    W_SCRIPT_FUNCTION_PROPERTY(IsNavigating),
    W_SCRIPT_FUNCTION_PROPERTY(IsApproachingDestination),
    W_SCRIPT_FUNCTION_PROPERTY(FindRandomPointAroundSphere, In, "Center", In, "Radius", In, "MaxAttempts", Out, "Point"),
    W_SCRIPT_FUNCTION_PROPERTY(GetValidCellNearby, In, "Start", In, "SearchRadius", Out, "Point"),
  }
  W_END_FUNCTIONS;
}
W_END_COMPONENT_TYPE
// clang-format on

WAiVoxelNavigationComponent::WAiVoxelNavigationComponent() = default;
WAiVoxelNavigationComponent::~WAiVoxelNavigationComponent() = default;

void WAiVoxelNavigationComponent::SetNavigationTargetReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetNavigationTarget(resolver(szReference, GetHandle(), "NavigationTarget"));
}

void WAiVoxelNavigationComponent::SetNavigationTarget(WGameObjectHandle hObject)
{
  m_hNavigationTarget = hObject;
}

void WAiVoxelNavigationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_uiSkipNextFrames = 3;
  m_vSteerPosition = GetOwner()->GetGlobalPosition();
  m_vPathPosition = GetOwner()->GetGlobalPosition(); // placeholder until the first SetDestination()/SetDestinationDirect() snaps it to a valid voxel
  m_bPathPositionInitialized = false;
  m_fPathSpeed = 0.0f;
  m_qSteerRotation = GetOwner()->GetGlobalRotation();
  m_vVelocity = WVec3::MakeZero();
}

void WAiVoxelNavigationComponent::SetDestination(const WVec3& vGlobalPos)
{
  auto* pVoxelModule = GetWorld()->GetOrCreateModule<WAiVoxelWorldModule>();
  if (pVoxelModule == nullptr || !pVoxelModule->IsReady())
  {
    // Grid not ready yet, don't treat as failure - caller can retry
    return;
  }

  // Track the actual voxel size of the grid we're in, so the runtime thresholds derived from it
  // (arrival/repath distances in Update()) scale with the grid instead of a fixed assumption.
  {
    WTempHybridArray<WAiVoxelGridComponent*, 8> grids;
    pVoxelModule->FindGridsInBox(WBoundingBox::MakeFromCenterAndHalfExtents(GetOwner()->GetGlobalPosition(), WVec3(0.01f)), grids);
    if (!grids.IsEmpty())
      m_fVoxelSize = grids[0]->GetVoxelSize();
  }

  if (!m_bPathPositionInitialized)
  {
    WVec3 vSnapped;
    if (GetValidCellNearby(GetOwner()->GetGlobalPosition(), 5.0f, vSnapped))
      m_vPathPosition = vSnapped;
    else
      m_vPathPosition = GetOwner()->GetGlobalPosition(); // best effort; FindPath() reports InvalidStartPosition below if this is still bad

    m_bPathPositionInitialized = true;
  }

  const WVec3 vCurrentPos = m_vPathPosition;

  m_vLastPathTargetPos = vGlobalPos;

  const WAiVoxelGridFinder gridFinder = [pVoxelModule](const WBoundingBox& searchBox, WDynamicArray<const WVoxelGrid*>& out_grids)
  {
    WHybridArray<WAiVoxelGridComponent*, 4> gridComponents;
    pVoxelModule->FindGridsInBox(searchBox, gridComponents);

    out_grids.Clear();
    for (const WAiVoxelGridComponent* pGridComponent : gridComponents)
      out_grids.PushBack(&pGridComponent->GetStaticVoxelGrid());
  };

  const auto result = m_Navigation.FindPath(vCurrentPos, vGlobalPos, gridFinder);

  switch (result)
  {
    case WAiVoxelNavigation::State::PathFound:
      m_State = WAiVoxelNavigationComponentState::Moving;
      // Deliberately not resetting m_vVelocity here: rerouting an already-moving object (e.g.
      // repathing towards a moving target) should carry momentum over smoothly instead of
      // stuttering to a stop and re-accelerating on every reroute.
      break;

    case WAiVoxelNavigation::State::InvalidStartPosition:
    case WAiVoxelNavigation::State::InvalidTargetPosition:
    default:
      // Failure reason is available via m_Navigation.GetState() if the caller needs to
      // distinguish InvalidStartPosition/InvalidTargetPosition/NoPathFound.
      m_State = WAiVoxelNavigationComponentState::Failed;
      break;
  }
}

void WAiVoxelNavigationComponent::SetDestinationDirect(const WVec3& vGlobalPos)
{
  if (!m_bPathPositionInitialized)
  {
    WVec3 vSnapped;
    if (GetValidCellNearby(GetOwner()->GetGlobalPosition(), 5.0f, vSnapped))
      m_vPathPosition = vSnapped;
    else
      m_vPathPosition = GetOwner()->GetGlobalPosition();

    m_bPathPositionInitialized = true;
  }

  const WVec3 vCurrentPos = m_vPathPosition;

  m_vLastPathTargetPos = vGlobalPos;
  m_Navigation.SetDirectPath(vCurrentPos, vGlobalPos);

  // Deliberately not resetting m_vVelocity here, same reasoning as SetDestination().
  m_State = WAiVoxelNavigationComponentState::Moving;
}

void WAiVoxelNavigationComponent::CancelNavigation()
{
  m_Navigation.CancelNavigation();
  m_State = WAiVoxelNavigationComponentState::Idle;

  // Deliberately not resetting m_vVelocity here: Update() decelerates to a stop gracefully via
  // DecelerateToStop() instead of coming to an instant halt.

  // Otherwise Update() would immediately re-navigate towards the tracked target again.
  m_hNavigationTarget.Invalidate();
}

WAngle WAiVoxelNavigationComponent::TurnTowards(const WVec3& vDirection)
{
  const float fTimeDiff = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();
  const WAngle remaining = WAiSteeringUtils::TurnTowards(m_qSteerRotation, vDirection, m_MaxAngularSpeed, fTimeDiff);

  if (m_bApplySteering)
  {
    GetOwner()->SetGlobalRotation(m_qSteerRotation);
  }

  return remaining;
}

WAngle WAiVoxelNavigationComponent::GetTurnAngleTowards(const WVec3& vDirection) const
{
  return WAiSteeringUtils::GetAngleTowards(m_qSteerRotation, vDirection);
}

float WAiVoxelNavigationComponent::GetRemainingDistance() const
{
  if (m_State != WAiVoxelNavigationComponentState::Moving)
    return 0.0f;

  const auto& waypoints = m_Navigation.GetWaypoints();
  const WUInt32 uiCurrent = m_Navigation.GetCurrentWaypointIndex();

  if (uiCurrent >= waypoints.GetCount())
    return 0.0f;

  float fDist = (waypoints[uiCurrent] - m_vPathPosition).GetLength();

  for (WUInt32 i = uiCurrent; i + 1 < waypoints.GetCount(); ++i)
  {
    fDist += (waypoints[i + 1] - waypoints[i]).GetLength();
  }

  return fDist;
}

bool WAiVoxelNavigationComponent::IsApproachingDestination() const
{
  if (m_State != WAiVoxelNavigationComponentState::Moving)
    return false;

  const float fBrakingDistance = ComputeBrakingDistance(m_fSpeed);
  return fBrakingDistance > 0.0f && GetRemainingDistance() < fBrakingDistance;
}

bool WAiVoxelNavigationComponent::FindRandomPointAroundSphere(const WVec3& vCenter, float fRadius, WUInt32 uiMaxAttempts, WVec3& out_vPoint)
{
  auto* pVoxelModule = GetWorld()->GetModule<WAiVoxelWorldModule>();
  if (pVoxelModule == nullptr || !pVoxelModule->IsReady())
    return false;

  const WBoundingBox searchBox = WBoundingBox::MakeFromCenterAndHalfExtents(vCenter, WVec3(fRadius));
  WTempHybridArray<WAiVoxelGridComponent*, 8> grids;
  pVoxelModule->FindGridsInBox(searchBox, grids);

  WRandom& rng = GetWorld()->GetRandomNumberGenerator();

  for (WUInt32 i = 0; i < uiMaxAttempts; ++i)
  {
    const WVec3 vCandidate = vCenter + WVec3::MakeRandomPointInSphere(rng) * fRadius;

    bool bBlocked = false;

    for (const WAiVoxelGridComponent* pGridComponent : grids)
    {
      const WVoxelGrid& grid = pGridComponent->GetStaticVoxelGrid();

      const WVec3I32 vCoord = grid.WorldToCoord(vCandidate);
      if (!grid.IsCoordValid(vCoord))
      {
        // outside this grid -> check the next one
        continue;
      }

      // Covered by a grid: solid means try another candidate, free means this point is good.
      bBlocked = grid.IsVoxelSet(vCoord);
      break;
    }

    // Accepted if free within its covering grid, or not covered by any grid at all (space outside
    // all grids is free, same convention as pathfinding and GetValidCellNearby()).
    if (!bBlocked)
    {
      out_vPoint = vCandidate;
      return true;
    }
  }

  return false;
}

bool WAiVoxelNavigationComponent::GetValidCellNearby(const WVec3& vStart, float fSearchRadius, WVec3& out_vPoint) const
{
  auto* pVoxelModule = GetWorld()->GetModule<WAiVoxelWorldModule>();
  if (pVoxelModule == nullptr || !pVoxelModule->IsReady())
    return false;

  const WBoundingBox searchBox = WBoundingBox::MakeFromCenterAndHalfExtents(vStart, WVec3(fSearchRadius));
  WTempHybridArray<WAiVoxelGridComponent*, 8> grids;
  pVoxelModule->FindGridsInBox(searchBox, grids);

  // Find which grid (if any) actually covers vStart - only that grid's occupancy is relevant to
  // whether vStart, and its neighborhood, are free.
  const WAiVoxelGridComponent* pOwningGrid = nullptr;
  WVec3I32 vStartCoord;

  for (const WAiVoxelGridComponent* pGridComponent : grids)
  {
    const WVoxelGrid& grid = pGridComponent->GetStaticVoxelGrid();
    const WVec3I32 vCoord = grid.WorldToCoord(vStart);

    if (grid.IsCoordValid(vCoord))
    {
      pOwningGrid = pGridComponent;
      vStartCoord = vCoord;
      break;
    }
  }

  if (pOwningGrid == nullptr)
  {
    // Not covered by any nearby grid -> free space, same convention as pathfinding.
    out_vPoint = vStart;
    return true;
  }

  const WVoxelGrid& grid = pOwningGrid->GetStaticVoxelGrid();

  if (!grid.IsVoxelSet(vStartCoord))
  {
    out_vPoint = vStart;
    return true;
  }

  // vStart is blocked - scan every voxel within fSearchRadius and keep the physically closest free
  // one found. Simple exhaustive scan rather than an expanding-shell search: fSearchRadius is
  // expected to stay small (this is meant for local recovery, not long-range searches).
  const float fVoxelSize = grid.GetVoxelSize();
  const WInt32 iMaxSteps = WMath::Max(1, (WInt32)WMath::Ceil(fSearchRadius / fVoxelSize));

  float fBestDistSqr = fSearchRadius * fSearchRadius;
  bool bFound = false;

  for (WInt32 dz = -iMaxSteps; dz <= iMaxSteps; ++dz)
  {
    for (WInt32 dy = -iMaxSteps; dy <= iMaxSteps; ++dy)
    {
      for (WInt32 dx = -iMaxSteps; dx <= iMaxSteps; ++dx)
      {
        if (dx == 0 && dy == 0 && dz == 0)
          continue;

        const WVec3I32 vCoord = vStartCoord + WVec3I32(dx, dy, dz);
        if (!grid.IsCoordValid(vCoord) || grid.IsVoxelSet(vCoord))
          continue;

        const WVec3 vCandidate = grid.CoordToWorld(vCoord);
        const float fDistSqr = (vCandidate - vStart).GetLengthSquared();

        if (fDistSqr < fBestDistSqr)
        {
          fBestDistSqr = fDistSqr;
          out_vPoint = vCandidate;
          bFound = true;
        }
      }
    }
  }

  return bFound;
}

void WAiVoxelNavigationComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_fSpeed;
  s << m_fAcceleration;
  s << m_fDeceleration;
  s << m_fReachedDistance;
  s << m_bApplySteering;
  s << m_DebugFlags;
  inout_stream.WriteGameObjectHandle(m_hNavigationTarget);
  s << m_fLookAheadDistance;
  s << m_MaxAngularSpeed;
  s << m_fBankAmount;
  s << m_MaxBankAngle;
  s << m_fMaxPathOffset;
  s << m_fCorridorCorrectionRate;
}

void WAiVoxelNavigationComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  WStreamReader& s = inout_stream.GetStream();
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_fSpeed;
  s >> m_fAcceleration;
  s >> m_fDeceleration;
  s >> m_fReachedDistance;
  s >> m_bApplySteering;
  s >> m_DebugFlags;
  m_hNavigationTarget = inout_stream.ReadGameObjectHandle();
  s >> m_fLookAheadDistance;
  s >> m_MaxAngularSpeed;
  s >> m_fBankAmount;
  s >> m_MaxBankAngle;
  s >> m_fMaxPathOffset;
  s >> m_fCorridorCorrectionRate;
}

void WAiVoxelNavigationComponent::Update()
{
  if (m_uiSkipNextFrames > 0)
  {
    m_uiSkipNextFrames--;
    return;
  }

  const float tDiff = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

  // If a navigation target is set, keep the path aimed at its current position
  if (!m_hNavigationTarget.IsInvalidated())
  {
    WGameObject* pTarget = nullptr;
    if (GetWorld()->TryGetObject(m_hNavigationTarget, pTarget))
    {
      const WVec3 vTargetPos = pTarget->GetGlobalPosition();
      const float fDistToTarget = (vTargetPos - m_vPathPosition).GetLength();

      // Only re-navigate if we're not already at the target
      const float fArrivalThreshold = WMath::Max(m_fReachedDistance, m_fVoxelSize * 0.5f);

      if (m_State == WAiVoxelNavigationComponentState::Idle)
      {
        if (fDistToTarget > fArrivalThreshold)
        {
          SetDestination(vTargetPos);
        }
      }
      else if (m_State == WAiVoxelNavigationComponentState::Moving)
      {
        // Target keeps moving while we're already following a path towards it - periodically
        // recompute the path so it doesn't go stale (e.g. prey changing direction mid-chase).
        m_fRepathCooldown -= tDiff;

        const float fRepathThreshold = WMath::Max(m_fVoxelSize * 2.0f, 1.0f);
        if (m_fRepathCooldown <= 0.0f && (vTargetPos - m_vLastPathTargetPos).GetLength() > fRepathThreshold)
        {
          SetDestination(vTargetPos);
          m_fRepathCooldown = 0.5f;
        }
      }
    }
  }

  if (m_State == WAiVoxelNavigationComponentState::Moving)
  {
    MoveAlongPath(tDiff);
  }
  else if (!m_vVelocity.IsZero(0.0001f))
  {
    DecelerateToStop(tDiff);
  }

  if (m_DebugFlags.IsAnyFlagSet())
  {
    if (m_DebugFlags.IsSet(WAiVoxelNavigationDebugFlags::PrintState))
    {
      const WVec3 vPosition = GetOwner()->GetGlobalPosition() + WVec3(0, 0, 1.5f);

      switch (m_State)
      {
        case WAiVoxelNavigationComponentState::Idle:
          WDebugRenderer::Draw3DText(GetWorld(), "Idle", vPosition, WColor::Grey);
          break;
        case WAiVoxelNavigationComponentState::Moving:
          WDebugRenderer::Draw3DText(GetWorld(), "Moving", vPosition, WColor::Yellow);
          break;
        case WAiVoxelNavigationComponentState::Failed:
          WDebugRenderer::Draw3DText(GetWorld(), "Failed", vPosition, WColor::Red);
          break;
      }
    }

    if (m_DebugFlags.IsSet(WAiVoxelNavigationDebugFlags::VisPath))
    {
      m_Navigation.DebugDrawPath(GetWorld(), WColor::DeepSkyBlue);
    }
  }
}

void WAiVoxelNavigationComponent::MoveAlongPath(float fTimeDiff)
{
  if (m_Navigation.IsPathComplete())
  {
    m_State = WAiVoxelNavigationComponentState::Idle;
    // Leave m_vVelocity as-is - DecelerateToStop() picks it up next frame.
    return;
  }

  const WVec3 vFinalWaypoint = m_Navigation.GetWaypoints().PeekBack();
  const float fRemainingDist = GetRemainingDistance();
  const float fBrakingDistance = ComputeBrakingDistance(m_fSpeed);

  // How well the ship is currently facing where it needs to go, measured against a look-ahead
  // point from the path position *before* advancing it this frame. Drives the speed of both the
  // ground-truth path advance and the visual steering below: a ship that still has to turn around
  // (e.g. a reversed destination) should brake to a near-stop and wait for its heading to catch up
  // rather than let the path position race ahead of it - otherwise the visual position, which can
  // only move nose-first, falls far behind and either has to be yanked back into the corridor or
  // silently lags, both of which look wrong.
  const WVec3 vLookAheadForTurn = m_Navigation.GetLookAheadPoint(m_vPathPosition, m_fLookAheadDistance);
  WVec3 vDesiredDir = vLookAheadForTurn - m_vSteerPosition;
  if (vDesiredDir.IsZero(0.0001f))
  {
    vDesiredDir = m_qSteerRotation * WVec3::MakeAxisX();
  }

  const WAngle angleToTarget = WAiSteeringUtils::GetAngleTowards(m_qSteerRotation, vDesiredDir);
  SteerAndBank(vDesiredDir, fTimeDiff);

  const float fTurnFactor = WMath::Clamp(1.0f - (angleToTarget / WAngle::MakeFromDegree(90.0f)), 0.05f, 1.0f);

  // --- Ground-truth path progress: an arc-length walk along the path polyline, which is
  // guaranteed clear (A*'d through free voxels, then string-pulled) - no runtime voxel validation
  // needed here, unlike the free-form visual steering below. Waypoint advancement is a side effect
  // of AdvanceAlongPath(), replacing the old per-waypoint plane-crossing loop entirely. Throttled by
  // fTurnFactor so it can't outrun the visual position while the ship is still turning to face it.
  float fPathTargetSpeed = m_fSpeed * fTurnFactor;
  if (fBrakingDistance > 0.0f && fRemainingDist < fBrakingDistance)
  {
    fPathTargetSpeed = WMath::Max(fPathTargetSpeed * (fRemainingDist / fBrakingDistance), 0.5f);
  }

  m_fPathSpeed = WAiSteeringUtils::ApplyAcceleration(m_fPathSpeed, fPathTargetSpeed, m_fAcceleration, m_fDeceleration, fTimeDiff);
  m_vPathPosition = m_Navigation.AdvanceAlongPath(m_vPathPosition, m_fPathSpeed * fTimeDiff);

  if ((m_vPathPosition - vFinalWaypoint).GetLength() <= m_fReachedDistance)
  {
    m_State = WAiVoxelNavigationComponentState::Idle;
    // Fall through - still steer the visual position towards the final point this frame instead
    // of snapping/stopping abruptly.
  }

  // --- Visual (nose-first) steering: free-form, chases a look-ahead point on the path. Explicitly
  // not voxel-validated - it may lag or stray from the path, corrected by the corridor clamp below
  // rather than by blocking movement outright. This is what actually gets applied to the transform.
  // Target speed: base speed, reduced for sharp turns (nose-first movement can't move fast while
  // turning hard) and for braking near the end of the path - same fTurnFactor as the path speed
  // above, so both ease off together instead of the path racing ahead of what's actually turning.
  float fVisualTargetSpeed = m_fSpeed * fTurnFactor;
  if (fBrakingDistance > 0.0f && fRemainingDist < fBrakingDistance)
  {
    fVisualTargetSpeed = WMath::Max(fVisualTargetSpeed * (fRemainingDist / fBrakingDistance), 0.5f);
  }

  // Nose-first speed integration: velocity always points in the current facing direction.
  const float fNewSpeed = WAiSteeringUtils::ApplyAcceleration(m_vVelocity.GetLength(), fVisualTargetSpeed, m_fAcceleration, m_fDeceleration, fTimeDiff);
  m_vVelocity = (m_qSteerRotation * WVec3::MakeAxisX()) * fNewSpeed;

  WVec3 vNewSteerPosition = m_vSteerPosition + m_vVelocity * fTimeDiff;
  vNewSteerPosition = ApplyCorridorClamp(vNewSteerPosition, fTimeDiff);

  m_vSteerPosition = vNewSteerPosition;

  if (m_bApplySteering)
  {
    GetOwner()->SetGlobalPosition(m_vSteerPosition);
    GetOwner()->SetGlobalRotation(m_qSteerRotation);
  }
}

float WAiVoxelNavigationComponent::ComputeBrakingDistance(float fSpeed) const
{
  return (fSpeed * fSpeed) / (2.0f * m_fDeceleration);
}

WVec3 WAiVoxelNavigationComponent::ApplyCorridorClamp(const WVec3& vCandidate, float fTimeDiff) const
{
  const WVec3 vOffset = vCandidate - m_vPathPosition;
  const float fOffsetDist = vOffset.GetLength();
  if (fOffsetDist <= m_fMaxPathOffset)
    return vCandidate;

  // Soft pull toward the corridor boundary (not all the way to the path position), proportional to
  // how far outside the corridor the point is - avoids a visible kink/snap when first crossing
  // MaxPathOffset, and lets the object visibly hug the outside of a tight corner rather than being
  // sucked onto the path's exact line.
  const float fPullAlpha = 1.0f - WMath::Exp(-m_fCorridorCorrectionRate * fTimeDiff);
  const WVec3 vCorridorEdge = m_vPathPosition + vOffset * (m_fMaxPathOffset / fOffsetDist);
  return WMath::Lerp(vCandidate, vCorridorEdge, fPullAlpha);
}

void WAiVoxelNavigationComponent::DecelerateToStop(float fTimeDiff)
{
  const float fCurrentSpeed = m_vVelocity.GetLength();
  const WVec3 vDir = m_vVelocity / fCurrentSpeed; // safe: only called when velocity is non-zero
  const float fNewSpeed = WAiSteeringUtils::ApplyAcceleration(fCurrentSpeed, 0.0f, m_fAcceleration, m_fDeceleration, fTimeDiff);

  m_vVelocity = vDir * fNewSpeed;

  // Braking glide (e.g. after CancelNavigation() mid-turn) is free-form, visual-only movement, same
  // as MoveAlongPath()'s visual step - not voxel-validated, just kept within the path corridor.
  m_vSteerPosition = ApplyCorridorClamp(m_vSteerPosition + m_vVelocity * fTimeDiff, fTimeDiff);

  // Keep facing the direction it was already moving/braking in - but still relax back to level
  // (and decay any leftover bank) since nothing is actively steering it anymore.
  SteerAndBank(vDir, fTimeDiff);

  if (m_bApplySteering)
  {
    GetOwner()->SetGlobalPosition(m_vSteerPosition);
    GetOwner()->SetGlobalRotation(m_qSteerRotation);
  }
}

void WAiVoxelNavigationComponent::SteerAndBank(const WVec3& vDesiredDir, float fTimeDiff)
{
  const WVec3 vOldForward = m_qSteerRotation * WVec3::MakeAxisX();

  // Also re-levels any existing roll, even if vDesiredDir == vOldForward (no actual turn).
  WAiSteeringUtils::TurnTowards(m_qSteerRotation, vDesiredDir, m_MaxAngularSpeed, fTimeDiff);
  m_qSteerRotation.Normalize();

  const WVec3 vNewForward = m_qSteerRotation * WVec3::MakeAxisX();
  const WAngle turnStepAngle = vOldForward.GetAngleBetween(vNewForward);

  WAngle bankTarget = WAngle::MakeFromRadian(0.0f);
  if (turnStepAngle > WAngle::MakeFromDegree(0.01f) && m_fBankAmount != 0.0f)
  {
    // Negative BankAmount flips which way the ship leans into a turn (some visuals read better
    // banking "outward" rather than "into" the curve) - clamp the magnitude to MaxBankAngle
    // regardless of sign, then apply the turn direction and the user-chosen bank direction together.
    const float fTurnSign = WMath::Sign(vOldForward.CrossRH(vNewForward).Dot(WVec3::MakeAxisZ()));
    const float fBankSign = WMath::Sign(m_fBankAmount);
    const WAngle tiltMagnitude = WMath::Min(turnStepAngle * WMath::Abs(m_fBankAmount), m_MaxBankAngle);
    bankTarget = tiltMagnitude * fTurnSign * fBankSign;
  }

  // Smooth the bank angle over time so it eases in/out instead of snapping, and decays back to
  // level (0) once the turn ends. Frame-rate independent exponential smoothing (same form as the
  // corridor clamp); the rate is tuned to retain ~0.85 of the previous angle per frame at 60 Hz.
  const float fBankSmoothingRate = 10.0f;
  const float fBankAlpha = 1.0f - WMath::Exp(-fBankSmoothingRate * fTimeDiff);
  m_BankAngle = WMath::Lerp(m_BankAngle, bankTarget, fBankAlpha);

  if (m_BankAngle.GetRadian() != 0.0f)
  {
    m_qSteerRotation = m_qSteerRotation * WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisX(), m_BankAngle);
  }
}

W_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation3D_Implementation_VoxelNavigationComponent);
