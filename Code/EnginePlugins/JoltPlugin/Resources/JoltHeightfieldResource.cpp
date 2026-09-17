#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <JoltPlugin/Resources/JoltHeightfieldResource.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/Utilities/JoltStreamUtils.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WJoltHeightfieldResource, 1, WRTTIDefaultAllocator<WJoltHeightfieldResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WJoltHeightfieldResource);
// clang-format on

WJoltHeightfieldResource::WJoltHeightfieldResource()
  : WResource(DoUpdate::OnMainThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(WJoltHeightfieldResource);
}

WJoltHeightfieldResource::~WJoltHeightfieldResource() = default;

WResourceLoadDesc WJoltHeightfieldResource::UnloadData(Unload WhatToUnload)
{
  m_uiContentHash = 0;
  m_uiCollisionLayer = 0;
  m_Surfaces.Clear();
  m_ShapeData.Clear();
  m_ShapeData.Compact();

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(WJoltHeightfieldResource);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;
  return res;
}

WResourceLoadDesc WJoltHeightfieldResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WJoltHeightfieldResource::UpdateContent", GetResourceIdOrDescription());

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // resource loader prepends the absolute file path
  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  {
    WAssetFileHeader header;
    header.Read(*Stream).IgnoreResult();
  }

  WUInt8 uiVersion = 0;
  WUInt8 uiCompressionMode = 0;
  *Stream >> uiVersion;
  *Stream >> uiCompressionMode;
  *Stream >> m_uiContentHash;

  WStreamReader* pChunkSource = Stream;

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
      pChunkSource = &decompressorZstd;
      break;
#else
      WLog::Error("Heightfield file '{}' uses zstd compression but support is not compiled in.", GetResourceID());
      res.m_State = WResourceState::LoadedResourceMissing;
      return res;
#endif
    default:
      WLog::Error("Heightfield file '{}' uses unknown compression mode {}.", GetResourceID(), uiCompressionMode);
      res.m_State = WResourceState::LoadedResourceMissing;
      return res;
  }

  WChunkStreamReader chunk(*pChunkSource);
  chunk.SetEndChunkFileMode(WChunkStreamReader::EndChunkFileMode::JustClose);
  chunk.BeginStream();

  while (chunk.GetCurrentChunk().m_bValid)
  {
    if (chunk.GetCurrentChunk().m_sChunkName == "Surfaces")
    {
      WUInt32 uiNumSurfaces = 0;
      chunk >> uiNumSurfaces;
      m_Surfaces.SetCount(uiNumSurfaces);
      WStringBuilder sTemp;
      for (WUInt32 i = 0; i < uiNumSurfaces; ++i)
      {
        chunk >> sTemp;
        m_Surfaces[i] = WResourceManager::LoadResource<WSurfaceResource>(sTemp);
      }
    }

    if (chunk.GetCurrentChunk().m_sChunkName == "Heightfield")
    {
      chunk >> m_uiCollisionLayer;

      WUInt32 uiShapeDataSize = 0;
      chunk >> uiShapeDataSize;
      m_ShapeData.SetCountUninitialized(uiShapeDataSize);
      chunk.ReadBytes(m_ShapeData.GetData(), uiShapeDataSize);
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  if (m_ShapeData.IsEmpty())
  {
    WLog::Error("No 'Heightfield' chunk found in heightfield file '{}'", GetResourceID());
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  res.m_State = WResourceState::Loaded;
  return res;
}

void WJoltHeightfieldResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WJoltHeightfieldResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_Surfaces.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_ShapeData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WJoltHeightfieldResource, WJoltHeightfieldResourceDescriptor)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  const WUInt32 N = descriptor.m_uiResolution;
  const float halfX = descriptor.m_vHalfExtents.x;
  const float halfY = descriptor.m_vHalfExtents.y;

  if (N < 4 || (N % 2) != 0 || descriptor.m_Heights.GetCount() != N * N)
  {
    WLog::Error("WJoltHeightfieldResource: invalid height data (N={}, count={}).", N, descriptor.m_Heights.GetCount());
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  const WUInt32 uiCellCount = (N - 1) * (N - 1);
  const bool bHasMaterials = !descriptor.m_Surfaces.IsEmpty() && !descriptor.m_MaterialIndices.IsEmpty();

  if (bHasMaterials && descriptor.m_MaterialIndices.GetCount() != uiCellCount)
  {
    WLog::Error("WJoltHeightfieldResource: material indices count ({}) must be (N-1)^2 = {}.", descriptor.m_MaterialIndices.GetCount(), uiCellCount);
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  m_Surfaces = descriptor.m_Surfaces;
  m_uiCollisionLayer = descriptor.m_uiCollisionLayer;

  // Resolve surface handles to Jolt material pointers.
  WDynamicArray<const WJoltMaterial*> materialPtrs;
  materialPtrs.SetCount(descriptor.m_Surfaces.GetCount(), nullptr);
  for (WUInt32 i = 0; i < descriptor.m_Surfaces.GetCount(); ++i)
  {
    if (descriptor.m_Surfaces[i].IsValid())
    {
      WResourceLock<WSurfaceResource> pSurface(descriptor.m_Surfaces[i], WResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pSurface.GetAcquireResult() == WResourceAcquireResult::Final && pSurface->m_pPhysicsMaterialJolt != nullptr)
        materialPtrs[i] = reinterpret_cast<const WJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  // Row order must be flipped so that Jolt's row axis maps to +Y in WorldEngine space after the +90° X rotation.
  WDynamicArray<float> flippedSamples;
  flippedSamples.SetCountUninitialized(N * N);
  for (WUInt32 row = 0; row < N; ++row)
  {
    const WUInt32 srcRow = N - 1 - row;
    for (WUInt32 col = 0; col < N; ++col)
      flippedSamples[row * N + col] = descriptor.m_Heights[srcRow * N + col];
  }

  JPH::PhysicsMaterialList joltMaterials;
  if (bHasMaterials)
  {
    for (const WJoltMaterial* pMat : materialPtrs)
      joltMaterials.push_back(pMat != nullptr ? pMat : WJoltCore::GetDefaultMaterial());
  }

  JPH::HeightFieldShapeSettings settings(
    flippedSamples.GetData(),
    JPH::Vec3(-halfX, 0.0f, -halfY),
    JPH::Vec3(2.0f * halfX / static_cast<float>(N - 1), 1.0f, 2.0f * halfY / static_cast<float>(N - 1)),
    N,
    bHasMaterials ? descriptor.m_MaterialIndices.GetData() : nullptr,
    joltMaterials);

  JPH::ShapeSettings::ShapeResult result = settings.Create();
  if (result.HasError())
  {
    WLog::Error("WJoltHeightfieldResource: failed to create JPH::HeightFieldShape: {}", result.GetError().c_str());
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // Serialize the shape to binary state so OnSimulationStarted can use sRestoreFromBinaryState,
  // the same path used for file-loaded resources.
  WContiguousMemoryStreamStorage storage;
  WMemoryStreamWriter memWriter(&storage);
  WJoltStreamOut joltOut(&memWriter);
  result.Get()->SaveBinaryState(joltOut);

  m_ShapeData.SetCountUninitialized(storage.GetStorageSize32());
  WMemoryUtils::Copy(m_ShapeData.GetData(), storage.GetData(), storage.GetStorageSize32());

  res.m_State = WResourceState::Loaded;
  return res;
}

W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Resources_JoltHeightfieldResource);
