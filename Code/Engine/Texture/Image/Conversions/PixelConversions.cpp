#include <Texture/TexturePCH.h>

#include <Foundation/Math/Float16.h>
#include <Texture/Image/Conversions/PixelConversions.h>
#include <Texture/Image/ImageConversion.h>

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_SSE_LEVEL >= W_SSE_20
#  include <emmintrin.h>
#endif

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_SSE_LEVEL >= W_SSE_30
#  include <tmmintrin.h>
#endif

namespace
{
  // 3D vector: 11/11/10 floating-point components
  // The 3D vector is packed into 32 bits as follows: a 5-bit biased exponent
  // and 6-bit mantissa for x component, a 5-bit biased exponent and
  // 6-bit mantissa for y component, a 5-bit biased exponent and a 5-bit
  // mantissa for z. The z component is stored in the most significant bits
  // and the x component in the least significant bits. No sign bits so
  // all partial-precision numbers are positive.
  // (Z10Y11X11): [32] ZZZZZzzz zzzYYYYY yyyyyyXX XXXxxxxx [0
  union R11G11B10
  {
    struct Parts
    {
      WUInt32 xm : 6; // x-mantissa
      WUInt32 xe : 5; // x-exponent
      WUInt32 ym : 6; // y-mantissa
      WUInt32 ye : 5; // y-exponent
      WUInt32 zm : 5; // z-mantissa
      WUInt32 ze : 5; // z-exponent
    } p;
    WUInt32 v;
  };
} // namespace

WColorBaseUB WDecompressA4B4G4R4(WUInt16 uiColor)
{
  WColorBaseUB result;
  result.r = ((uiColor & 0xF000u) * 17) >> 12;
  result.g = ((uiColor & 0x0F00u) * 17) >> 8;
  result.b = ((uiColor & 0x00F0u) * 17) >> 4;
  result.a = ((uiColor & 0x000Fu) * 17);
  return result;
}

WUInt16 WCompressA4B4G4R4(WColorBaseUB color)
{
  WUInt32 r = (color.r * 15 + 135) >> 8;
  WUInt32 g = (color.g * 15 + 135) >> 8;
  WUInt32 b = (color.b * 15 + 135) >> 8;
  WUInt32 a = (color.a * 15 + 135) >> 8;
  return static_cast<WUInt16>((r << 12) | (g << 8) | (b << 4) | a);
}

WColorBaseUB WDecompressB4G4R4A4(WUInt16 uiColor)
{
  WColorBaseUB result;
  result.r = ((uiColor & 0x0F00u) * 17) >> 8;
  result.g = ((uiColor & 0x00F0u) * 17) >> 4;
  result.b = ((uiColor & 0x000Fu) * 17);
  result.a = ((uiColor & 0xF000u) * 17) >> 12;
  return result;
}

WUInt16 WCompressB4G4R4A4(WColorBaseUB color)
{
  WUInt32 r = (color.r * 15 + 135) >> 8;
  WUInt32 g = (color.g * 15 + 135) >> 8;
  WUInt32 b = (color.b * 15 + 135) >> 8;
  WUInt32 a = (color.a * 15 + 135) >> 8;
  return static_cast<WUInt16>((a << 12) | (r << 8) | (g << 4) | b);
}

WColorBaseUB WDecompressB5G6R5(WUInt16 uiColor)
{
  WColorBaseUB result;
  result.r = static_cast<WUInt8>(((uiColor & 0xF800u) * 527 + 47104) >> 17);
  result.g = static_cast<WUInt8>(((uiColor & 0x07E0u) * 259 + 1056) >> 11);
  result.b = static_cast<WUInt8>(((uiColor & 0x001Fu) * 527 + 23) >> 6);
  result.a = 0xFF;

  return result;
}

WUInt16 WCompressB5G6R5(WColorBaseUB color)
{
  WUInt32 r = (color.r * 249 + 1024) >> 11;
  WUInt32 g = (color.g * 253 + 512) >> 10;
  WUInt32 b = (color.b * 249 + 1024) >> 11;
  return static_cast<WUInt16>((r << 11) | (g << 5) | b);
}

WColorBaseUB WDecompressB5G5R5X1(WUInt16 uiColor)
{
  WColorBaseUB result;
  result.r = static_cast<WUInt8>(((uiColor & 0x7C00u) * 527 + 23552) >> 16);
  result.g = static_cast<WUInt8>(((uiColor & 0x03E0u) * 527 + 736) >> 11);
  result.b = static_cast<WUInt8>(((uiColor & 0x001Fu) * 527 + 23) >> 6);
  result.a = 0xFF;
  return result;
}

WUInt16 WCompressB5G5R5X1(WColorBaseUB color)
{
  WUInt32 r = (color.r * 249 + 1024) >> 11;
  WUInt32 g = (color.g * 249 + 1024) >> 11;
  WUInt32 b = (color.b * 249 + 1024) >> 11;
  return static_cast<WUInt16>((1 << 15) | (r << 10) | (g << 5) | b);
}

