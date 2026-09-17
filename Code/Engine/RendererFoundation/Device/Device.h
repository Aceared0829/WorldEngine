#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/DeviceCapabilities.h>
#include <RendererFoundation/Device/ReadbackLock.h>
#include <RendererFoundation/Utils/DependencyTracker.h>

class WColor;

/// The WRenderDevice class is the primary interface for interactions with rendering APIs
/// It contains a set of (non-virtual) functions to set state, create resources etc. which rely on
/// API specific implementations provided by protected virtual functions.
/// Redundant state changes are prevented at the platform independent level in the non-virtual functions.
class W_RENDERERFOUNDATION_DLL WGALDevice
{
public:
  static WEvent<const WGALDeviceEvent&, WMutex> s_Events;
  static WEvent<const WGALSwapChain*, WMutex> s_SwapChainUpdatedEvent;

  // Init & shutdown functions

  WResult Init();
  WResult Shutdown();
  WStringView GetRenderer();

  // Commands functions

  /// Begin recording GPU commands on the returned command encoder.
  WGALCommandEncoder* BeginCommands(const char* szName);
  /// Stop recording commands on the command encoder.
  /// \param pCommandEncoder Must match the command encoder returned by BeginCommands.
  void EndCommands(WGALCommandEncoder* pCommandEncoder);

  // State creation functions

  WGALBlendStateHandle CreateBlendState(const WGALBlendStateCreationDescription& description);
  void DestroyBlendState(WGALBlendStateHandle& inout_hBlendState);

  WGALDepthStencilStateHandle CreateDepthStencilState(const WGALDepthStencilStateCreationDescription& description);
  void DestroyDepthStencilState(WGALDepthStencilStateHandle& inout_hDepthStencilState);

  WGALRasterizerStateHandle CreateRasterizerState(const WGALRasterizerStateCreationDescription& description);
  void DestroyRasterizerState(WGALRasterizerStateHandle& inout_hRasterizerState);

  WGALSamplerStateHandle CreateSamplerState(const WGALSamplerStateCreationDescription& description);
  void DestroySamplerState(WGALSamplerStateHandle& inout_hSamplerState);

  WGALBindGroupLayoutHandle CreateBindGroupLayout(const WGALBindGroupLayoutCreationDescription& description);
  void DestroyBindGroupLayout(WGALBindGroupLayoutHandle& inout_hBindGroupLayout);

  // Bind group functions
  WGALBindGroupHandle CreateBindGroup(const WGALBindGroupCreationDescription& description);
  void DestroyBindGroup(WGALBindGroupHandle& inout_hBindGroup);

  WGALPipelineLayoutHandle CreatePipelineLayout(const WGALPipelineLayoutCreationDescription& description);
  void DestroyPipelineLayout(WGALPipelineLayoutHandle& inout_hPipelineLayout);

  WGALGraphicsPipelineHandle CreateGraphicsPipeline(const WGALGraphicsPipelineCreationDescription& description);
  void DestroyGraphicsPipeline(WGALGraphicsPipelineHandle& inout_hGraphicsPipeline);

  WGALComputePipelineHandle CreateComputePipeline(const WGALComputePipelineCreationDescription& description);
  void DestroyComputePipeline(WGALComputePipelineHandle& inout_hComputePipeline);

  // Resource creation functions

  WGALShaderHandle CreateShader(const WGALShaderCreationDescription& description);
  void DestroyShader(WGALShaderHandle& inout_hShader);

  WGALBufferHandle CreateBuffer(const WGALBufferCreationDescription& description, WArrayPtr<const WUInt8> initialData = WArrayPtr<const WUInt8>());
  void DestroyBuffer(WGALBufferHandle& inout_hBuffer);

  WGALDynamicBufferHandle CreateDynamicBuffer(const WGALBufferCreationDescription& description, WStringView sDebugName);
  void DestroyDynamicBuffer(WGALDynamicBufferHandle& inout_hBuffer);

  // Helper functions for buffers (for common, simple use cases)

