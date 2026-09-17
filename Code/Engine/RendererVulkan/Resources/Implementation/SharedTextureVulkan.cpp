#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/System/Process.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Resources/SharedTextureVulkan.h>

#if W_ENABLED(W_PLATFORM_LINUX)
#  include <errno.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#endif

WGALSharedTextureVulkan::WGALSharedTextureVulkan(const WGALTextureCreationDescription& Description, WEnum<WGALSharedTextureType> sharedType, WGALPlatformSharedHandle hSharedHandle)
  : WGALTextureVulkan(Description)
  , m_SharedType(sharedType)
  , m_hSharedHandle(hSharedHandle)
{
}

WGALSharedTextureVulkan::~WGALSharedTextureVulkan() = default;

WResult WGALSharedTextureVulkan::InitPlatform(WGALDevice* pDevice, WArrayPtr<WGALSystemMemoryDescription> pInitialData)
{
  m_pDevice = static_cast<WGALDeviceVulkan*>(pDevice);

  vk::ImageFormatListCreateInfo imageFormats;
  vk::ImageCreateInfo createInfo = {};

  m_ImageFormat = ComputeImageFormat(m_pDevice, m_Description.m_Format, createInfo, imageFormats);

  ComputeCreateInfo(m_pDevice, m_Description, createInfo);

  if (m_Description.m_pExisitingNativeObject == nullptr)
  {
    vk::ExternalMemoryImageCreateInfo extMemoryCreateInfo;
    if (m_SharedType == WGALSharedTextureType::Exported || m_SharedType == WGALSharedTextureType::Imported)
    {
#if W_ENABLED(W_PLATFORM_LINUX)
      extMemoryCreateInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
#elif W_ENABLED(W_PLATFORM_WINDOWS)
      extMemoryCreateInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
#endif
      extMemoryCreateInfo.pNext = createInfo.pNext;
      createInfo.pNext = &extMemoryCreateInfo;
    }

    if (m_SharedType == WGALSharedTextureType::None || m_SharedType == WGALSharedTextureType::Exported)
    {

      WVulkanAllocationCreateInfo allocInfo;
      ComputeAllocInfo(allocInfo);

      if (m_SharedType == WGALSharedTextureType::Exported)
      {
        allocInfo.m_bExportSharedAllocation = true;
      }

      vk::ImageFormatProperties props2;
      VK_ASSERT_DEBUG(m_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(createInfo.format, createInfo.imageType, createInfo.tiling, createInfo.usage, createInfo.flags, &props2));
      VK_SUCCEED_OR_RETURN_W_FAILURE(WMemoryAllocatorVulkan::CreateImage(createInfo, allocInfo, m_Image, m_pAlloc, &m_AllocInfo));

      if (m_SharedType == WGALSharedTextureType::Exported)
      {
        if (!m_pDevice->GetExtensions().m_bTimelineSemaphore)
        {
          WLog::Error("Can not create shared textures because timeline semaphores are not supported");
          return W_FAILURE;
        }

#if W_ENABLED(W_PLATFORM_LINUX) && defined(SYS_pidfd_getfd)
        if (!m_pDevice->GetExtensions().m_bExternalMemoryFd)
        {
          WLog::Error("Can not create shared textures because external memory fd is not supported");
          return W_FAILURE;
        }

        if (!m_pDevice->GetExtensions().m_bExternalSemaphoreFd)
        {
          WLog::Error("Can not create shared textures because external semaphore fd is not supported");
          return W_FAILURE;
        }

        vk::MemoryGetFdInfoKHR getWin32HandleInfo{m_AllocInfo.m_deviceMemory, vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd};
        int fd = -1;
        vk::Device device = m_pDevice->GetVulkanDevice();
        VK_SUCCEED_OR_RETURN_W_FAILURE(device.getMemoryFdKHR(&getWin32HandleInfo, &fd, m_pDevice->GetDispatchContext()));
        m_hSharedHandle.m_uiProcessId = WProcess::GetCurrentProcessID();
        m_hSharedHandle.m_hSharedTexture = (size_t)fd;
        m_hSharedHandle.m_uiMemoryTypeIndex = m_AllocInfo.m_memoryType;
        m_hSharedHandle.m_uiSize = m_AllocInfo.m_size;

        vk::ExportSemaphoreCreateInfoKHR exportInfo{vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd};
        vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0, &exportInfo};
        vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
        m_SharedSemaphore = device.createSemaphore(semCreateInfo);

        int semaphoreFd = -1;
        vk::SemaphoreGetFdInfoKHR getSemaphoreWin32Info{m_SharedSemaphore, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd};
        VK_SUCCEED_OR_RETURN_W_FAILURE(device.getSemaphoreFdKHR(&getSemaphoreWin32Info, &semaphoreFd, m_pDevice->GetDispatchContext()));
        m_hSharedHandle.m_hSemaphore = (size_t)semaphoreFd;
#elif W_ENABLED(W_PLATFORM_WINDOWS)
        if (!m_pDevice->GetExtensions().m_bExternalMemoryWin32)
        {
          WLog::Error("Can not create shared textures because external memory win32 is not supported");
          return W_FAILURE;
        }

        if (!m_pDevice->GetExtensions().m_bExternalSemaphoreWin32)
        {
          WLog::Error("Can not create shared textures because external semaphore win32 is not supported");
          return W_FAILURE;
        }

        vk::Device device = m_pDevice->GetVulkanDevice();
        vk::MemoryGetWin32HandleInfoKHR getWin32HandleInfo{m_AllocInfo.m_deviceMemory, vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32};
        HANDLE handle = 0;
        VK_SUCCEED_OR_RETURN_W_FAILURE(device.getMemoryWin32HandleKHR(&getWin32HandleInfo, &handle, m_pDevice->GetDispatchContext()));
        m_hSharedHandle.m_uiProcessId = WProcess::GetCurrentProcessID();
        m_hSharedHandle.m_hSharedTexture = (size_t)handle;
        m_hSharedHandle.m_uiMemoryTypeIndex = m_AllocInfo.m_memoryType;
        m_hSharedHandle.m_uiSize = m_AllocInfo.m_size;

        vk::ExportSemaphoreWin32HandleInfoKHR exportInfoWin32;
        exportInfoWin32.dwAccess = GENERIC_ALL;
        vk::ExportSemaphoreCreateInfoKHR exportInfo{vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32, &exportInfoWin32};
        vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0, &exportInfo};
        vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
        VK_SUCCEED_OR_RETURN_W_FAILURE(device.createSemaphore(&semCreateInfo, nullptr, &m_SharedSemaphore));

        HANDLE semaphoreHandle = 0;
        vk::SemaphoreGetWin32HandleInfoKHR getSemaphoreWin32Info{m_SharedSemaphore, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32};
        VK_SUCCEED_OR_RETURN_W_FAILURE(device.getSemaphoreWin32HandleKHR(&getSemaphoreWin32Info, &semaphoreHandle, m_pDevice->GetDispatchContext()));
        m_hSharedHandle.m_hSemaphore = (size_t)semaphoreHandle;
#else
        W_ASSERT_NOT_IMPLEMENTED
#endif
      }
    }
    else if (m_SharedType == WGALSharedTextureType::Imported)
    {
      if (!m_pDevice->GetExtensions().m_bTimelineSemaphore)
      {
        WLog::Error("Can not open shared texture: timeline semaphores not supported");
        return W_FAILURE;
      }

#if W_ENABLED(W_PLATFORM_LINUX) && defined(SYS_pidfd_getfd)
      if (m_hSharedHandle.m_hSharedTexture == 0 || m_hSharedHandle.m_hSemaphore == 0)
      {
        WLog::Error("Can not open shared texture: invalid handle given");
        return W_FAILURE;
      }


      if (!m_pDevice->GetExtensions().m_bExternalMemoryFd)
      {
        WLog::Error("Can not open shared texture: external memory fd not supported");
        return W_FAILURE;
      }

      if (!m_pDevice->GetExtensions().m_bExternalSemaphoreFd)
      {
        WLog::Error("Can not open shared texture: external semaphore fd not supported");
        return W_FAILURE;
      }

      bool bNeedToImportForeignProcessFileDescriptors = m_hSharedHandle.m_uiProcessId != WProcess::GetCurrentProcessID();
      if (bNeedToImportForeignProcessFileDescriptors)
      {
        int processFd = syscall(SYS_pidfd_open, m_hSharedHandle.m_uiProcessId, 0);
        if (processFd == -1)
        {
          WLog::Error("SYS_pidfd_open failed with errno: {}", WArgErrno(errno));
          m_hSharedHandle.m_hSharedTexture = 0;
          m_hSharedHandle.m_hSemaphore = 0;
          return W_FAILURE;
        }
        W_SCOPE_EXIT(close(processFd));

        m_hSharedHandle.m_hSharedTexture = syscall(SYS_pidfd_getfd, processFd, m_hSharedHandle.m_hSharedTexture, 0);
        if (m_hSharedHandle.m_hSharedTexture == -1)
        {
          WLog::Error("SYS_pidfd_getfd for texture failed with errno: {}", WArgErrno(errno));
          m_hSharedHandle.m_hSharedTexture = 0;
          m_hSharedHandle.m_hSemaphore = 0;
          return W_FAILURE;
        }

        m_hSharedHandle.m_hSemaphore = syscall(SYS_pidfd_getfd, processFd, m_hSharedHandle.m_hSemaphore, 0);
        if (m_hSharedHandle.m_hSemaphore == -1)
        {
          WLog::Error("SYS_pidfd_getfd for semaphore failed with errno: {}", WArgErrno(errno));
          m_hSharedHandle.m_hSemaphore = 0;
          return W_FAILURE;
        }
      }

      vk::Device device = m_pDevice->GetVulkanDevice();

      // Import semaphore
      vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0};
      vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
      m_SharedSemaphore = device.createSemaphore(semCreateInfo);

      vk::ImportSemaphoreFdInfoKHR importSemaphoreInfo{m_SharedSemaphore, {}, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd, static_cast<int>(m_hSharedHandle.m_hSemaphore)};
      VK_SUCCEED_OR_RETURN_W_FAILURE(device.importSemaphoreFdKHR(&importSemaphoreInfo, m_pDevice->GetDispatchContext()));
      // Spec: "Importing a semaphore payload from a file descriptor transfers ownership of the file descriptor from the application to the Vulkan implementation. The application must not perform any operations on the file descriptor after a successful import."
      m_hSharedHandle.m_hSemaphore = 0;

      // Create Image
      VK_SUCCEED_OR_RETURN_W_FAILURE(device.createImage(&createInfo, nullptr, &m_Image));

      vk::ImageMemoryRequirementsInfo2 imageRequirementsInfo{m_Image};
      vk::MemoryRequirements2 imageMemoryRequirements;
      device.getImageMemoryRequirements2(&imageRequirementsInfo, &imageMemoryRequirements);

      // Import memory
      W_ASSERT_DEBUG(imageMemoryRequirements.memoryRequirements.size == m_hSharedHandle.m_uiSize, "");

      vk::ImportMemoryFdInfoKHR fdInfo{vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd, static_cast<int>(m_hSharedHandle.m_hSharedTexture)};
      vk::MemoryAllocateInfo allocateInfo{imageMemoryRequirements.memoryRequirements.size, m_hSharedHandle.m_uiMemoryTypeIndex, &fdInfo};

      m_AllocInfo = {};
      VK_SUCCEED_OR_RETURN_W_FAILURE(device.allocateMemory(&allocateInfo, nullptr, &m_AllocInfo.m_deviceMemory));
      m_AllocInfo.m_offset = 0;
      m_AllocInfo.m_size = imageMemoryRequirements.memoryRequirements.size;
      m_AllocInfo.m_memoryType = m_hSharedHandle.m_uiMemoryTypeIndex;
      // Spec: "Importing memory from a file descriptor transfers ownership of the file descriptor from the application to the Vulkan implementation. The application must not perform any operations on the file descriptor after a successful import. "
      m_hSharedHandle.m_hSharedTexture = 0;

      device.bindImageMemory(m_Image, m_AllocInfo.m_deviceMemory, 0);

