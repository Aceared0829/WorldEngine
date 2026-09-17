#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <OpenXRPlugin/Graphics/OpenXRGraphicsBindingD3D11.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Logging/Log.h>
#  include <OpenXRPlugin/OpenXRDeclarations.h>
#  include <RendererCore/Textures/TextureUtils.h>
#  include <RendererDX11/Device/DeviceDX11.h>
#  include <RendererFoundation/Device/Device.h>
#  include <Texture/Image/Formats/ImageFormatMappings.h>

WOpenXRGraphicsBindingD3D11::WOpenXRGraphicsBindingD3D11() = default;

WOpenXRGraphicsBindingD3D11::~WOpenXRGraphicsBindingD3D11()
{
  Deinitialize();
}

XrResult WOpenXRGraphicsBindingD3D11::SelectExtension(WDynamicArray<const char*>& extensions, const WDynamicArray<XrExtensionProperties>& extensionProperties)
{
  bool bFound = false;
  for (const XrExtensionProperties& prop : extensionProperties)
  {
    if (WStringUtils::IsEqual(prop.extensionName, XR_KHR_D3D11_ENABLE_EXTENSION_NAME))
    {
      bFound = true;
      break;
    }
  }

  if (bFound)
  {
    extensions.PushBack(XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
    WLog::Info("OpenXR: Enabled {} extension", XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
    return XR_SUCCESS;
  }

  WLog::Error("OpenXR: {} extension not supported", XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
  return XR_ERROR_EXTENSION_NOT_PRESENT;
}

void WOpenXRGraphicsBindingD3D11::LoadFunctionPointers(XrInstance instance)
{
  xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetD3D11GraphicsRequirementsKHR));
}

XrResult WOpenXRGraphicsBindingD3D11::Initialize(XrInstance instance, XrSystemId systemId, WGALDevice* pDevice)
{
  W_ASSERT_DEV(m_GraphicsBinding.device == nullptr, "D3D11 graphics binding already initialized");

  if (!m_pfnGetD3D11GraphicsRequirementsKHR)
  {
    WLog::Error("OpenXR: xrGetD3D11GraphicsRequirementsKHR function pointer not loaded");
    return XR_ERROR_FUNCTION_UNSUPPORTED;
  }

  // Query graphics requirements
  XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
  XrResult result = m_pfnGetD3D11GraphicsRequirementsKHR(instance, systemId, &graphicsRequirements);
  if (result != XR_SUCCESS)
  {
    WLog::Error("OpenXR: xrGetD3D11GraphicsRequirementsKHR failed: {}", (int)result);
    return result;
  }

  // Get the D3D11 device
  WGALDeviceDX11* pD3dDevice = static_cast<WGALDeviceDX11*>(pDevice);
  m_GraphicsBinding.device = pD3dDevice->GetDXDevice();

  WLog::Info("OpenXR D3D11 binding initialized");
  return XR_SUCCESS;
}

void WOpenXRGraphicsBindingD3D11::Deinitialize()
{
  m_GraphicsBinding.device = nullptr;
  CleanupSwapchainImages();
}

const void* WOpenXRGraphicsBindingD3D11::GetGraphicsBinding() const
{
  return &m_GraphicsBinding;
}

XrResult WOpenXRGraphicsBindingD3D11::SelectSwapchainFormats(XrSession session, bool bDepthComposition, int64_t& out_colorFormat, int64_t& out_depthFormat)
{
  // Enumerate available formats
  uint32_t formatCount;
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr));
  WDynamicArray<int64_t> swapchainFormats;
  swapchainFormats.SetCountUninitialized(formatCount);
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateSwapchainFormats(session, formatCount, &formatCount, swapchainFormats.GetData()));

  // Build a set for fast lookup
  WSet<int64_t> availableFormats;
  for (int64_t format : swapchainFormats)
  {
    availableFormats.Insert(format);
  }

  // Supported color formats in preference order
  constexpr DXGI_FORMAT SupportedColorFormats[] = {
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
  };

  // Find matching color format
  out_colorFormat = 0;
  for (DXGI_FORMAT colorFormat : SupportedColorFormats)
  {
    if (availableFormats.Contains(static_cast<int64_t>(colorFormat)))
    {
      out_colorFormat = static_cast<int64_t>(colorFormat);
      break;
    }
  }

  if (out_colorFormat == 0)
  {
    WLog::Error("OpenXR D3D11: No supported color swapchain format found");
    return XR_ERROR_INITIALIZATION_FAILED;
  }

  // Select depth format if needed
  if (bDepthComposition)
  {
    constexpr DXGI_FORMAT SupportedDepthFormats[] = {
      DXGI_FORMAT_D32_FLOAT,
      DXGI_FORMAT_D16_UNORM,
      DXGI_FORMAT_D24_UNORM_S8_UINT,
    };

    out_depthFormat = 0;
    for (DXGI_FORMAT depthFormat : SupportedDepthFormats)
    {
      if (availableFormats.Contains(static_cast<int64_t>(depthFormat)))
      {
        out_depthFormat = static_cast<int64_t>(depthFormat);
        break;
      }
    }

    if (out_depthFormat == 0)
    {
      WLog::Error("OpenXR D3D11: No supported depth swapchain format found");
      return XR_ERROR_INITIALIZATION_FAILED;
    }
  }

  return XR_SUCCESS;
}

