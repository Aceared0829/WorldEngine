#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation3D/VoxelGrid.h>
#include <AiPlugin/Navigation3D/VoxelNavigation.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Math.h>
#include <RendererCore/Debug/DebugRenderer.h>

WAiVoxelNavigation::WAiVoxelNavigation() = default;
WAiVoxelNavigation::~WAiVoxelNavigation() = default;

// How many voxels FindPathToTarget()/FindPathToExit() search around a blocked start coordinate
// before giving up. 2 voxels covers the common case of the immediate neighbor also being blocked
// (e.g. a tight corner), without letting recovery silently teleport the object very far.
static const WUInt32 s_uiStartRecoveryRadiusVoxels = 2;

bool WAiVoxelNavigation::FindNearbyValidCoord(const WVoxelGrid& grid, const WVec3I32& vCoord, WUInt32 uiMaxRadius, WVec3I32& out_vValidCoord)
{
  // Exhaustive search of the (2*uiMaxRadius+1)^3 cube around vCoord, keeping the closest valid,
  // non-solid coordinate found (squared voxel-offset distance is monotonic with physical distance
  // since a grid's voxels are uniform in size, so this doesn't need to convert to world space).
  // uiMaxRadius is expected to stay small (a handful of voxels) - this is meant for local recovery,
  // not long-range searches.
  const WInt32 iMaxRadius = (WInt32)uiMaxRadius;
  WInt32 iBestDistSqr = WMath::MaxValue<WInt32>();
  bool bFound = false;

  for (WInt32 dz = -iMaxRadius; dz <= iMaxRadius; ++dz)
  {
    for (WInt32 dy = -iMaxRadius; dy <= iMaxRadius; ++dy)
    {
      for (WInt32 dx = -iMaxRadius; dx <= iMaxRadius; ++dx)
      {
        if (dx == 0 && dy == 0 && dz == 0)
          continue;

        const WInt32 iDistSqr = dx * dx + dy * dy + dz * dz;
        if (iDistSqr >= iBestDistSqr)
          continue;

        const WVec3I32 vNeighbor = vCoord + WVec3I32(dx, dy, dz);
        if (!grid.IsCoordValid(vNeighbor) || grid.IsVoxelSet(vNeighbor))
          continue;

        iBestDistSqr = iDistSqr;
        out_vValidCoord = vNeighbor;
        bFound = true;
      }
    }
  }

  return bFound;
}

namespace
{
  struct AStarNode
  {
    W_DECLARE_POD_TYPE();

    float fGCost = WMath::MaxValue<float>();
    float fFCost = WMath::MaxValue<float>();
    WUInt32 uiParent = WInvalidIndex;
    bool bClosed = false;
  };

  W_FORCE_INLINE WUInt32 PackCoord(const WVec3I32& vCoord, WUInt32 uiDimX, WUInt32 uiDimY)
  {
    return (WUInt32)vCoord.z * uiDimX * uiDimY + (WUInt32)vCoord.y * uiDimX + (WUInt32)vCoord.x;
  }

  W_FORCE_INLINE WVec3I32 UnpackCoord(WUInt32 uiIndex, WUInt32 uiDimX, WUInt32 uiDimY)
  {
    const WInt32 z = (WInt32)(uiIndex / (uiDimX * uiDimY));
    const WUInt32 uiRemaining = uiIndex - (WUInt32)z * uiDimX * uiDimY;
    const WInt32 y = (WInt32)(uiRemaining / uiDimX);
    const WInt32 x = (WInt32)(uiRemaining % uiDimX);
    return WVec3I32(x, y, z);
  }

  W_FORCE_INLINE bool IsBoundaryCoord(const WVec3I32& vCoord, const WVec3U32& vDims)
  {
    return vCoord.x == 0 || vCoord.y == 0 || vCoord.z == 0 ||
           (WUInt32)vCoord.x == vDims.x - 1 || (WUInt32)vCoord.y == vDims.y - 1 || (WUInt32)vCoord.z == vDims.z - 1;
  }