  WGALBufferHandle CreateVertexBuffer(WUInt32 uiVertexSize, WUInt32 uiVertexCount, WArrayPtr<const WUInt8> initialData = WArrayPtr<const WUInt8>(), bool bDataIsMutable = false);
  WGALBufferHandle CreateIndexBuffer(WGALIndexType::Enum indexType, WUInt32 uiIndexCount, WArrayPtr<const WUInt8> initialData = WArrayPtr<const WUInt8>(), bool bDataIsMutable = false);
  WGALBufferHandle CreateConstantBuffer(WUInt32 uiBufferSize);

  WGALTextureHandle CreateTexture(const WGALTextureCreationDescription& description, WArrayPtr<WGALSystemMemoryDescription> initialData = WArrayPtr<WGALSystemMemoryDescription>());
  void DestroyTexture(WGALTextureHandle& inout_hTexture);

  WGALTextureHandle CreateProxyTexture(WGALTextureHandle hParentTexture, WUInt32 uiSlice);
  void DestroyProxyTexture(WGALTextureHandle& inout_hProxyTexture);

  WGALTextureHandle CreateSharedTexture(const WGALTextureCreationDescription& description, WArrayPtr<WGALSystemMemoryDescription> initialData = {});
  WGALTextureHandle OpenSharedTexture(const WGALTextureCreationDescription& description, WGALPlatformSharedHandle hSharedHandle);
  void DestroySharedTexture(WGALTextureHandle& inout_hTexture);

  WGALReadbackBufferHandle CreateReadbackBuffer(const WGALBufferCreationDescription& description);
  void DestroyReadbackBuffer(WGALReadbackBufferHandle& inout_hBuffer);

  WGALReadbackTextureHandle CreateReadbackTexture(const WGALTextureCreationDescription& description);
  void DestroyReadbackTexture(WGALReadbackTextureHandle& inout_hTexture);

  // Resource update functions

  /// Ensures that the given buffer is updated at the beginning of the next frame.
  void UpdateBufferForNextFrame(WGALBufferHandle hBuffer, WConstByteArrayPtr sourceData, WUInt32 uiDestOffset = 0);

  /// Ensures that the given texture is updated at the beginning of the next frame.
  void UpdateTextureForNextFrame(WGALTextureHandle hTexture, const WGALSystemMemoryDescription& sourceData, const WGALTextureSubresource& destinationSubResource = {}, const WBoundingBoxu32& destinationBox = WBoundingBoxu32::MakeZero());

  // Render target views
  WGALRenderTargetViewHandle GetDefaultRenderTargetView(WGALTextureHandle hTexture);

  WGALRenderTargetViewHandle GetRenderTargetView(const WGALRenderTargetViewCreationDescription& description);

  // Other rendering creation functions

  using SwapChainFactoryFunction = WDelegate<WGALSwapChain*(WAllocator*)>;
  WGALSwapChainHandle CreateSwapChain(const SwapChainFactoryFunction& func);
  WResult UpdateSwapChain(WGALSwapChainHandle hSwapChain, WEnum<WGALPresentMode> newPresentMode);
  void DestroySwapChain(WGALSwapChainHandle& inout_hSwapChain);

  WGALVertexDeclarationHandle CreateVertexDeclaration(const WGALVertexDeclarationCreationDescription& description);
  void DestroyVertexDeclaration(WGALVertexDeclarationHandle& inout_hVertexDeclaration);

  // GPU -> CPU query functions

  /// Queries the result of a timestamp.
  /// Should be called every frame until WGALAsyncResult::Ready is returned.
  /// \param hTimestamp The timestamp handle to query.
  /// \param out_result If WGALAsyncResult::Ready is returned, this will be the timestamp at which this handle was inserted into the command encoder.
  /// \return If WGALAsyncResult::Expired is returned, the result was in a ready state for more than 4 frames and was thus deleted.
  /// \sa WCommandEncoder::InsertTimestamp
  WEnum<WGALAsyncResult> GetTimestampResult(WGALTimestampHandle hTimestamp, WTime& out_result);

