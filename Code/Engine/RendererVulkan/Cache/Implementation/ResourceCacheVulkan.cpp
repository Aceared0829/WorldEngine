#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Basics.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

WGALDeviceVulkan* WResourceCacheVulkan::s_pDevice;
vk::Device WResourceCacheVulkan::s_Device;
vk::PipelineCache WResourceCacheVulkan::s_PipelineCache;

WHashTable<WGALRenderPassDescriptor, vk::RenderPass, WResourceCacheVulkan::ResourceCacheHash> WResourceCacheVulkan::s_RenderPasses;
WHashTable<WResourceCacheVulkan::FramebufferKey, vk::Framebuffer, WResourceCacheVulkan::ResourceCacheHash> WResourceCacheVulkan::s_FrameBuffers;
WUniquePtr<WResourceCacheVulkan::FrameBufferTracker> WResourceCacheVulkan::s_pFrameBufferTracker;

// #define W_LOG_VULKAN_RESOURCES

static_assert(sizeof(WUInt32) == sizeof(WGALRenderTargetViewHandle));
namespace
{
  W_ALWAYS_INLINE WStreamWriter& operator<<(WStreamWriter& ref_stream, const WGALRenderTargetViewHandle& hValue)
  {
    ref_stream << reinterpret_cast<const WUInt32&>(hValue);
    return ref_stream;
  }

  static constexpr WUInt32 PIPELINE_CACHE_MAGIC = 0x45A9BCD7; // Arbitrary m_uiMagic number
  // Header structure for Vulkan pipeline cache
  struct PipelineCachePrefixHeader
  {
    W_DECLARE_POD_TYPE();

    WUInt32 m_uiMagic;             // An arbitrary m_uiMagic header to make sure this is actually our file
    WUInt32 m_uiDataSize;          // Equal to *pDataSize returned by vkGetPipelineCacheData
    WUInt64 m_uiDataHash;          // A hash of pipeline cache data, including the header
    WUInt32 m_uiVendorID;          // Equal to VkPhysicalDeviceProperties::vendorID
    WUInt32 m_uiDeviceID;          // Equal to VkPhysicalDeviceProperties::deviceID
    WUInt32 m_uiDriverVersion;     // Equal to VkPhysicalDeviceProperties::driverVersion
    WUInt32 m_uiDriverABI;         // Equal to sizeof(void*)
    WUInt8 m_uiUuid[VK_UUID_SIZE]; // Equal to VkPhysicalDeviceProperties::pipelineCacheUUID
  };

  WString GetPipelineCacheFilename(const vk::PhysicalDeviceProperties& deviceProperties)
  {
    WStringBuilder sCacheFilePath;
    WString sAppName = WApplication::GetApplicationInstance() ? WApplication::GetApplicationInstance()->GetApplicationName().GetView() : "WorldEngine"_wsv;
#if W_ENABLED(W_PLATFORM_ANDROID)
    // Be extra pedantic on Android and don't reuse caches on driver version changes, see https://zeux.io/2019/07/17/serializing-pipeline-cache/
    sCacheFilePath.SetFormat("{}/PipelineCache_{}_{}_{}.bin",
      WOSFile::GetUserDataFolder(sAppName),
      WArgU(deviceProperties.vendorID, 8, true, 16, true),
      WArgU(deviceProperties.deviceID, 8, true, 16, true),
      WArgU(deviceProperties.driverVersion, 8, true, 16, true));
#else
    // On desktop, we assume that vendors can actually write proper code and caches can be reused after driver updates.
    sCacheFilePath.SetFormat("{}/PipelineCache_{}_{}.bin",
      WOSFile::GetUserDataFolder(sAppName),
      WArgU(deviceProperties.vendorID, 8, true, 16, true),
      WArgU(deviceProperties.deviceID, 8, true, 16, true));
#endif
    return sCacheFilePath;
  }
} // namespace



