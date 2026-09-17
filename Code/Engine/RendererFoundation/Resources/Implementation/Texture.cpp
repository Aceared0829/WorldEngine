#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Texture.h>

WGALTexture::WGALTexture(const WGALTextureCreationDescription& Description)
  : WGALResource(Description)
{
}

WGALTexture::~WGALTexture()
{
  W_ASSERT_DEV(m_hDefaultRenderTargetView.IsInvalidated(), "");
  W_ASSERT_DEV(m_RenderTargetViews.IsEmpty(), "Dangling render target views");
}

WVec3U32 WGALTexture::GetMipMapSize(WUInt32 uiMipLevel) const
{
  return m_Description.GetMipMapSize(uiMipLevel);
}

WGALTextureRange WGALTexture::ClampRange(WGALTextureRange range) const
{
  const WUInt16 uiSlices = (m_Description.m_Type == WGALTextureType::TextureCube || m_Description.m_Type == WGALTextureType::TextureCubeArray) ? (WUInt16)m_Description.m_uiArraySize * 6 : (WUInt16)m_Description.m_uiArraySize;
  const WUInt8 uiMipLevels = (WUInt8)m_Description.m_uiMipLevelCount;
  if (range.m_uiArraySlices == W_GAL_ALL_ARRAY_SLICES)
  {
    range.m_uiArraySlices = static_cast<WUInt16>(uiSlices - range.m_uiBaseArraySlice);
  }
  if (range.m_uiMipLevels == W_GAL_ALL_MIP_LEVELS)
  {
    range.m_uiMipLevels = static_cast<WUInt8>(uiMipLevels - range.m_uiBaseMipLevel);
  }
  W_ASSERT_DEBUG(range.m_uiBaseArraySlice + range.m_uiArraySlices <= uiSlices, "Invalid WGALTextureRange: Base array slice {} + array slices {} is bigger than texture's slice count {}", range.m_uiBaseArraySlice, range.m_uiArraySlices, uiSlices);
  W_ASSERT_DEBUG(range.m_uiBaseMipLevel + range.m_uiMipLevels <= uiMipLevels, "Invalid WGALTextureRange: Base mip level {} + mip levels {} is bigger than texture's mip level count {}", range.m_uiBaseMipLevel, range.m_uiMipLevels, uiMipLevels);
  return range;
}
