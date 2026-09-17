#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererVulkan/Device/DispatchContext.h>
// #include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <RendererVulkan/Device/DeclarationsVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

W_DEFINE_AS_POD_TYPE(vk::Format);

class WGALCommandEncoderImplVulkan;
class WFenceQueueVulkan;

struct WGALFormatLookupEntryVulkan
{
  WGALFormatLookupEntryVulkan() = default;
  WGALFormatLookupEntryVulkan(vk::Format format)
  {
    m_format = format;
    m_readback = format;
  }

  WGALFormatLookupEntryVulkan(vk::Format format, WArrayPtr<vk::Format> mutableFormats)
  {
    m_format = format;
    m_readback = format;
    m_mutableFormats = mutableFormats;
  }

  inline WGALFormatLookupEntryVulkan& R(vk::Format readbackType)
  {
    m_readback = readbackType;
    return *this;
  }

  vk::Format m_format = vk::Format::eUndefined;
  vk::Format m_readback = vk::Format::eUndefined;
  WHybridArray<vk::Format, 6> m_mutableFormats;
};

using WGALFormatLookupTableVulkan = WGALFormatLookupTable<WGALFormatLookupEntryVulkan>;

class WGALBufferVulkan;
class WGALTextureVulkan;
class WCommandBufferPoolVulkan;
class WStagingBufferPoolVulkan;
class WQueryPoolVulkan;
class WInitContextVulkan;
class WDescriptorWritePoolVulkan;

/// The Vulkan device implementation of the graphics abstraction layer.
class W_RENDERERVULKAN_DLL WGALDeviceVulkan : public WGALDevice
{
private:
  friend WInternal::NewInstance<WGALDevice> CreateVulkanDevice(WAllocator* pAllocator, const WGALDeviceCreationDescription& description);
  WGALDeviceVulkan(const WGALDeviceCreationDescription& Description);

public:
  virtual ~WGALDeviceVulkan();

public:
  struct PendingDeletionFlags
  {
    using StorageType = WUInt32;

    enum Enum
    {
      UsesExternalMemory = W_BIT(0),
      IsFileDescriptor = W_BIT(1),
      Default = 0
    };

    struct Bits
    {
      StorageType UsesExternalMemory : 1;
      StorageType IsFileDescriptor : 1;
    };
  };

  struct PendingDeletion
  {
    W_DECLARE_POD_TYPE();
    vk::ObjectType m_type;                    ///< What type to cast m_pObject to.
    WBitflags<PendingDeletionFlags> m_flags; ///< In case m_type == eUnknown, defines the custom deletion to be performed.
    void* m_pObject;                          ///< The object to be deleted, usually cast to a vk::* type.
    union
    {
      WVulkanAllocation m_allocation;        ///< For convenience to omit casting of m_pContext.
      void* m_pContext;                       ///< 64bit of context data.
    };
  };

  struct ReclaimResource
  {
    W_DECLARE_POD_TYPE();
    vk::ObjectType m_type;      ///< What type to cast m_pObject to.
    WUInt32 m_Data;            ///< 32bit of context data.
    void* m_pObject = nullptr;  ///< The object to be reclaimed, usually cast to a vk::* type.
    void* m_pContext = nullptr; ///< 64bit of context data. Usually the object that reclaims the resource.
  };

  struct Extensions
  {
    bool m_bSurface = false;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    bool m_bWin32Surface = false;
#elif W_ENABLED(W_SUPPORTS_GLFW)
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    bool m_bAndroidSurface = false;
#else
#  error "Vulkan Platform not supported"
#endif

#if W_ENABLED(W_PLATFORM_LINUX)
    bool m_bSurfaceXcb = false;
#endif

    bool m_bDebugUtils = false;
    bool m_bDebugUtilsMarkers = false;

    bool m_bDeviceSwapChain = false;
    bool m_bShaderViewportIndexLayer = false;

    bool m_bConservativeRasterization = false;

    vk::PhysicalDeviceCustomBorderColorFeaturesEXT m_borderColorEXT;
    bool m_bBorderColorFloat = false;