void WResourceCacheVulkan::Initialize(WGALDeviceVulkan* pDevice, vk::Device device)
{
  s_pDevice = pDevice;
  s_Device = device;
  s_pFrameBufferTracker = W_NEW(pDevice->GetAllocator(), FrameBufferTracker);
  s_pFrameBufferTracker->m_ResourceInvalidatedEvent.AddEventHandler(&WResourceCacheVulkan::OnFrameBufferInvalidated);

  if (LoadPipelineCache(s_PipelineCache).Failed())
  {
    vk::PipelineCacheCreateInfo pipelineCacheInfo;
    pipelineCacheInfo.initialDataSize = 0;
    pipelineCacheInfo.pInitialData = nullptr;
    VK_ASSERT_DEV(s_Device.createPipelineCache(&pipelineCacheInfo, nullptr, &s_PipelineCache));
  }
}

void WResourceCacheVulkan::DeInitialize()
{
  if (s_PipelineCache)
  {
    if (SavePipelineCache().Failed())
    {
      WLog::Error("Failed to save Vulkan pipeline cache");
    }
    // Destroy the pipeline cache
    s_Device.destroyPipelineCache(s_PipelineCache, nullptr);
    s_PipelineCache = nullptr;
  }

  // Destroy other resources
  for (auto it : s_RenderPasses)
  {
    s_Device.destroyRenderPass(it.Value(), nullptr);
  }
  s_RenderPasses.Clear();
  s_RenderPasses.Compact();

  for (auto it : s_FrameBuffers)
  {
    s_Device.destroyFramebuffer(it.Value(), nullptr);
  }
  s_FrameBuffers.Clear();
  s_FrameBuffers.Compact();

  s_pFrameBufferTracker->m_ResourceInvalidatedEvent.RemoveEventHandler(&WResourceCacheVulkan::OnFrameBufferInvalidated);
  s_pFrameBufferTracker.Clear();

  s_Device = nullptr;
}

W_DEFINE_AS_POD_TYPE(vk::AttachmentDescription);
W_DEFINE_AS_POD_TYPE(vk::AttachmentReference);

