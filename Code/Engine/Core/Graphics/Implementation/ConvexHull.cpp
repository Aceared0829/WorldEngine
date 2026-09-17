#include <Core/CorePCH.h>

#include <Core/Graphics/ConvexHull.h>
#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Plane.h>

WConvexHullGenerator::WConvexHullGenerator() = default;
WConvexHullGenerator::~WConvexHullGenerator() = default;

WResult WConvexHullGenerator::ComputeCenterAndScale(const WArrayPtr<const WVec3> vertices)
{
  if (vertices.IsEmpty())
    return W_FAILURE;

  WBoundingBox box = WBoundingBox::MakeFromPoints(vertices.GetPtr(), vertices.GetCount());

  const WVec3 c = box.GetCenter();
  m_vCenter.Set(c.x, c.y, c.z);

  const WVec3 ext = box.GetHalfExtents();

  const double minExt = WMath::Min(ext.x, ext.y, ext.z);

  if (minExt <= 0.000001)
    return W_FAILURE;

  const double maxExt = WMath::Max(ext.x, ext.y, ext.z);

  m_fScale = 1.0 / maxExt;

  return W_SUCCESS;
}

WResult WConvexHullGenerator::StoreNormalizedVertices(const WArrayPtr<const WVec3> vertices)
{
  struct Comparer
  {
    W_ALWAYS_INLINE bool Less(const WVec3d& a, const WVec3d& b) const
    {
      constexpr double eps = 0.01;

      if (a.x < b.x - eps)
        return true;
      if (a.x > b.x + eps)
        return false;

      if (a.y < b.y - eps)
        return true;
      if (a.y > b.y + eps)
        return false;

      return (a.z < b.z - eps);
    }
  };

  WSet<WVec3d, Comparer> used;

  m_Vertices.Clear();
  m_Vertices.Reserve(vertices.GetCount());

  for (WVec3 v : vertices)
  {
    WVec3d norm;
    norm.Set(v.x, v.y, v.z);

    // bring into [-1; +1] range for normalized precision
    norm -= m_vCenter;
    norm *= m_fScale;

    if (!used.Contains(norm))
    {
      m_Vertices.PushBack(norm);
      used.Insert(norm);
    }
  }

  if (m_Vertices.GetCount() < 4)
    return W_FAILURE;

  return W_SUCCESS;
}

void WConvexHullGenerator::StoreTriangle(WUInt16 i, WUInt16 j, WUInt16 k)
{
  Triangle& triangle = m_Triangles.ExpandAndGetRef();

  W_ASSERT_DEBUG((i < j) && (i < k) && (j < k), "Invalid Triangle");

  WVec3d tangent1 = m_Vertices[k] - m_Vertices[i];
  WVec3d tangent2 = m_Vertices[j] - m_Vertices[i];

  triangle.m_vNormal = tangent1.CrossRH(tangent2);
  triangle.m_bIsDegenerate = triangle.m_vNormal.IsZero(0.0000001);

  if (triangle.m_bIsDegenerate)
  {
    // triangle has degenerated to a line
    // use some made up normal to pretend it has some direction

    const WVec3d orth = m_Vertices[i] - m_vInside;
    tangent2 = tangent1.CrossRH(orth);
    triangle.m_vNormal = tangent1.CrossRH(tangent2);

    W_ASSERT_DEBUG(!triangle.m_vNormal.IsZero(0.0000001), "Normal is still invalid");
  }

  // needs to be normalized for later pruning
  triangle.m_vNormal.Normalize();
  triangle.m_fPlaneDistance = triangle.m_vNormal.Dot(m_Vertices[i]);

  triangle.m_uiVertexIdx[0] = i;
  triangle.m_uiVertexIdx[1] = j;
  triangle.m_uiVertexIdx[2] = k;

  const WUInt32 uiMaxVertices = m_Vertices.GetCount();

  m_Edges[i * uiMaxVertices + j].Add(k);
  m_Edges[i * uiMaxVertices + k].Add(j);
  m_Edges[j * uiMaxVertices + k].Add(i);

  triangle.m_bFlip = triangle.m_vNormal.Dot(m_vInside) > triangle.m_fPlaneDistance;

  if (triangle.m_bFlip)
  {
    triangle.m_vNormal = -triangle.m_vNormal;
    triangle.m_fPlaneDistance = -triangle.m_fPlaneDistance;
  }
}