  /// Returns the (unnormalized) outward-facing normal of the grid boundary at vCoord, i.e. the sum
  /// of the outward unit vectors of every face vCoord touches (nonzero only where IsBoundaryCoord is true).
  W_FORCE_INLINE WVec3 GetBoundaryOutwardNormal(const WVec3I32& vCoord, const WVec3U32& vDims)
  {
    WVec3 vNormal = WVec3::MakeZero();

    if (vCoord.x == 0)
      vNormal.x -= 1.0f;
    if ((WUInt32)vCoord.x == vDims.x - 1)
      vNormal.x += 1.0f;

    if (vCoord.y == 0)
      vNormal.y -= 1.0f;
    if ((WUInt32)vCoord.y == vDims.y - 1)
      vNormal.y += 1.0f;

    if (vCoord.z == 0)
      vNormal.z -= 1.0f;
    if ((WUInt32)vCoord.z == vDims.z - 1)
      vNormal.z += 1.0f;

    return vNormal;
  }

  /// Whether exiting the grid at vCoord (which must satisfy IsBoundaryCoord) actually leads away
  /// from the grid, towards vTargetCoord - i.e. leaving through this particular voxel doesn't just
  /// walk straight back into the same grid. vTargetCoord may lie outside the grid.
  bool IsUsableExit(const WVec3I32& vCoord, const WVec3U32& vDims, const WVec3I32& vTargetCoord)
  {
    const WVec3 vNormal = GetBoundaryOutwardNormal(vCoord, vDims);

    const WVec3 vToTarget(
      (float)(vTargetCoord.x - vCoord.x),
      (float)(vTargetCoord.y - vCoord.y),
      (float)(vTargetCoord.z - vCoord.z));

    if (vToTarget.IsZero(0.001f))
      return true; // no clear direction to check against, don't reject

    return vNormal.Dot(vToTarget) > 0.0f;
  }

  /// Grids are not supposed to overlap, but in case they do, reject exit voxels that fall inside a
  /// solid voxel of another grid - that world position wouldn't actually be reachable.
  bool IsCoordFreeInOtherGrids(const WVoxelGrid& grid, const WVec3I32& vCoord, WArrayPtr<const WVoxelGrid* const> otherGrids)
  {
    if (otherGrids.IsEmpty())
      return true;

    const WVec3 vWorldPos = grid.CoordToWorld(vCoord);

    for (const WVoxelGrid* pOther : otherGrids)
    {
      if (pOther == &grid)
        continue;

      if (!pOther->GetAABB().Contains(vWorldPos))
        continue;

      const WVec3I32 vOtherCoord = pOther->WorldToCoord(vWorldPos);
      if (pOther->IsCoordValid(vOtherCoord) && pOther->IsVoxelSet(vOtherCoord))
        return false;
    }

    return true;
  }

  // Admissible heuristic for 6-connected (face-neighbor only) movement: the minimum number of
  // axis-aligned unit steps needed, which is exactly the Manhattan distance.
  float ManhattanHeuristic(const WVec3I32& vA, const WVec3I32& vB)
  {
    return (float)(WMath::Abs(vA.x - vB.x) + WMath::Abs(vA.y - vB.y) + WMath::Abs(vA.z - vB.z));
  }

  // Minimal binary min-heap for A* open set
  struct OpenSetEntry
  {
    W_DECLARE_POD_TYPE();

    WUInt32 uiIndex;
    float fFCost;
  };

  void HeapPush(WDynamicArray<OpenSetEntry>& ref_heap, WUInt32 uiIndex, float fFCost)
  {
    OpenSetEntry entry;
    entry.uiIndex = uiIndex;
    entry.fFCost = fFCost;
    ref_heap.PushBack(entry);

    // Sift up
    WUInt32 i = ref_heap.GetCount() - 1;
    while (i > 0)
    {
      const WUInt32 uiParent = (i - 1) / 2;
      if (ref_heap[uiParent].fFCost > ref_heap[i].fFCost)
      {
        WMath::Swap(ref_heap[uiParent], ref_heap[i]);
        i = uiParent;
      }
      else
      {
        break;
      }
    }
  }