#elif W_ENABLED(W_PLATFORM_WINDOWS)
      if (m_hSharedHandle.m_hSharedTexture == 0 || m_hSharedHandle.m_hSemaphore == 0)
      {
        WLog::Error("Can not open shared texture: invalid handle given");
        return W_FAILURE;
      }


      if (!m_pDevice->GetExtensions().m_bExternalMemoryWin32)
      {
        WLog::Error("Can not open shared texture: external memory win32 not supported");
        return W_FAILURE;
      }

      if (!m_pDevice->GetExtensions().m_bExternalSemaphoreWin32)
      {
        WLog::Error("Can not open shared texture: external semaphore win32 not supported");
        return W_FAILURE;
      }

      bool bNeedToImportForeignProcessFileDescriptors = m_hSharedHandle.m_uiProcessId != WProcess::GetCurrentProcessID();
      if (bNeedToImportForeignProcessFileDescriptors)
      {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, m_hSharedHandle.m_uiProcessId);
        if (hProcess == 0)
        {
          WLog::Error("OpenProcess failed with error: {}", WArgErrorCode(GetLastError()));
          m_hSharedHandle.m_hSharedTexture = 0;
          m_hSharedHandle.m_hSemaphore = 0;
          return W_FAILURE;
        }

        HANDLE duplicateA = 0;
        BOOL res = DuplicateHandle(hProcess, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSharedTexture), GetCurrentProcess(), &duplicateA, 0, FALSE, DUPLICATE_SAME_ACCESS);
        m_hSharedHandle.m_hSharedTexture = reinterpret_cast<WUInt64>(duplicateA);
        if (res == FALSE)
        {
          WLog::Error("DuplicateHandle failed with error: {}", WArgErrorCode(GetLastError()));
          m_hSharedHandle.m_hSharedTexture = 0;
          m_hSharedHandle.m_hSemaphore = 0;
          return W_FAILURE;
        }

        HANDLE duplicateB = 0;
        res = DuplicateHandle(hProcess, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSemaphore), GetCurrentProcess(), &duplicateB, 0, FALSE, DUPLICATE_SAME_ACCESS);
        m_hSharedHandle.m_hSemaphore = reinterpret_cast<WUInt64>(duplicateB);
        if (res == FALSE)
        {
          WLog::Error("DuplicateHandle failed with error: {}", WArgErrorCode(GetLastError()));
          m_hSharedHandle.m_hSemaphore = 0;
          return W_FAILURE;
        }
      }

      vk::Device device = m_pDevice->GetVulkanDevice();

      // Import semaphore
      vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0};
      vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
      m_SharedSemaphore = device.createSemaphore(semCreateInfo);

      vk::ImportSemaphoreWin32HandleInfoKHR importSemaphoreInfo{m_SharedSemaphore, {}, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSemaphore)};
      vk::Result res = device.importSemaphoreWin32HandleKHR(&importSemaphoreInfo, m_pDevice->GetDispatchContext());
      VK_SUCCEED_OR_RETURN_W_FAILURE(res);

      // Create Image
      VK_SUCCEED_OR_RETURN_W_FAILURE(device.createImage(&createInfo, nullptr, &m_Image));

      vk::ImageMemoryRequirementsInfo2 imageRequirementsInfo{m_Image};
      vk::MemoryRequirements2 imageMemoryRequirements;
      device.getImageMemoryRequirements2(&imageRequirementsInfo, &imageMemoryRequirements);

      // Import memory
      W_ASSERT_DEBUG(imageMemoryRequirements.memoryRequirements.size == m_hSharedHandle.m_uiSize, "");

      vk::ImportMemoryWin32HandleInfoKHR fdInfo{vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSharedTexture)};
      vk::MemoryAllocateInfo allocateInfo{imageMemoryRequirements.memoryRequirements.size, m_hSharedHandle.m_uiMemoryTypeIndex, &fdInfo};

      // vk::MemoryWin32HandlePropertiesKHR handleProperties;
      //  device.getMemoryWin32HandlePropertiesKHR(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSharedTexture), m_pDevice->GetDispatchContext());

      m_AllocInfo = {};
      VK_SUCCEED_OR_RETURN_W_FAILURE(device.allocateMemory(&allocateInfo, nullptr, &m_AllocInfo.m_deviceMemory));
      m_AllocInfo.m_offset = 0;
      m_AllocInfo.m_size = imageMemoryRequirements.memoryRequirements.size;
      m_AllocInfo.m_memoryType = m_hSharedHandle.m_uiMemoryTypeIndex;

      device.bindImageMemory(m_Image, m_AllocInfo.m_deviceMemory, 0);
