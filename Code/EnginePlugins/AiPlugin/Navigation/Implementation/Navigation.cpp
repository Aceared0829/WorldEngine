#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <DetourNavMesh.h>
#include <Foundation/Math/Rect.h>
#include <Recast.h>
#include <RendererCore/Debug/DebugRenderer.h>

WResult FindNavMeshPolyAt(dtNavMeshQuery& ref_query, const dtQueryFilter* pQueryFilter, WRcPos position, dtPolyRef& out_polyRef, WVec3* out_pAdjustedPosition /*= nullptr*/, float fPlaneEpsilon /*= 0.01f*/, float fHeightEpsilon /*= 1.0f*/)
{
  WVec3 vSize(fPlaneEpsilon, fHeightEpsilon, fPlaneEpsilon);

  WRcPos resultPos;
  if (dtStatusFailed(ref_query.findNearestPoly(position, &vSize.x, pQueryFilter, &out_polyRef, resultPos)))
    return W_FAILURE;

  if (out_polyRef == 0)
    return W_FAILURE;

  if (!WMath::IsEqual(position.m_Pos[0], resultPos.m_Pos[0], fPlaneEpsilon) ||
      !WMath::IsEqual(position.m_Pos[1], resultPos.m_Pos[1], fHeightEpsilon) ||
      !WMath::IsEqual(position.m_Pos[2], resultPos.m_Pos[2], fPlaneEpsilon))
  {
    return W_FAILURE;
  }

  if (out_pAdjustedPosition != nullptr)
  {
    *out_pAdjustedPosition = resultPos;
  }

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WAiNavigation::WAiNavigation()
{
  m_uiCurrentPositionChangedBit = 0;
  m_uiTargetPositionChangedBit = 0;
  m_uiReinitQueryBit = 0;
  m_uiOptimizeWhenFoundBit = 0;

  m_PathCorridor.init(MaxPathNodes);
}

WAiNavigation::~WAiNavigation() = default;

void WAiNavigation::Update()
{
  if (m_pNavmesh == nullptr || m_pFilter == nullptr)
    return;

  if (m_uiReinitQueryBit)
  {
    m_uiReinitQueryBit = 0;
    m_Query.init(m_pNavmesh->GetDetourNavMesh(), MaxSearchNodes);
  }

  if (!UpdatePathSearch())
    return;

  if (m_PathCorridor.getPathCount() == 0)
  {
    switch (m_State)
    {
      case State::Idle:
      case State::InvalidTargetPosition:
        if (m_uiTargetPositionChangedBit)
          m_State = State::StartNewSearch;
        break;

      case State::InvalidCurrentPosition:
        if (m_uiCurrentPositionChangedBit)
          m_State = State::StartNewSearch;
        break;

      case State::NoPathFound:
        if (m_uiCurrentPositionChangedBit || m_uiTargetPositionChangedBit)
          m_State = State::StartNewSearch;
        break;

        W_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    if (m_State == State::StartNewSearch)
    {
      // already kick off the path search
      UpdatePathSearch();
    }

    return;
  }

  if (m_uiCurrentPositionChangedBit)
  {
    // make sure the system creates the necessary sectors at some point
    {
      WRectFloat r = WRectFloat::MakeInvalid();
      r.ExpandToInclude(m_vCurrentPosition.GetAsVec2());
      r.Grow(c_fPathSearchBoundary);
      if (!m_pNavmesh->RequestSector(r.GetCenter(), r.GetHalfExtents()))
      {
        // The sectors around the new position aren't streamed in yet. Validating against the incomplete navmesh
        // below would spuriously report InvalidCurrentPosition, so defer to a full replan instead - the
        // StartNewSearch state waits on the very same sector gate until the navmesh is ready.
        m_State = State::StartNewSearch;
        return;
      }
    }

    const dtPolyRef firstPoly = m_PathCorridor.getFirstPoly();

    if (!m_PathCorridor.movePosition(WRcPos(m_vCurrentPosition), &m_Query, m_pFilter))
    {
      W_REPORT_FAILURE("Steered into invalid position."); // not sure under which conditions this can happen
      CancelNavigation();
      m_State = State::StartNewSearch;
      return;
    }

    WVec3 resPos = WRcPos(m_PathCorridor.getPos());
    if (!resPos.GetAsVec2().IsEqual(m_vCurrentPosition.GetAsVec2(), 0.2f))
    {
      const float fHalfSearchVert = (m_fPolySearchUp + m_fPolySearchDown) * 0.5f;
      const WVec3 vPosOffset(0, 0, m_fPolySearchUp - fHalfSearchVert);

      dtPolyRef startRef;
      if (FindNavMeshPolyAt(m_Query, m_pFilter, m_vCurrentPosition + vPosOffset, startRef, nullptr, m_fPolySearchRadius, fHalfSearchVert).Failed())
      {
        CancelNavigation();
        m_State = State::InvalidCurrentPosition;
        return;
      }
      else if (startRef != m_PathCorridor.getFirstPoly())
      {
        CancelNavigation();
        m_State = State::StartNewSearch;
        return;
      }

      // the character's center is outside the navmesh, but the expanded poly query still returns the same polygon
      // so we keep this path corridor
    }

    if (firstPoly != m_PathCorridor.getFirstPoly())
    {
      // the path corridor got modified (may got shorter or longer)
      m_uiOptimizeTopologyCounter++;
    }

    m_uiCurrentPositionChangedBit = 0;
  }

  if (m_uiTargetPositionChangedBit)
  {
    // make sure the system creates the necessary sectors at some point
    {
      WRectFloat r = WRectFloat::MakeInvalid();
      r.ExpandToInclude(m_vTargetPosition.GetAsVec2());
      r.Grow(c_fPathSearchBoundary);
      if (!m_pNavmesh->RequestSector(r.GetCenter(), r.GetHalfExtents()))
      {
        // The sectors around the new target aren't streamed in yet. Validating against the incomplete navmesh
        // below would spuriously report InvalidTargetPosition (e.g. right after retargeting to a far-away point),
        // so defer to a full replan instead - the StartNewSearch state waits on the very same sector gate until
        // the navmesh is ready.
        m_State = State::StartNewSearch;
        return;
      }
    }

    const dtPolyRef lastPoly = m_PathCorridor.getLastPoly();

    if (!m_PathCorridor.moveTargetPosition(WRcPos(m_vTargetPosition), &m_Query, m_pFilter))
    {
      WLog::Error("Target position not reachable anymore.");
      CancelNavigation();
      m_State = State::StartNewSearch;
      return;
    }

    WVec3 resPos = WRcPos(m_PathCorridor.getTarget());
    if (!resPos.GetAsVec2().IsEqual(m_vTargetPosition.GetAsVec2(), 0.2f))
    {
      const float fHalfSearchVert = (m_fPolySearchUp + m_fPolySearchDown) * 0.5f;
      const WVec3 vPosOffset(0, 0, m_fPolySearchUp - fHalfSearchVert);

      dtPolyRef endRef;
      if (FindNavMeshPolyAt(m_Query, m_pFilter, m_vTargetPosition + vPosOffset, endRef, nullptr, m_fPolySearchRadius, fHalfSearchVert).Failed())
      {
        CancelNavigation();
        m_State = State::InvalidTargetPosition;
        return;
      }
      else if (endRef != m_PathCorridor.getLastPoly())
      {
        CancelNavigation();
        m_State = State::StartNewSearch;
        return;
      }

      // the target's center is outside the navmesh, but the expanded poly query still returns the same polygon
      // so we keep this path corridor
    }

    if (lastPoly != m_PathCorridor.getLastPoly())
    {
      // the path corridor got modified (may got shorter or longer)
      m_uiOptimizeTopologyCounter++;
    }

    m_uiTargetPositionChangedBit = 0;
  }

  if (m_uiOptimizeTopologyCounter > 10)
  {
    m_uiOptimizeTopologyCounter = 0;
    m_PathCorridor.optimizePathTopology(&m_Query, m_pFilter);
  }
}

void WAiNavigation::CancelNavigation()
{
  m_PathCorridor.clear();
  m_uiTargetPositionChangedBit = 0; // don't start another path search
  m_State = State::Idle;
}

void WAiNavigation::SetCurrentPosition(const WVec3& vPosition)
{
  if (m_vCurrentPosition == vPosition)
    return;

  m_vCurrentPosition = vPosition;
  m_uiCurrentPositionChangedBit = 1;
}

void WAiNavigation::SetTargetPosition(const WVec3& vPosition, bool bOptimizeWhenFound /*= false*/)
{
  if (m_vTargetPosition != vPosition)
  {
    // A genuinely new target starts a fresh progress budget for the partial-path auto-repath guard. Guarded by
    // the comparison so that callers re-setting the same target every frame don't keep resetting the guard.
    m_fLastRepathStartDistToTarget = WMath::HighValue<float>();
  }

  m_vTargetPosition = vPosition;
  m_uiTargetPositionChangedBit = 1;

  if (bOptimizeWhenFound)
  {
    m_uiOptimizeWhenFoundBit = 1;
  }
}

const WVec3& WAiNavigation::GetTargetPosition() const
{
  return m_vTargetPosition;
}

void WAiNavigation::SetNavmesh(WAiNavMesh* pNavmesh)
{
  if (m_pNavmesh == pNavmesh)
    return;

  m_pNavmesh = pNavmesh;
  m_uiReinitQueryBit = 1;
}

void WAiNavigation::SetQueryFilter(const dtQueryFilter& filter)
{
  if (m_pFilter == &filter)
    return;

  m_pFilter = &filter;
}

void WAiNavigation::ComputeAllWaypoints(WDynamicArray<WVec3>& out_waypoints) const
{
  out_waypoints.Clear();

  if (m_PathCorridor.getPathCount() == 0)
    return;

  WUInt8 cornerFlags[MaxPathNodes];
  dtPolyRef cornerPolys[MaxPathNodes];
  WRcPos straightPath[MaxPathNodes];

  const int straightLen = m_PathCorridor.findCorners(straightPath[0], cornerFlags, cornerPolys, MaxPathNodes, &m_Query);

  out_waypoints.SetCountUninitialized((WUInt32)straightLen);

  for (int i = 0; i < straightLen; ++i)
  {
    out_waypoints[i] = straightPath[i]; // automatically swaps Y and Z
  }
}

void WAiNavigation::OptimizeCurrentPath()
{
  if (m_pNavmesh == nullptr || m_pFilter == nullptr)
    return;

  if (m_PathCorridor.getPathCount() == 0)
    return;

  // local area re-search to straighten out a weird corridor shape
  m_PathCorridor.optimizePathTopology(&m_Query, m_pFilter);
  m_uiOptimizeTopologyCounter = 0;

  // string-pull the corridor toward the farthest currently visible corner
  WUInt8 cornerFlags[MaxPathNodes];
  dtPolyRef cornerPolys[MaxPathNodes];
  WRcPos straightPath[MaxPathNodes];

  const int straightLen = m_PathCorridor.findCorners(straightPath[0], cornerFlags, cornerPolys, MaxPathNodes, &m_Query);

  if (straightLen > 0)
  {
    m_PathCorridor.optimizePathVisibility(straightPath[straightLen - 1], 10.0f, &m_Query, m_pFilter);
    m_uiOptimizeVisibilityCounter = 0;
  }
}

bool WAiNavigation::IsPointInPathCorridor(const WVec3& vPosition, float fHeightTolerance) const
{
  const WUInt32 uiCorrLen = m_PathCorridor.getPathCount();
  if (uiCorrLen == 0)
    return false;

  const dtPolyRef* pCorrArr = m_PathCorridor.getPath();
  const WRcPos rcPos(vPosition);

  for (WUInt32 c = 0; c < uiCorrLen; ++c)
  {
    WRcPos closest;
    bool bPosOverPoly = false;

    // closestPointOnPoly reports whether the point is directly over this polygon;
    // that is what makes this a corridor-containment test rather than a nearest-poly query.
    if (dtStatusFailed(m_Query.closestPointOnPoly(pCorrArr[c], rcPos, closest, &bPosOverPoly)))
      continue;

    if (bPosOverPoly)
    {
      const WVec3 vClosest(closest);
      if (WMath::Abs(vClosest.z - vPosition.z) <= fHeightTolerance)
        return true;
    }
  }

  return false;
}

bool WAiNavigation::UpdatePathSearch()
{
  if (m_State == State::StartNewSearch)
  {
    WRectFloat r = WRectFloat::MakeInvalid();
    r.ExpandToInclude(m_vCurrentPosition.GetAsVec2());
    r.ExpandToInclude(m_vTargetPosition.GetAsVec2());
    r.Grow(c_fPathSearchBoundary);

    if (!m_pNavmesh->RequestSector(r.GetCenter(), r.GetHalfExtents()))
    {
      // navmesh sectors aren't loaded yet
      return false;
    }

    m_uiCurrentPositionChangedBit = 0;

    const float fHalfSearchVert = (m_fPolySearchUp + m_fPolySearchDown) * 0.5f;
    const WVec3 vPosOffset(0, 0, m_fPolySearchUp - fHalfSearchVert);

    dtPolyRef startRef;
    if (FindNavMeshPolyAt(m_Query, m_pFilter, m_vCurrentPosition + vPosOffset, startRef, nullptr, m_fPolySearchRadius, fHalfSearchVert).Failed())
    {
      m_State = State::InvalidCurrentPosition;
      return false;
    }

    m_uiTargetPositionChangedBit = 0;

    if (FindNavMeshPolyAt(m_Query, m_pFilter, m_vTargetPosition + vPosOffset, m_PathSearchTargetPoly, nullptr, m_fPolySearchRadius, fHalfSearchVert).Failed())
    {
      m_State = State::InvalidTargetPosition;
      return false;
    }

    m_vPathSearchTargetPos = m_vTargetPosition;
    if (dtStatusFailed(m_Query.initSlicedFindPath(startRef, m_PathSearchTargetPoly, WRcPos(m_vCurrentPosition), WRcPos(m_vTargetPosition), m_pFilter)))
    {
      m_State = State::NoPathFound;
      W_REPORT_FAILURE("Detour: initSlicedFindPath failed.");
      return false;
    }

    m_State = State::Searching;
    return false;
  }

  if (m_State == State::Searching)
  {
    const int iMaxIterations = 32;
    int iIterationsDone = 0;
    dtStatus res = m_Query.updateSlicedFindPath(iMaxIterations, &iIterationsDone);

    if (dtStatusInProgress(res))
    {
      // still searching
      return false;
    }

    if (dtStatusFailed(res))
    {
      m_State = State::NoPathFound;
      return false;
    }

    WInt32 iPathCorridorLength = 0;
    dtPolyRef resultPolys[MaxPathNodes];

    const dtStatus finalizeStatus = m_Query.finalizeSlicedFindPath(resultPolys, &iPathCorridorLength, (int)MaxPathNodes);
    if (dtStatusFailed(finalizeStatus))
    {
      m_State = State::NoPathFound;
      W_REPORT_FAILURE("Detour: finalizeSlicedFindPath failed.");
      return false;
    }

    // reduce to actual length
    W_ASSERT_DEV(iPathCorridorLength >= 1, "Expected path corridor to have at least length 1");

    if (resultPolys[iPathCorridorLength - 1] != m_PathSearchTargetPoly)
    {
      // The target position cannot be reached with this corridor, but we can walk close to it. There are two
      // very different reasons for a partial result, and they must be told apart:
      // - DT_OUT_OF_NODES / DT_BUFFER_TOO_SMALL: the A* search ran out of budget before finishing. The endpoint
      //   is just how far it got. Repathing from closer to the target (once we've travelled along this corridor)
      //   can complete it - see the auto-repath below.
      // - neither flag: the search fully explored the reachable area and the target simply isn't reachable.
      //   The endpoint is the genuinely closest reachable point and will not improve on a repath.
      const bool bBudgetLimited = dtStatusDetail(finalizeStatus, DT_OUT_OF_NODES) ||
                                  dtStatusDetail(finalizeStatus, DT_BUFFER_TOO_SMALL);
      m_State = bBudgetLimited ? State::PartialPathSearchLimited : State::PartialPathUnreachable;
    }
    else
    {
      m_State = State::FullPathFound;
    }

    // the target position here may already differ from the target position when the search was started
    // so we need to use m_vPathSearchTargetPos
    // the final target position will be updated in the next Update()
    m_PathCorridor.reset(resultPolys[0], WRcPos(m_vCurrentPosition));
    m_PathCorridor.setCorridor(WRcPos(m_vPathSearchTargetPos), resultPolys, (WUInt32)iPathCorridorLength);

    m_uiOptimizeTopologyCounter = 0;
    m_uiOptimizeVisibilityCounter = 0;

    if (m_uiOptimizeWhenFoundBit)
    {
      m_uiOptimizeWhenFoundBit = 0;
      OptimizeCurrentPath();
    }

    // Remember how long the corridor started out, so the auto-repath below can tell how much of it has been
    // used up. Capture after the optimize pass, which can change the corridor length. Only meaningful for a
    // budget-limited partial - a full path never repaths, an unreachable one must not.
    m_uiPartialCorridorInitialLength = (m_State == State::PartialPathSearchLimited) ? m_PathCorridor.getPathCount() : 0;
  }

  // Replan if path has become invalid due to navmesh modifications
  if (m_State == State::FullPathFound || m_State == State::PartialPathSearchLimited || m_State == State::PartialPathUnreachable)
  {
    constexpr WInt32 PathLookahead = 10;

    dtPolyRef currentPoly = m_PathCorridor.getFirstPoly();
    if (!m_Query.isValidPolyRef(currentPoly, m_pFilter) || !m_Query.isValidPolyRef(m_PathSearchTargetPoly, m_pFilter) || !m_PathCorridor.isValid(PathLookahead, &m_Query, m_pFilter))
    {
      CancelNavigation();
      m_State = State::StartNewSearch;
      return false;
    }
  }

  // Auto-repath a budget-limited partial path once enough of its corridor has been consumed. Triggering on
  // corridor usage rather than on physically reaching the corridor's end is important: the agent may brake,
  // overshoot or never touch that endpoint, so waiting for arrival can leave it following a stale partial path
  // forever. Starting a fresh search from further along (closer to the target) often lets it finish within the
  // node budget. This never fires for PartialPathUnreachable (the target really isn't reachable - retrying just
  // reproduces the same result) nor for FullPathFound. The progress guard prevents oscillation: we only repath
  // if we have gotten strictly closer to the target than at the previous repath, so a partial endpoint that
  // merely shifts sideways (e.g. along the rim of a chasm) cannot cause an endless back-and-forth.
  if (m_State == State::PartialPathSearchLimited)
  {
    const WUInt32 uiRemaining = m_PathCorridor.getPathCount();

    const bool bFewPolysLeft = uiRemaining <= c_uiRepathMinPolysRemaining;
    const bool bHalfConsumed = m_uiPartialCorridorInitialLength > 0 &&
                               uiRemaining * c_uiRepathConsumedFractionDivisor <= m_uiPartialCorridorInitialLength;

    const float fToTarget = m_vCurrentPosition.GetDistanceTo(m_vTargetPosition);

    if ((bFewPolysLeft || bHalfConsumed) && fToTarget < m_fLastRepathStartDistToTarget - c_fRepathMinProgress)
    {
      // CancelNavigation() first, then record the guard value - CancelNavigation() must not clobber it.
      CancelNavigation();
      m_fLastRepathStartDistToTarget = fToTarget;
      m_State = State::StartNewSearch;
      return false;
    }
  }

  return true;
}

void WAiNavigation::DebugDrawPathCorridor(const WDebugRendererContext& context, WColor tilesColor, float fPolyRenderOffsetZ)
{
  const WUInt32 uiCorrLen = m_PathCorridor.getPathCount();
  const dtPolyRef* pCorrArr = m_PathCorridor.getPath();

  WTempHybridArray<WDebugRendererTriangle, 64> tris;

  const auto pNavmesh = m_Query.getAttachedNavMesh();

  for (WUInt32 c = 0; c < uiCorrLen; ++c)
  {
    dtPolyRef poly = pCorrArr[c];

    const dtMeshTile* pTile;
    const dtPoly* pPoly;
    pNavmesh->getTileAndPolyByRef(poly, &pTile, &pPoly);

    for (WUInt32 i = 2; i < pPoly->vertCount; ++i)
    {
      WRcPos rcPos[3];
      rcPos[0] = &(pTile->verts[pPoly->verts[0] * 3]);
      rcPos[1] = &(pTile->verts[pPoly->verts[i - 1] * 3]);
      rcPos[2] = &(pTile->verts[pPoly->verts[i] * 3]);

      auto& tri = tris.ExpandAndGetRef();
      tri.m_position[0] = WVec3(rcPos[0]);
      tri.m_position[2] = WVec3(rcPos[1]);
      tri.m_position[1] = WVec3(rcPos[2]);

      tri.m_position[0].z += fPolyRenderOffsetZ;
      tri.m_position[1].z += fPolyRenderOffsetZ;
      tri.m_position[2].z += fPolyRenderOffsetZ;
    }
  }

  WDebugRenderer::DrawSolidTriangles(context, tris, tilesColor);
}

void WAiNavigation::DebugDrawPathLine(const WDebugRendererContext& context, WColor straightLineColor, float fLineRenderOffsetZ)
{
  WTempHybridArray<WDebugRendererLine, 64> lines;
  WTempHybridArray<WVec3, 64> waypoints;
  ComputeAllWaypoints(waypoints);

  if (!waypoints.IsEmpty())
  {
    WVec3 vStart = m_vCurrentPosition;
    vStart.z += fLineRenderOffsetZ;

    for (WUInt32 i = 0; i < waypoints.GetCount(); ++i)
    {
      WVec3 vthis = waypoints[i];
      vthis.z += fLineRenderOffsetZ;

      auto& line = lines.ExpandAndGetRef();
      line.m_start = vStart;
      line.m_end = vthis;
      vStart = vthis;
    }

    WDebugRenderer::DrawLinesOccluded(context, lines, straightLineColor.GetDarker());
    WDebugRenderer::DrawLines(context, lines, straightLineColor);
  }
}

float WAiNavigation::GetCurrentElevation() const
{
  if (m_PathCorridor.getPathCount() > 0)
  {
    float h = m_vCurrentPosition.z;
    m_Query.getPolyHeight(m_PathCorridor.getFirstPoly(), m_PathCorridor.getPos(), &h);
    return h;
  }

  return m_vCurrentPosition.z;
}

void WAiNavigation::ComputeSteeringInfo(WAiSteeringInfo& out_info, const WVec2& vForwardDir, float fMaxLookAhead)
{
  out_info.m_vNextWaypoint = m_vCurrentPosition;

  if (m_PathCorridor.getPathCount() <= 0)
    return;

  static constexpr WUInt32 MaxTempNodes = 8;

  WUInt8 cornerFlags[MaxTempNodes];
  dtPolyRef cornerPolys[MaxTempNodes];
  WRcPos straightPath[MaxTempNodes];

  const bool bOptimize = m_uiOptimizeVisibilityCounter++ > 30;

  const int straightLen = m_PathCorridor.findCorners(straightPath[0], cornerFlags, cornerPolys, MaxTempNodes, &m_Query);

  if (straightLen > 0 && bOptimize)
  {
    m_uiOptimizeVisibilityCounter = 0;

    m_PathCorridor.optimizePathVisibility(straightPath[straightLen - 1], 10.0f, &m_Query, m_pFilter);
  }

  bool bFoundWaypoint = false;
  float fDistToPt = 0;

  out_info.m_fDistanceToWaypoint = 0;
  out_info.m_fArrivalDistance = WMath::HighValue<float>();
  out_info.m_vNextWaypoint = m_vCurrentPosition;
  out_info.m_vDirectionTowardsWaypoint = vForwardDir;
  out_info.m_AbsRotationTowardsWaypoint = WAngle::MakeZero();
  out_info.m_MaxAbsRotationAfterWaypoint = WAngle::MakeZero();
  // out_info.m_fWaypointCorridorWidth = WMath::HighValue<float>();

  WVec3 vPrevPos = m_vCurrentPosition;

  for (int idx = 0; idx < straightLen; ++idx)
  {
    fDistToPt += (WVec3(straightPath[idx]) - vPrevPos).GetLength();
    vPrevPos = straightPath[idx];

    if (cornerFlags[idx] & dtStraightPathFlags::DT_STRAIGHTPATH_END)
    {
      out_info.m_fArrivalDistance = fDistToPt;
    }

    if (!bFoundWaypoint && ((cornerFlags[idx] & dtStraightPathFlags::DT_STRAIGHTPATH_START) == 0))
    {
      bFoundWaypoint = true;
      out_info.m_vNextWaypoint = straightPath[idx];
      out_info.m_fDistanceToWaypoint = fDistToPt;
      out_info.m_vDirectionTowardsWaypoint = out_info.m_vNextWaypoint.GetAsVec2() - m_vCurrentPosition.GetAsVec2();
      out_info.m_vDirectionTowardsWaypoint.NormalizeIfNotZero(vForwardDir).IgnoreResult();

      out_info.m_AbsRotationTowardsWaypoint = out_info.m_vDirectionTowardsWaypoint.GetAngleBetween(vForwardDir);

      continue;
    }

    if (bFoundWaypoint && fDistToPt < fMaxLookAhead)
    {
      const WVec3 vNextPt = straightPath[idx];
      WVec2 vNextDir = (vNextPt - out_info.m_vNextWaypoint).GetAsVec2();
      if (vNextDir.NormalizeIfNotZero(WVec2::MakeZero()).Succeeded())
      {
        WAngle absDir = vNextDir.GetAngleBetween(out_info.m_vDirectionTowardsWaypoint);
        out_info.m_MaxAbsRotationAfterWaypoint = WMath::Max(absDir, out_info.m_MaxAbsRotationAfterWaypoint);
      }
    }
  }
}

void WAiNavigation::DebugDrawState(const WDebugRendererContext& context, const WVec3& vPosition) const
{
  switch (m_State)
  {
    case WAiNavigation::State::Idle:
      WDebugRenderer::Draw3DText(context, "Idle", vPosition, WColor::Grey);
      break;
    case WAiNavigation::State::StartNewSearch:
      WDebugRenderer::Draw3DText(context, "Starting Search...", vPosition, WColor::Yellow);
      break;
    case WAiNavigation::State::InvalidCurrentPosition:
      WDebugRenderer::Draw3DText(context, "Invalid Start Position", vPosition, WColor::Black);
      break;
    case WAiNavigation::State::InvalidTargetPosition:
      WDebugRenderer::Draw3DText(context, "Invalid Target Position", vPosition, WColor::IndianRed);
      break;
    case WAiNavigation::State::NoPathFound:
      WDebugRenderer::Draw3DText(context, "No Path Found", vPosition, WColor::White);
      break;
    case WAiNavigation::State::PartialPathSearchLimited:
      WDebugRenderer::Draw3DText(context, "Partial Path (search limited)", vPosition, WColor::Turquoise);
      break;
    case WAiNavigation::State::PartialPathUnreachable:
      WDebugRenderer::Draw3DText(context, "Partial Path (target unreachable)", vPosition, WColor::Orange);
      break;
    case WAiNavigation::State::FullPathFound:
      WDebugRenderer::Draw3DText(context, "Full Path Found", vPosition, WColor::LawnGreen);
      break;
    case WAiNavigation::State::Searching:
      WDebugRenderer::Draw3DText(context, "Searching...", vPosition, WColor::Yellow);
      break;
  }
}
