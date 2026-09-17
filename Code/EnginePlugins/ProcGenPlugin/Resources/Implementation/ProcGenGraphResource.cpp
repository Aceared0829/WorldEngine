#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>

namespace WProcGenInternal
{
  extern Pattern* GetPattern(WProcPlacementPattern::Enum pattern);
}

using namespace WProcGenInternal;

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGenGraphResource, 1, WRTTIDefaultAllocator<WProcGenGraphResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WProcGenGraphResource);
// clang-format on

WProcGenGraphResource::WProcGenGraphResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WProcGenGraphResource::~WProcGenGraphResource() = default;

const WDynamicArray<WSharedPtr<const PlacementOutput>>& WProcGenGraphResource::GetPlacementOutputs() const
{
  return m_PlacementOutputs;
}

const WDynamicArray<WSharedPtr<const VertexColorOutput>>& WProcGenGraphResource::GetVertexColorOutputs() const
{
  return m_VertexColorOutputs;
}

WResourceLoadDesc WProcGenGraphResource::UnloadData(Unload WhatToUnload)
{
  m_PlacementOutputs.Clear();
  m_VertexColorOutputs.Clear();
  m_pSharedData = nullptr;

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WProcGenGraphResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WProcGenGraphResource::UpdateContent", GetResourceIdOrDescription());

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

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

  WUniquePtr<WStringDeduplicationReadContext> pStringDedupReadContext;
  if (AssetHash.GetFileVersion() >= 5)
  {
    pStringDedupReadContext = W_DEFAULT_NEW(WStringDeduplicationReadContext, *Stream);
  }

  // load
  {
    WChunkStreamReader chunk(*Stream);
    chunk.SetEndChunkFileMode(WChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    WStringBuilder sTemp;

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "SharedData")
      {
        WSharedPtr<GraphSharedData> pSharedData = W_DEFAULT_NEW(GraphSharedData);
        if (pSharedData->Load(chunk).Succeeded())
        {
          m_pSharedData = pSharedData;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "PlacementOutputs")
      {
        if (chunk.GetCurrentChunk().m_uiChunkVersion < 9)
        {
          WLog::Error("Invalid PlacementOutputs Chunk Version {0}. Expected >= 9", chunk.GetCurrentChunk().m_uiChunkVersion);
          chunk.NextChunk();
          continue;
        }

        WUInt32 uiNumOutputs = 0;
        chunk >> uiNumOutputs;

        m_PlacementOutputs.Reserve(uiNumOutputs);
        for (WUInt32 uiIndex = 0; uiIndex < uiNumOutputs; ++uiIndex)
        {
          WUniquePtr<WExpressionByteCode> pByteCode = W_DEFAULT_NEW(WExpressionByteCode);
          if (pByteCode->Load(chunk).Failed())
          {
            break;
          }

          WSharedPtr<PlacementOutput> pOutput = W_DEFAULT_NEW(PlacementOutput);
          pOutput->m_pByteCode = std::move(pByteCode);

          chunk >> pOutput->m_sName;
          chunk.ReadArray(pOutput->m_VolumeTagSetIndices).IgnoreResult();
          chunk.ReadArray(pOutput->m_CurveIndices).IgnoreResult();

          WUInt64 uiNumObjectsToPlace = 0;
          chunk >> uiNumObjectsToPlace;

          for (WUInt32 uiObjectIndex = 0; uiObjectIndex < static_cast<WUInt32>(uiNumObjectsToPlace); ++uiObjectIndex)
          {
            chunk >> sTemp;
            pOutput->m_ObjectsToPlace.ExpandAndGetRef() = WResourceManager::LoadResource<WPrefabResource>(sTemp);
          }

          chunk >> pOutput->m_fFootprint;

          chunk >> pOutput->m_vMinOffset;
          chunk >> pOutput->m_vMaxOffset;

          if (chunk.GetCurrentChunk().m_uiChunkVersion >= 6)
          {
            chunk >> pOutput->m_YawRotationSnap;
          }
          chunk >> pOutput->m_fAlignToNormal;

          chunk >> pOutput->m_vMinScale;
          chunk >> pOutput->m_vMaxScale;

          chunk >> pOutput->m_fCullDistance;

          chunk >> pOutput->m_uiCollisionLayer;

          chunk >> sTemp;
          if (!sTemp.IsEmpty())
          {
            pOutput->m_hColorGradient = WResourceManager::LoadResource<WColorGradientResource>(sTemp);
          }

          chunk >> sTemp;
          if (!sTemp.IsEmpty())
          {
            pOutput->m_hSurface = WResourceManager::LoadResource<WSurfaceResource>(sTemp);
          }

          if (chunk.GetCurrentChunk().m_uiChunkVersion >= 5)
          {
            chunk >> pOutput->m_Mode;
          }

          if (chunk.GetCurrentChunk().m_uiChunkVersion >= 8)
          {
            chunk >> pOutput->m_uiNumAdditionalRays;
            chunk >> pOutput->m_fRaySpread;
          }

          WEnum<WProcPlacementPattern> pattern = WProcPlacementPattern::RegularGrid;
          if (chunk.GetCurrentChunk().m_uiChunkVersion >= 7)
          {
            chunk >> pattern;
          }

          pOutput->m_pPattern = WProcGenInternal::GetPattern(pattern);

          m_PlacementOutputs.PushBack(pOutput);
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "VertexColorOutputs")
      {
        if (chunk.GetCurrentChunk().m_uiChunkVersion < 3)
        {
          WLog::Error("Invalid VertexColorOutputs Chunk Version {0}. Expected >= 3", chunk.GetCurrentChunk().m_uiChunkVersion);
          chunk.NextChunk();
          continue;
        }

        WUInt32 uiNumOutputs = 0;
        chunk >> uiNumOutputs;

        m_VertexColorOutputs.Reserve(uiNumOutputs);
        for (WUInt32 uiIndex = 0; uiIndex < uiNumOutputs; ++uiIndex)
        {
          WUniquePtr<WExpressionByteCode> pByteCode = W_DEFAULT_NEW(WExpressionByteCode);
          if (pByteCode->Load(chunk).Failed())
          {
            break;
          }

          WSharedPtr<VertexColorOutput> pOutput = W_DEFAULT_NEW(VertexColorOutput);
          pOutput->m_pByteCode = std::move(pByteCode);

          chunk >> pOutput->m_sName;
          chunk.ReadArray(pOutput->m_VolumeTagSetIndices).IgnoreResult();
          chunk.ReadArray(pOutput->m_CurveIndices).IgnoreResult();

          m_VertexColorOutputs.PushBack(pOutput);
        }
      }

      chunk.NextChunk();
    }

    chunk.EndStream();
    pStringDedupReadContext = nullptr;

    // link shared data
    if (m_pSharedData != nullptr)
    {
      for (auto& pPlacementOutput : m_PlacementOutputs)
      {
        const_cast<PlacementOutput*>(pPlacementOutput.Borrow())->m_pGraphSharedData = m_pSharedData;
      }

      for (auto& pVertexColorOutput : m_VertexColorOutputs)
      {
        const_cast<VertexColorOutput*>(pVertexColorOutput.Borrow())->m_pGraphSharedData = m_pSharedData;
      }
    }
  }

  res.m_State = WResourceState::Loaded;
  return res;
}

void WProcGenGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WProcGenGraphResource, WProcGenGraphResourceDescriptor)
{
  // W_REPORT_FAILURE("This resource type does not support creating data.");

  // Missing resource

  auto pOutput = W_DEFAULT_NEW(PlacementOutput);
  pOutput->m_sName.Assign("MissingPlacementOutput");
  pOutput->m_ObjectsToPlace.PushBack(WResourceManager::GetResourceTypeMissingFallback<WPrefabResource>());
  pOutput->m_pPattern = WProcGenInternal::GetPattern(WProcPlacementPattern::RegularGrid);
  pOutput->m_fFootprint = 3.0f;
  pOutput->m_vMinOffset.Set(-1.0f, -1.0f, -0.5f);
  pOutput->m_vMaxOffset.Set(1.0f, 1.0f, 0.0f);
  pOutput->m_vMinScale.Set(1.0f, 1.0f, 1.0f);
  pOutput->m_vMaxScale.Set(1.5f, 1.5f, 2.0f);

  m_PlacementOutputs.PushBack(pOutput);
  //

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}


W_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Resources_Implementation_ProcGenGraphResource);
