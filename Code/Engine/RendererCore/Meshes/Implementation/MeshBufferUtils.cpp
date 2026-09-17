#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WMeshVertexColorConversion, 1)
  W_ENUM_CONSTANT(WMeshVertexColorConversion::None),
  W_ENUM_CONSTANT(WMeshVertexColorConversion::LinearToSrgb),
  W_ENUM_CONSTANT(WMeshVertexColorConversion::SrgbToLinear),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
WResult WMeshBufferUtils::EncodeFromFloat(const float fSource, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat)
{
  W_ASSERT_DEBUG(dest.GetCount() >= WGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case WGALResourceFormat::RFloat:
      *reinterpret_cast<float*>(dest.GetPtr()) = fSource;
      return W_SUCCESS;
    case WGALResourceFormat::RHalf:
      *reinterpret_cast<WFloat16*>(dest.GetPtr()) = fSource;
      return W_SUCCESS;
    default:
      return W_FAILURE;
  }
}

// static
WResult WMeshBufferUtils::EncodeFromVec2(const WVec2& vSource, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat)
{
  W_ASSERT_DEBUG(dest.GetCount() >= WGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case WGALResourceFormat::RGFloat:
      *reinterpret_cast<WVec2*>(dest.GetPtr()) = vSource;
      return W_SUCCESS;

    case WGALResourceFormat::RGHalf:
      *reinterpret_cast<WFloat16Vec2*>(dest.GetPtr()) = vSource;
      return W_SUCCESS;

    default:
      return W_FAILURE;
  }
}

// static
WResult WMeshBufferUtils::EncodeFromVec3(const WVec3& vSource, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat)
{
  W_ASSERT_DEBUG(dest.GetCount() >= WGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case WGALResourceFormat::RGBFloat:
      *reinterpret_cast<WVec3*>(dest.GetPtr()) = vSource;
      return W_SUCCESS;

    case WGALResourceFormat::RGBAUShortNormalized:
      reinterpret_cast<WUInt16*>(dest.GetPtr())[0] = WMath::ColorFloatToShort(vSource.x);
      reinterpret_cast<WUInt16*>(dest.GetPtr())[1] = WMath::ColorFloatToShort(vSource.y);
      reinterpret_cast<WUInt16*>(dest.GetPtr())[2] = WMath::ColorFloatToShort(vSource.z);
      reinterpret_cast<WUInt16*>(dest.GetPtr())[3] = 0;
      return W_SUCCESS;

    case WGALResourceFormat::RGBAShortNormalized:
      reinterpret_cast<WInt16*>(dest.GetPtr())[0] = WMath::ColorFloatToSignedShort(vSource.x);
      reinterpret_cast<WInt16*>(dest.GetPtr())[1] = WMath::ColorFloatToSignedShort(vSource.y);
      reinterpret_cast<WInt16*>(dest.GetPtr())[2] = WMath::ColorFloatToSignedShort(vSource.z);
      reinterpret_cast<WInt16*>(dest.GetPtr())[3] = 0;
      return W_SUCCESS;

    case WGALResourceFormat::RGB10A2UIntNormalized:
      *reinterpret_cast<WUInt32*>(dest.GetPtr()) = WMath::ColorFloatToUnsignedInt<10>(vSource.x);
      *reinterpret_cast<WUInt32*>(dest.GetPtr()) |= WMath::ColorFloatToUnsignedInt<10>(vSource.y) << 10;
      *reinterpret_cast<WUInt32*>(dest.GetPtr()) |= WMath::ColorFloatToUnsignedInt<10>(vSource.z) << 20;
      return W_SUCCESS;

    case WGALResourceFormat::RGBAUByteNormalized:
      dest.GetPtr()[0] = WMath::ColorFloatToByte(vSource.x);
      dest.GetPtr()[1] = WMath::ColorFloatToByte(vSource.y);
      dest.GetPtr()[2] = WMath::ColorFloatToByte(vSource.z);
      dest.GetPtr()[3] = 0;
      return W_SUCCESS;

    case WGALResourceFormat::RGBAByteNormalized:
      dest.GetPtr()[0] = WMath::ColorFloatToSignedByte(vSource.x);
      dest.GetPtr()[1] = WMath::ColorFloatToSignedByte(vSource.y);
      dest.GetPtr()[2] = WMath::ColorFloatToSignedByte(vSource.z);
      dest.GetPtr()[3] = 0;
      return W_SUCCESS;
    default:
      return W_FAILURE;
  }
}

