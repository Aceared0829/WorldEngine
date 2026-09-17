#include <RendererTest/RendererTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/RendererReflection.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererTest/Basics/Readback.h>

WResult WRendererTestReadback::InitializeTest()
{
  WStartup::StartupCoreSystems();

  if (SetupRenderer().Failed())
    return W_FAILURE;

  W_SUCCEED_OR_RETURN(CreateWindow(320, 240));

  m_hUVColorShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/ReadbackFloat.WShader");
  m_hUVColorIntShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/ReadbackInt.WShader");
  m_hUVColorUIntShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/ReadbackUInt.WShader");
  m_hUVColorDepthShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/ReadbackDepth.WShader");

  m_hTexture2DShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Texture2D.WShader");
  m_hTexture2DDepthShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Texture2DReadbackDepth.WShader");
  m_hTexture2DIntShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Texture2DReadbackInt.WShader");
  m_hTexture2DUIntShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Texture2DReadbackUInt.WShader");

  return W_SUCCESS;
}

WResult WRendererTestReadback::DeInitializeTest()
{
  m_hShader.Invalidate();
  m_hUVColorShader.Invalidate();
  m_hUVColorIntShader.Invalidate();
  m_hUVColorUIntShader.Invalidate();
  m_hUVColorDepthShader.Invalidate();
  m_hTexture2DShader.Invalidate();
  m_hTexture2DDepthShader.Invalidate();
  m_hTexture2DIntShader.Invalidate();
  m_hTexture2DUIntShader.Invalidate();

  DestroyWindow();
  ShutdownRenderer();
  WStartup::ShutdownCoreSystems();
  WMemoryTracker::DumpMemoryLeaks();

  return W_SUCCESS;
}

void WRendererTestReadback::SetupSubTests()
{
  const WGALDeviceCapabilities& caps = GetDeviceCapabilities();

  m_TestableFormats.Clear();
  for (WUInt32 i = 1; i < WGALResourceFormat::ENUM_COUNT; i++)
  {
    switch (i)
    {
      case WGALResourceFormat::AUByteNormalized:      // What use is this format over RUByteNormalized?
      case WGALResourceFormat::RGB10A2UInt:           // no WImage support
      case WGALResourceFormat::RGB10A2UIntNormalized: // no WImage support
      case WGALResourceFormat::D24S8:                 // no stencil readback implemented in Vulkan
        break;

      default:
      {
        if (caps.m_FormatSupport[i].AreAllSet(WGALResourceFormatSupport::Texture | WGALResourceFormatSupport::RenderTarget))
        {
          m_TestableFormats.PushBack((WGALResourceFormat::Enum)i);
        }
      }
    }
  }

  m_TestableFormatStrings.Reserve(m_TestableFormats.GetCount());
  for (WGALResourceFormat::Enum format : m_TestableFormats)
  {
    WStringBuilder sFormat;
    WReflectionUtils::EnumerationToString(WGetStaticRTTI<WGALResourceFormat>(), format, sFormat, WReflectionUtils::EnumConversionMode::ValueNameOnly);
    m_TestableFormatStrings.PushBack(sFormat);
    AddSubTest(m_TestableFormatStrings.PeekBack(), format);
  }
}

WResult WRendererTestReadback::InitializeSubTest(WInt32 iIdentifier)
{
  m_iFrame = -1;
  m_bCaptureImage = false;
  m_ImgCompFrames.Clear();
  m_bReadbackInProgress = true;

  // Texture2D
  {
    m_Format = (WGALResourceFormat::Enum)iIdentifier;
    WGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, m_Format, WGALMSAASampleCount::None);
    m_hTexture2DReadback = m_pDevice->CreateTexture(desc);

    W_ASSERT_DEBUG(!m_hTexture2DReadback.IsInvalidated(), "Failed to create readback texture");
  }

  return W_SUCCESS;
}

WResult WRendererTestReadback::DeInitializeSubTest(WInt32 iIdentifier)
{
  m_Readback.Reset();
  m_ReadBackResult.Clear();
  if (!m_hTexture2DReadback.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2DReadback);
    m_hTexture2DReadback.Invalidate();
  }
  if (!m_hTexture2DUpload.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2DUpload);
    m_hTexture2DUpload.Invalidate();
  }

  // Don't call parent's DeInitializeSubTest - renderer shutdown happens in DeInitializeTest

  return W_SUCCESS;
}

