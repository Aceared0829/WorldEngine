#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/System/JoltDebugRenderer.h>

#ifdef JPH_DEBUG_RENDERER

#  include <Jolt/Renderer/DebugRenderer.h>

void WJoltDebugRenderer::TriangleBatch::AddRef()
{
  ++m_iRefCount;
}

void WJoltDebugRenderer::TriangleBatch::Release()
{
  --m_iRefCount;

  if (m_iRefCount == 0)
  {
    auto* pThis = this;
    W_DEFAULT_DELETE(pThis);
  }
}

WJoltDebugRenderer::WJoltDebugRenderer()
{
  Initialize();
}

void WJoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
  auto& l = m_Lines.ExpandAndGetRef();
  l.m_start = WJoltConversionUtils::ToVec3(inFrom);
  l.m_end = WJoltConversionUtils::ToVec3(inTo);
  l.m_startColor = WJoltConversionUtils::ToColor(inColor);
  l.m_endColor = l.m_startColor;
}


void WJoltDebugRenderer::DrawTriangle(JPH::Vec3Arg inV1, JPH::Vec3Arg inV2, JPH::Vec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow)
{
  auto& t = m_Triangles.ExpandAndGetRef();
  t.m_position[0] = WJoltConversionUtils::ToVec3(inV1);
  t.m_position[1] = WJoltConversionUtils::ToVec3(inV2);
  t.m_position[2] = WJoltConversionUtils::ToVec3(inV3);
  t.m_color = WJoltConversionUtils::ToColor(inColor);
}


JPH::DebugRenderer::Batch WJoltDebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Triangle* pInTriangles, int iInTriangleCount)
{
  TriangleBatch* pBatch = W_DEFAULT_NEW(TriangleBatch);
  pBatch->m_Triangles.Reserve(iInTriangleCount);

  for (int i = 0; i < iInTriangleCount; ++i)
  {
    auto& t = pBatch->m_Triangles.ExpandAndGetRef();
    t.m_position[0] = WJoltConversionUtils::ToVec3(pInTriangles[i].mV[0].mPosition);
    t.m_position[1] = WJoltConversionUtils::ToVec3(pInTriangles[i].mV[1].mPosition);
    t.m_position[2] = WJoltConversionUtils::ToVec3(pInTriangles[i].mV[2].mPosition);
    t.m_color = WJoltConversionUtils::ToColor(pInTriangles[i].mV[0].mColor);
  }

  return pBatch;
}


JPH::DebugRenderer::Batch WJoltDebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Vertex* pInVertices, int iInVertexCount, const JPH::uint32* pInIndices, int iInIndexCount)
{
  const WUInt32 numTris = iInIndexCount / 3;

  TriangleBatch* pBatch = W_DEFAULT_NEW(TriangleBatch);
  pBatch->m_Triangles.Reserve(numTris);

  WUInt32 index = 0;

  for (WUInt32 i = 0; i < numTris; ++i)
  {
    auto& t = pBatch->m_Triangles.ExpandAndGetRef();
    t.m_position[0] = WJoltConversionUtils::ToVec3(pInVertices[pInIndices[index + 0]].mPosition);
    t.m_position[1] = WJoltConversionUtils::ToVec3(pInVertices[pInIndices[index + 1]].mPosition);
    t.m_position[2] = WJoltConversionUtils::ToVec3(pInVertices[pInIndices[index + 2]].mPosition);
    t.m_color = WJoltConversionUtils::ToColor(pInVertices[pInIndices[index + 0]].mColor);

    index += 3;
  }

  return pBatch;
}


void WJoltDebugRenderer::DrawGeometry(JPH::Mat44Arg modelMatrix, const JPH::AABox& worldSpaceBounds, float fInLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& geometry, ECullMode inCullMode /*= ECullMode::CullBackFace*/, ECastShadow inCastShadow /*= ECastShadow::On*/, EDrawMode inDrawMode /*= EDrawMode::Solid*/)
{
  if (geometry == nullptr)
    return;

  WUInt32 uiLod = 0;
  if (geometry->mLODs.size() > 1)
    uiLod = 1;
  if (geometry->mLODs.size() > 2)
    uiLod = 2;

  const TriangleBatch* pBatch = static_cast<const TriangleBatch*>(geometry->mLODs[uiLod].mTriangleBatch.GetPtr());

  const WMat4 trans = reinterpret_cast<const WMat4&>(modelMatrix);
  const WColor color = WJoltConversionUtils::ToColor(inModelColor);

  if (inDrawMode == JPH::DebugRenderer::EDrawMode::Solid)
  {
    m_Triangles.Reserve(m_Triangles.GetCount() + pBatch->m_Triangles.GetCount() * ((inCullMode == JPH::DebugRenderer::ECullMode::Off) ? 2 : 1));

    if (inCullMode == JPH::DebugRenderer::ECullMode::CullBackFace || inCullMode == JPH::DebugRenderer::ECullMode::Off)
    {
      for (WUInt32 t = 0; t < pBatch->m_Triangles.GetCount(); ++t)
      {
        auto& tri = m_Triangles.ExpandAndGetRef();
        tri.m_color = pBatch->m_Triangles[t].m_color * color;
        tri.m_position[0] = trans * pBatch->m_Triangles[t].m_position[0];
        tri.m_position[1] = trans * pBatch->m_Triangles[t].m_position[1];
        tri.m_position[2] = trans * pBatch->m_Triangles[t].m_position[2];
      }
    }

    if (inCullMode == JPH::DebugRenderer::ECullMode::CullFrontFace || inCullMode == JPH::DebugRenderer::ECullMode::Off)
    {
      for (WUInt32 t = 0; t < pBatch->m_Triangles.GetCount(); ++t)
      {
        auto& tri = m_Triangles.ExpandAndGetRef();
        tri.m_color = pBatch->m_Triangles[t].m_color * color;
        tri.m_position[0] = trans * pBatch->m_Triangles[t].m_position[0];
        tri.m_position[1] = trans * pBatch->m_Triangles[t].m_position[2];
        tri.m_position[2] = trans * pBatch->m_Triangles[t].m_position[1];
      }
    }
  }
  else
  {
    m_Lines.Reserve(m_Lines.GetCount() + pBatch->m_Triangles.GetCount() * 3);

    for (WUInt32 t = 0; t < pBatch->m_Triangles.GetCount(); ++t)
    {
      const auto& inTri = pBatch->m_Triangles[t];
      const WColor col = pBatch->m_Triangles[t].m_color * color;

      const WVec3 v0 = trans * inTri.m_position[0];
      const WVec3 v1 = trans * inTri.m_position[1];
      const WVec3 v2 = trans * inTri.m_position[2];

      m_Lines.PushBack({v0, v1, col});
      m_Lines.PushBack({v1, v2, col});
      m_Lines.PushBack({v2, v0, col});
    }
  }
}

#endif