    bool m_bImageFormatList = false;
    vk::PhysicalDeviceTimelineSemaphoreFeatures m_timelineSemaphoresEXT;
    bool m_bTimelineSemaphore = false;

    vk::PhysicalDeviceSynchronization2Features m_synchronization2Features;
    bool m_bSynchronization2 = false;

    bool m_bExternalMemoryCapabilities = false;
    bool m_bExternalSemaphoreCapabilities = false;
    bool m_bExternalFenceCapabilities = false;

    bool m_bExternalMemory = false;
    bool m_bExternalSemaphore = false;

    bool m_bExternalMemoryFd = false;
    bool m_bExternalSemaphoreFd = false;

    bool m_bExternalMemoryWin32 = false;
    bool m_bExternalSemaphoreWin32 = false;

    bool m_bPhysicalDeviceProperties2 = false;

    bool m_bSurfaceCapabilities2 = false;
    bool m_bSurfaceMaintenance1 = false;
    vk::PhysicalDeviceSwapchainMaintenance1FeaturesKHR m_swapchainMaintenance1Features;
    bool m_bSwapchainMaintenance1 = false;
  };

  struct Queue
  {
    vk::Queue m_queue;
    WUInt32 m_uiQueueFamily = -1;
    WUInt32 m_uiQueueIndex = 0;
  };

  vk::Instance GetVulkanInstance() const;
  vk::Device GetVulkanDevice() const;
  const Queue& GetGraphicsQueue() const;
  const Queue& GetTransferQueue() const;

  vk::PhysicalDevice GetVulkanPhysicalDevice() const;
  W_ALWAYS_INLINE const vk::PhysicalDeviceProperties& GetPhysicalDeviceProperties() const { return m_Properties.properties; }
  vk::PhysicalDeviceFeatures2 GetPhysicalDeviceFeatures(void* pNext = nullptr) const;

  const Extensions& GetExtensions() const { return m_Extensions; }
  const WVulkanDispatchContext& GetDispatchContext() const { return m_DispatchContext; }
  vk::PipelineStageFlags GetSupportedStages() const;

  /// Shader stages the device lacks the features for. These must be masked out of every pipeline barrier, otherwise the stage mask is invalid (VUID-vkCmdPipelineBarrier-srcStageMask-04996).
  vk::PipelineStageFlags GetUnsupportedStages() const;

  vk::CommandBuffer& GetCurrentCommandBuffer();
  WQueryPoolVulkan& GetQueryPool() const;
  WFenceQueueVulkan& GetFenceQueue() const;
  WStagingBufferPoolVulkan& GetStagingBufferPool() const;
  WInitContextVulkan& GetInitContext() const;
  WDescriptorWritePoolVulkan& GetDescriptorWritePool() const;


  WGALTextureHandle CreateTextureInternal(const WGALTextureCreationDescription& description, WArrayPtr<WGALSystemMemoryDescription> initialData);
  WGALBufferHandle CreateBufferInternal(const WGALBufferCreationDescription& description, WArrayPtr<const WUInt8> initialData);

  const WGALFormatLookupTableVulkan& GetFormatLookupTable() const;

  WInt32 GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const;

  vk::Fence Submit(bool bAddSignalSemaphore = true, bool bAddUpdateForNextFrameCommands = false);

  void DeleteLaterImpl(const PendingDeletion& deletion);

  void DeleteLater(vk::Image& ref_image, vk::DeviceMemory& ref_externalMemory)
  {
    if (ref_image)
    {
      PendingDeletion del = {vk::ObjectType::eImage, {PendingDeletionFlags::UsesExternalMemory}, (void*)ref_image, nullptr};
      del.m_pContext = (void*)ref_externalMemory;
      DeleteLaterImpl(del);
    }
    ref_image = nullptr;
    ref_externalMemory = nullptr;
  }

  template <typename T>
  void DeleteLater(T& ref_object, WVulkanAllocation& ref_pAllocation)
  {
    if (ref_object)
    {
      DeleteLaterImpl({ref_object.objectType, {}, (void*)ref_object, ref_pAllocation});
    }
    ref_object = nullptr;
    ref_pAllocation = nullptr;
  }