  /// Queries the result of an occlusion query.
  /// Should be called every frame until WGALAsyncResult::Ready is returned.
  /// \param hOcclusion The occlusion query handle to query.
  /// \param out_uiResult If WGALAsyncResult::Ready is returned, this will be the number of pixels of the occlusion query.
  /// \return If WGALAsyncResult::Expired is returned, the result was in a ready state for more than 4 frames and was thus deleted.
  /// \sa WCommandEncoder::BeginOcclusionQuery, WCommandEncoder::EndOcclusionQuery
  WEnum<WGALAsyncResult> GetOcclusionQueryResult(WGALOcclusionHandle hOcclusion, WUInt64& out_uiResult);

  /// Queries the result of a fence.
  /// Fences can never expire as they are just monotonically increasing numbers over time.
  /// \param hFence The fence handle to query.
  /// \param timeout If set to > 0, the function will block until the fence is ready or the timeout is reached.
  /// \return Returns either Ready or Pending.
  /// \sa WCommandEncoder::InsertFence
  WEnum<WGALAsyncResult> GetFenceResult(WGALFenceHandle hFence, WTime timeout = WTime::MakeZero());

  /// Tries to lock a readback buffer for reading. Only fails if the handle is invalid.
  /// \param hReadbackBuffer The buffer to lock.
  /// \param out_memory If successful, contains the memory of the buffer. Only allowed to be accessed within the lifetime of the returns lock object.
  /// \return Returns the lock. WReadbackBufferLock::IsValid needs to be called to ensure the locking was successful.
  WReadbackBufferLock LockBuffer(WGALReadbackBufferHandle hReadbackBuffer, WArrayPtr<const WUInt8>& out_memory);

  /// Tries to lock a readback texture for reading. Only fails if the handle is invalid.
  /// \param hReadbackTexture The texture to lock.
  /// \param subResources The sub-resources that should to be locked.
  /// \param out_memory If successful, contains the memory locations of each sub-resource. Only allowed to be accessed within the lifetime of the returns lock object.
  /// \return Returns the lock. WReadbackTextureLock::IsValid needs to be called to ensure the locking was successful.
  WReadbackTextureLock LockTexture(WGALReadbackTextureHandle hReadbackTexture, const WArrayPtr<const WGALTextureSubresource>& subResources, WDynamicArray<WGALSystemMemoryDescription>& out_memory);


  // Swap chain functions

  WGALTextureHandle GetBackBufferTextureFromSwapChain(WGALSwapChainHandle hSwapChain) const;

  void GetAllSwapChains(WDynamicArray<WGALSwapChainHandle>& out_swapChains) const;

  // Misc functions

  /// Adds a swap-chain to be used for the next frame.
  /// Must be called before or during the WGALDeviceEvent::BeforeBeginFrame event (BeginFrame function) and repeated for every frame the swap-chain is to be used. This approach guarantees that all swap-chains of a frame acquire and present at the same time, which improves frame pacing.
  /// \param hSwapChain Swap-chain used in this frame. The device will ensure to acquire an image from the swap-chain during BeginFrame and present it when calling EndFrame.
  void EnqueueFrameSwapChain(WGALSwapChainHandle hSwapChain);

  /// Begins rendering of a frame. This needs to be called first before any rendering function can be called.
  /// \param uiAppFrame Frame index for debugging purposes, has no effect on GetCurrentFrame.
  void BeginFrame(const WUInt64 uiAppFrame = 0);

  /// Ends rendering of a frame and submits all data to the GPU. No further rendering calls are allowed until BeginFrame is called again.
  void EndFrame();

  /// The current rendering frame.
  /// This is a monotonically increasing number which changes +1 every time EndFrame is called. You can use this to synchronize read/writes between CPU and GPU, see GetSafeFrame.
  /// \sa GetSafeFrame
  WUInt64 GetCurrentFrame() const;
  /// The latest frame that has been fully executed on the GPU.
  /// Whenever you execute any work that requires synchronization between CPU and GPU, remember the GetCurrentFrame result in which the operation was done. When GetSafeFrame reaches this number, you know for sure that the GPU has completed all operations of that frame.
  /// \sa GetCurrentFrame
  WUInt64 GetSafeFrame() const;


  const WGALDeviceCreationDescription* GetDescription() const;

  const WGALSwapChain* GetSwapChain(WGALSwapChainHandle hSwapChain) const;
  template <typename T>
  const T* GetSwapChain(WGALSwapChainHandle hSwapChain) const
  {
    return static_cast<const T*>(GetSwapChainInternal(hSwapChain, WGetStaticRTTI<T>()));
  }