// static
WResult WMeshBufferUtils::EncodeFromVec4(const WVec4& vSource, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat)
{
  W_ASSERT_DEBUG(dest.GetCount() >= WGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case WGALResourceFormat::RGBAFloat:
      *reinterpret_cast<WVec4*>(dest.GetPtr()) = vSource;
      return W_SUCCESS;

    case WGALResourceFormat::RGBAHalf:
      *reinterpret_cast<WFloat16Vec4*>(dest.GetPtr()) = vSource;
      return W_SUCCESS;

    case WGALResourceFormat::RGBAUShortNormalized:
      reinterpret_cast<WUInt16*>(dest.GetPtr())[0] = WMath::ColorFloatToShort(vSource.x);
      reinterpret_cast<WUInt16*>(dest.GetPtr())[1] = WMath::ColorFloatToShort(vSource.y);
      reinterpret_cast<WUInt16*>(dest.GetPtr())[2] = WMath::ColorFloatToShort(vSource.z);
      reinterpret_cast<WUInt16*>(dest.GetPtr())[3] = WMath::ColorFloatToShort(vSource.w);
      return W_SUCCESS;

    case WGALResourceFormat::RGBAShortNormalized:
      reinterpret_cast<WInt16*>(dest.GetPtr())[0] = WMath::ColorFloatToSignedShort(vSource.x);
      reinterpret_cast<WInt16*>(dest.GetPtr())[1] = WMath::ColorFloatToSignedShort(vSource.y);
      reinterpret_cast<WInt16*>(dest.GetPtr())[2] = WMath::ColorFloatToSignedShort(vSource.z);
      reinterpret_cast<WInt16*>(dest.GetPtr())[3] = WMath::ColorFloatToSignedShort(vSource.w);
      return W_SUCCESS;

    case WGALResourceFormat::RGB10A2UIntNormalized:
      *reinterpret_cast<WUInt32*>(dest.GetPtr()) = WMath::ColorFloatToUnsignedInt<10>(vSource.x);
      *reinterpret_cast<WUInt32*>(dest.GetPtr()) |= WMath::ColorFloatToUnsignedInt<10>(vSource.y) << 10;
      *reinterpret_cast<WUInt32*>(dest.GetPtr()) |= WMath::ColorFloatToUnsignedInt<10>(vSource.z) << 20;
      *reinterpret_cast<WUInt32*>(dest.GetPtr()) |= WMath::ColorFloatToUnsignedInt<2>(vSource.w) << 30;
      return W_SUCCESS;

    case WGALResourceFormat::RGBAUByteNormalized:
      dest.GetPtr()[0] = WMath::ColorFloatToByte(vSource.x);
      dest.GetPtr()[1] = WMath::ColorFloatToByte(vSource.y);
      dest.GetPtr()[2] = WMath::ColorFloatToByte(vSource.z);
      dest.GetPtr()[3] = WMath::ColorFloatToByte(vSource.w);
      return W_SUCCESS;

    case WGALResourceFormat::RGBAByteNormalized:
      dest.GetPtr()[0] = WMath::ColorFloatToSignedByte(vSource.x);
      dest.GetPtr()[1] = WMath::ColorFloatToSignedByte(vSource.y);
      dest.GetPtr()[2] = WMath::ColorFloatToSignedByte(vSource.z);
      dest.GetPtr()[3] = WMath::ColorFloatToSignedByte(vSource.w);
      return W_SUCCESS;

    default:
      return W_FAILURE;
  }
}

// static
WResult WMeshBufferUtils::DecodeToFloat(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, float& out_fDest)
{
  W_ASSERT_DEBUG(source.GetCount() >= WGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case WGALResourceFormat::RFloat:
      out_fDest = *reinterpret_cast<const float*>(source.GetPtr());
      return W_SUCCESS;
    case WGALResourceFormat::RHalf:
      out_fDest = *reinterpret_cast<const WFloat16*>(source.GetPtr());
      return W_SUCCESS;
    default:
      return W_FAILURE;
  }
}

// static
WResult WMeshBufferUtils::DecodeToVec2(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec2& out_vDest)
{
  W_ASSERT_DEBUG(source.GetCount() >= WGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case WGALResourceFormat::RGFloat:
      out_vDest = *reinterpret_cast<const WVec2*>(source.GetPtr());
      return W_SUCCESS;
    case WGALResourceFormat::RGHalf:
      out_vDest = *reinterpret_cast<const WFloat16Vec2*>(source.GetPtr());
      return W_SUCCESS;
    default:
      return W_FAILURE;
  }
}