  template <typename T>
  void DeleteLater(T& ref_object, void* pContext, WBitflags<PendingDeletionFlags> flags = {})
  {
    if (ref_object)
    {
      PendingDeletion del = {ref_object.objectType, flags, (void*)ref_object, nullptr};
      del.m_pContext = pContext;
      DeleteLaterImpl(static_cast<const PendingDeletion&>(del));
    }
    ref_object = nullptr;
  }

  template <typename T>
  void DeleteLater(T& ref_object)
  {
    if (ref_object)
    {
      DeleteLaterImpl({ref_object.objectType, {}, (void*)ref_object, nullptr});
    }
    ref_object = nullptr;
  }

  void ReclaimLater(const ReclaimResource& reclaim);

  template <typename T>
  void ReclaimLater(T& ref_object, void* pContext = nullptr, WUInt32 uiData = 0)
  {
    ReclaimLater({ref_object.objectType, uiData, (void*)ref_object, pContext});
    ref_object = nullptr;
  }

  void SetDebugName(const vk::DebugUtilsObjectNameInfoEXT& info, WVulkanAllocation pAllocation = nullptr);

  template <typename T>
  void SetDebugName(const char* szName, T& ref_object, WVulkanAllocation pAllocation = nullptr)
  {
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    if (ref_object)
    {
      vk::DebugUtilsObjectNameInfoEXT nameInfo;
      nameInfo.objectType = ref_object.objectType;
      nameInfo.objectHandle = (uint64_t) static_cast<typename T::NativeType>(ref_object);
      nameInfo.pObjectName = szName;

      SetDebugName(nameInfo, pAllocation);
    }
#endif
  }

  void ReportLiveGpuObjects();

  static void UploadBufferStaging(WGALDeviceVulkan& ref_device, WStagingBufferPoolVulkan* pStagingBufferPool, vk::CommandBuffer commandBuffer, const WGALBufferVulkan* pBuffer, WArrayPtr<const WUInt8> initialData, vk::DeviceSize dstOffset = 0);
  static void UploadTextureStaging(WGALDeviceVulkan& ref_device, WStagingBufferPoolVulkan* pStagingBufferPool, vk::CommandBuffer commandBuffer, const WGALTextureVulkan* pTexture, const vk::ImageSubresourceLayers& subResource, const vk::Offset3D& imageOffset, const vk::Extent3D& imageExtent, const WGALSystemMemoryDescription& data);

  struct OnBeforeImageDestroyedData
  {
    vk::Image image;
    WGALDeviceVulkan& GALDeviceVulkan;
  };
  WEvent<OnBeforeImageDestroyedData> OnBeforeImageDestroyed;

  virtual const WGALSharedTexture* GetSharedTexture(WGALTextureHandle hTexture) const override;

  struct SemaphoreInfo
  {
    static SemaphoreInfo MakeWaitSemaphore(vk::Semaphore semaphore, vk::PipelineStageFlagBits waitStage = vk::PipelineStageFlagBits::eAllCommands, vk::SemaphoreType type = vk::SemaphoreType::eBinary, WUInt64 uiValue = 0)
    {
      return SemaphoreInfo{semaphore, type, waitStage, uiValue};
    }

    static SemaphoreInfo MakeSignalSemaphore(vk::Semaphore semaphore, vk::SemaphoreType type = vk::SemaphoreType::eBinary, WUInt64 uiValue = 0)
    {
      return SemaphoreInfo{semaphore, type, vk::PipelineStageFlagBits::eNone, uiValue};
    }

    vk::Semaphore m_semaphore;
    vk::SemaphoreType m_type = vk::SemaphoreType::eBinary;
    vk::PipelineStageFlagBits m_waitStage = vk::PipelineStageFlagBits::eAllCommands;
    WUInt64 m_uiValue = 0;
  };
  void AddWaitSemaphore(const SemaphoreInfo& waitSemaphore);
  void AddSignalSemaphore(const SemaphoreInfo& signalSemaphore);

  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  vk::Result SelectInstanceExtensions(WDynamicArray<WString>& extensions);
  vk::Result SelectDeviceExtensions(vk::DeviceCreateInfo& deviceCreateInfo, WDynamicArray<WString>& extensions);

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