WResult WRendererTestReadback::GetImage(WImage& ref_img, const WSubTestEntry& subTest, WUInt32 uiImageNumber)
{
  if (m_ReadBackResult.IsValid())
  {
    ref_img.ResetAndCopy(m_ReadBackResult);
    m_ReadBackResult.Clear();
    return W_SUCCESS;
  }

  return SUPER::GetImage(ref_img, subTest, uiImageNumber);
}


void WRendererTestReadback::MapImageNumberToString(const char* szTestName, const WSubTestEntry& subTest, WUInt32 uiImageNumber, WStringBuilder& out_sString) const
{
  if (!m_sReadBackReferenceImage.IsEmpty())
  {
    out_sString = m_sReadBackReferenceImage;
    m_sReadBackReferenceImage.Clear();
    return;
  }

  return SUPER::MapImageNumberToString(szTestName, subTest, uiImageNumber, out_sString);
}

void WRendererTestReadback::CompareReadbackImage(WImage&& image)
{
  WStringBuilder sTemp;
  WUInt8 uiChannels = WGALResourceFormat::GetChannelCount(m_Format);
  if (WGALResourceFormat::IsDepthFormat(m_Format))
  {
    sTemp = "Readback_Depth";
  }
  else
  {
    sTemp.SetFormat("Readback_Color{}Channel", uiChannels);
  }
  m_sReadBackReferenceImage = sTemp;
  m_ReadBackResult.ResetAndMove(std::move(image));

  W_TEST_IMAGE(0, 1);
}

void WRendererTestReadback::CompareUploadImage()
{
  WStringBuilder sTemp;
  WUInt8 uiChannels = WGALResourceFormat::GetChannelCount(m_Format);
  if (WGALResourceFormat::IsDepthFormat(m_Format))
  {
    sTemp = "Readback_Upload_Depth";
  }
  else
  {
    sTemp.SetFormat("Readback_Upload_Color{}Channel", uiChannels);
  }
  m_sReadBackReferenceImage = sTemp;
  W_TEST_IMAGE(1, 3);
}

WTestAppRun WRendererTestReadback::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;
  BeginFrame();
  WTestAppRun res = Readback(uiInvocationCount);
  EndFrame();
  return res;
}


