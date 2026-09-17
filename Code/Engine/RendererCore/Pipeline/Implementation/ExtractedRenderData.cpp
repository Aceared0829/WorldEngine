#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

WExtractedRenderData::WExtractedRenderData() = default;

WExtractedRenderData::~WExtractedRenderData()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  for (auto& dataPerCategory : m_DataPerCategory)
  {
    pDevice->DestroyBuffer(dataPerCategory.m_hDataOffsetsBuffer);
  }
}

void WExtractedRenderData::AddSamplerBinding(WTempHashedString sSlotName, WGALSamplerStateHandle hSampler)
{
  if (hSampler.IsInvalidated())
    return;
  WSamplerBinding& sampler = m_SamplerBindings.ExpandAndGetRef();
  sampler.m_sSlotName = sSlotName;
  sampler.m_Sampler.m_hSampler = hSampler;
}

void WExtractedRenderData::AddBufferBinding(WTempHashedString sSlotName, WGALBufferHandle hBuffer, WGALBufferRange bufferRange, WEnum<WGALResourceFormat> overrideTexelBufferFormat)
{
  if (hBuffer.IsInvalidated())
    return;
  WBufferBinding& buffer = m_BufferBindings.ExpandAndGetRef();
  buffer.m_sSlotName = sSlotName;
  buffer.m_Buffer.m_hBuffer = hBuffer;
  buffer.m_Buffer.m_BufferRange = bufferRange;
  buffer.m_Buffer.m_OverrideTexelBufferFormat = overrideTexelBufferFormat;
}


void WExtractedRenderData::AddTextureBinding(WTempHashedString sSlotName, WGALTextureHandle hTexture, WGALTextureRange textureRange, WEnum<WGALResourceFormat> overrideViewFormat, WEnum<WGALTextureType> overrideViewType)
{
  if (hTexture.IsInvalidated())
    return;
  WTextureBinding& texture = m_TextureBindings.ExpandAndGetRef();
  texture.m_sSlotName = sSlotName;
  texture.m_Texture.m_hTexture = hTexture;
  texture.m_Texture.m_TextureRange = textureRange;
  texture.m_Texture.m_OverrideViewFormat = overrideViewFormat;
  texture.m_Texture.m_OverrideViewType = overrideViewType;
}

void WExtractedRenderData::AddTextureBinding(WTempHashedString sSlotName, const WTexture2DResourceHandle& hTexture, WResourceAcquireMode acquireMode, WGALTextureRange textureRange, WEnum<WGALResourceFormat> overrideViewFormat, WEnum<WGALTextureType> overrideViewType)
{
  if (hTexture.IsValid())
  {
    WResourceLock<WTexture2DResource> pTexture(hTexture, acquireMode);
    AddTextureBinding(sSlotName, pTexture->GetGALTexture(), textureRange, overrideViewFormat, overrideViewType);
    AddSamplerBinding(sSlotName, pTexture->GetGALSamplerState());
  }
}

void WExtractedRenderData::AddTextureBinding(WTempHashedString sSlotName, const WTexture3DResourceHandle& hTexture, WResourceAcquireMode acquireMode, WGALTextureRange textureRange, WEnum<WGALResourceFormat> overrideViewFormat, WEnum<WGALTextureType> overrideViewType)
{
  if (hTexture.IsValid())
  {
    WResourceLock<WTexture3DResource> pTexture(hTexture, acquireMode);
    AddTextureBinding(sSlotName, pTexture->GetGALTexture(), textureRange, overrideViewFormat, overrideViewType);
    AddSamplerBinding(sSlotName, pTexture->GetGALSamplerState());
  }
}

void WExtractedRenderData::AddTextureBinding(WTempHashedString sSlotName, const WTextureCubeResourceHandle& hTexture, WResourceAcquireMode acquireMode, WGALTextureRange textureRange, WEnum<WGALResourceFormat> overrideViewFormat, WEnum<WGALTextureType> overrideViewType)
{
  if (hTexture.IsValid())
  {
    WResourceLock<WTextureCubeResource> pTexture(hTexture, acquireMode);
    AddTextureBinding(sSlotName, pTexture->GetGALTexture(), textureRange, overrideViewFormat, overrideViewType);
    AddSamplerBinding(sSlotName, pTexture->GetGALSamplerState());
  }
}

void WExtractedRenderData::SortAndBatch()
{
  W_PROFILE_SCOPE("WExtractedRenderData::SortAndBatch");

  for (WUInt32 i = 0; i < m_DataPerCategory.GetCount(); ++i)
  {
    auto& dataPerCategory = m_DataPerCategory[i];
    if (dataPerCategory.m_SortableRenderData.IsEmpty())
      continue;

    SortAndBatchCategory(dataPerCategory, WRenderData::Category(i));
  }
}