WResult WConvexHullGenerator::InitializeHull()
{
  WVec3d minV = m_Vertices[0];
  WVec3d maxV = m_Vertices[0];
  WUInt32 minIx = 0;
  WUInt32 minIy = 0;
  WUInt32 minIz = 0;
  WUInt32 maxIx = 0;
  WUInt32 maxIy = 0;
  WUInt32 maxIz = 0;

  for (WUInt32 i = 0; i < m_Vertices.GetCount(); ++i)
  {
    const auto& v = m_Vertices[i];

    if (v.x < minV.x)
    {
      minV.x = v.x;
      minIx = i;
    }

    if (v.x > maxV.x)
    {
      maxV.x = v.x;
      maxIx = i;
    }

    if (v.y < minV.y)
    {
      minV.y = v.y;
      minIy = i;
    }

    if (v.y > maxV.y)
    {
      maxV.y = v.y;
      maxIy = i;
    }

    if (v.z < minV.z)
    {
      minV.z = v.z;
      minIz = i;
    }

    if (v.z > maxV.z)
    {
      maxV.z = v.z;
      maxIz = i;
    }
  }

  const WVec3d extents = maxV - minV;
  WUInt32 uiMainAxis1;
  WUInt32 uiMainAxis2;

  if (extents.x >= extents.y && extents.x >= extents.z)
  {
    uiMainAxis1 = minIx;
    uiMainAxis2 = maxIx;
  }
  else if (extents.y >= extents.x && extents.y >= extents.z)
  {
    uiMainAxis1 = minIy;
    uiMainAxis2 = maxIy;
  }
  else
  {
    uiMainAxis1 = minIz;
    uiMainAxis2 = maxIz;
  }

  if (uiMainAxis1 == uiMainAxis2)
    return W_FAILURE;

  WTempHybridArray<WUInt32, 6> testIdx;
  testIdx.PushBack(uiMainAxis1);
  testIdx.PushBack(uiMainAxis2);

  if (!testIdx.Contains(minIx))
    testIdx.PushBack(minIx);
  if (!testIdx.Contains(minIy))
    testIdx.PushBack(minIy);
  if (!testIdx.Contains(minIz))
    testIdx.PushBack(minIz);
  if (!testIdx.Contains(maxIx))
    testIdx.PushBack(maxIx);
  if (!testIdx.Contains(maxIy))
    testIdx.PushBack(maxIy);
  if (!testIdx.Contains(maxIz))
    testIdx.PushBack(maxIz);

  if (testIdx.GetCount() < 4)
  {
    // if we could not find enough vertices for the initial shape,
    // we will look at a couple more, even if those might not be the best candidates

    WUInt32 uiMaxVts = WMath::Min(50U, m_Vertices.GetCount());
    for (WUInt32 i = 0; i < uiMaxVts; ++i)
    {
      testIdx.PushBack(i);
    }
  }

  WVec3d planePoints[3];
  planePoints[0] = m_Vertices[testIdx[0]];
  planePoints[1] = m_Vertices[testIdx[1]];

  double maxDist = 0;
  WUInt32 uiIx1 = 0xFFFFFFFF, uiIx2 = 0xFFFFFFFF;

  for (WUInt32 i = 2; i < testIdx.GetCount(); ++i)
  {
    planePoints[2] = m_Vertices[testIdx[i]];

    WPlaned p;
    if (p.SetFromPoints(planePoints).Failed())
      continue;

    for (WUInt32 j = 2; j < testIdx.GetCount(); ++j)
    {
      if (i == j)
        continue;

      const double thisDist = WMath::Abs(p.GetDistanceTo(m_Vertices[testIdx[j]]));
      if (thisDist > maxDist)
      {
        maxDist = thisDist;
        uiIx1 = testIdx[i];
        uiIx2 = testIdx[j];
      }
    }
  }

  if (uiIx1 == 0xFFFFFFFF || uiIx2 == 0xFFFFFFFF)
    return W_FAILURE;

  // move the four chosen ones to the front of the queue
  testIdx.Clear();
  testIdx.PushBack(uiMainAxis1);
  testIdx.PushBack(uiMainAxis2);
  testIdx.PushBack(uiIx1);
  testIdx.PushBack(uiIx2);
  testIdx.Sort();

  for (int i = 0; i < 4; ++i)
  {
    if (i > 0)
    {
      W_ASSERT_DEBUG(testIdx[i - 1] != testIdx[i], "Same index used twice");
    }

    WMath::Swap(m_Vertices[i], m_Vertices[testIdx[i]]);
  }

  // precompute the 'inside' position
  {
    m_vInside.SetZero();
    for (WUInt32 v = 0; v < 4; ++v)
      m_vInside += m_Vertices[v];
    m_vInside /= 4.0;
  }

  // construct the hull as containing only the first four points
  for (WUInt16 i = 0; i < 4; i++)
  {
    for (WUInt16 j = i + 1; j < 4; j++)
    {
      for (WUInt16 k = j + 1; k < 4; k++)
      {
        StoreTriangle(i, j, k);
      }
    }
  }

  return W_SUCCESS;
}

