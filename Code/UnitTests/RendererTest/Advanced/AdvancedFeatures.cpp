#include <RendererTest/RendererTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererTest/Advanced/AdvancedFeatures.h>
#include <RendererTest/Basics/RendererTestUtils.h>
#undef CreateWindow
#if W_ENABLED(W_PLATFORM_LINUX)
#  include <sys/prctl.h>
#endif

namespace
{
  /// One column of the depth bias test. Each column draws a reference quad, then the same quad again pushed away from the viewer by s_uiDepthBiasGapUnits and rasterized with the depth bias under test. The biased quad wins the 'Less' depth test exactly if the bias pulls it back in front of the reference quad.
  struct DepthBiasCase
  {
    const char* m_szName;
    bool m_bSloped;        ///< Whether the quads are tilted, which is what makes the slope scaled bias do anything.
    WInt32 m_iDepthBias;
    float m_fSlopeScaledDepthBias;
    float m_fClampUnits;   ///< Depth bias clamp, in depth format units. Zero means no clamping.
    bool m_bExpectVisible; ///< Whether the biased quad is expected to pass the depth test.
  };

  constexpr DepthBiasCase s_DepthBiasCases[] = {
    // Without any bias the biased quad is behind the reference quad and must not show up. This is the baseline the other cases are contrasted against.
    {"NoBias", false, 0, 0.0f, 0.0f, false},
    // A negative constant bias is much larger than the gap and pulls the quad in front.
    {"ConstantTowardsViewer", false, -4000, 0.0f, 0.0f, true},
    // The same magnitude in the other direction pushes it further away.
    {"ConstantAwayFromViewer", false, 4000, 0.0f, 0.0f, false},
    // The slope scaled bias is multiplied with the depth slope of the primitive, which is zero for a screen aligned quad.
    {"SlopeScaledOnFlatGeometry", false, 0, -8.0f, 0.0f, false},
    {"SlopedNoBias", true, 0, 0.0f, 0.0f, false},
    // Same but with tilted geometry. Now the non-zero slope makes the slope scaled bias take effect.
    {"SlopedSlopeScaled", true, 0, -4.0f, 0.0f, true},
    {"SlopedConstant", true, -4000, 0.0f, 0.0f, true},
    // The clamp limits the bias to a magnitude smaller than the gap, so the quad stays hidden despite the large constant bias.
    {"ClampBelowGap", false, -4000, 0.0f, -50.0f, false},
    // The same clamp, but now permissive enough to still clear the gap.
    {"ClampAboveGap", false, -4000, 0.0f, -2000.0f, true},
  };

  constexpr WUInt32 s_uiDepthBiasCaseCount = W_ARRAY_SIZE(s_DepthBiasCases);
  constexpr WUInt32 s_uiDepthBiasCellSize = 32;
  constexpr float s_fDepthBiasBaseDepth = 0.5f;
  /// Depth difference between the reference and the biased quad, in depth format units. Small enough that every 'visible' case clears it by at least a factor of four, large enough that the depth buffer quantization cannot swallow it.
  constexpr float s_fDepthBiasGapUnits = 200.0f;
  /// Depth added per unit of the quad's local x, which spans the full cell. The resulting window space slope is 2 * s / s_uiDepthBiasCellSize.
  constexpr float s_fDepthBiasSlope = 0.15f;

  /// One cell of the conservative rasterization test. Each cell rasterizes a single quad, either one that is too small to contain a pixel center or
  /// one that comfortably covers 8x8 pixels, and counts how many pixels of the cell ended up lit.
  struct ConservativeRasterCase
  {
    const char* m_szName;
    bool m_bSubPixel; ///< Whether the quad is the sub-pixel one. Otherwise it is the 8x8 pixel one.
    bool m_bConservative;
    WUInt32 m_uiMinLitPixels;
    WUInt32 m_uiMaxLitPixels;
  };

  constexpr ConservativeRasterCase s_ConservativeRasterCases[] = {
    // Standard rasterization only produces a fragment when the pixel center is covered, which the sub-pixel quad deliberately avoids.
    {"SubPixelNormal", true, false, 0, 0},
    // Conservative (overestimated) rasterization produces a fragment for every pixel the quad touches at all, so the very same quad now shows up.
    {"SubPixelConservative", true, true, 1, 9},
    // A quad that covers whole pixels must be unaffected: overestimation may only ever add coverage, never remove any.
    {"CoveringNormal", false, false, 64, 64},
    {"CoveringConservative", false, true, 64, 100},
  };

  constexpr WUInt32 s_uiConservativeRasterCaseCount = W_ARRAY_SIZE(s_ConservativeRasterCases);
  constexpr WUInt32 s_uiConservativeRasterCellSize = 16;
  /// Window space rect of the sub-pixel quad within its cell. It lies inside pixel 8 but excludes that pixel's center at 8.5.
  constexpr float s_fConservativeRasterSubPixelMin = 8.05f;
  constexpr float s_fConservativeRasterSubPixelMax = 8.45f;
  /// Window space rect of the covering quad within its cell. The fractional bounds keep the pixel centers 4.5 to 11.5 covered without landing on a
  /// pixel edge, so standard rasterization produces exactly 8x8 fragments regardless of the fill rule.
  constexpr float s_fConservativeRasterCoveringMin = 4.4f;
  constexpr float s_fConservativeRasterCoveringMax = 11.6f;
} // namespace

void WRendererTestAdvancedFeatures::SetupSubTests()
{
  const WGALDeviceCapabilities& caps = GetDeviceCapabilities();

  AddSubTest("01 - ReadRenderTarget", SubTests::ST_ReadRenderTarget);
  if (caps.m_bSupportsVSRenderTargetArrayIndex)
  {
    AddSubTest("02 - VertexShaderRenderTargetArrayIndex", SubTests::ST_VertexShaderRenderTargetArrayIndex);
  }
#if W_ENABLED(W_SUPPORTS_PROCESSES)
  if (caps.m_bSupportsSharedTextures)
  {
    AddSubTest("03 - SharedTexture", SubTests::ST_SharedTexture);
  }
#endif

  if (caps.m_bShaderStageSupported[WGALShaderStage::HullShader])
  {
    AddSubTest("04 - Tessellation", SubTests::ST_Tessellation);
  }
  if (caps.m_bShaderStageSupported[WGALShaderStage::ComputeShader])
  {
    AddSubTest("05 - Compute", SubTests::ST_Compute);
  }
  AddSubTest("06 - FloatSampling", SubTests::ST_FloatSampling);
  AddSubTest("07 - ProxyTexture", SubTests::ST_ProxyTexture);
  AddSubTest("08 - Material", SubTests::ST_Material);

  // MSAA support is per-format. We pick the first sample count that the swap chain color format and the depth format both support.
  const auto colorSupport = caps.m_FormatSupport[WGALResourceFormat::BGRAUByteNormalized];
  const auto depthSupport = caps.m_FormatSupport[WGALResourceFormat::D24S8];
  if (colorSupport.AreAllSet(WGALResourceFormatSupport::RenderTarget | WGALResourceFormatSupport::MSAA4x) && depthSupport.IsSet(WGALResourceFormatSupport::MSAA4x))
  {
    AddSubTest("09 - MSAAResolve", SubTests::ST_MSAAResolve);
  }
  else if (colorSupport.AreAllSet(WGALResourceFormatSupport::RenderTarget | WGALResourceFormatSupport::MSAA2x) && depthSupport.IsSet(WGALResourceFormatSupport::MSAA2x))
  {
    AddSubTest("09 - MSAAResolve", SubTests::ST_MSAAResolve);
  }

  AddSubTest("10 - ViewFormatOverride", SubTests::ST_ViewFormatOverride);
  AddSubTest("11 - DepthBias", SubTests::ST_DepthBias);
  if (caps.m_bSupportsConservativeRasterization)
  {
    AddSubTest("12 - ConservativeRasterization", SubTests::ST_ConservativeRasterization);
  }
}