WColorBaseUB WDecompressB5G5R5A1(WUInt16 uiColor)
{
  WColorBaseUB result;
  result.r = static_cast<WUInt8>(((uiColor & 0x7C00u) * 527 + 23552) >> 16);
  result.g = static_cast<WUInt8>(((uiColor & 0x03E0u) * 527 + 736) >> 11);
  result.b = static_cast<WUInt8>(((uiColor & 0x001Fu) * 527 + 23) >> 6);
  result.a = static_cast<WUInt8>(((uiColor & 0x8000u) * 255) >> 15);
  return result;
}

WUInt16 WCompressB5G5R5A1(WColorBaseUB color)
{
  WUInt32 r = (color.r * 249 + 1024) >> 11;
  WUInt32 g = (color.g * 249 + 1024) >> 11;
  WUInt32 b = (color.b * 249 + 1024) >> 11;
  WUInt32 a = (color.a) >> 7;
  return static_cast<WUInt16>((a << 15) | (r << 10) | (g << 5) | b);
}

WColorBaseUB WDecompressX1B5G5R5(WUInt16 uiColor)
{
  WColorBaseUB result;
  result.r = static_cast<WUInt8>(((uiColor & 0xF800u) * 527 + 23552) >> 17);
  result.g = static_cast<WUInt8>(((uiColor & 0x07C0u) * 527 + 736) >> 12);
  result.b = static_cast<WUInt8>(((uiColor & 0x003Eu) * 527 + 23) >> 7);
  result.a = 0xFF;
  return result;
}

WUInt16 WCompressX1B5G5R5(WColorBaseUB color)
{
  WUInt32 r = (color.r * 249 + 1024) >> 11;
  WUInt32 g = (color.g * 249 + 1024) >> 11;
  WUInt32 b = (color.b * 249 + 1024) >> 11;
  return static_cast<WUInt16>((r << 11) | (g << 6) | (b << 1) | 1);
}

WColorBaseUB WDecompressA1B5G5R5(WUInt16 uiColor)
{
  WColorBaseUB result;
  result.r = static_cast<WUInt8>(((uiColor & 0xF800u) * 527 + 23552) >> 17);
  result.g = static_cast<WUInt8>(((uiColor & 0x07C0u) * 527 + 736) >> 12);
  result.b = static_cast<WUInt8>(((uiColor & 0x003Eu) * 527 + 23) >> 7);
  result.a = static_cast<WUInt8>((uiColor & 0x0001u) * 255);
  return result;
}

WUInt16 WCompressA1B5G5R5(WColorBaseUB color)
{
  WUInt32 r = (color.r * 249 + 1024) >> 11;
  WUInt32 g = (color.g * 249 + 1024) >> 11;
  WUInt32 b = (color.b * 249 + 1024) >> 11;
  WUInt32 a = color.a >> 7;
  return static_cast<WUInt16>((r << 11) | (g << 6) | (b << 1) | a);
}

template <WColorBaseUB (*decompressFunc)(WUInt16), WImageFormat::Enum templateSourceFormat>
class WImageConversionStep_Decompress16bpp : WImageConversionStepLinear
{
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    WImageFormat::Enum sourceFormatSrgb = WImageFormat::AsSrgb(templateSourceFormat);
    W_ASSERT_DEV(
      sourceFormatSrgb != templateSourceFormat, "Format '%s' should have a corresponding sRGB format", WImageFormat::GetName(templateSourceFormat));

    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(templateSourceFormat, WImageFormat::R8G8B8A8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(sourceFormatSrgb, WImageFormat::R8G8B8A8_UNORM_SRGB, WImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 numElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);
    W_IGNORE_UNUSED(targetFormat);

    WUInt32 sourceStride = 2;
    WUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<WColorBaseUB*>(targetPointer) = decompressFunc(*reinterpret_cast<const WUInt16*>(sourcePointer));

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return W_SUCCESS;
  }
};

template <WUInt16 (*compressFunc)(WColorBaseUB), WImageFormat::Enum templateTargetFormat>
class WImageConversionStep_Compress16bpp : WImageConversionStepLinear
{
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    WImageFormat::Enum targetFormatSrgb = WImageFormat::AsSrgb(templateTargetFormat);
    W_ASSERT_DEV(
      targetFormatSrgb != templateTargetFormat, "Format '%s' should have a corresponding sRGB format", WImageFormat::GetName(templateTargetFormat));

    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM, templateTargetFormat, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM_SRGB, targetFormatSrgb, WImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 numElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);
    W_IGNORE_UNUSED(targetFormat);

    WUInt32 sourceStride = 4;
    WUInt32 targetStride = 2;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<WUInt16*>(targetPointer) = compressFunc(*reinterpret_cast<const WColorBaseUB*>(sourcePointer));

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return W_SUCCESS;
  }
};

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE

static bool IsAligned(const void* pPointer)
{
  return reinterpret_cast<size_t>(pPointer) % 16 == 0;
}

#endif

