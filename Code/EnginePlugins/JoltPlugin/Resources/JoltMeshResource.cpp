#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Shapes/Implementation/JoltCustomShapeInfo.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltStreamUtils.h>
#include <RendererCore/Meshes/CpuMeshResource.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WJoltMeshResource, 1, WRTTIDefaultAllocator<WJoltMeshResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WJoltMeshResource);
// clang-format on

WJoltMeshResource::WJoltMeshResource()
  : WResource(DoUpdate::OnMainThread, 1)
{
  m_Bounds = WBoundingBoxSphere::MakeFromCenterExtents(WVec3::MakeZero(), WVec3::MakeZero(), 0);

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(WJoltMeshResource);
}

WJoltMeshResource::~WJoltMeshResource() = default;

WResourceLoadDesc WJoltMeshResource::UnloadData(Unload WhatToUnload)
{
  for (auto pMesh : m_ConvexMeshesData)
  {
    if (pMesh != nullptr)
    {
      W_DEFAULT_DELETE(pMesh);
    }
  }

  for (auto pMesh : m_ConvexMeshInstances)
  {
    if (pMesh != nullptr)
    {
      pMesh->Release();
    }
  }

  if (m_pTriangleMeshInstance)
  {
    m_pTriangleMeshInstance->Release();
    m_pTriangleMeshInstance = nullptr;
  }

  m_ConvexMeshesData.Clear();
  m_ConvexMeshInstances.Clear();

  m_TriangleMeshData.Clear();
  m_TriangleMeshData.Compact();

  m_uiNumTriangles = 0;
  m_uiNumVertices = 0;

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  /// \todo Compute memory usage
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(WJoltMeshResource);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

static void ReadConvexMesh(WStreamReader& inout_stream, WDataBuffer* pBuffer)
{
  WUInt32 uiSize = 0;

  inout_stream >> uiSize;
  pBuffer->SetCountUninitialized(uiSize);
  W_VERIFY(inout_stream.ReadBytes(pBuffer->GetData(), uiSize) == uiSize, "Reading cooked convex mesh data failed.");
}

static void AddStats(WStreamReader& inout_stream, WUInt32& ref_uiVertices, WUInt32& ref_uiTriangles)
{
  WUInt32 verts = 0, tris = 0;

  inout_stream >> verts;
  inout_stream >> tris;

  ref_uiVertices += verts;
  ref_uiTriangles += tris;
}

WResourceLoadDesc WJoltMeshResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WJoltMeshResource::UpdateContent", GetResourceIdOrDescription());

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  m_uiNumTriangles = 0;
  m_uiNumVertices = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  // version specified in WJoltMeshResourceWriter::WriteMeshResource
  WUInt8 uiVersion = 0;
  WUInt8 uiCompressionMode = 0;

  if (AssetHash.GetFileVersion() >= 6) // asset document version, in version 6 the 'resource file format version' was added
  {
    *Stream >> uiVersion;
    *Stream >> uiCompressionMode;
  }

  if (uiVersion < 3)
  {
    // older cooked Jolt meshes are incompatible
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  if (uiVersion >= 4)
  {
    WUInt64 uiContentHash = 0;
    *Stream >> uiContentHash;
  }

  WStreamReader* pCompressor = Stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  WCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(Stream);
      pCompressor = &decompressorZstd;
      break;
#else
      WLog::Error("Collision mesh is compressed with zstandard, but support for this compressor is not compiled in.");
      res.m_State = WResourceState::LoadedResourceMissing;
      return res;
#endif

    default:
      WLog::Error("Collision mesh is compressed with an unknown algorithm.");
      res.m_State = WResourceState::LoadedResourceMissing;
      return res;
  }

  // load and create the Jolt mesh
  {
    WChunkStreamReader chunk(*pCompressor);
    chunk.SetEndChunkFileMode(WChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "Surfaces")
      {
        WUInt32 uiNumSurfaces = 0;
        chunk >> uiNumSurfaces;

        m_Surfaces.SetCount(uiNumSurfaces);
        WStringBuilder sTemp;

        for (WUInt32 surf = 0; surf < uiNumSurfaces; ++surf)
        {
          chunk >> sTemp;

          m_Surfaces[surf] = WResourceManager::LoadResource<WSurfaceResource>(sTemp);
        }
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "Details")
      {
        chunk >> m_Bounds;
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "TriangleMesh")
      {
        WUInt32 uiBufferSize = 0;
        chunk >> uiBufferSize;

        m_TriangleMeshData.SetCountUninitialized(uiBufferSize);
        chunk.ReadBytes(m_TriangleMeshData.GetData(), uiBufferSize);
        AddStats(chunk, m_uiNumVertices, m_uiNumTriangles);
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "ConvexMesh")
      {
        m_ConvexMeshesData.PushBack(W_DEFAULT_NEW(WDataBuffer));
        m_ConvexMeshInstances.SetCount(1);
        ReadConvexMesh(chunk, m_ConvexMeshesData.PeekBack());
        AddStats(chunk, m_uiNumVertices, m_uiNumTriangles);
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "ConvexDecompositionMesh")
      {
        WUInt16 uiNumParts = 0;
        chunk >> uiNumParts;

        m_ConvexMeshesData.Reserve(uiNumParts);
        m_ConvexMeshInstances.SetCount(uiNumParts);

        for (WUInt32 i = 0; i < uiNumParts; ++i)
        {
          m_ConvexMeshesData.PushBack(W_DEFAULT_NEW(WDataBuffer));
          ReadConvexMesh(chunk, m_ConvexMeshesData.PeekBack());
          AddStats(chunk, m_uiNumVertices, m_uiNumTriangles);
        }
      }

      chunk.NextChunk();
    }

    if (m_TriangleMeshData.IsEmpty() && m_ConvexMeshesData.IsEmpty())
    {
      WLog::Error("Could neither find a 'TriangleMesh' chunk, nor a 'ConvexMesh' chunk in the JoltMesh file '{0}'", GetResourceID());
    }

    chunk.EndStream();
  }

  res.m_State = WResourceState::Loaded;
  return res;
}

void WJoltMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WJoltMeshResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;

  out_NewMemoryUsage.m_uiMemoryCPU += m_Surfaces.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_TriangleMeshData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_ConvexMeshesData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_ConvexMeshInstances.GetHeapMemoryUsage();

  for (const auto pConvex : m_ConvexMeshesData)
  {
    if (pConvex)
    {
      out_NewMemoryUsage.m_uiMemoryCPU += pConvex->GetHeapMemoryUsage();
    }
  }
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WJoltMeshResource, WJoltMeshResourceDescriptor)
{
  // creates just an empty mesh

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}

struct ShapeTriangle
{
  WVec3 m_Vertices[3];
  const WSurfaceResource* m_pSurface = nullptr;
};

void RetrieveShapeTriangles(const JPH::Shape* pShape, WDynamicArray<ShapeTriangle>& out_triangles)
{
  constexpr int cMaxTriangles = 128;

  WStaticArray<WVec3, cMaxTriangles * 3> positionsTmp;
  positionsTmp.SetCountUninitialized(cMaxTriangles * 3);

  WStaticArray<const JPH::PhysicsMaterial*, cMaxTriangles> materialsTmp;
  materialsTmp.SetCountUninitialized(cMaxTriangles);

  JPH::Shape::GetTrianglesContext ctxt;

  pShape->GetTrianglesStart(ctxt, JPH::AABox::sBiggest(), pShape->GetCenterOfMass(), JPH::Quat::sIdentity(), JPH::Vec3(1, 1, 1));

  while (true)
  {
    const int triCount = pShape->GetTrianglesNext(ctxt, cMaxTriangles, reinterpret_cast<JPH::Float3*>(positionsTmp.GetData()), materialsTmp.GetData());

    if (triCount == 0)
      break;

    out_triangles.Reserve(out_triangles.GetCount() + triCount);

    for (int i = 0; i < triCount; ++i)
    {
      const WJoltMaterial* pMat = static_cast<const WJoltMaterial*>(materialsTmp[i]);

      auto& tri = out_triangles.ExpandAndGetRef();
      tri.m_pSurface = pMat ? pMat->m_pSurface : nullptr;
      tri.m_Vertices[0] = positionsTmp[i * 3 + 0];
      tri.m_Vertices[1] = positionsTmp[i * 3 + 1];
      tri.m_Vertices[2] = positionsTmp[i * 3 + 2];
    }
  }
}

