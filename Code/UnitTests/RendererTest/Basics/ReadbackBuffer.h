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
class WRendererTestReadbackBuffer : public WGraphicsTest
{
  using SUPER = WGraphicsTest;

public:
  virtual const char* GetTestName() const override { return "Readback Buffer"; }

private:
  enum SubTests
  {
    ST_VertexBuffer,
    ST_IndexBuffer,
    ST_TexelBuffer,
    ST_StructuredBuffer,
    ST_ByteAddressBuffer,
  };

  virtual void SetupSubTests() override;

  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;
  WTestAppRun ReadbackBuffer(WUInt32 uiInvocationCount);

private:
  WShaderResourceHandle m_hComputeShader;

  WDynamicArray<WUInt8> m_BufferData;
  WGALBufferHandle m_hBufferReadback;
  bool m_bReadbackInProgress = true;
  WGALReadbackBufferHelper m_Readback;
};
