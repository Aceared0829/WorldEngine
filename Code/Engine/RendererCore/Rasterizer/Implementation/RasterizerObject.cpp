#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/Rasterizer/Thirdparty/VectorMath.h>

WMutex WRasterizerObject::s_Mutex;
WMap<WString, WSharedPtr<WRasterizerObject>> WRasterizerObject::s_Objects;

WRasterizerObject::WRasterizerObject() = default;
WRasterizerObject::~WRasterizerObject() = default;

#if W_ENABLED(W_RASTERIZER_SUPPORTED)

// needed for WHybridArray below
W_DEFINE_AS_POD_TYPE(__m128);

void WRasterizerObject::CreateMesh(const WGeometry& geo)
{
  WTempHybridArray<__m128, 64> vertices;
  vertices.Reserve(geo.GetPolygons().GetCount() * 4);

  Aabb bounds;

  auto addVtx = [&](WVec3 vtxPos)
  {
    WSimdVec4f v;
    v.Load<4>(vtxPos.GetAsPositionVec4().GetData());
    vertices.PushBack(v.m_v);
  };

  for (const auto& poly : geo.GetPolygons())
  {
    const WUInt32 uiNumVertices = poly.m_Vertices.GetCount();
    WUInt32 uiQuadVtx = 0;

    // ignore complex polygons entirely
    if (uiNumVertices > 4)
      continue;

    for (WUInt32 i = 0; i < uiNumVertices; ++i)
    {
      if (uiQuadVtx == 4)
      {
        // TODO: restart next quad (also flip this one's front face)
        break;
      }

      const WUInt32 vtxIdx = poly.m_Vertices[i];

      addVtx(geo.GetVertices()[vtxIdx].m_vPosition);

      bounds.include(vertices.PeekBack());
      ++uiQuadVtx;
    }

    // if the polygon is a triangle, duplicate the last vertex to make it a degenerate quad
    if (uiQuadVtx == 3)
    {
      vertices.PushBack(vertices.PeekBack());
      ++uiQuadVtx;
    }

    if (uiQuadVtx == 4)
    {
      const WUInt32 n = vertices.GetCount();

      // swap two vertices in the quad to flip the front face (different convention between W and the rasterizer)
      WMath::Swap(vertices[n - 1], vertices[n - 3]);
    }

    W_ASSERT_DEV(uiQuadVtx == 4, "Degenerate polygon encountered");
  }

  // pad vertices to 32 for proper alignment during baking
  while (vertices.GetCount() % 32 != 0)
  {
    vertices.PushBack(vertices[0]);
  }

  m_Occluder.bake(vertices.GetData(), vertices.GetCount(), bounds.m_min, bounds.m_max);
}

WSharedPtr<const WRasterizerObject> WRasterizerObject::GetObject(WStringView sUniqueName)
{
  W_LOCK(s_Mutex);

  auto it = s_Objects.Find(sUniqueName);

  if (it.IsValid())
    return it.Value();

  return nullptr;
}

WSharedPtr<const WRasterizerObject> WRasterizerObject::CreateBox(const WVec3& vFullExtents)
{
  W_LOCK(s_Mutex);

  WStringBuilder sName;
  sName.SetFormat("Box-{}-{}-{}", vFullExtents.x, vFullExtents.y, vFullExtents.z);

  WSharedPtr<WRasterizerObject>& pObj = s_Objects[sName];

  if (pObj == nullptr)
  {
    pObj = W_NEW(WFoundation::GetAlignedAllocator(), WRasterizerObject);

    WGeometry geometry;
    geometry.AddBox(vFullExtents, false, {});

    pObj->CreateMesh(geometry);
  }

  return pObj;
}

WSharedPtr<const WRasterizerObject> WRasterizerObject::CreateQuadX(const WVec2& vYZExtents)
{
  W_LOCK(s_Mutex);

  WStringBuilder sName;
  sName.SetFormat("Quad-{}-{}", vYZExtents.x, vYZExtents.y);

  WSharedPtr<WRasterizerObject>& pObj = s_Objects[sName];

  if (pObj == nullptr)
  {
    pObj = W_NEW(WFoundation::GetAlignedAllocator(), WRasterizerObject);

    WGeometry::GeoOptions opt;
    opt.m_MainAxis = WBasisAxis::PositiveX;

    WGeometry geometry;
    geometry.AddRect(vYZExtents, 1, 1, opt);

    pObj->CreateMesh(geometry);
  }

  return pObj;
}

WSharedPtr<const WRasterizerObject> WRasterizerObject::CreateMesh(WStringView sUniqueName, const WGeometry& geometry)
{
  W_LOCK(s_Mutex);

  WSharedPtr<WRasterizerObject>& pObj = s_Objects[sUniqueName];

  if (pObj == nullptr)
  {
    pObj = W_NEW(WFoundation::GetAlignedAllocator(), WRasterizerObject);

    pObj->CreateMesh(geometry);
  }

  return pObj;
}

#else

void WRasterizerObject::CreateMesh(const WGeometry& geo)
{
}

WSharedPtr<const WRasterizerObject> WRasterizerObject::GetObject(WStringView sUniqueName)
{
  return nullptr;
}

WSharedPtr<const WRasterizerObject> WRasterizerObject::CreateBox(const WVec3& vFullExtents)
{
  return nullptr;
}

WSharedPtr<const WRasterizerObject> WRasterizerObject::CreateQuadX(const WVec2& vYZExtents)
{
  return nullptr;
}

WSharedPtr<const WRasterizerObject> WRasterizerObject::CreateMesh(WStringView sUniqueName, const WGeometry& geometry)
{
  return nullptr;
}

#endif
