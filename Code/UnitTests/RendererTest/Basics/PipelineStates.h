#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

class WRendererTestPipelineStates : public WGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "PipelineStates"; }

private:
  enum SubTests
  {
    ST_MostBasicShader,
    ST_ViewportScissor,
    ST_VertexBuffer,
    ST_IndexBuffer,
    ST_ConstantBuffer,
    ST_StructuredBuffer,
    ST_TexelBuffer,
    ST_ByteAddressBuffer,
    ST_Texture2D,
    ST_Texture2DArray,
    ST_GenerateMipMaps,
    ST_PushConstants,
    ST_BindGroups,
    ST_Timestamps,
    ST_OcclusionQueries,
    ST_CustomVertexStreams,
  };

  enum ImageCaptureFrames
  {
    DefaultCapture = 5,
    StructuredBuffer_InitialData = 5,
    StructuredBuffer_UpdateForNextFrame = 6,
    StructuredBuffer_UpdateForNextFrame2 = 7,
    StructuredBuffer_Transient1 = 8,
    StructuredBuffer_Transient2 = 9,
    StructuredBuffer_UAV = 10,
    CustomVertexStreams_Offsets = 6,
    Timestamps_MaxWaitTime = WMath::MaxValue<WUInt32>(),
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;
  virtual void MapImageNumberToString(const char* szTestName, const WSubTestEntry& subTest, WUInt32 uiImageNumber, WStringBuilder& out_sString) const override;

  void RenderBlock(WMeshBufferResourceHandle mesh, WColor clearColor = WColor::CornflowerBlue, WUInt32 uiRenderTargetClearMask = 0xFFFFFFFF, WRectFloat* pViewport = nullptr, WRectU32* pScissor = nullptr);

  void MostBasicTriangleTest();
  void ViewportScissorTest();
  void VertexBufferTest();
  void IndexBufferTest();
  void ConstantBufferTest();
  void StructuredBufferTestUpload();
  void StructuredBufferTest(WGALShaderResourceType::Enum bufferType);
  void Texture2D();
  void Texture2DArray();
  void GenerateMipMaps();
  void PushConstantsTest();
  void BindGroupsTest();
  void CustomVertexStreams();
  WTestAppRun Timestamps();
  WTestAppRun OcclusionQueries();

private:
  WShaderResourceHandle m_hMostBasicTriangleShader;
  WShaderResourceHandle m_hNDCPositionOnlyShader;
  WShaderResourceHandle m_hConstantBufferShader;
  WShaderResourceHandle m_hPushConstantsShader;
  WShaderResourceHandle m_hInstancingShader;
  WShaderResourceHandle m_hCopyBufferShader;
  WShaderResourceHandle m_hCustomVertexStreamShader;

  WMeshBufferResourceHandle m_hTriangleMesh;
  WMeshBufferResourceHandle m_hSphereMesh;

  WConstantBufferStorageHandle m_hTestPerFrameConstantBuffer;
  WConstantBufferStorageHandle m_hTestColorsConstantBuffer;
  WConstantBufferStorageHandle m_hTestPositionsConstantBuffer;

  WGALBufferHandle m_hInstancingData;
  WGALBufferHandle m_hInstancingDataTransient;
  WGALBufferHandle m_hInstancingDataUAV;

  WGALBufferHandle m_hInstancingDataVertexStream;
  WSmallArray<WGALVertexAttribute, 8> m_VertexAttributes;

  WGALTextureHandle m_hTexture2D;
  WGALTextureHandle m_hTexture2DArray;


  // Timestamps / Occlusion Queries test
  WInt32 m_iDelay = 0;
  WTime m_CPUTime[2];
  WTime m_GPUTime[2];
  WGALTimestampHandle m_timestamps[2];
  WGALOcclusionHandle m_queries[4];
  WGALFenceHandle m_hFence = {};
};