  const WGALShader* GetShader(WGALShaderHandle hShader) const;
  const WGALTexture* GetTexture(WGALTextureHandle hTexture) const;
  virtual const WGALSharedTexture* GetSharedTexture(WGALTextureHandle hTexture) const = 0;
  const WGALBuffer* GetBuffer(WGALBufferHandle hBuffer) const;
  const WGALDynamicBuffer* GetDynamicBuffer(WGALDynamicBufferHandle hBuffer) const;
  WGALDynamicBuffer* GetDynamicBuffer(WGALDynamicBufferHandle hBuffer);
  const WGALReadbackBuffer* GetReadbackBuffer(WGALReadbackBufferHandle hBuffer) const;
  const WGALReadbackTexture* GetReadbackTexture(WGALReadbackTextureHandle hTexture) const;
  const WGALDepthStencilState* GetDepthStencilState(WGALDepthStencilStateHandle hDepthStencilState) const;
  const WGALBlendState* GetBlendState(WGALBlendStateHandle hBlendState) const;
  const WGALRasterizerState* GetRasterizerState(WGALRasterizerStateHandle hRasterizerState) const;
  const WGALVertexDeclaration* GetVertexDeclaration(WGALVertexDeclarationHandle hVertexDeclaration) const;
  const WGALSamplerState* GetSamplerState(WGALSamplerStateHandle hSamplerState) const;
  const WGALBindGroupLayout* GetBindGroupLayout(WGALBindGroupLayoutHandle hBindGroupLayout) const;
  const WGALBindGroup* GetBindGroup(WGALBindGroupHandle hBindGroup) const;
  const WGALPipelineLayout* GetPipelineLayout(WGALPipelineLayoutHandle hPipelineLayout) const;
  const WGALGraphicsPipeline* GetGraphicsPipeline(WGALGraphicsPipelineHandle hGraphicsPipeline) const;
  const WGALComputePipeline* GetComputePipeline(WGALComputePipelineHandle hComputePipeline) const;
  const WGALRenderTargetView* GetRenderTargetView(WGALRenderTargetViewHandle hRenderTargetView) const;

  const WGALDeviceCapabilities& GetCapabilities() const;

  virtual WUInt64 GetMemoryConsumptionForTexture(const WGALTextureCreationDescription& description) const;
  virtual WUInt64 GetMemoryConsumptionForBuffer(const WGALBufferCreationDescription& description) const;

  static void SetDefaultDevice(WGALDevice* pDefaultDevice);
  static WGALDevice* GetDefaultDevice();
  static bool HasDefaultDevice();

  // Sends the queued up commands to the GPU.
  // Same as WCommandEncoder:Flush.
  void Flush();

  /// Waits for the GPU to be idle and destroys any pending resources and GPU objects.
  void WaitIdle();

  // public in case someone external needs to lock multiple operations
  mutable WMutex m_Mutex;

  /// Internal: Returns the allocator used by the device.
  WAllocator* GetAllocator();

  /// Sets the texture quality assigned to the given quality slot.
  ///
  /// Quality mode slots are referenced by samplers that opt in via m_useTextureQualitySlot.
  /// Call UpdateTextureQuality() after changing slots to apply the change to existing samplers.
  void SetTextureQualityMode(WGALTextureQualitySlot::Enum slot, WGALTextureQuality::Enum quality);

  /// Overrides the filter settings in \a inout_desc using the quality assigned to its quality mode slot.
  ///
  /// No-op if inout_desc.m_useTextureQualitySlot is WGALTextureQualitySlot::None.
  void AdjustSamplerStateDescription(WGALSamplerStateCreationDescription& inout_desc);

  /// Recreates all quality-adjustable sampler states to reflect the current quality mode settings.
  ///
  /// Called automatically by WRenderContext when the texture quality changes.
  void UpdateTextureQuality();


private:
  static WGALDevice* s_pDefaultDevice;

protected:
  WGALDevice(const WGALDeviceCreationDescription& Description);

  virtual ~WGALDevice();