struct WImageSwizzleConversion32_2103 : public WImageConversionStepLinear
{
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::B8G8R8A8_UNORM, WImageFormat::R8G8B8A8_UNORM, WImageConversionFlags::InPlace),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM, WImageFormat::B8G8R8A8_UNORM, WImageConversionFlags::InPlace),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM, WImageFormat::B8G8R8X8_UNORM, WImageConversionFlags::InPlace),
      WImageConversionEntry(WImageFormat::B8G8R8A8_UNORM_SRGB, WImageFormat::R8G8B8A8_UNORM_SRGB, WImageConversionFlags::InPlace),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM_SRGB, WImageFormat::B8G8R8A8_UNORM_SRGB, WImageConversionFlags::InPlace),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM_SRGB, WImageFormat::B8G8R8X8_UNORM_SRGB, WImageConversionFlags::InPlace),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);
    W_IGNORE_UNUSED(targetFormat);

    WUInt32 sourceStride = 4;
    WUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
    if (IsAligned(sourcePointer) && IsAligned(targetPointer))
    {
#  if W_SSE_LEVEL >= W_SSE_30
      const WUInt32 elementsPerBatch = 8;

      __m128i shuffleMask = _mm_set_epi8(15, 12, 13, 14, 11, 8, 9, 10, 7, 4, 5, 6, 3, 0, 1, 2);

      // Intel optimization manual, Color Pixel Format Conversion Using SSE3
      while (uiNumElements >= elementsPerBatch)
      {
        __m128i in0 = reinterpret_cast<const __m128i*>(sourcePointer)[0];
        __m128i in1 = reinterpret_cast<const __m128i*>(sourcePointer)[1];

        reinterpret_cast<__m128i*>(targetPointer)[0] = _mm_shuffle_epi8(in0, shuffleMask);
        reinterpret_cast<__m128i*>(targetPointer)[1] = _mm_shuffle_epi8(in1, shuffleMask);

        sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
#  else
      const WUInt32 elementsPerBatch = 8;

      __m128i mask1 = _mm_set1_epi32(0xff00ff00);
      __m128i mask2 = _mm_set1_epi32(0x00ff00ff);

      // Intel optimization manual, Color Pixel Format Conversion Using SSE2
      while (uiNumElements >= elementsPerBatch)
      {
        __m128i in0 = reinterpret_cast<const __m128i*>(sourcePointer)[0];
        __m128i in1 = reinterpret_cast<const __m128i*>(sourcePointer)[1];

        reinterpret_cast<__m128i*>(targetPointer)[0] =
          _mm_or_si128(_mm_and_si128(in0, mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(in0, 16), _mm_srli_epi32(in0, 16)), mask2));
        reinterpret_cast<__m128i*>(targetPointer)[1] =
          _mm_or_si128(_mm_and_si128(in1, mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(in1, 16), _mm_srli_epi32(in1, 16)), mask2));

        sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
#  endif
    }
#endif

    while (uiNumElements)
    {
      WUInt8 a, b, c, d;
      a = reinterpret_cast<const WUInt8*>(sourcePointer)[2];
      b = reinterpret_cast<const WUInt8*>(sourcePointer)[1];
      c = reinterpret_cast<const WUInt8*>(sourcePointer)[0];
      d = reinterpret_cast<const WUInt8*>(sourcePointer)[3];
      reinterpret_cast<WUInt8*>(targetPointer)[0] = a;
      reinterpret_cast<WUInt8*>(targetPointer)[1] = b;
      reinterpret_cast<WUInt8*>(targetPointer)[2] = c;
      reinterpret_cast<WUInt8*>(targetPointer)[3] = d;

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

struct WImageConversion_BGRX_BGRA : public WImageConversionStepLinear
{
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      {WImageFormat::B8G8R8X8_UNORM, WImageFormat::B8G8R8A8_UNORM, WImageConversionFlags::InPlace},
      {WImageFormat::B8G8R8X8_UNORM_SRGB, WImageFormat::B8G8R8A8_UNORM_SRGB, WImageConversionFlags::InPlace},
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);
    W_IGNORE_UNUSED(targetFormat);

    WUInt32 sourceStride = 4;
    WUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_SSE_LEVEL >= W_SSE_20
    if (IsAligned(sourcePointer) && IsAligned(targetPointer))
    {
      const WUInt32 elementsPerBatch = 4;

      __m128i mask = _mm_set1_epi32(0xFF000000);

      while (uiNumElements >= elementsPerBatch)
      {
        const __m128i* pSource = reinterpret_cast<const __m128i*>(sourcePointer);
        __m128i* pTarget = reinterpret_cast<__m128i*>(targetPointer);

        pTarget[0] = _mm_or_si128(pSource[0], mask);

        sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
    }
#endif

    while (uiNumElements)
    {
      WUInt32 x = *(reinterpret_cast<const WUInt32*>(sourcePointer));

#if W_ENABLED(W_PLATFORM_LITTLE_ENDIAN)
      x |= 0xFF000000;
#else
      x |= 0x000000FF;
#endif

      *(reinterpret_cast<WUInt32*>(targetPointer)) = x;

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_F32_U8 : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R32_FLOAT, WImageFormat::R8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32_FLOAT, WImageFormat::R8G8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32_FLOAT, WImageFormat::R8G8B8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_FLOAT, WImageFormat::R8G8B8A8_UNORM, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels
    uiNumElements *= WImageFormat::GetBitsPerPixel(targetFormat) / 8;

    WUInt32 sourceStride = 4;
    WUInt32 targetStride = 1;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_SSE_LEVEL >= W_SSE_20
    {
      const WUInt32 elementsPerBatch = 16;

      __m128 zero = _mm_setzero_ps();
      __m128 one = _mm_set1_ps(1.0f);
      __m128 scale = _mm_set1_ps(255.0f);
      __m128 half = _mm_set1_ps(0.5f);

      while (uiNumElements >= elementsPerBatch)
      {
        __m128 float0 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 0);
        __m128 float1 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 4);
        __m128 float2 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 8);
        __m128 float3 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 12);

        // Clamp NaN to zero
        float0 = _mm_and_ps(_mm_cmpord_ps(float0, zero), float0);
        float1 = _mm_and_ps(_mm_cmpord_ps(float1, zero), float1);
        float2 = _mm_and_ps(_mm_cmpord_ps(float2, zero), float2);
        float3 = _mm_and_ps(_mm_cmpord_ps(float3, zero), float3);

        // Saturate
        float0 = _mm_max_ps(zero, _mm_min_ps(one, float0));
        float1 = _mm_max_ps(zero, _mm_min_ps(one, float1));
        float2 = _mm_max_ps(zero, _mm_min_ps(one, float2));
        float3 = _mm_max_ps(zero, _mm_min_ps(one, float3));

        float0 = _mm_mul_ps(float0, scale);
        float1 = _mm_mul_ps(float1, scale);
        float2 = _mm_mul_ps(float2, scale);
        float3 = _mm_mul_ps(float3, scale);

        // Add 0.5f and truncate for rounding as required by D3D spec
        float0 = _mm_add_ps(float0, half);
        float1 = _mm_add_ps(float1, half);
        float2 = _mm_add_ps(float2, half);
        float3 = _mm_add_ps(float3, half);

        __m128i int0 = _mm_cvttps_epi32(float0);
        __m128i int1 = _mm_cvttps_epi32(float1);
        __m128i int2 = _mm_cvttps_epi32(float2);
        __m128i int3 = _mm_cvttps_epi32(float3);

        __m128i short0 = _mm_packs_epi32(int0, int1);
        __m128i short1 = _mm_packs_epi32(int2, int3);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(targetPointer), _mm_packus_epi16(short0, short1));

        sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
    }
