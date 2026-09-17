#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>
#include <RendererVulkan/State/StateVulkan.h>

// Mapping tables to map WGAL constants to Vulkan constants
#include <RendererVulkan/State/Implementation/StateVulkan_MappingTables.inl>

// Blend state

WGALBlendStateVulkan::WGALBlendStateVulkan(const WGALBlendStateCreationDescription& Description)
  : WGALBlendState(Description)
{
  m_BlendState.pAttachments = m_blendAttachmentState;
}

WGALBlendStateVulkan::~WGALBlendStateVulkan() = default;

static vk::BlendOp ToVulkanBlendOp(WGALBlendOp::Enum e)
{
  switch (e)
  {
    case WGALBlendOp::Add:
      return vk::BlendOp::eAdd;
    case WGALBlendOp::Max:
      return vk::BlendOp::eMax;
    case WGALBlendOp::Min:
      return vk::BlendOp::eMin;
    case WGALBlendOp::RevSubtract:
      return vk::BlendOp::eReverseSubtract;
    case WGALBlendOp::Subtract:
      return vk::BlendOp::eSubtract;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  return vk::BlendOp::eAdd;
}

static vk::BlendFactor ToVulkanBlendFactor(WGALBlend::Enum e)
{
  switch (e)
  {
    case WGALBlend::BlendFactor:
      W_ASSERT_NOT_IMPLEMENTED;
      return vk::BlendFactor::eZero;
    case WGALBlend::DestAlpha:
      return vk::BlendFactor::eDstAlpha;
    case WGALBlend::DestColor:
      return vk::BlendFactor::eDstColor;
    case WGALBlend::InvBlendFactor:
      W_ASSERT_NOT_IMPLEMENTED;
      return vk::BlendFactor::eZero;
    case WGALBlend::InvDestAlpha:
      return vk::BlendFactor::eOneMinusDstAlpha;
    case WGALBlend::InvDestColor:
      return vk::BlendFactor::eOneMinusDstColor;
    case WGALBlend::InvSrcAlpha:
      return vk::BlendFactor::eOneMinusSrcAlpha;
    case WGALBlend::InvSrcColor:
      return vk::BlendFactor::eOneMinusSrcColor;
    case WGALBlend::One:
      return vk::BlendFactor::eOne;
    case WGALBlend::SrcAlpha:
      return vk::BlendFactor::eSrcAlpha;
    case WGALBlend::SrcAlphaSaturated:
      return vk::BlendFactor::eSrcAlphaSaturate;
    case WGALBlend::SrcColor:
      return vk::BlendFactor::eSrcColor;
    case WGALBlend::Zero:
      return vk::BlendFactor::eZero;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  return vk::BlendFactor::eOne;
}

WResult WGALBlendStateVulkan::InitPlatform(WGALDevice* pDevice)
{
  // TODO attachment count has to be set when render targets are known
  // TODO alpha2coverage needs to be implemented in MultisampleStateCreateInfo
  // TODO independent blend is a device feature that is always enabled if present

  for (WInt32 i = 0; i < 8; ++i)
  {
    m_blendAttachmentState[i].blendEnable = m_Description.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled ? VK_TRUE : VK_FALSE;
    m_blendAttachmentState[i].colorBlendOp = ToVulkanBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOp);
    m_blendAttachmentState[i].alphaBlendOp = ToVulkanBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha);
    m_blendAttachmentState[i].dstColorBlendFactor = ToVulkanBlendFactor(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlend);
    m_blendAttachmentState[i].dstAlphaBlendFactor = ToVulkanBlendFactor(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha);
    m_blendAttachmentState[i].srcColorBlendFactor = ToVulkanBlendFactor(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlend);
    m_blendAttachmentState[i].srcAlphaBlendFactor = ToVulkanBlendFactor(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha);
    m_blendAttachmentState[i].colorWriteMask = (vk::ColorComponentFlags)(m_Description.m_RenderTargetBlendDescriptions[i].m_uiWriteMask & 0x0F);
  }

  return W_SUCCESS;
}

WResult WGALBlendStateVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  return W_SUCCESS;
}

// Depth Stencil state

WGALDepthStencilStateVulkan::WGALDepthStencilStateVulkan(const WGALDepthStencilStateCreationDescription& Description)
  : WGALDepthStencilState(Description)
{
}

WGALDepthStencilStateVulkan::~WGALDepthStencilStateVulkan() = default;

