#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/TexConv/TexConvProcessor.h>

WResult WTexConvProcessor::Assemble2DArrayTexture(WImage& dst) const
{
  W_PROFILE_SCOPE("Assemble2DArrayTexture");

  const auto& mappings = m_Descriptor.m_ChannelMappings;
  const WUInt32 uiNumSlices = mappings.GetCount();

  if (uiNumSlices == 0)
  {
    WLog::Error("No channel mappings provided for 2D array texture assembly. Need at least one slice.");
    return W_FAILURE;
  }

  const WUInt32 uiWidth = m_Descriptor.m_InputImages[0].GetWidth();
  const WUInt32 uiHeight = m_Descriptor.m_InputImages[0].GetHeight();

  WImageHeader header;
  header.SetWidth(uiWidth);
  header.SetHeight(uiHeight);
  header.SetDepth(1);
  header.SetNumFaces(1);
  header.SetNumMipLevels(1);
  header.SetNumArrayIndices(uiNumSlices);
  header.SetImageFormat(WImageFormat::R32G32B32A32_FLOAT);

  dst.ResetAndAlloc(header);

  for (WUInt32 uiSlice = 0; uiSlice < uiNumSlices; ++uiSlice)
  {
    WColor* pPixelOut = dst.GetPixelPointer<WColor>(0, 0, uiSlice);
    W_SUCCEED_OR_RETURN(Assemble2DSlice(mappings[uiSlice], uiWidth, uiHeight, pPixelOut));
  }

  return W_SUCCESS;
}
