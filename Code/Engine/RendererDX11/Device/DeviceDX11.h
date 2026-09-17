#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/Device/Device.h>

// TODO: This should not be included in a header, it exposes Windows.h to the outside
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <dxgi.h>

struct ID3D11Device;
struct ID3D11Device3;
struct ID3D11DeviceContext;
struct ID3D11Debug;
struct IDXGIFactory1;
struct IDXGIAdapter1;
struct IDXGIDevice1;
struct ID3D11Resource;
struct ID3D11Query;
struct IDXGIAdapter;

using WGALFormatLookupEntryDX11 = WGALFormatLookupEntry<DXGI_FORMAT, (DXGI_FORMAT)0>;
using WGALFormatLookupTableDX11 = WGALFormatLookupTable<WGALFormatLookupEntryDX11>;

class WFenceQueueDX11;
class WQueryPoolDX11;

/// The DX11 device implementation of the graphics abstraction layer.
class W_RENDERERDX11_DLL WGALDeviceDX11 : public WGALDevice
{
private:
  friend WInternal::NewInstance<WGALDevice> CreateDX11Device(WAllocator* pAllocator, const WGALDeviceCreationDescription& description);
  WGALDeviceDX11(const WGALDeviceCreationDescription& Description);

public:
  virtual ~WGALDeviceDX11();

public:
  ID3D11Device* GetDXDevice() const;
  ID3D11Device3* GetDXDevice3() const;
  ID3D11DeviceContext* GetDXImmediateContext() const;
  IDXGIFactory1* GetDXGIFactory() const;
  WGALCommandEncoder* GetCommandEncoder() const;

  WFenceQueueDX11& GetFenceQueue() const;
  WQueryPoolDX11& GetQueryPool() const;

  const WGALFormatLookupTableDX11& GetFormatLookupTable() const;

  void ReportLiveGpuObjects();

  void FlushDeadObjects();

  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  /// Internal version of device init that allows to modify device creation flags and graphics adapter.
  ///
  /// \param pUsedAdapter
  ///   Null means default adapter.
  WResult InitPlatform(DWORD flags, IDXGIAdapter* pUsedAdapter);

  virtual WStringView GetRendererPlatform() override;
  virtual WResult InitPlatform() override;
  virtual WResult ShutdownPlatform() override;

  // Command encoder functions

  virtual WGALCommandEncoder* BeginCommandsPlatform(const char* szName) override;
  virtual void EndCommandsPlatform(WGALCommandEncoder* pPass) override;

  virtual void FlushPlatform() override;


  // State creation functions

  virtual WGALBlendState* CreateBlendStatePlatform(const WGALBlendStateCreationDescription& Description) override;
  virtual void DestroyBlendStatePlatform(WGALBlendState* pBlendState) override;

  virtual WGALDepthStencilState* CreateDepthStencilStatePlatform(const WGALDepthStencilStateCreationDescription& Description) override;
  virtual void DestroyDepthStencilStatePlatform(WGALDepthStencilState* pDepthStencilState) override;

  virtual WGALRasterizerState* CreateRasterizerStatePlatform(const WGALRasterizerStateCreationDescription& Description) override;
  virtual void DestroyRasterizerStatePlatform(WGALRasterizerState* pRasterizerState) override;

  virtual WGALSamplerState* CreateSamplerStatePlatform(const WGALSamplerStateCreationDescription& Description) override;
  virtual void DestroySamplerStatePlatform(WGALSamplerState* pSamplerState) override;
  virtual void RecreateSamplerStatePlatform(WGALSamplerState* pSamplerState) override;

  virtual WGALBindGroupLayout* CreateBindGroupLayoutPlatform(const WGALBindGroupLayoutCreationDescription& Description) override;
  virtual void DestroyBindGroupLayoutPlatform(WGALBindGroupLayout* pBindGroupLayout) override;

  virtual WGALBindGroup* CreateBindGroupPlatform(const WGALBindGroupCreationDescription& Description) override;
  virtual void DestroyBindGroupPlatform(WGALBindGroup* pBindGroup) override;
  virtual void RecreateBindGroupPlatform(WGALBindGroup* pBindGroup) override;

  virtual WGALPipelineLayout* CreatePipelineLayoutPlatform(const WGALPipelineLayoutCreationDescription& Description) override;
  virtual void DestroyPipelineLayoutPlatform(WGALPipelineLayout* pPipelineLayout) override;

