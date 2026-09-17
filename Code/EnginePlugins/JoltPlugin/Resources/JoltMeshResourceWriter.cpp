#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>

#include <Core/Graphics/ConvexHull.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/Progress.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltStreamUtils.h>

#define ENABLE_VHACD_IMPLEMENTATION 1
#include <VHACD/VHACD.h>
using namespace VHACD;

//////////////////////////////////////////////////////////////////////////

static constexpr WUInt8 uiColliderFileVersion = 4;

// static
WResult WJoltMeshResourceWriter::WriteMeshResource(const WJoltMeshDesc& meshDesc, WStreamWriter& inout_stream, bool bWriteAssetHeader /*= true*/, WUInt64 uiAssetHash /*= 0*/, WJoltCookedMeshStats* out_pStats /*= nullptr*/)
{
  WJoltCookedMeshStats stats;
  if (bWriteAssetHeader)
  {
    WAssetFileHeader header;
    header.SetFileHashAndVersion(uiAssetHash, 10); // WGetStaticRTTI<WJoltCollisionMeshAssetDocument>()->GetTypeVersion();
    W_SUCCEED_OR_RETURN(header.Write(inout_stream));
  }

  WUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  WCompressedStreamWriterZstd compressor(&inout_stream, 0, WCompressedStreamWriterZstd::Compression::Average);
  WChunkStreamWriter chunk(compressor);
#else
  WChunkStreamWriter chunk(inout_stream);
#endif

  inout_stream << uiColliderFileVersion;
  inout_stream << uiCompressionMode;
  inout_stream << meshDesc.m_uiContentHash;

  chunk.BeginStream(1);

  // Write chunks
  {
    {
      chunk.BeginChunk("Surfaces", 1);

      chunk << meshDesc.m_Surfaces.GetCount();

      for (const WString& sSurface : meshDesc.m_Surfaces)
      {
        chunk << sSurface;
      }

      chunk.EndChunk();
    }

    {
      chunk.BeginChunk("Details", 1);

      WBoundingBoxSphere aabb = WBoundingBoxSphere::MakeFromPoints(meshDesc.m_Vertices.GetData(), meshDesc.m_Vertices.GetCount());

      chunk << aabb;

      chunk.EndChunk();
    }

    WResult resCooking = W_FAILURE;

    if (meshDesc.m_Type == WJoltMeshDesc::Type::Triangle)
    {
      chunk.BeginChunk("TriangleMesh", 1);

      WStopwatch timer;
      resCooking = CookTriangleMesh(meshDesc, chunk);

      // A triangle mesh is cooked as it is, so the input counts are the output counts.
      stats.m_uiNumVertices = meshDesc.m_Vertices.GetCount();
      stats.m_uiNumTriangles = meshDesc.m_TriangleIndices.GetCount() / 3;
      WLog::Dev("Triangle Mesh Cooking time: {0}s", WArgF(timer.GetRunningTotal().GetSeconds(), 2));

      chunk.EndChunk();
    }
    else if (meshDesc.m_Type == WJoltMeshDesc::Type::ConvexHull)
    {
      chunk.BeginChunk("ConvexMesh", 1);

      WStopwatch timer;
      resCooking = CookConvexMesh(meshDesc, chunk, stats);
      WLog::Dev("Convex Mesh Cooking time: {0}s", WArgF(timer.GetRunningTotal().GetSeconds(), 2));

      chunk.EndChunk();
    }
    else if (meshDesc.m_Type == WJoltMeshDesc::Type::ConvexDecomposition)
    {
      chunk.BeginChunk("ConvexDecompositionMesh", 1);

      WStopwatch timer;
      resCooking = CookDecomposedConvexMesh(meshDesc, chunk, stats);
      WLog::Dev("Decomposed Convex Mesh Cooking time: {0}s", WArgF(timer.GetRunningTotal().GetSeconds(), 2));

      chunk.EndChunk();
    }
    else if (meshDesc.m_Type == WJoltMeshDesc::Type::ConvexHullGroup)
    {
      chunk.BeginChunk("ConvexDecompositionMesh", 1);

      WStopwatch timer;
      resCooking = CookConvexHullGroup(meshDesc, chunk, stats);
      WLog::Dev("Decomposed Convex Mesh Cooking time: {0}s", WArgF(timer.GetRunningTotal().GetSeconds(), 2));

      chunk.EndChunk();
    }

    if (resCooking.Failed())
    {
      WLog::Error("Cooking the collision mesh failed.");
      return W_FAILURE;
    }
  }

  chunk.EndStream();

  if (out_pStats != nullptr)
  {
    *out_pStats = stats;
  }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  if (compressor.FinishCompressedStream().Failed())
  {
    WLog::Error("Failed to finish compressing stream.");
    return W_FAILURE;
  }

  WLog::Dev("Compressed collision mesh data from {0} to {1} ({2}%%)", WArgFileSize(compressor.GetUncompressedSize()), WArgFileSize(compressor.GetCompressedSize()), WArgF(100.0f * compressor.GetCompressedSize() / compressor.GetUncompressedSize(), 1));

#endif

  return W_SUCCESS;
}