WResult WRendererTestAdvancedFeatures::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(WGraphicsTest::InitializeSubTest(iIdentifier));
  W_SUCCEED_OR_RETURN(CreateWindow(320, 240));

  if (iIdentifier == ST_ReadRenderTarget)
  {
    // Texture2D
    WGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, WGALResourceFormat::BGRAUByteNormalizedsRGB, WGALMSAASampleCount::None);
    m_hTexture2D = m_pDevice->CreateTexture(desc);

    m_Texture2DRange = {};
    m_Texture2DRange.m_uiMipLevels = 1;
    m_Texture2DRange.m_uiBaseMipLevel = 0;

    m_hShader2 = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/UVColor.WShader");
    m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Texture2D.WShader");
  }

  if (iIdentifier == ST_ProxyTexture)
  {
    // Texture2DArray
    WGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, WGALResourceFormat::BGRAUByteNormalizedsRGB, WGALMSAASampleCount::None);
    desc.m_Type = WGALTextureType::Texture2DArray;
    desc.m_uiArraySize = 2;
    m_hTexture2DArray = m_pDevice->CreateTexture(desc);

    // Proxy texture
    m_hProxyTexture2D[0] = m_pDevice->CreateProxyTexture(m_hTexture2DArray, 0);
    m_hProxyTexture2D[1] = m_pDevice->CreateProxyTexture(m_hTexture2DArray, 1);

    m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Texture2D.WShader");
    m_hShader2 = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/UVColor.WShader");
    m_hShader3 = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/UVColor2.WShader");
  }

  if (iIdentifier == ST_ViewFormatOverride)
  {
    // Rendering through both the UNorm and the sRGB view of one image is only guaranteed for one of the two channel orders, so use whichever the device supports.
    const WGALDeviceCapabilities& caps = m_pDevice->GetCapabilities();
    auto IsRenderTarget = [&](WGALResourceFormat::Enum format)
    { return caps.m_FormatSupport[format].IsSet(WGALResourceFormatSupport::RenderTarget); };

    WEnum<WGALResourceFormat> unormFormat;
    if (IsRenderTarget(WGALResourceFormat::BGRAUByteNormalized) && IsRenderTarget(WGALResourceFormat::BGRAUByteNormalizedsRGB))
    {
      unormFormat = WGALResourceFormat::BGRAUByteNormalized;
      m_OverrideSrgbFormat = WGALResourceFormat::BGRAUByteNormalizedsRGB;
    }
    else if (IsRenderTarget(WGALResourceFormat::RGBAUByteNormalized) && IsRenderTarget(WGALResourceFormat::RGBAUByteNormalizedsRGB))
    {
      unormFormat = WGALResourceFormat::RGBAUByteNormalized;
      m_OverrideSrgbFormat = WGALResourceFormat::RGBAUByteNormalizedsRGB;
    }
    else
    {
      W_TEST_FAILURE("No suitable format", "Neither the BGRA nor the RGBA UNorm/sRGB pair is supported as a render target, at least one of them has to be.");
      return W_FAILURE;
    }

    // Both textures are created as UNorm, the second one is rendered into through an sRGB render target view so the hardware applies the linear -> sRGB transfer function on write and the stored bits differ. Combining both with an sRGB sampled view isolates the write-side and read-side effects of the format override against each other.
    for (WUInt32 i = 0; i < 2; i++)
    {
      WGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(8, 8, unormFormat, WGALMSAASampleCount::None);
      m_hOverrideTexture2D[i] = m_pDevice->CreateTexture(desc);

      WGALRenderTargetViewCreationDescription viewDesc;
      viewDesc.m_hTexture = m_hOverrideTexture2D[i];
      viewDesc.m_OverrideViewFormat = i == 0 ? WEnum<WGALResourceFormat>(WGALResourceFormat::Invalid) : m_OverrideSrgbFormat;

      m_hOverrideRTV[i] = m_pDevice->GetRenderTargetView(viewDesc);
      if (!W_TEST_BOOL(!m_hOverrideRTV[i].IsInvalidated()))
        return W_FAILURE;
    }

    m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Texture2D.WShader");
    m_hShader2 = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/UVColor.WShader");
  }

  if (iIdentifier == ST_FloatSampling)
  {
    // Texture2DArray
    WGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, WGALResourceFormat::D16, WGALMSAASampleCount::None);
    desc.m_Type = WGALTextureType::Texture2DArray;
    m_hTexture2DArray = m_pDevice->CreateTexture(desc);

    m_hShader2 = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/ReadbackDepth.WShader");
    m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/SampleLevel_PointClampBorder.WShader");

    WGALSamplerStateCreationDescription samplerDesc;
    samplerDesc.m_MinFilter = WGALTextureFilterMode::Point;
    samplerDesc.m_MagFilter = WGALTextureFilterMode::Point;
    samplerDesc.m_MipFilter = WGALTextureFilterMode::Point;
    samplerDesc.m_AddressU = WImageAddressMode::ClampBorder;
    samplerDesc.m_AddressV = WImageAddressMode::ClampBorder;
    samplerDesc.m_AddressW = WImageAddressMode::ClampBorder;
    samplerDesc.m_BorderColor = WColor::White;

    m_hDepthSamplerState = WGALDevice::GetDefaultDevice()->CreateSamplerState(samplerDesc);
  }

  if (iIdentifier == ST_Compute)
  {
    // Texture2D as compute RW target. Note that SRGB and depth formats are not supported by most graphics cards for this purpose.
    WEnum<WGALResourceFormat> textureFormat;
    WGALResourceFormat::Enum formats[] = {WGALResourceFormat::RGBAFloat, WGALResourceFormat::BGRAUByteNormalized, WGALResourceFormat::RGBAUByteNormalized};
    for (auto format : formats)
    {
      if (m_pDevice->GetCapabilities().m_FormatSupport[format].IsSet(WGALResourceFormatSupport::TextureRW))
      {
        textureFormat = format;
        break;
      }
    }
    if (!W_TEST_BOOL(textureFormat != WGALResourceFormat::Invalid))
      return W_FAILURE;

    // We are only rendering to mip map level 4 (8x8).
    // The array and levels and mip size is only here to test sub-resource views and Nvidia bugs (image will be too dark): https://forums.developer.nvidia.com/t/vulkan-driver-bug-regression-in-hlsl-getdimensions-on-rwtexture2darray/315282
    WGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(128, 128, textureFormat, WGALMSAASampleCount::None);
    desc.m_Type = WGALTextureType::Texture2DArray;
    desc.m_TextureFlags = WGALTextureUsageFlags::ShaderResource | WGALTextureUsageFlags::UnorderedAccess;
    desc.m_uiArraySize = 2;
    desc.m_uiMipLevelCount = 6;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hTexture2D = m_pDevice->CreateTexture(desc);

    m_Texture2DRange = {};
    m_Texture2DRange.m_uiBaseMipLevel = 4;
    m_Texture2DRange.m_uiMipLevels = 1;
    m_Texture2DRange.m_uiBaseArraySlice = 0;
    m_Texture2DRange.m_uiArraySlices = 1;

    m_hShader2 = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/UVColorCompute.WShader");
    m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Texture2DReadbackDepth.WShader");
  }

  if (iIdentifier == ST_VertexShaderRenderTargetArrayIndex)
  {
    // Texture2DArray
    WGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(320 / 2, 240, WGALResourceFormat::BGRAUByteNormalizedsRGB, WGALMSAASampleCount::None);
    desc.m_Type = WGALTextureType::Texture2DArray;
    desc.m_uiArraySize = 2;
    m_hTexture2DArray = m_pDevice->CreateTexture(desc);

    m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Stereo.WShader");
    m_hShader2 = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/StereoPreview.WShader");
  }

  if (iIdentifier == ST_Material)
  {
    // Texture Resource
    WGALTextureCreationDescription galTexDesc;
    galTexDesc.m_uiWidth = 8;
    galTexDesc.m_uiHeight = 8;
    galTexDesc.m_uiMipLevelCount = 1;
    galTexDesc.m_Format = WGALResourceFormat::BGRAUByteNormalizedsRGB;

    WImage coloredMips;
    WRendererTestUtils::CreateImage(coloredMips, galTexDesc.m_uiWidth, galTexDesc.m_uiHeight, 1, true);

    WTempHybridArray<WGALSystemMemoryDescription, 1> initialData;
    initialData.SetCount(galTexDesc.m_uiMipLevelCount);
    for (WUInt32 m = 0; m < galTexDesc.m_uiMipLevelCount; m++)
    {
      WGALSystemMemoryDescription& memoryDesc = initialData[m];
      memoryDesc.m_pData = coloredMips.GetSubImageView(m).GetByteBlobPtr();
      memoryDesc.m_uiRowPitch = static_cast<WUInt32>(coloredMips.GetRowPitch(m));
      memoryDesc.m_uiSlicePitch = static_cast<WUInt32>(coloredMips.GetDepthPitch(m));
    }

    m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/TestMaterial.WShader");

    WGALSamplerStateCreationDescription samplerDesc;
    samplerDesc.m_MinFilter = WGALTextureFilterMode::Point;
    samplerDesc.m_MagFilter = WGALTextureFilterMode::Point;
    samplerDesc.m_MipFilter = WGALTextureFilterMode::Point;
    samplerDesc.m_AddressU = WImageAddressMode::ClampBorder;
    samplerDesc.m_AddressV = WImageAddressMode::ClampBorder;
    samplerDesc.m_AddressW = WImageAddressMode::ClampBorder;
    samplerDesc.m_BorderColor = WColor::White;

    WTexture2DResourceDescriptor texDesc;
    texDesc.m_DescGAL = galTexDesc;
    texDesc.m_InitialContent = initialData;
    texDesc.m_SamplerDesc = samplerDesc;

    m_hTexture = WResourceManager::LoadResource<WTexture2DResource>("White.color");
    m_hTexture2 = WResourceManager::CreateResource<WTexture2DResource>("TestTexture", std::move(texDesc), "A Test Texture");

    // Material
    WMaterialResourceDescriptor matDesc;
    matDesc.m_hShader = m_hShader;
    matDesc.m_RenderDataCategory = WDefaultRenderDataCategories::LitOpaque;
    m_sBaseColor.Assign("BaseColor");
    m_sBaseColor2.Assign("BaseColor2");
    matDesc.m_Parameters.PushBack({m_sBaseColor, WColor::White});
    matDesc.m_Parameters.PushBack({m_sBaseColor2, WColor::White});

    m_sTexture.Assign("DiffuseTexture");
    matDesc.m_Texture2DBindings.PushBack({m_sTexture, m_hTexture});

    m_hMaterial = WResourceManager::CreateResource<WMaterialResource>("TestMaterial", std::move(matDesc), "A Test Material");
  }

