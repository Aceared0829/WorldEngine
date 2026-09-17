#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/Physics/Breakable2D.h>

#define JC_VORONOI_IMPLEMENTATION
#include <GameEngine/ThirdParty/jc_voronoi.h>

WBreakable2D::WBreakable2D() = default;
WBreakable2D::~WBreakable2D() = default;

void WBreakable2D::Clear()
{
  m_Shards.Clear();
  m_fMaxRadius = 0.0f;
}

void WBreakable2D::Initialize()
{
  Clear();

  m_Shards.SetCount(1);
  m_Shards[0].m_vCenterPosition.SetZero();
  m_Shards[0].m_fBoundingRadius = 5.0f;
  m_Shards[0].m_uiBreakablePatterns = (WUInt8)WBreakablePattern::All;
}

void WBreakable2D::RemoveShard(WUInt32 uiShardIdx)
{
  auto& shard = m_Shards[uiShardIdx];
  shard.m_bShattered = true;
}

void WBreakable2D::ShatterShard(WUInt32 uiShardIdx, const WVec2& vShatterPosition, WRandom& ref_rng, float fImpactRadius, float fCellSize, WUInt8 uiAllowedBreakPatterns)
{
  W_PROFILE_SCOPE("ShatterShard");

  if (m_Shards[uiShardIdx].m_bShattered)
    return;

  m_Shards[uiShardIdx].m_bShattered = true;

  if (m_Shards[uiShardIdx].m_bDynamic)
  {
    // don't further shatter shards that are already dynamic
    // just remove them
    return;
  }

  uiAllowedBreakPatterns &= m_Shards[uiShardIdx].m_uiBreakablePatterns;
  if (uiAllowedBreakPatterns == (WUInt8)WBreakablePattern::None)
    return;

  WTempHybridArray<ClipPlane, 6> clipPlanes;
  {
    const WVec3 vNormal = WVec3::MakeAxisZ();
    const auto& shard = m_Shards[uiShardIdx];

    WUInt32 uiPrevIdx = shard.m_Edges.GetCount() - 1;

    for (WUInt32 i = 0; i < shard.m_Edges.GetCount(); ++i)
    {
      ClipPlane& cp = clipPlanes.ExpandAndGetRef();
      if (cp.m_Plane.SetFromPoints(shard.m_Edges[i].m_vStartPosition.GetAsVec3(0), shard.m_Edges[uiPrevIdx].m_vStartPosition.GetAsVec3(0), shard.m_Edges[i].m_vStartPosition.GetAsVec3(0) + vNormal).Failed())
      {
        clipPlanes.PopBack();
        continue;
      }

      cp.m_uiOutsideShardIdx = shard.m_Edges[uiPrevIdx].m_uiOutsideShardIdx;
      uiPrevIdx = i;
    }
  }

  const WUInt32 uiPrevShardCount = m_Shards.GetCount();

  if ((uiAllowedBreakPatterns & (WUInt8)WBreakablePattern::Radial) != 0)
  {
    ShatterWithRadialPattern(clipPlanes, vShatterPosition, ref_rng, fImpactRadius);
  }
  else if ((uiAllowedBreakPatterns & (WUInt8)WBreakablePattern::Cellular) != 0)
  {
    ShatterWithCellularPattern(uiShardIdx, clipPlanes, ref_rng, fCellSize);
  }
  else
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }

  if (uiPrevShardCount == m_Shards.GetCount())
  {
    // too small for shatter size -> keep it, mark it as not-shatterable
    m_Shards[uiShardIdx].m_bShattered = false;
    m_Shards[uiShardIdx].m_uiBreakablePatterns = 0;
  }
}