  // Misc functions

  virtual void BeginFramePlatform(WArrayPtr<WGALSwapChain*> swapchains, const WUInt64 uiAppFrame) override;
  virtual void EndFramePlatform(WArrayPtr<WGALSwapChain*> swapchains) override;
  virtual WUInt64 GetCurrentFramePlatform() const override;
  virtual WUInt64 GetSafeFramePlatform() const override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  /// \endcond

private:
  void WaitIdleInternal(bool bAddUpdateForNextFrameCommands);

  struct PerFrameData
  {
    /// These are all fences passed into submit calls. For some reason waiting for the fence of the last submit is not enough. At least I can't get it to work (neither semaphores nor barriers make it past the validation layer).
    WHybridArray<vk::Fence, 2> m_CommandBufferFences;

    vk::CommandBuffer m_currentCommandBuffer;
    WUInt64 m_uiFrame = -1;

    WMutex m_pendingDeletionsMutex;
    WDeque<PendingDeletion> m_pendingDeletions;
    WDeque<PendingDeletion> m_pendingDeletionsPrevious;

    WMutex m_reclaimResourcesMutex;
    WDeque<ReclaimResource> m_reclaimResources;
    WDeque<ReclaimResource> m_reclaimResourcesPrevious;
  };

  void DeletePendingResources(WDeque<PendingDeletion>& pendingDeletions);
  void ReclaimResources(WDeque<ReclaimResource>& resources);

  void FillFormatLookupTable();

  static constexpr WUInt32 FRAMES = 4;

  // These are atomic as the WInitContextVulkan is accessing these on worker threads when uploading resources in the background.
  WAtomicInteger<WUInt64> m_uiFrameCounter = 1; ///< We start at 1 so m_uiFrameCounter and m_uiSafeFrame are not equal at the start.
  WAtomicInteger<WUInt64> m_uiSafeFrame = 0;
  WUInt8 m_uiCurrentPerFrameData = m_uiFrameCounter % FRAMES;

  vk::Instance m_Instance;
  vk::PhysicalDevice m_PhysicalDevice;
  vk::PhysicalDeviceProperties2 m_Properties;
  vk::Device m_Device;
  Queue m_GraphicsQueue;
  Queue m_TransferQueue;

  WGALFormatLookupTableVulkan m_FormatLookupTable;
  vk::PipelineStageFlags m_SupportedStages;
  vk::PipelineStageFlags m_UnsupportedStages;
  vk::PhysicalDeviceMemoryProperties m_MemoryProperties;

  WUniquePtr<WGALCommandEncoderImplVulkan> m_pCommandEncoderImpl;
  WUniquePtr<WGALCommandEncoder> m_pCommandEncoder;

  WUniquePtr<WCommandBufferPoolVulkan> m_pCommandBufferPool;
  WUniquePtr<WStagingBufferPoolVulkan> m_pStagingBufferPool;
  WUniquePtr<WQueryPoolVulkan> m_pQueryPool;
  WUniquePtr<WFenceQueueVulkan> m_pFenceQueue;
  WUniquePtr<WInitContextVulkan> m_pInitContext;

  WDynamicArray<WPendingBufferCopyVulkan, WLocalAllocatorWrapper> m_PendingBufferCopies;
  WDynamicArray<WPendingTextureCopyVulkan, WLocalAllocatorWrapper> m_PendingTextureCopies;

  // We daisy-chain all command buffers in a frame in sequential order via this semaphore for now.
  vk::Semaphore m_LastCommandBufferFinished;

  PerFrameData m_PerFrameData[FRAMES];

#if W_ENABLED(W_USE_PROFILING)
  struct GPUTimingScope* m_pFrameTimingScope = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope = nullptr;
#endif

  Extensions m_Extensions;
  WVulkanDispatchContext m_DispatchContext;
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  vk::DebugUtilsMessengerEXT m_DebugMessenger = nullptr;
#endif
  WHybridArray<SemaphoreInfo, 3> m_WaitSemaphores;
  WHybridArray<SemaphoreInfo, 3> m_SignalSemaphores;
};

#include <RendererVulkan/Device/Implementation/DeviceVulkan_inl.h>