#if W_ENABLED(W_SUPPORTS_PROCESSES)
  if (iIdentifier == ST_SharedTexture)
  {
    WCVarFloat* pProfilingThreshold = (WCVarFloat*)WCVar::FindCVarByName("Profiling.DiscardThresholdMS");
    W_ASSERT_DEBUG(pProfilingThreshold, "Profiling.cpp cvar was renamed");
    m_fOldProfilingThreshold = *pProfilingThreshold;
    *pProfilingThreshold = 0.0f;

    m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Texture2D.WShader");

    const WStringBuilder pathToSelf = WCommandLineUtils::GetGlobalInstance()->GetParameter(0);

    WProcessOptions opt;
    opt.m_sProcess = pathToSelf;

    WStringBuilder sIPC;
    WConversionUtils::ToString(WUuid::MakeUuid(), sIPC);

    WStringBuilder sPID;
    WConversionUtils::ToString(WProcess::GetCurrentProcessID(), sPID);

#  ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
    constexpr const char* szDefaultRenderer = "Vulkan";
#  else
    constexpr const char* szDefaultRenderer = "DX11";
#  endif
    WStringView sRendererName = WCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);

    opt.m_Arguments.PushBack("-offscreen");
    opt.m_Arguments.PushBack("-IPC");
    opt.m_Arguments.PushBack(sIPC);
    opt.m_Arguments.PushBack("-PID");
    opt.m_Arguments.PushBack(sPID);
    opt.m_Arguments.PushBack("-renderer");
    opt.m_Arguments.PushBack(sRendererName);
    opt.m_Arguments.PushBack("-outputDir");
    opt.m_Arguments.PushBack(WTestFramework::GetInstance()->GetAbsOutputPath());
    m_pOffscreenProcess = W_DEFAULT_NEW(WProcess);

    // Start the IPC server and wait for the "Connecting" state before starting the client process or it will fail to connect.
    m_pChannel = WIpcChannel::CreatePipeChannel(sIPC, WIpcChannel::Mode::Server);
    m_pProtocol = W_DEFAULT_NEW(WIpcProcessMessageProtocol, m_pChannel.Borrow());
    m_pProtocol->m_MessageEvent.AddEventHandler(WMakeDelegate(&WRendererTestAdvancedFeatures::OffscreenProcessMessageFunc, this));
    W_SUCCEED_OR_RETURN(m_pChannel->Connect());
    while (m_pChannel->GetConnectionState() != WIpcChannel::ConnectionState::Connecting)
    {
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(16));
    }

    W_SUCCEED_OR_RETURN(m_pOffscreenProcess->Launch(opt));

#  if W_ENABLED(W_PLATFORM_LINUX)
    // pidfd_getfd which is used to open the shared textures on Linux Vulkan is blocked by Yama ptrace_scope. With this command we allow our child process to ptrace us.
    if (prctl(PR_SET_PTRACER, m_pOffscreenProcess->GetProcessID()) != 0)
    {
      WLog::Error("prctl command failed with: {}", WArgErrno(errno));
    }
#  endif

    m_bExiting = false;
    m_uiReceivedTextures = 0;

    m_SharedTextureDesc.SetAsRenderTarget(8, 8, WGALResourceFormat::BGRAUByteNormalizedsRGB);
    m_SharedTextureDesc.m_Type = WGALTextureType::Texture2DShared;

    m_SharedTextureQueue.Clear();
    for (WUInt32 i = 0; i < s_SharedTextureCount; i++)
    {
      m_hSharedTextures[i] = m_pDevice->CreateSharedTexture(m_SharedTextureDesc);
      W_TEST_BOOL(!m_hSharedTextures[i].IsInvalidated());
      m_SharedTextureQueue.PushBack({i, 0});
    }

    while (m_pChannel->GetConnectionState() == WIpcChannel::ConnectionState::Connecting)
    {
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(16));
      if (m_pOffscreenProcess->GetState() == WProcessState::Finished)
      {
        WUInt32 uiExitCode = m_pOffscreenProcess->GetExitCode();
        WLog::Error("Process exited prematurely with code: {}", uiExitCode);
        return W_FAILURE;
      }
    }

    if (m_pChannel->GetConnectionState() != WIpcChannel::ConnectionState::Connected)
    {
      WLog::Error("Failed to connect to offscreen process");
      return W_FAILURE;
    }

    WOffscreenTest_OpenMsg msg;
    msg.m_TextureDesc = m_SharedTextureDesc;
    for (auto& hSharedTexture : m_hSharedTextures)
    {
      const WGALSharedTexture* pSharedTexture = m_pDevice->GetSharedTexture(hSharedTexture);
      if (pSharedTexture == nullptr)
      {
        return W_FAILURE;
      }

      msg.m_TextureHandles.PushBack(pSharedTexture->GetSharedHandle());
    }
    m_pProtocol->Send(&msg);
  }
#endif

  if (iIdentifier == ST_Tessellation)
  {
    {
      WGeometry geom;
      geom.AddStackedSphere(0.5f, 3, 2);

      WMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

      m_hSphereMesh = WResourceManager::CreateResource<WMeshBufferResource>("UnitTest-SphereMesh", std::move(desc), "SphereMesh");
    }

    m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Tessellation.WShader");
  }

  if (iIdentifier == ST_MSAAResolve)
  {
    const auto colorSupport = m_pDevice->GetCapabilities().m_FormatSupport[WGALResourceFormat::BGRAUByteNormalized];
    const auto depthSupport = m_pDevice->GetCapabilities().m_FormatSupport[WGALResourceFormat::D24S8];
    if (colorSupport.IsSet(WGALResourceFormatSupport::MSAA4x) && depthSupport.IsSet(WGALResourceFormatSupport::MSAA4x))
      m_MSAASamples = WGALMSAASampleCount::FourSamples;
    else
      m_MSAASamples = WGALMSAASampleCount::TwoSamples;

    constexpr WUInt32 uiW = 64;
    constexpr WUInt32 uiH = 64;

    {
      WGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(uiW, uiH, WGALResourceFormat::BGRAUByteNormalized, m_MSAASamples);
      m_hMSAAColor = m_pDevice->CreateTexture(desc);
      W_TEST_BOOL(!m_hMSAAColor.IsInvalidated());
    }
    {
      WGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(uiW, uiH, WGALResourceFormat::D24S8, m_MSAASamples);
      m_hMSAADepthStencil = m_pDevice->CreateTexture(desc);
      W_TEST_BOOL(!m_hMSAADepthStencil.IsInvalidated());
    }
    {
      // Resolve target must match the MSAA color format and is read back for verification, so it needs the default usage flags only.
      WGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(uiW, uiH, WGALResourceFormat::BGRAUByteNormalized, WGALMSAASampleCount::None);
      m_hMSAAResolveTarget = m_pDevice->CreateTexture(desc);
      W_TEST_BOOL(!m_hMSAAResolveTarget.IsInvalidated());
    }

    {
      // A simple full-NDC quad. The stencil shader (StencilColor.WShader) only reads POSITION.
      WGeometry geom;
      geom.AddRect(WVec2(2.0f, 2.0f), 1, 1);

      WMeshBufferResourceDescriptor desc;
      desc.AddStream(WMeshVertexStreamType::Position);
      desc.AddStream(WMeshVertexStreamType::Color0);
      desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

      m_hMSAAQuadMesh = WResourceManager::GetOrCreateResource<WMeshBufferResource>("MSAAResolveQuad", std::move(desc), "MSAAResolveQuad");
    }

    m_hMSAAStencilShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/StencilColor.WShader");
  }

  if (iIdentifier == ST_DepthBias)
  {
    // The depth bias is expressed in multiples of the smallest resolvable difference of the depth format, so only formats with a known fixed-point
    // representation can be used here. For a floating point depth buffer that unit depends on the depth values of the primitive itself.
    const WGALDeviceCapabilities& caps = m_pDevice->GetCapabilities();
    WEnum<WGALResourceFormat> depthFormat;
    if (caps.m_FormatSupport[WGALResourceFormat::D16].IsSet(WGALResourceFormatSupport::RenderTarget))
    {
      depthFormat = WGALResourceFormat::D16;
      m_fDepthBiasUnit = 1.0f / 65535.0f;
    }
    else if (caps.m_FormatSupport[WGALResourceFormat::D24S8].IsSet(WGALResourceFormatSupport::RenderTarget))
    {
      depthFormat = WGALResourceFormat::D24S8;
      m_fDepthBiasUnit = 1.0f / 16777215.0f;
    }
    else
    {
      W_TEST_FAILURE("No suitable format", "Neither D16 nor D24S8 is supported as a depth target, at least one of them has to be.");
      return W_FAILURE;
    }

    {
      // Linear format so the readback values can be compared without an sRGB conversion.
      WGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(s_uiDepthBiasCellSize * s_uiDepthBiasCaseCount, s_uiDepthBiasCellSize, WGALResourceFormat::BGRAUByteNormalized, WGALMSAASampleCount::None);
      m_hDepthBiasColor = m_pDevice->CreateTexture(desc);
      if (!W_TEST_BOOL(!m_hDepthBiasColor.IsInvalidated()))
        return W_FAILURE;
    }
    {
      WGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(s_uiDepthBiasCellSize * s_uiDepthBiasCaseCount, s_uiDepthBiasCellSize, depthFormat, WGALMSAASampleCount::None);
      m_hDepthBiasDepth = m_pDevice->CreateTexture(desc);
      if (!W_TEST_BOOL(!m_hDepthBiasDepth.IsInvalidated()))
        return W_FAILURE;
    }

    {
      // A full-NDC quad. StencilColor.WShader only reads POSITION, the depth of each quad comes entirely from its transform.
      WGeometry geom;
      geom.AddRect(WVec2(2.0f, 2.0f), 1, 1);

      WMeshBufferResourceDescriptor desc;
      desc.AddStream(WMeshVertexStreamType::Position);
      desc.AddStream(WMeshVertexStreamType::Color0);
      desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

      m_hDepthBiasQuadMesh = WResourceManager::GetOrCreateResource<WMeshBufferResource>("DepthBiasQuad", std::move(desc), "DepthBiasQuad");
    }

    m_hDepthBiasShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/StencilColor.WShader");
  }

  if (iIdentifier == ST_ConservativeRasterization)
  {
    {
      // Linear format so the readback values can be compared without an sRGB conversion.
      WGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(s_uiConservativeRasterCellSize * s_uiConservativeRasterCaseCount, s_uiConservativeRasterCellSize, WGALResourceFormat::BGRAUByteNormalized, WGALMSAASampleCount::None);
      m_hConservativeRasterColor = m_pDevice->CreateTexture(desc);
      if (!W_TEST_BOOL(!m_hConservativeRasterColor.IsInvalidated()))
        return W_FAILURE;
    }

    {
      // A full-NDC quad. StencilColor.WShader only reads POSITION, the position of each quad comes entirely from its transform.
      WGeometry geom;
      geom.AddRect(WVec2(2.0f, 2.0f), 1, 1);

      WMeshBufferResourceDescriptor desc;
      desc.AddStream(WMeshVertexStreamType::Position);
      desc.AddStream(WMeshVertexStreamType::Color0);
      desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

      m_hConservativeRasterQuadMesh = WResourceManager::GetOrCreateResource<WMeshBufferResource>("ConservativeRasterQuad", std::move(desc), "ConservativeRasterQuad");
    }

    m_hConservativeRasterShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/StencilColor.WShader");
  }

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_SharedTexture:
      m_ImgCompFrames.PushBack(100000000);
      break;
    case SubTests::ST_Tessellation:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_Compute:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_FloatSampling:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_ProxyTexture:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_Material:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::Material_ColorChange);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::Material_ColorChange2);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::Material_ChangeTexture);
      break;
    case SubTests::ST_ViewFormatOverride:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_MSAAResolve:
    case SubTests::ST_DepthBias:
    case SubTests::ST_ConservativeRasterization:
      // Uses readback for verification
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return W_SUCCESS;
}

