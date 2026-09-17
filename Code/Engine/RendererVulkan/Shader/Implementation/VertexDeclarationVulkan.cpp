#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

WGALVertexDeclarationVulkan::WGALVertexDeclarationVulkan(const WGALVertexDeclarationCreationDescription& Description)
  : WGALVertexDeclaration(Description)
{
}

WGALVertexDeclarationVulkan::~WGALVertexDeclarationVulkan() = default;

WResult WGALVertexDeclarationVulkan::InitPlatform(WGALDevice* pDevice)
{
  WGALDeviceVulkan* pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);

  const WGALShaderVulkan* pShader = static_cast<const WGALShaderVulkan*>(pDevice->GetShader(m_Description.m_hShader));

  if (pShader == nullptr || !pShader->GetDescription().HasByteCodeForStage(WGALShaderStage::VertexShader))
  {
    return W_FAILURE;
  }

  WHybridArray<WShaderVertexInputAttribute, 8> vias(pShader->GetVertexInputAttributes());
  auto FindLocation = [&](WGALVertexAttributeSemantic::Enum sematic, WGALResourceFormat::Enum format) -> WUInt32
  {
    for (WUInt32 i = 0; i < vias.GetCount(); i++)
    {
      if (vias[i].m_eSemantic == sematic)
      {
        // W_ASSERT_DEBUG(vias[i].m_eFormat == format, "Found matching sematic {} but format differs: {} : {}", sematic, format, vias[i].m_eFormat);
        WUInt32 uiLocation = vias[i].m_uiLocation;
        vias.RemoveAtAndSwap(i);
        return uiLocation;
      }
    }
    return WMath::MaxValue<WUInt32>();
  };

  // Copy attribute descriptions
  WUInt32 usedBindings = 0;
  for (WUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); i++)
  {
    const WGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];

    const WUInt32 uiLocation = FindLocation(Current.m_eSemantic, Current.m_eFormat);
    if (uiLocation == WMath::MaxValue<WUInt32>())
    {
      // WLog::Warning("Vertex buffer semantic {} not used by shader", Current.m_eSemantic);
      continue;
    }
    vk::VertexInputAttributeDescription& attrib = m_Attributes.ExpandAndGetRef();
    attrib.binding = Current.m_uiVertexBufferSlot;
    attrib.location = uiLocation;
    attrib.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_format;
    attrib.offset = Current.m_uiOffset;

    if (attrib.format == vk::Format::eUndefined)
    {
      WLog::Error("Vertex attribute format {0} of attribute at index {1} is undefined!", Current.m_eFormat, i);
      return W_FAILURE;
    }

    usedBindings |= W_BIT(Current.m_uiVertexBufferSlot);
  }

  const WUInt32 uiBindings = m_Description.m_VertexBindings.GetCount();
  m_Bindings.SetCount(uiBindings);
  for (WUInt32 uiBinding = 0; uiBinding < uiBindings; ++uiBinding)
  {
    const WGALVertexBinding& binding = m_Description.m_VertexBindings[uiBinding];
    vk::VertexInputBindingDescription& vkBinding = m_Bindings[uiBinding];
    vkBinding.binding = uiBinding;
    vkBinding.stride = binding.m_uiStride;
    vkBinding.inputRate = WConversionUtilsVulkan::GetVertexBindingRate(binding.m_Rate);
  }

  // Remove unused vertex bindings.
  for (WInt32 i = (WInt32)m_Bindings.GetCount() - 1; i >= 0; --i)
  {
    if ((usedBindings & W_BIT(i)) == 0)
    {
      m_Bindings.RemoveAtAndCopy(i);
    }
  }

  m_CreateInfo.pVertexAttributeDescriptions = m_Attributes.GetData();
  m_CreateInfo.vertexAttributeDescriptionCount = m_Attributes.GetCount();
  m_CreateInfo.pVertexBindingDescriptions = m_Bindings.GetData();
  m_CreateInfo.vertexBindingDescriptionCount = m_Bindings.GetCount();

  if (!vias.IsEmpty())
  {
    WLog::Error("Vertex buffers do not cover all vertex attributes defined in the shader!");
    return W_FAILURE;
  }
  return W_SUCCESS;
}

WResult WGALVertexDeclarationVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  return W_SUCCESS;
}
