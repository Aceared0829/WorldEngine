

// static
W_ALWAYS_INLINE WResult WMeshBufferUtils::EncodeNormal(const WVec3& vNormal, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat)
{
  // we store normals in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec3(vNormal * 0.5f + WVec3(0.5f), dest, destFormat);
}

// static
W_ALWAYS_INLINE WResult WMeshBufferUtils::EncodeTangent(const WVec3& vTangent, float fTangentSign, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat)
{
  // make sure biTangentSign is either -1 or 1
  fTangentSign = (fTangentSign < 0.0f) ? -1.0f : 1.0f;

  // we store tangents in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec4(vTangent.GetAsVec4(fTangentSign) * 0.5f + WVec4(0.5f), dest, destFormat);
}

// static
W_ALWAYS_INLINE WResult WMeshBufferUtils::EncodeTexCoord(const WVec2& vTexCoord, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat)
{
  return EncodeFromVec2(vTexCoord, dest, destFormat);
}

// static
W_ALWAYS_INLINE WResult WMeshBufferUtils::EncodeBoneWeights(const WVec4& vWeights, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat)
{
  return EncodeFromVec4(vWeights, dest, destFormat);
}

// static
W_ALWAYS_INLINE WResult WMeshBufferUtils::EncodeColor(const WColor& color, WByteArrayPtr dest, WGALResourceFormat::Enum destFormat, WMeshVertexColorConversion::Enum conversion)
{
  WVec4 finalColor = color.GetAsVec4();
  if (conversion == WMeshVertexColorConversion::LinearToSrgb)
  {
    finalColor = WColor::LinearToGamma(finalColor.GetAsVec3()).GetAsVec4(finalColor.w);
  }
  else if (conversion == WMeshVertexColorConversion::SrgbToLinear)
  {
    finalColor = WColor::GammaToLinear(finalColor.GetAsVec3()).GetAsVec4(finalColor.w);
  }

  return EncodeFromVec4(finalColor, dest, destFormat);
}

// static
W_ALWAYS_INLINE WResult WMeshBufferUtils::DecodeNormal(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec3& out_vDestNormal)
{
  WVec3 tempNormal;
  W_SUCCEED_OR_RETURN(DecodeToVec3(source, sourceFormat, tempNormal));
  out_vDestNormal = tempNormal * 2.0f - WVec3(1.0f);
  return W_SUCCESS;
}

// static
W_ALWAYS_INLINE WResult WMeshBufferUtils::DecodeTangent(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec3& out_vDestTangent, float& out_fDestBiTangentSign)
{
  WVec4 tempTangent;
  W_SUCCEED_OR_RETURN(DecodeToVec4(source, sourceFormat, tempTangent));
  out_vDestTangent = tempTangent.GetAsVec3() * 2.0f - WVec3(1.0f);
  out_fDestBiTangentSign = tempTangent.w * 2.0f - 1.0f;
  return W_SUCCESS;
}

// static
W_ALWAYS_INLINE WResult WMeshBufferUtils::DecodeTexCoord(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec2& out_vDestTexCoord)
{
  return DecodeToVec2(source, sourceFormat, out_vDestTexCoord);
}

// static
W_ALWAYS_INLINE WResult WMeshBufferUtils::DecodeBoneWeights(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WVec4& out_vDestWeights)
{
  return DecodeToVec4(source, sourceFormat, out_vDestWeights);
}

// static
W_ALWAYS_INLINE WResult WMeshBufferUtils::DecodeColor(WConstByteArrayPtr source, WGALResourceFormat::Enum sourceFormat, WColor& out_destColor)
{
  WVec4 res;
  W_SUCCEED_OR_RETURN(DecodeToVec4(source, sourceFormat, res));
  out_destColor = WColor(res.x, res.y, res.z, res.w);
  return W_SUCCESS;
}
