#include <Texture/TexturePCH.h>

#if W_ENABLED(W_PLATFORM_LINUX)

// BC.h poisons the preprocessor, making it impossible to include algorithm afterwards, so this has to be here.
#  include <algorithm>

#  include <Texture/DirectXTex/BC.h>

#  include <Texture/Image/ImageConversion.h>

#  include <Foundation/Threading/TaskSystem.h>

WImageConversionEntry g_DXTexCpuConversions[] = {
  WImageConversionEntry(WImageFormat::R32G32B32A32_FLOAT, WImageFormat::BC6H_UF16, WImageConversionFlags::Default),

  WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM, WImageFormat::BC1_UNORM, WImageConversionFlags::Default, 100),
  WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM, WImageFormat::BC7_UNORM, WImageConversionFlags::Default, 100),

  WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM_SRGB, WImageFormat::BC1_UNORM_SRGB, WImageConversionFlags::Default, 100),
  WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM_SRGB, WImageFormat::BC7_UNORM_SRGB, WImageConversionFlags::Default, 100),
};

class WImageConversion_CompressDxTexCpu : public WImageConversionStepCompressBlocks
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    return g_DXTexCpuConversions;
  }

  virtual WResult CompressBlocks(WConstByteBlobPtr source, WByteBlobPtr target, WUInt32 numBlocksX, WUInt32 numBlocksY,
    WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    if (targetFormat == WImageFormat::BC7_UNORM || targetFormat == WImageFormat::BC7_UNORM_SRGB)
    {
      const WUInt32 srcStride = numBlocksX * 4 * 4;
      const WUInt32 targetStride = numBlocksX * 16;

      WTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](WUInt32 startIndex, WUInt32 endIndex)
        {
        const WUInt8* srcIt = source.GetPtr() + srcStride * startIndex * 4;
        WUInt8* targetIt = target.GetPtr() + targetStride * startIndex;
        for (WUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (WUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (WUInt32 y = 0; y < 4; y++)
            {
              for (WUInt32 x = 0; x < 4; x++)
              {
                const WUInt8* pixel = srcIt + y * srcStride + x * 4;
                temp[y * 4 + x] = DirectX::XMVectorSet(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, pixel[3] / 255.0f);
              }
            }
            DirectX::D3DXEncodeBC7(targetIt, temp, 0);

            srcIt += 4 * 4;
            targetIt += 16;
          }
          srcIt += 3 * srcStride;
        } });

      return W_SUCCESS;
    }
    else if (targetFormat == WImageFormat::BC1_UNORM || targetFormat == WImageFormat::BC1_UNORM_SRGB)
    {
      const WUInt32 srcStride = numBlocksX * 4 * 4;
      const WUInt32 targetStride = numBlocksX * 8;

      WTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](WUInt32 startIndex, WUInt32 endIndex)
        {
        const WUInt8* srcIt = source.GetPtr() + srcStride * startIndex * 4;
        WUInt8* targetIt = target.GetPtr() + targetStride * startIndex;
        for (WUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (WUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (WUInt32 y = 0; y < 4; y++)
            {
              for (WUInt32 x = 0; x < 4; x++)
              {
                const WUInt8* pixel = srcIt + y * srcStride + x * 4;
                temp[y * 4 + x] = DirectX::XMVectorSet(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, pixel[3] / 255.0f);
              }
            }
            DirectX::D3DXEncodeBC1(targetIt, temp, 1.0f, 0);

            srcIt += 4 * 4;
            targetIt += 8;
          }
          srcIt += 3 * srcStride;
        } });

      return W_SUCCESS;
    }
    else if (targetFormat == WImageFormat::BC6H_UF16)
    {
      const WUInt32 srcStride = numBlocksX * 4 * 4 * sizeof(float);
      const WUInt32 targetStride = numBlocksX * 16;

      WTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](WUInt32 startIndex, WUInt32 endIndex)
        {
        const WUInt8* srcIt = source.GetPtr() + srcStride * startIndex * 4;
        WUInt8* targetIt = target.GetPtr() + targetStride * startIndex;
        for (WUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (WUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (WUInt32 y = 0; y < 4; y++)
            {
              for (WUInt32 x = 0; x < 4; x++)
              {
                const float* pixel = reinterpret_cast<const float*>(srcIt + y * srcStride + x * 4 * sizeof(float));
                temp[y * 4 + x] = DirectX::XMVectorSet(pixel[0], pixel[1], pixel[2], pixel[3]);
              }
            }
            DirectX::D3DXEncodeBC6HU(targetIt, temp, 0);

            srcIt += 4 * 4 * sizeof(float);
            targetIt += 16;
          }
          srcIt += 3 * srcStride;
        } });

      return W_SUCCESS;
    }

    return W_FAILURE;
  }
};

W_STATICLINK_FORCE static WImageConversion_CompressDxTexCpu s_conversion_compressDxTexCpu;

#endif

W_STATICLINK_FILE(Texture, Texture_Image_Conversions_DXTexCpuConversions);