  template <typename IdTableType, typename ReturnType>
  ReturnType* Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const;

  void DestroyViews(WGALTexture* pResource);

  template <typename HandleType>
  void AddDeadObject(WUInt32 uiType, HandleType handle);

  template <typename HandleType>
  void ReviveDeadObject(WUInt32 uiType, HandleType handle);

  void DestroyDeadObjects();

  void OnBindGroupInvalidatedEventHandler(WGALBindGroup* pBindGroup);

  /// Asserts that either this device supports multi-threaded resource creation, or that this function is executed on the main thread.
  void VerifyMultithreadedAccess() const;

  const WGALSwapChain* GetSwapChainInternal(WGALSwapChainHandle hSwapChain, const WRTTI* pRequestedType) const;

  WGALTextureHandle FinalizeTextureInternal(const WGALTextureCreationDescription& desc, WGALTexture* pTexture);

  template <typename Handle, typename Resource, typename Table, typename CacheTable, typename HashType>
  Handle TryGetHashedResource(HashType uiHash, Table& table, CacheTable& cacheTable, WUInt32 galObjectType, WUInt32& ref_uiCounter);
  template <typename Handle, typename Resource, typename Table, typename CacheTable, typename HashType>
  Handle InsertHashedResource(HashType uiHash, Resource* pResource, Table& table, CacheTable& cacheTable, WUInt32& ref_uiCounter);
  template <typename Resource, typename Handle, typename Table>
  void DestroyHashedResource(Handle& inout_hResource, Table& table, WUInt32 galObjectType, WUInt32& ref_uiCounter);

  WProxyAllocator m_Allocator;
  WLocalAllocatorWrapper m_AllocatorWrapper;

  using ShaderTable = WIdTable<WGALShaderHandle::IdType, WGALShader*, WLocalAllocatorWrapper>;
  using BlendStateTable = WIdTable<WGALBlendStateHandle::IdType, WGALBlendState*, WLocalAllocatorWrapper>;
  using DepthStencilStateTable = WIdTable<WGALDepthStencilStateHandle::IdType, WGALDepthStencilState*, WLocalAllocatorWrapper>;
  using RasterizerStateTable = WIdTable<WGALRasterizerStateHandle::IdType, WGALRasterizerState*, WLocalAllocatorWrapper>;
  using BufferTable = WIdTable<WGALBufferHandle::IdType, WGALBuffer*, WLocalAllocatorWrapper>;
  using DynamicBufferTable = WIdTable<WGALDynamicBufferHandle::IdType, WGALDynamicBuffer*, WLocalAllocatorWrapper>;
  using TextureTable = WIdTable<WGALTextureHandle::IdType, WGALTexture*, WLocalAllocatorWrapper>;
  using ReadbackBufferTable = WIdTable<WGALReadbackBufferHandle::IdType, WGALReadbackBuffer*, WLocalAllocatorWrapper>;
  using ReadbackTextureTable = WIdTable<WGALReadbackTextureHandle::IdType, WGALReadbackTexture*, WLocalAllocatorWrapper>;
  using SamplerStateTable = WIdTable<WGALSamplerStateHandle::IdType, WGALSamplerState*, WLocalAllocatorWrapper>;
  using RenderTargetViewTable = WIdTable<WGALRenderTargetViewHandle::IdType, WGALRenderTargetView*, WLocalAllocatorWrapper>;
  using SwapChainTable = WIdTable<WGALSwapChainHandle::IdType, WGALSwapChain*, WLocalAllocatorWrapper>;
  using VertexDeclarationTable = WIdTable<WGALVertexDeclarationHandle::IdType, WGALVertexDeclaration*, WLocalAllocatorWrapper>;
  using BindGroupLayoutTable = WIdTable<WGALBindGroupLayoutHandle::IdType, WGALBindGroupLayout*, WLocalAllocatorWrapper>;
  using BindGroupTable = WIdTable<WGALBindGroupHandle::IdType, WGALBindGroup*, WLocalAllocatorWrapper>;
  using PipelineLayoutTable = WIdTable<WGALPipelineLayoutHandle::IdType, WGALPipelineLayout*, WLocalAllocatorWrapper>;
  using GraphicsPipelineTable = WIdTable<WGALGraphicsPipelineHandle::IdType, WGALGraphicsPipeline*, WLocalAllocatorWrapper>;
  using ComputePipelineTable = WIdTable<WGALComputePipelineHandle::IdType, WGALComputePipeline*, WLocalAllocatorWrapper>;

