#pragma once

void WExtractedRenderData::AddRenderData(const WRenderData* pRenderData, WRenderData::Category category)
{
  m_DataPerCategory.EnsureCount(category.m_uiValue + 1);

  auto& sortableRenderData = m_DataPerCategory[category.m_uiValue].m_SortableRenderData.ExpandAndGetRef();
  sortableRenderData.m_pRenderData = pRenderData;
  sortableRenderData.m_uiSortingKey = pRenderData->GetFinalSortingKey(category, m_Camera);
}

void WExtractedRenderData::AddFrameData(const WRenderData* pFrameData)
{
  m_FrameData.PushBack(pFrameData);
}

void WExtractedRenderData::AddViewDependency(WGALTextureHandle hTexture, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage)
{
  if (hTexture.IsInvalidated())
    return;

  auto& dep = m_ViewTextureDependencies.ExpandAndGetRef();
  dep.m_hTexture = hTexture;
  dep.m_RequiredState = requiredState;
  dep.m_Stage = stage;
}

void WExtractedRenderData::AddViewDependency(WGALBufferHandle hBuffer, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage)
{
  if (hBuffer.IsInvalidated())
    return;

  auto& dep = m_ViewBufferDependencies.ExpandAndGetRef();
  dep.m_hBuffer = hBuffer;
  dep.m_RequiredState = requiredState;
  dep.m_Stage = stage;
}

void WExtractedRenderData::AddDependency(const WTextureDependency& dependency)
{
  W_ASSERT_DEBUG(dependency.m_uiCategory != WInvalidRenderDataCategory.m_uiValue, "Per-category texture dependencies require a valid render data category. Use AddViewDependency for view-level dependencies.");
  m_DataPerCategory.EnsureCount(dependency.m_uiCategory + 1);
  m_DataPerCategory[dependency.m_uiCategory].m_TextureDependencies.PushBack(dependency);
}

void WExtractedRenderData::AddDependency(const WBufferDependency& dependency)
{
  W_ASSERT_DEBUG(dependency.m_uiCategory != WInvalidRenderDataCategory.m_uiValue, "Per-category buffer dependencies require a valid render data category. Use AddViewDependency for view-level dependencies.");
  m_DataPerCategory.EnsureCount(dependency.m_uiCategory + 1);
  m_DataPerCategory[dependency.m_uiCategory].m_BufferDependencies.PushBack(dependency);
}

void WExtractedRenderData::AddSamplerBinding(const WSamplerBinding& binding)
{
  m_SamplerBindings.PushBack(binding);
}

void WExtractedRenderData::AddBufferBinding(const WBufferBinding& binding)
{
  m_BufferBindings.PushBack(binding);
}

void WExtractedRenderData::AddTextureBinding(const WTextureBinding& binding)
{
  m_TextureBindings.PushBack(binding);
}