vk::RenderPass WResourceCacheVulkan::RequestRenderPass(const WGALRenderPassDescriptor& renderPass)
{
  if (const vk::RenderPass* pPass = s_RenderPasses.GetValue(renderPass))
  {
    return *pPass;
  }

#ifdef W_LOG_VULKAN_RESOURCES
  WLog::Info("Creating RenderPass #{}", s_RenderPasses.GetCount());
#endif // W_LOG_VULKAN_RESOURCES

  WHybridArray<vk::AttachmentDescription, 4> attachments;
  WHybridArray<vk::AttachmentReference, 1> depthAttachmentRefs;
  WHybridArray<vk::AttachmentReference, 4> colorAttachmentRefs;

  if (renderPass.m_DepthFormat != WGALResourceFormat::Invalid)
  {
    vk::AttachmentDescription& vkAttachment = attachments.ExpandAndGetRef();
    const auto& formatInfo = s_pDevice->GetFormatLookupTable().GetFormatInfo(renderPass.m_DepthFormat);

    vkAttachment.format = formatInfo.m_format;
    vkAttachment.samples = WConversionUtilsVulkan::GetSamples(renderPass.m_Msaa);
    vkAttachment.loadOp = WConversionUtilsVulkan::GetAttachmentLoadOp(renderPass.m_DepthLoadOp);
    vkAttachment.storeOp = WConversionUtilsVulkan::GetAttachmentStoreOp(renderPass.m_DepthStoreOp);
    vkAttachment.stencilLoadOp = WConversionUtilsVulkan::GetAttachmentLoadOp(renderPass.m_StencilLoadOp);
    vkAttachment.stencilStoreOp = WConversionUtilsVulkan::GetAttachmentStoreOp(renderPass.m_StencilStoreOp);
    vkAttachment.initialLayout = vkAttachment.loadOp == vk::AttachmentLoadOp::eLoad || vkAttachment.stencilLoadOp == vk::AttachmentLoadOp::eLoad ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eUndefined;
    vkAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference& depthAttachment = depthAttachmentRefs.ExpandAndGetRef();
    depthAttachment.attachment = attachments.GetCount() - 1;
    depthAttachment.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  }

  const WUInt32 uiCount = renderPass.m_uiRTCount;
  for (WUInt32 i = 0; i < uiCount; i++)
  {
    vk::AttachmentDescription& vkAttachment = attachments.ExpandAndGetRef();
    const auto& formatInfo = s_pDevice->GetFormatLookupTable().GetFormatInfo(renderPass.m_ColorFormat[i]);

    vkAttachment.format = formatInfo.m_format;
    vkAttachment.samples = WConversionUtilsVulkan::GetSamples(renderPass.m_Msaa);
    vkAttachment.loadOp = WConversionUtilsVulkan::GetAttachmentLoadOp(renderPass.m_ColorLoadOp[i]);
    vkAttachment.storeOp = WConversionUtilsVulkan::GetAttachmentStoreOp(renderPass.m_ColorStoreOp[i]);
    vkAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    vkAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    vkAttachment.initialLayout = vkAttachment.loadOp == vk::AttachmentLoadOp::eLoad ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::eUndefined;
    vkAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference& colorAttachment = colorAttachmentRefs.ExpandAndGetRef();
    colorAttachment.attachment = attachments.GetCount() - 1;
    colorAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
  }

  const bool bHasColor = !colorAttachmentRefs.IsEmpty();
  const bool bHasDepth = !depthAttachmentRefs.IsEmpty();
  vk::SubpassDescription subpass;
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = colorAttachmentRefs.GetCount();
  subpass.pColorAttachments = bHasColor ? colorAttachmentRefs.GetData() : nullptr;
  subpass.pDepthStencilAttachment = bHasDepth ? depthAttachmentRefs.GetData() : nullptr;

  vk::SubpassDependency dependency;
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion; // VK_DEPENDENCY_BY_REGION_BIT;

  dependency.srcAccessMask = {};
  if (bHasColor)
    dependency.dstAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;

  if (bHasDepth)
    dependency.dstAccessMask |= vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;

  dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
  dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;

  vk::RenderPassCreateInfo renderPassCreateInfo;
  renderPassCreateInfo.attachmentCount = attachments.GetCount();
  renderPassCreateInfo.pAttachments = attachments.GetData();
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpass;
  renderPassCreateInfo.dependencyCount = 1;
  renderPassCreateInfo.pDependencies = &dependency;

  vk::RenderPass vkRenderPass;
  VK_LOG_ERROR(s_Device.createRenderPass(&renderPassCreateInfo, nullptr, &vkRenderPass));

  s_RenderPasses.Insert(renderPass, vkRenderPass);
  return vkRenderPass;
}

vk::Framebuffer WResourceCacheVulkan::RequestFrameBuffer(vk::RenderPass vkRenderPass, const WGALFrameBufferDescriptor& frameBuffer)
{
  FramebufferKey key;
  key.m_renderPass = vkRenderPass;
  key.m_frameBuffer = frameBuffer;

  if (const vk::Framebuffer* pFrameBuffer = s_FrameBuffers.GetValue(key))
  {
    return *pFrameBuffer;
  }

#ifdef W_LOG_VULKAN_RESOURCES
  WLog::Info("Creating FrameBuffer #{}", s_FrameBuffers.GetCount());
#endif // W_LOG_VULKAN_RESOURCES

  WHybridArray<vk::ImageView, W_GAL_MAX_RENDERTARGET_COUNT + 1> attachments;
  if (!frameBuffer.m_hDepthTarget.IsInvalidated())
  {
    const WGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const WGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(frameBuffer.m_hDepthTarget));
    attachments.PushBack(pRenderTargetView->GetImageView());
  }
  for (WUInt32 i = 0; i < W_GAL_MAX_RENDERTARGET_COUNT; ++i)
  {
    if (frameBuffer.m_hColorTarget[i].IsInvalidated())
      break;

    const WGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const WGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(frameBuffer.m_hColorTarget[i]));
    attachments.PushBack(pRenderTargetView->GetImageView());
  }

  vk::FramebufferCreateInfo framebufferInfo;
  framebufferInfo.renderPass = vkRenderPass;
  framebufferInfo.attachmentCount = attachments.GetCount();
  framebufferInfo.pAttachments = attachments.GetData();
  framebufferInfo.width = frameBuffer.m_Size.width;
  framebufferInfo.height = frameBuffer.m_Size.height;
  framebufferInfo.layers = frameBuffer.m_uiSliceCount;

  vk::Framebuffer vkFrameBuffer;
  VK_LOG_ERROR(s_Device.createFramebuffer(&framebufferInfo, nullptr, &vkFrameBuffer));

  s_FrameBuffers.Insert(key, vkFrameBuffer);

  WSet<vk::ImageView> dependencies(WTempAllocatorWrapper::GetAllocator());
  for (vk::ImageView attachment : attachments)
  {
    dependencies.Insert(attachment);
  }
  s_pFrameBufferTracker->AddResource(key, dependencies);

  return vkFrameBuffer;
}

