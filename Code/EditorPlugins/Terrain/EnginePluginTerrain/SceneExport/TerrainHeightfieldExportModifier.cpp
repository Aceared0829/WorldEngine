#include <EnginePluginTerrain/EnginePluginTerrainPCH.h>

#include <EnginePluginTerrain/SceneExport/TerrainHeightfieldExportModifier.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <JoltPlugin/Actors/JoltHeightfieldColliderComponent.h>
#include <JoltPlugin/Resources/JoltHeightfieldResource.h>
#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>
#include <TerrainPlugin/Components/TerrainPatchComponent.h>
#include <TerrainPlugin/TerrainSystem.h>
#include <meshoptimizer/meshoptimizer.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneExportModifier_TerrainHeightfieldCollision, 1, WRTTIDefaultAllocator<WSceneExportModifier_TerrainHeightfieldCollision>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static bool CheckExistingHeightfieldFileContentHash(WStringView sPath, WUInt64 uiExpectedHash)
{
  WFileReader file;
  if (file.Open(sPath).Failed())
    return false;

  WAssetFileHeader header;
  if (header.Read(file).Failed())
    return false;

  WUInt8 uiVersion = 0;
  WUInt8 uiCompressionMode = 0;
  file >> uiVersion;
  file >> uiCompressionMode;

  WUInt64 uiStoredHash = 0;
  file >> uiStoredHash;

  return uiStoredHash == uiExpectedHash;
}

/// Bump this when BuildOccluderMesh() changes, to discard cached meshes from the older algorithm.
static constexpr WUInt8 g_uiOccluderAlgorithmVersion = 1;
static constexpr WUInt8 g_uiOccluderFileVersion = 1;

/// Fails when the cache file doesn't exist or was built from different inputs.
static WResult ReadCachedOccluder(WStringView sPath, WUInt64 uiExpectedHash, WDynamicArray<WVec3>& out_vertices, WDynamicArray<WUInt32>& out_indices)
{
  WFileReader file;
  if (file.Open(sPath).Failed())
    return W_FAILURE;

  WUInt8 uiVersion = 0;
  file >> uiVersion;

  if (uiVersion != g_uiOccluderFileVersion)
    return W_FAILURE;

  WUInt64 uiStoredHash = 0;
  file >> uiStoredHash;

  if (uiStoredHash != uiExpectedHash)
    return W_FAILURE;

  W_SUCCEED_OR_RETURN(file.ReadArray(out_vertices));
  W_SUCCEED_OR_RETURN(file.ReadArray(out_indices));

  return W_SUCCESS;
}

static WResult WriteCachedOccluder(WStringView sPath, WUInt64 uiContentHash, const WDynamicArray<WVec3>& vertices, const WDynamicArray<WUInt32>& indices)
{
  WDeferredFileWriter file;
  file.SetOutput(sPath);

  file << g_uiOccluderFileVersion;
  file << uiContentHash;

  W_SUCCEED_OR_RETURN(file.WriteArray(vertices));
  W_SUCCEED_OR_RETURN(file.WriteArray(indices));

  return file.Close();
}

struct PatchGeometry
{
  WUInt32 uiVertexCount = 0; ///< Number of vertices per side of the collider grid
  float fJoltHalfExtent = 0.0f;
  float fColliderCenter = 0.0f;
  WUInt32 uiStoredStart = 0;
  WUInt32 uiSubsampleStride = 0; ///< Step size in full-res cells between collider vertices
};