// static
WResult WMeshBufferUtils::DecodeToVec3(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec3& out_vDest)
{
  W_ASSERT_DEBUG(source.GetCount() >= WGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case WGALResourceFormat::RGBFloat:
      out_vDest = *reinterpret_cast<const WVec3*>(source.GetPtr());
      return W_SUCCESS;

    case WGALResourceFormat::RGBAUShortNormalized:
      out_vDest.x = WMath::ColorShortToFloat(reinterpret_cast<const WUInt16*>(source.GetPtr())[0]);
      out_vDest.y = WMath::ColorShortToFloat(reinterpret_cast<const WUInt16*>(source.GetPtr())[1]);
      out_vDest.z = WMath::ColorShortToFloat(reinterpret_cast<const WUInt16*>(source.GetPtr())[2]);
      return W_SUCCESS;

    case WGALResourceFormat::RGBAShortNormalized:
      out_vDest.x = WMath::ColorSignedShortToFloat(reinterpret_cast<const WInt16*>(source.GetPtr())[0]);
      out_vDest.y = WMath::ColorSignedShortToFloat(reinterpret_cast<const WInt16*>(source.GetPtr())[1]);
      out_vDest.z = WMath::ColorSignedShortToFloat(reinterpret_cast<const WInt16*>(source.GetPtr())[2]);
      return W_SUCCESS;

    case WGALResourceFormat::RGB10A2UIntNormalized:
      out_vDest.x = WMath::ColorUnsignedIntToFloat<10>(*reinterpret_cast<const WUInt32*>(source.GetPtr()));
      out_vDest.y = WMath::ColorUnsignedIntToFloat<10>(*reinterpret_cast<const WUInt32*>(source.GetPtr()) >> 10);
      out_vDest.z = WMath::ColorUnsignedIntToFloat<10>(*reinterpret_cast<const WUInt32*>(source.GetPtr()) >> 20);
      return W_SUCCESS;

    case WGALResourceFormat::RGBAUByteNormalized:
      out_vDest.x = WMath::ColorByteToFloat(source.GetPtr()[0]);
      out_vDest.y = WMath::ColorByteToFloat(source.GetPtr()[1]);
      out_vDest.z = WMath::ColorByteToFloat(source.GetPtr()[2]);
      return W_SUCCESS;

    case WGALResourceFormat::RGBAByteNormalized:
      out_vDest.x = WMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      out_vDest.y = WMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      out_vDest.z = WMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      return W_SUCCESS;
    default:
      return W_FAILURE;
  }
}

// static
WResult WMeshBufferUtils::DecodeToVec4(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec4& out_vDest)
{
  W_ASSERT_DEBUG(source.GetCount() >= WGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case WGALResourceFormat::RGBAFloat:
      out_vDest = *reinterpret_cast<const WVec4*>(source.GetPtr());
      return W_SUCCESS;

    case WGALResourceFormat::RGBAHalf:
      out_vDest = *reinterpret_cast<const WFloat16Vec4*>(source.GetPtr());
      return W_SUCCESS;

    case WGALResourceFormat::RGBAUShortNormalized:
      out_vDest.x = WMath::ColorShortToFloat(reinterpret_cast<const WUInt16*>(source.GetPtr())[0]);
      out_vDest.y = WMath::ColorShortToFloat(reinterpret_cast<const WUInt16*>(source.GetPtr())[1]);
      out_vDest.z = WMath::ColorShortToFloat(reinterpret_cast<const WUInt16*>(source.GetPtr())[2]);
      out_vDest.w = WMath::ColorShortToFloat(reinterpret_cast<const WUInt16*>(source.GetPtr())[3]);
      return W_SUCCESS;

    case WGALResourceFormat::RGBAShortNormalized:
      out_vDest.x = WMath::ColorSignedShortToFloat(reinterpret_cast<const WInt16*>(source.GetPtr())[0]);
      out_vDest.y = WMath::ColorSignedShortToFloat(reinterpret_cast<const WInt16*>(source.GetPtr())[1]);
      out_vDest.z = WMath::ColorSignedShortToFloat(reinterpret_cast<const WInt16*>(source.GetPtr())[2]);
      out_vDest.w = WMath::ColorSignedShortToFloat(reinterpret_cast<const WInt16*>(source.GetPtr())[3]);
      return W_SUCCESS;

    case WGALResourceFormat::RGB10A2UIntNormalized:
      out_vDest.x = WMath::ColorUnsignedIntToFloat<10>(*reinterpret_cast<const WUInt32*>(source.GetPtr()));
      out_vDest.y = WMath::ColorUnsignedIntToFloat<10>(*reinterpret_cast<const WUInt32*>(source.GetPtr()) >> 10);
      out_vDest.z = WMath::ColorUnsignedIntToFloat<10>(*reinterpret_cast<const WUInt32*>(source.GetPtr()) >> 20);
      out_vDest.w = WMath::ColorUnsignedIntToFloat<2>(*reinterpret_cast<const WUInt32*>(source.GetPtr()) >> 30);
      return W_SUCCESS;

    case WGALResourceFormat::RGBAUByteNormalized:
      out_vDest.x = WMath::ColorByteToFloat(source.GetPtr()[0]);
      out_vDest.y = WMath::ColorByteToFloat(source.GetPtr()[1]);
      out_vDest.z = WMath::ColorByteToFloat(source.GetPtr()[2]);
      out_vDest.w = WMath::ColorByteToFloat(source.GetPtr()[3]);
      return W_SUCCESS;

    case WGALResourceFormat::RGBAByteNormalized:
      out_vDest.x = WMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      out_vDest.y = WMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      out_vDest.z = WMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      out_vDest.w = WMath::ColorSignedByteToFloat(source.GetPtr()[3]);
      return W_SUCCESS;

    default:
      return W_FAILURE;
  }
}

W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshBufferUtils);