void WResourceCacheVulkan::RenderTargetViewDestroyed(vk::ImageView imageView)
{
  s_pFrameBufferTracker->DependencyDestroyed(imageView);
}

void WResourceCacheVulkan::OnFrameBufferInvalidated(FramebufferKey key)
{
  // DependencyDestroyed only severed the link to the destroyed view, the links to the other attachments are still around.
  s_pFrameBufferTracker->RemoveResource(key);

  vk::Framebuffer frameBuffer;
  if (s_FrameBuffers.Remove(key, &frameBuffer))
  {
    s_pDevice->DeleteLater(frameBuffer);
  }
}

WResult WResourceCacheVulkan::SavePipelineCache()
{
  // Get physical device properties for cache validation
  vk::PhysicalDeviceProperties deviceProperties = s_pDevice->GetVulkanPhysicalDevice().getProperties();

  // Get the cache data
  size_t dataSize = 0;
  VK_SUCCEED_OR_RETURN_W_FAILURE(s_Device.getPipelineCacheData(s_PipelineCache, &dataSize, nullptr));
  if (dataSize == 0)
    return W_SUCCESS;

  WDynamicArray<WUInt8> pipelineCacheData;
  pipelineCacheData.SetCountUninitialized(static_cast<WUInt32>(dataSize));
  VK_SUCCEED_OR_RETURN_W_FAILURE(s_Device.getPipelineCacheData(s_PipelineCache, &dataSize, pipelineCacheData.GetData()));

  // Create our custom prefix header
  PipelineCachePrefixHeader prefixHeader;
  prefixHeader.m_uiMagic = PIPELINE_CACHE_MAGIC;
  prefixHeader.m_uiDataSize = static_cast<WUInt32>(dataSize);
  prefixHeader.m_uiVendorID = deviceProperties.vendorID;
  prefixHeader.m_uiDeviceID = deviceProperties.deviceID;
  prefixHeader.m_uiDriverVersion = deviceProperties.driverVersion;
  prefixHeader.m_uiDriverABI = sizeof(void*);
  for (WUInt32 i = 0; i < VK_UUID_SIZE; ++i)
  {
    prefixHeader.m_uiUuid[i] = deviceProperties.pipelineCacheUUID[i];
  }
  prefixHeader.m_uiDataHash = WHashingUtils::xxHash64(pipelineCacheData.GetData(), dataSize);

  WString sCacheFilePath = GetPipelineCacheFilename(deviceProperties);
  WStringBuilder sTempFilePath;
  sTempFilePath.SetFormat("{}_temp", sCacheFilePath);
  // Write to the temporary file first
  {
    WOSFile file;
    // WFileWriter file;
    W_SUCCEED_OR_RETURN(file.Open(sTempFilePath, WFileOpenMode::Write, WFileShareMode::Exclusive));
    W_SUCCEED_OR_RETURN(file.Write(&prefixHeader, sizeof(PipelineCachePrefixHeader)));
    W_SUCCEED_OR_RETURN(file.Write(pipelineCacheData.GetData(), dataSize));
    file.Close();
  }

  // Now rename the temporary file to the final file
  W_SUCCEED_OR_RETURN(WOSFile::DeleteFile(sCacheFilePath));
  W_SUCCEED_OR_RETURN(WOSFile::MoveFileOrDirectory(sTempFilePath, sCacheFilePath));
  return W_SUCCESS;
}

