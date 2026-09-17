#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshQuery.h>

static constexpr WUInt32 MaxSearchNodes = 16;

WAiNavmeshQuery::WAiNavmeshQuery()
{
  m_uiReinitQueryBit = 1;
}

void WAiNavmeshQuery::SetNavmesh(WAiNavMesh* pNavmesh)
{
  if (m_pNavmesh == pNavmesh)
    return;

  m_pNavmesh = pNavmesh;
  m_uiReinitQueryBit = 1;
}

void WAiNavmeshQuery::SetQueryFilter(const dtQueryFilter& filter)
{
  if (m_pFilter == &filter)
    return;

  m_pFilter = &filter;
}

bool WAiNavmeshQuery::PrepareQueryArea(const WVec3& vCenter, float fRadius)
{
  W_ASSERT_DEV(m_pNavmesh != nullptr, "Navmesh has not been set.");
  return m_pNavmesh->RequestSector(vCenter.GetAsVec2(), WVec2(fRadius));
}

bool WAiNavmeshQuery::Raycast(const WVec3& vStart, const WVec3& vDir, float fDistance, WAiNavmeshRaycastHit& out_raycastHit)
{
  if (m_uiReinitQueryBit)
  {
    W_ASSERT_DEV(m_pNavmesh != nullptr, "Navmesh has not been set.");
    W_ASSERT_DEV(m_pFilter != nullptr, "Navmesh filter has not been set.");

    m_uiReinitQueryBit = 0;
    m_Query.init(m_pNavmesh->GetDetourNavMesh(), MaxSearchNodes);
  }

  // Recast is Y-up: index 1 is the vertical (W Z) extent, 0 and 2 are horizontal (W X/Y).
  float he[3] = {m_fSearchExtentsXY, m_fSearchExtentsZ, m_fSearchExtentsXY};

  dtPolyRef ref;
  float pt[3];

  if (dtStatusFailed(m_Query.findNearestPoly(WRcPos(vStart), he, m_pFilter, &ref, pt)))
    return false;

  if (ref == 0)
    return false;

  dtRaycastHit hit{};
  if (dtStatusFailed(m_Query.raycast(ref, WRcPos(vStart), WRcPos(vStart + vDir * fDistance), m_pFilter, 0, &hit)))
    return false;

  if (hit.t > 1.0f)
    return false;

  out_raycastHit.m_fHitDistanceNormalized = hit.t;
  out_raycastHit.m_fHitDistance = hit.t * fDistance;
  out_raycastHit.m_vHitPosition = vStart + (vDir * fDistance * hit.t);

  return true;
}

thread_local WRandom* tl_pRandom = nullptr;

static float frand()
{
  return tl_pRandom->FloatZeroToOneInclusive();
}

bool WAiNavmeshQuery::FindRandomPointAroundCircle(const WVec3& vStart, float fRadius, WRandom& ref_rng, WVec3& out_vPoint)
{
  if (m_uiReinitQueryBit)
  {
    W_ASSERT_DEV(m_pNavmesh != nullptr, "Navmesh has not been set.");
    W_ASSERT_DEV(m_pFilter != nullptr, "Navmesh filter has not been set.");

    m_uiReinitQueryBit = 0;
    m_Query.init(m_pNavmesh->GetDetourNavMesh(), MaxSearchNodes);
  }

  // Recast is Y-up: index 1 is the vertical (W Z) extent, 0 and 2 are horizontal (W X/Y).
  float he[3] = {m_fSearchExtentsXY, m_fSearchExtentsZ, m_fSearchExtentsXY};

  dtPolyRef ref;
  float pt[3];

  if (dtStatusFailed(m_Query.findNearestPoly(WRcPos(vStart), he, m_pFilter, &ref, pt)))
    return false;

  if (ref == 0)
    return false;

  tl_pRandom = &ref_rng;

  dtPolyRef resultRef;
  WRcPos resPt;
  if (dtStatusFailed(m_Query.findRandomPointAroundCircle(ref, WRcPos(vStart), fRadius, m_pFilter, frand, &resultRef, resPt)))
    return false;

  out_vPoint = resPt;
  return true;
}

bool WAiNavmeshQuery::FindClosestPointOnNavmesh(const WVec3& vPos, WVec3& out_vPoint, WVec3* out_pNormal)
{
  if (m_uiReinitQueryBit)
  {
    W_ASSERT_DEV(m_pNavmesh != nullptr, "Navmesh has not been set.");
    W_ASSERT_DEV(m_pFilter != nullptr, "Navmesh filter has not been set.");

    m_uiReinitQueryBit = 0;
    m_Query.init(m_pNavmesh->GetDetourNavMesh(), MaxSearchNodes);
  }

  // Recast is Y-up: index 1 is the vertical (W Z) extent, 0 and 2 are horizontal (W X/Y).
  float he[3] = {m_fSearchExtentsXY, m_fSearchExtentsZ, m_fSearchExtentsXY};

  dtPolyRef ref;
  WRcPos closestPt;

  if (dtStatusFailed(m_Query.findNearestPoly(WRcPos(vPos), he, m_pFilter, &ref, closestPt)))
    return false;

  if (ref == 0)
    return false;

  out_vPoint = closestPt;

  if (out_pNormal != nullptr)
  {
    // Query from the original (possibly off-mesh) position, not closestPt: closestPt sits exactly on the wall,
    // which would make hitDist ~0 and the normal (a near-zero vector, normalized) NaN.
    float hitDist, hitPos[3], hitNormal[3];
    if (dtStatusSucceed(m_Query.findDistanceToWall(ref, WRcPos(vPos), m_fSearchExtentsXY, m_pFilter, &hitDist, hitPos, hitNormal)) &&
        hitDist > 0.001f)
    {
      *out_pNormal = WRcPos(hitNormal);
    }
  }

  return true;
}