  OpenSetEntry HeapPop(WDynamicArray<OpenSetEntry>& ref_heap)
  {
    OpenSetEntry top = ref_heap[0];
    ref_heap[0] = ref_heap[ref_heap.GetCount() - 1];
    ref_heap.PopBack();

    // Sift down
    WUInt32 i = 0;
    const WUInt32 uiCount = ref_heap.GetCount();
    while (true)
    {
      WUInt32 uiSmallest = i;
      const WUInt32 uiLeft = 2 * i + 1;
      const WUInt32 uiRight = 2 * i + 2;

      if (uiLeft < uiCount && ref_heap[uiLeft].fFCost < ref_heap[uiSmallest].fFCost)
        uiSmallest = uiLeft;
      if (uiRight < uiCount && ref_heap[uiRight].fFCost < ref_heap[uiSmallest].fFCost)
        uiSmallest = uiRight;

      if (uiSmallest != i)
      {
        WMath::Swap(ref_heap[i], ref_heap[uiSmallest]);
        i = uiSmallest;
      }
      else
      {
        break;
      }
    }

    return top;
  }
} // namespace

// Shared A* core, used both to search for an exact target coordinate inside a grid, and to search
// for a way out of a grid (any voxel on its boundary), biased towards vTargetCoord.
//
// vStartCoord must be a valid, non-solid coordinate in grid. If bExitSearch is false, vTargetCoord
// must also be a valid, non-solid coordinate in grid and is searched for exactly. If bExitSearch is
// true, vTargetCoord is only used to bias the search heuristic and may lie outside grid; the search
// stops at the first voxel reached that lies on the boundary of grid.
static WAiVoxelNavigation::State RunGridAStar(const WVoxelGrid& grid, const WVec3I32& vStartCoord, const WVec3I32& vTargetCoord,
  bool bExitSearch, WArrayPtr<const WVoxelGrid* const> otherGrids, WUInt32 uiMaxIterations, WDynamicArray<WVec3>& out_waypoints)
{
  using State = WAiVoxelNavigation::State;

  const WUInt32 uiDimX = grid.GetDimensions().x;
  const WUInt32 uiDimY = grid.GetDimensions().y;
  const WVec3U32 vDims = grid.GetDimensions();

  const WUInt32 uiStartPacked = PackCoord(vStartCoord, uiDimX, uiDimY);
  const WUInt32 uiTargetPacked = PackCoord(vTargetCoord, uiDimX, uiDimY);

  WHashTable<WUInt32, AStarNode> nodes;
  WDynamicArray<OpenSetEntry> openSet;

  // Initialize start node
  {
    AStarNode startNode;
    startNode.fGCost = 0.0f;
    startNode.fFCost = ManhattanHeuristic(vStartCoord, vTargetCoord);
    startNode.uiParent = WInvalidIndex;
    nodes.Insert(uiStartPacked, startNode);
    HeapPush(openSet, uiStartPacked, startNode.fFCost);
  }

  // 6-connected (face) neighbor offsets. Diagonal (26-connected) movement was removed: it lets a
  // path cut across a corner between two solid voxels that only touch edge-to-edge or corner-to-
  // corner, squeezing through a gap that isn't actually open.
  struct Neighbor
  {
    WInt32 dx, dy, dz;
  };

  static const Neighbor neighbors[6] = {
    {-1, 0, 0},
    {1, 0, 0},
    {0, -1, 0},
    {0, 1, 0},
    {0, 0, -1},
    {0, 0, 1},
  };
  const WUInt32 uiNeighborCount = 6;

  WUInt32 uiIterations = 0;
  WUInt32 uiGoalPacked = WInvalidIndex;

  while (!openSet.IsEmpty() && uiIterations < uiMaxIterations)
  {
    ++uiIterations;

    const OpenSetEntry current = HeapPop(openSet);

    AStarNode* pCurrentNode = nullptr;
    nodes.TryGetValue(current.uiIndex, pCurrentNode);

    if (pCurrentNode == nullptr || pCurrentNode->bClosed)
      continue;

    pCurrentNode->bClosed = true;

    const WVec3I32 vCurrentCoord = UnpackCoord(current.uiIndex, uiDimX, uiDimY);

    const bool bIsGoal = bExitSearch
                           ? (IsBoundaryCoord(vCurrentCoord, vDims) && IsUsableExit(vCurrentCoord, vDims, vTargetCoord) &&
                               IsCoordFreeInOtherGrids(grid, vCurrentCoord, otherGrids))
                           : current.uiIndex == uiTargetPacked;

    if (bIsGoal)
    {
      uiGoalPacked = current.uiIndex;
      break;
    }

    const float fCurrentG = pCurrentNode->fGCost;

    for (WUInt32 n = 0; n < uiNeighborCount; ++n)
    {
      const WVec3I32 vNeighborCoord(
        vCurrentCoord.x + neighbors[n].dx,
        vCurrentCoord.y + neighbors[n].dy,
        vCurrentCoord.z + neighbors[n].dz);

      if (!grid.IsCoordValid(vNeighborCoord))
        continue;

      if (grid.IsVoxelSet(vNeighborCoord))
        continue;

      const WUInt32 uiNeighborPacked = PackCoord(vNeighborCoord, uiDimX, uiDimY);
      const float fTentativeG = fCurrentG + 1.0f;

      AStarNode* pNeighborNode = nullptr;
      if (!nodes.TryGetValue(uiNeighborPacked, pNeighborNode))
      {
        AStarNode newNode;
        newNode.fGCost = fTentativeG;
        newNode.fFCost = fTentativeG + ManhattanHeuristic(vNeighborCoord, vTargetCoord);
        newNode.uiParent = current.uiIndex;
        nodes.Insert(uiNeighborPacked, newNode);
        HeapPush(openSet, uiNeighborPacked, newNode.fFCost);
      }
      else if (!pNeighborNode->bClosed && fTentativeG < pNeighborNode->fGCost)
      {
        pNeighborNode->fGCost = fTentativeG;
        pNeighborNode->fFCost = fTentativeG + ManhattanHeuristic(vNeighborCoord, vTargetCoord);
        pNeighborNode->uiParent = current.uiIndex;
        // Re-insert into open set (lazy deletion handles stale entries)
        HeapPush(openSet, uiNeighborPacked, pNeighborNode->fFCost);
      }
    }
  }

  if (uiGoalPacked == WInvalidIndex)
    return State::NoPathFound;

  // Back-trace path
  WDynamicArray<WVec3> rawPath;
  WUInt32 uiCurrent = uiGoalPacked;

  while (uiCurrent != WInvalidIndex)
  {
    const WVec3I32 vCoord = UnpackCoord(uiCurrent, uiDimX, uiDimY);
    rawPath.PushBack(grid.CoordToWorld(vCoord));

    AStarNode* pNode = nullptr;
    if (nodes.TryGetValue(uiCurrent, pNode))
    {
      uiCurrent = pNode->uiParent;
    }
    else
    {
      break;
    }
  }

  // Reverse to get start-to-target order
  for (WUInt32 i = 0; i < rawPath.GetCount() / 2; ++i)
  {
    WMath::Swap(rawPath[i], rawPath[rawPath.GetCount() - 1 - i]);
  }

  out_waypoints = std::move(rawPath);
  return State::PathFound;
}