void WBreakable2D::ShatterAll(float fShardSize, WRandom& ref_rng, bool bMakeAllDynamic)
{
  const WUInt32 uiNumShards = m_Shards.GetCount();

  for (WUInt32 i = 0; i < uiNumShards; ++i)
  {
    auto& shard = m_Shards[i];
    if (shard.m_bShattered || shard.m_bDynamic)
      continue;

    if ((shard.m_uiBreakablePatterns & (WUInt8)WBreakablePattern::Cellular) == 0)
      continue;

    ShatterShard(i, WVec2::MakeZero(), ref_rng, 0.0f, fShardSize, (WUInt8)WBreakablePattern::Cellular);
  }

  if (bMakeAllDynamic)
  {
    for (WUInt32 i = 0; i < m_Shards.GetCount(); ++i)
    {
      m_Shards[i].m_bDynamic = true;
    }
  }
}

void WBreakable2D::ShatterWithRadialPattern(WArrayPtr<const ClipPlane> clipPlanes, const WVec2& vShatterPosition, WRandom& ref_rng, float fImpactRadius)
{
  const float fMinAngle = 15.0f;
  const float fMaxAngle = 30.0f;

  WTempHybridArray<WAngle, 32> angles;
  float fRemainingAngle = 360.0f;

  while (fRemainingAngle > fMaxAngle)
  {
    const float fAngle = ref_rng.FloatMinMax(fMinAngle, fMaxAngle);
    fRemainingAngle -= fAngle;
    angles.PushBack(WAngle::MakeFromDegree(fAngle));
  }
  angles.PushBack(WAngle::MakeFromDegree(fRemainingAngle));

  const WUInt32 uiRingDetail = angles.GetCount();

  WTempHybridArray<WQuat, 32> qRots;
  WTempHybridArray<float, 32> radii1;
  WTempHybridArray<float, 32> radii2;
  WTempHybridArray<float, 32> radii3;
  WTempHybridArray<float, 32> radii4;
  WTempHybridArray<float, 32> radii5;
  WTempHybridArray<float, 32> radii6;
  WTempHybridArray<WVec2, 16> ring0, ring1;
  WTempHybridArray<WUInt32, 16> shardIDs0, shardIDs1;

  radii1.Reserve(uiRingDetail);
  radii2.Reserve(uiRingDetail);
  radii3.Reserve(uiRingDetail);
  radii4.Reserve(uiRingDetail);
  radii5.Reserve(uiRingDetail);
  radii6.Reserve(uiRingDetail);

  for (WUInt32 i = 0; i < uiRingDetail; ++i)
  {
    const float r1i = 1.0f;
    const float r1o = r1i * 1.5f;

    const float r2i = r1i * 2.5f;
    const float r2o = r2i * 1.5f;

    const float r3i = r2i * 2.5f;
    const float r3o = r3i * 1.5f;

    const float r4i = r3i * 2.5f;
    const float r4o = r4i * 1.5f;

    const float r5i = r4i * 2.5f;
    const float r5o = r5i * 1.5f;

    qRots.PushBack(WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisZ(), angles[i]));
    radii1.PushBack(ref_rng.FloatMinMax(r1i * fImpactRadius, r1o * fImpactRadius));
    radii2.PushBack(ref_rng.FloatMinMax(r2i * fImpactRadius, r2o * fImpactRadius));
    radii3.PushBack(ref_rng.FloatMinMax(r3i * fImpactRadius, r3o * fImpactRadius));
    radii4.PushBack(ref_rng.FloatMinMax(r4i * fImpactRadius, r4o * fImpactRadius));
    radii5.PushBack(ref_rng.FloatMinMax(r5i * fImpactRadius, r5o * fImpactRadius));
    radii6.PushBack(100.0f);
    shardIDs0.PushBack(WInvalidIndex);
  }

  GenerateRingVertices(ring1, vShatterPosition, radii6, qRots);

  GenerateRingVertices(ring0, vShatterPosition, radii5, qRots);
  GenerateRingShards(clipPlanes, ring0, ring1, shardIDs0, shardIDs1);

  GenerateRingVertices(ring1, vShatterPosition, radii4, qRots);
  GenerateRingShards(clipPlanes, ring1, ring0, shardIDs1, shardIDs0);

  GenerateRingVertices(ring0, vShatterPosition, radii3, qRots);
  GenerateRingShards(clipPlanes, ring0, ring1, shardIDs0, shardIDs1);

  GenerateRingVertices(ring1, vShatterPosition, radii2, qRots);
  GenerateRingShards(clipPlanes, ring1, ring0, shardIDs1, shardIDs0);

  GenerateRingVertices(ring0, vShatterPosition, radii1, qRots);
  GenerateRingShards(clipPlanes, ring0, ring1, shardIDs0, shardIDs1);
}