WResult WJoltMeshResourceWriter::ComputeConvexHull(const WDynamicArray<WVec3>& vertices, WDynamicArray<WVec3>& out_hullVertices)
{
  WStopwatch timer;

  WConvexHullGenerator gen;
  if (gen.Build(vertices).Failed())
  {
    WLog::Error("Computing the convex hull failed.");
    return W_FAILURE;
  }

  WDynamicArray<WConvexHullGenerator::Face> faces;
  gen.Retrieve(out_hullVertices, faces);

  if (faces.GetCount() >= 255)
  {
    WConvexHullGenerator gen2;
    gen2.SetSimplificationMinTriangleAngle(WAngle::MakeFromDegree(30));
    gen2.SetSimplificationFlatVertexNormalThreshold(WAngle::MakeFromDegree(10));
    gen2.SetSimplificationMinTriangleEdgeLength(0.08f);

    if (gen2.Build(out_hullVertices).Failed())
    {
      WLog::Error("Computing the convex hull failed (second try).");
      return W_FAILURE;
    }

    gen2.Retrieve(out_hullVertices, faces);
  }

  WLog::Dev("Computed the convex hull in {0} milliseconds", WArgF(timer.GetRunningTotal().GetMilliseconds(), 1));
  return W_SUCCESS;
}

WResult WJoltMeshResourceWriter::CookSingleConvexJoltMesh(const WDynamicArray<WVec3>& vertices, WStreamWriter& inout_stream, WJoltCookedMeshStats& ref_stats)
{
  if (JPH::Allocate == nullptr)
  {
    // make sure an allocator exists
    JPH::RegisterDefaultAllocator();
  }

  WTempHybridArray<JPH::Vec3, 256> verts;
  verts.SetCountUninitialized(vertices.GetCount());

  for (WUInt32 i = 0; i < verts.GetCount(); ++i)
  {
    verts[i] = WJoltConversionUtils::ToVec3(vertices[i]);
  }

  JPH::ConvexHullShapeSettings shapeSettings(verts.GetData(), (int)verts.GetCount());

  auto shapeRes = shapeSettings.Create();

  if (shapeRes.HasError())
  {
    WLog::Error("Cooking convex Jolt mesh failed: {}", shapeRes.GetError().c_str());
    return W_FAILURE;
  }

  WDefaultMemoryStreamStorage storage;
  WMemoryStreamWriter memWriter(&storage);

  WJoltStreamOut jOut(&memWriter);
  shapeRes.Get()->SaveBinaryState(jOut);

  inout_stream << storage.GetStorageSize32();
  storage.CopyToStream(inout_stream).AssertSuccess();

  const WUInt32 uiNumVertices = verts.GetCount();
  inout_stream << uiNumVertices;

  const WUInt32 uiNumTriangles = shapeRes.Get()->GetStats().mNumTriangles;
  inout_stream << uiNumTriangles;

  // Summed, because a decomposition or hull group cooks several shapes into one file.
  ref_stats.m_uiNumVertices += uiNumVertices;
  ref_stats.m_uiNumTriangles += uiNumTriangles;
  ref_stats.m_uiNumParts += 1;

  return W_SUCCESS;
}

