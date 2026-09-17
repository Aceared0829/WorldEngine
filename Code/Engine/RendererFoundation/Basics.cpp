#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>

const WUInt8 WGALIndexType::s_Size[WGALIndexType::ENUM_COUNT] = {
  0,               // None
  sizeof(WInt16), // UShort
  sizeof(WInt32)  // UInt
};

const char* WGALShaderStage::Names[ENUM_COUNT] = {
  "VertexShader",
  "HullShader",
  "DomainShader",
  "GeometryShader",
  "PixelShader",
  "ComputeShader",
};