static void ComputePatchGeometry(const WTerrainPatchComponent* pPatch, WUInt32 uiCellsPerSide, PatchGeometry& out_patchGeo)
{
  const float fFullGridSpacing = pPatch->GetSize() / static_cast<float>(uiCellsPerSide);
  out_patchGeo.uiSubsampleStride = static_cast<WUInt32>(pPatch->GetCollider().GetValue());
  const WUInt32 uiSubsampledCells = uiCellsPerSide / out_patchGeo.uiSubsampleStride;
  out_patchGeo.uiVertexCount = uiSubsampledCells + 2;
  const float fGridSpacing = fFullGridSpacing * static_cast<float>(out_patchGeo.uiSubsampleStride);
  out_patchGeo.fJoltHalfExtent = static_cast<float>(out_patchGeo.uiVertexCount - 1) * fGridSpacing * 0.5f;
  // iHalfExtra: how many extra subsampled vertices the collider grid extends beyond the render
  // patch on each side (subsampled coords). uiStoredStart adjusts from the stored border (offset 4)
  // so the collider vertices align with the center of the patch.
  const WInt32 iHalfExtra = static_cast<WInt32>((out_patchGeo.uiVertexCount - 1) * out_patchGeo.uiSubsampleStride - uiCellsPerSide) / 2;
  out_patchGeo.uiStoredStart = static_cast<WUInt32>(WMath::Max(0, 4 - iHalfExtra));
  const float fLocalStart = static_cast<float>(static_cast<WInt32>(out_patchGeo.uiStoredStart) - 4) * fFullGridSpacing;
  out_patchGeo.fColliderCenter = fLocalStart + out_patchGeo.fJoltHalfExtent;
}