WTestAppRun WRendererTestReadback::Readback(WUInt32 uiInvocationCount)
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const WUInt32 uiColumns = 2;
  const WUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const WMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  const bool bIsDepthTexture = WGALResourceFormat::IsDepthFormat(m_Format);
  const bool bIsIntTexture = WGALResourceFormat::IsIntegerFormat(m_Format);
  const bool bIsSigned = WGALResourceFormat::IsSignedFormat(m_Format);
  if (m_iFrame == 1)
  {
    BeginCommands("Offscreen");
    {
      WGALRenderingSetup renderingSetup;

      WShaderResourceHandle shader;
      if (bIsDepthTexture)
      {
        TransitionTexture(m_hTexture2DReadback, WGALResourceState::DepthStencilWrite);
        renderingSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hTexture2DReadback));
        renderingSetup.SetClearDepth().SetClearStencil();
        shader = m_hUVColorDepthShader;
      }
      else
      {
        TransitionTexture(m_hTexture2DReadback, WGALResourceState::RenderTarget);
        renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2DReadback));
        renderingSetup.SetClearColor(0, WColor::RebeccaPurple);
        if (bIsIntTexture)
        {
          shader = bIsSigned ? m_hUVColorIntShader : m_hUVColorUIntShader;
        }
        else
        {
          shader = m_hUVColorShader;
        }
      }

      WRectFloat viewport = WRectFloat(0, 0, 8, 8);
      WRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
      SetClipSpace();

      WRenderContext::GetDefaultInstance()->BindShader(shader);
      WRenderContext::GetDefaultInstance()->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
      WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

      WRenderContext::GetDefaultInstance()->EndRendering();
    }

    // Queue readback
    {
      TransitionTexture(m_hTexture2DReadback, WGALResourceState::CopySource);
      m_Readback.ReadbackTexture(*m_pEncoder, m_hTexture2DReadback);
    }

    // Wait for results
    {
      WEnum<WGALAsyncResult> res = m_Readback.GetReadbackResult(WTime::MakeFromHours(1));
      W_ASSERT_ALWAYS(res == WGALAsyncResult::Ready, "Readback of texture failed");
    }

    // Readback result
    {
      m_bReadbackInProgress = false;

      const WGALTexture* pBackbuffer = WGALDevice::GetDefaultDevice()->GetTexture(m_hTexture2DReadback);
      WImage readBackResult;
      {
        WGALTextureSubresource sourceSubResource;
        WArrayPtr<WGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
        WTempHybridArray<WGALSystemMemoryDescription, 1> memory;
        WReadbackTextureLock lock = m_Readback.LockTexture(sourceSubResources, memory);
        W_ASSERT_ALWAYS(lock, "Failed to lock readback texture");
        WTextureUtils::CopySubResourceToImage(pBackbuffer->GetDescription(), sourceSubResource, memory[0], readBackResult, false);
      }

      {
        WGALSystemMemoryDescription MemDesc;
        MemDesc.m_pData = readBackResult.GetByteBlobPtr();
        MemDesc.m_uiRowPitch = static_cast<WUInt32>(readBackResult.GetRowPitch());
        MemDesc.m_uiSlicePitch = static_cast<WUInt32>(readBackResult.GetDepthPitch());
        WArrayPtr<WGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);

        WGALTextureCreationDescription desc;
        desc.m_uiWidth = 8;
        desc.m_uiHeight = 8;
        desc.m_Format = m_Format;
        m_hTexture2DUpload = m_pDevice->CreateTexture(desc, SysMemDescs);
        W_ASSERT_DEV(!m_hTexture2DUpload.IsInvalidated(), "Texture creation failed");
      }


      W_TEST_BOOL(readBackResult.Convert(WImageFormat::R32G32B32A32_FLOAT).Succeeded());
      if (bIsIntTexture)
      {
        // For int textures, we multiply by 127 in the shader. We reverse this here to make all formats fit into the [0-1] float range.
        const WImageFormat::Enum imageFormat = readBackResult.GetImageFormat();
        WUInt64 uiNumElements = WUInt64(8) * readBackResult.GetByteBlobPtr().GetCount() / (WUInt64)WImageFormat::GetBitsPerPixel(imageFormat);
        // Work with single channels instead of pixels
        uiNumElements *= WImageFormat::GetBitsPerPixel(imageFormat) / 32;

        const WUInt32 uiStride = 4;
        void* targetPointer = readBackResult.GetByteBlobPtr().GetPtr();
        while (uiNumElements)
        {
          WUInt8 uiChannels = WGALResourceFormat::GetChannelCount(m_Format);
          float& pixel = *reinterpret_cast<float*>(targetPointer);
          if (bIsDepthTexture)
          {
            // Don't normalize alpha channel which was added by the format extension from R to RGBA.
            if ((uiNumElements % 4) != 1)
              pixel /= WMath::MaxValue<WUInt16>();
          }
          else
          {
            // Don't normalize alpha channel if it was added by the format extension to RGBA.
            if (uiChannels == 4 || ((uiNumElements % 4) != 1))
              pixel /= 127.0f;
          }

          targetPointer = WMemoryUtils::AddByteOffset(targetPointer, uiStride);
          uiNumElements--;
        }
      }
      CompareReadbackImage(std::move(readBackResult));
    }
    EndCommands();
  }

  BeginCommands("Readback");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

    TransitionTexture(m_hTexture2DReadback, bIsDepthTexture ? WGALResourceState::DepthStencilRead : WGALResourceState::ShaderResource);

    if (bIsDepthTexture || WGALResourceFormat::IsFloatFormat(m_Format))
    {
      m_hShader = m_hTexture2DDepthShader;
    }
    else if (bIsIntTexture)
    {
      m_hShader = WGALResourceFormat::IsSignedFormat(m_Format) ? m_hTexture2DIntShader : m_hTexture2DUIntShader;
    }
    else
    {
      m_hShader = m_hTexture2DShader;
    }

    {
      WRectFloat viewport = WRectFloat(0, 0, fElementWidth, fElementHeight);
      RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2DReadback);
    }
    if (!m_bReadbackInProgress)
    {
      TransitionTexture(m_hTexture2DUpload, bIsDepthTexture ? WGALResourceState::DepthStencilRead : WGALResourceState::ShaderResource);

      WRectFloat viewport = WRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);

      WGALCommandEncoder* pCommandEncoder = BeginRendering(WColor::RebeccaPurple, 0, &viewport);
      WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroupTest.BindTexture("DiffuseTexture", m_hTexture2DUpload);
      RenderObject(m_hCubeUV, mMVP, WColor(1, 1, 1, 1), WShaderBindFlags::None);
      EndRendering();
      TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
      CompareUploadImage();
    }
  }
  EndCommands();
  return m_bReadbackInProgress ? WTestAppRun::Continue : WTestAppRun::Quit;
}

static WRendererTestReadback g_ReadbackTest;
