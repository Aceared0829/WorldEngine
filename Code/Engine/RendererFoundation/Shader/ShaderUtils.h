#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

class WShaderUtils
{
public:
  W_ALWAYS_INLINE static WUInt32 Float3ToRGB10(WVec3 value)
  {
    const WVec3 unsignedValue = value * 0.5f + WVec3(0.5f);

    const WUInt32 r = WMath::ColorFloatToUnsignedInt<10>(unsignedValue.x);
    const WUInt32 g = WMath::ColorFloatToUnsignedInt<10>(unsignedValue.y);
    const WUInt32 b = WMath::ColorFloatToUnsignedInt<10>(unsignedValue.z);

    return r | (g << 10) | (b << 20);
  }

  W_ALWAYS_INLINE static WUInt32 PackFloat16intoUint(WFloat16 x, WFloat16 y)
  {
    const WUInt32 r = x.GetRawData();
    const WUInt32 g = y.GetRawData();

    return r | (g << 16);
  }

  W_ALWAYS_INLINE static WUInt32 Float2ToRG16F(WVec2 value)
  {
    const WUInt32 r = WFloat16(value.x).GetRawData();
    const WUInt32 g = WFloat16(value.y).GetRawData();

    return r | (g << 16);
  }

  W_ALWAYS_INLINE static void Float4ToRGBA16F(WVec4 value, WUInt32& out_uiRG, WUInt32& out_uiBA)
  {
    out_uiRG = Float2ToRG16F(WVec2(value.x, value.y));
    out_uiBA = Float2ToRG16F(WVec2(value.z, value.w));
  }

  enum class WBuiltinShaderType
  {
    CopyImage,
    CopyImageArray,
    DownscaleImage,
    DownscaleImageArray,
  };

  struct WBuiltinShader
  {
    WGALShaderHandle m_hActiveGALShader;
    WGALBlendStateHandle m_hBlendState;
    WGALDepthStencilStateHandle m_hDepthStencilState;
    WGALRasterizerStateHandle m_hRasterizerState;
  };

  W_RENDERERFOUNDATION_DLL static WDelegate<void(WBuiltinShaderType type, WBuiltinShader& out_shader)> g_RequestBuiltinShaderCallback;

  W_ALWAYS_INLINE static void RequestBuiltinShader(WBuiltinShaderType type, WBuiltinShader& out_shader)
  {
    g_RequestBuiltinShaderCallback(type, out_shader);
  }
};
W_DEFINE_AS_POD_TYPE(WShaderUtils::WBuiltinShaderType);
