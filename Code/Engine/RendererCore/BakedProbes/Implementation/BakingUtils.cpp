#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/BakedProbes/BakingUtils.h>

WVec3 WBakingUtils::FibonacciSphere(WUInt32 uiSampleIndex, WUInt32 uiNumSamples)
{
  float offset = 2.0f / uiNumSamples;
  float increment = WMath::Pi<float>() * (3.0f - WMath::Sqrt(5.0f));

  float y = ((uiSampleIndex * offset) - 1) + (offset / 2);
  float r = WMath::Sqrt(1 - y * y);

  WAngle phi = WAngle::MakeFromRadian(((uiSampleIndex + 1) % uiNumSamples) * increment);

  float x = WMath::Cos(phi) * r;
  float z = WMath::Sin(phi) * r;

  return WVec3(x, y, z);
}

static WUInt32 s_BitsPerDir[WAmbientCubeBasis::NumDirs] = {5, 5, 5, 5, 6, 6};

WCompressedSkyVisibility WBakingUtils::CompressSkyVisibility(const WAmbientCube<float>& skyVisibility)
{
  WCompressedSkyVisibility result = 0;
  WUInt32 uiOffset = 0;
  for (WUInt32 i = 0; i < WAmbientCubeBasis::NumDirs; ++i)
  {
    float maxValue = static_cast<float>((1u << s_BitsPerDir[i]) - 1u);
    WUInt32 compressedDir = static_cast<WUInt8>(WMath::Saturate(skyVisibility.m_Values[i]) * maxValue + 0.5f);
    result |= (compressedDir << uiOffset);
    uiOffset += s_BitsPerDir[i];
  }

  return result;
}

void WBakingUtils::DecompressSkyVisibility(WCompressedSkyVisibility compressedSkyVisibility, WAmbientCube<float>& out_skyVisibility)
{
  WUInt32 uiOffset = 0;
  for (WUInt32 i = 0; i < WAmbientCubeBasis::NumDirs; ++i)
  {
    WUInt32 maxValue = (1u << s_BitsPerDir[i]) - 1u;
    out_skyVisibility.m_Values[i] = static_cast<float>((compressedSkyVisibility >> uiOffset) & maxValue) * (1.0f / maxValue);
    uiOffset += s_BitsPerDir[i];
  }
}