void WExtractedRenderData::Clear()
{
  for (auto& dataPerCategory : m_DataPerCategory)
  {
    dataPerCategory.m_Batches.Clear();
    dataPerCategory.m_SortableRenderData.Clear();
    dataPerCategory.m_DataOffsets.Clear();
    dataPerCategory.m_TextureDependencies.Clear();
    dataPerCategory.m_BufferDependencies.Clear();
  }

  m_FrameData.Clear();
  m_ViewTextureDependencies.Clear();
  m_ViewBufferDependencies.Clear();

  m_TextureBindings.Clear();
  m_BufferBindings.Clear();
  m_SamplerBindings.Clear();
}

WRenderDataBatchList WExtractedRenderData::GetRenderDataBatchesWithCategory(WRenderData::Category category) const
{
  if (category.m_uiValue < m_DataPerCategory.GetCount())
  {
    WRenderDataBatchList list;
    list.m_Batches = m_DataPerCategory[category.m_uiValue].m_Batches;

    return list;
  }

  return WRenderDataBatchList();
}

WArrayPtr<const WRenderDataBatch::SortableRenderData> WExtractedRenderData::GetRawRenderDataWithCategory(WRenderData::Category category) const
{
  if (category.m_uiValue < m_DataPerCategory.GetCount())
  {
    return m_DataPerCategory[category.m_uiValue].m_SortableRenderData;
  }

  return {};
}

WArrayPtr<const WTextureDependency> WExtractedRenderData::GetTextureDependenciesWithCategory(WRenderData::Category category) const
{
  if (category.m_uiValue < m_DataPerCategory.GetCount())
  {
    return m_DataPerCategory[category.m_uiValue].m_TextureDependencies;
  }

  return {};
}

WArrayPtr<const WBufferDependency> WExtractedRenderData::GetBufferDependenciesWithCategory(WRenderData::Category category) const
{
  if (category.m_uiValue < m_DataPerCategory.GetCount())
  {
    return m_DataPerCategory[category.m_uiValue].m_BufferDependencies;
  }

  return {};
}

const WRenderData* WExtractedRenderData::GetFrameData(const WRTTI* pRtti) const
{
  for (auto pData : m_FrameData)
  {
    if (pData->IsInstanceOf(pRtti))
    {
      return pData;
    }
  }

  return nullptr;
}