WCpuMeshResourceHandle WJoltMeshResource::ConvertToCpuMesh() const
{
  WStringBuilder sCpuMeshName = GetResourceID();
  sCpuMeshName.AppendFormat("-({})", GetCurrentResourceChangeCounter());

  WCpuMeshResourceHandle hCpuMesh = WResourceManager::GetExistingResource<WCpuMeshResource>(sCpuMeshName);
  if (hCpuMesh.IsValid())
    return hCpuMesh;

  WMeshResourceDescriptor desc;
  desc.MeshBufferDesc().AddStream(WMeshVertexStreamType::Position);

  WDynamicArray<ShapeTriangle> triangles;
  triangles.Reserve(256);

  const WUInt32 uiConvexParts = GetNumConvexParts();
  {
    for (WUInt32 i = 0; i < uiConvexParts; ++i)
    {
      auto pShape = InstantiateConvexPart(i, 0, nullptr, 1);
      RetrieveShapeTriangles(pShape, triangles);
      pShape->Release();
    }
  }

  if (m_pTriangleMeshInstance != nullptr || !m_TriangleMeshData.IsEmpty())
  {
    auto pShape = InstantiateTriangleMesh(0, {});
    RetrieveShapeTriangles(pShape, triangles);
    pShape->Release();
  }

  if (triangles.IsEmpty())
    return {};

  // we do copy the surfaces over as "materials" here, but if anyone needs to render this mesh, they need to replace that with a proper material
  for (WUInt32 i = 0; i < m_Surfaces.GetCount(); ++i)
  {
    desc.SetMaterial(i, m_Surfaces[i].GetResourceID());
  }

  triangles.Sort([](const ShapeTriangle& a, const ShapeTriangle& b)
    { return a.m_pSurface < b.m_pSurface; });

  WDynamicArray<WVec3> positions;
  WDynamicArray<WUInt32> indices;
  WMap<WVec3, WUInt32> vertexToIndex;
  const WSurfaceResource* pLastSurface = triangles[0].m_pSurface;
  WUInt32 uiFirstTriangleOfCurrentSurface = 0;

  const WUInt32 uiNumTriangles = triangles.GetCount();
  for (WUInt32 i = 0; i < uiNumTriangles; ++i)
  {
    auto& triangle = triangles[i];

    for (WUInt32 v = 0; v < 3; ++v)
    {
      WUInt32 uiIndex;
      if (!vertexToIndex.TryGetValue(triangle.m_Vertices[v], uiIndex))
      {
        uiIndex = positions.GetCount();
        positions.PushBack(triangle.m_Vertices[v]);
        vertexToIndex.Insert(triangle.m_Vertices[v], uiIndex);
      }

      indices.PushBack(uiIndex);
    }

    if (triangle.m_pSurface != pLastSurface)
    {
      const WUInt32 uiSurfaceIndex = pLastSurface != nullptr ? m_Surfaces.IndexOf(pLastSurface->GetResourceHandle()) : 0;
      W_ASSERT_DEV(uiSurfaceIndex != WInvalidIndex, "Surface not found in surface array.");

      desc.AddSubMesh(i - uiFirstTriangleOfCurrentSurface, uiFirstTriangleOfCurrentSurface, uiSurfaceIndex);
      pLastSurface = triangle.m_pSurface;
      uiFirstTriangleOfCurrentSurface = i;
    }
  }

  const WUInt32 uiSurfaceIndex = pLastSurface != nullptr ? m_Surfaces.IndexOf(pLastSurface->GetResourceHandle()) : 0;
  W_ASSERT_DEV(uiSurfaceIndex != WInvalidIndex, "Surface not found in surface array.");

  desc.AddSubMesh(uiNumTriangles - uiFirstTriangleOfCurrentSurface, uiFirstTriangleOfCurrentSurface, uiSurfaceIndex);

  desc.MeshBufferDesc().AllocateStreams(positions.GetCount(), WGALPrimitiveTopology::Triangles, uiNumTriangles);
  desc.MeshBufferDesc().GetPositionData().CopyFrom(positions);

  if (desc.MeshBufferDesc().Uses32BitIndices())
  {
    desc.MeshBufferDesc().GetIndexBufferData() = indices.GetByteArrayPtr();
  }
  else
  {
    WDynamicArray<WUInt16> indices16;
    indices16.SetCountUninitialized(indices.GetCount());
    for (WUInt32 i = 0; i < indices.GetCount(); ++i)
    {
      indices16[i] = static_cast<WUInt16>(indices[i]);
    }
    desc.MeshBufferDesc().GetIndexBufferData() = indices16.GetByteArrayPtr();
  }

  desc.ComputeBounds();

  return WResourceManager::GetOrCreateResource<WCpuMeshResource>(sCpuMeshName, std::move(desc), GetResourceDescription());
}