WResult WConvexHullGenerator::ComputeHull()
{
  const WUInt32 uiMaxVertices = m_Vertices.GetCount();

  if (uiMaxVertices < 4)
    return W_FAILURE;

  m_Edges.Clear();
  m_Edges.SetCount(WMath::Square(uiMaxVertices));
  m_Triangles.Clear();
  m_Triangles.Reserve(512);

  W_SUCCEED_OR_RETURN(InitializeHull());

  // Add the points to the hull, one at a time.
  for (WUInt32 vtxId = 4; vtxId < uiMaxVertices; ++vtxId)
  {
    if (IsInside(vtxId))
      continue;

    // Find and delete all faces with their outside 'illuminated' by this point.
    RemoveVisibleFaces(vtxId);

    // Now for any edge still in the hull that is only part of one face
    // add another face containing the new point and that edge to the hull.
    PatchHole(vtxId);
  }

  if (m_Triangles.GetCount() < 4)
    return W_FAILURE;

  return W_SUCCESS;
}

bool WConvexHullGenerator::IsInside(WUInt32 vtxId) const
{
  const WVec3d pos = m_Vertices[vtxId];

  const WInt32 iNumTriangles = m_Triangles.GetCount();
  for (WInt32 j = 0; j < iNumTriangles; j++)
  {
    const auto& tri = m_Triangles[j];

    const double dist = tri.m_vNormal.Dot(pos);
    if (dist > tri.m_fPlaneDistance + 0.01)
      return false;
  }

  return true;
}

void WConvexHullGenerator::RemoveVisibleFaces(WUInt32 vtxId)
{
  const WUInt32 uiMaxVertices = m_Vertices.GetCount();
  const WVec3d pos = m_Vertices[vtxId];

  WInt32 iNumTriangles = m_Triangles.GetCount();
  for (WInt32 j = 0; j < iNumTriangles; j++)
  {
    const auto& tri = m_Triangles[j];

    const double dist = tri.m_vNormal.Dot(pos);
    if (dist <= tri.m_fPlaneDistance)
      continue;

    const WUInt16 vtx0 = tri.m_uiVertexIdx[0];
    const WUInt16 vtx1 = tri.m_uiVertexIdx[1];
    const WUInt16 vtx2 = tri.m_uiVertexIdx[2];

    m_Edges[vtx0 * uiMaxVertices + vtx1].Remove(vtx2);
    m_Edges[vtx0 * uiMaxVertices + vtx2].Remove(vtx1);
    m_Edges[vtx1 * uiMaxVertices + vtx2].Remove(vtx0);

    m_Triangles.RemoveAtAndSwap(j);

    --j;
    --iNumTriangles;
  }
}