WResult WJoltMeshResourceWriter::CookTriangleMesh(const WJoltMeshDesc& meshDesc, WStreamWriter& inout_stream)
{
  if (JPH::Allocate == nullptr)
  {
    // make sure an allocator exists
    JPH::RegisterDefaultAllocator();
  }

  JPH::VertexList vertexList;
  JPH::IndexedTriangleList triangleList;

  // copy vertices
  {
    vertexList.resize(meshDesc.m_Vertices.GetCount());
    for (WUInt32 i = 0; i < meshDesc.m_Vertices.GetCount(); ++i)
    {
      vertexList[i] = WJoltConversionUtils::ToFloat3(meshDesc.m_Vertices[i]);
    }
  }

  WUInt32 uiMaxMaterialIndex = 0;

  const WUInt32 uiTriCount = meshDesc.m_TriangleIndices.GetCount() / 3;

  // copy triangles
  {
    const bool bNoSurfaceIDs = meshDesc.m_TriangleSurfaceID.IsEmpty();
    if (bNoSurfaceIDs && uiTriCount > 0)
    {
      WLog::Warning("CookTriangleMesh: no triangle surface IDs provided. Using surface 0 for all {} triangles.", uiTriCount);
    }

    triangleList.reserve(bNoSurfaceIDs ? uiTriCount : meshDesc.m_TriangleSurfaceID.GetCount());
    const WUInt32 uiLoopCount = bNoSurfaceIDs ? uiTriCount : meshDesc.m_TriangleSurfaceID.GetCount();
    for (WUInt32 i = 0; i < uiLoopCount; ++i)
    {
      const WUInt32 uiMaterialID = bNoSurfaceIDs ? 0 : meshDesc.m_TriangleSurfaceID[i];
      if (uiMaterialID == 0xFFFF)
        continue;

      uiMaxMaterialIndex = WMath::Max(uiMaxMaterialIndex, uiMaterialID);

      const WUInt32 idx0 = meshDesc.m_TriangleIndices[i * 3 + 0];
      const WUInt32 idx1 = meshDesc.m_TriangleIndices[i * 3 + 1];
      const WUInt32 idx2 = meshDesc.m_TriangleIndices[i * 3 + 2];

      if (idx0 == idx1 || idx0 == idx2 || idx1 == idx2)
      {
        // triangle is degenerate, skip it
        continue;
      }

      const WVec3 v0 = WJoltConversionUtils::ToVec3(vertexList[idx0]);
      const WVec3 v1 = WJoltConversionUtils::ToVec3(vertexList[idx1]);
      const WVec3 v2 = WJoltConversionUtils::ToVec3(vertexList[idx2]);

      if (v0.IsEqual(v1, 0.001f) || v0.IsEqual(v2, 0.001f) || v1.IsEqual(v2, 0.001f))
      {
        // triangle is degenerate, skip it
        continue;
      }

      auto& triangle = triangleList.emplace_back();
      triangle.mMaterialIndex = uiMaterialID;
      triangle.mIdx[0] = idx0;
      triangle.mIdx[1] = idx1;
      triangle.mIdx[2] = idx2;
    }
  }

  // cook mesh (create Jolt shape, then save to binary stream)
  {
    JPH::MeshShapeSettings meshSettings(vertexList, triangleList);
    meshSettings.mMaterials.resize(uiMaxMaterialIndex + 1);

    auto shapeRes = meshSettings.Create();

    if (shapeRes.HasError())
    {
      WLog::Error("Cooking Jolt triangle mesh failed: {}", shapeRes.GetError().c_str());
      return W_FAILURE;
    }

    WDefaultMemoryStreamStorage storage;
    WMemoryStreamWriter memWriter(&storage);

    WJoltStreamOut jOut(&memWriter);
    shapeRes.Get()->SaveBinaryState(jOut);

    inout_stream << storage.GetStorageSize32();
    storage.CopyToStream(inout_stream).AssertSuccess();

    const WUInt32 uiNumVertices = static_cast<WUInt32>(vertexList.size());
    inout_stream << uiNumVertices;

    const WUInt32 uiNumTriangles = shapeRes.Get()->GetStats().mNumTriangles;
    inout_stream << uiNumTriangles;
  }

  return W_SUCCESS;
}

WResult WJoltMeshResourceWriter::CookConvexMesh(const WJoltMeshDesc& meshDesc, WStreamWriter& inout_stream, WJoltCookedMeshStats& ref_stats)
{
  WProgressRange range("Cooking Convex Mesh", 2, false);

  range.BeginNextStep("Computing Convex Hull");

  WTempHybridArray<WVec3, 256> hullVertices;
  W_SUCCEED_OR_RETURN(ComputeConvexHull(meshDesc.m_Vertices, hullVertices));

  range.BeginNextStep("Cooking Convex Hull");

  W_SUCCEED_OR_RETURN(CookSingleConvexJoltMesh(hullVertices, inout_stream, ref_stats));

  return W_SUCCESS;
}