#endif

    while (uiNumElements)
    {

      *reinterpret_cast<WUInt8*>(targetPointer) = WMath::ColorFloatToByte(*reinterpret_cast<const float*>(sourcePointer));

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_F32_sRGB : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R32G32B32A32_FLOAT, WImageFormat::R8G8B8A8_UNORM_SRGB, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);
    W_IGNORE_UNUSED(targetFormat);

    WUInt32 sourceStride = 16;
    WUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<WColorGammaUB*>(targetPointer) = *reinterpret_cast<const WColor*>(sourcePointer);

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_F32_U16 : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R32_FLOAT, WImageFormat::R16_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32_FLOAT, WImageFormat::R16G16_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32_FLOAT, WImageFormat::R16G16B16_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_FLOAT, WImageFormat::R16G16B16A16_UNORM, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels
    uiNumElements *= WImageFormat::GetBitsPerPixel(targetFormat) / 16;

    WUInt32 sourceStride = 4;
    WUInt32 targetStride = 2;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {

      *reinterpret_cast<WUInt16*>(targetPointer) = WMath::ColorFloatToShort(*reinterpret_cast<const float*>(sourcePointer));

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_F32_F16 : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R32_FLOAT, WImageFormat::R16_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32_FLOAT, WImageFormat::R16G16_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_FLOAT, WImageFormat::R16G16B16A16_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels
    uiNumElements *= WImageFormat::GetBitsPerPixel(targetFormat) / 16;

    WUInt32 sourceStride = 4;
    WUInt32 targetStride = 2;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {

      *reinterpret_cast<WFloat16*>(targetPointer) = *reinterpret_cast<const float*>(sourcePointer);

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_F32_S8 : public WImageConversionStepLinear
{
public:
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R32_FLOAT, WImageFormat::R8_SNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32_FLOAT, WImageFormat::R8G8_SNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_FLOAT, WImageFormat::R8G8B8A8_SNORM, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels
    uiNumElements *= WImageFormat::GetBitsPerPixel(targetFormat) / 8;

    WUInt32 sourceStride = 4;
    WUInt32 targetStride = 1;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {

      *reinterpret_cast<WInt8*>(targetPointer) = WMath::ColorFloatToSignedByte(*reinterpret_cast<const float*>(sourcePointer));

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_U8_F32 : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R8_UNORM, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8_UNORM, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8_UNORM, WImageFormat::R32G32B32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels
    uiNumElements *= WImageFormat::GetBitsPerPixel(targetFormat) / 32;

    WUInt32 sourceStride = 1;
    WUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = WMath::ColorByteToFloat(*reinterpret_cast<const WUInt8*>(sourcePointer));

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_sRGB_F32 : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM_SRGB, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);
    W_IGNORE_UNUSED(targetFormat);

    WUInt32 sourceStride = 4;
    WUInt32 targetStride = 16;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<WColor*>(targetPointer) = *reinterpret_cast<const WColorGammaUB*>(sourcePointer);

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_U16_F32 : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R16_UNORM, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16_UNORM, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16_UNORM, WImageFormat::R32G32B32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_UNORM, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels
    uiNumElements *= WImageFormat::GetBitsPerPixel(targetFormat) / 32;

    WUInt32 sourceStride = 2;
    WUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = WMath::ColorShortToFloat(*reinterpret_cast<const WUInt16*>(sourcePointer));

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_S16_F32 : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R16_SNORM, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16_SNORM, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_SNORM, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);
    W_IGNORE_UNUSED(targetFormat);

    // Work with single channels instead of pixels
    uiNumElements *= WImageFormat::GetBitsPerPixel(targetFormat) / 32;

    WUInt32 sourceStride = 2;
    WUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = WMath::ColorSignedShortToFloat(*reinterpret_cast<const WInt16*>(sourcePointer));

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_F16_F32 : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R16_FLOAT, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16_FLOAT, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_FLOAT, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels
    uiNumElements *= WImageFormat::GetBitsPerPixel(targetFormat) / 32;

    WUInt32 sourceStride = 2;
    WUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = *reinterpret_cast<const WFloat16*>(sourcePointer);

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_S8_F32 : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R8_SNORM, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8_SNORM, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_SNORM, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels
    uiNumElements *= WImageFormat::GetBitsPerPixel(targetFormat) / 32;

    WUInt32 sourceStride = 1;
    WUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = WMath::ColorSignedByteToFloat(*reinterpret_cast<const WInt8*>(sourcePointer));

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

struct WImageConversion_Pad_To_RGBA_U8 : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R8_UNORM, WImageFormat::R8G8B8A8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8_UNORM, WImageFormat::R8G8B8A8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8_UNORM, WImageFormat::R8G8B8A8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8_UNORM_SRGB, WImageFormat::R8G8B8A8_UNORM_SRGB, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::B8G8R8_UNORM, WImageFormat::B8G8R8A8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::B8G8R8_UNORM_SRGB, WImageFormat::B8G8R8A8_UNORM_SRGB, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat) const override
  {
    WUInt32 sourceStride = WImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    WUInt32 targetStride = WImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const WUInt8* sourcePointer = static_cast<const WUInt8*>(source.GetPtr());
    WUInt8* targetPointer = static_cast<WUInt8*>(target.GetPtr());

    const WUInt32 numChannels = sourceStride / sizeof(WUInt8);

#if W_ENABLED(W_PLATFORM_LITTLE_ENDIAN)
    if (numChannels == 3)
    {
      // Fast path for RGB -> RGBA
      const WUInt32 elementsPerBatch = 4;

      while (uiNumElements >= elementsPerBatch)
      {
        WUInt32 source0 = reinterpret_cast<const WUInt32*>(sourcePointer)[0];
        WUInt32 source1 = reinterpret_cast<const WUInt32*>(sourcePointer)[1];
        WUInt32 source2 = reinterpret_cast<const WUInt32*>(sourcePointer)[2];

        WUInt32 target0 = source0 | 0xFF000000;
        WUInt32 target1 = (source0 >> 24) | (source1 << 8) | 0xFF000000;
        WUInt32 target2 = (source1 >> 16) | (source2 << 16) | 0xFF000000;
        WUInt32 target3 = (source2 >> 8) | 0xFF000000;

        reinterpret_cast<WUInt32*>(targetPointer)[0] = target0;
        reinterpret_cast<WUInt32*>(targetPointer)[1] = target1;
        reinterpret_cast<WUInt32*>(targetPointer)[2] = target2;
        reinterpret_cast<WUInt32*>(targetPointer)[3] = target3;

        sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
    }
#endif


    while (uiNumElements)
    {
      // Copy existing channels
      memcpy(targetPointer, sourcePointer, numChannels);

      // Fill others with zero
      memset(targetPointer + numChannels, 0, 3 * sizeof(WUInt8) - numChannels);

      // Set alpha to 1
      targetPointer[3] = 0xFF;

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

struct WImageConversion_Pad_To_RGBA_F32 : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R32_FLOAT, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32_FLOAT, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32_FLOAT, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat) const override
  {
    WUInt32 sourceStride = WImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    WUInt32 targetStride = WImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const float* sourcePointer = static_cast<const float*>(static_cast<const void*>(source.GetPtr()));
    float* targetPointer = static_cast<float*>(static_cast<void*>(target.GetPtr()));

    const WUInt32 numChannels = sourceStride / sizeof(float);

    while (uiNumElements)
    {
      // Copy existing channels
      memcpy(targetPointer, sourcePointer, numChannels * sizeof(float));

      // Fill others with zero
      memset(targetPointer + numChannels, 0, sizeof(float) * (3 - numChannels));

      // Set alpha to 1
      targetPointer[3] = 1.0f;

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

struct WImageConversion_DiscardChannels : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R32G32B32A32_FLOAT, WImageFormat::R32G32B32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_FLOAT, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_FLOAT, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_UINT, WImageFormat::R32G32B32_UINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_UINT, WImageFormat::R32G32_UINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_UINT, WImageFormat::R32_UINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_SINT, WImageFormat::R32G32B32_SINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_SINT, WImageFormat::R32G32_SINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_SINT, WImageFormat::R32_SINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32_FLOAT, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32_FLOAT, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32_UINT, WImageFormat::R32G32_UINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32_UINT, WImageFormat::R32_UINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32_SINT, WImageFormat::R32G32_SINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32_SINT, WImageFormat::R32_SINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_FLOAT, WImageFormat::R16G16_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_FLOAT, WImageFormat::R16_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_UNORM, WImageFormat::R16G16B16_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_UNORM, WImageFormat::R16G16_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_UNORM, WImageFormat::R16_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_UINT, WImageFormat::R16G16_UINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_UINT, WImageFormat::R16_UINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_SNORM, WImageFormat::R16G16_SNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_SNORM, WImageFormat::R16_SNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_SINT, WImageFormat::R16G16_SINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_SINT, WImageFormat::R16_SINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16_UNORM, WImageFormat::R16G16_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16_UNORM, WImageFormat::R16_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32_FLOAT, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32_UINT, WImageFormat::R32_UINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32_SINT, WImageFormat::R32_SINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::D32_FLOAT_S8X24_UINT, WImageFormat::D32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM, WImageFormat::R8G8B8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM, WImageFormat::R8G8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM, WImageFormat::R8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UNORM_SRGB, WImageFormat::R8G8B8_UNORM_SRGB, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UINT, WImageFormat::R8G8_UINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UINT, WImageFormat::R8_UINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_SNORM, WImageFormat::R8G8_SNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_SNORM, WImageFormat::R8_SNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_SINT, WImageFormat::R8G8_SINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_SINT, WImageFormat::R8_SINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::B8G8R8A8_UNORM, WImageFormat::B8G8R8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::B8G8R8A8_UNORM_SRGB, WImageFormat::B8G8R8_UNORM_SRGB, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::B8G8R8X8_UNORM, WImageFormat::B8G8R8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::B8G8R8X8_UNORM_SRGB, WImageFormat::B8G8R8_UNORM_SRGB, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16_FLOAT, WImageFormat::R16_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16_UNORM, WImageFormat::R16_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16_UINT, WImageFormat::R16_UINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16_SNORM, WImageFormat::R16_SNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16_SINT, WImageFormat::R16_SINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8_UNORM, WImageFormat::R8G8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8_UNORM, WImageFormat::R8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8_UNORM, WImageFormat::R8_UNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8_UINT, WImageFormat::R8_UINT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8_SNORM, WImageFormat::R8_SNORM, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8_SINT, WImageFormat::R8_SINT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat) const override
  {
    WUInt32 sourceStride = WImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    WUInt32 targetStride = WImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    if (WImageFormat::GetBitsPerPixel(sourceFormat) == 32 && WImageFormat::GetBitsPerPixel(targetFormat) == 24)
    {
      // Fast path for RGBA -> RGB
      while (uiNumElements)
      {
        const WUInt8* src = static_cast<const WUInt8*>(sourcePointer);
        WUInt8* dst = static_cast<WUInt8*>(targetPointer);

        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];

        sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
        targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
        uiNumElements--;
      }
    }

    while (uiNumElements)
    {
      memcpy(targetPointer, sourcePointer, targetStride);

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_FLOAT_to_R11G11B10 : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R32G32B32A32_FLOAT, WImageFormat::R11G11B10_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32_FLOAT, WImageFormat::R11G11B10_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat) const override
  {
    WUInt32 sourceStride = WImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    WUInt32 targetStride = WImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      // Adapted from DirectXMath's XMStoreFloat3PK
      WUInt32 IValue[3];
      memcpy(IValue, sourcePointer, 12);

      WUInt32 Result[3];

      // X & Y Channels (5-bit exponent, 6-bit mantissa)
      for (WUInt32 j = 0; j < 2; ++j)
      {
        WUInt32 Sign = IValue[j] & 0x80000000;
        WUInt32 I = IValue[j] & 0x7FFFFFFF;

        if ((I & 0x7F800000) == 0x7F800000)
        {
          // INF or NAN
          Result[j] = 0x7c0;
          if ((I & 0x7FFFFF) != 0)
          {
            Result[j] = 0x7c0 | (((I >> 17) | (I >> 11) | (I >> 6) | (I)) & 0x3f);
          }
          else if (Sign)
          {
            // -INF is clamped to 0 since 3PK is positive only
            Result[j] = 0;
          }
        }
        else if (Sign)
        {
          // 3PK is positive only, so clamp to zero
          Result[j] = 0;
        }
        else if (I > 0x477E0000U)
        {
          // The number is too large to be represented as a float11, set to max
          Result[j] = 0x7BF;
        }
        else
        {
          if (I < 0x38800000U)
          {
            // The number is too small to be represented as a normalized float11
            // Convert it to a denormalized value.
            WUInt32 Shift = 113U - (I >> 23U);
            I = (0x800000U | (I & 0x7FFFFFU)) >> Shift;
          }
          else
          {
            // Rebias the exponent to represent the value as a normalized float11
            I += 0xC8000000U;
          }

          Result[j] = ((I + 0xFFFFU + ((I >> 17U) & 1U)) >> 17U) & 0x7ffU;
        }
      }

      // Z Channel (5-bit exponent, 5-bit mantissa)
      WUInt32 Sign = IValue[2] & 0x80000000;
      WUInt32 I = IValue[2] & 0x7FFFFFFF;

      if ((I & 0x7F800000) == 0x7F800000)
      {
        // INF or NAN
        Result[2] = 0x3e0;
        if (I & 0x7FFFFF)
        {
          Result[2] = 0x3e0 | (((I >> 18) | (I >> 13) | (I >> 3) | (I)) & 0x1f);
        }
        else if (Sign)
        {
          // -INF is clamped to 0 since 3PK is positive only
          Result[2] = 0;
        }
      }
      else if (Sign)
      {
        // 3PK is positive only, so clamp to zero
        Result[2] = 0;
      }
      else if (I > 0x477C0000U)
      {
        // The number is too large to be represented as a float10, set to max
        Result[2] = 0x3df;
      }
      else
      {
        if (I < 0x38800000U)
        {
          // The number is too small to be represented as a normalized float10
          // Convert it to a denormalized value.
          WUInt32 Shift = 113U - (I >> 23U);
          I = (0x800000U | (I & 0x7FFFFFU)) >> Shift;
        }
        else
        {
          // Rebias the exponent to represent the value as a normalized float10
          I += 0xC8000000U;
        }

        Result[2] = ((I + 0x1FFFFU + ((I >> 18U) & 1U)) >> 18U) & 0x3ffU;
      }

      // Pack Result into memory
      *reinterpret_cast<WUInt32*>(targetPointer) = (Result[0] & 0x7ff) | ((Result[1] & 0x7ff) << 11) | ((Result[2] & 0x3ff) << 22);

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_R11G11B10_to_FLOAT : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R11G11B10_FLOAT, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R11G11B10_FLOAT, WImageFormat::R32G32B32_FLOAT, WImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat) const override
  {
    WUInt32 sourceStride = WImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    WUInt32 targetStride = WImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      const R11G11B10* pSource = reinterpret_cast<const R11G11B10*>(sourcePointer);
      WUInt32* targetUi = reinterpret_cast<WUInt32*>(targetPointer);

      // Adapted from XMLoadFloat3PK
      WUInt32 Mantissa;
      WUInt32 Exponent;

      // X Channel (6-bit mantissa)
      Mantissa = pSource->p.xm;

      if (pSource->p.xe == 0x1f) // INF or NAN
      {
        targetUi[0] = 0x7f800000 | (pSource->p.xm << 17);
      }
      else
      {
        if (pSource->p.xe != 0) // The value is normalized
        {
          Exponent = pSource->p.xe;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x40) == 0);

          Mantissa &= 0x3F;
        }
        else // The value is zero
        {
          Exponent = (WUInt32)-112;
        }

        targetUi[0] = ((Exponent + 112) << 23) | (Mantissa << 17);
      }

      // Y Channel (6-bit mantissa)
      Mantissa = pSource->p.ym;

      if (pSource->p.ye == 0x1f) // INF or NAN
      {
        targetUi[1] = 0x7f800000 | (pSource->p.ym << 17);
      }
      else
      {
        if (pSource->p.ye != 0) // The value is normalized
        {
          Exponent = pSource->p.ye;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x40) == 0);

          Mantissa &= 0x3F;
        }
        else // The value is zero
        {
          Exponent = (WUInt32)-112;
        }

        targetUi[1] = ((Exponent + 112) << 23) | (Mantissa << 17);
      }

      // Z Channel (5-bit mantissa)
      Mantissa = pSource->p.zm;

      if (pSource->p.ze == 0x1f) // INF or NAN
      {
        targetUi[2] = 0x7f800000 | (pSource->p.zm << 17);
      }
      else
      {
        if (pSource->p.ze != 0) // The value is normalized
        {
          Exponent = pSource->p.ze;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x20) == 0);

          Mantissa &= 0x1F;
        }
        else // The value is zero
        {
          Exponent = (WUInt32)-112;
        }

        targetUi[2] = ((Exponent + 112) << 23) | (Mantissa << 18);
      }

      if (targetStride > sizeof(float) * 3)
      {
        reinterpret_cast<float*>(targetPointer)[3] = 1.0f; // Write alpha channel
      }
      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};