  ShaderTable m_Shaders;
  VertexDeclarationTable m_VertexDeclarations;
  BlendStateTable m_BlendStates;
  DepthStencilStateTable m_DepthStencilStates;
  RasterizerStateTable m_RasterizerStates;
  SamplerStateTable m_SamplerStates;
  BindGroupLayoutTable m_BindGroupLayouts;
  BindGroupTable m_BindGroups;
  WDependencyTracker<WGALBindGroup*, const WGALResourceBase*> m_BindGroupTracker;
  PipelineLayoutTable m_PipelineLayouts;
  GraphicsPipelineTable m_GraphicsPipelines;
  ComputePipelineTable m_ComputePipelines;

  BufferTable m_Buffers;
  DynamicBufferTable m_DynamicBuffers;
  TextureTable m_Textures;
  ReadbackBufferTable m_ReadbackBuffers;
  ReadbackTextureTable m_ReadbackTextures;
  RenderTargetViewTable m_RenderTargetViews;
  SwapChainTable m_SwapChains;

  // Hash tables used to prevent state object duplication
  WHashTable<WUInt32, WGALShaderHandle, WHashHelper<WUInt32>, WLocalAllocatorWrapper> m_ShaderTable;
  WHashTable<WUInt32, WGALVertexDeclarationHandle, WHashHelper<WUInt32>, WLocalAllocatorWrapper> m_VertexDeclarationTable;
  WHashTable<WUInt32, WGALBlendStateHandle, WHashHelper<WUInt32>, WLocalAllocatorWrapper> m_BlendStateTable;
  WHashTable<WUInt32, WGALDepthStencilStateHandle, WHashHelper<WUInt32>, WLocalAllocatorWrapper> m_DepthStencilStateTable;
  WHashTable<WUInt32, WGALRasterizerStateHandle, WHashHelper<WUInt32>, WLocalAllocatorWrapper> m_RasterizerStateTable;
  WHashTable<WUInt32, WGALSamplerStateHandle, WHashHelper<WUInt32>, WLocalAllocatorWrapper> m_SamplerStateTable;
  WHashTable<WUInt32, WGALBindGroupLayoutHandle, WHashHelper<WUInt32>, WLocalAllocatorWrapper> m_BindGroupLayoutTable;
  WHashTable<WUInt64, WGALBindGroupHandle, WHashHelper<WUInt64>, WLocalAllocatorWrapper> m_BindGroupTable;
  WHashTable<WUInt32, WGALPipelineLayoutHandle, WHashHelper<WUInt32>, WLocalAllocatorWrapper> m_PipelineLayoutTable;
  WHashTable<WUInt32, WGALGraphicsPipelineHandle, WHashHelper<WUInt32>, WLocalAllocatorWrapper> m_GraphicsPipelineTable;
  WHashTable<WUInt32, WGALComputePipelineHandle, WHashHelper<WUInt32>, WLocalAllocatorWrapper> m_ComputePipelineTable;

  struct DeadObject
  {
    W_DECLARE_POD_TYPE();

    WUInt32 m_uiType;
    WUInt32 m_uiHandle;
  };

  WDynamicArray<DeadObject, WLocalAllocatorWrapper> m_DeadObjects;

  WGALDeviceCreationDescription m_Description;

  WGALDeviceCapabilities m_Capabilities;

  // Deactivate Doxygen document generation for the following block. (API abstraction only)
  /// \cond

  // These functions need to be implemented by a render API abstraction
protected:
  friend class WMemoryUtils;
  friend class WReadbackBufferLock;
  friend class WReadbackTextureLock;

  // Init & shutdown functions

  virtual WResult InitPlatform() = 0;
  virtual WResult ShutdownPlatform() = 0;
  virtual WStringView GetRendererPlatform() = 0;

  // Pipeline & Pass functions



  // Command Encoder