JPH::Shape* WJoltMeshResource::InstantiateTriangleMesh(WUInt64 uiUserData, const WDynamicArray<const WJoltMaterial*>& materials) const
{
  if (m_pTriangleMeshInstance == nullptr)
  {
    W_ASSERT_DEV(!m_TriangleMeshData.IsEmpty(), "Jolt mesh resource doesn't contain a triangle mesh.");

    WRawMemoryStreamReader memReader(m_TriangleMeshData);

    WJoltStreamIn jStream(&memReader);
    auto shapeRes = JPH::Shape::sRestoreFromBinaryState(jStream);

    if (shapeRes.HasError())
    {
      W_REPORT_FAILURE("Failed to instantiate Jolt triangle mesh: {}", shapeRes.GetError().c_str());
      return nullptr;
    }

    if (jStream.IsFailed())
    {
      W_REPORT_FAILURE("Failed to read Jolt triangle mesh from stream.");
      return nullptr;
    }

    WTempHybridArray<JPH::PhysicsMaterialRefC, 32> materials;
    materials.SetCount(m_Surfaces.GetCount());

    for (WUInt32 i = 0; i < m_Surfaces.GetCount(); ++i)
    {
      if (!m_Surfaces[i].IsValid())
        continue;

      WResourceLock pSurf(m_Surfaces[i], WResourceAcquireMode::BlockTillLoaded_NeverFail);

      if (pSurf.GetAcquireResult() != WResourceAcquireResult::None)
      {
        const WJoltMaterial* pMat = static_cast<const WJoltMaterial*>(pSurf->m_pPhysicsMaterialJolt);

        materials[i] = pMat;
      }
      else
      {
        WLog::Warning("Surface resource '{}' not available.", m_Surfaces[i].GetResourceID());
      }
    }

    shapeRes.Get()->RestoreMaterialState(materials.GetData(), materials.GetCount());

    m_pTriangleMeshInstance = shapeRes.Get();
    m_pTriangleMeshInstance->AddRef();

    // not needed anymore
    m_TriangleMeshData.Clear();
    m_TriangleMeshData.Compact();
  }

  {
    WJoltCustomShapeInfo* pShapeDeco = new WJoltCustomShapeInfo(m_pTriangleMeshInstance);
    pShapeDeco->SetUserData(uiUserData);

    // only override the materials, if they differ
    if (materials.GetCount() == m_Surfaces.GetCount())
    {
      pShapeDeco->m_CustomMaterials.SetCount(materials.GetCount());
      for (WUInt32 i = 0; i < materials.GetCount(); ++i)
      {
        pShapeDeco->m_CustomMaterials[i] = materials[i];
      }
    }

    pShapeDeco->AddRef();
    return pShapeDeco;
  }
}