WAiVoxelNavigation::State WAiVoxelNavigation::FindPathToTarget(const WVoxelGrid& grid, const WVec3& vStart, const WVec3& vTarget,
  WUInt32 uiMaxIterations, WDynamicArray<WVec3>& out_waypoints) const
{
  out_waypoints.Clear();

  WVec3I32 vStartCoord = grid.WorldToCoord(vStart);
  const WVec3I32 vTargetCoord = grid.WorldToCoord(vTarget);

  if (!grid.IsCoordValid(vStartCoord) || grid.IsVoxelSet(vStartCoord))
  {
    // The exact start voxel is blocked or out of bounds - before giving up, try to recover by
    // stepping into a free voxel immediately next to it (e.g. the object got pushed into a voxel
    // that turned solid after it last pathed).
    WVec3I32 vRecoveredCoord;
    if (!FindNearbyValidCoord(grid, vStartCoord, s_uiStartRecoveryRadiusVoxels, vRecoveredCoord))
      return State::InvalidStartPosition;

    vStartCoord = vRecoveredCoord;
  }

  if (!grid.IsCoordValid(vTargetCoord) || grid.IsVoxelSet(vTargetCoord))
    return State::InvalidTargetPosition;

  const State result = RunGridAStar(grid, vStartCoord, vTargetCoord, false, WArrayPtr<const WVoxelGrid* const>(), uiMaxIterations, out_waypoints);

  if (result == State::PathFound)
  {
    SmoothPath(grid, out_waypoints);
  }

  return result;
}