WResult WJoltMeshResourceWriter::CookDecomposedConvexMesh(const WJoltMeshDesc& meshDesc, WStreamWriter& inout_stream, WJoltCookedMeshStats& ref_stats)
{
  W_LOG_BLOCK("Decomposing Mesh");

  IVHACD* pConDec = CreateVHACD();
  IVHACD::Parameters params;
  params.m_maxConvexHulls = WMath::Max(1u, meshDesc.m_uiMaxConvexPieces);

  if (meshDesc.m_uiMaxConvexPieces <= 2)
  {
    params.m_resolution = 10 * 10 * 10;
  }
  else if (meshDesc.m_uiMaxConvexPieces <= 5)
  {
    params.m_resolution = 20 * 20 * 20;
  }
  else if (meshDesc.m_uiMaxConvexPieces <= 10)
  {
    params.m_resolution = 40 * 40 * 40;
  }
  else if (meshDesc.m_uiMaxConvexPieces <= 25)
  {
    params.m_resolution = 60 * 60 * 60;
  }
  else if (meshDesc.m_uiMaxConvexPieces <= 50)
  {
    params.m_resolution = 80 * 80 * 80;
  }
  else
  {
    params.m_resolution = 100 * 100 * 100;
  }

  if (!pConDec->Compute(meshDesc.m_Vertices.GetData()->GetData(), meshDesc.m_Vertices.GetCount(), meshDesc.m_TriangleIndices.GetData(), meshDesc.m_TriangleSurfaceID.GetCount(), params))
  {
    WLog::Error("Failed to compute convex decomposition");
    return W_FAILURE;
  }

  WUInt16 uiNumParts = 0;

  for (WUInt32 i = 0; i < pConDec->GetNConvexHulls(); ++i)
  {
    IVHACD::ConvexHull ch;
    pConDec->GetConvexHull(i, ch);

    if (ch.m_triangles.empty())
      continue;

    ++uiNumParts;
  }

  WLog::Dev("Convex mesh parts: {}", uiNumParts);

  inout_stream << uiNumParts;

  WTempHybridArray<WVec3, 256> hullVertices;
  for (WUInt32 i = 0; i < pConDec->GetNConvexHulls(); ++i)
  {
    IVHACD::ConvexHull ch;
    pConDec->GetConvexHull(i, ch);

    if (ch.m_triangles.empty())
      continue;

    hullVertices.SetCount((WUInt32)ch.m_points.size());

    for (WUInt32 v = 0; v < (WUInt32)ch.m_points.size(); ++v)
    {
      hullVertices[v].Set((float)ch.m_points[v].mX, (float)ch.m_points[v].mY, (float)ch.m_points[v].mZ);
    }

    W_SUCCEED_OR_RETURN(CookSingleConvexJoltMesh(hullVertices, inout_stream, ref_stats));
  }

  return W_SUCCESS;
}

WResult WJoltMeshResourceWriter::CookConvexHullGroup(const WJoltMeshDesc& meshDesc, WStreamWriter& inout_stream, WJoltCookedMeshStats& ref_stats)
{
  WMap<WUInt16, WDynamicArray<WVec3>> parts;

  WUInt32 uiVertexIdx = 0;
  for (WUInt32 faceIdx = 0; faceIdx < meshDesc.m_TriangleSurfaceID.GetCount(); ++faceIdx)
  {
    const WUInt16 materialID = meshDesc.m_TriangleSurfaceID[faceIdx];

    if (materialID == 0xFFFF)
      continue;

    auto& meshPartVertices = parts[materialID];

    for (WUInt8 v = 0; v < 3; ++v)
    {
      const WUInt32 vtxIdx = meshDesc.m_TriangleIndices[uiVertexIdx++];

      meshPartVertices.PushBack(meshDesc.m_Vertices[vtxIdx]);
    }
  }

  WUInt16 uiNumParts = parts.GetCount();
  inout_stream << uiNumParts;

  for (auto it : parts)
  {
    const auto& meshPartVertices = it.Value();
    WTempHybridArray<WVec3, 256> hullVertices;
    W_SUCCEED_OR_RETURN(ComputeConvexHull(meshPartVertices, hullVertices));

    W_SUCCEED_OR_RETURN(CookSingleConvexJoltMesh(hullVertices, inout_stream, ref_stats));
  }

  return W_SUCCESS;
}

