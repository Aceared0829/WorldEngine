#include <RendererVulkan/RendererVulkanPCH.h>

W_STATICLINK_LIBRARY(RendererVulkan)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(RendererVulkan_Device_Implementation_DeviceVulkan);
}