class WImageConversion_R11G11B10_to_HALF : public WImageConversionStepLinear
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R11G11B10_FLOAT, WImageFormat::R16G16B16A16_FLOAT, WImageConversionFlags::Default)};
    return supportedConversions;
  }

  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat) const override
  {
    WUInt32 sourceStride = WImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    WUInt32 targetStride = WImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      WUInt16* result = reinterpret_cast<WUInt16*>(targetPointer);
      const R11G11B10* r11g11b10 = reinterpret_cast<const R11G11B10*>(sourcePointer);

      // We can do a straight forward conversion here because R11G11B10 uses the same number of bits for the exponent as a half
      // This means that all special values, e.g. denormals, inf, nan map exactly.
      result[0] = static_cast<WUInt16>((r11g11b10->p.xe << 10) | (r11g11b10->p.xm << 4));
      result[1] = static_cast<WUInt16>((r11g11b10->p.ye << 10) | (r11g11b10->p.ym << 4));
      result[2] = static_cast<WUInt16>((r11g11b10->p.ze << 10) | (r11g11b10->p.zm << 5));
      result[3] = 0x3C00; // hex value of 1.0f as half

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};


template <typename T>
class WImageConversion_Int_To_F32 : public WImageConversionStepLinear
{
public:
  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const override
  {
    W_IGNORE_UNUSED(sourceFormat);

    // Work with single channels instead of pixels
    uiNumElements *= WImageFormat::GetBitsPerPixel(targetFormat) / 32;

    const WUInt32 sourceStride = sizeof(T);
    const WUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = static_cast<float>(*reinterpret_cast<const T*>(sourcePointer));

      sourcePointer = WMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = WMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return W_SUCCESS;
  }
};