/// Builds the coarse occluder mesh for one patch.
///
/// The heights are reduced to a grid of about fCellSize meters per cell, where every vertex takes the minimum
/// of all full resolution heights of the cells it belongs to. Since a triangle never rises above its highest
/// corner, the result stays below the real terrain, so it can't cull objects standing on it. Cells containing
/// carved (tunnel, cave) samples are dropped; the resulting holes can only lose occlusion, never add any.
///
/// Decimation is the one step that isn't strictly conservative: meshopt only removes vertices, so the surface
/// can rise across a narrow gully. The error budget (a tenth of the cell size) stays well below the pushdown
/// that the min filter already produces on any real slope (roughly cell size * tan(slope)).
static void BuildOccluderMesh(WUInt32 uiCellsPerSide, float fPatchSize, float fCellSize, WArrayPtr<const float> bakedHeights, WArrayPtr<const WUInt8> dominantMat, WDynamicArray<WVec3>& out_vertices, WDynamicArray<WUInt32>& out_indices)
{
  out_vertices.Clear();
  out_indices.Clear();

  const WInt32 iStoredRowStride = static_cast<WInt32>(uiCellsPerSide) + 9;
  const float fFullSpacing = fPatchSize / static_cast<float>(uiCellsPerSide);

  // Safety net only, the caller clamps to the collider grid spacing, which is coarser than this.
  fCellSize = WMath::Clamp(fCellSize, fFullSpacing, fPatchSize);
  const WUInt32 uiStride = WMath::Clamp(static_cast<WUInt32>(WMath::RoundToInt(fCellSize / fFullSpacing)), 1u, uiCellsPerSide);

  // Rounded up, so the grid spans the full patch. A stride that doesn't divide the patch evenly leaves the
  // last row and column narrower than the rest.
  const WUInt32 uiCells = (uiCellsPerSide + uiStride - 1) / uiStride;
  const WUInt32 uiVerts = uiCells + 1;

  const float fZSnap = fCellSize / 5.0f;
  const bool bHasDominantIndices = dominantMat.GetCount() == bakedHeights.GetCount();

  // Full resolution grid coordinate of each coarse grid line, clamped to the patch edge.
  WTempArray<WUInt32> lineToFine;
  lineToFine.SetCountUninitialized(uiVerts);
  for (WUInt32 i = 0; i < uiVerts; ++i)
  {
    lineToFine[i] = WMath::Min(i * uiStride, uiCellsPerSide);
  }

  WTempArray<float> heights;
  heights.SetCountUninitialized(uiVerts * uiVerts);

  for (WUInt32 cy = 0; cy < uiVerts; ++cy)
  {
    for (WUInt32 cx = 0; cx < uiVerts; ++cx)
    {
      // The stored grid has a 4 vertex border ring around the rendered patch, hence the +4.
      const WInt32 iCenterX = 4 + static_cast<WInt32>(lineToFine[cx]);
      const WInt32 iCenterY = 4 + static_cast<WInt32>(lineToFine[cy]);
      const WInt32 iRadius = static_cast<WInt32>(uiStride);

      float fMin = WMath::MaxValue<float>();
      bool bCarved = false;

      for (WInt32 dy = -iRadius; dy <= iRadius && !bCarved; ++dy)
      {
        const WInt32 iY = WMath::Clamp(iCenterY + dy, 0, iStoredRowStride - 1);

        for (WInt32 dx = -iRadius; dx <= iRadius; ++dx)
        {
          const WInt32 iX = WMath::Clamp(iCenterX + dx, 0, iStoredRowStride - 1);
          const WUInt32 uiIdx = static_cast<WUInt32>(iY * iStoredRowStride + iX);

          if (bHasDominantIndices && dominantMat[uiIdx] == 0xFFu)
          {
            bCarved = true;
            break;
          }

          fMin = WMath::Min(fMin, bakedHeights[uiIdx]);
        }
      }

      // Snapping downwards stays conservative and makes near-flat regions exactly planar, so that the
      // decimation can collapse them completely.
      if (!bCarved)
      {
        fMin = WMath::Floor(fMin / fZSnap) * fZSnap;
      }

      heights[cy * uiVerts + cx] = bCarved ? WMath::NaN<float>() : fMin;
    }
  }

  // Emit only the vertices that a kept cell actually uses.
  WTempArray<WUInt32> vertexRemap;
  vertexRemap.SetCount(uiVerts * uiVerts, WInvalidIndex);

  auto GetVertex = [&](WUInt32 x, WUInt32 y) -> WUInt32
  {
    WUInt32& uiRemapped = vertexRemap[y * uiVerts + x];

    if (uiRemapped == WInvalidIndex)
    {
      uiRemapped = out_vertices.GetCount();
      out_vertices.PushBack(WVec3(lineToFine[x] * fFullSpacing, lineToFine[y] * fFullSpacing, heights[y * uiVerts + x]));
    }

    return uiRemapped;
  };

  for (WUInt32 y = 0; y + 1 < uiVerts; ++y)
  {
    for (WUInt32 x = 0; x + 1 < uiVerts; ++x)
    {
      if (WMath::IsNaN(heights[y * uiVerts + x]) || WMath::IsNaN(heights[y * uiVerts + x + 1]) ||
          WMath::IsNaN(heights[(y + 1) * uiVerts + x]) || WMath::IsNaN(heights[(y + 1) * uiVerts + x + 1]))
        continue;

      const WUInt32 i00 = GetVertex(x, y);
      const WUInt32 i10 = GetVertex(x + 1, y);
      const WUInt32 i01 = GetVertex(x, y + 1);
      const WUInt32 i11 = GetVertex(x + 1, y + 1);

      // Counter-clockwise seen from +Z, so the faces point up and are backface culled from below.
      out_indices.PushBack(i00);
      out_indices.PushBack(i10);
      out_indices.PushBack(i11);

      out_indices.PushBack(i00);
      out_indices.PushBack(i11);
      out_indices.PushBack(i01);
    }
  }

  const WUInt32 uiGridTriangles = out_indices.GetCount() / 3;

  const float fError = fCellSize / 10.0f;

  if (uiGridTriangles > 0)
  {
    WTempArray<WUInt32> simplified;
    simplified.SetCountUninitialized(out_indices.GetCount());

    float fResultError = 0.0f;
    const size_t uiNumIndices = meshopt_simplify(simplified.GetData(), out_indices.GetData(), out_indices.GetCount(),
      &out_vertices[0].x, out_vertices.GetCount(), sizeof(WVec3), 0, fError, meshopt_SimplifyErrorAbsolute, &fResultError);

    simplified.SetCount(static_cast<WUInt32>(uiNumIndices));
    out_indices = simplified;

    // meshopt keeps referencing the original vertex buffer, so drop the vertices that no triangle uses anymore.
    WTempArray<WVec3> compacted;
    compacted.SetCountUninitialized(out_vertices.GetCount());

    const size_t uiNumVertices = meshopt_optimizeVertexFetch(compacted.GetData(), out_indices.GetData(), out_indices.GetCount(),
      out_vertices.GetData(), out_vertices.GetCount(), sizeof(WVec3));

    compacted.SetCount(static_cast<WUInt32>(uiNumVertices));
    out_vertices = compacted;

    WLog::Info("Terrain occluder: {} cells ({} m), {} -> {} triangles, error {} m", uiCells, WArgF(fCellSize, 2), uiGridTriangles, out_indices.GetCount() / 3, WArgF(fResultError, 3));
  }
}