JPH::Shape* WJoltMeshResource::InstantiateConvexPart(WUInt32 uiPartIdx, WUInt64 uiUserData, const WJoltMaterial* pMaterial, float fDensity) const
{
  if (m_ConvexMeshInstances[uiPartIdx] == nullptr)
  {
    W_ASSERT_DEV(!m_ConvexMeshesData.IsEmpty(), "Jolt mesh resource doesn't contain any convex mesh.");

    WRawMemoryStreamReader memReader(*m_ConvexMeshesData[uiPartIdx]);

    WJoltStreamIn jStream(&memReader);
    auto shapeRes = JPH::Shape::sRestoreFromBinaryState(jStream);

    if (shapeRes.HasError())
    {
      W_REPORT_FAILURE("Failed to instantiate Jolt convex mesh: {}", shapeRes.GetError().c_str());
      return nullptr;
    }

    if (jStream.IsFailed())
    {
      W_REPORT_FAILURE("Failed to read Jolt convex mesh from stream.");
      return nullptr;
    }

    JPH::ConvexShape* pConvexShape = static_cast<JPH::ConvexShape*>(shapeRes.Get().GetPtr());
    pConvexShape->SetDensity(1.0f); // density will be multiplied by the decoration shape, so set the base value to 1

    WTempHybridArray<JPH::PhysicsMaterialRefC, 1> materials;
    materials.SetCount(m_Surfaces.GetCount());

    for (WUInt32 i = 0; i < m_Surfaces.GetCount(); ++i)
    {
      if (!m_Surfaces[i].IsValid())
        continue;

      WResourceLock pSurf(m_Surfaces[i], WResourceAcquireMode::BlockTillLoaded_NeverFail);

      if (pSurf.GetAcquireResult() == WResourceAcquireResult::Final)
      {
        const WJoltMaterial* pMat = static_cast<const WJoltMaterial*>(pSurf->m_pPhysicsMaterialJolt);
        materials[i] = pMat;
      }
      else
      {
        WLog::Warning("Surface for collision mesh was not available: '{}'", m_Surfaces[i].GetResourceID());
      }
    }

    if (materials.GetCount() > 1)
    {
      if (materials.GetCount() > uiPartIdx)
      {
        JPH::PhysicsMaterialRefC pMat = materials[uiPartIdx];
        materials.SetCount(1);
        materials[0] = pMat;
      }

      materials.SetCount(1);
    }

    W_ASSERT_DEBUG(materials.GetCount() <= 1, "Convex meshes should only have a single material. '{}' has {}", GetResourceIdOrDescription(), materials.GetCount());
    shapeRes.Get()->RestoreMaterialState(materials.GetData(), materials.GetCount());


    m_ConvexMeshInstances[uiPartIdx] = shapeRes.Get();
    m_ConvexMeshInstances[uiPartIdx]->AddRef();

    W_DEFAULT_DELETE(m_ConvexMeshesData[uiPartIdx]);
  }

  {
    WJoltCustomShapeInfo* pShapeDeco = new WJoltCustomShapeInfo(m_ConvexMeshInstances[uiPartIdx]);
    pShapeDeco->SetUserData(uiUserData);
    pShapeDeco->m_fDensity = fDensity;

    if (pMaterial && pMaterial->m_pSurface != nullptr)
    {
      pShapeDeco->m_CustomMaterials.SetCount(1);
      pShapeDeco->m_CustomMaterials[0] = pMaterial;
    }

    pShapeDeco->AddRef();
    return pShapeDeco;
  }
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Resources_JoltMeshResource);