WResult WRendererTestAdvancedFeatures::DeInitializeSubTest(WInt32 iIdentifier)
{
  if (iIdentifier == ST_Tessellation)
  {
    m_hSphereMesh.Invalidate();
  }
#if W_ENABLED(W_SUPPORTS_PROCESSES)
  else if (iIdentifier == ST_SharedTexture)
  {
    W_TEST_BOOL(m_pOffscreenProcess->WaitToFinish(WTime::MakeFromSeconds(5)).Succeeded());
    W_TEST_BOOL(m_pOffscreenProcess->GetState() == WProcessState::Finished);
    W_TEST_INT(m_pOffscreenProcess->GetExitCode(), 0);
    m_pOffscreenProcess = nullptr;

    m_pProtocol = nullptr;
    m_pChannel = nullptr;

    for (WUInt32 i = 0; i < s_SharedTextureCount; i++)
    {
      m_pDevice->DestroySharedTexture(m_hSharedTextures[i]);
    }
    m_SharedTextureQueue.Clear();

    WStringView sPath = ":imgout/Profiling/sharedTexture.json"_wsv;
    W_TEST_RESULT(WProfilingUtils::SaveProfilingCapture(sPath));
    WStringView sPath2 = ":imgout/Profiling/offscreenProfiling.json"_wsv;
    WStringView sMergedFile = ":imgout/Profiling/sharedTexturesMerged.json"_wsv;
    W_TEST_RESULT(WProfilingUtils::MergeProfilingCaptures(sPath, sPath2, sMergedFile));

    WCVarFloat* pProfilingThreshold = (WCVarFloat*)WCVar::FindCVarByName("Profiling.DiscardThresholdMS");
    W_ASSERT_DEBUG(pProfilingThreshold, "Profiling.cpp cvar was renamed");
    *pProfilingThreshold = m_fOldProfilingThreshold;
  }
#endif
  else if (iIdentifier == ST_FloatSampling)
  {
    WGALDevice::GetDefaultDevice()->DestroySamplerState(m_hDepthSamplerState);
  }

  if (iIdentifier == ST_ProxyTexture)
  {
    for (WUInt32 i = 0; i < 2; i++)
    {
      WGALDevice::GetDefaultDevice()->DestroyProxyTexture(m_hProxyTexture2D[i]);
    }
  }
  if (iIdentifier == ST_ViewFormatOverride)
  {
    for (WUInt32 i = 0; i < 2; i++)
    {
      m_hOverrideRTV[i].Invalidate();
      m_pDevice->DestroyTexture(m_hOverrideTexture2D[i]);
    }
    m_OverrideSrgbFormat = WGALResourceFormat::Invalid;
  }
  m_hShader2.Invalidate();
  m_hShader3.Invalidate();

  m_hTexture.Invalidate();
  m_hTexture2.Invalidate();
  m_hMaterial.Invalidate();

  if (iIdentifier == ST_MSAAResolve)
  {
    m_MSAAReadback.Reset();
    m_hMSAAQuadMesh.Invalidate();
    m_hMSAAStencilShader.Invalidate();
    m_pDevice->DestroyTexture(m_hMSAAColor);
    m_pDevice->DestroyTexture(m_hMSAADepthStencil);
    m_pDevice->DestroyTexture(m_hMSAAResolveTarget);
  }

  m_pDevice->DestroyTexture(m_hTexture2D);
  m_pDevice->DestroyTexture(m_hTexture2DArray);

  if (iIdentifier == ST_DepthBias)
  {
    m_DepthBiasReadback.Reset();
    m_hDepthBiasQuadMesh.Invalidate();
    m_hDepthBiasShader.Invalidate();
    m_pDevice->DestroyTexture(m_hDepthBiasColor);
    m_pDevice->DestroyTexture(m_hDepthBiasDepth);
    m_hDepthBiasDepth.Invalidate();
  }

  if (iIdentifier == ST_ConservativeRasterization)
  {
    m_ConservativeRasterReadback.Reset();
    m_hConservativeRasterQuadMesh.Invalidate();
    m_hConservativeRasterShader.Invalidate();
    m_pDevice->DestroyTexture(m_hConservativeRasterColor);
  }

  DestroyWindow();
  W_SUCCEED_OR_RETURN(WGraphicsTest::DeInitializeSubTest(iIdentifier));
  return W_SUCCESS;
}

WTestAppRun WRendererTestAdvancedFeatures::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;

#if W_ENABLED(W_SUPPORTS_PROCESSES)
  if (iIdentifier == ST_SharedTexture)
  {
    return SharedTexture();
  }
#endif
  if (iIdentifier == ST_Material)
  {
    return Material();
  }

  BeginFrame();

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      ReadRenderTarget();
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      if (!m_pDevice->GetCapabilities().m_bSupportsVSRenderTargetArrayIndex)
        return WTestAppRun::Quit;
      VertexShaderRenderTargetArrayIndex();
      break;
    case SubTests::ST_Tessellation:
      Tessellation();
      break;
    case SubTests::ST_Compute:
      Compute();
      break;
    case SubTests::ST_FloatSampling:
      FloatSampling();
      break;
    case SubTests::ST_ProxyTexture:
      ProxyTexture();
      break;
    case SubTests::ST_MSAAResolve:
      MSAAResolve();
      break;
    case SubTests::ST_ViewFormatOverride:
      ViewFormatOverride();
      break;
    case SubTests::ST_DepthBias:
      DepthBias();
      break;
    case SubTests::ST_ConservativeRasterization:
      ConservativeRasterization();
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  EndFrame();

  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return WTestAppRun::Quit;
  }
  return WTestAppRun::Continue;
}

void WRendererTestAdvancedFeatures::ReadRenderTarget()
{
  BeginCommands("Offscreen");
  {
    TransitionTexture(m_hTexture2D, WGALResourceState::RenderTarget);

    WGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2D));
    renderingSetup.SetClearColor(0, WColor::RebeccaPurple);

    WRectFloat viewport = WRectFloat(0, 0, 8, 8);
    WRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    WRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    WRenderContext::GetDefaultInstance()->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
    WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    WRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();


  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const WUInt32 uiColumns = 2;
  const WUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const WMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  BeginCommands("Texture2D");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
    TransitionTexture(m_hTexture2D, WGALResourceState::ShaderResource, m_Texture2DRange);
    TransitionTexture(m_hDepthStencilTexture, WGALResourceState::DepthStencilWrite);

    WRectFloat viewport = WRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D, m_Texture2DRange);
    viewport = WRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D, m_Texture2DRange);
    viewport = WRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D, m_Texture2DRange);
    m_bCaptureImage = true;
    viewport = WRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D, m_Texture2DRange);
  }
  EndCommands();
}

void WRendererTestAdvancedFeatures::FloatSampling()
{
  BeginCommands("Offscreen");
  {
    TransitionTexture(m_hTexture2DArray, WGALResourceState::DepthStencilWrite);

    WGALRenderingSetup renderingSetup;
    renderingSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hTexture2DArray));
    renderingSetup.SetClearDepth();

    WRectFloat viewport = WRectFloat(0, 0, 8, 8);
    WRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    WRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    WRenderContext::GetDefaultInstance()->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
    WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    WRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();


  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const WUInt32 uiColumns = 2;
  const WUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const WMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  BeginCommands("FloatSampling");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
    TransitionTexture(m_hTexture2DArray, WGALResourceState::DepthStencilRead);

    WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();
    bindGroupTest.BindSampler("DepthSampler", m_hDepthSamplerState);
    bindGroupTest.BindTexture("DepthTexture", m_hTexture2DArray);

    WRectFloat viewport = WRectFloat(0, 0, fElementWidth, fElementHeight);
    {
      WGALCommandEncoder* pCommandEncoder = BeginRendering(WColor::RebeccaPurple, 0xFFFFFFFF, &viewport);
      RenderObject(m_hCubeUV, mMVP, WColor(1, 1, 1, 1), WShaderBindFlags::None);
      EndRendering();
      if (m_ImgCompFrames.Contains(m_iFrame))
      {
        TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
        W_TEST_IMAGE(m_iFrame, 100);
      }
    }
  }
  EndCommands();
}