WAiVoxelNavigation::State WAiVoxelNavigation::FindPathToExit(const WVoxelGrid& grid, const WVec3& vStart, const WVec3& vRealTarget,
  WArrayPtr<const WVoxelGrid* const> otherGrids, WUInt32 uiMaxIterations, WDynamicArray<WVec3>& out_waypoints) const
{
  out_waypoints.Clear();

  WVec3I32 vStartCoord = grid.WorldToCoord(vStart);
  const WVec3I32 vTargetCoord = grid.WorldToCoord(vRealTarget); // may lie outside grid, only used for the heuristic

  if (!grid.IsCoordValid(vStartCoord) || grid.IsVoxelSet(vStartCoord))
  {
    WVec3I32 vRecoveredCoord;
    if (!FindNearbyValidCoord(grid, vStartCoord, s_uiStartRecoveryRadiusVoxels, vRecoveredCoord))
      return State::InvalidStartPosition;

    vStartCoord = vRecoveredCoord;
  }

  const State result = RunGridAStar(grid, vStartCoord, vTargetCoord, true, otherGrids, uiMaxIterations, out_waypoints);

  if (result == State::PathFound)
  {
    SmoothPath(grid, out_waypoints);
  }

  return result;
}

WAiVoxelNavigation::State WAiVoxelNavigation::FindPath(const WVec3& vStart, const WVec3& vTarget, const WAiVoxelGridFinder& gridFinder,
  float fSearchMargin, WUInt32 uiMaxIterationsPerHop, WUInt32 uiMaxHops)
{
  m_Waypoints.Clear();
  m_SegmentInsideGrid.Clear();
  m_uiCurrentWaypoint = 0;
  m_Waypoints.PushBack(vStart);

  WVec3 vCurrent = vStart;

  for (WUInt32 uiHop = 0; uiHop < uiMaxHops; ++uiHop)
  {
    if ((vTarget - vCurrent).GetLengthSquared() < 0.0001f)
    {
      m_State = State::PathFound;
      return m_State;
    }

    WBoundingBox searchBox = WBoundingBox::MakeInvalid();
    searchBox.ExpandToInclude(vCurrent);
    searchBox.ExpandToInclude(vTarget);
    searchBox.Grow(WVec3(fSearchMargin));

    WDynamicArray<const WVoxelGrid*> grids;
    gridFinder(searchBox, grids);

    const WVoxelGrid* pGrid = nullptr;
    WVec3 vEntry = vCurrent;
    float fBestEnter = WMath::HighValue<float>();

    for (const WVoxelGrid* pCandidate : grids)
    {
      const WBoundingBox box = pCandidate->GetAABB();

      if (box.Contains(vCurrent))
      {
        pGrid = pCandidate;
        vEntry = vCurrent;
        break;
      }

      float fEnter;
      if (box.GetLineSegmentIntersection(vCurrent, vTarget, &fEnter) && fEnter > 0.0001f && fEnter < fBestEnter)
      {
        fBestEnter = fEnter;
        pGrid = pCandidate;

        // The intersection point lies exactly on the grid's boundary surface. Nudge it a tiny bit
        // further along the segment, into the grid's interior: a point exactly on the max-corner
        // face would otherwise floor to a voxel coordinate one past the last valid index (an
        // off-by-one at the boundary), making the grid falsely look unreachable.
        const WVec3 vTravelDir = (vTarget - vCurrent).GetNormalized();
        vEntry = WMath::Lerp(vCurrent, vTarget, fEnter) + vTravelDir * (pCandidate->GetVoxelSize() * 0.1f);
      }
    }

    if (pGrid == nullptr)
    {
      // Rest of the way is free space (not covered by any grid)
      m_Waypoints.PushBack(vTarget);
      m_SegmentInsideGrid.PushBack(false);
      m_State = State::PathFound;
      return m_State;
    }

    if (!vEntry.IsEqual(vCurrent, 0.0001f))
    {
      m_Waypoints.PushBack(vEntry);
      m_SegmentInsideGrid.PushBack(false);
    }

    const bool bTargetInside = pGrid->GetAABB().Contains(vTarget);

    WDynamicArray<WVec3> hopWaypoints;
    const State hopState = bTargetInside
                             ? FindPathToTarget(*pGrid, vEntry, vTarget, uiMaxIterationsPerHop, hopWaypoints)
                             : FindPathToExit(*pGrid, vEntry, vTarget, grids, uiMaxIterationsPerHop, hopWaypoints);

    if (hopState != State::PathFound)
    {
      m_State = hopState;
      return m_State;
    }

    for (WUInt32 i = 0; i < hopWaypoints.GetCount(); ++i)
    {
      if (i == 0 && !m_Waypoints.IsEmpty() && hopWaypoints[i].IsEqual(m_Waypoints.PeekBack(), 0.0001f))
        continue;

      m_Waypoints.PushBack(hopWaypoints[i]);
      m_SegmentInsideGrid.PushBack(true);
    }

    vCurrent = m_Waypoints.PeekBack();

    if (bTargetInside)
    {
      m_State = State::PathFound;
      return m_State;
    }

    // vCurrent is the center of the boundary voxel the exit search stopped at, which is only up to
    // half a voxel away from the grid's true edge - not actually outside the grid yet. Push it out
    // along the boundary face normal of that voxel (not the direction from the grid's center through
    // the exit point - for non-cubic grids that vector is dominated by whichever axis the exit point
    // is farthest from center on, which usually isn't the axis of the face that was actually crossed).
    // A full voxel step guarantees clearing the boundary even when exiting through a corner or edge,
    // where the face normal is not axis-aligned.
    const WVec3I32 vExitCoord = pGrid->WorldToCoord(vCurrent);
    const WVec3 vOutward = GetBoundaryOutwardNormal(vExitCoord, pGrid->GetDimensions());
    if (!vOutward.IsZero(0.0001f))
    {
      vCurrent += vOutward.GetNormalized() * pGrid->GetVoxelSize();
    }
  }

  m_State = State::NoPathFound;
  return m_State;
}