void WBreakable2D::GenerateRingVertices(WDynamicArray<WVec2>& vertices, const WVec2& vCenter, WArrayPtr<float> radii, const WArrayPtr<WQuat> qRotations)
{
  vertices.Clear();

  WVec2 vSide(1, 0);

  const WUInt32 uiRingDetail = radii.GetCount();

  for (WUInt32 i = 0; i < uiRingDetail; ++i)
  {
    const float fRadius = radii[i];

    const WVec2 vPos1 = vCenter + vSide * fRadius;
    vSide = (qRotations[i] * vSide.GetAsVec3(0)).GetAsVec2();
    const WVec2 vPos2 = vCenter + vSide * fRadius;

    vertices.PushBack(vPos1);
    vertices.PushBack(vPos2);
  }
}

void WBreakable2D::GenerateRingShards(WArrayPtr<const ClipPlane> clipPlanes, WArrayPtr<WVec2> innerVertices, WArrayPtr<WVec2> outerVertices, WArrayPtr<WUInt32> prevShardIDs, WDynamicArray<WUInt32>& out_ShardIDs)
{
  out_ShardIDs.Clear();

  WTempHybridArray<WBreakableShard2D::Edge, 6> shape;

  const WUInt32 uiRingDetail = innerVertices.GetCount() / 2;

  for (WUInt32 i = 0; i < uiRingDetail; ++i)
  {
    shape.Clear();

    auto& e0 = shape.ExpandAndGetRef();
    e0.m_vStartPosition = innerVertices[i * 2];
    e0.m_uiOutsideShardIdx = WInvalidIndex; // not connected to the inner ring

    auto& e1 = shape.ExpandAndGetRef();
    e1.m_vStartPosition = innerVertices[i * 2 + 1];
    e1.m_uiOutsideShardIdx = WInvalidIndex; // would need to be connected to the next shard, but we don't know whether that even will exist

    auto& e2 = shape.ExpandAndGetRef();
    e2.m_vStartPosition = outerVertices[i * 2 + 1];
    e2.m_uiOutsideShardIdx = prevShardIDs[i]; // connected to the outer ring

    auto& e3 = shape.ExpandAndGetRef();
    e3.m_vStartPosition = outerVertices[i * 2];
    e3.m_uiOutsideShardIdx = (i > 0) ? out_ShardIDs.PeekBack() : WInvalidIndex; // connected to the previous shard

    out_ShardIDs.PushBack(AddShard(clipPlanes, shape));
  }
}