WResult WGALDepthStencilStateVulkan::InitPlatform(WGALDevice* pDevice)
{
  m_DepthStencilState.depthBoundsTestEnable = VK_FALSE;
  m_DepthStencilState.depthCompareOp = GALCompareFuncToVulkan[m_Description.m_DepthTestFunc];
  m_DepthStencilState.depthTestEnable = m_Description.m_bDepthEnable ? VK_TRUE : VK_FALSE;
  m_DepthStencilState.depthWriteEnable = m_Description.m_bDepthWrite ? VK_TRUE : VK_FALSE;
  m_DepthStencilState.minDepthBounds = 0.f;
  m_DepthStencilState.maxDepthBounds = 1.f;

  m_DepthStencilState.stencilTestEnable = m_Description.m_bStencilEnable ? VK_TRUE : VK_FALSE;
  m_DepthStencilState.front.compareMask = m_Description.m_uiStencilReadMask;
  m_DepthStencilState.front.writeMask = m_Description.m_uiStencilWriteMask;
  m_DepthStencilState.front.compareOp = GALCompareFuncToVulkan[m_Description.m_FrontFaceStencilOp.m_StencilFunc];
  m_DepthStencilState.front.depthFailOp = GALStencilOpTableIndexToVulkan[m_Description.m_FrontFaceStencilOp.m_DepthFailOp];
  m_DepthStencilState.front.failOp = GALStencilOpTableIndexToVulkan[m_Description.m_FrontFaceStencilOp.m_FailOp];
  m_DepthStencilState.front.passOp = GALStencilOpTableIndexToVulkan[m_Description.m_FrontFaceStencilOp.m_PassOp];

  const WGALStencilOpDescription& backFaceStencilOp = m_Description.m_BackFaceStencilOp;
  m_DepthStencilState.back.compareMask = m_Description.m_uiStencilReadMask;
  m_DepthStencilState.back.writeMask = m_Description.m_uiStencilWriteMask;
  m_DepthStencilState.back.compareOp = GALCompareFuncToVulkan[backFaceStencilOp.m_StencilFunc];
  m_DepthStencilState.back.depthFailOp = GALStencilOpTableIndexToVulkan[backFaceStencilOp.m_DepthFailOp];
  m_DepthStencilState.back.failOp = GALStencilOpTableIndexToVulkan[backFaceStencilOp.m_FailOp];
  m_DepthStencilState.back.passOp = GALStencilOpTableIndexToVulkan[backFaceStencilOp.m_PassOp];

  return W_SUCCESS;
}

WResult WGALDepthStencilStateVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  return W_SUCCESS;
}


// Rasterizer state

WGALRasterizerStateVulkan::WGALRasterizerStateVulkan(const WGALRasterizerStateCreationDescription& Description)
  : WGALRasterizerState(Description)
{
}

WGALRasterizerStateVulkan::~WGALRasterizerStateVulkan() = default;



WResult WGALRasterizerStateVulkan::InitPlatform(WGALDevice* pDevice)
{
  // TODO scissor test is always enabled for vulkan

  auto pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);

  if (m_Description.m_bConservativeRasterization)
  {
    if (!pVulkanDevice->GetCapabilities().m_bSupportsConservativeRasterization)
    {
      WLog::Error("Rasterizer state description enables conservative rasterization which is not available!");
      return W_FAILURE;
    }

    // Overestimation is what D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON does. Any overestimation beyond the hardware minimum is not requested.
    m_ConservativeRasterState.conservativeRasterizationMode = vk::ConservativeRasterizationModeEXT::eOverestimate;
    m_ConservativeRasterState.extraPrimitiveOverestimationSize = 0.0f;
    m_RasterizerState.pNext = &m_ConservativeRasterState;
  }

  m_RasterizerState.cullMode = GALCullModeToVulkan[m_Description.m_CullMode];
  m_RasterizerState.depthBiasEnable = m_Description.m_iDepthBias != 0 || m_Description.m_fSlopeScaledDepthBias != 0.0f || m_Description.m_fDepthBiasClamp != 0.0f;
  m_RasterizerState.depthBiasConstantFactor = static_cast<float>(m_Description.m_iDepthBias);
  m_RasterizerState.depthBiasSlopeFactor = m_Description.m_fSlopeScaledDepthBias;
  // A non-zero bias clamp requires a device feature, without it the value must stay at zero.
  m_RasterizerState.depthBiasClamp = pVulkanDevice->GetCapabilities().m_bSupportsDepthBiasClamp ? m_Description.m_fDepthBiasClamp : 0.0f;

  // Matches DX11, which always sets DepthClipEnable. Vulkan's depthClampEnable is the inverse of depth clipping and unrelated to the depth bias clamp above.
  m_RasterizerState.depthClampEnable = VK_FALSE;
  m_RasterizerState.frontFace = m_Description.m_bFrontCounterClockwise ? vk::FrontFace::eCounterClockwise : vk::FrontFace::eClockwise;
  m_RasterizerState.lineWidth = 1.f;
  m_RasterizerState.polygonMode = m_Description.m_bWireFrame ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;

  return W_SUCCESS;
}


