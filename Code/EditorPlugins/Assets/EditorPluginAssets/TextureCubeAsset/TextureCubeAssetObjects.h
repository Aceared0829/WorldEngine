#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <Texture/TexConv/TexConvEnums.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct WPropertyMetaStateEvent;

struct WTextureCubeChannelMappingEnum
{
  using StorageType = WInt8;

  enum Enum
  {
    RGB1,
    RGBA1,

    RGB1TO6,
    RGBA1TO6,

    Default = RGB1,
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WTextureCubeChannelMappingEnum);


class WTextureCubeAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WTextureCubeAssetProperties, WReflectedClass);

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
  void SetInputFile4(const char* szFile) { m_Input[4] = szFile; }
  const char* GetInputFile4() const { return m_Input[4]; }
  void SetInputFile5(const char* szFile) { m_Input[5] = szFile; }
  const char* GetInputFile5() const { return m_Input[5]; }

  WString GetAbsoluteInputFilePath(WInt32 iInput) const;
  WInt32 GetNumInputFiles() const;

  WEnum<WTexConvCompressionMode> m_CompressionMode;
  WEnum<WTexConvMipmapMode> m_MipmapMode;

  WEnum<WTextureFilterSetting> m_TextureFilter;
  WEnum<WTexConvUsage> m_TextureUsage;
  WEnum<WTextureCubeChannelMappingEnum> m_ChannelMapping;

  float m_fHdrExposureBias = 0;

private:
  WString m_Input[6];
};