WResult WResourceCacheVulkan::LoadPipelineCache(vk::PipelineCache& out_pipelineCache)
{
  // Pipeline cache implementation following https://zeux.io/2019/07/17/serializing-pipeline-cache/
  vk::PhysicalDeviceProperties deviceProperties = s_pDevice->GetVulkanPhysicalDevice().getProperties();

  // Try to load the pipeline cache from file
  WString cacheFilePath = GetPipelineCacheFilename(deviceProperties);

  WOSFile file;
  W_SUCCEED_OR_RETURN(file.Open(cacheFilePath, WFileOpenMode::Read, WFileShareMode::Default));

  // Read our custom prefix header
  PipelineCachePrefixHeader prefixHeader;
  WUInt64 uiReadSize = file.Read(&prefixHeader, sizeof(PipelineCachePrefixHeader));
  if (uiReadSize != sizeof(PipelineCachePrefixHeader))
    return W_FAILURE;

  bool bCacheUuidValid = true;
  for (WUInt32 i = 0; i < VK_UUID_SIZE; ++i)
  {
    if (prefixHeader.m_uiUuid[i] != deviceProperties.pipelineCacheUUID[i])
    {
      bCacheUuidValid = false;
      break;
    }
  }
  const bool bIsValid =
    bCacheUuidValid && prefixHeader.m_uiMagic == PIPELINE_CACHE_MAGIC && prefixHeader.m_uiVendorID == deviceProperties.vendorID && prefixHeader.m_uiDeviceID == deviceProperties.deviceID
#if W_ENABLED(W_PLATFORM_ANDROID)
    && prefixHeader.m_uiDriverVersion == deviceProperties.driverVersion && prefixHeader.m_uiDriverABI == sizeof(void*)
#endif
    ;

  if (!bIsValid)
    return W_FAILURE;

  // Read the cache data into memory
  WDynamicArray<WUInt8> initialData;
  initialData.SetCountUninitialized(prefixHeader.m_uiDataSize);
  uiReadSize = file.Read(initialData.GetData(), initialData.GetCount());
  if (uiReadSize != prefixHeader.m_uiDataSize)
    return W_FAILURE;

  file.Close();

  // Verify hash
  const WUInt64 computedHash = WHashingUtils::xxHash64(initialData.GetData(), initialData.GetCount());
  if (computedHash != prefixHeader.m_uiDataHash)
    return W_FAILURE;

  // Create the pipeline cache
  vk::PipelineCacheCreateInfo pipelineCacheInfo;
  pipelineCacheInfo.initialDataSize = initialData.GetCount();
  pipelineCacheInfo.pInitialData = initialData.GetData();
  if (s_Device.createPipelineCache(&pipelineCacheInfo, nullptr, &s_PipelineCache) != vk::Result::eSuccess)
    return W_FAILURE;

  return W_SUCCESS;
}

WUInt32 WResourceCacheVulkan::ResourceCacheHash::Hash(const WGALRenderPassDescriptor& desc)
{
  return desc.CalculateHash();
}

bool WResourceCacheVulkan::ResourceCacheHash::Equal(const WGALRenderPassDescriptor& a, const WGALRenderPassDescriptor& b)
{
  bool equal = a == b;
  return equal;
}

WUInt32 WResourceCacheVulkan::ResourceCacheHash::Hash(const FramebufferKey& key)
{
  WHashStreamWriter32 writer;
  writer << key.m_renderPass;
  writer << key.m_frameBuffer.CalculateHash();
  return writer.GetHashValue();
}

bool WResourceCacheVulkan::ResourceCacheHash::Equal(const FramebufferKey& a, const FramebufferKey& b)
{
  return a.m_renderPass == b.m_renderPass && a.m_frameBuffer == b.m_frameBuffer;
}