WResult WGALRasterizerStateVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  return W_SUCCESS;
}

// Sampler state

WGALSamplerStateVulkan::WGALSamplerStateVulkan(const WGALSamplerStateCreationDescription& Description)
  : WGALSamplerState(Description)
{
}

WGALSamplerStateVulkan::~WGALSamplerStateVulkan() = default;

WResult WGALSamplerStateVulkan::InitPlatform(WGALDevice* pDevice)
{
  WGALSamplerStateCreationDescription desc = this->GetDescription();
  pDevice->AdjustSamplerStateDescription(desc);

  auto pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);

  vk::SamplerCreateInfo samplerCreateInfo = {};
  samplerCreateInfo.addressModeU = GALTextureAddressModeToVulkan[desc.m_AddressU];
  samplerCreateInfo.addressModeV = GALTextureAddressModeToVulkan[desc.m_AddressV];
  samplerCreateInfo.addressModeW = GALTextureAddressModeToVulkan[desc.m_AddressW];
  if (desc.m_MagFilter == WGALTextureFilterMode::Anisotropic || desc.m_MinFilter == WGALTextureFilterMode::Anisotropic || desc.m_MipFilter == WGALTextureFilterMode::Anisotropic)
  {
    const float fMaxAnisotropy = pVulkanDevice->GetPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
    if (pVulkanDevice->GetPhysicalDeviceFeatures().features.samplerAnisotropy && fMaxAnisotropy > 1.0f)
    {
      samplerCreateInfo.anisotropyEnable = VK_TRUE;
      samplerCreateInfo.maxAnisotropy = WMath::Clamp(static_cast<float>(desc.m_uiMaxAnisotropy), 1.0f, fMaxAnisotropy);
    }
  }

  vk::SamplerCustomBorderColorCreateInfoEXT customBorderColor;
  if (samplerCreateInfo.addressModeU == vk::SamplerAddressMode::eClampToBorder || samplerCreateInfo.addressModeV == vk::SamplerAddressMode::eClampToBorder || samplerCreateInfo.addressModeW == vk::SamplerAddressMode::eClampToBorder)
  {
    const WColor col = desc.m_BorderColor;
    if (col == WColor(0, 0, 0, 0))
    {
      samplerCreateInfo.borderColor = vk::BorderColor::eFloatTransparentBlack;
    }
    else if (col == WColor(0, 0, 0, 1))
    {
      samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
    }
    else if (col == WColor(1, 1, 1, 1))
    {
      samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
    }
    else if (pVulkanDevice->GetExtensions().m_bBorderColorFloat)
    {
      customBorderColor.customBorderColor.setFloat32({col.r, col.g, col.b, col.a});
      samplerCreateInfo.borderColor = vk::BorderColor::eFloatCustomEXT;
      samplerCreateInfo.pNext = &customBorderColor;
    }
    else
    {
      // Fallback to close enough.
      const bool bTransparent = desc.m_BorderColor.a == 0.0f;
      const bool bBlack = desc.m_BorderColor.r == 0.0f;
      if (bBlack)
      {
        samplerCreateInfo.borderColor = bTransparent ? vk::BorderColor::eFloatTransparentBlack : vk::BorderColor::eFloatOpaqueBlack;
      }
      else
      {
        samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
      }
    }
  }
  samplerCreateInfo.compareEnable = desc.m_SampleCompareFunc == WGALCompareFunc::Never ? VK_FALSE : VK_TRUE;
  samplerCreateInfo.compareOp = GALCompareFuncToVulkan[desc.m_SampleCompareFunc];
  samplerCreateInfo.magFilter = GALFilterToVulkanFilter[desc.m_MagFilter];
  samplerCreateInfo.minFilter = GALFilterToVulkanFilter[desc.m_MinFilter];
  samplerCreateInfo.maxLod = desc.m_fMaxMip;
  samplerCreateInfo.minLod = desc.m_fMinMip;
  samplerCreateInfo.mipLodBias = desc.m_fMipLodBias;
  samplerCreateInfo.mipmapMode = GALFilterToVulkanMipmapMode[desc.m_MipFilter];

  m_ResourceImageInfo.imageLayout = vk::ImageLayout::eUndefined;
  VK_SUCCEED_OR_RETURN_W_FAILURE(pVulkanDevice->GetVulkanDevice().createSampler(&samplerCreateInfo, nullptr, &m_ResourceImageInfo.sampler));
  return W_SUCCESS;
}


WResult WGALSamplerStateVulkan::DeInitPlatform(WGALDevice* pDevice)
{
  WGALDeviceVulkan* pVulkanDevice = static_cast<WGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_ResourceImageInfo.sampler);
  return W_SUCCESS;
}
