#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/RendererFallbackResources.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, FallbackResources)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WGALRendererFallbackResources::Initialize();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WGALRendererFallbackResources::DeInitialize();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WGALDevice* WGALRendererFallbackResources::s_pDevice = nullptr;
WEventSubscriptionID WGALRendererFallbackResources::s_EventID = 0;

WHashTable<WGALRendererFallbackResources::Key, WGALTextureHandle, WGALRendererFallbackResources::KeyHash> WGALRendererFallbackResources::s_TextureResourceViews;
WHashTable<WEnum<WGALShaderResourceType>, WGALBufferHandle, WGALRendererFallbackResources::KeyHash> WGALRendererFallbackResources::s_BufferResourceViews;
WDynamicArray<WGALBufferHandle> WGALRendererFallbackResources::s_Buffers;
WDynamicArray<WGALTextureHandle> WGALRendererFallbackResources::s_Textures;

void WGALRendererFallbackResources::Initialize()
{
  s_EventID = WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&WGALRendererFallbackResources::GALDeviceEventHandler));
}

void WGALRendererFallbackResources::DeInitialize()
{
  WGALDevice::s_Events.RemoveEventHandler(s_EventID);
}
void WGALRendererFallbackResources::GALDeviceEventHandler(const WGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case WGALDeviceEvent::AfterInit:
    {
      s_pDevice = e.m_pDevice;
      auto CreateTexture = [](WGALTextureType::Enum type, WGALMSAASampleCount::Enum samples, bool bDepth) -> WGALTextureHandle
      {
        WGALTextureCreationDescription desc;
        desc.m_uiWidth = 4;
        desc.m_uiHeight = 4;
        if (type == WGALTextureType::Texture3D)
          desc.m_uiDepth = 4;
        desc.m_uiMipLevelCount = 1;
        desc.m_Format = bDepth ? WGALResourceFormat::D16 : WGALResourceFormat::BGRAUByteNormalizedsRGB;
        desc.m_Type = type;
        desc.m_SampleCount = samples;
        desc.m_ResourceAccess.m_bImmutable = false;
        WGALTextureHandle hTexture = s_pDevice->CreateTexture(desc);
        W_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackResource");
        s_Textures.PushBack(hTexture);

        return hTexture;
      };
      {
        WGALTextureHandle tex = CreateTexture(WGALTextureType::Texture2D, WGALMSAASampleCount::None, false);
        s_TextureResourceViews[{WGALShaderResourceType::Texture, WGALShaderTextureType::Texture2D, false}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::Texture, WGALShaderTextureType::Texture2DArray, false}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::TextureAndSampler, WGALShaderTextureType::Texture2D, false}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::TextureAndSampler, WGALShaderTextureType::Texture2DArray, false}] = tex;
      }
      {
        WGALTextureHandle tex = CreateTexture(WGALTextureType::Texture2D, WGALMSAASampleCount::None, true);
        s_TextureResourceViews[{WGALShaderResourceType::Texture, WGALShaderTextureType::Texture2D, true}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::Texture, WGALShaderTextureType::Texture2DArray, true}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::TextureAndSampler, WGALShaderTextureType::Texture2D, true}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::TextureAndSampler, WGALShaderTextureType::Texture2DArray, true}] = tex;
      }

      // Swift shader can only do 4x MSAA. Add a check anyways.
      const bool bSupported = s_pDevice->GetCapabilities().m_FormatSupport[WGALResourceFormat::BGRAUByteNormalizedsRGB].AreAllSet(WGALResourceFormatSupport::Texture | WGALResourceFormatSupport::MSAA4x);

      if (bSupported)
      {
        WGALTextureHandle tex = CreateTexture(WGALTextureType::Texture2D, WGALMSAASampleCount::FourSamples, false);
        s_TextureResourceViews[{WGALShaderResourceType::Texture, WGALShaderTextureType::Texture2DMS, false}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::Texture, WGALShaderTextureType::Texture2DMSArray, false}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::TextureAndSampler, WGALShaderTextureType::Texture2DMS, false}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::TextureAndSampler, WGALShaderTextureType::Texture2DMSArray, false}] = tex;
      }
      {
        WGALTextureHandle tex = CreateTexture(WGALTextureType::TextureCube, WGALMSAASampleCount::None, false);
        s_TextureResourceViews[{WGALShaderResourceType::Texture, WGALShaderTextureType::TextureCube, false}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::Texture, WGALShaderTextureType::TextureCubeArray, false}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::TextureAndSampler, WGALShaderTextureType::TextureCube, false}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::TextureAndSampler, WGALShaderTextureType::TextureCubeArray, false}] = tex;
      }
      {
        WGALTextureHandle tex = CreateTexture(WGALTextureType::Texture3D, WGALMSAASampleCount::None, false);
        s_TextureResourceViews[{WGALShaderResourceType::Texture, WGALShaderTextureType::Texture3D, false}] = tex;
        s_TextureResourceViews[{WGALShaderResourceType::TextureAndSampler, WGALShaderTextureType::Texture3D, false}] = tex;
      }
      {
        WGALBufferCreationDescription desc;
        desc.m_BufferFlags = WGALBufferUsageFlags::ConstantBuffer | WGALBufferUsageFlags::ShaderResource;
        desc.m_uiStructSize = 0;
        desc.m_uiTotalSize = 128;
        desc.m_ResourceAccess.m_bImmutable = false;
        WGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackConstantBuffer");
        s_Buffers.PushBack(hBuffer);
        s_BufferResourceViews[WGALShaderResourceType::ConstantBuffer] = hBuffer;
        s_BufferResourceViews[WGALShaderResourceType::ConstantBuffer] = hBuffer;
      }
      {
        WGALBufferCreationDescription desc;
        desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
        desc.m_uiStructSize = 128;
        desc.m_uiTotalSize = 1280;
        desc.m_ResourceAccess.m_bImmutable = false;
        WGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackStructuredBuffer");
        s_Buffers.PushBack(hBuffer);
        s_BufferResourceViews[WGALShaderResourceType::StructuredBuffer] = hBuffer;
        s_BufferResourceViews[WGALShaderResourceType::StructuredBuffer] = hBuffer;
      }
      {
        WGALBufferCreationDescription desc;
        desc.m_uiStructSize = 0;
        desc.m_uiTotalSize = 1024;
        desc.m_Format = WGALResourceFormat::RUInt;
        desc.m_BufferFlags = WGALBufferUsageFlags::TexelBuffer | WGALBufferUsageFlags::ShaderResource;
        desc.m_ResourceAccess.m_bImmutable = false;
        WGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackTexelBuffer");
        s_Buffers.PushBack(hBuffer);
        s_BufferResourceViews[WGALShaderResourceType::TexelBuffer] = hBuffer;
      }
      {
        WGALTextureCreationDescription desc;
        desc.m_uiWidth = 4;
        desc.m_uiHeight = 4;
        desc.m_uiMipLevelCount = 1;
        desc.m_Format = WGALResourceFormat::RGBAHalf;
        desc.m_Type = WGALTextureType::Texture2D;
        desc.m_SampleCount = WGALMSAASampleCount::None;
        desc.m_ResourceAccess.m_bImmutable = false;
        desc.m_TextureFlags = WGALTextureUsageFlags::ShaderResource | WGALTextureUsageFlags::UnorderedAccess;
        WGALTextureHandle hTexture = s_pDevice->CreateTexture(desc);
        W_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackTextureRW");
        s_Textures.PushBack(hTexture);

        s_TextureResourceViews[{WGALShaderResourceType::TextureRW, WGALShaderTextureType::Texture2D, false}] = hTexture;
        s_TextureResourceViews[{WGALShaderResourceType::TextureRW, WGALShaderTextureType::Texture2DArray, false}] = hTexture;
      }
      {
        WGALBufferCreationDescription desc;
        desc.m_uiStructSize = 0;
        desc.m_uiTotalSize = 1024;
        desc.m_Format = WGALResourceFormat::RUInt;
        desc.m_BufferFlags = WGALBufferUsageFlags::TexelBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
        desc.m_ResourceAccess.m_bImmutable = false;
        WGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackTexelBufferRW");
        s_Buffers.PushBack(hBuffer);
        s_BufferResourceViews[WGALShaderResourceType::TexelBufferRW] = hBuffer;
      }
      {
        WGALBufferCreationDescription desc;
        desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
        desc.m_uiStructSize = 128;
        desc.m_uiTotalSize = 1280;
        desc.m_ResourceAccess.m_bImmutable = false;
        WGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackStructuredBufferRW");
        s_Buffers.PushBack(hBuffer);
        s_BufferResourceViews[WGALShaderResourceType::StructuredBufferRW] = hBuffer;
      }
    }
    break;
    case WGALDeviceEvent::BeforeShutdown:
    {
      s_TextureResourceViews.Clear();
      s_TextureResourceViews.Compact();
      s_BufferResourceViews.Clear();
      s_BufferResourceViews.Compact();

      for (WGALBufferHandle hBuffer : s_Buffers)
      {
        s_pDevice->DestroyBuffer(hBuffer);
      }
      s_Buffers.Clear();
      s_Buffers.Compact();

      for (WGALTextureHandle hTexture : s_Textures)
      {
        s_pDevice->DestroyTexture(hTexture);
      }
      s_Textures.Clear();
      s_Textures.Compact();
      s_pDevice = nullptr;
    }
    break;
    default:
      break;
  }
}