void WRendererTestAdvancedFeatures::ProxyTexture()
{
  // We render normal pattern to layer 0 and the blue pattern to layer 1.
  BeginCommands("Offscreen");
  for (WUInt8 i = 0; i < 2; i++)
  {
    TransitionTexture(m_hProxyTexture2D[i], WGALResourceState::RenderTarget);

    WGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hProxyTexture2D[i]));
    renderingSetup.SetClearColor(0, WColor::RebeccaPurple);

    WRectFloat viewport = WRectFloat(0, 0, 8, 8);
    WRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    WRenderContext::GetDefaultInstance()->BindShader(i == 0 ? m_hShader2 : m_hShader3);
    WRenderContext::GetDefaultInstance()->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
    WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    WRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();

  // Render both layers using proxy texture (2D) and manually created resource view (2DArray).
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const WUInt32 uiColumns = 2;
  const WUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const WMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  BeginCommands("Texture2DProxy");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
    TransitionTexture(m_hTexture2DArray, WGALResourceState::ShaderResource, {0, 1, 0, 1});
    TransitionTexture(m_hTexture2DArray, WGALResourceState::ShaderResource, {1, 1, 0, 1});

    WRectFloat viewport = WRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hProxyTexture2D[0]);
    viewport = WRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hProxyTexture2D[1]);
    viewport = WRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray, {0, 1, 0, 1});
    m_bCaptureImage = true;
    viewport = WRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray, {1, 1, 0, 1});
  }
  EndCommands();
}

void WRendererTestAdvancedFeatures::ViewFormatOverride()
{
  // Render the same gradient into two identically created UNorm textures, the second one through an sRGB render target view.
  BeginCommands("Offscreen");
  for (WUInt32 i = 0; i < 2; i++)
  {
    TransitionTexture(m_hOverrideTexture2D[i], WGALResourceState::RenderTarget);

    WGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_hOverrideRTV[i]);
    renderingSetup.SetClearColor(0, WColor::RebeccaPurple);

    WRectFloat viewport = WRectFloat(0, 0, 8, 8);
    WRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    WRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    WRenderContext::GetDefaultInstance()->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
    WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    WRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();

  // Each column combines one of the two textures with one of the two sampled view formats, so the write-side and read-side overrides can be told apart:
  // 0: write UNorm, read UNorm - the raw gradient.
  // 1: write sRGB,  read UNorm - encoded bits sampled raw, brighter.
  // 2: write UNorm, read sRGB  - raw bits decoded on read, darker.
  // 3: write sRGB,  read sRGB  - the encode cancels the decode, so this must match column 0.
  struct Column
  {
    WUInt32 m_uiTexture;
    WEnum<WGALResourceFormat> m_ReadFormat;
  };
  const Column columns[] = {
    {0, WGALResourceFormat::Invalid},
    {1, WGALResourceFormat::Invalid},
    {0, m_OverrideSrgbFormat},
    {1, m_OverrideSrgbFormat},
  };

  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const WUInt32 uiColumns = W_ARRAY_SIZE(columns);
  const float fElementWidth = fWidth / uiColumns;

  const WMat4 mMVP = CreateSimpleMVP(fElementWidth / fHeight);
  BeginCommands("ViewFormatOverride");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
    TransitionTexture(m_hDepthStencilTexture, WGALResourceState::DepthStencilWrite);
    TransitionTexture(m_hOverrideTexture2D[0], WGALResourceState::ShaderResource);
    TransitionTexture(m_hOverrideTexture2D[1], WGALResourceState::ShaderResource);

    for (WUInt32 i = 0; i < uiColumns; i++)
    {
      if (i == uiColumns - 1)
        m_bCaptureImage = true;

      WRectFloat viewport = WRectFloat(fElementWidth * i, 0, fElementWidth, fHeight);
      BeginRendering(WColor::RebeccaPurple, i == 0 ? 0xFFFFFFFF : 0, &viewport);
      {
        WBindGroupBuilder& bindGroup = WRenderContext::GetDefaultInstance()->GetBindGroup();
        bindGroup.BindTexture("DiffuseTexture", m_hOverrideTexture2D[columns[i].m_uiTexture], {}, columns[i].m_ReadFormat);
        RenderObject(m_hCubeUV, mMVP, WColor(1, 1, 1, 1), WShaderBindFlags::None);
      }
      EndRendering();

      if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
      {
        TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
        W_TEST_IMAGE(m_iFrame, 100);
      }
    }
  }
  EndCommands();
}

void WRendererTestAdvancedFeatures::VertexShaderRenderTargetArrayIndex()
{
  m_bCaptureImage = true;
  const WMat4 mMVP = CreateSimpleMVP((m_pWindow->GetClientAreaSize().width / 2.0f) / (float)m_pWindow->GetClientAreaSize().height);
  BeginCommands("Offscreen Stereo");
  {
    TransitionTexture(m_hTexture2DArray, WGALResourceState::RenderTarget);

    WGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2DArray));
    renderingSetup.SetClearColor(0, WColor::RebeccaPurple);

    WRectFloat viewport = WRectFloat(0, 0, m_pWindow->GetClientAreaSize().width / 2.0f, (float)m_pWindow->GetClientAreaSize().height);
    WRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    WRenderContext::GetDefaultInstance()->BindShader(m_hShader, WShaderBindFlags::None);
    ObjectCB* ocb = WRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP = mMVP;
    ocb->m_Color = WColor(1, 1, 1, 1);
    WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();
    bindGroupTest.BindBuffer("PerObject", m_hObjectTransformCB);
    WRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hCubeUV);
    WRenderContext::GetDefaultInstance()->DrawMeshBuffer(0xFFFFFFFF, 0, 2).IgnoreResult();

    WRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();


  BeginCommands("Texture2DArray");
  {
    TransitionTexture(m_hTexture2DArray, WGALResourceState::ShaderResource);

    WRectFloat viewport = WRectFloat(0, 0, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

    WGALCommandEncoder* pCommandEncoder = BeginRendering(WColor::RebeccaPurple, 0xFFFFFFFF, &viewport);

    WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();
    bindGroupTest.BindTexture("DiffuseTexture", m_hTexture2DArray);

    WRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    WRenderContext::GetDefaultInstance()->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
    WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    EndRendering();
    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
      W_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

void WRendererTestAdvancedFeatures::Tessellation()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const WMat4 mMVP = CreateSimpleMVP((float)fWidth / (float)fHeight);
  BeginCommands("Tessellation");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
    WRectFloat viewport = WRectFloat(0, 0, fWidth, fHeight);
    WGALCommandEncoder* pCommandEncoder = BeginRendering(WColor::RebeccaPurple, 0xFFFFFFFF, &viewport);
    RenderObject(m_hSphereMesh, mMVP, WColor(1, 1, 1, 1), WShaderBindFlags::None);

    EndRendering();
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
      W_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}


void WRendererTestAdvancedFeatures::Compute()
{
  BeginCommands("Compute");
  {
    WUInt32 uiWidth = 8;
    WUInt32 uiHeight = 8;

    WRenderContext::GetDefaultInstance()->BeginCompute("Compute");
    {
      WRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
      WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();

      WGALTextureRange textureRange;
      textureRange.m_uiBaseMipLevel = 4;
      textureRange.m_uiMipLevels = 1;
      textureRange.m_uiBaseArraySlice = 0;
      textureRange.m_uiArraySlices = 2;

      TransitionTexture(m_hTexture2D, WGALResourceState::UnorderedAccess, textureRange);

      bindGroupTest.BindTexture("OutputTexture", m_hTexture2D, textureRange);

      // The compute shader uses [numthreads(8, 8, 1)], so we need to compute how many of these groups we need to dispatch to fill the entire image.
      constexpr WUInt32 uiThreadsX = 8;
      constexpr WUInt32 uiThreadsY = 8;
      const WUInt32 uiDispatchX = (uiWidth + uiThreadsX - 1) / uiThreadsX;
      const WUInt32 uiDispatchY = (uiHeight + uiThreadsY - 1) / uiThreadsY;
      // As the image is exactly as big as one of our groups, we need to dispatch exactly one group:
      W_TEST_INT(uiDispatchX, 1);
      W_TEST_INT(uiDispatchY, 1);
      WRenderContext::GetDefaultInstance()->Dispatch(uiDispatchX, uiDispatchY, 2).AssertSuccess();
    }
    WRenderContext::GetDefaultInstance()->EndCompute();
  }
  EndCommands();


  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;

  const WMat4 mMVP = CreateSimpleMVP((float)fWidth / (float)fHeight);
  BeginCommands("Texture2D");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
    TransitionTexture(m_hTexture2D, WGALResourceState::ShaderResource, m_Texture2DRange);

    m_bCaptureImage = true;
    WRectFloat viewport = WRectFloat(0, 0, fWidth, fHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D, m_Texture2DRange);
  }
  EndCommands();
}