  virtual WGALCommandEncoder* BeginCommandsPlatform(const char* szName) = 0;
  virtual void EndCommandsPlatform(WGALCommandEncoder* pPass) = 0;

  // State creation functions

  virtual WGALBlendState* CreateBlendStatePlatform(const WGALBlendStateCreationDescription& Description) = 0;
  virtual void DestroyBlendStatePlatform(WGALBlendState* pBlendState) = 0;

  virtual WGALDepthStencilState* CreateDepthStencilStatePlatform(const WGALDepthStencilStateCreationDescription& Description) = 0;
  virtual void DestroyDepthStencilStatePlatform(WGALDepthStencilState* pDepthStencilState) = 0;

  virtual WGALRasterizerState* CreateRasterizerStatePlatform(const WGALRasterizerStateCreationDescription& Description) = 0;
  virtual void DestroyRasterizerStatePlatform(WGALRasterizerState* pRasterizerState) = 0;

  virtual WGALSamplerState* CreateSamplerStatePlatform(const WGALSamplerStateCreationDescription& Description) = 0;
  virtual void DestroySamplerStatePlatform(WGALSamplerState* pSamplerState) = 0;
  /// Destroys and reinitializes a sampler state in-place, applying AdjustSamplerStateDescription to pick up current quality settings.
  virtual void RecreateSamplerStatePlatform(WGALSamplerState* pSamplerState) = 0;

  virtual WGALBindGroupLayout* CreateBindGroupLayoutPlatform(const WGALBindGroupLayoutCreationDescription& Description) = 0;
  virtual void DestroyBindGroupLayoutPlatform(WGALBindGroupLayout* pBindGroupLayout) = 0;

  // Bind group platform functions
  virtual WGALBindGroup* CreateBindGroupPlatform(const WGALBindGroupCreationDescription& Description) = 0;
  virtual void DestroyBindGroupPlatform(WGALBindGroup* pBindGroup) = 0;
  /// Destroys and reinitializes a bind group in-place.
  virtual void RecreateBindGroupPlatform(WGALBindGroup* pBindGroup) = 0;

  virtual WGALPipelineLayout* CreatePipelineLayoutPlatform(const WGALPipelineLayoutCreationDescription& Description) = 0;
  virtual void DestroyPipelineLayoutPlatform(WGALPipelineLayout* pPipelineLayout) = 0;

  virtual WGALGraphicsPipeline* CreateGraphicsPipelinePlatform(const WGALGraphicsPipelineCreationDescription& Description) = 0;
  virtual void DestroyGraphicsPipelinePlatform(WGALGraphicsPipeline* pGraphicsPipeline) = 0;

  virtual WGALComputePipeline* CreateComputePipelinePlatform(const WGALComputePipelineCreationDescription& Description) = 0;
  virtual void DestroyComputePipelinePlatform(WGALComputePipeline* pComputePipeline) = 0;

  // Resource creation functions

  virtual WGALShader* CreateShaderPlatform(const WGALShaderCreationDescription& Description) = 0;
  virtual void DestroyShaderPlatform(WGALShader* pShader) = 0;

  virtual WGALBuffer* CreateBufferPlatform(const WGALBufferCreationDescription& Description, WArrayPtr<const WUInt8> pInitialData) = 0;
  virtual void DestroyBufferPlatform(WGALBuffer* pBuffer) = 0;

  virtual WGALTexture* CreateTexturePlatform(const WGALTextureCreationDescription& Description, WArrayPtr<WGALSystemMemoryDescription> pInitialData) = 0;
  virtual void DestroyTexturePlatform(WGALTexture* pTexture) = 0;

  virtual WGALTexture* CreateSharedTexturePlatform(const WGALTextureCreationDescription& Description, WArrayPtr<WGALSystemMemoryDescription> pInitialData, WEnum<WGALSharedTextureType> sharedType, WGALPlatformSharedHandle handle) = 0;
  virtual void DestroySharedTexturePlatform(WGALTexture* pTexture) = 0;

  virtual WGALReadbackBuffer* CreateReadbackBufferPlatform(const WGALBufferCreationDescription& Description) = 0;
  virtual void DestroyReadbackBufferPlatform(WGALReadbackBuffer* pReadbackBuffer) = 0;

