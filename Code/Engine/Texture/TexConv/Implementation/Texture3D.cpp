#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

WResult WTexConvProcessor::Assemble3DTexture(WImage& dst) const
{
  W_PROFILE_SCOPE("Assemble3DTexture");

  const auto& images = m_Descriptor.m_InputImages;

  return WImageUtils::CreateVolumeTextureFromSingleFile(dst, images[0]);
}