void WRendererTestAdvancedFeatures::MSAAResolve()
{
  // Exercises three currently-untested encoder paths in one pass:
  // 1. Rendering into MSAA color + MSAA depth-stencil targets with stencil writes / tests.
  // 2. WGALCommandEncoder::Clear inside an active render pass (color-only clear that must preserve the stencil contents).
  // 3. WGALCommandEncoder::ResolveTexture to downsample the MSAA color into a single-sample texture.
  //
  // Verification is done via readback of the resolved texture so the test does not need a reference image.

  constexpr WUInt32 uiW = 64;
  constexpr WUInt32 uiH = 64;

  WGALDepthStencilStateCreationDescription writeStencilDesc;
  writeStencilDesc.m_bDepthEnable = false;
  writeStencilDesc.m_bDepthWrite = false;
  writeStencilDesc.m_bStencilEnable = true;
  writeStencilDesc.m_uiStencilReadMask = 0xFF;
  writeStencilDesc.m_uiStencilWriteMask = 0xFF;
  writeStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Always;
  writeStencilDesc.m_FrontFaceStencilOp.m_PassOp = WGALStencilOp::Replace;
  writeStencilDesc.m_BackFaceStencilOp = writeStencilDesc.m_FrontFaceStencilOp;

  WGALDepthStencilStateCreationDescription testStencilDesc;
  testStencilDesc.m_bDepthEnable = false;
  testStencilDesc.m_bDepthWrite = false;
  testStencilDesc.m_bStencilEnable = true;
  testStencilDesc.m_uiStencilReadMask = 0xFF;
  testStencilDesc.m_uiStencilWriteMask = 0x00;
  testStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = WGALCompareFunc::Equal;
  testStencilDesc.m_BackFaceStencilOp = testStencilDesc.m_FrontFaceStencilOp;

  WGALDepthStencilStateHandle hWriteStencil = m_pDevice->CreateDepthStencilState(writeStencilDesc);
  WGALDepthStencilStateHandle hTestStencil = m_pDevice->CreateDepthStencilState(testStencilDesc);

  // Pre-multiplied identity MVP. The full-screen quad uses a 2x2 NDC rect (geom.AddRect(2x2)), so identity already covers the viewport.
  WMat4 mFull = WMat4::MakeIdentity();
  // A centered half-size quad for the stencil-write pass.
  WMat4 mCenter = WMat4::MakeScaling(WVec3(0.5f, 0.5f, 1.0f));
  if (WClipSpaceYMode::RenderToTextureDefault == WClipSpaceYMode::Flipped)
  {
    WMat4 flipY = WMat4::MakeScaling(WVec3(1.0f, -1.0f, 1.0f));
    mFull = flipY * mFull;
    mCenter = flipY * mCenter;
  }

  WRenderContext* pRenderContext = WRenderContext::GetDefaultInstance();

  BeginCommands("MSAAResolve");
  {
    TransitionTexture(m_hMSAAColor, WGALResourceState::RenderTarget);
    TransitionTexture(m_hMSAADepthStencil, WGALResourceState::DepthStencilWrite);

    WGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hMSAAColor));
    renderingSetup.SetClearColor(0, WColor::Black);
    renderingSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hMSAADepthStencil));
    renderingSetup.SetClearDepth().SetClearStencil();

    WRectFloat viewport = WRectFloat(0, 0, (float)uiW, (float)uiH);
    pRenderContext->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    // 1. Write stencil = 1 in the centered quad area. Color writes are not relevant here; we will overwrite color in step 2.
    {
      pRenderContext->SetDepthStencilState(hWriteStencil);
      pRenderContext->SetStencilRefValue(1);

      ObjectCB* ocb = WRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
      ocb->m_MVP = mCenter;
      ocb->m_Color = WColor::Black;
      WBindGroupBuilder& bg = pRenderContext->GetBindGroup();
      bg.BindBuffer("PerObject", m_hObjectTransformCB);

      pRenderContext->BindShader(m_hMSAAStencilShader, WShaderBindFlags::NoDepthStencilState);
      pRenderContext->BindMeshBuffer(m_hMSAAQuadMesh);
      pRenderContext->DrawMeshBuffer().AssertSuccess();
    }

    // 2. Clear color to red but preserve the stencil buffer. This exercises WGALCommandEncoder::Clear.
    m_pEncoder->Clear(WColor::Red, 0xFFFFFFFFu, false, false);

    // 3. Draw a full-screen quad in green where stencil == 1. Pixels outside the centered region keep their red color from the Clear.
    {
      pRenderContext->SetDepthStencilState(hTestStencil);
      pRenderContext->SetStencilRefValue(1);

      ObjectCB* ocb = WRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
      ocb->m_MVP = mFull;
      // Use linear pure green directly. WColor::Green is the HTML #008000 dark green and would readback as ~55/255 due to sRGB->linear conversion.
      ocb->m_Color = WColor(0.0f, 1.0f, 0.0f);
      WBindGroupBuilder& bg = pRenderContext->GetBindGroup();
      bg.BindBuffer("PerObject", m_hObjectTransformCB);

      pRenderContext->BindShader(m_hMSAAStencilShader, WShaderBindFlags::NoDepthStencilState);
      pRenderContext->BindMeshBuffer(m_hMSAAQuadMesh);
      pRenderContext->DrawMeshBuffer().AssertSuccess();
    }

    pRenderContext->EndRendering();

    // 4. Resolve the MSAA color into the single-sample target.
    TransitionTexture(m_hMSAAColor, WGALResourceState::ResolveSource);
    TransitionTexture(m_hMSAAResolveTarget, WGALResourceState::ResolveDestination);
    m_pEncoder->ResolveTexture(m_hMSAAResolveTarget, WGALTextureSubresource(), m_hMSAAColor, WGALTextureSubresource());

    // 5. Read the resolved texture back so we can verify pixel values.
    TransitionTexture(m_hMSAAResolveTarget, WGALResourceState::CopySource);
    m_MSAAReadback.ReadbackTexture(*m_pEncoder, m_hMSAAResolveTarget);
  }
  EndCommands();

  m_pDevice->DestroyDepthStencilState(hWriteStencil);
  m_pDevice->DestroyDepthStencilState(hTestStencil);

  WEnum<WGALAsyncResult> res = m_MSAAReadback.GetReadbackResult(WTime::MakeFromHours(1));
  if (!W_TEST_BOOL_MSG(res == WGALAsyncResult::Ready, "MSAA readback timed out"))
    return;

  WGALTextureSubresource sub;
  WArrayPtr<WGALTextureSubresource> subs(&sub, 1);
  WTempHybridArray<WGALSystemMemoryDescription, 1> memory;
  WReadbackTextureLock lock = m_MSAAReadback.LockTexture(subs, memory);
  W_ASSERT_ALWAYS(lock, "Failed to lock MSAA readback texture");

  // BGRAUByteNormalized, 4 bytes per pixel as B, G, R, A.
  auto sampleBGRA = [&](WUInt32 x, WUInt32 y) -> WColorLinearUB
  {
    const WUInt8* pRow = static_cast<const WUInt8*>(memory[0].m_pData.GetPtr()) + memory[0].m_uiRowPitch * y;
    const WUInt8* p = pRow + x * 4;
    return WColorLinearUB(p[2], p[1], p[0], p[3]);
  };

  // The center is inside the stencil-marked region, so it must be green.
  const WColorLinearUB centerPixel = sampleBGRA(uiW / 2, uiH / 2);
  W_TEST_INT(centerPixel.r, 0);
  W_TEST_BOOL_MSG(centerPixel.g > 200, "Center pixel should be green (stencil pass)");
  W_TEST_INT(centerPixel.b, 0);

  // The corner is outside the stencil-marked region, so the encoder Clear color (red) must be visible.
  const WColorLinearUB cornerPixel = sampleBGRA(1, 1);
  W_TEST_BOOL_MSG(cornerPixel.r > 200, "Corner pixel should be red (encoder Clear, stencil != 1)");
  W_TEST_INT(cornerPixel.g, 0);
  W_TEST_INT(cornerPixel.b, 0);
}