  virtual WGALReadbackTexture* CreateReadbackTexturePlatform(const WGALTextureCreationDescription& Description) = 0;
  virtual void DestroyReadbackTexturePlatform(WGALReadbackTexture* pReadbackTexture) = 0;

  virtual WGALRenderTargetView* CreateRenderTargetViewPlatform(WGALTexture* pTexture, const WGALRenderTargetViewCreationDescription& Description) = 0;
  virtual void DestroyRenderTargetViewPlatform(WGALRenderTargetView* pRenderTargetView) = 0;

  // Other rendering creation functions

  virtual WGALVertexDeclaration* CreateVertexDeclarationPlatform(const WGALVertexDeclarationCreationDescription& Description) = 0;
  virtual void DestroyVertexDeclarationPlatform(WGALVertexDeclaration* pVertexDeclaration) = 0;

  // Resource update functions

  virtual void UpdateBufferForNextFramePlatform(const WGALBuffer* pBuffer, WConstByteArrayPtr sourceData, WUInt32 uiDestOffset) = 0;
  virtual void UpdateTextureForNextFramePlatform(const WGALTexture* pTexture, const WGALSystemMemoryDescription& sourceData, const WGALTextureSubresource& destinationSubResource, const WBoundingBoxu32& destinationBox) = 0;

  // GPU -> CPU query functions

  virtual WEnum<WGALAsyncResult> GetTimestampResultPlatform(WGALTimestampHandle hTimestamp, WTime& out_result) = 0;
  virtual WEnum<WGALAsyncResult> GetOcclusionResultPlatform(WGALOcclusionHandle hOcclusion, WUInt64& out_uiResult) = 0;
  virtual WEnum<WGALAsyncResult> GetFenceResultPlatform(WGALFenceHandle hFence, WTime timeout) = 0;
  virtual WResult LockBufferPlatform(const WGALReadbackBuffer* pBuffer, WArrayPtr<const WUInt8>& out_memory) const = 0;
  virtual void UnlockBufferPlatform(const WGALReadbackBuffer* pBuffer) const = 0;
  virtual WResult LockTexturePlatform(const WGALReadbackTexture* pTexture, const WArrayPtr<const WGALTextureSubresource>& subResources, WDynamicArray<WGALSystemMemoryDescription>& out_memory) const = 0;
  virtual void UnlockTexturePlatform(const WGALReadbackTexture* pTexture, const WArrayPtr<const WGALTextureSubresource>& subResources) const = 0;

  // Misc functions

  virtual void BeginFramePlatform(WArrayPtr<WGALSwapChain*> swapchains, const WUInt64 uiAppFrame) = 0;
  virtual void EndFramePlatform(WArrayPtr<WGALSwapChain*> swapchains) = 0;

  virtual WUInt64 GetCurrentFramePlatform() const = 0;
  virtual WUInt64 GetSafeFramePlatform() const = 0;

  virtual void FillCapabilitiesPlatform() = 0;

  virtual void FlushPlatform() = 0;
  virtual void WaitIdlePlatform() = 0;


  /// \endcond

private:
  bool m_bBeginFrameCalled = false;
  WHybridArray<WGALSwapChain*, 8> m_FrameSwapChains;
  bool m_bBeginPipelineCalled = false;
  WGALCommandEncoder* m_pCommandEncoder = nullptr;

  WUInt32 m_uiShaders = 0;
  WUInt32 m_uiVertexDeclarations = 0;
  WUInt32 m_uiBlendStates = 0;
  WUInt32 m_uiDepthStencilStates = 0;
  WUInt32 m_uiRasterizerStates = 0;
  WUInt32 m_uiSamplerStates = 0;
  WUInt32 m_uiBindGroupLayouts = 0;
  WUInt32 m_uiBindGroups = 0;
  WUInt32 m_uiPipelineLayouts = 0;
  WUInt32 m_uiGraphicsPipelines = 0;
  WUInt32 m_uiComputePipelines = 0;
  WGALCommandEncoderStats m_EncoderStats;

  WEnum<WGALTextureQuality> m_QualityModes[5];
};

#include <RendererFoundation/Device/Implementation/Device_inl.h>
