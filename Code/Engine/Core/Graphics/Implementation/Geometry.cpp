#include <Core/CorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Quat.h>
#include <meshoptimizer/meshoptimizer.h>

bool WGeometry::GeoOptions::IsFlipWindingNecessary() const
{
  return m_Transform.GetRotationalPart().GetDeterminant() < 0;
}

bool WGeometry::Vertex::operator<(const WGeometry::Vertex& rhs) const
{
  if (m_vPosition != rhs.m_vPosition)
    return m_vPosition < rhs.m_vPosition;

  if (m_vNormal != rhs.m_vNormal)
    return m_vNormal < rhs.m_vNormal;

  if (m_vTangent != rhs.m_vTangent)
    return m_vTangent < rhs.m_vTangent;

  if (m_fBiTangentSign != rhs.m_fBiTangentSign)
    return m_fBiTangentSign < rhs.m_fBiTangentSign;

  if (m_vTexCoord != rhs.m_vTexCoord)
    return m_vTexCoord < rhs.m_vTexCoord;

  if (m_Color != rhs.m_Color)
    return m_Color < rhs.m_Color;

  if (m_BoneIndices != rhs.m_BoneIndices)
    return m_BoneIndices < rhs.m_BoneIndices;

  return m_BoneWeights < rhs.m_BoneWeights;
}

bool WGeometry::Vertex::operator==(const WGeometry::Vertex& rhs) const
{
  return m_vPosition == rhs.m_vPosition &&
         m_vNormal == rhs.m_vNormal &&
         m_vTangent == rhs.m_vTangent &&
         m_fBiTangentSign == rhs.m_fBiTangentSign &&
         m_vTexCoord == rhs.m_vTexCoord &&
         m_Color == rhs.m_Color &&
         m_BoneIndices == rhs.m_BoneIndices &&
         m_BoneWeights == rhs.m_BoneWeights;
}

void WGeometry::Polygon::FlipWinding()
{
  const WUInt32 uiCount = m_Vertices.GetCount();
  const WUInt32 uiHalfCount = uiCount / 2;
  for (WUInt32 i = 0; i < uiHalfCount; i++)
  {
    WMath::Swap(m_Vertices[i], m_Vertices[uiCount - i - 1]);
  }
}

void WGeometry::Clear()
{
  m_Vertices.Clear();
  m_Polygons.Clear();
  m_Lines.Clear();
}

WUInt32 WGeometry::AddVertex(const WVec3& vPos, const WVec3& vNormal, const WVec2& vTexCoord, const WColor& color, const WVec4U16& vBoneIndices /*= WVec4U16::MakeZero()*/, const WColorLinearUB& boneWeights /*= WColorLinearUB(255, 0, 0, 0)*/)
{
  Vertex& v = m_Vertices.ExpandAndGetRef();
  v.m_vPosition = vPos;
  v.m_vNormal = vNormal;
  v.m_vTangent.SetZero();
  v.m_fBiTangentSign = 0;
  v.m_vTexCoord = vTexCoord;
  v.m_Color = color;
  v.m_BoneIndices = vBoneIndices;
  v.m_BoneWeights = boneWeights;

  return m_Vertices.GetCount() - 1;
}

