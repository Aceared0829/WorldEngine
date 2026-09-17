#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <Texture/TexConv/TexConvEnums.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct WPropertyMetaStateEvent;

struct WTexture2DChannelMappingEnum
{
  using StorageType = WInt8;

  enum Enum
  {
    R1,
    R1_ALPHA,

    RG1,
    R1_G2,

    RGB1,
    R1_G2_B3,

    RGBA1,
    RGB1_A2,
    RGB1_ABLACK,
    R1_G2_B3_A4,

    // 'mask' textures: RGB is a constant white, the mask itself ends up in the alpha channel.
    // Ideally this would be a single channel texture combined with a GPU side texture swizzle,
    // but D3D11 has no support for component swizzles, so for now the white is baked in.
    RGBWHITE_A1,
    RGBWHITE_R1,

    Default = RGB1,
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WTexture2DChannelMappingEnum);

/// Channel mapping for array textures.
///
/// Unlike WTexture2DChannelMappingEnum, these modes can't combine multiple input files,
/// since every input file becomes one slice of the array. They only select which channels
/// of each input are written to the output.
struct WTextureArrayChannelMappingEnum
{
  using StorageType = WInt8;

  enum Enum
  {
    RGBA,      ///< Take all four channels as they are.
    RGB,       ///< Take RGB, discard alpha.
    RG,        ///< Take red and green.
    R_Red,     ///< Single channel output, taken from the input's red channel.
    R_Green,   ///< Single channel output, taken from the input's green channel.
    R_Blue,    ///< Single channel output, taken from the input's blue channel.
    R_Alpha,   ///< Single channel output, taken from the input's alpha channel.

    Default = RGBA,
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WTextureArrayChannelMappingEnum);

struct WTexture2DResolution
{
  using StorageType = WInt8;

  enum Enum
  {
    Fixed64x64,
    Fixed128x128,
    Fixed256x256,
    Fixed512x512,
    Fixed1024x1024,
    Fixed2048x2048,
    CVarRtResolution1,
    CVarRtResolution2,

    Default = Fixed256x256
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WTexture2DResolution);

struct WRenderTargetFormat
{
  using StorageType = WInt8;

  enum Enum
  {
    RGBA8sRgb,
    RGBA8,
    RGB10,
    RGBA16,
    R8,
    R16,
    R32,
    RG8,
    RG16,
    RG32,

    Default = RGBA8sRgb
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WRenderTargetFormat);

class WTextureAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WTextureAssetProperties, WReflectedClass);

public:
  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

  const char* GetInputFile(WInt32 iInput) const { return m_Input[iInput]; }

  void SetInputFile0(const char* szFile) { m_Input[0] = szFile; }
  const char* GetInputFile0() const { return m_Input[0]; }
  void SetInputFile1(const char* szFile) { m_Input[1] = szFile; }
  const char* GetInputFile1() const { return m_Input[1]; }
  void SetInputFile2(const char* szFile) { m_Input[2] = szFile; }
  const char* GetInputFile2() const { return m_Input[2]; }
  void SetInputFile3(const char* szFile) { m_Input[3] = szFile; }
  const char* GetInputFile3() const { return m_Input[3]; }

  WString GetAbsoluteInputFilePath(WInt32 iInput) const;

  WTexture2DChannelMappingEnum::Enum GetChannelMapping() const { return m_ChannelMapping; }
  WTextureArrayChannelMappingEnum::Enum GetArrayChannelMapping() const { return m_ArrayChannelMapping; }

  WInt32 GetNumInputFiles() const;

  bool m_bIsRenderTarget = false;
  bool m_bIsArrayTexture = false;
  bool m_bPremultipliedAlpha = false;
  bool m_bFlipHorizontal = false;
  bool m_bDilateColor = false;
  bool m_bPreserveAlphaCoverage = false;
  float m_fCVarResolutionScale = 1.0f;
  float m_fHdrExposureBias = 0;
  float m_fAlphaThreshold = 0.25f;

  WEnum<WTextureFilterSetting> m_TextureFilter;
  WEnum<WImageAddressMode> m_AddressModeU;
  WEnum<WImageAddressMode> m_AddressModeV;
  WEnum<WImageAddressMode> m_AddressModeW;
  WEnum<WTexture2DResolution> m_Resolution;
  WEnum<WTexConvUsage> m_TextureUsage;
  WEnum<WRenderTargetFormat> m_RtFormat;

  WEnum<WTexConvCompressionMode> m_CompressionMode;
  WEnum<WTexConvMipmapMode> m_MipmapMode;

  WDynamicArray<WString> m_ArraySlices;

private:
  WEnum<WTexture2DChannelMappingEnum> m_ChannelMapping;
  WEnum<WTextureArrayChannelMappingEnum> m_ArrayChannelMapping;
  WString m_Input[4];
};