void WAiVoxelNavigation::SmoothPath(const WVoxelGrid& grid, WDynamicArray<WVec3>& inout_waypoints) const
{
  if (inout_waypoints.GetCount() <= 2)
    return;

  WDynamicArray<WVec3> smoothed;
  smoothed.PushBack(inout_waypoints[0]);

  WUInt32 uiCurrent = 0;

  while (uiCurrent < inout_waypoints.GetCount() - 1)
  {
    WUInt32 uiFarthestVisible = uiCurrent + 1;

    for (WUInt32 i = inout_waypoints.GetCount() - 1; i > uiCurrent + 1; --i)
    {
      WVec3I32 vCoordA = grid.WorldToCoord(inout_waypoints[uiCurrent]);
      WVec3I32 vCoordB = grid.WorldToCoord(inout_waypoints[i]);

      if (grid.IsCoordValid(vCoordA) &&
          grid.IsCoordValid(vCoordB) &&
          grid.CheckLineOfSight(vCoordA, vCoordB))
      {
        uiFarthestVisible = i;
        break;
      }
    }

    smoothed.PushBack(inout_waypoints[uiFarthestVisible]);
    uiCurrent = uiFarthestVisible;
  }

  inout_waypoints = std::move(smoothed);
}

bool WAiVoxelNavigation::AdvanceWaypoint()
{
  if (m_uiCurrentWaypoint + 1 < m_Waypoints.GetCount())
  {
    ++m_uiCurrentWaypoint;
    return true;
  }
  return false;
}

WVec3 WAiVoxelNavigation::GetNextWaypoint() const
{
  if (m_Waypoints.IsEmpty())
    return WVec3::MakeZero();

  if (m_uiCurrentWaypoint < m_Waypoints.GetCount())
    return m_Waypoints[m_uiCurrentWaypoint];

  return m_Waypoints[m_Waypoints.GetCount() - 1];
}