void WGeometry::AddPolygon(const WArrayPtr<WUInt32>& vertices, bool bFlipWinding)
{
  W_ASSERT_DEV(vertices.GetCount() >= 3, "Polygon must have at least 3 vertices, not {0}", vertices.GetCount());

  for (WUInt32 v = 0; v < vertices.GetCount(); ++v)
  {
    W_ASSERT_DEBUG(vertices[v] < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", vertices[v], m_Vertices.GetCount());
  }

  m_Polygons.ExpandAndGetRef().m_Vertices = vertices;

  if (bFlipWinding)
  {
    m_Polygons.PeekBack().FlipWinding();
  }
}

void WGeometry::AddLine(WUInt32 uiStartVertex, WUInt32 uiEndVertex)
{
  W_ASSERT_DEV(uiStartVertex < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", uiStartVertex, m_Vertices.GetCount());
  W_ASSERT_DEV(uiEndVertex < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", uiEndVertex, m_Vertices.GetCount());

  Line l;
  l.m_uiStartVertex = uiStartVertex;
  l.m_uiEndVertex = uiEndVertex;

  m_Lines.PushBack(l);
}


void WGeometry::TriangulatePolygons(WUInt32 uiMaxVerticesInPolygon /*= 3*/)
{
  uiMaxVerticesInPolygon = WMath::Max<WUInt32>(uiMaxVerticesInPolygon, 3);

  const WUInt32 uiNumPolys = m_Polygons.GetCount();

  for (WUInt32 p = 0; p < uiNumPolys; ++p)
  {
    const auto& poly = m_Polygons[p];

    const WUInt32 uiNumVerts = poly.m_Vertices.GetCount();
    if (uiNumVerts > uiMaxVerticesInPolygon)
    {
      for (WUInt32 v = 2; v < uiNumVerts; ++v)
      {
        auto& tri = m_Polygons.ExpandAndGetRef();
        tri.m_vNormal = poly.m_vNormal;
        tri.m_Vertices.SetCountUninitialized(3);
        tri.m_Vertices[0] = poly.m_Vertices[0];
        tri.m_Vertices[1] = poly.m_Vertices[v - 1];
        tri.m_Vertices[2] = poly.m_Vertices[v];
      }

      m_Polygons.RemoveAtAndSwap(p);
    }
  }
}

void WGeometry::ComputeFaceNormals()
{
  for (WUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
  {
    Polygon& poly = m_Polygons[p];

    const WVec3& v1 = m_Vertices[poly.m_Vertices[0]].m_vPosition;
    const WVec3& v2 = m_Vertices[poly.m_Vertices[1]].m_vPosition;
    const WVec3& v3 = m_Vertices[poly.m_Vertices[2]].m_vPosition;

    poly.m_vNormal.CalculateNormal(v1, v2, v3).IgnoreResult();
  }
}

void WGeometry::ComputeSmoothVertexNormals()
{
  // reset all vertex normals
  for (WUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vNormal.SetZero();
  }

  // add face normal of all adjacent faces to each vertex
  for (WUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
  {
    Polygon& poly = m_Polygons[p];

    for (WUInt32 v = 0; v < poly.m_Vertices.GetCount(); ++v)
    {
      m_Vertices[poly.m_Vertices[v]].m_vNormal += poly.m_vNormal;
    }
  }

  // normalize all vertex normals
  for (WUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vNormal.NormalizeIfNotZero(WVec3(0, 1, 0)).IgnoreResult();
  }
}

void WGeometry::ComputeTangents()
{
  // the tangent generation works on triangles only
  TriangulatePolygons();

  const WUInt32 uiVertexCount = m_Vertices.GetCount();
  const WUInt32 uiIndexCount = m_Polygons.GetCount() * 3;

  if (uiVertexCount == 0 || uiIndexCount == 0)
    return;

  // meshopt needs contiguous data, WDeque is not

  WTempArray<WVec3> positions;
  positions.SetCountUninitialized(uiVertexCount);

  WTempArray<WVec3> normals;
  normals.SetCountUninitialized(uiVertexCount);

  WTempArray<WVec2> texCoords;
  texCoords.SetCountUninitialized(uiVertexCount);

  for (WUInt32 v = 0; v < uiVertexCount; ++v)
  {
    positions[v] = m_Vertices[v].m_vPosition;
    normals[v] = m_Vertices[v].m_vNormal;
    texCoords[v] = m_Vertices[v].m_vTexCoord;
  }

  WTempArray<WUInt32> indices;
  indices.SetCountUninitialized(uiIndexCount);

  for (WUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
  {
    indices[p * 3 + 0] = m_Polygons[p].m_Vertices[0];
    indices[p * 3 + 1] = m_Polygons[p].m_Vertices[1];
    indices[p * 3 + 2] = m_Polygons[p].m_Vertices[2];
  }

  // one tangent per triangle corner
  WTempArray<WVec4> tangents;
  tangents.SetCountUninitialized(uiIndexCount);

  meshopt_generateTangents(&tangents[0].x, indices.GetData(), uiIndexCount, &positions[0].x, uiVertexCount, sizeof(WVec3), &normals[0].x, sizeof(WVec3), &texCoords[0].x, sizeof(WVec2), meshopt_TangentCompatible);

  // build a new vertex list, splitting up vertices whose corners ended up with different tangents
  // (and merging those that become identical)
  WMap<Vertex, WUInt32> vertMap;
  WDeque<Vertex> newVertices;

  for (WUInt32 i = 0; i < uiIndexCount; ++i)
  {
    Vertex v = m_Vertices[indices[i]];
    v.m_vTangent = tangents[i].GetAsVec3();
    v.m_fBiTangentSign = tangents[i].w;

    bool bExisted = false;
    auto it = vertMap.FindOrAdd(v, &bExisted);
    if (!bExisted)
    {
      it.Value() = newVertices.GetCount();
      newVertices.PushBack(v);
    }

    m_Polygons[i / 3].m_Vertices[i % 3] = it.Value();
  }

  m_Vertices = std::move(newVertices);
}

void WGeometry::ValidateTangents(float fEpsilon)
{
  for (auto& vertex : m_Vertices)
  {
    // checking for orthogonality to the normal and for squared unit length (standard case) or 3 (magic number for binormal inversion)
    if (!WMath::IsEqual(vertex.m_vNormal.GetLengthSquared(), 1.f, fEpsilon) || !WMath::IsEqual(vertex.m_vNormal.Dot(vertex.m_vTangent), 0.f, fEpsilon) || !(WMath::IsEqual(vertex.m_vTangent.GetLengthSquared(), 1.f, fEpsilon) || WMath::IsEqual(vertex.m_vTangent.GetLengthSquared(), 3.f, fEpsilon)))
    {
      vertex.m_vTangent.SetZero();
    }
  }
}

WUInt32 WGeometry::CalculateTriangleCount() const
{
  const WUInt32 numPolys = m_Polygons.GetCount();
  WUInt32 numTris = 0;

  for (WUInt32 p = 0; p < numPolys; ++p)
  {
    numTris += m_Polygons[p].m_Vertices.GetCount() - 2;
  }

  return numTris;
}

void WGeometry::SetAllVertexBoneIndices(const WVec4U16& vBoneIndices, WUInt32 uiFirstVertex)
{
  for (WUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_BoneIndices = vBoneIndices;
}

void WGeometry::SetAllVertexColor(const WColor& color, WUInt32 uiFirstVertex)
{
  for (WUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_Color = color;
}


void WGeometry::SetAllVertexTexCoord(const WVec2& vTexCoord, WUInt32 uiFirstVertex /*= 0*/)
{
  for (WUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_vTexCoord = vTexCoord;
}

void WGeometry::TransformVertices(const WMat4& mTransform, WUInt32 uiFirstVertex)
{
  if (mTransform.IsIdentity(WMath::SmallEpsilon<float>()))
    return;

  for (WUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vPosition = mTransform.TransformPosition(m_Vertices[v].m_vPosition);
    m_Vertices[v].m_vNormal = mTransform.TransformDirection(m_Vertices[v].m_vNormal);
  }
}

void WGeometry::Transform(const WMat4& mTransform, bool bTransformPolyNormals)
{
  TransformVertices(mTransform, 0);

  if (bTransformPolyNormals)
  {
    for (WUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
    {
      m_Polygons[p].m_vNormal = mTransform.TransformDirection(m_Polygons[p].m_vNormal);
    }
  }
}

void WGeometry::Merge(const WGeometry& other)
{
  const WUInt32 uiVertexOffset = m_Vertices.GetCount();

  for (WUInt32 v = 0; v < other.m_Vertices.GetCount(); ++v)
  {
    m_Vertices.PushBack(other.m_Vertices[v]);
  }

  for (WUInt32 p = 0; p < other.m_Polygons.GetCount(); ++p)
  {
    m_Polygons.PushBack(other.m_Polygons[p]);
    Polygon& poly = m_Polygons.PeekBack();

    for (WUInt32 pv = 0; pv < poly.m_Vertices.GetCount(); ++pv)
    {
      poly.m_Vertices[pv] += uiVertexOffset;
    }
  }

  for (WUInt32 l = 0; l < other.m_Lines.GetCount(); ++l)
  {
    Line line;
    line.m_uiStartVertex = other.m_Lines[l].m_uiStartVertex + uiVertexOffset;
    line.m_uiEndVertex = other.m_Lines[l].m_uiEndVertex + uiVertexOffset;

    m_Lines.PushBack(line);
  }
}

void WGeometry::AddRect(const WVec2& vSize, WUInt32 uiTesselationX, WUInt32 uiTesselationY, const GeoOptions& options)
{
  if (uiTesselationX == 0)
    uiTesselationX = 1;
  if (uiTesselationY == 0)
    uiTesselationY = 1;

  const WVec2 halfSize = vSize * 0.5f;
  const bool bFlipWinding = options.IsFlipWindingNecessary();

  const WQuat mainDir = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveZ, options.m_MainAxis);

  const WVec2 sizeFraction = vSize.CompDiv(WVec2(static_cast<float>(uiTesselationX), static_cast<float>(uiTesselationY)));

  for (WUInt32 vy = 0; vy < uiTesselationY + 1; ++vy)
  {
    for (WUInt32 vx = 0; vx < uiTesselationX + 1; ++vx)
    {
      const WVec2 tc((float)vx / (float)uiTesselationX, (float)vy / (float)uiTesselationY);

      AddVertex(options, mainDir * WVec3(-halfSize.x + vx * sizeFraction.x, -halfSize.y + vy * sizeFraction.y, 0), mainDir * WVec3(0, 0, 1), tc);
    }
  }

  WUInt32 idx[4];

  WUInt32 uiFirstIndex = 0;

  for (WUInt32 vy = 0; vy < uiTesselationY; ++vy)
  {
    for (WUInt32 vx = 0; vx < uiTesselationX; ++vx)
    {

      idx[0] = uiFirstIndex;
      idx[1] = uiFirstIndex + 1;
      idx[2] = uiFirstIndex + uiTesselationX + 2;
      idx[3] = uiFirstIndex + uiTesselationX + 1;

      AddPolygon(idx, bFlipWinding);

      ++uiFirstIndex;
    }

    ++uiFirstIndex;
  }
}

void WGeometry::AddBox(const WVec3& vFullExtents, bool bExtraVerticesForTexturing, const GeoOptions& options)
{
  const WVec3 halfSize = vFullExtents * 0.5f;
  const bool bFlipWinding = options.IsFlipWindingNecessary();

  if (bExtraVerticesForTexturing)
  {
    WUInt32 idx[4];

    {
      idx[0] = AddVertex(options, WVec3(-halfSize.x, -halfSize.y, +halfSize.z), WVec3(0, 0, 1), WVec2(0, 1));
      idx[1] = AddVertex(options, WVec3(+halfSize.x, -halfSize.y, +halfSize.z), WVec3(0, 0, 1), WVec2(0, 0));
      idx[2] = AddVertex(options, WVec3(+halfSize.x, +halfSize.y, +halfSize.z), WVec3(0, 0, 1), WVec2(1, 0));
      idx[3] = AddVertex(options, WVec3(-halfSize.x, +halfSize.y, +halfSize.z), WVec3(0, 0, 1), WVec2(1, 1));
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(options, WVec3(-halfSize.x, +halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(1, 0));
      idx[1] = AddVertex(options, WVec3(+halfSize.x, +halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(1, 1));
      idx[2] = AddVertex(options, WVec3(+halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0, 1));
      idx[3] = AddVertex(options, WVec3(-halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0, 0));
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(options, WVec3(-halfSize.x, -halfSize.y, -halfSize.z), WVec3(-1, 0, 0), WVec2(0, 1));
      idx[1] = AddVertex(options, WVec3(-halfSize.x, -halfSize.y, +halfSize.z), WVec3(-1, 0, 0), WVec2(0, 0));
      idx[2] = AddVertex(options, WVec3(-halfSize.x, +halfSize.y, +halfSize.z), WVec3(-1, 0, 0), WVec2(1, 0));
      idx[3] = AddVertex(options, WVec3(-halfSize.x, +halfSize.y, -halfSize.z), WVec3(-1, 0, 0), WVec2(1, 1));
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(options, WVec3(+halfSize.x, +halfSize.y, -halfSize.z), WVec3(1, 0, 0), WVec2(0, 1));
      idx[1] = AddVertex(options, WVec3(+halfSize.x, +halfSize.y, +halfSize.z), WVec3(1, 0, 0), WVec2(0, 0));
      idx[2] = AddVertex(options, WVec3(+halfSize.x, -halfSize.y, +halfSize.z), WVec3(1, 0, 0), WVec2(1, 0));
      idx[3] = AddVertex(options, WVec3(+halfSize.x, -halfSize.y, -halfSize.z), WVec3(1, 0, 0), WVec2(1, 1));
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(options, WVec3(+halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, -1, 0), WVec2(0, 1));
      idx[1] = AddVertex(options, WVec3(+halfSize.x, -halfSize.y, +halfSize.z), WVec3(0, -1, 0), WVec2(0, 0));
      idx[2] = AddVertex(options, WVec3(-halfSize.x, -halfSize.y, +halfSize.z), WVec3(0, -1, 0), WVec2(1, 0));
      idx[3] = AddVertex(options, WVec3(-halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, -1, 0), WVec2(1, 1));
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(options, WVec3(-halfSize.x, +halfSize.y, -halfSize.z), WVec3(0, +1, 0), WVec2(0, 1));
      idx[1] = AddVertex(options, WVec3(-halfSize.x, +halfSize.y, +halfSize.z), WVec3(0, +1, 0), WVec2(0, 0));
      idx[2] = AddVertex(options, WVec3(+halfSize.x, +halfSize.y, +halfSize.z), WVec3(0, +1, 0), WVec2(1, 0));
      idx[3] = AddVertex(options, WVec3(+halfSize.x, +halfSize.y, -halfSize.z), WVec3(0, +1, 0), WVec2(1, 1));
      AddPolygon(idx, bFlipWinding);
    }
  }
  else
  {
    WUInt32 idx[8];

    idx[0] = AddVertex(options, WVec3(-halfSize.x, -halfSize.y, halfSize.z), WVec3(0, 0, 1), WVec2(0));
    idx[1] = AddVertex(options, WVec3(halfSize.x, -halfSize.y, halfSize.z), WVec3(0, 0, 1), WVec2(0));
    idx[2] = AddVertex(options, WVec3(halfSize.x, halfSize.y, halfSize.z), WVec3(0, 0, 1), WVec2(0));
    idx[3] = AddVertex(options, WVec3(-halfSize.x, halfSize.y, halfSize.z), WVec3(0, 0, 1), WVec2(0));

    idx[4] = AddVertex(options, WVec3(-halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0));
    idx[5] = AddVertex(options, WVec3(halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0));
    idx[6] = AddVertex(options, WVec3(halfSize.x, halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0));
    idx[7] = AddVertex(options, WVec3(-halfSize.x, halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0));

    WUInt32 poly[4];

    poly[0] = idx[0];
    poly[1] = idx[1];
    poly[2] = idx[2];
    poly[3] = idx[3];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[1];
    poly[1] = idx[5];
    poly[2] = idx[6];
    poly[3] = idx[2];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[5];
    poly[1] = idx[4];
    poly[2] = idx[7];
    poly[3] = idx[6];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[4];
    poly[1] = idx[0];
    poly[2] = idx[3];
    poly[3] = idx[7];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[4];
    poly[1] = idx[5];
    poly[2] = idx[1];
    poly[3] = idx[0];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[3];
    poly[1] = idx[2];
    poly[2] = idx[6];
    poly[3] = idx[7];
    AddPolygon(poly, bFlipWinding);
  }
}

void WGeometry::AddLineBox(const WVec3& vSize, const GeoOptions& options)
{
  const WVec3 halfSize = vSize * 0.5f;

  AddVertex(options, WVec3(-halfSize.x, -halfSize.y, halfSize.z), WVec3(0, 0, 1), WVec2(0));
  AddVertex(options, WVec3(halfSize.x, -halfSize.y, halfSize.z), WVec3(0, 0, 1), WVec2(0));
  AddVertex(options, WVec3(halfSize.x, halfSize.y, halfSize.z), WVec3(0, 0, 1), WVec2(0));
  AddVertex(options, WVec3(-halfSize.x, halfSize.y, halfSize.z), WVec3(0, 0, 1), WVec2(0));

  AddVertex(options, WVec3(-halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0));
  AddVertex(options, WVec3(halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0));
  AddVertex(options, WVec3(halfSize.x, halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0));
  AddVertex(options, WVec3(-halfSize.x, halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0));

  AddLine(0, 1);
  AddLine(1, 2);
  AddLine(2, 3);
  AddLine(3, 0);

  AddLine(4, 5);
  AddLine(5, 6);
  AddLine(6, 7);
  AddLine(7, 4);

  AddLine(0, 4);
  AddLine(1, 5);
  AddLine(2, 6);
  AddLine(3, 7);
}

void WGeometry::AddLineBoxCorners(const WVec3& vSize, float fCornerFraction, const GeoOptions& options)
{
  fCornerFraction = WMath::Clamp(fCornerFraction, 0.0f, 1.0f);
  fCornerFraction *= 0.5f;
  const WVec3 halfSize = vSize * 0.5f;

  AddVertex(options, WVec3(-halfSize.x, -halfSize.y, halfSize.z), WVec3(0, 0, 1), WVec2(0));
  AddVertex(options, WVec3(halfSize.x, -halfSize.y, halfSize.z), WVec3(0, 0, 1), WVec2(0));
  AddVertex(options, WVec3(halfSize.x, halfSize.y, halfSize.z), WVec3(0, 0, 1), WVec2(0));
  AddVertex(options, WVec3(-halfSize.x, halfSize.y, halfSize.z), WVec3(0, 0, 1), WVec2(0));

  AddVertex(options, WVec3(-halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0));
  AddVertex(options, WVec3(halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0));
  AddVertex(options, WVec3(halfSize.x, halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0));
  AddVertex(options, WVec3(-halfSize.x, halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0));

  for (WUInt32 c = 0; c < 8; ++c)
  {
    const WVec3& op = m_Vertices[c].m_vPosition;

    const WVec3 op1 = WVec3(op.x, op.y, -WMath::Sign(op.z) * WMath::Abs(op.z));
    const WVec3 op2 = WVec3(op.x, -WMath::Sign(op.y) * WMath::Abs(op.y), op.z);
    const WVec3 op3 = WVec3(-WMath::Sign(op.x) * WMath::Abs(op.x), op.y, op.z);

    const WUInt32 ix1 = AddVertex(options, WMath::Lerp(op, op1, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord);
    const WUInt32 ix2 = AddVertex(options, WMath::Lerp(op, op2, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord);
    const WUInt32 ix3 = AddVertex(options, WMath::Lerp(op, op3, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord);

    AddLine(c, ix1);
    AddLine(c, ix2);
    AddLine(c, ix3);
  }
}

void WGeometry::AddPyramid(float fBaseSize, float fHeight, bool bCap, const GeoOptions& options)
{
  const WQuat tilt = WBasisAxis::GetBasisRotation(options.m_MainAxis, WBasisAxis::PositiveZ);
  const WMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const float halfSize = fBaseSize * 0.5f;
  const bool bFlipWinding = options.IsFlipWindingNecessary();
  WUInt32 quad[4];

  quad[0] = AddVertex(trans, options, WVec3(-halfSize, halfSize, 0), WVec3(-1, 1, 0).GetNormalized(), WVec2(0));
  quad[1] = AddVertex(trans, options, WVec3(halfSize, halfSize, 0), WVec3(1, 1, 0).GetNormalized(), WVec2(0));
  quad[2] = AddVertex(trans, options, WVec3(halfSize, -halfSize, 0), WVec3(1, -1, 0).GetNormalized(), WVec2(0));
  quad[3] = AddVertex(trans, options, WVec3(-halfSize, -halfSize, 0), WVec3(-1, -1, 0).GetNormalized(), WVec2(0));

  const WUInt32 tip = AddVertex(trans, options, WVec3(0, 0, fHeight), WVec3(0, 0, 1), WVec2(0));

  if (bCap)
  {
    AddPolygon(quad, bFlipWinding);
  }

  WUInt32 tri[3];

  tri[0] = quad[1];
  tri[1] = quad[0];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);

  tri[0] = quad[2];
  tri[1] = quad[1];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);

  tri[0] = quad[3];
  tri[1] = quad[2];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);

  tri[0] = quad[0];
  tri[1] = quad[3];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);
}

void WGeometry::AddGeodesicSphere(float fRadius, WUInt8 uiSubDivisions, const GeoOptions& options)
{
  const bool bFlipWinding = options.IsFlipWindingNecessary();
  struct Triangle
  {
    Triangle(WUInt32 ui1, WUInt32 ui2, WUInt32 ui3)
    {
      m_uiIndex[0] = ui1;
      m_uiIndex[1] = ui2;
      m_uiIndex[2] = ui3;
    }

    WUInt32 m_uiIndex[3];
  };

  struct Edge
  {
    Edge() = default;

    Edge(WUInt32 uiId1, WUInt32 uiId2)
    {
      m_uiVertex[0] = WMath::Min(uiId1, uiId2);
      m_uiVertex[1] = WMath::Max(uiId1, uiId2);
    }

    bool operator<(const Edge& rhs) const
    {
      if (m_uiVertex[0] < rhs.m_uiVertex[0])
        return true;
      if (m_uiVertex[0] > rhs.m_uiVertex[0])
        return false;
      return m_uiVertex[1] < rhs.m_uiVertex[1];
    }

    bool operator==(const Edge& rhs) const { return m_uiVertex[0] == rhs.m_uiVertex[0] && m_uiVertex[1] == rhs.m_uiVertex[1]; }

    WUInt32 m_uiVertex[2];
  };

  const WUInt32 uiFirstVertex = m_Vertices.GetCount();

  WInt32 iCurrentList = 0;
  WDeque<Triangle> Tris[2];
  WVec4U16 boneIndices(options.m_uiBoneIndex, 0, 0, 0);

  // create icosahedron
  {
    WMat3 mRotX, mRotZ, mRotZh;
    mRotX = WMat3::MakeRotationX(WAngle::MakeFromDegree(360.0f / 6.0f));
    mRotZ = WMat3::MakeRotationZ(WAngle::MakeFromDegree(-360.0f / 5.0f));
    mRotZh = WMat3::MakeRotationZ(WAngle::MakeFromDegree(-360.0f / 10.0f));

    WUInt32 vert[12];
    WVec3 vDir(0, 0, 1);

    vDir.Normalize();
    vert[0] = AddVertex(vDir * fRadius, vDir, WVec2::MakeZero(), options.m_Color, boneIndices);

    vDir = mRotX * vDir;

    for (WInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[1 + i] = AddVertex(vDir * fRadius, vDir, WVec2::MakeZero(), options.m_Color, boneIndices);
      vDir = mRotZ * vDir;
    }

    vDir = mRotX * vDir;
    vDir = mRotZh * vDir;

    for (WInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[6 + i] = AddVertex(vDir * fRadius, vDir, WVec2::MakeZero(), options.m_Color, boneIndices);
      vDir = mRotZ * vDir;
    }

    vDir.Set(0, 0, -1);
    vDir.Normalize();
    vert[11] = AddVertex(vDir * fRadius, vDir, WVec2::MakeZero(), options.m_Color, boneIndices);


    Tris[0].PushBack(Triangle(vert[0], vert[2], vert[1]));
    Tris[0].PushBack(Triangle(vert[0], vert[3], vert[2]));
    Tris[0].PushBack(Triangle(vert[0], vert[4], vert[3]));
    Tris[0].PushBack(Triangle(vert[0], vert[5], vert[4]));
    Tris[0].PushBack(Triangle(vert[0], vert[1], vert[5]));

    Tris[0].PushBack(Triangle(vert[1], vert[2], vert[6]));
    Tris[0].PushBack(Triangle(vert[2], vert[3], vert[7]));
    Tris[0].PushBack(Triangle(vert[3], vert[4], vert[8]));
    Tris[0].PushBack(Triangle(vert[4], vert[5], vert[9]));
    Tris[0].PushBack(Triangle(vert[5], vert[1], vert[10]));

    Tris[0].PushBack(Triangle(vert[2], vert[7], vert[6]));
    Tris[0].PushBack(Triangle(vert[3], vert[8], vert[7]));
    Tris[0].PushBack(Triangle(vert[4], vert[9], vert[8]));
    Tris[0].PushBack(Triangle(vert[5], vert[10], vert[9]));
    Tris[0].PushBack(Triangle(vert[6], vert[10], vert[1]));

    Tris[0].PushBack(Triangle(vert[7], vert[11], vert[6]));
    Tris[0].PushBack(Triangle(vert[8], vert[11], vert[7]));
    Tris[0].PushBack(Triangle(vert[9], vert[11], vert[8]));
    Tris[0].PushBack(Triangle(vert[10], vert[11], vert[9]));
    Tris[0].PushBack(Triangle(vert[6], vert[11], vert[10]));
  }

  WMap<Edge, WUInt32> NewVertices;

  // subdivide the icosahedron n times (splitting every triangle into 4 new triangles)
  for (WUInt32 div = 0; div < uiSubDivisions; ++div)
  {
    // switch the last result and the new result
    const WInt32 iPrevList = iCurrentList;
    iCurrentList = (iCurrentList + 1) % 2;

    Tris[iCurrentList].Clear();
    NewVertices.Clear();

    for (WUInt32 tri = 0; tri < Tris[iPrevList].GetCount(); ++tri)
    {
      WUInt32 uiVert[3] = {Tris[iPrevList][tri].m_uiIndex[0], Tris[iPrevList][tri].m_uiIndex[1], Tris[iPrevList][tri].m_uiIndex[2]};

      Edge Edges[3] = {Edge(uiVert[0], uiVert[1]), Edge(uiVert[1], uiVert[2]), Edge(uiVert[2], uiVert[0])};

      WUInt32 uiNewVert[3];

      // split each edge of the triangle in half
      for (WUInt32 i = 0; i < 3; ++i)
      {
        // do not split an edge that was split before, we want shared vertices everywhere
        if (NewVertices.Find(Edges[i]).IsValid())
          uiNewVert[i] = NewVertices[Edges[i]];
        else
        {
          const WVec3 vCenter = (m_Vertices[Edges[i].m_uiVertex[0]].m_vPosition + m_Vertices[Edges[i].m_uiVertex[1]].m_vPosition).GetNormalized();
          uiNewVert[i] = AddVertex(vCenter * fRadius, vCenter, WVec2::MakeZero(), options.m_Color, boneIndices);

          NewVertices[Edges[i]] = uiNewVert[i];
        }
      }

      // now we turn one triangle into 4 smaller ones
      Tris[iCurrentList].PushBack(Triangle(uiVert[0], uiNewVert[0], uiNewVert[2]));
      Tris[iCurrentList].PushBack(Triangle(uiNewVert[0], uiVert[1], uiNewVert[1]));
      Tris[iCurrentList].PushBack(Triangle(uiNewVert[1], uiVert[2], uiNewVert[2]));

      Tris[iCurrentList].PushBack(Triangle(uiNewVert[0], uiNewVert[1], uiNewVert[2]));
    }
  }

  // add the final list of triangles to the output
  for (WUInt32 tri = 0; tri < Tris[iCurrentList].GetCount(); ++tri)
  {
    AddPolygon(Tris[iCurrentList][tri].m_uiIndex, bFlipWinding);
  }

  // finally apply the user transformation on the new vertices
  TransformVertices(options.m_Transform, uiFirstVertex);
}

void WGeometry::AddCylinder(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, bool bCapTop, bool bCapBottom, WUInt16 uiSegments, const GeoOptions& options, WAngle fraction /*= WAngle::MakeFromDegree(360.0f)*/)
{
  uiSegments = WMath::Max<WUInt16>(uiSegments, 3u);
  fraction = WMath::Clamp(fraction, WAngle(), WAngle::MakeFromDegree(360.0f));

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const bool bIsFraction = fraction.GetDegree() < 360.0f;
  const WAngle fDegStep = WAngle::MakeFromDegree(fraction.GetDegree() / uiSegments);

  const WVec3 vTopCenter(0, 0, fPositiveLength);
  const WVec3 vBottomCenter(0, 0, -fNegativeLength);

  const WQuat tilt = WBasisAxis::GetBasisRotation(options.m_MainAxis, WBasisAxis::PositiveZ);
  const WMat4 trans = options.m_Transform * tilt.GetAsMat4();

  // cylinder wall
  {
    WTempHybridArray<WUInt32, 512> VertsTop;
    WTempHybridArray<WUInt32, 512> VertsBottom;

    for (WInt32 i = 0; i <= uiSegments; ++i)
    {
      const WAngle deg = (float)i * fDegStep;

      float fU = 4.0f - deg.GetDegree() / 90.0f;

      const float fX = WMath::Cos(deg);
      const float fY = WMath::Sin(deg);

      const WVec3 vDir(fX, fY, 0);

      VertsTop.PushBack(AddVertex(trans, options, vTopCenter + vDir * fRadiusTop, vDir, WVec2(fU, 0)));
      VertsBottom.PushBack(AddVertex(trans, options, vBottomCenter + vDir * fRadiusBottom, vDir, WVec2(fU, 1)));
    }

    for (WUInt32 i = 1; i <= uiSegments; ++i)
    {
      WUInt32 quad[4];
      quad[0] = VertsBottom[i - 1];
      quad[1] = VertsBottom[i];
      quad[2] = VertsTop[i];
      quad[3] = VertsTop[i - 1];


      AddPolygon(quad, bFlipWinding);
    }
  }

  // walls for fractional cylinders
  if (bIsFraction)
  {
    const WVec3 vDir0(1, 0, 0);
    const WVec3 vDir1(WMath::Cos(fraction), WMath::Sin(fraction), 0);

    WUInt32 quad[4];

    const WVec3 vNrm0 = -WVec3(0, 0, 1).CrossRH(vDir0).GetNormalized();
    quad[0] = AddVertex(trans, options, vTopCenter + vDir0 * fRadiusTop, vNrm0, WVec2(0, 0));
    quad[1] = AddVertex(trans, options, vTopCenter, vNrm0, WVec2(1, 0));
    quad[2] = AddVertex(trans, options, vBottomCenter, vNrm0, WVec2(1, 1));
    quad[3] = AddVertex(trans, options, vBottomCenter + vDir0 * fRadiusBottom, vNrm0, WVec2(0, 1));


    AddPolygon(quad, bFlipWinding);

    const WVec3 vNrm1 = WVec3(0, 0, 1).CrossRH(vDir1).GetNormalized();
    quad[0] = AddVertex(trans, options, vTopCenter, vNrm1, WVec2(0, 0));
    quad[1] = AddVertex(trans, options, vTopCenter + vDir1 * fRadiusTop, vNrm1, WVec2(1, 0));
    quad[2] = AddVertex(trans, options, vBottomCenter + vDir1 * fRadiusBottom, vNrm1, WVec2(1, 1));
    quad[3] = AddVertex(trans, options, vBottomCenter, vNrm1, WVec2(0, 1));

    AddPolygon(quad, bFlipWinding);
  }

  if (bCapBottom)
  {
    WTempHybridArray<WUInt32, 512> VertsBottom;

    if (bIsFraction)
    {
      const WUInt32 uiCenterVtx = AddVertex(trans, options, vBottomCenter, WVec3(0, 0, -1), WVec2(0));

      for (WInt32 i = uiSegments; i >= 0; --i)
      {
        const WAngle deg = (float)i * fDegStep;

        const float fX = WMath::Cos(deg);
        const float fY = WMath::Sin(deg);

        const WVec3 vDir(fX, fY, 0);

        AddVertex(trans, options, vBottomCenter + vDir * fRadiusBottom, WVec3(0, 0, -1), WVec2(fY, fX));
      }

      VertsBottom.SetCountUninitialized(3);
      VertsBottom[0] = uiCenterVtx;

      for (WUInt32 i = 0; i < uiSegments; ++i)
      {
        VertsBottom[1] = uiCenterVtx + i + 1;
        VertsBottom[2] = uiCenterVtx + i + 2;

        AddPolygon(VertsBottom, bFlipWinding);
      }
    }
    else
    {
      for (WInt32 i = uiSegments - 1; i >= 0; --i)
      {
        const WAngle deg = (float)i * fDegStep;

        const float fX = WMath::Cos(deg);
        const float fY = WMath::Sin(deg);

        const WVec3 vDir(fX, fY, 0);

        VertsBottom.PushBack(AddVertex(trans, options, vBottomCenter + vDir * fRadiusBottom, WVec3(0, 0, -1), WVec2(fY, fX)));
      }

      AddPolygon(VertsBottom, bFlipWinding);
    }
  }

  if (bCapTop)
  {
    WTempHybridArray<WUInt32, 512> VertsTop;

    if (bIsFraction)
    {
      const WUInt32 uiCenterVtx = AddVertex(trans, options, vTopCenter, WVec3(0, 0, 1), WVec2(0));

      for (WInt32 i = 0; i <= uiSegments; ++i)
      {
        const WAngle deg = (float)i * fDegStep;

        const float fX = WMath::Cos(deg);
        const float fY = WMath::Sin(deg);

        const WVec3 vDir(fX, fY, 0);

        AddVertex(trans, options, vTopCenter + vDir * fRadiusTop, WVec3(0, 0, 1), WVec2(fY, -fX));
      }

      VertsTop.SetCountUninitialized(3);
      VertsTop[0] = uiCenterVtx;

      for (WUInt32 i = 0; i < uiSegments; ++i)
      {
        VertsTop[1] = uiCenterVtx + i + 1;
        VertsTop[2] = uiCenterVtx + i + 2;

        AddPolygon(VertsTop, bFlipWinding);
      }
    }
    else
    {
      for (WInt32 i = 0; i < uiSegments; ++i)
      {
        const WAngle deg = (float)i * fDegStep;

        const float fX = WMath::Cos(deg);
        const float fY = WMath::Sin(deg);

        const WVec3 vDir(fX, fY, 0);

        VertsTop.PushBack(AddVertex(trans, options, vTopCenter + vDir * fRadiusTop, WVec3(0, 0, 1), WVec2(fY, -fX)));
      }

      AddPolygon(VertsTop, bFlipWinding);
    }
  }
}

void WGeometry::AddCylinderOnePiece(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, WUInt16 uiSegments, const GeoOptions& options)
{
  uiSegments = WMath::Max<WUInt16>(uiSegments, 3u);

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const WAngle fDegStep = WAngle::MakeFromDegree(360.0f / uiSegments);

  const WQuat tilt = WBasisAxis::GetBasisRotation(options.m_MainAxis, WBasisAxis::PositiveZ);
  const WMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const WVec3 vTopCenter(0, 0, fPositiveLength);
  const WVec3 vBottomCenter(0, 0, -fNegativeLength);

  // cylinder wall
  {
    WTempHybridArray<WUInt32, 512> VertsTop;
    WTempHybridArray<WUInt32, 512> VertsBottom;

    for (WInt32 i = 0; i < uiSegments; ++i)
    {
      const WAngle deg = (float)i * fDegStep;

      float fU = 4.0f - deg.GetDegree() / 90.0f;

      const float fX = WMath::Cos(deg);
      const float fY = WMath::Sin(deg);

      const WVec3 vDir(fX, fY, 0);

      VertsTop.PushBack(AddVertex(trans, options, vTopCenter + vDir * fRadiusTop, vDir, WVec2(fU, 0)));
      VertsBottom.PushBack(AddVertex(trans, options, vBottomCenter + vDir * fRadiusBottom, vDir, WVec2(fU, 1)));
    }

    for (WUInt32 i = 1; i <= uiSegments; ++i)
    {
      WUInt32 quad[4];
      quad[0] = VertsBottom[i - 1];
      quad[1] = VertsBottom[i % uiSegments];
      quad[2] = VertsTop[i % uiSegments];
      quad[3] = VertsTop[i - 1];

      AddPolygon(quad, bFlipWinding);
    }

    AddPolygon(VertsTop, bFlipWinding);
    AddPolygon(VertsBottom, !bFlipWinding);
  }
}

void WGeometry::AddLineCylinder(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, WUInt16 uiSegments, const GeoOptions& options /*= GeoOptions()*/)
{
  uiSegments = WMath::Max<WUInt16>(uiSegments, 3u);

  const WAngle fDegStep = WAngle::MakeFromDegree(360.0f / uiSegments);

  const WQuat tilt = WBasisAxis::GetBasisRotation(options.m_MainAxis, WBasisAxis::PositiveZ);
  const WMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const WVec3 vTopCenter(0, 0, fPositiveLength);
  const WVec3 vBottomCenter(0, 0, -fNegativeLength);

  // cylinder wall
  {
    WTempHybridArray<WUInt32, 512> VertsTop;
    WTempHybridArray<WUInt32, 512> VertsBottom;

    for (WInt32 i = 0; i < uiSegments; ++i)
    {
      const WAngle deg = (float)i * fDegStep;

      float fU = 4.0f - deg.GetDegree() / 90.0f;

      const float fX = WMath::Cos(deg);
      const float fY = WMath::Sin(deg);

      const WVec3 vDir(fX, fY, 0);

      VertsTop.PushBack(AddVertex(trans, options, vTopCenter + vDir * fRadiusTop, vDir, WVec2(fU, 0)));
      VertsBottom.PushBack(AddVertex(trans, options, vBottomCenter + vDir * fRadiusBottom, vDir, WVec2(fU, 1)));
    }

    for (WUInt32 i = 1; i <= uiSegments; ++i)
    {
      WUInt32 quad[4];
      quad[0] = VertsBottom[i - 1];
      quad[1] = VertsBottom[i % uiSegments];
      quad[2] = VertsTop[i % uiSegments];
      quad[3] = VertsTop[i - 1];

      AddLine(quad[0], quad[1]);
      AddLine(quad[1], quad[2]);
      AddLine(quad[2], quad[3]);
      AddLine(quad[3], quad[0]);
    }
  }
}

void WGeometry::AddCone(float fRadius, float fHeight, bool bCap, WUInt16 uiSegments, const GeoOptions& options)
{
  uiSegments = WMath::Max<WUInt16>(uiSegments, 3);

  const WQuat tilt = WBasisAxis::GetBasisRotation(options.m_MainAxis, WBasisAxis::PositiveZ);
  const WMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const bool bFlipWinding = options.IsFlipWindingNecessary();

  WTempHybridArray<WUInt32, 512> VertsBottom;

  const WAngle fDegStep = WAngle::MakeFromDegree(360.0f / uiSegments);

  const WUInt32 uiTip = AddVertex(trans, options, WVec3(0, 0, fHeight), WVec3(0, 0, 1));

  for (WInt32 i = uiSegments - 1; i >= 0; --i)
  {
    const WAngle deg = (float)i * fDegStep;

    WVec3 vDir(WMath::Cos(deg), WMath::Sin(deg), 0);

    VertsBottom.PushBack(AddVertex(trans, options, vDir * fRadius, vDir));
  }

  WUInt32 uiPrevSeg = uiSegments - 1;

  for (WUInt32 i = 0; i < uiSegments; ++i)
  {
    WUInt32 tri[3];
    tri[0] = VertsBottom[uiPrevSeg];
    tri[1] = uiTip;
    tri[2] = VertsBottom[i];

    uiPrevSeg = i;

    AddPolygon(tri, bFlipWinding);
  }

  if (bCap)
  {
    AddPolygon(VertsBottom, bFlipWinding);
  }
}

void WGeometry::AddStackedSphere(float fRadius, WUInt16 uiSegments, WUInt16 uiStacks, const GeoOptions& options)
{
  uiSegments = WMath::Max<WUInt16>(uiSegments, 3u);
  uiStacks = WMath::Max<WUInt16>(uiStacks, 2u);

  const WQuat tilt = WBasisAxis::GetBasisRotation(options.m_MainAxis, WBasisAxis::PositiveZ);
  const WMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const WAngle fDegreeDiffSegments = WAngle::MakeFromDegree(360.0f / (float)(uiSegments));
  const WAngle fDegreeDiffStacks = WAngle::MakeFromDegree(180.0f / (float)(uiStacks));

  const WUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  for (WUInt32 st = 1; st < uiStacks; ++st)
  {
    const WAngle fDegreeStack = WAngle::MakeFromDegree(-90.0f + (st * fDegreeDiffStacks.GetDegree()));
    const float fCosDS = WMath::Cos(fDegreeStack);
    const float fSinDS = WMath::Sin(fDegreeStack);
    const float fY = -fSinDS * fRadius;

    const float fV = (float)st / (float)uiStacks;

    for (WUInt32 sp = 0; sp < uiSegments + 1u; ++sp)
    {
      float fU = ((float)sp / (float)(uiSegments)) * 2.0f;

      const WAngle fDegree = (float)sp * fDegreeDiffSegments;

      WVec3 vPos;
      vPos.x = WMath::Cos(fDegree) * fRadius * fCosDS;
      vPos.y = -WMath::Sin(fDegree) * fRadius * fCosDS;
      vPos.z = fY;

      WVec3 vNormal = vPos;
      vNormal.NormalizeIfNotZero(WVec3(0, 0, 1)).IgnoreResult();
      AddVertex(trans, options, vPos, vNormal, WVec2(fU, fV));
    }
  }

  WUInt32 tri[3];
  WUInt32 quad[4];

  // now create the top cone
  for (WUInt32 p = 0; p < uiSegments; ++p)
  {
    float fU = ((p + 0.5f) / (float)(uiSegments)) * 2.0f;

    tri[0] = AddVertex(trans, options, WVec3(0, 0, fRadius), WVec3(0, 0, 1), WVec2(fU, 0));
    tri[1] = uiFirstVertex + p + 1;
    tri[2] = uiFirstVertex + p;

    AddPolygon(tri, bFlipWinding);
  }

  // now create the stacks in the middle
  for (WUInt16 st = 0; st < uiStacks - 2; ++st)
  {
    const WUInt32 uiRowBottom = (uiSegments + 1) * st;
    const WUInt32 uiRowTop = (uiSegments + 1) * (st + 1);

    for (WInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + i + 1);
      quad[1] = uiFirstVertex + (uiRowTop + i);
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[3] = uiFirstVertex + (uiRowBottom + i + 1);

      AddPolygon(quad, bFlipWinding);
    }
  }

  const WInt32 iTopStack = (uiSegments + 1) * (uiStacks - 2);

  // now create the bottom cone
  for (WUInt32 p = 0; p < uiSegments; ++p)
  {
    float fU = ((p + 0.5f) / (float)(uiSegments)) * 2.0f;

    tri[0] = AddVertex(trans, options, WVec3(0, 0, -fRadius), WVec3(0, 0, -1), WVec2(fU, 1));
    tri[1] = uiFirstVertex + (iTopStack + p);
    tri[2] = uiFirstVertex + (iTopStack + p + 1);

    AddPolygon(tri, bFlipWinding);
  }
}

void WGeometry::AddHalfSphere(float fRadius, WUInt16 uiSegments, WUInt16 uiStacks, bool bCap, const GeoOptions& options)
{
  uiSegments = WMath::Max<WUInt16>(uiSegments, 3u);
  uiStacks = WMath::Max<WUInt16>(uiStacks, 1u);

  const WQuat tilt = WBasisAxis::GetBasisRotation(options.m_MainAxis, WBasisAxis::PositiveZ);
  const WMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const WAngle fDegreeDiffSegments = WAngle::MakeFromDegree(360.0f / (float)(uiSegments));
  const WAngle fDegreeDiffStacks = WAngle::MakeFromDegree(90.0f / (float)(uiStacks));

  const WUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  for (WUInt32 st = 0; st < uiStacks; ++st)
  {
    const WAngle fDegreeStack = WAngle::MakeFromDegree(-90.0f + ((st + 1) * fDegreeDiffStacks.GetDegree()));
    const float fCosDS = WMath::Cos(fDegreeStack);
    const float fSinDS = WMath::Sin(fDegreeStack);
    const float fY = -fSinDS * fRadius;

    const float fV = (float)(st + 1) / (float)uiStacks;

    for (WUInt32 sp = 0; sp <= uiSegments; ++sp)
    {
      float fU = ((float)sp / (float)(uiSegments)) * 2.0f;

      if (fU > 1.0f)
        fU = 2.0f - fU;

      // the vertices for the bottom disk
      const WAngle fDegree = (float)sp * fDegreeDiffSegments;

      WVec3 vPos;
      vPos.x = WMath::Cos(fDegree) * fRadius * fCosDS;
      vPos.y = WMath::Sin(fDegree) * fRadius * fCosDS;
      vPos.z = fY;

      AddVertex(trans, options, vPos, vPos.GetNormalized(), WVec2(fU, fV));
    }
  }

  WUInt32 uiTopVertex = AddVertex(trans, options, WVec3(0, 0, fRadius), WVec3(0, 0, 1), WVec2(0.0f));

  WUInt32 tri[3];
  WUInt32 quad[4];

  // now create the top cone
  for (WUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiTopVertex;
    tri[1] = uiFirstVertex + p;
    tri[2] = uiFirstVertex + ((p + 1) % (uiSegments + 1));

    AddPolygon(tri, bFlipWinding);
  }

  // now create the stacks in the middle

  for (WUInt16 st = 0; st < uiStacks - 1; ++st)
  {
    const WUInt32 uiRowBottom = (uiSegments + 1) * st;
    const WUInt32 uiRowTop = (uiSegments + 1) * (st + 1);

    for (WInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + ((i + 1) % (uiSegments + 1)));
      quad[1] = uiFirstVertex + (uiRowBottom + ((i + 1) % (uiSegments + 1)));
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[3] = uiFirstVertex + (uiRowTop + i);

      AddPolygon(quad, bFlipWinding);
    }
  }

  if (bCap)
  {
    WTempHybridArray<WUInt32, 256> uiCap;

    for (WUInt32 i = uiTopVertex - 1; i >= uiTopVertex - uiSegments; --i)
      uiCap.PushBack(i);

    AddPolygon(uiCap, bFlipWinding);
  }
}

void WGeometry::AddCapsule(float fRadius, float fHeight, WUInt16 uiSegments, WUInt16 uiStacks, const GeoOptions& options)
{
  uiSegments = WMath::Max<WUInt16>(uiSegments, 3u);
  uiStacks = WMath::Max<WUInt16>(uiStacks, 1u);
  fHeight = WMath::Max(fHeight, 0.0f);

  const WQuat tilt = WBasisAxis::GetBasisRotation(options.m_MainAxis, WBasisAxis::PositiveZ);
  const WMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const WAngle fDegreeDiffStacks = WAngle::MakeFromDegree(90.0f / (float)(uiStacks));

  const WUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  const float fDegreeStepSlices = 360.0f / (float)(uiSegments);

  float fOffset = fHeight * 0.5f;

  // for (WUInt32 h = 0; h < 2; ++h)
  {
    for (WUInt32 st = 0; st < uiStacks; ++st)
    {
      const WAngle fDegreeStack = WAngle::MakeFromDegree(-90.0f + ((st + 1) * fDegreeDiffStacks.GetDegree()));
      const float fCosDS = WMath::Cos(fDegreeStack);
      const float fSinDS = WMath::Sin(fDegreeStack);
      const float fY = -fSinDS * fRadius;

      for (WUInt32 sp = 0; sp < uiSegments; ++sp)
      {
        const WAngle fDegree = WAngle::MakeFromDegree(sp * fDegreeStepSlices);

        WVec3 vPos;
        vPos.x = WMath::Cos(fDegree) * fRadius * fCosDS;
        vPos.z = fY + fOffset;
        vPos.y = WMath::Sin(fDegree) * fRadius * fCosDS;

        AddVertex(trans, options, vPos, vPos.GetNormalized(), WVec2(0));
      }
    }

    fOffset -= fHeight;

    for (WUInt32 st = 0; st < uiStacks; ++st)
    {
      const WAngle fDegreeStack = WAngle::MakeFromDegree(0.0f - (st * fDegreeDiffStacks.GetDegree()));
      const float fCosDS = WMath::Cos(fDegreeStack);
      const float fSinDS = WMath::Sin(fDegreeStack);
      const float fY = fSinDS * fRadius;

      for (WUInt32 sp = 0; sp < uiSegments; ++sp)
      {
        const WAngle fDegree = WAngle::MakeFromDegree(sp * fDegreeStepSlices);

        WVec3 vPos;
        vPos.x = WMath::Cos(fDegree) * fRadius * fCosDS;
        vPos.z = fY + fOffset;
        vPos.y = WMath::Sin(fDegree) * fRadius * fCosDS;

        AddVertex(trans, options, vPos, vPos.GetNormalized(), WVec2(0));
      }
    }
  }

  WUInt32 uiTopVertex = AddVertex(trans, options, WVec3(0, 0, fRadius + fHeight * 0.5f), WVec3(0, 0, 1), WVec2(0));
  WUInt32 uiBottomVertex = AddVertex(trans, options, WVec3(0, 0, -fRadius - fHeight * 0.5f), WVec3(0, 0, -1), WVec2(0));

  WUInt32 tri[3];
  WUInt32 quad[4];

  // now create the top cone
  for (WUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiTopVertex;
    tri[2] = uiFirstVertex + ((p + 1) % uiSegments);
    tri[1] = uiFirstVertex + p;

    AddPolygon(tri, bFlipWinding);
  }

  // now create the stacks in the middle
  WUInt16 uiMaxStacks = static_cast<WUInt16>(uiStacks * 2 - 1);
  for (WUInt16 st = 0; st < uiMaxStacks; ++st)
  {
    const WUInt32 uiRowBottom = uiSegments * st;
    const WUInt32 uiRowTop = uiSegments * (st + 1);

    for (WInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + ((i + 1) % uiSegments));
      quad[3] = uiFirstVertex + (uiRowTop + i);
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[1] = uiFirstVertex + (uiRowBottom + ((i + 1) % uiSegments));

      AddPolygon(quad, bFlipWinding);
    }
  }

  const WInt32 iBottomStack = uiSegments * (uiStacks * 2 - 1);

  // now create the bottom cone
  for (WUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiBottomVertex;
    tri[2] = uiFirstVertex + (iBottomStack + p);
    tri[1] = uiFirstVertex + (iBottomStack + ((p + 1) % uiSegments));

    AddPolygon(tri, bFlipWinding);
  }
}

void WGeometry::AddTorus(float fInnerRadius, float fOuterRadius, WUInt16 uiSegments, WUInt16 uiSegmentDetail, bool bExtraVerticesForTexturing, const GeoOptions& options)
{
  uiSegments = WMath::Max<WUInt16>(uiSegments, 3u);
  uiSegmentDetail = WMath::Max<WUInt16>(uiSegmentDetail, 3u);
  fOuterRadius = WMath::Max(fInnerRadius + 0.01f, fOuterRadius);

  const WQuat tilt = WBasisAxis::GetBasisRotation(options.m_MainAxis, WBasisAxis::PositiveZ);
  const WMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const float fCylinderRadius = (fOuterRadius - fInnerRadius) * 0.5f;
  const float fLoopRadius = fInnerRadius + fCylinderRadius;

  const WAngle fAngleStepSegment = WAngle::MakeFromDegree(360.0f / uiSegments);
  const WAngle fAngleStepCylinder = WAngle::MakeFromDegree(360.0f / uiSegmentDetail);

  const WUInt16 uiFirstVertex = static_cast<WUInt16>(m_Vertices.GetCount());

  const WUInt16 uiNumSegments = bExtraVerticesForTexturing ? uiSegments + 1 : uiSegments;
  const WUInt16 uiNumSegmentDetail = bExtraVerticesForTexturing ? uiSegmentDetail + 1 : uiSegmentDetail;

  // this is the loop for the torus ring
  for (WUInt16 seg = 0; seg < uiNumSegments; ++seg)
  {
    float fU = ((float)seg / (float)uiSegments) * 2.0f;

    const WAngle fAngle = float(seg) * fAngleStepSegment;

    const float fSinAngle = WMath::Sin(fAngle);
    const float fCosAngle = WMath::Cos(fAngle);

    const WVec3 vLoopPos = WVec3(fSinAngle, fCosAngle, 0) * fLoopRadius;

    // this is the loop to go round the cylinder
    for (WUInt16 p = 0; p < uiNumSegmentDetail; ++p)
    {
      float fV = (float)p / (float)uiSegmentDetail;

      const WAngle fCylinderAngle = float(p) * fAngleStepCylinder;

      const WVec3 vDir(WMath::Cos(fCylinderAngle) * fSinAngle, WMath::Cos(fCylinderAngle) * fCosAngle, WMath::Sin(fCylinderAngle));

      const WVec3 vPos = vLoopPos + fCylinderRadius * vDir;

      AddVertex(trans, options, vPos, vDir, WVec2(fU, fV));
    }
  }

  if (bExtraVerticesForTexturing)
  {
    for (WUInt16 seg = 0; seg < uiSegments; ++seg)
    {
      const WUInt16 rs0 = uiFirstVertex + seg * (uiSegmentDetail + 1);
      const WUInt16 rs1 = uiFirstVertex + (seg + 1) * (uiSegmentDetail + 1);

      for (WUInt16 p = 0; p < uiSegmentDetail; ++p)
      {
        WUInt32 quad[4];
        quad[0] = rs1 + p;
        quad[3] = rs1 + p + 1;
        quad[2] = rs0 + p + 1;
        quad[1] = rs0 + p;

        AddPolygon(quad, bFlipWinding);
      }
    }
  }
  else
  {
    WUInt16 prevRing = (uiSegments - 1);

    for (WUInt16 seg = 0; seg < uiSegments; ++seg)
    {
      const WUInt16 thisRing = seg;

      const WUInt16 prevRingFirstVtx = uiFirstVertex + (prevRing * uiSegmentDetail);
      WUInt16 prevRingPrevVtx = prevRingFirstVtx + (uiSegmentDetail - 1);

      const WUInt16 thisRingFirstVtx = uiFirstVertex + (thisRing * uiSegmentDetail);
      WUInt16 thisRingPrevVtx = thisRingFirstVtx + (uiSegmentDetail - 1);

      for (WUInt16 p = 0; p < uiSegmentDetail; ++p)
      {
        const WUInt16 prevRingThisVtx = prevRingFirstVtx + p;
        const WUInt16 thisRingThisVtx = thisRingFirstVtx + p;

        WUInt32 quad[4];

        quad[0] = prevRingPrevVtx;
        quad[1] = prevRingThisVtx;
        quad[2] = thisRingThisVtx;
        quad[3] = thisRingPrevVtx;

        AddPolygon(quad, bFlipWinding);

        prevRingPrevVtx = prevRingThisVtx;
        thisRingPrevVtx = thisRingThisVtx;
      }

      prevRing = thisRing;
    }
  }
}

void WGeometry::AddTexturedRamp(const WVec3& vSize, const GeoOptions& options)
{
  const WVec3 halfSize = vSize * 0.5f;
  const bool bFlipWinding = options.IsFlipWindingNecessary();
  WUInt32 idx[4];
  WUInt32 idx3[3];

  {
    WVec3 vNormal = WVec3(-halfSize.z, 0, halfSize.x).GetNormalized();
    idx[0] = AddVertex(options, WVec3(-halfSize.x, -halfSize.y, -halfSize.z), vNormal, WVec2(0, 1));
    idx[1] = AddVertex(options, WVec3(+halfSize.x, -halfSize.y, +halfSize.z), vNormal, WVec2(0, 0));
    idx[2] = AddVertex(options, WVec3(+halfSize.x, +halfSize.y, +halfSize.z), vNormal, WVec2(1, 0));
    idx[3] = AddVertex(options, WVec3(-halfSize.x, +halfSize.y, -halfSize.z), vNormal, WVec2(1, 1));
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(options, WVec3(-halfSize.x, +halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(1, 0));
    idx[1] = AddVertex(options, WVec3(+halfSize.x, +halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(1, 1));
    idx[2] = AddVertex(options, WVec3(+halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0, 1));
    idx[3] = AddVertex(options, WVec3(-halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, 0, -1), WVec2(0, 0));
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(options, WVec3(+halfSize.x, +halfSize.y, -halfSize.z), WVec3(1, 0, 0), WVec2(0, 1));
    idx[1] = AddVertex(options, WVec3(+halfSize.x, +halfSize.y, +halfSize.z), WVec3(1, 0, 0), WVec2(0, 0));
    idx[2] = AddVertex(options, WVec3(+halfSize.x, -halfSize.y, +halfSize.z), WVec3(1, 0, 0), WVec2(1, 0));
    idx[3] = AddVertex(options, WVec3(+halfSize.x, -halfSize.y, -halfSize.z), WVec3(1, 0, 0), WVec2(1, 1));
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx3[0] = AddVertex(options, WVec3(+halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, -1, 0), WVec2(0, 1));
    idx3[1] = AddVertex(options, WVec3(+halfSize.x, -halfSize.y, +halfSize.z), WVec3(0, -1, 0), WVec2(0, 0));
    idx3[2] = AddVertex(options, WVec3(-halfSize.x, -halfSize.y, -halfSize.z), WVec3(0, -1, 0), WVec2(1, 1));
    AddPolygon(idx3, bFlipWinding);
  }

  {
    idx3[0] = AddVertex(options, WVec3(-halfSize.x, +halfSize.y, -halfSize.z), WVec3(0, +1, 0), WVec2(0, 1));
    idx3[1] = AddVertex(options, WVec3(+halfSize.x, +halfSize.y, +halfSize.z), WVec3(0, +1, 0), WVec2(1, 0));
    idx3[2] = AddVertex(options, WVec3(+halfSize.x, +halfSize.y, -halfSize.z), WVec3(0, +1, 0), WVec2(1, 1));
    AddPolygon(idx3, bFlipWinding);
  }
}

void WGeometry::AddStairs(const WVec3& vSize, WUInt32 uiNumSteps, WAngle curvature, bool bSmoothSloped, const GeoOptions& options)
{
  const bool bFlipWinding = options.IsFlipWindingNecessary();

  curvature = WMath::Clamp(curvature, -WAngle::MakeFromDegree(360), WAngle::MakeFromDegree(360));
  const WAngle curveStep = curvature / (float)uiNumSteps;

  const float fStepDiv = 1.0f / uiNumSteps;
  const float fStepDepth = vSize.x / uiNumSteps;
  const float fStepHeight = vSize.z / uiNumSteps;

  WVec3 vMoveFwd(fStepDepth, 0, 0);
  const WVec3 vMoveUp(0, 0, fStepHeight);
  WVec3 vMoveUpFwd(fStepDepth, 0, fStepHeight);

  WVec3 vBaseL0(-vSize.x * 0.5f, -vSize.y * 0.5f, -vSize.z * 0.5f);
  WVec3 vBaseL1(-vSize.x * 0.5f, +vSize.y * 0.5f, -vSize.z * 0.5f);
  WVec3 vBaseR0 = vBaseL0 + vMoveFwd;
  WVec3 vBaseR1 = vBaseL1 + vMoveFwd;

  WVec3 vTopL0 = vBaseL0 + vMoveUp;
  WVec3 vTopL1 = vBaseL1 + vMoveUp;
  WVec3 vTopR0 = vBaseR0 + vMoveUp;
  WVec3 vTopR1 = vBaseR1 + vMoveUp;

  WVec3 vPrevTopR0 = vBaseL0;
  WVec3 vPrevTopR1 = vBaseL1;

  float fTexU0 = 0;
  float fTexU1 = fStepDiv;

  WVec3 vSideNormal0(0, 1, 0);
  WVec3 vSideNormal1(0, 1, 0);
  WVec3 vStepFrontNormal(-1, 0, 0);

  WQuat qRot = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), curveStep);

  for (WUInt32 step = 0; step < uiNumSteps; ++step)
  {
    {
      const WVec3 vAvg = (vTopL0 + vTopL1 + vTopR0 + vTopR1) / 4.0f;

      vTopR0 = vAvg + qRot * (vTopR0 - vAvg);
      vTopR1 = vAvg + qRot * (vTopR1 - vAvg);
      vBaseR0 = vAvg + qRot * (vBaseR0 - vAvg);
      vBaseR1 = vAvg + qRot * (vBaseR1 - vAvg);

      vMoveFwd = qRot * vMoveFwd;
      vMoveUpFwd = vMoveFwd;
      vMoveUpFwd.z = fStepHeight;

      vSideNormal1 = qRot * vSideNormal1;
    }

    if (bSmoothSloped)
    {
      // don't care about exact normals for the top surfaces
      vTopL0 = vPrevTopR0;
      vTopL1 = vPrevTopR1;
    }

    WUInt32 poly[4];

    // top
    poly[0] = AddVertex(options, vTopL0, WVec3(0, 0, 1), WVec2(fTexU0, 0));
    poly[3] = AddVertex(options, vTopL1, WVec3(0, 0, 1), WVec2(fTexU0, 1));
    poly[1] = AddVertex(options, vTopR0, WVec3(0, 0, 1), WVec2(fTexU1, 0));
    poly[2] = AddVertex(options, vTopR1, WVec3(0, 0, 1), WVec2(fTexU1, 1));
    AddPolygon(poly, bFlipWinding);

    // bottom
    poly[0] = AddVertex(options, vBaseL0, WVec3(0, 0, -1), WVec2(fTexU0, 0));
    poly[1] = AddVertex(options, vBaseL1, WVec3(0, 0, -1), WVec2(fTexU0, 1));
    poly[3] = AddVertex(options, vBaseR0, WVec3(0, 0, -1), WVec2(fTexU1, 0));
    poly[2] = AddVertex(options, vBaseR1, WVec3(0, 0, -1), WVec2(fTexU1, 1));
    AddPolygon(poly, bFlipWinding);

    // step front
    if (!bSmoothSloped)
    {
      poly[0] = AddVertex(options, vPrevTopR0, WVec3(-1, 0, 0), WVec2(0, fTexU0));
      poly[3] = AddVertex(options, vPrevTopR1, WVec3(-1, 0, 0), WVec2(1, fTexU0));
      poly[1] = AddVertex(options, vTopL0, WVec3(-1, 0, 0), WVec2(0, fTexU1));
      poly[2] = AddVertex(options, vTopL1, WVec3(-1, 0, 0), WVec2(1, fTexU1));
      AddPolygon(poly, bFlipWinding);
    }

    // side 1
    poly[0] = AddVertex(options, vBaseL0, -vSideNormal0, WVec2(fTexU0, 0));
    poly[1] = AddVertex(options, vBaseR0, -vSideNormal1, WVec2(fTexU1, 0));
    poly[3] = AddVertex(options, vTopL0, -vSideNormal0, WVec2(fTexU0, fTexU1));
    poly[2] = AddVertex(options, vTopR0, -vSideNormal1, WVec2(fTexU1, fTexU1));
    AddPolygon(poly, bFlipWinding);

    // side 2
    poly[0] = AddVertex(options, vBaseL1, vSideNormal0, WVec2(fTexU0, 0));
    poly[3] = AddVertex(options, vBaseR1, vSideNormal1, WVec2(fTexU1, 0));
    poly[1] = AddVertex(options, vTopL1, vSideNormal0, WVec2(fTexU0, fTexU1));
    poly[2] = AddVertex(options, vTopR1, vSideNormal1, WVec2(fTexU1, fTexU1));
    AddPolygon(poly, bFlipWinding);

    vPrevTopR0 = vTopR0;
    vPrevTopR1 = vTopR1;

    vBaseL0 = vBaseR0;
    vBaseL1 = vBaseR1;
    vBaseR0 += vMoveFwd;
    vBaseR1 += vMoveFwd;

    vTopL0 = vTopR0 + vMoveUp;
    vTopL1 = vTopR1 + vMoveUp;
    vTopR0 += vMoveUpFwd;
    vTopR1 += vMoveUpFwd;

    fTexU0 = fTexU1;
    fTexU1 += fStepDiv;

    vSideNormal0 = vSideNormal1;
    vStepFrontNormal = qRot * vStepFrontNormal;
  }

  // back
  {
    WUInt32 poly[4];
    poly[0] = AddVertex(options, vBaseL0, -vStepFrontNormal, WVec2(0, 0));
    poly[1] = AddVertex(options, vBaseL1, -vStepFrontNormal, WVec2(1, 0));
    poly[3] = AddVertex(options, vPrevTopR0, -vStepFrontNormal, WVec2(0, 1));
    poly[2] = AddVertex(options, vPrevTopR1, -vStepFrontNormal, WVec2(1, 1));
    AddPolygon(poly, bFlipWinding);
  }
}

void WGeometry::AddArch(const WVec3& vSize0, WUInt32 uiNumSegments, float fThickness, WAngle angle, bool bMakeSteps, bool bSmoothBottom, bool bSmoothTop, bool bCapTopAndBottom, const GeoOptions& options)
{
  const WQuat tilt = WBasisAxis::GetBasisRotation(options.m_MainAxis, WBasisAxis::PositiveZ);
  const WMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const WVec3 vSize = tilt * vSize0;

  // sanitize input values
  {
    if (angle.GetRadian() == 0.0f)
      angle = WAngle::MakeFromDegree(360);

    angle = WMath::Clamp(angle, WAngle::MakeFromDegree(-360.0f), WAngle::MakeFromDegree(360.0f));

    fThickness = WMath::Clamp(fThickness, 0.01f, WMath::Min(vSize.x, vSize.y) * 0.45f);

    bSmoothBottom = bMakeSteps && bSmoothBottom;
    bSmoothTop = bMakeSteps && bSmoothTop;
  }

  bool bFlipWinding = options.IsFlipWindingNecessary();

  if (angle.GetRadian() < 0)
    bFlipWinding = !bFlipWinding;

  const WAngle angleStep = angle / (float)uiNumSegments;
  const float fScaleX = vSize.x * 0.5f;
  const float fScaleY = vSize.y * 0.5f;
  const float fHalfHeight = vSize.z * 0.5f;
  const float fStepHeight = vSize.z / (float)uiNumSegments;

  float fBottomZ = -fHalfHeight;
  float fTopZ = +fHalfHeight;

  if (bMakeSteps)
  {
    fTopZ = fBottomZ + fStepHeight;
  }

  // mutable variables
  WAngle nextAngle;
  WVec3 vCurDirOutwards, vNextDirOutwards;
  WVec3 vCurBottomOuter, vCurBottomInner, vCurTopOuter, vCurTopInner;
  WVec3 vNextBottomOuter, vNextBottomInner, vNextTopOuter, vNextTopInner;

  // Setup first round
  {
    vNextDirOutwards.Set(WMath::Cos(nextAngle), WMath::Sin(nextAngle), 0);
    vNextBottomOuter.Set(WMath::Cos(nextAngle) * fScaleX, WMath::Sin(nextAngle) * fScaleY, fBottomZ);
    vNextTopOuter.Set(vNextBottomOuter.x, vNextBottomOuter.y, fTopZ);

    const WVec3 vNextThickness = vNextDirOutwards * fThickness;
    vNextBottomInner = vNextBottomOuter - vNextThickness;
    vNextTopInner = vNextTopOuter - vNextThickness;

    if (bSmoothBottom)
    {
      vNextBottomInner.z += fStepHeight * 0.5f;
      vNextBottomOuter.z += fStepHeight * 0.5f;
    }

    if (bSmoothTop)
    {
      vNextTopInner.z += fStepHeight * 0.5f;
      vNextTopOuter.z += fStepHeight * 0.5f;
    }
  }

  const bool isFullCircle = WMath::Abs(angle.GetRadian()) >= WAngle::MakeFromDegree(360).GetRadian();

  const float fOuterUstep = 3.0f / uiNumSegments;
  for (WUInt32 segment = 0; segment < uiNumSegments; ++segment)
  {
    // step values
    {
      nextAngle = angleStep * (segment + 1.0f);

      vCurDirOutwards = vNextDirOutwards;

      vCurBottomOuter = vNextBottomOuter;
      vCurBottomInner = vNextBottomInner;
      vCurTopOuter = vNextTopOuter;
      vCurTopInner = vNextTopInner;

      vNextDirOutwards.Set(WMath::Cos(nextAngle), WMath::Sin(nextAngle), 0);

      vNextBottomOuter.Set(vNextDirOutwards.x * fScaleX, vNextDirOutwards.y * fScaleY, fBottomZ);
      vNextTopOuter.Set(vNextBottomOuter.x, vNextBottomOuter.y, fTopZ);

      const WVec3 vNextThickness = vNextDirOutwards * fThickness;
      vNextBottomInner = vNextBottomOuter - vNextThickness;
      vNextTopInner = vNextTopOuter - vNextThickness;

      if (bSmoothBottom)
      {
        vCurBottomInner.z -= fStepHeight;
        vCurBottomOuter.z -= fStepHeight;

        vNextBottomInner.z += fStepHeight * 0.5f;
        vNextBottomOuter.z += fStepHeight * 0.5f;
      }

      if (bSmoothTop)
      {
        vCurTopInner.z -= fStepHeight;
        vCurTopOuter.z -= fStepHeight;

        vNextTopInner.z += fStepHeight * 0.5f;
        vNextTopOuter.z += fStepHeight * 0.5f;
      }
    }

    const float fCurOuterU = segment * fOuterUstep;
    const float fNextOuterU = (1 + segment) * fOuterUstep;

    WUInt32 poly[4];

    // Outside
    {
      poly[0] = AddVertex(trans, options, vCurBottomOuter, vCurDirOutwards, WVec2(fCurOuterU, 0));
      poly[1] = AddVertex(trans, options, vNextBottomOuter, vNextDirOutwards, WVec2(fNextOuterU, 0));
      poly[3] = AddVertex(trans, options, vCurTopOuter, vCurDirOutwards, WVec2(fCurOuterU, 1));
      poly[2] = AddVertex(trans, options, vNextTopOuter, vNextDirOutwards, WVec2(fNextOuterU, 1));
      AddPolygon(poly, bFlipWinding);
    }

    // Inside
    {
      poly[0] = AddVertex(trans, options, vCurBottomInner, -vCurDirOutwards, WVec2(fCurOuterU, 0));
      poly[3] = AddVertex(trans, options, vNextBottomInner, -vNextDirOutwards, WVec2(fNextOuterU, 0));
      poly[1] = AddVertex(trans, options, vCurTopInner, -vCurDirOutwards, WVec2(fCurOuterU, 1));
      poly[2] = AddVertex(trans, options, vNextTopInner, -vNextDirOutwards, WVec2(fNextOuterU, 1));
      AddPolygon(poly, bFlipWinding);
    }

    // Bottom
    if (bCapTopAndBottom)
    {
      poly[0] = AddVertex(trans, options, vCurBottomInner, WVec3(0, 0, -1), vCurBottomInner.GetAsVec2());
      poly[1] = AddVertex(trans, options, vNextBottomInner, WVec3(0, 0, -1), vNextBottomInner.GetAsVec2());
      poly[3] = AddVertex(trans, options, vCurBottomOuter, WVec3(0, 0, -1), vCurBottomOuter.GetAsVec2());
      poly[2] = AddVertex(trans, options, vNextBottomOuter, WVec3(0, 0, -1), vNextBottomOuter.GetAsVec2());
      AddPolygon(poly, bFlipWinding);
    }

    // Top
    if (bCapTopAndBottom)
    {
      poly[0] = AddVertex(trans, options, vCurTopInner, WVec3(0, 0, 1), vCurTopInner.GetAsVec2());
      poly[3] = AddVertex(trans, options, vNextTopInner, WVec3(0, 0, 1), vNextTopInner.GetAsVec2());
      poly[1] = AddVertex(trans, options, vCurTopOuter, WVec3(0, 0, 1), vCurTopOuter.GetAsVec2());
      poly[2] = AddVertex(trans, options, vNextTopOuter, WVec3(0, 0, 1), vNextTopOuter.GetAsVec2());
      AddPolygon(poly, bFlipWinding);
    }

    // Front
    if (bMakeSteps || (!isFullCircle && segment == 0))
    {
      const WVec3 vNormal = (bFlipWinding ? -1.0f : 1.0f) * vCurDirOutwards.CrossRH(WVec3(0, 0, 1));
      poly[0] = AddVertex(trans, options, vCurBottomInner, vNormal, WVec2(0, 0));
      poly[1] = AddVertex(trans, options, vCurBottomOuter, vNormal, WVec2(1, 0));
      poly[3] = AddVertex(trans, options, vCurTopInner, vNormal, WVec2(0, 1));
      poly[2] = AddVertex(trans, options, vCurTopOuter, vNormal, WVec2(1, 1));
      AddPolygon(poly, bFlipWinding);
    }

    // Back
    if (bMakeSteps || (!isFullCircle && segment == uiNumSegments - 1))
    {
      const WVec3 vNormal = (bFlipWinding ? -1.0f : 1.0f) * -vNextDirOutwards.CrossRH(WVec3(0, 0, 1));
      poly[0] = AddVertex(trans, options, vNextBottomInner, vNormal, WVec2(0, 0));
      poly[3] = AddVertex(trans, options, vNextBottomOuter, vNormal, WVec2(1, 0));
      poly[1] = AddVertex(trans, options, vNextTopInner, vNormal, WVec2(0, 1));
      poly[2] = AddVertex(trans, options, vNextTopOuter, vNormal, WVec2(1, 1));
      AddPolygon(poly, bFlipWinding);
    }

    if (bMakeSteps)
    {
      vNextTopOuter.z += fStepHeight;
      vNextTopInner.z += fStepHeight;
      vNextBottomOuter.z += fStepHeight;
      vNextBottomInner.z += fStepHeight;

      fBottomZ = fTopZ;
      fTopZ += fStepHeight;
    }
  }
}
