#pragma once

#include "../TestClass/TestClass.h"

class WRendererTestIndirectDraw : public WGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "IndirectDraw"; }

private:
  enum SubTests
  {
    ST_DrawInstancedIndirect,
    ST_DrawIndexedInstancedIndirect,
    ST_DrawIndexedInstancedIndirectOffset,
    ST_DispatchIndirect,
  };

  enum ImageCaptureFrames
  {
    DefaultCapture = 5,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  void DrawInstancedIndirect();
  void DrawIndexedInstancedIndirect();
  void DrawIndexedInstancedIndirectOffset();
  void DispatchIndirect();

  void FillIndirectArgsViaCompute(WUInt32 arg0, WUInt32 arg1, WUInt32 arg2, WUInt32 arg3, WUInt32 arg4 = 0);
  void CaptureImage();

  // Resources
  static constexpr WUInt32 s_uiRTSize = 32;

  WGALBufferHandle m_hIndirectArgsBuffer;
  WMeshBufferResourceHandle m_hTriangleMesh;
  WMeshBufferResourceHandle m_hIndexedTriangleMesh;

  // For dispatch test
  WGALTextureHandle m_hDispatchOutputTexture;

  // Shaders
  WShaderResourceHandle m_hFillArgsShader;
  WShaderResourceHandle m_hDrawShader;
  WShaderResourceHandle m_hInstancedDrawShader;
  WShaderResourceHandle m_hIndexedInstancedDrawShader;
  WShaderResourceHandle m_hDispatchWriteShader;
};