WUInt32 WBreakable2D::AddShard(WArrayPtr<const ClipPlane> clipPlanes, WArrayPtr<WBreakableShard2D::Edge> shape)
{
  WTempHybridArray<WBreakableShard2D::Edge, 16> input(shape);
  WTempHybridArray<WBreakableShard2D::Edge, 16> output;

  for (WUInt32 pIdx = 0; pIdx < clipPlanes.GetCount(); ++pIdx)
  {
    if (input.GetCount() < 3)
      return WInvalidIndex;

    const WPlane& clipPlane = clipPlanes[pIdx].m_Plane;

    output.Clear();

    const WUInt32 uiNumVtx = input.GetCount();
    WUInt32 uiPrevVtx = uiNumVtx - 1;

    for (WUInt32 uiCurVtx = 0; uiCurVtx < uiNumVtx; ++uiCurVtx)
    {
      const auto& edge1 = input[uiPrevVtx];
      const auto& edge2 = input[uiCurVtx];
      const WVec3 v1 = edge1.m_vStartPosition.GetAsVec3(0);
      const WVec3 v2 = edge2.m_vStartPosition.GetAsVec3(0);

      uiPrevVtx = uiCurVtx;

      const bool bInside1 = clipPlane.GetPointPosition(v1) == WPositionOnPlane::Back;
      const bool bInside2 = clipPlane.GetPointPosition(v2) == WPositionOnPlane::Back;

      if (bInside1)
      {
        {
          auto& res = output.ExpandAndGetRef();
          res.m_vStartPosition = v1.GetAsVec2();
          res.m_uiOutsideShardIdx = edge1.m_uiOutsideShardIdx;
        }

        if (!bInside2)
        {
          WVec3 vIntersection;
          if (clipPlane.GetRayIntersectionBiDirectional(v1, v2 - v1, nullptr, &vIntersection))
          {
            auto& res = output.ExpandAndGetRef();
            res.m_vStartPosition = vIntersection.GetAsVec2();
            res.m_uiOutsideShardIdx = clipPlanes[pIdx].m_uiOutsideShardIdx;
          }
        }
      }
      else
      {
        if (bInside2)
        {
          WVec3 vIntersection;
          if (clipPlane.GetRayIntersectionBiDirectional(v1, v2 - v1, nullptr, &vIntersection))
          {
            auto& res = output.ExpandAndGetRef();
            res.m_vStartPosition = vIntersection.GetAsVec2();
            res.m_uiOutsideShardIdx = edge1.m_uiOutsideShardIdx;
          }
        }
      }
    }

    input = output;
  }

  if (output.GetCount() > 12)
  {
    // clamp maximum detail
    output.SetCount(12);
  }

  if (output.GetCount() <= 2)
    return WInvalidIndex;

  auto& shard = m_Shards.ExpandAndGetRef();
  shard.m_Edges = output;

  WBoundingBox bbox = WBoundingBox::MakeInvalid();

  constexpr float fCellularLength = WMath::Square(0.35f); // any edge must be longer
  constexpr float fGlassMinLength = WMath::Square(0.2f);  // no edge must be shorter
  constexpr float fGlassMaxLength = WMath::Square(0.4f);  // any edge must be longer

  bool bAllowGlassMin = true;
  bool bAllowGlassMax = false;
  bool bAllowCellular = false;

  WUInt32 uiPrevEdge = shard.m_Edges.GetCount() - 1;
  for (WUInt32 e = 0; e < shard.m_Edges.GetCount(); ++e)
  {
    const auto& edge = shard.m_Edges[e];
    bbox.ExpandToInclude(edge.m_vStartPosition.GetAsVec3(0));

    const float fEdgeLengthSqr = (edge.m_vStartPosition - shard.m_Edges[uiPrevEdge].m_vStartPosition).GetLengthSquared();
    uiPrevEdge = e;

    if (fEdgeLengthSqr < fGlassMinLength)
      bAllowGlassMin = false;
    if (fEdgeLengthSqr > fGlassMaxLength)
      bAllowGlassMax = true;

    if (fEdgeLengthSqr > fCellularLength)
      bAllowCellular = true;
  }

  shard.m_uiBreakablePatterns = 0;
  if (bAllowGlassMin && bAllowGlassMax)
    shard.m_uiBreakablePatterns |= (WUInt8)WBreakablePattern::Radial;
  if (bAllowCellular)
    shard.m_uiBreakablePatterns |= (WUInt8)WBreakablePattern::Cellular;

  shard.m_vCenterPosition = bbox.GetCenter().GetAsVec2();
  shard.m_fBoundingRadius = (bbox.m_vMax - bbox.m_vMin).GetLength() * 0.5f;

  m_fMaxRadius = WMath::Max(m_fMaxRadius, shard.m_fBoundingRadius);

  return m_Shards.GetCount() - 1;
}

