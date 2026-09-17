#pragma once

#include <Core/Graphics/Geometry.h>
#include <Core/System/Window.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>
#include <RendererFoundation/Utils/ResourceStateTracker.h>
#include <TestFramework/Framework/TestBaseClass.h>

#undef CreateWindow

class WImage;
class WRenderGraph;

struct ObjectCB
{
  WMat4 m_MVP;
  WColor m_Color;
};

class WGraphicsTest : public WTestBaseClass
{
public:
  static WResult CreateRenderer(WGALDevice*& out_pDevice);
  static void SetClipSpace();

public:
  WGraphicsTest();

  void ReadbackImage(WRenderGraph& ref_graph);
  virtual WResult GetImage(WImage& ref_img, const WSubTestEntry& subTest, WUInt32 uiImageNumber) override;

protected:
  virtual void SetupSubTests() override {}
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override { return WTestAppRun::Quit; }

  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;

  WSizeU32 GetResolution() const;

protected:
  const WGALDeviceCapabilities& GetDeviceCapabilities();
  WResult SetupRenderer();
  void ShutdownRenderer();

  WResult CreateWindow(WUInt32 uiResolutionX = 960, WUInt32 uiResolutionY = 540);
  void DestroyWindow();

  void BeginFrame();
  void EndFrame();

  void BeginCommands(const char* szPassName);
  void EndCommands();

  WGALCommandEncoder* BeginRendering(WColor clearColor, WUInt32 uiRenderTargetClearMask = 0xFFFFFFFF, WRectFloat* pViewport = nullptr, WRectU32* pScissor = nullptr);
  void EndRendering();
  WGALResourceStateTracker* GetResourceStateTracker();
  void TransitionTexture(WGALTextureHandle hTexture, WBitflags<WGALResourceState> newState, WGALTextureRange range = {}, WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);
  void TransitionBuffer(WGALBufferHandle hBuffer, WBitflags<WGALResourceState> newState, WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  /// Renders a unit cube and makes an image comparison if m_bCaptureImage is set and the current frame is in m_ImgCompFrames.
  /// \param viewport Viewport to render into.
  /// \param mMVP Model View Projection matrix for camera. Use CreateSimpleMVP for convenience.
  /// \param uiRenderTargetClearMask What render targets if any should be cleared.
  /// \param hTexture The texture to render onto the cube.
  /// \param textureRange The texture range to use when rendering the cube.
  void RenderCube(WRectFloat viewport, WMat4 mMVP, WUInt32 uiRenderTargetClearMask, WGALTextureHandle hTexture, const WGALTextureRange& textureRange = {});

  WMat4 CreateSimpleMVP(float fAspectRatio);


  WMeshBufferResourceHandle CreateMesh(const WGeometry& geom, const char* szResourceName);
  WMeshBufferResourceHandle CreateSphere(WInt32 iSubDivs, float fRadius);
  WMeshBufferResourceHandle CreateTorus(WInt32 iSubDivs, float fInnerRadius, float fOuterRadius);
  WMeshBufferResourceHandle CreateBox(float fWidth, float fHeight, float fDepth);
  WMeshBufferResourceHandle CreateLineBox(float fWidth, float fHeight, float fDepth);
  void RenderObject(WMeshBufferResourceHandle hObject, const WMat4& mTransform, const WColor& color, WBitflags<WShaderBindFlags> ShaderBindFlags = WShaderBindFlags::Default);
  WGALTextureHandle GetBackbuffer() const;
  void TextureBarrier(const WGALTextureBarrier& barrier);
  void BufferBarrier(const WGALBufferBarrier& barrier);

  WWindow* m_pWindow = nullptr;
  WGALDevice* m_pDevice = nullptr;
  WGALCommandEncoder* m_pEncoder = nullptr;

  WGALSwapChainHandle m_hSwapChain;
  WGALTextureHandle m_hDepthStencilTexture;

  WConstantBufferStorageHandle m_hObjectTransformCB;
  WShaderResourceHandle m_hShader;
  WMeshBufferResourceHandle m_hCubeUV;

  WInt32 m_iFrame = 0;
  bool m_bCaptureImage = false;
  WHybridArray<WUInt32, 8> m_ImgCompFrames;
  WGALReadbackTextureHelper m_Readback;
  bool m_bReadBackInProgress = false;

  WUniquePtr<WGALResourceStateTracker> m_pResourceStateTracker;
};