const WGALBufferHandle WGALRendererFallbackResources::GetFallbackBuffer(WEnum<WGALShaderResourceType> resourceType)
{
  if (WGALBufferHandle* pView = s_BufferResourceViews.GetValue(resourceType))
  {
    return *pView;
  }
  W_REPORT_FAILURE("No fallback resource set, update WGALRendererFallbackResources::GALDeviceEventHandler.");
  return {};
}

const WGALTextureHandle WGALRendererFallbackResources::GetFallbackTexture(WEnum<WGALShaderResourceType> resourceType, WEnum<WGALShaderTextureType> textureType, bool bDepth)
{
  if (WGALTextureHandle* pView = s_TextureResourceViews.GetValue(Key{resourceType, textureType, bDepth}))
  {
    return *pView;
  }
  W_REPORT_FAILURE("No fallback resource set, update WGALRendererFallbackResources::GALDeviceEventHandler.");
  return {};
}

WUInt32 WGALRendererFallbackResources::KeyHash::Hash(const Key& a)
{
  WHashStreamWriter32 writer;
  writer << a.m_ResourceType.GetValue();
  writer << a.m_WType.GetValue();
  writer << a.m_bDepth;
  return writer.GetHashValue();
}

bool WGALRendererFallbackResources::KeyHash::Equal(const Key& a, const Key& b)
{
  return a.m_ResourceType == b.m_ResourceType && a.m_WType == b.m_WType && a.m_bDepth == b.m_bDepth;
}

WUInt32 WGALRendererFallbackResources::KeyHash::Hash(const WEnum<WGALShaderResourceType>& a)
{
  WHashStreamWriter32 writer;
  writer << a.GetValue();
  return writer.GetHashValue();
}

bool WGALRendererFallbackResources::KeyHash::Equal(const WEnum<WGALShaderResourceType>& a, const WEnum<WGALShaderResourceType>& b)
{
  return a == b;
}


W_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_RendererFallbackResources);