void WBreakable2D::RecalculateDymamic()
{
  m_fMaxRadius = 0.0f;

  for (WUInt32 i = 0; i < m_Shards.GetCount(); ++i)
  {
    auto& shard = m_Shards[i];
    if (shard.m_bShattered)
      continue;

    m_fMaxRadius = WMath::Max(m_fMaxRadius, shard.m_fBoundingRadius);

    if (shard.m_bDynamic)
      continue;

    bool bHasSupport = false;

    for (WUInt32 e = 0; e < shard.m_Edges.GetCount(); ++e)
    {
      const WUInt32 outIdx = shard.m_Edges[e].m_uiOutsideShardIdx;

      if (outIdx == WInvalidIndex)
        continue;

      if (outIdx == WBreakableShard2D::FixedEdge)
      {
        bHasSupport = true;
        break;
      }

      if (outIdx > i)
      {
        // only consider shards that we already updated before
        continue;
      }

      if (!m_Shards[outIdx].m_bShattered && !m_Shards[outIdx].m_bDynamic)
      {
        bHasSupport = true;
        break;
      }
    }

    if (!bHasSupport)
    {
      shard.m_bDynamic = true;
    }
  }
}

W_DEFINE_AS_POD_TYPE(jcv_point);

bool WBreakable2D::ShatterWithCellularPattern(WUInt32 uiShardIdx, WArrayPtr<const ClipPlane> clipPlanes, WRandom& ref_rng, float fShardSize)
{
  const WBreakableShard2D& shard = m_Shards[uiShardIdx];

  WBoundingBox box = WBoundingBox::MakeInvalid();
  for (const auto& edge : shard.m_Edges)
  {
    box.ExpandToInclude(edge.m_vStartPosition.GetAsVec3(0));
  }

  const WVec2 pointCenter = box.GetCenter().GetAsVec2();
  const WVec2 halfext = box.GetHalfExtents().GetAsVec2();
  const WVec2 pointBounds = halfext * 0.9f;
  const WVec2 pointStep = WVec2(WMath::Clamp(fShardSize, 0.2f, 2.0f));
  const WVec2 variation = pointStep * 0.7f;

  WTempHybridArray<jcv_point, 64> diagramPoints;

  for (float y = pointCenter.y - pointBounds.y; y < pointCenter.y + pointBounds.y; y += pointStep.y)
  {
    for (float x = pointCenter.x - pointBounds.x; x < pointCenter.x + pointBounds.x; x += pointStep.x)
    {
      const float cx = static_cast<float>(ref_rng.DoubleMinMax(x, x + variation.x));
      const float cy = static_cast<float>(ref_rng.DoubleMinMax(y, y + variation.y));
      diagramPoints.PushBack({cx, cy});
    }
  }

  // too small area?
  if (diagramPoints.GetCount() < 3)
    return false;

  jcv_rect boundingBox;
  boundingBox.min.x = box.m_vMin.x - 1;
  boundingBox.min.y = box.m_vMin.y - 1;
  boundingBox.max.x = box.m_vMax.x + 1;
  boundingBox.max.y = box.m_vMax.y + 1;

  jcv_diagram diagram;
  WMemoryUtils::ZeroFill(&diagram, 1);
  jcv_diagram_generate(diagramPoints.GetCount(), diagramPoints.GetData(), &boundingBox, nullptr, &diagram);

  if (diagram.numsites == 0)
    return false;

  WTempHybridArray<WBreakableShard2D::Edge, 12> shape, shapeInv;

  const jcv_site* sites = jcv_diagram_get_sites(&diagram);

  WMap<const jcv_site*, WUInt32> ptrToShard;

  for (int i = 0; i < diagram.numsites; ++i)
  {
    shape.Clear();

    const jcv_site* site = &sites[i];
    const jcv_graphedge* e = site->edges;

    while (e)
    {
      auto& outEdge = shape.ExpandAndGetRef();
      outEdge.m_vStartPosition.Set(e->pos[0].x, e->pos[0].y);

      auto pBuddy = e->neighbor;

      auto itBuddy = ptrToShard.Find(pBuddy);
      if (itBuddy.IsValid())
      {
        outEdge.m_uiOutsideShardIdx = itBuddy.Value();
      }

      e = e->next;
    }

    shapeInv.Clear();
    shapeInv.SetCount(shape.GetCount());
    WUInt32 i2 = shape.GetCount() - 1;
    for (WUInt32 i = 0; i < shape.GetCount(); ++i)
    {
      shapeInv[i2] = shape[i];
      --i2;
    }

    ptrToShard[site] = AddShard(clipPlanes, shapeInv);
  }

  return true;
}