void WConvexHullGenerator::PatchHole(WUInt32 vtxId)
{
  W_ASSERT_DEBUG(vtxId < 0xFFFFu, "Vertex Id is larger than 16 bits can address.");
  const WUInt32 uiMaxVertices = m_Vertices.GetCount();

  const WUInt32 uiNumFaces = m_Triangles.GetCount();
  for (WUInt32 j = 0; j < uiNumFaces; j++)
  {
    const auto& tri = m_Triangles[j];

    for (WInt32 a = 0; a < 3; a++)
    {
      for (WInt32 b = a + 1; b < 3; b++)
      {
        const WUInt16 vtxA = tri.m_uiVertexIdx[a];
        const WUInt16 vtxB = tri.m_uiVertexIdx[b];

        if (m_Edges[vtxA * uiMaxVertices + vtxB].GetSize() == 2)
          continue;

        StoreTriangle(vtxA, vtxB, static_cast<WUInt16>(vtxId));
      }
    }
  }
}

bool WConvexHullGenerator::PruneFlatVertices(double fNormalThreshold)
{
  struct VertexNormals
  {
    WVec3d m_vNormals[2];
    WInt32 m_iDifferentNormals = 0;
  };

  WDynamicArray<VertexNormals> VtxNorms;
  VtxNorms.SetCount(m_Vertices.GetCount());

  WUInt32 uiNumVerticesRemaining = 0;

  for (const auto& tri : m_Triangles)
  {
    if (tri.m_bIsDegenerate)
      continue;

    const WVec3d planeNorm = tri.m_vNormal;

    for (int v = 0; v < 3; ++v)
    {
      VertexNormals& norms = VtxNorms[tri.m_uiVertexIdx[v]];

      if (norms.m_iDifferentNormals > 2)
        continue;

      for (int d = 0; d < norms.m_iDifferentNormals; ++d)
      {

        if (norms.m_vNormals[d].Dot(planeNorm) > fNormalThreshold)
          goto same;
      }

      if (norms.m_iDifferentNormals == 2)
      {
        uiNumVerticesRemaining++;
        norms.m_iDifferentNormals = 3;
      }
      else
      {
        norms.m_vNormals[norms.m_iDifferentNormals] = planeNorm;
        ++norms.m_iDifferentNormals;
      }

    same:
      continue;
    }
  }

  if (uiNumVerticesRemaining < 4)
    return false;

  if (uiNumVerticesRemaining == m_Vertices.GetCount())
    return false;

  WDynamicArray<WVec3d> remaining;
  remaining.Reserve(uiNumVerticesRemaining);

  // now only keep the vertices that have at least 3 different normals
  for (WUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
  {
    if (VtxNorms[v].m_iDifferentNormals < 3)
      continue;

    remaining.PushBack(m_Vertices[v]);
  }

  m_Vertices = remaining;

  return true;
}


bool WConvexHullGenerator::PruneDegenerateTriangles(double fMaxCosAngle)
{
  bool bChanged = false;

  WDynamicBitfield discardVtx;
  discardVtx.SetCount(m_Vertices.GetCount(), false);

  for (const auto& tri : m_Triangles)
  {
    const WUInt32 idx0 = tri.m_uiVertexIdx[0];
    const WUInt32 idx1 = tri.m_uiVertexIdx[1];
    const WUInt32 idx2 = tri.m_uiVertexIdx[2];
    const WVec3d v0 = m_Vertices[idx0];
    const WVec3d v1 = m_Vertices[idx1];
    const WVec3d v2 = m_Vertices[idx2];
    const WVec3d e0 = (v1 - v0).GetNormalized();
    const WVec3d e1 = (v2 - v1).GetNormalized();
    const WVec3d e2 = (v0 - v2).GetNormalized();

    if (e0.Dot(e1) > fMaxCosAngle)
    {
      discardVtx.SetBit(idx1);
      bChanged = true;
    }

    if (e1.Dot(e2) > fMaxCosAngle)
    {
      discardVtx.SetBit(idx2);
      bChanged = true;
    }

    if (e2.Dot(e0) > fMaxCosAngle)
    {
      discardVtx.SetBit(idx0);
      bChanged = true;
    }
  }

  if (bChanged)
  {
    for (WUInt32 n = discardVtx.GetCount(); n > 0; --n)
    {
      if (discardVtx.IsBitSet(n - 1))
      {
        m_Vertices.RemoveAtAndSwap(n - 1);
      }
    }
  }

  return bChanged;
}

bool WConvexHullGenerator::PruneSmallTriangles(double fMaxEdgeLen)
{
  bool bChanged = false;

  WDynamicBitfield discardVtx;
  discardVtx.SetCount(m_Vertices.GetCount(), false);

  for (const auto& tri : m_Triangles)
  {
    if (tri.m_bIsDegenerate)
      continue;

    const WUInt32 idx0 = tri.m_uiVertexIdx[0];
    const WUInt32 idx1 = tri.m_uiVertexIdx[1];
    const WUInt32 idx2 = tri.m_uiVertexIdx[2];
    const WVec3d v0 = m_Vertices[idx0];
    const WVec3d v1 = m_Vertices[idx1];
    const WVec3d v2 = m_Vertices[idx2];
    const double len0 = (v1 - v0).GetLength();
    const double len1 = (v2 - v1).GetLength();
    const double len2 = (v0 - v2).GetLength();

    if (len0 < fMaxEdgeLen && len1 < fMaxEdgeLen && len2 < fMaxEdgeLen)
    {
      discardVtx.SetBit(idx0);
      discardVtx.SetBit(idx1);
      discardVtx.SetBit(idx2);

      const WVec3d center = (v0 + v1 + v2) / 3.0;
      m_Vertices.PushBack(center);

      bChanged = true;

      continue;
    }

    if (len0 < fMaxEdgeLen)
    {
      discardVtx.SetBit(idx0);
      discardVtx.SetBit(idx1);

      const WVec3d center = (v0 + v1) / 2.0;
      m_Vertices.PushBack(center);

      bChanged = true;
    }

    if (len1 < fMaxEdgeLen)
    {
      discardVtx.SetBit(idx2);
      discardVtx.SetBit(idx1);

      const WVec3d center = (v2 + v1) / 2.0;
      m_Vertices.PushBack(center);

      bChanged = true;
    }

    if (len2 < fMaxEdgeLen)
    {
      discardVtx.SetBit(idx0);
      discardVtx.SetBit(idx2);

      const WVec3d center = (v0 + v2) / 2.0;
      m_Vertices.PushBack(center);

      bChanged = true;
    }
  }

  if (bChanged)
  {
    for (WUInt32 n = discardVtx.GetCount(); n > 0; --n)
    {
      if (discardVtx.IsBitSet(n - 1))
      {
        m_Vertices.RemoveAtAndSwap(n - 1);
      }
    }
  }

  return bChanged;
}

WResult WConvexHullGenerator::ProcessVertices(const WArrayPtr<const WVec3> vertices)
{
  WUInt32 uiFirstVertex = 0;
  WUInt32 uiNumVerticesLeft = vertices.GetCount();
  const WUInt32 uiVerticesPerBatch = 1000;

  WDynamicArray<WVec3> workingSet;

  while (uiNumVerticesLeft > 0)
  {
    RetrieveVertices(workingSet);

    const WUInt32 uiAdd = WMath::Min(uiNumVerticesLeft, uiVerticesPerBatch);
    const WArrayPtr<const WVec3> range = vertices.GetSubArray(uiFirstVertex, uiAdd);
    workingSet.PushBackRange(range);

    uiFirstVertex += uiAdd;
    uiNumVerticesLeft -= uiAdd;

    W_SUCCEED_OR_RETURN(StoreNormalizedVertices(workingSet));

    if (m_Vertices.GetCount() >= 16384)
      return W_FAILURE;

    W_SUCCEED_OR_RETURN(ComputeHull());
  }

  if (m_Triangles.GetCount() < 4)
    return W_FAILURE;

  return W_SUCCESS;
}

WResult WConvexHullGenerator::Build(const WArrayPtr<const WVec3> vertices)
{
  m_Vertices.Clear();

  W_SUCCEED_OR_RETURN(ComputeCenterAndScale(vertices));

  W_SUCCEED_OR_RETURN(ProcessVertices(vertices));

  bool prune = true;
  while (prune)
  {
    prune = false;

    if (PruneDegenerateTriangles(WMath::Cos(m_MinTriangleAngle)))
    {
      W_SUCCEED_OR_RETURN(ComputeHull());
      prune = true;
    }

    if (PruneFlatVertices(WMath::Cos(m_FlatVertexNormalThreshold)))
    {
      W_SUCCEED_OR_RETURN(ComputeHull());
      prune = true;
    }

    if (PruneSmallTriangles(m_fMinTriangleEdgeLength))
    {
      W_SUCCEED_OR_RETURN(ComputeHull());
      prune = true;
    }
  }

  return W_SUCCESS;
}

void WConvexHullGenerator::Retrieve(WDynamicArray<WVec3>& out_vertices, WDynamicArray<Face>& out_faces)
{
  out_vertices.Clear();
  out_faces.Clear();

  out_vertices.Reserve(m_Triangles.GetCount() * 2);
  out_faces.Reserve(m_Triangles.GetCount());

  WMap<WUInt32, WUInt32> vtxMap;

  const double fScaleBack = 1.0 / m_fScale;

  for (const auto& tri : m_Triangles)
  {
    auto& face = out_faces.ExpandAndGetRef();

    for (int v = 0; v < 3; ++v)
    {
      const WUInt32 orgIdx = tri.m_uiVertexIdx[v];

      bool bExisted = false;
      auto it = vtxMap.FindOrAdd(orgIdx, &bExisted);
      if (!bExisted)
      {
        it.Value() = out_vertices.GetCount();

        const WVec3d pos = (m_Vertices[orgIdx] * fScaleBack) + m_vCenter;

        WVec3& vtx = out_vertices.ExpandAndGetRef();
        vtx.Set((float)pos.x, (float)pos.y, (float)pos.z);
      }

      face.m_uiVertexIdx[v] = static_cast<WUInt16>(it.Value());
    }

    if (tri.m_bFlip)
    {
      WMath::Swap(face.m_uiVertexIdx[1], face.m_uiVertexIdx[2]);
    }
  }
}

void WConvexHullGenerator::RetrieveVertices(WDynamicArray<WVec3>& out_vertices)
{
  out_vertices.Clear();
  out_vertices.Reserve(m_Triangles.GetCount() * 2);

  WMap<WUInt32, WUInt32> vtxMap;

  const double fScaleBack = 1.0 / m_fScale;

  for (const auto& tri : m_Triangles)
  {
    for (int v = 0; v < 3; ++v)
    {
      const WUInt32 orgIdx = tri.m_uiVertexIdx[v];

      bool bExisted = false;
      auto it = vtxMap.FindOrAdd(orgIdx, &bExisted);
      if (!bExisted)
      {
        it.Value() = out_vertices.GetCount();

        const WVec3d pos = (m_Vertices[orgIdx] * fScaleBack) + m_vCenter;

        WVec3& vtx = out_vertices.ExpandAndGetRef();
        vtx.Set((float)pos.x, (float)pos.y, (float)pos.z);
      }
    }
  }
}