XrResult WOpenXRGraphicsBindingD3D11::CreateSwapchainImages(XrSwapchain swapchainHandle, int64_t format, WUInt32 imageCount, WSizeU32 size, WGALMSAASampleCount::Enum msaaCount, bool bIsDepth, WGALDevice* pDevice, WDynamicArray<WGALTextureHandle>& out_textures)
{
  // Select the appropriate image storage
  auto& imageStorage = bIsDepth ? m_DepthSwapchainImages : m_ColorSwapchainImages;
  imageStorage.SetCount(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});

  // Enumerate swapchain images
  XrResult result = xrEnumerateSwapchainImages(swapchainHandle, imageCount, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(imageStorage.GetData()));

  if (result != XR_SUCCESS)
  {
    WLog::Error("OpenXR D3D11: xrEnumerateSwapchainImages failed: {}", (int)result);
    return result;
  }

  // Create texture handles for each swapchain image
  for (WUInt32 i = 0; i < imageCount; i++)
  {
    ID3D11Texture2D* pTex = imageStorage[i].texture;

    D3D11_TEXTURE2D_DESC backBufferDesc;
    pTex->GetDesc(&backBufferDesc);

    WGALTextureCreationDescription textureDesc;
    textureDesc.SetAsRenderTarget(backBufferDesc.Width, backBufferDesc.Height, ConvertTextureFormat(format), WGALMSAASampleCount::Enum(backBufferDesc.SampleDesc.Count));
    textureDesc.m_uiArraySize = backBufferDesc.ArraySize;
    textureDesc.m_pExisitingNativeObject = pTex;
    textureDesc.m_Type = WGALTextureType::Texture2DArray;

    // Need to add a ref as the W texture will always remove one on destruction
    pTex->AddRef();

    out_textures.PushBack(pDevice->CreateTexture(textureDesc));
  }

  return XR_SUCCESS;
}

WGALResourceFormat::Enum WOpenXRGraphicsBindingD3D11::ConvertTextureFormat(int64_t format) const
{
  switch (format)
  {
    case DXGI_FORMAT_D32_FLOAT:
      return WGALResourceFormat::DFloat;
    case DXGI_FORMAT_D16_UNORM:
      return WGALResourceFormat::D16;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      return WGALResourceFormat::D24S8;
    default:
      WImageFormat::Enum imageFormat = WImageFormatMappings::FromDxgiFormat(static_cast<WUInt32>(format));
      WGALResourceFormat::Enum galFormat = WTextureUtils::ImageFormatToGalFormat(imageFormat, false);
      return galFormat;
  }
}

void WOpenXRGraphicsBindingD3D11::CleanupSwapchainImages()
{
  m_ColorSwapchainImages.Clear();
  m_DepthSwapchainImages.Clear();
}

#endif // W_ENABLED(W_PLATFORM_WINDOWS)
