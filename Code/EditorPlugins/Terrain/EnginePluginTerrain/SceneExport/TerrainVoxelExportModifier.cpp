#include <EnginePluginTerrain/EnginePluginTerrainPCH.h>

#include <EnginePluginTerrain/SceneExport/TerrainVoxelExportModifier.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>
#include <TerrainPlugin/Components/TerrainVolumeComponent.h>
#include <TerrainPlugin/TerrainSystem.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneExportModifier_TerrainVoxelCollision, 1, WRTTIDefaultAllocator<WSceneExportModifier_TerrainVoxelCollision>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static bool CheckExistingVoxelMeshFileContentHash(WStringView sPath, WUInt64 uiExpectedHash)
{
  WFileReader file;
  if (file.Open(sPath).Failed())
    return false;

  // JoltMeshResource files written via WriteMeshResource start with an asset file header,
  // followed by the version byte, compression mode, and content hash.
  WAssetFileHeader header;
  if (header.Read(file).Failed())
    return false;

  WUInt8 uiVersion = 0;
  WUInt8 uiCompressionMode = 0;
  file >> uiVersion;
  file >> uiCompressionMode;

  if (uiVersion < 4)
    return false;

  WUInt64 uiStoredHash = 0;
  file >> uiStoredHash;

  return uiStoredHash == uiExpectedHash;
}

void WSceneExportModifier_TerrainVoxelCollision::ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport)
{
  W_LOCK(ref_world.GetWriteMarker());

  WTerrainSystem* pTerrain = ref_world.GetOrCreateModule<WTerrainSystem>();
  if (pTerrain == nullptr)
    return;

  auto* pVolumeMan = ref_world.GetComponentManager<WTerrainVolumeComponentManager>();
  if (pVolumeMan == nullptr)
    return;

  auto* pActorMan = ref_world.GetOrCreateComponentManager<WJoltStaticActorComponentManager>();

  for (auto it = pVolumeMan->GetComponents(); it.IsValid(); ++it)
  {
    WTerrainVolumeComponent* pVolume = it;

    if (!pVolume->IsActive())
      continue;

    if (!pVolume->GetEnableCollider())
      continue;

    const WUInt32 uiVoxelIdx = pVolume->GetVoxelIndex();
    if (uiVoxelIdx == WInvalidIndex)
      continue;

    const WUInt64 uiContentHash = pVolume->ComputeColliderContentHash(pTerrain->GetVoxelBrushOverlapHash(uiVoxelIdx));

    WStringBuilder sPath;
    sPath.SetFormat(":project/AssetCache/Generated/TerrainVolume_{}.WBinJoltTriangleMesh", WArgU(pVolume->GetStableId(), 16, true, 16, true));

    const bool bFileUpToDate = CheckExistingVoxelMeshFileContentHash(sPath, uiContentHash);

    if (!bFileUpToDate)
    {
      WTempArray<VoxelGpuVertex> cpuVerts;
      WTempArray<WUInt32> cpuIdxs;
      WUInt32 uiVertCount = 0;
      WUInt32 uiTriCount = 0;
      if (pTerrain->ReadbackVoxelData(uiVoxelIdx, cpuVerts, cpuIdxs, uiVertCount, uiTriCount).Failed())
      {
        WLog::Warning("TerrainVoxelExportModifier: ReadbackVoxelData failed for volume (stableId={}), skipping baked collider.", pVolume->GetStableId());
        continue;
      }

      if (uiVertCount == 0 || uiTriCount == 0)
        continue;

      WJoltMeshDesc meshDesc;
      meshDesc.m_uiContentHash = uiContentHash;
      meshDesc.m_Type = WJoltMeshDesc::Type::Triangle;

      meshDesc.m_Vertices.SetCountUninitialized(uiVertCount);
      for (WUInt32 i = 0; i < uiVertCount; ++i)
      {
        meshDesc.m_Vertices[i] = cpuVerts[i].Position;
      }

      const WUInt32 uiIdxCount = uiTriCount * 3;
      meshDesc.m_TriangleIndices.SetCountUninitialized(uiIdxCount);
      WMemoryUtils::Copy(meshDesc.m_TriangleIndices.GetData(), cpuIdxs.GetData(), uiIdxCount);

      const WUInt32 uiNumSurfaces = pVolume->Surfaces_GetCount();
      if (uiNumSurfaces > 0)
      {
        meshDesc.m_Surfaces.SetCount(uiNumSurfaces);
        for (WUInt32 s = 0; s < uiNumSurfaces; ++s)
        {
          meshDesc.m_Surfaces[s] = pVolume->Surfaces_GetValue(s);
        }

        const WUInt8 uiFallback = pVolume->GetBaseMaterialIndex();
        meshDesc.m_TriangleSurfaceID.SetCountUninitialized(uiTriCount);

        for (WUInt32 tri = 0; tri < uiTriCount; ++tri)
        {
          WUInt8 votes[3];
          for (WUInt32 v = 0; v < 3; ++v)
          {
            const WUInt32 idx = cpuIdxs[tri * 3 + v];
            const VoxelGpuVertex& vert = cpuVerts[idx];
            const WUInt8 raw = (vert.Material != 0xFFFFFFFFu) ? static_cast<WUInt8>(vert.Material) : 0;
            votes[v] = (vert.MaterialStrength > 0.5f && raw < uiNumSurfaces) ? raw : uiFallback;
          }
          WUInt8 chosen = votes[0];
          if (votes[1] == votes[2] && votes[1] != votes[0])
          {
            chosen = votes[1];
          }

          meshDesc.m_TriangleSurfaceID[tri] = static_cast<WUInt16>(chosen);
        }
      }

      WDeferredFileWriter fileWriter;
      fileWriter.SetOutput(sPath);

      if (WJoltMeshResourceWriter::WriteMeshResource(meshDesc, fileWriter, true, 0).Failed())
      {
        WLog::Error("TerrainVoxelExportModifier: failed to cook mesh for '{}'.", sPath);
        continue;
      }

      if (fileWriter.Close().Failed())
      {
        WLog::Error("TerrainVoxelExportModifier: failed to write file '{}'.", sPath);
        continue;
      }
    }

    pVolume->SetEnableCollider(false);

    // Create or reuse the "VoxelCollider" child game object.
    WGameObject* pColliderObject = nullptr;
    WGameObject* pVolumeOwner = pVolume->GetOwner();
    for (auto childIt = pVolumeOwner->GetChildren(); childIt.IsValid(); ++childIt)
    {
      if (childIt->GetNameHashed() == WTempHashedString("VoxelCollider"))
      {
        pColliderObject = &(*childIt);
        break;
      }
    }

    if (pColliderObject == nullptr)
    {
      WGameObjectDesc objDesc;
      objDesc.m_sName.Assign("VoxelCollider");
      objDesc.m_hParent = pVolumeOwner->GetHandle();
      ref_world.CreateObject(objDesc, pColliderObject);
    }

    WJoltStaticActorComponent* pActor = nullptr;
    if (!pColliderObject->TryGetComponentOfBaseType(pActor))
    {
      pActorMan->CreateComponent(pColliderObject, pActor);
    }

    pActor->SetMesh(WResourceManager::LoadResource<WJoltMeshResource>(sPath));
  }
}

W_STATICLINK_FILE(EnginePluginTerrain, EnginePluginTerrain_SceneExport_TerrainVoxelExportModifier);