  virtual WGALGraphicsPipeline* CreateGraphicsPipelinePlatform(const WGALGraphicsPipelineCreationDescription& Description) override;
  virtual void DestroyGraphicsPipelinePlatform(WGALGraphicsPipeline* pGraphicsPipeline) override;

  virtual WGALComputePipeline* CreateComputePipelinePlatform(const WGALComputePipelineCreationDescription& Description) override;
  virtual void DestroyComputePipelinePlatform(WGALComputePipeline* pComputePipeline) override;


  // Resource creation functions

  virtual WGALShader* CreateShaderPlatform(const WGALShaderCreationDescription& Description) override;
  virtual void DestroyShaderPlatform(WGALShader* pShader) override;

  virtual WGALBuffer* CreateBufferPlatform(const WGALBufferCreationDescription& Description, WArrayPtr<const WUInt8> pInitialData) override;
  virtual void DestroyBufferPlatform(WGALBuffer* pBuffer) override;

  virtual WGALTexture* CreateTexturePlatform(const WGALTextureCreationDescription& Description, WArrayPtr<WGALSystemMemoryDescription> pInitialData) override;
  virtual void DestroyTexturePlatform(WGALTexture* pTexture) override;

  virtual WGALTexture* CreateSharedTexturePlatform(const WGALTextureCreationDescription& Description, WArrayPtr<WGALSystemMemoryDescription> pInitialData, WEnum<WGALSharedTextureType> sharedType, WGALPlatformSharedHandle handle) override;
  virtual void DestroySharedTexturePlatform(WGALTexture* pTexture) override;

  virtual WGALReadbackBuffer* CreateReadbackBufferPlatform(const WGALBufferCreationDescription& Description) override;
  virtual void DestroyReadbackBufferPlatform(WGALReadbackBuffer* pReadbackBuffer) override;

  virtual WGALReadbackTexture* CreateReadbackTexturePlatform(const WGALTextureCreationDescription& Description) override;
  virtual void DestroyReadbackTexturePlatform(WGALReadbackTexture* pReadbackTexture) override;

  virtual WGALRenderTargetView* CreateRenderTargetViewPlatform(WGALTexture* pTexture, const WGALRenderTargetViewCreationDescription& Description) override;
  virtual void DestroyRenderTargetViewPlatform(WGALRenderTargetView* pRenderTargetView) override;

  // Other rendering creation functions

  virtual WGALVertexDeclaration* CreateVertexDeclarationPlatform(const WGALVertexDeclarationCreationDescription& Description) override;
  virtual void DestroyVertexDeclarationPlatform(WGALVertexDeclaration* pVertexDeclaration) override;

  // Resource update functions

  virtual void UpdateBufferForNextFramePlatform(const WGALBuffer* pBuffer, WConstByteArrayPtr sourceData, WUInt32 uiDestOffset) override;
  virtual void UpdateTextureForNextFramePlatform(const WGALTexture* pTexture, const WGALSystemMemoryDescription& sourceData, const WGALTextureSubresource& destinationSubResource, const WBoundingBoxu32& destinationBox) override;

  // GPU -> CPU query functions

  virtual WEnum<WGALAsyncResult> GetTimestampResultPlatform(WGALTimestampHandle hTimestamp, WTime& out_result) override;
  virtual WEnum<WGALAsyncResult> GetOcclusionResultPlatform(WGALOcclusionHandle hOcclusion, WUInt64& out_uiResult) override;
  virtual WEnum<WGALAsyncResult> GetFenceResultPlatform(WGALFenceHandle hFence, WTime timeout) override;
  virtual WResult LockBufferPlatform(const WGALReadbackBuffer* pBuffer, WArrayPtr<const WUInt8>& out_Memory) const override;
  virtual void UnlockBufferPlatform(const WGALReadbackBuffer* pBuffer) const override;
  virtual WResult LockTexturePlatform(const WGALReadbackTexture* pTexture, const WArrayPtr<const WGALTextureSubresource>& subResources, WDynamicArray<WGALSystemMemoryDescription>& out_Memory) const override;
  virtual void UnlockTexturePlatform(const WGALReadbackTexture* pTexture, const WArrayPtr<const WGALTextureSubresource>& subResources) const override;
  // Swap chain functions

  void PresentPlatform(const WGALSwapChain* pSwapChain, bool bVSync);

  // Misc functions