bool WAiVoxelNavigation::IsPathComplete() const
{
  return m_Waypoints.IsEmpty() || m_uiCurrentWaypoint >= m_Waypoints.GetCount();
}

WVec3 WAiVoxelNavigation::WalkPathForward(const WVec3& vCurrentPos, float fDistance, WUInt32& inout_uiIndex) const
{
  WVec3 vFrom = vCurrentPos;
  WVec3 vTo = m_Waypoints[inout_uiIndex];
  float fRemaining = WMath::Max(fDistance, 0.0f);

  while (true)
  {
    const WVec3 vSeg = vTo - vFrom;
    const float fSegLen = vSeg.GetLength();

    if (fSegLen > 0.0001f)
    {
      if (fSegLen >= fRemaining)
        return vFrom + vSeg * (fRemaining / fSegLen);

      fRemaining -= fSegLen;
    }

    if (inout_uiIndex + 1 >= m_Waypoints.GetCount())
      return vTo;

    vFrom = vTo;
    ++inout_uiIndex;
    vTo = m_Waypoints[inout_uiIndex];
  }
}

WVec3 WAiVoxelNavigation::GetLookAheadPoint(const WVec3& vCurrentPos, float fLookAheadDistance) const
{
  if (IsPathComplete())
    return vCurrentPos;

  WUInt32 uiIndex = m_uiCurrentWaypoint; // local copy - peek only, does not advance the path
  return WalkPathForward(vCurrentPos, fLookAheadDistance, uiIndex);
}

WVec3 WAiVoxelNavigation::AdvanceAlongPath(const WVec3& vCurrentPos, float fDistance)
{
  if (IsPathComplete())
    return vCurrentPos;

  return WalkPathForward(vCurrentPos, fDistance, m_uiCurrentWaypoint); // mutates the current waypoint index directly
}

void WAiVoxelNavigation::SetDirectPath(const WVec3& vStart, const WVec3& vTarget)
{
  m_Waypoints.Clear();
  m_SegmentInsideGrid.Clear();
  m_uiCurrentWaypoint = 0;

  m_Waypoints.PushBack(vStart);
  m_Waypoints.PushBack(vTarget);
  m_SegmentInsideGrid.PushBack(false);

  m_State = State::PathFound;
}

void WAiVoxelNavigation::CancelNavigation()
{
  m_Waypoints.Clear();
  m_SegmentInsideGrid.Clear();
  m_uiCurrentWaypoint = 0;
  m_State = State::Idle;
}

void WAiVoxelNavigation::DebugDrawPath(const WDebugRendererContext& context, const WColor& color) const
{
  if (m_Waypoints.GetCount() < 2)
    return;

  WDynamicArray<WDebugRendererLine> lines;
  lines.Reserve(m_Waypoints.GetCount() - 1);

  for (WUInt32 i = 0; i + 1 < m_Waypoints.GetCount(); ++i)
  {
    auto& line = lines.ExpandAndGetRef();
    line.m_start = m_Waypoints[i];
    line.m_end = m_Waypoints[i + 1];

    if (i < m_uiCurrentWaypoint)
    {
      line.m_startColor = WColor::Grey;
      line.m_endColor = WColor::Grey;
    }
    else
    {
      line.m_startColor = color;
      line.m_endColor = color;
    }
  }

  WDebugRenderer::DrawLines(context, lines, color);
}

void WAiVoxelNavigation::DebugDrawPathSegments(const WDebugRendererContext& context, const WColor& insideGridColor,
  const WColor& freeSpaceColor) const
{
  if (m_Waypoints.GetCount() < 2)
    return;

  WDynamicArray<WDebugRendererLine> lines;
  lines.Reserve(m_Waypoints.GetCount() - 1);

  for (WUInt32 i = 0; i + 1 < m_Waypoints.GetCount(); ++i)
  {
    auto& line = lines.ExpandAndGetRef();
    line.m_start = m_Waypoints[i];
    line.m_end = m_Waypoints[i + 1];

    const WColor& color = m_SegmentInsideGrid[i] ? insideGridColor : freeSpaceColor;
    line.m_startColor = color;
    line.m_endColor = color;
  }

  WDebugRenderer::DrawLines(context, lines, WColor::White);
}