void WSceneExportModifier_TerrainHeightfieldCollision::ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport)
{
  W_LOCK(ref_world.GetWriteMarker());

  WTerrainSystem* pTerrain = ref_world.GetOrCreateModule<WTerrainSystem>();
  if (pTerrain == nullptr)
    return;

  auto* pPatchMan = ref_world.GetComponentManager<WTerrainPatchComponentManager>();
  if (pPatchMan == nullptr)
    return;

  auto* pColliderMan = ref_world.GetOrCreateComponentManager<WJoltHeightfieldColliderComponentManager>();

  for (auto it = pPatchMan->GetComponents(); it.IsValid(); ++it)
  {
    WTerrainPatchComponent* pPatch = it;

    if (!pPatch->IsActive())
      continue;

    const bool bWantsCollider = pPatch->GetCollider() != WTerrainPatchColliderMode::None;
    const bool bWantsOccluder = pPatch->GetOcclusionCellSize() > 0.0f;

    if (!bWantsCollider)
    {
      if (bWantsOccluder)
      {
        WLog::Warning("TerrainHeightfieldExportModifier: patch (stableId={}) has an occlusion cell size but no collider. The occluder is baked from the collider data and is skipped as well.", pPatch->GetStableId());
      }

      continue;
    }

    const WUInt32 uiHeightfieldIdx = pPatch->GetHeightfieldIndex();
    if (uiHeightfieldIdx == WInvalidIndex)
      continue;

    const WUInt32 numCellsPerSide = pTerrain->GetHeightfieldCellsPerSide(uiHeightfieldIdx);
    const float fFullGridSpacing = pPatch->GetSize() / static_cast<float>(numCellsPerSide);

    PatchGeometry patchGeo;
    ComputePatchGeometry(pPatch, numCellsPerSide, patchGeo);

    const WUInt64 uiContentHash = pPatch->ComputeColliderContentHash(pTerrain->GetHeightfieldBrushOverlapHash(uiHeightfieldIdx));

    WStringBuilder sPath;
    sPath.SetFormat(":project/AssetCache/Generated/TerrainPatch_{}.WBinJoltHeightfield", WArgU(pPatch->GetStableId(), 16, true, 16, true));

    const bool bColliderFileUpToDate = CheckExistingHeightfieldFileContentHash(sPath, uiContentHash);

    // A detail level that's good enough for collision is also good enough for occlusion.
    const float fOccluderCellSize = WMath::Clamp(pPatch->GetOcclusionCellSize(), fFullGridSpacing * static_cast<float>(patchGeo.uiSubsampleStride), pPatch->GetSize());

    WStringBuilder sOccluderPath;
    WUInt64 uiOccluderHash = 0;
    bool bOccluderCacheUpToDate = false;

    WTempArray<WVec3> occluderVertices;
    WTempArray<WUInt32> occluderIndices;

    if (bWantsOccluder)
    {
      WHashStreamWriter64 hashWriter;
      hashWriter << uiContentHash;
      hashWriter << fOccluderCellSize;
      hashWriter << numCellsPerSide;
      hashWriter << g_uiOccluderAlgorithmVersion;
      uiOccluderHash = hashWriter.GetHashValue();

      sOccluderPath.SetFormat(":project/AssetCache/Generated/TerrainOccluder_{}.WBinTerrainOccluder", WArgU(pPatch->GetStableId(), 16, true, 16, true));

      bOccluderCacheUpToDate = ReadCachedOccluder(sOccluderPath, uiOccluderHash, occluderVertices, occluderIndices).Succeeded();
    }

    // One readback serves both, it forces a full re-bake of the patch.
    const bool bNeedsReadback = !bColliderFileUpToDate || (bWantsOccluder && !bOccluderCacheUpToDate);

    WTempArray<float> bakedHeights;
    WTempArray<WUInt8> dominantIndices;

    if (bNeedsReadback && pTerrain->ReadbackHeightfieldData(uiHeightfieldIdx, bakedHeights, dominantIndices).Failed())
    {
      WLog::Warning("TerrainHeightfieldExportModifier: ReadbackHeightfieldData failed for patch (stableId={}), skipping baked collider and occluder.", pPatch->GetStableId());
      continue;
    }

    if (bWantsOccluder)
    {
      if (!bOccluderCacheUpToDate)
      {
        BuildOccluderMesh(numCellsPerSide, pPatch->GetSize(), fOccluderCellSize, bakedHeights, dominantIndices, occluderVertices, occluderIndices);

        if (WriteCachedOccluder(sOccluderPath, uiOccluderHash, occluderVertices, occluderIndices).Failed())
        {
          // Not fatal, only costs bake time on the next run.
          WLog::Warning("TerrainHeightfieldExportModifier: failed to write occluder cache '{}'.", sOccluderPath);
        }
      }

      pPatch->SetBakedOccluder(occluderVertices, occluderIndices);
    }

    if (!bColliderFileUpToDate)
    {
      const WUInt32 uiStoredRowStride = numCellsPerSide + 9;
      const bool bHasDominantIndices = dominantIndices.GetCount() == bakedHeights.GetCount();
      constexpr float fJoltNoCollision = std::numeric_limits<float>::max();

      WTempArray<float> heights;
      heights.SetCountUninitialized(patchGeo.uiVertexCount * patchGeo.uiVertexCount);
      for (WUInt32 row = 0; row < patchGeo.uiVertexCount; ++row)
      {
        const WUInt32 srcRow = WMath::Min(patchGeo.uiStoredStart + row * patchGeo.uiSubsampleStride, uiStoredRowStride - 1);
        for (WUInt32 col = 0; col < patchGeo.uiVertexCount; ++col)
        {
          const WUInt32 srcCol = WMath::Min(patchGeo.uiStoredStart + col * patchGeo.uiSubsampleStride, uiStoredRowStride - 1);
          const WUInt32 srcIdx = srcRow * uiStoredRowStride + srcCol;

          if (bHasDominantIndices && dominantIndices[srcIdx] == 0xFFu)
          {
            heights[row * patchGeo.uiVertexCount + col] = fJoltNoCollision;
          }
          else
          {
            heights[row * patchGeo.uiVertexCount + col] = bakedHeights[srcIdx];
          }
        }
      }

      WTempArray<WUInt8> matIndices;
      WTempArray<WString> surfacePaths;

      const WUInt32 uiNumSurfaces = pPatch->Surfaces_GetCount();
      if (uiNumSurfaces > 0 && bHasDominantIndices)
      {
        surfacePaths.SetCount(uiNumSurfaces);

        for (WUInt32 surfaceIdx = 0; surfaceIdx < uiNumSurfaces; ++surfaceIdx)
        {
          surfacePaths[surfaceIdx] = pPatch->Surfaces_GetValue(surfaceIdx);
        }

        const WUInt32 uiNumQuads = (patchGeo.uiVertexCount - 1) * (patchGeo.uiVertexCount - 1);
        matIndices.SetCountUninitialized(uiNumQuads);
        for (WUInt32 row = 0; row < patchGeo.uiVertexCount - 1; ++row)
        {
          const WUInt32 srcSubRow = patchGeo.uiVertexCount - 1 - row;
          const WUInt32 srcFullRow = WMath::Min(patchGeo.uiStoredStart + srcSubRow * patchGeo.uiSubsampleStride, uiStoredRowStride - 1);
          for (WUInt32 col = 0; col < patchGeo.uiVertexCount - 1; ++col)
          {
            const WUInt32 srcFullCol = WMath::Min(patchGeo.uiStoredStart + col * patchGeo.uiSubsampleStride, uiStoredRowStride - 1);
            const WUInt8 matIdx = dominantIndices[srcFullRow * uiStoredRowStride + srcFullCol];
            matIndices[row * (patchGeo.uiVertexCount - 1) + col] = (matIdx < uiNumSurfaces) ? matIdx : 0;
          }
        }
      }

      WJoltHeightfieldWriteDesc desc;
      desc.uiContentHash = uiContentHash;
      desc.uiSizeX = patchGeo.uiVertexCount;
      desc.uiSizeY = patchGeo.uiVertexCount;
      desc.vHalfExtent = WVec2(patchGeo.fJoltHalfExtent, patchGeo.fJoltHalfExtent);
      desc.heights = heights.GetArrayPtr();
      desc.matIndices = matIndices.GetArrayPtr();
      desc.surfacePaths = surfacePaths.GetArrayPtr();
      desc.uiCollisionLayer = 0;

      {
        WDeferredFileWriter fileWriter;
        fileWriter.SetOutput(sPath);
        if (WJoltMeshResourceWriter::WriteHeightfieldResource(desc, fileWriter).Failed() || fileWriter.Close().Failed())
        {
          WLog::Error("TerrainHeightfieldExportModifier: failed to write '{}'.", sPath);
          continue;
        }
      }
    }

    pPatch->SetCollider(WTerrainPatchColliderMode::None);

    // Create or reuse the "HeightfieldCollider" child game object at the correct local offset.
    WGameObject* pColliderObject = nullptr;
    WGameObject* pPatchOwner = pPatch->GetOwner();
    for (auto childIt = pPatchOwner->GetChildren(); childIt.IsValid(); ++childIt)
    {
      if (childIt->GetNameHashed() == WTempHashedString("HeightfieldCollider"))
      {
        pColliderObject = &(*childIt);
        break;
      }
    }

    if (pColliderObject == nullptr)
    {
      WGameObjectDesc objDesc;
      objDesc.m_sName.Assign("HeightfieldCollider");
      objDesc.m_hParent = pPatchOwner->GetHandle();
      objDesc.m_LocalPosition = WVec3(patchGeo.fColliderCenter, patchGeo.fColliderCenter, 0.0f);
      ref_world.CreateObject(objDesc, pColliderObject);
    }
    else
    {
      pColliderObject->SetLocalPosition(WVec3(patchGeo.fColliderCenter, patchGeo.fColliderCenter, 0.0f));
    }

    WJoltHeightfieldColliderComponent* pCollider = nullptr;
    if (!pColliderObject->TryGetComponentOfBaseType(pCollider))
    {
      pColliderMan->CreateComponent(pColliderObject, pCollider);
    }

    pCollider->m_hHeightfield = WResourceManager::LoadResource<WJoltHeightfieldResource>(sPath);
  }
}

W_STATICLINK_FILE(EnginePluginTerrain, EnginePluginTerrain_SceneExport_TerrainHeightfieldExportModifier);