  virtual void BeginFramePlatform(WArrayPtr<WGALSwapChain*> swapchains, const WUInt64 uiAppFrame) override;
  virtual void EndFramePlatform(WArrayPtr<WGALSwapChain*> swapchains) override;
  virtual WUInt64 GetCurrentFramePlatform() const override;
  virtual WUInt64 GetSafeFramePlatform() const override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  virtual const WGALSharedTexture* GetSharedTexture(WGALTextureHandle hTexture) const override;

  /// \endcond

private:
  friend class WGALCommandEncoderImplDX11;

  struct TempResourceType
  {
    enum Enum
    {
      Buffer,
      Texture,

      ENUM_COUNT
    };
  };

  struct TempResource
  {
    ID3D11Resource* m_pResource = nullptr;
    void* m_pData = nullptr;
    WUInt32 m_uiRowPitch = 0;
    WUInt32 m_uiDepthPitch = 0;

    operator bool() const
    {
      return m_pResource != nullptr;
    }
  };

  TempResource CopyToTempBuffer(WConstByteArrayPtr sourceData, WUInt64 uiLastUseFrame = WUInt64(-1));
  TempResource CopyToTempTexture(const WGALSystemMemoryDescription& sourceData, WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiDepth, WGALResourceFormat::Enum format, WUInt64 uiLastUseFrame = WUInt64(-1));
  void MapTempResource(TempResource& tempResource);
  void UnmapTempResource(TempResource& tempResource);
  void FreeTempResources(WUInt64 uiFrame);

  void ProcessPendingCopies();

  void FillFormatLookupTable();

  static constexpr WUInt32 FRAMES = 4;

  ID3D11Device* m_pDevice = nullptr;
  ID3D11Device3* m_pDevice3 = nullptr;
  ID3D11DeviceContext* m_pImmediateContext;
  ID3D11Debug* m_pDebug = nullptr;
  IDXGIFactory1* m_pDXGIFactory = nullptr;
  IDXGIAdapter1* m_pDXGIAdapter = nullptr;
  IDXGIDevice1* m_pDXGIDevice = nullptr;

  WUniquePtr<WFenceQueueDX11> m_pFenceQueue;
  WUniquePtr<WQueryPoolDX11> m_pQueryPool;
  WGALFormatLookupTableDX11 m_FormatLookupTable;

  // NOLINTNEXTLINE
  WUInt32 m_uiFeatureLevel; // D3D_FEATURE_LEVEL can't be forward declared

  WUniquePtr<WGALCommandEncoderImplDX11> m_pCommandEncoderImpl;
  WUniquePtr<WGALCommandEncoder> m_pCommandEncoder;

  struct PerFrameData
  {
    WGALFenceHandle m_hFence = {};
    WUInt64 m_uiFrame = WUInt64(-1);
  };

  PerFrameData m_PerFrameData[FRAMES];

  WUInt64 m_uiFrameCounter = 1;
  WUInt64 m_uiSafeFrame = 0;
  WUInt8 m_uiCurrentPerFrameData = m_uiFrameCounter % FRAMES;

  bool m_bSupportsAlwaysMappedTempResources = true;

  struct UsedTempResource
  {
    W_DECLARE_POD_TYPE();

    ID3D11Resource* m_pResource;
    WUInt64 m_uiFrame;
    WUInt32 m_uiHash;
  };

  WMap<WUInt32, WDynamicArray<TempResource>, WCompareHelper<WUInt32>, WLocalAllocatorWrapper> m_FreeTempResources[TempResourceType::ENUM_COUNT];
  WDynamicArray<UsedTempResource, WLocalAllocatorWrapper> m_UsedTempResources[TempResourceType::ENUM_COUNT];

  struct PendingCopy
  {
    TempResource m_SourceResource = {};
    WGALSystemMemoryDescription m_SourceData; // Used in case always mapped temp resources are not supported

    ID3D11Resource* m_pDestResource = nullptr;
    WUInt32 m_uiDestSubResource = 0;
    WVec3U32 m_vDestPoint = WVec3U32::MakeZero();
    WVec3U32 m_vSourceSize = WVec3U32::MakeZero();
    bool m_bCopySubresource = false;
    WGALResourceFormat::Enum m_SourceFormat = WGALResourceFormat::Invalid;
  };

  WDynamicArray<PendingCopy, WLocalAllocatorWrapper> m_PendingCopies;

  struct GPUTimingScope* m_pFrameTimingScope = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope = nullptr;
};

#include <RendererDX11/Device/Implementation/DeviceDX11_inl.h>