// static
WResult WJoltMeshResourceWriter::WriteHeightfieldResource(const WJoltHeightfieldWriteDesc& desc, WStreamWriter& inout_stream, bool bWriteAssetHeader /*= true*/, WUInt64 uiAssetHash /*= 0*/)
{
  if (JPH::Allocate == nullptr)
    JPH::RegisterDefaultAllocator();

  const WUInt32 N = desc.uiSizeX;
  if (N < 4 || (N % 2) != 0 || desc.uiSizeX != desc.uiSizeY || desc.heights.GetCount() != N * N)
  {
    WLog::Error("WriteHeightfieldResource: invalid grid dimensions (N={}, count={}).", N, desc.heights.GetCount());
    return W_FAILURE;
  }

  const bool bHasMaterials = !desc.surfacePaths.IsEmpty() && !desc.matIndices.IsEmpty();
  const WUInt32 uiCellCount = (N - 1) * (N - 1);

  if (bHasMaterials && desc.matIndices.GetCount() != uiCellCount)
  {
    WLog::Error("WriteHeightfieldResource: matIndices count ({}) must be (N-1)^2 = {}.", desc.matIndices.GetCount(), uiCellCount);
    return W_FAILURE;
  }

  // Row order must be flipped so that Jolt's row axis maps to +Y in WorldEngine space after the
  // +90° X rotation applied at body-creation time.
  WDynamicArray<float> flippedHeights;
  flippedHeights.SetCountUninitialized(N * N);
  for (WUInt32 row = 0; row < N; ++row)
  {
    const WUInt32 srcRow = N - 1 - row;
    for (WUInt32 col = 0; col < N; ++col)
      flippedHeights[row * N + col] = desc.heights[srcRow * N + col];
  }

  JPH::PhysicsMaterialList joltMaterials;
  if (bHasMaterials)
  {
    joltMaterials.resize(desc.surfacePaths.GetCount(), nullptr);
  }

  JPH::HeightFieldShapeSettings settings(
    flippedHeights.GetData(),
    JPH::Vec3(-desc.vHalfExtent.x, 0.0f, -desc.vHalfExtent.y),
    JPH::Vec3(2.0f * desc.vHalfExtent.x / static_cast<float>(N - 1), 1.0f, 2.0f * desc.vHalfExtent.y / static_cast<float>(N - 1)),
    N,
    bHasMaterials ? desc.matIndices.GetPtr() : nullptr,
    joltMaterials);

  JPH::ShapeSettings::ShapeResult result = settings.Create();
  if (result.HasError())
  {
    WLog::Error("WriteHeightfieldResource: Jolt cooking failed: {}", result.GetError().c_str());
    return W_FAILURE;
  }

  WDefaultMemoryStreamStorage shapeMem;
  {
    WMemoryStreamWriter memWriter(&shapeMem);
    WJoltStreamOut jOut(&memWriter);
    result.Get()->SaveBinaryState(jOut);
    if (jOut.IsFailed())
    {
      WLog::Error("WriteHeightfieldResource: failed to serialize Jolt shape.");
      return W_FAILURE;
    }
  }

  if (bWriteAssetHeader)
  {
    WAssetFileHeader header;
    header.SetFileHashAndVersion(uiAssetHash, 1);
    W_SUCCEED_OR_RETURN(header.Write(inout_stream));
  }

  WUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  WCompressedStreamWriterZstd compressor(&inout_stream, 0, WCompressedStreamWriterZstd::Compression::Average);
  WChunkStreamWriter chunk(compressor);
#else
  WChunkStreamWriter chunk(inout_stream);
#endif

  inout_stream << uiColliderFileVersion;
  inout_stream << uiCompressionMode;
  inout_stream << desc.uiContentHash;

  chunk.BeginStream(1);
  {
    chunk.BeginChunk("Surfaces", 1);
    const WUInt32 uiNumSurfaces = bHasMaterials ? desc.surfacePaths.GetCount() : 0u;
    chunk << uiNumSurfaces;
    for (WUInt32 i = 0; i < uiNumSurfaces; ++i)
      chunk << desc.surfacePaths[i];
    chunk.EndChunk();

    chunk.BeginChunk("Heightfield", 1);
    chunk << desc.uiCollisionLayer;
    const WUInt32 uiShapeDataSize = shapeMem.GetStorageSize32();
    chunk << uiShapeDataSize;
    shapeMem.CopyToStream(chunk).AssertSuccess();
    chunk.EndChunk();
  }
  chunk.EndStream();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  if (compressor.FinishCompressedStream().Failed())
  {
    WLog::Error("WriteHeightfieldResource: failed to finish compression.");
    return W_FAILURE;
  }
#endif

  return W_SUCCESS;
}