#else
      W_ASSERT_NOT_IMPLEMENTED
#endif
    }
  }
  else
  {
    m_Image = static_cast<VkImage>(m_Description.m_pExisitingNativeObject);
  }
  m_pDevice->GetInitContext().InitTexture(this, createInfo, pInitialData);

  return W_SUCCESS;
}


WResult WGALSharedTextureVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  WGALDeviceVulkan* pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);

  if (m_SharedType == WGALSharedTextureType::Imported)
  {
    pVulkanDevice->DeleteLater(m_Image, m_AllocInfo.m_deviceMemory);
    pVulkanDevice->DeleteLater(m_SharedSemaphore);
  }
  else if (m_SharedType == WGALSharedTextureType::Exported)
  {
    pVulkanDevice->DeleteLater(m_Image, m_pAlloc);
    pVulkanDevice->DeleteLater(m_SharedSemaphore);
  }

  auto res = SUPER::DeInitPlatform(pDevice);

#if W_ENABLED(W_PLATFORM_LINUX) && defined(SYS_pidfd_getfd)
  // These are only needed if init failed before importing the semaphore, which would have transferred ownership.
  if (m_hSharedHandle.m_hSharedTexture != 0)
  {
    pVulkanDevice->DeleteLaterImpl({vk::ObjectType::eUnknown, {WGALDeviceVulkan::PendingDeletionFlags::IsFileDescriptor}, (void*)static_cast<size_t>(m_hSharedHandle.m_hSharedTexture), nullptr});
    m_hSharedHandle.m_hSharedTexture = 0;
  }
  if (m_hSharedHandle.m_hSemaphore != 0)
  {

    pVulkanDevice->DeleteLaterImpl({vk::ObjectType::eUnknown, {WGALDeviceVulkan::PendingDeletionFlags::IsFileDescriptor}, (void*)static_cast<size_t>(m_hSharedHandle.m_hSemaphore), nullptr});
    m_hSharedHandle.m_hSemaphore = 0;
  }
#endif
  return res;
}

WGALPlatformSharedHandle WGALSharedTextureVulkan::GetSharedHandle() const
{
  return m_hSharedHandle;
}

void WGALSharedTextureVulkan::WaitSemaphoreGPU(WUInt64 uiValue) const
{
  m_pDevice->AddWaitSemaphore(WGALDeviceVulkan::SemaphoreInfo::MakeWaitSemaphore(m_SharedSemaphore, vk::PipelineStageFlagBits::eAllCommands, vk::SemaphoreType::eTimeline, uiValue));
}

void WGALSharedTextureVulkan::SignalSemaphoreGPU(WUInt64 uiValue) const
{
  m_pDevice->AddSignalSemaphore(WGALDeviceVulkan::SemaphoreInfo::MakeSignalSemaphore(m_SharedSemaphore, vk::SemaphoreType::eTimeline, uiValue));
}