class WImageConversion_UINT8_F32 : public WImageConversion_Int_To_F32<WUInt8>
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R8_UINT, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8_UINT, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_UINT, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class WImageConversion_SINT8_F32 : public WImageConversion_Int_To_F32<WInt8>
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R8_SINT, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8_SINT, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R8G8B8A8_SINT, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class WImageConversion_UINT16_F32 : public WImageConversion_Int_To_F32<WUInt16>
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R16_UINT, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16_UINT, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_UINT, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class WImageConversion_SINT16_F32 : public WImageConversion_Int_To_F32<WInt16>
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R16_SINT, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16_SINT, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R16G16B16A16_SINT, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class WImageConversion_UINT32_F32 : public WImageConversion_Int_To_F32<WUInt32>
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R32_UINT, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32_UINT, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32_UINT, WImageFormat::R32G32B32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_UINT, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class WImageConversion_SINT32_F32 : public WImageConversion_Int_To_F32<WInt32>
{
public:
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const override
  {
    static WImageConversionEntry supportedConversions[] = {
      WImageConversionEntry(WImageFormat::R32_SINT, WImageFormat::R32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32_SINT, WImageFormat::R32G32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32_SINT, WImageFormat::R32G32B32_FLOAT, WImageConversionFlags::Default),
      WImageConversionEntry(WImageFormat::R32G32B32A32_SINT, WImageFormat::R32G32B32A32_FLOAT, WImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};


#define ADD_16BPP_CONVERSION(format)                                                                                                   \
  static WImageConversionStep_Decompress16bpp<WDecompress##format, WImageFormat::format##_UNORM> s_conversion_WDecompress##format; \
  static WImageConversionStep_Compress16bpp<WCompress##format, WImageFormat::format##_UNORM> s_conversion_WCompress##format

ADD_16BPP_CONVERSION(A4B4G4R4);
ADD_16BPP_CONVERSION(B4G4R4A4);
ADD_16BPP_CONVERSION(B5G6R5);
ADD_16BPP_CONVERSION(B5G5R5X1);
ADD_16BPP_CONVERSION(B5G5R5A1);
ADD_16BPP_CONVERSION(X1B5G5R5);
ADD_16BPP_CONVERSION(A1B5G5R5);

W_STATICLINK_FORCE
static WImageSwizzleConversion32_2103 s_conversion_swizzle2103;
static WImageConversion_BGRX_BGRA s_conversion_BGRX_BGRA;
static WImageConversion_F32_U8 s_conversion_F32_U8;
static WImageConversion_F32_sRGB s_conversion_F32_sRGB;
static WImageConversion_F32_U16 s_conversion_F32_U16;
static WImageConversion_F32_F16 s_conversion_F32_F16;
static WImageConversion_F32_S8 s_conversion_F32_S8;
static WImageConversion_U8_F32 s_conversion_U8_F32;
static WImageConversion_sRGB_F32 s_conversion_sRGB_F32;
static WImageConversion_U16_F32 s_conversion_U16_F32;
static WImageConversion_S16_F32 s_conversion_S16_F32;
static WImageConversion_F16_F32 s_conversion_F16_F32;
static WImageConversion_S8_F32 s_conversion_S8_F32;
static WImageConversion_UINT8_F32 s_conversion_UINT8_F32;
static WImageConversion_SINT8_F32 s_conversion_SINT8_F32;
static WImageConversion_UINT16_F32 s_conversion_UINT16_F32;
static WImageConversion_SINT16_F32 s_conversion_SINT16_F32;
static WImageConversion_UINT32_F32 s_conversion_UINT32_F32;
static WImageConversion_SINT32_F32 s_conversion_SINT32_F32;

static WImageConversion_Pad_To_RGBA_U8 s_conversion_Pad_To_RGBA_U8;
static WImageConversion_Pad_To_RGBA_F32 s_conversion_Pad_To_RGBA_F32;
static WImageConversion_DiscardChannels s_conversion_DiscardChannels;

static WImageConversion_R11G11B10_to_FLOAT s_conversion_R11G11B10_to_FLOAT;
static WImageConversion_R11G11B10_to_HALF s_conversion_R11G11B10_to_HALF;
static WImageConversion_FLOAT_to_R11G11B10 s_conversion_FLOAT_to_R11G11B10;



W_STATICLINK_FILE(Texture, Texture_Image_Conversions_PixelConversions);