void WRendererTestAdvancedFeatures::DepthBias()
{
  // Verifies WGALRasterizerStateCreationDescription::m_iDepthBias, m_fSlopeScaledDepthBias and m_fDepthBiasClamp.
  //
  // Every case gets its own square cell of the render target. In each cell a reference quad is drawn with an unbiased rasterizer state and depth writes enabled, then the exact same quad is drawn again, moved away from the viewer by a fixed gap and rasterized with the depth bias under test. Because both quads are coplanar apart from that gap, the biased quad passes the 'Less' depth test if and only if the applied bias exceeds the gap towards the viewer. The result is therefore a binary green (biased quad won) / red (reference quad still visible) per cell, which is
  // verified via readback instead of a reference image.
  //
  // All bias magnitudes are expressed in multiples of the depth format's smallest resolvable difference, which is what m_iDepthBias is counted in. The exact value is implementation defined within a factor of two, hence the generous margins between the gap and the expected biases.

  const bool bSupportsClamp = m_pDevice->GetCapabilities().m_bSupportsDepthBiasClamp;
  if (!bSupportsClamp)
  {
    WLog::Info("The depth bias clamp is not supported by this device, skipping the cases that rely on it.");
  }

  WGALDepthStencilStateCreationDescription writeDepthDesc;
  writeDepthDesc.m_bDepthEnable = true;
  writeDepthDesc.m_bDepthWrite = true;
  writeDepthDesc.m_DepthTestFunc = WGALCompareFunc::Always;

  WGALDepthStencilStateCreationDescription testDepthDesc;
  testDepthDesc.m_bDepthEnable = true;
  testDepthDesc.m_bDepthWrite = false;
  testDepthDesc.m_DepthTestFunc = WGALCompareFunc::Less;

  WGALDepthStencilStateHandle hWriteDepth = m_pDevice->CreateDepthStencilState(writeDepthDesc);
  WGALDepthStencilStateHandle hTestDepth = m_pDevice->CreateDepthStencilState(testDepthDesc);

  WGALRasterizerStateCreationDescription rasterDesc;
  rasterDesc.m_CullMode = WGALCullMode::None;
  WGALRasterizerStateHandle hNoBias = m_pDevice->CreateRasterizerState(rasterDesc);

  WHybridArray<WGALRasterizerStateHandle, s_uiDepthBiasCaseCount> biasStates;
  for (const DepthBiasCase& testCase : s_DepthBiasCases)
  {
    WGALRasterizerStateCreationDescription desc;
    desc.m_CullMode = WGALCullMode::None;
    desc.m_iDepthBias = testCase.m_iDepthBias;
    desc.m_fSlopeScaledDepthBias = testCase.m_fSlopeScaledDepthBias;
    desc.m_fDepthBiasClamp = testCase.m_fClampUnits * m_fDepthBiasUnit;
    biasStates.PushBack(m_pDevice->CreateRasterizerState(desc));
  }

  // Maps the quad's local NDC space onto the cell of the given case and places it at s_fDepthBiasBaseDepth, optionally tilted along x.
  auto MakeTransform = [](WUInt32 uiCase, bool bSloped, float fDepthOffset) -> WMat4
  {
    const float fScaleX = 1.0f / s_uiDepthBiasCaseCount;
    const float fCenterX = -1.0f + (2.0f * uiCase + 1.0f) * fScaleX;

    WMat4 m = WMat4::MakeIdentity();
    m.SetRow(0, WVec4(fScaleX, 0.0f, 0.0f, fCenterX));
    m.SetRow(2, WVec4(bSloped ? s_fDepthBiasSlope : 0.0f, 0.0f, 1.0f, s_fDepthBiasBaseDepth + fDepthOffset));
    return m;
  };

  WRenderContext* pRenderContext = WRenderContext::GetDefaultInstance();

  auto DrawQuad = [&](const WMat4& mTransform, const WColor& color, WGALDepthStencilStateHandle hDepthStencil, WGALRasterizerStateHandle hRasterizer)
  {
    pRenderContext->SetDepthStencilState(hDepthStencil);
    pRenderContext->SetRasterizerState(hRasterizer);

    ObjectCB* ocb = WRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP = mTransform;
    ocb->m_Color = color;
    pRenderContext->GetBindGroup().BindBuffer("PerObject", m_hObjectTransformCB);

    pRenderContext->BindShader(m_hDepthBiasShader, WShaderBindFlags::NoRasterizerState | WShaderBindFlags::NoDepthStencilState);
    pRenderContext->BindMeshBuffer(m_hDepthBiasQuadMesh);
    pRenderContext->DrawMeshBuffer().AssertSuccess();
  };

  BeginCommands("DepthBias");
  {
    TransitionTexture(m_hDepthBiasColor, WGALResourceState::RenderTarget);
    TransitionTexture(m_hDepthBiasDepth, WGALResourceState::DepthStencilWrite);

    WGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hDepthBiasColor));
    renderingSetup.SetClearColor(0, WColor::Black);
    renderingSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthBiasDepth));
    renderingSetup.SetClearDepth();

    WRectFloat viewport = WRectFloat(0, 0, (float)(s_uiDepthBiasCellSize * s_uiDepthBiasCaseCount), (float)s_uiDepthBiasCellSize);
    pRenderContext->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    for (WUInt32 i = 0; i < s_uiDepthBiasCaseCount; ++i)
    {
      const DepthBiasCase& testCase = s_DepthBiasCases[i];
      DrawQuad(MakeTransform(i, testCase.m_bSloped, 0.0f), WColor(1.0f, 0.0f, 0.0f), hWriteDepth, hNoBias);
      DrawQuad(MakeTransform(i, testCase.m_bSloped, s_fDepthBiasGapUnits * m_fDepthBiasUnit), WColor(0.0f, 1.0f, 0.0f), hTestDepth, biasStates[i]);
    }

    pRenderContext->EndRendering();

    TransitionTexture(m_hDepthBiasColor, WGALResourceState::CopySource);
    m_DepthBiasReadback.ReadbackTexture(*m_pEncoder, m_hDepthBiasColor);
  }
  EndCommands();

  m_pDevice->DestroyDepthStencilState(hWriteDepth);
  m_pDevice->DestroyDepthStencilState(hTestDepth);
  m_pDevice->DestroyRasterizerState(hNoBias);
  for (WGALRasterizerStateHandle hState : biasStates)
  {
    m_pDevice->DestroyRasterizerState(hState);
  }

  WEnum<WGALAsyncResult> res = m_DepthBiasReadback.GetReadbackResult(WTime::MakeFromHours(1));
  if (!W_TEST_BOOL_MSG(res == WGALAsyncResult::Ready, "Depth bias readback timed out"))
    return;

  WGALTextureSubresource sub;
  WArrayPtr<WGALTextureSubresource> subs(&sub, 1);
  WTempHybridArray<WGALSystemMemoryDescription, 1> memory;
  WReadbackTextureLock lock = m_DepthBiasReadback.LockTexture(subs, memory);
  W_ASSERT_ALWAYS(lock, "Failed to lock depth bias readback texture");

  // BGRAUByteNormalized, 4 bytes per pixel as B, G, R, A.
  auto SampleBGRA = [&](WUInt32 x, WUInt32 y) -> WColorLinearUB
  {
    const WUInt8* pRow = static_cast<const WUInt8*>(memory[0].m_pData.GetPtr()) + memory[0].m_uiRowPitch * y;
    const WUInt8* p = pRow + x * 4;
    return WColorLinearUB(p[2], p[1], p[0], p[3]);
  };

  for (WUInt32 i = 0; i < s_uiDepthBiasCaseCount; ++i)
  {
    const DepthBiasCase& testCase = s_DepthBiasCases[i];
    if (testCase.m_fClampUnits != 0.0f && !bSupportsClamp)
      continue;

    const WColorLinearUB pixel = SampleBGRA(i * s_uiDepthBiasCellSize + s_uiDepthBiasCellSize / 2, s_uiDepthBiasCellSize / 2);
    const bool bVisible = pixel.g > 200 && pixel.r < 55;
    const bool bHidden = pixel.r > 200 && pixel.g < 55;

    if (!W_TEST_BOOL_MSG(bVisible || bHidden, "'%s': neither quad is clearly visible, got RGB (%d, %d, %d)", testCase.m_szName, (int)pixel.r, (int)pixel.g, (int)pixel.b))
      continue;

    W_TEST_BOOL_MSG(bVisible == testCase.m_bExpectVisible, "'%s': the biased quad is %s but was expected to be %s", testCase.m_szName, bVisible ? "visible" : "hidden", testCase.m_bExpectVisible ? "visible" : "hidden");
  }
}

void WRendererTestAdvancedFeatures::ConservativeRasterization()
{
  // Verifies WGALRasterizerStateCreationDescription::m_bConservativeRasterization.
  //
  // Every case gets its own square cell of the render target and draws a single white quad into it. Standard rasterization only produces a fragment when the pixel center lies inside the primitive, conservative (overestimated) rasterization produces one for every pixel the primitive touches at all. Feeding a quad that is smaller than a pixel and placed so that it misses that pixel's center therefore yields nothing without the feature and at least one lit pixel with it. The two remaining cells draw a quad that covers whole pixels to confirm that ordinary geometry is unaffected.
  //
  // The number of lit pixels per cell is verified via readback instead of a reference image.

  constexpr WUInt32 uiWidth = s_uiConservativeRasterCellSize * s_uiConservativeRasterCaseCount;
  constexpr WUInt32 uiHeight = s_uiConservativeRasterCellSize;

  // Maps the quad's local NDC space onto the given window space rect of the render target.
  auto MakeTransform = [](float fMinX, float fMinY, float fMaxX, float fMaxY) -> WMat4
  {
    WMat4 m = WMat4::MakeIdentity();
    m.SetRow(0, WVec4((fMaxX - fMinX) / uiWidth, 0.0f, 0.0f, (fMinX + fMaxX) / uiWidth - 1.0f));
    m.SetRow(1, WVec4(0.0f, (fMaxY - fMinY) / uiHeight, 0.0f, (fMinY + fMaxY) / uiHeight - 1.0f));
    return m;
  };

  WRenderContext* pRenderContext = WRenderContext::GetDefaultInstance();

  WHybridArray<WGALRasterizerStateHandle, s_uiConservativeRasterCaseCount> rasterStates;
  for (const ConservativeRasterCase& testCase : s_ConservativeRasterCases)
  {
    WGALRasterizerStateCreationDescription desc;
    desc.m_CullMode = WGALCullMode::None;
    desc.m_bConservativeRasterization = testCase.m_bConservative;
    WGALRasterizerStateHandle hState = m_pDevice->CreateRasterizerState(desc);
    if (!W_TEST_BOOL_MSG(!hState.IsInvalidated(), "'%s': failed to create the rasterizer state", testCase.m_szName))
      return;

    rasterStates.PushBack(hState);
  }

  BeginCommands("ConservativeRasterization");
  {
    TransitionTexture(m_hConservativeRasterColor, WGALResourceState::RenderTarget);

    WGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hConservativeRasterColor));
    renderingSetup.SetClearColor(0, WColor::Black);

    WRectFloat viewport = WRectFloat(0, 0, (float)uiWidth, (float)uiHeight);
    pRenderContext->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    for (WUInt32 i = 0; i < s_uiConservativeRasterCaseCount; ++i)
    {
      const ConservativeRasterCase& testCase = s_ConservativeRasterCases[i];
      const float fCellOffset = (float)(i * s_uiConservativeRasterCellSize);
      const float fMin = testCase.m_bSubPixel ? s_fConservativeRasterSubPixelMin : s_fConservativeRasterCoveringMin;
      const float fMax = testCase.m_bSubPixel ? s_fConservativeRasterSubPixelMax : s_fConservativeRasterCoveringMax;

      pRenderContext->SetRasterizerState(rasterStates[i]);

      ObjectCB* ocb = WRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
      ocb->m_MVP = MakeTransform(fCellOffset + fMin, fMin, fCellOffset + fMax, fMax);
      ocb->m_Color = WColor::White;
      pRenderContext->GetBindGroup().BindBuffer("PerObject", m_hObjectTransformCB);

      pRenderContext->BindShader(m_hConservativeRasterShader, WShaderBindFlags::NoRasterizerState);
      pRenderContext->BindMeshBuffer(m_hConservativeRasterQuadMesh);
      pRenderContext->DrawMeshBuffer().AssertSuccess();
    }

    pRenderContext->EndRendering();
    TransitionTexture(m_hConservativeRasterColor, WGALResourceState::CopySource);
    m_ConservativeRasterReadback.ReadbackTexture(*m_pEncoder, m_hConservativeRasterColor);
  }
  EndCommands();

  for (WGALRasterizerStateHandle hState : rasterStates)
  {
    m_pDevice->DestroyRasterizerState(hState);
  }

  WEnum<WGALAsyncResult> res = m_ConservativeRasterReadback.GetReadbackResult(WTime::MakeFromHours(1));
  if (!W_TEST_BOOL_MSG(res == WGALAsyncResult::Ready, "Conservative rasterization readback timed out"))
    return;

  WGALTextureSubresource sub;
  WArrayPtr<WGALTextureSubresource> subs(&sub, 1);
  WTempHybridArray<WGALSystemMemoryDescription, 1> memory;
  WReadbackTextureLock lock = m_ConservativeRasterReadback.LockTexture(subs, memory);
  W_ASSERT_ALWAYS(lock, "Failed to lock conservative rasterization readback texture");

  for (WUInt32 i = 0; i < s_uiConservativeRasterCaseCount; ++i)
  {
    const ConservativeRasterCase& testCase = s_ConservativeRasterCases[i];

    WUInt32 uiLitPixels = 0;
    for (WUInt32 y = 0; y < s_uiConservativeRasterCellSize; ++y)
    {
      // BGRAUByteNormalized, 4 bytes per pixel as B, G, R, A.
      const WUInt8* pRow = static_cast<const WUInt8*>(memory[0].m_pData.GetPtr()) + memory[0].m_uiRowPitch * y;
      for (WUInt32 x = 0; x < s_uiConservativeRasterCellSize; ++x)
      {
        if (pRow[(i * s_uiConservativeRasterCellSize + x) * 4] > 128)
          ++uiLitPixels;
      }
    }

    W_TEST_BOOL_MSG(uiLitPixels >= testCase.m_uiMinLitPixels && uiLitPixels <= testCase.m_uiMaxLitPixels, "'%s': %d pixels are lit, expected between %d and %d", testCase.m_szName, (int)uiLitPixels, (int)testCase.m_uiMinLitPixels, (int)testCase.m_uiMaxLitPixels);
  }
}

