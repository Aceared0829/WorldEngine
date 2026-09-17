#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

/// Tests texture readback from the GPU
///
/// Only renderable textures are tested.
/// The test first renders a pattern into the 8x8 texture.
/// Then the texture is read back and uploaded again to another texture.
/// The readback result is converted to RGBA32float and compared to a reference image. As these only change depending on channel count the MapImageNumberToString function is overwritten to always point to the same images.
/// Finally the original an re-uploaded texture is rendered to the screen and the result is again compared to a reference image.
///
/// The subtest list is dynamic, only formats that support render and sample are tested.
class WRendererTestReadback : public WGraphicsTest
{
  using SUPER = WGraphicsTest;

public:
  virtual const char* GetTestName() const override { return "Readback"; }

private:
  virtual void SetupSubTests() override;

  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult GetImage(WImage& ref_img, const WSubTestEntry& subTest, WUInt32 uiImageNumber) override;
  virtual void MapImageNumberToString(const char* szTestName, const WSubTestEntry& subTest, WUInt32 uiImageNumber, WStringBuilder& out_sString) const override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;
  WTestAppRun Readback(WUInt32 uiInvocationCount);

  void CompareReadbackImage(WImage&& image);
  void CompareUploadImage();

private:
  WDynamicArray<WEnum<WGALResourceFormat>> m_TestableFormats;
  WDynamicArray<WString> m_TestableFormatStrings;

  WGALResourceFormat::Enum m_Format = WGALResourceFormat::Invalid;
  WShaderResourceHandle m_hUVColorShader;
  WShaderResourceHandle m_hUVColorIntShader;
  WShaderResourceHandle m_hUVColorUIntShader;
  WShaderResourceHandle m_hUVColorDepthShader;
  WShaderResourceHandle m_hTexture2DShader;
  WShaderResourceHandle m_hTexture2DDepthShader;
  WShaderResourceHandle m_hTexture2DIntShader;
  WShaderResourceHandle m_hTexture2DUIntShader;
  WGALTextureHandle m_hTexture2DReadback;
  WGALTextureHandle m_hTexture2DUpload;
  mutable WImage m_ReadBackResult;
  mutable WString m_sReadBackReferenceImage;
  bool m_bReadbackInProgress = true;
  WGALReadbackTextureHelper m_Readback;
};
