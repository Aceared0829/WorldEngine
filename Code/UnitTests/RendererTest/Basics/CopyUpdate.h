#pragma once

#include "../TestClass/TestClass.h"

/// Tests buffer and texture copy and update operations on the GAL command encoder.
///
/// Each sub-test creates source and/or destination resources, issues the operation
/// under test, reads back the destination and binary-compares the readback bytes
/// against the expected CPU data. No image comparison is needed since the result
/// is validated directly on the bytes.
class WRendererTestCopyUpdate : public WGraphicsTest
{
  using SUPER = WGraphicsTest;

public:
  virtual const char* GetTestName() const override { return "CopyUpdate"; }

private:
  enum SubTests
  {
    ST_CopyBuffer,
    ST_CopyTexture,
    ST_CopyTextureArray,
    ST_CopyTextureCube,
    ST_CopyTextureCubeArray,
    ST_CopyTextureNpot,
    ST_CopyTextureBC1,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  void RunCopyBuffer();
  void RunCopyTextureRegion(WInt32 iIdentifier);
  void RunCopyTexture();
  void RunUpdateTexture(WInt32 iIdentifier);
  void RunUpdateTextureForNextFrame(WInt32 iIdentifier);

  /// Initializes a texture source + destination pair. Texture2D is treated as a one-layer texture.
  void SetupTextureCopyUpdatePair(
    WGALTextureType::Enum type, WUInt32 uiArraySize, WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiMipLevels, WUInt32 uiPaddingBytes, WGALResourceFormat::Enum format = WGALResourceFormat::BGRAUByteNormalizedsRGB);

  void VerifyBufferReadback(WGALBufferHandle hBuffer, WArrayPtr<const WUInt8> expected);
  /// Reads back all slices and mips of a texture and compares each against expectedLayers[slice].
  void VerifyTextureSliceReadback(WGALTextureHandle hTexture, WArrayPtr<const WImage> expectedLayers);

  /// Width and height for the power-of-two texture sub-tests. 8x8 keeps the data small and easy to inspect.
  static constexpr WUInt32 s_uiTextureSize = 8;
  static constexpr WUInt32 s_uiNpotTextureWidth = 10;
  static constexpr WUInt32 s_uiNpotTextureHeight = 6;
  /// Number of mip levels for the texture sub-tests so we exercise the multi-mip branch of CopyTexture.
  static constexpr WUInt32 s_uiTextureMips = 3;
  /// Buffer size in bytes for the buffer sub-tests.
  static constexpr WUInt32 s_uiBufferSize = 256;

  WGALBufferHandle m_hBufferSource;
  WGALBufferHandle m_hBufferDest;
  WGALTextureHandle m_hTextureSource;
  WGALTextureHandle m_hTextureDest;

  WDynamicArray<WUInt8> m_BufferSourceData;
  /// Per-slice CPU copy of the source texture.
  WDynamicArray<WImage> m_TextureSourceImages;
  /// Per-slice CPU copy of the destination texture. Each texture test updates this alongside the GPU texture.
  WDynamicArray<WImage> m_TextureDestImages;

  WGALReadbackBufferHelper m_BufferReadback;
  WGALReadbackTextureHelper m_TextureReadback;
};