WTestAppRun WRendererTestAdvancedFeatures::Material()
{
  {
    WResourceLock<WMaterialResource> pMaterial(m_hMaterial, WResourceAcquireMode::BlockTillLoaded);
    const WMaterialResourceDescriptor& desc = pMaterial->GetCurrentDesc();
    W_TEST_INT(desc.m_PermutationVars.GetCount(), 0);
    W_TEST_INT(desc.m_Parameters.GetCount(), 2);
    W_TEST_INT(desc.m_Texture2DBindings.GetCount(), 1);
    W_TEST_INT(desc.m_TextureCubeBindings.GetCount(), 0);
    WVariant color1 = pMaterial->GetParameter(m_sBaseColor);
    WVariant color2 = pMaterial->GetParameter(m_sBaseColor2);
    WTexture2DResourceHandle hTexture = pMaterial->GetTexture2DBinding(m_sTexture);

    if (m_iFrame == ImageCaptureFrames::Material_ColorChange)
    {
      W_TEST_BOOL(color1.IsA<WColor>() && color1.Get<WColor>() == WColor::White);
      W_TEST_BOOL(color2.IsA<WColor>() && color2.Get<WColor>() == WColor::White);
      W_TEST_BOOL(hTexture == m_hTexture);
      pMaterial->SetParameter(m_sBaseColor, WColor::Yellow);
    }
    else if (m_iFrame == ImageCaptureFrames::Material_ColorChange2)
    {
      W_TEST_BOOL(color1.IsA<WColor>() && color1.Get<WColor>() == WColor::Yellow);
      W_TEST_BOOL(color2.IsA<WColor>() && color2.Get<WColor>() == WColor::White);
      W_TEST_BOOL(hTexture == m_hTexture);
      pMaterial->SetParameter(m_sBaseColor2, WColor::Cyan);
    }
    else if (m_iFrame == ImageCaptureFrames::Material_ChangeTexture)
    {
      W_TEST_BOOL(color1.IsA<WColor>() && color1.Get<WColor>() == WColor::Yellow);
      W_TEST_BOOL(color2.IsA<WColor>() && color2.Get<WColor>() == WColor::Cyan);
      W_TEST_BOOL(hTexture == m_hTexture);
      pMaterial->SetTexture2DBinding(m_sTexture, m_hTexture2);
    }
  }

  BeginFrame();
  {
    const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
    const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
    const WMat4 mMVP = CreateSimpleMVP((float)fWidth / (float)fHeight);
    BeginCommands("MaterialTest");
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
      WRectFloat viewport = WRectFloat(0, 0, fWidth, fHeight);
      WGALCommandEncoder* pCommandEncoder = BeginRendering(WColor::RebeccaPurple, 0xFFFFFFFF, &viewport);

      WRenderContext* pContext = WRenderContext::GetDefaultInstance();
      pContext->SetAllowAsyncShaderLoading(false);
      pContext->BindMaterial(m_hMaterial);

      ObjectCB* ocb = WRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
      ocb->m_MVP = mMVP;
      ocb->m_Color = WColor(1, 1, 1, 1);

      WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroupTest.BindBuffer("PerObject", m_hObjectTransformCB);

      WRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hCubeUV);
      WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

      EndRendering();
      if (m_ImgCompFrames.Contains(m_iFrame))
      {
        TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
        W_TEST_IMAGE(m_iFrame, 100);
      }
    }
    EndCommands();
  }
  EndFrame();

  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return WTestAppRun::Quit;
  }
  return WTestAppRun::Continue;
}

#if W_ENABLED(W_SUPPORTS_PROCESSES)
WTestAppRun WRendererTestAdvancedFeatures::SharedTexture()
{
  if (m_pOffscreenProcess->GetState() != WProcessState::Running)
  {
    W_TEST_BOOL(m_bExiting);
    return WTestAppRun::Quit;
  }

  m_pProtocol->WaitForMessages(WTime::MakeFromMilliseconds(16)).IgnoreResult();

  WOffscreenTest_SharedTexture texture = m_SharedTextureQueue.PeekFront();
  m_SharedTextureQueue.PopFront();

  WStringBuilder sTemp;
  sTemp.SetFormat("Render {}:{}|{}", m_uiReceivedTextures, texture.m_uiCurrentTextureIndex, texture.m_uiCurrentSemaphoreValue);
  W_PROFILE_SCOPE(sTemp);
  BeginFrame();
  {
    const WGALSharedTexture* pSharedTexture = m_pDevice->GetSharedTexture(m_hSharedTextures[texture.m_uiCurrentTextureIndex]);
    W_ASSERT_DEV(pSharedTexture != nullptr, "Shared texture did not resolve");

    pSharedTexture->WaitSemaphoreGPU(texture.m_uiCurrentSemaphoreValue);

    const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
    const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
    const WUInt32 uiColumns = 1;
    const WUInt32 uiRows = 1;
    const float fElementWidth = fWidth / uiColumns;
    const float fElementHeight = fHeight / uiRows;

    const WMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
    BeginCommands("Texture2D");
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
      TransitionTexture(m_hSharedTextures[texture.m_uiCurrentTextureIndex], WGALResourceState::ShaderResource);

      WRectFloat viewport = WRectFloat(0, 0, fElementWidth, fElementHeight);
      m_bCaptureImage = true;
      viewport = WRectFloat(0, 0, fElementWidth, fElementHeight);

      WGALCommandEncoder* pCommandEncoder = BeginRendering(WColor::RebeccaPurple, 0xFFFFFFFF, &viewport);

      WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroupTest.BindTexture("DiffuseTexture", m_hSharedTextures[texture.m_uiCurrentTextureIndex]);
      RenderObject(m_hCubeUV, mMVP, WColor(1, 1, 1, 1), WShaderBindFlags::None);

      EndRendering();
      if (!m_bExiting && m_uiReceivedTextures > 10)
      {
        TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
        W_TEST_IMAGE(0, 10);

        WOffscreenTest_CloseMsg msg;
        W_TEST_BOOL(m_pProtocol->Send(&msg));
        m_bExiting = true;
      }
    }
    EndCommands();

    texture.m_uiCurrentSemaphoreValue++;
    pSharedTexture->SignalSemaphoreGPU(texture.m_uiCurrentSemaphoreValue);
  }
  EndFrame();

  if (m_SharedTextureQueue.IsEmpty() || !m_pChannel->IsConnected())
  {
    m_SharedTextureQueue.PushBack(texture);
  }
  else if (!m_bExiting)
  {
    WOffscreenTest_RenderMsg msg;
    msg.m_Texture = texture;
    W_TEST_BOOL(m_pProtocol->Send(&msg));
  }

  return WTestAppRun::Continue;
}

void WRendererTestAdvancedFeatures::OffscreenProcessMessageFunc(const WIpcProcessMessageProtocol::Event& msg)
{
  if (const auto* pAction = WDynamicCast<const WOffscreenTest_RenderResponseMsg*>(msg.m_pMessage))
  {
    m_uiReceivedTextures++;
    WStringBuilder sTemp;
    sTemp.SetFormat("Receive {}|{}", pAction->m_Texture.m_uiCurrentTextureIndex, pAction->m_Texture.m_uiCurrentSemaphoreValue);
    W_PROFILE_SCOPE(sTemp);
    m_SharedTextureQueue.PushBack(pAction->m_Texture);
  }
}
#endif

static WRendererTestAdvancedFeatures g_AdvancedFeaturesTest;