void WExtractedRenderData::SortAndBatchCategory(DataPerCategory& dataPerCategory, WRenderData::Category category)
{
  struct RenderDataComparer
  {
    W_FORCE_INLINE bool Less(const WRenderDataBatch::SortableRenderData& a, const WRenderDataBatch::SortableRenderData& b) const
    {
      if (a.m_uiSortingKey != b.m_uiSortingKey)
      {
        return a.m_uiSortingKey < b.m_uiSortingKey;
      }

      return a.m_pRenderData->m_hOwner < b.m_pRenderData->m_hOwner;
    }
  };

  W_PROFILE_SCOPE("SortCategory");

  auto& data = dataPerCategory.m_SortableRenderData;

  // Sort
  data.Sort(RenderDataComparer());

  const bool bIsStereo = m_Camera.GetCameraMode() == WCameraMode::Stereo;
  const WUInt32 uiStereoCorrectionShift = bIsStereo ? 1 : 0;

  auto FillDataOffsets = [&](const WRenderData* pRenderData, const WRTTI* pType)
  {
    if (!pType->IsDerivedFrom<WInstanceableRenderData>())
      return;

    auto pInstanceableRenderData = static_cast<const WInstanceableRenderData*>(pRenderData);
    if (pInstanceableRenderData->m_uiNumInstances > 0)
    {
      auto& dataOffsets = pInstanceableRenderData->m_DataOffsets;
      for (WUInt32 uiInstanceIndex = 0; uiInstanceIndex < pInstanceableRenderData->m_uiNumInstances; ++uiInstanceIndex)
      {
        auto& instanceDataOffset = dataPerCategory.m_DataOffsets.ExpandAndGetRef();
        instanceDataOffset.m_uiInstance = dataOffsets.m_uiInstance + uiInstanceIndex;
        instanceDataOffset.m_uiCustomInstance = dataOffsets.m_uiCustomInstance + uiInstanceIndex;
        instanceDataOffset.m_uiMaterial = dataOffsets.m_uiMaterial;
        instanceDataOffset.m_uiSkinning = dataOffsets.m_uiSkinning;

        // Stereo rendering uses twice the number of instances so we need to duplicate the data offsets
        if (bIsStereo)
        {
          dataPerCategory.m_DataOffsets.PushBack(instanceDataOffset);
        }
      }
    }
  };

  // Find batches
  const WRenderData* pCurrentBatchRenderData = data[0].m_pRenderData;
  const WRTTI* pCurrentBatchType = pCurrentBatchRenderData->GetDynamicRTTI();
  WUInt32 uiCurrentBatchStartIndex = 0;
  WUInt32 uiCurrentDataOffsetIndex = 0;
  FillDataOffsets(pCurrentBatchRenderData, pCurrentBatchType);

  for (WUInt32 uiRenderDataIndex = 1; uiRenderDataIndex < data.GetCount(); ++uiRenderDataIndex)
  {
    const WRenderData* pRenderData = data[uiRenderDataIndex].m_pRenderData;
    const WRTTI* pRenderDataType = pRenderData->GetDynamicRTTI();

    if (pRenderDataType != pCurrentBatchType || pRenderData->CanBatch(*pCurrentBatchRenderData) == false)
    {
      auto& batch = dataPerCategory.m_Batches.ExpandAndGetRef();
      batch.m_Data = WMakeArrayPtr(&data[uiCurrentBatchStartIndex], uiRenderDataIndex - uiCurrentBatchStartIndex);
      batch.m_uiFirstDataOffsetIndex = uiCurrentDataOffsetIndex;
      batch.m_uiInstanceCount = (dataPerCategory.m_DataOffsets.GetCount() - uiCurrentDataOffsetIndex) >> uiStereoCorrectionShift;

      pCurrentBatchRenderData = pRenderData;
      pCurrentBatchType = pRenderDataType;
      uiCurrentBatchStartIndex = uiRenderDataIndex;
      uiCurrentDataOffsetIndex = dataPerCategory.m_DataOffsets.GetCount();
    }

    FillDataOffsets(pRenderData, pRenderDataType);
  }

  auto& batch = dataPerCategory.m_Batches.ExpandAndGetRef();
  batch.m_Data = WMakeArrayPtr(&data[uiCurrentBatchStartIndex], data.GetCount() - uiCurrentBatchStartIndex);
  batch.m_uiFirstDataOffsetIndex = uiCurrentDataOffsetIndex;
  batch.m_uiInstanceCount = (dataPerCategory.m_DataOffsets.GetCount() - uiCurrentDataOffsetIndex) >> uiStereoCorrectionShift;

  // Create or update data offsets buffer
  if (dataPerCategory.m_DataOffsets.IsEmpty() == false)
  {
    WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

    if (!dataPerCategory.m_hDataOffsetsBuffer.IsInvalidated())
    {
      auto& bufferDesc = pDevice->GetBuffer(dataPerCategory.m_hDataOffsetsBuffer)->GetDescription();
      if (bufferDesc.m_uiTotalSize < dataPerCategory.m_DataOffsets.GetCount() * sizeof(WInstanceableRenderData::DataOffsets))
      {
        pDevice->DestroyBuffer(dataPerCategory.m_hDataOffsetsBuffer);
      }
    }

    const WUInt32 uiNumDataOffsets = WMemoryUtils::AlignSize(dataPerCategory.m_DataOffsets.GetCount(), 64u);

    if (dataPerCategory.m_hDataOffsetsBuffer.IsInvalidated())
    {
      dataPerCategory.m_DataOffsets.SetCount(uiNumDataOffsets); // make sure the buffer is large enough

      WGALBufferCreationDescription bufferDesc;
      bufferDesc.m_uiStructSize = sizeof(WInstanceableRenderData::DataOffsets);
      bufferDesc.m_uiTotalSize = uiNumDataOffsets * bufferDesc.m_uiStructSize;
      bufferDesc.m_BufferFlags = WGALBufferUsageFlags::VertexBuffer;
      bufferDesc.m_ResourceAccess.m_bImmutable = false;

      dataPerCategory.m_hDataOffsetsBuffer = pDevice->CreateBuffer(bufferDesc, dataPerCategory.m_DataOffsets.GetByteArrayPtr());

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
      WStringBuilder sb = m_ViewData.m_sName.GetView();
      sb.Append(" - ");
      sb.Append(WRenderData::GetCategoryName(category).GetView());
      sb.Append(" - Data Offsets");

      pDevice->GetBuffer(dataPerCategory.m_hDataOffsetsBuffer)->SetDebugName(sb);
#endif
    }
    else
    {
      pDevice->UpdateBufferForNextFrame(dataPerCategory.m_hDataOffsetsBuffer, dataPerCategory.m_DataOffsets.GetByteArrayPtr(), 0);
    }

    // Set buffer handle on batches
    for (auto& batch : dataPerCategory.m_Batches)
    {
      batch.m_hDataOffsetsBuffer = dataPerCategory.m_hDataOffsetsBuffer;
    }
  }
}
