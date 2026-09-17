
#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

/// Color space conversion modes for vertex colors.
struct WMeshVertexColorConversion
{
  using StorageType = WUInt8;

  enum Enum
  {
    None,         ///< No conversion applied.
    LinearToSrgb, ///< Convert from linear to sRGB color space.
    SrgbToLinear, ///< Convert from sRGB to linear color space.

    Default = None
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WMeshVertexColorConversion);

/// Utility functions for encoding and decoding mesh vertex attributes.
///
/// Provides conversion between high-level types (WVec3, WColor, etc.) and various
/// GPU buffer formats. Handles normals, tangents, texture coordinates, bone weights,
/// and vertex colors with optional color space conversion.
struct W_RENDERERCORE_DLL WMeshBufferUtils
{
  /// Encodes a normal vector to the specified GPU buffer format.
  static WResult EncodeNormal(const WVec3& vNormal, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat);

  /// Encodes a tangent vector and bitangent sign to the specified GPU buffer format.
  ///
  /// The tangent sign is typically used to reconstruct the bitangent in the shader.
  static WResult EncodeTangent(const WVec3& vTangent, float fTangentSign, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat);

  /// Encodes texture coordinates to the specified GPU buffer format.
  static WResult EncodeTexCoord(const WVec2& vTexCoord, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat);

  /// Encodes bone weights for skinned meshes to the specified GPU buffer format.
  static WResult EncodeBoneWeights(const WVec4& vWeights, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat);

  /// Encodes a vertex color with optional color space conversion.
  static WResult EncodeColor(const WColor& color, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat, WMeshVertexColorConversion::Enum conversion);


  /// Decodes a normal vector from the specified GPU buffer format.
  static WResult DecodeNormal(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec3& out_vDestNormal);

  /// Decodes a tangent vector and bitangent sign from the specified GPU buffer format.
  static WResult DecodeTangent(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec3& out_vDestTangent, float& out_fDestBiTangentSign);

  /// Decodes texture coordinates from the specified GPU buffer format.
  static WResult DecodeTexCoord(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec2& out_vDestTexCoord);

  /// Decodes bone weights from the specified GPU buffer format.
  static WResult DecodeBoneWeights(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec4& out_vDestWeights);

  /// Decodes a vertex color from the specified GPU buffer format.
  static WResult DecodeColor(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WColor& out_destColor);


  /// Low-level function to encode a float value to the specified GPU buffer format.
  static WResult EncodeFromFloat(const float fSource, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat);

  /// Low-level function to encode a 2D vector to the specified GPU buffer format.
  static WResult EncodeFromVec2(const WVec2& vSource, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat);

  /// Low-level function to encode a 3D vector to the specified GPU buffer format.
  static WResult EncodeFromVec3(const WVec3& vSource, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat);

  /// Low-level function to encode a 4D vector to the specified GPU buffer format.
  static WResult EncodeFromVec4(const WVec4& vSource, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat);

  /// Low-level function to decode a float value from the specified GPU buffer format.
  static WResult DecodeToFloat(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, float& out_fDest);


  /// Low-level function to decode a 2D vector from the specified GPU buffer format.
  static WResult DecodeToVec2(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec2& out_vDest);

  /// Low-level function to decode a 3D vector from the specified GPU buffer format.
  static WResult DecodeToVec3(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec3& out_vDest);

  /// Low-level function to decode a 4D vector from the specified GPU buffer format.
  static WResult DecodeToVec4(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec4& out_vDest);
};

#include <RendererCore/Meshes/Implementation/MeshBufferUtils_inl.h>
