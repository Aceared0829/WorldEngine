#include <RendererTest/RendererTestPCH.h>

#include <RendererTest/Basics/PipelineStates.h>

#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderGraph/RenderGraphUtils.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/ColorScheme.h>
#include <RendererTest/Basics/RendererTestUtils.h>

#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestConstants.h>
#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestPushConstants.h>


void WRendererTestPipelineStates::SetupSubTests()
{
  const WGALDeviceCapabilities& caps = GetDeviceCapabilities();

  AddSubTest("01 - MostBasicShader", SubTests::ST_MostBasicShader);
  AddSubTest("02 - ViewportScissor", SubTests::ST_ViewportScissor);
  AddSubTest("03 - VertexBuffer", SubTests::ST_VertexBuffer);
  AddSubTest("04 - IndexBuffer", SubTests::ST_IndexBuffer);
  AddSubTest("05 - ConstantBuffer", SubTests::ST_ConstantBuffer);
  AddSubTest("06 - StructuredBuffer", SubTests::ST_StructuredBuffer);
  if (caps.m_bSupportsTexelBuffer)
  {
    AddSubTest("06b - TexelBuffer", SubTests::ST_TexelBuffer);
  }
  AddSubTest("06c - ByteAddressBuffer", SubTests::ST_ByteAddressBuffer);
  AddSubTest("07 - Texture2D", SubTests::ST_Texture2D);
  AddSubTest("08 - Texture2DArray", SubTests::ST_Texture2DArray);
  AddSubTest("09 - GenerateMipMaps", SubTests::ST_GenerateMipMaps);
  AddSubTest("10 - PushConstants", SubTests::ST_PushConstants);
  AddSubTest("11 - BindGroups", SubTests::ST_BindGroups);
  AddSubTest("12 - Timestamps", SubTests::ST_Timestamps); // Disabled due to CI failure on AMD.
  AddSubTest("13 - OcclusionQueries", SubTests::ST_OcclusionQueries);
  AddSubTest("14 - CustomVertexStreams", SubTests::ST_CustomVertexStreams);
}

WResult WRendererTestPipelineStates::InitializeTest()
{
  // Initialize core systems and renderer once for all sub-tests
  WStartup::StartupCoreSystems();

  if (SetupRenderer().Failed())
    return W_FAILURE;

  // Create window once for all sub-tests
  W_SUCCEED_OR_RETURN(CreateWindow(320, 240));

  // Load shaders that are used across multiple sub-tests
  m_hMostBasicTriangleShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/MostBasicTriangle.WShader");
  m_hNDCPositionOnlyShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/NDCPositionOnly.WShader");
  m_hConstantBufferShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/ConstantBuffer.WShader");
  m_hPushConstantsShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/PushConstants.WShader");
  m_hCustomVertexStreamShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/CustomVertexStreams.WShader");

  // Create meshes that are used across multiple sub-tests
  {
    WMeshBufferResourceDescriptor desc;
    desc.AddStream(WMeshVertexStreamType::Position);
    desc.AllocateStreams(3);

    if (WClipSpaceYMode::RenderToTextureDefault == WClipSpaceYMode::Flipped)
    {
      desc.SetPosition(0, WVec3(1.f, 1.f, 0.0f));
      desc.SetPosition(1, WVec3(-1.f, 1.f, 0.0f));
      desc.SetPosition(2, WVec3(0.f, -1.f, 0.0f));
    }
    else
    {
      desc.SetPosition(0, WVec3(1.f, -1.f, 0.0f));
      desc.SetPosition(1, WVec3(-1.f, -1.f, 0.0f));
      desc.SetPosition(2, WVec3(0.f, 1.f, 0.0f));
    }

    m_hTriangleMesh = WResourceManager::CreateResource<WMeshBufferResource>("UnitTest-TriangleMesh", std::move(desc), "TriangleMesh");
  }
  {
    WGeometry geom;
    geom.AddStackedSphere(0.5f, 16, 16);

    WMeshBufferResourceDescriptor desc;
    desc.AddStream(WMeshVertexStreamType::Position);
    desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

    m_hSphereMesh = WResourceManager::CreateResource<WMeshBufferResource>("UnitTest-SphereMesh", std::move(desc), "SphereMesh");
  }

  // Create constant buffers that are used across multiple sub-tests
  m_hTestPerFrameConstantBuffer = WRenderContext::CreateConstantBufferStorage<WTestPerFrame>();
  m_hTestColorsConstantBuffer = WRenderContext::CreateConstantBufferStorage<WTestColors>();
  m_hTestPositionsConstantBuffer = WRenderContext::CreateConstantBufferStorage<WTestPositions>();

  return W_SUCCESS;
}

WResult WRendererTestPipelineStates::DeInitializeTest()
{
  // Clean up resources created in InitializeTest
  m_hTriangleMesh.Invalidate();
  m_hSphereMesh.Invalidate();

  m_hMostBasicTriangleShader.Invalidate();
  m_hNDCPositionOnlyShader.Invalidate();
  m_hConstantBufferShader.Invalidate();
  m_hPushConstantsShader.Invalidate();
  m_hCustomVertexStreamShader.Invalidate();

  m_hTestPerFrameConstantBuffer.Invalidate();
  m_hTestColorsConstantBuffer.Invalidate();
  m_hTestPositionsConstantBuffer.Invalidate();

  // Destroy window once after all sub-tests
  DestroyWindow();

  // Shut down renderer and core systems once after all sub-tests
  ShutdownRenderer();
  WStartup::ShutdownCoreSystems();
  WMemoryTracker::DumpMemoryLeaks();

  return W_SUCCESS;
}

WResult WRendererTestPipelineStates::InitializeSubTest(WInt32 iIdentifier)
{
  // Reset per-sub-test state
  m_iFrame = -1;
  m_bCaptureImage = false;
  m_ImgCompFrames.Clear();

  {
    m_iDelay = 0;
    m_CPUTime[0] = {};
    m_CPUTime[1] = {};
    m_GPUTime[0] = {};
    m_GPUTime[1] = {};
    m_timestamps[0] = {};
    m_timestamps[1] = {};
    m_queries[0] = {};
    m_queries[1] = {};
    m_queries[2] = {};
    m_queries[3] = {};
    m_hFence = {};
  }

  if (iIdentifier == SubTests::ST_StructuredBuffer || iIdentifier == SubTests::ST_TexelBuffer || iIdentifier == SubTests::ST_ByteAddressBuffer)
  {
    WGALBufferCreationDescription desc;
    WGALShaderResourceType::Enum slotType;
    desc.m_uiTotalSize = 16 * sizeof(WTestShaderData);
    desc.m_ResourceAccess.m_bImmutable = false;
    switch (iIdentifier)
    {
      case SubTests::ST_StructuredBuffer:
        desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
        desc.m_uiStructSize = sizeof(WTestShaderData);
        m_hInstancingShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/InstancingStructuredBuffer.WShader");
        m_hCopyBufferShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/CopyStructuredBuffer.WShader");
        slotType = WGALShaderResourceType::StructuredBuffer;
        break;
      case SubTests::ST_TexelBuffer:
        desc.m_Format = WGALResourceFormat::RGBAFloat;
        desc.m_BufferFlags = WGALBufferUsageFlags::TexelBuffer | WGALBufferUsageFlags::ShaderResource;
        m_hInstancingShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/InstancingTexelBuffer.WShader");
        m_hCopyBufferShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/CopyTexelBuffer.WShader");
        slotType = WGALShaderResourceType::TexelBuffer;
        break;
      default:
      case SubTests::ST_ByteAddressBuffer:
        desc.m_BufferFlags = WGALBufferUsageFlags::ByteAddressBuffer | WGALBufferUsageFlags::ShaderResource;
        m_hInstancingShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/InstancingByteAddressBuffer.WShader");
        m_hCopyBufferShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/CopyByteAddressBuffer.WShader");
        slotType = WGALShaderResourceType::ByteAddressBuffer;
        break;
    }

    // We only fill the first 8 elements with data. The rest is dynamically updated during testing.
    WTempHybridArray<WTestShaderData, 16> instanceData;
    WRendererTestUtils::FillStructuredBuffer(instanceData);
    m_hInstancingData = m_pDevice->CreateBuffer(desc, instanceData.GetByteArrayPtr());

    // Create another transient variant of the buffer. If supported, this will extend support to all SRV types.
    WGALBufferCreationDescription transientDesc = desc;
    transientDesc.m_BufferFlags |= WGALBufferUsageFlags::Transient;
    if (m_pDevice->GetCapabilities().m_bSupportsMultipleSRVTypes)
    {
      transientDesc.m_BufferFlags |= WGALBufferUsageFlags::StructuredBuffer;
      transientDesc.m_uiStructSize = sizeof(WTestShaderData);
      transientDesc.m_BufferFlags |= WGALBufferUsageFlags::ByteAddressBuffer;
      if (m_pDevice->GetCapabilities().m_bSupportsTexelBuffer)
      {
        transientDesc.m_BufferFlags |= WGALBufferUsageFlags::TexelBuffer;
        transientDesc.m_Format = WGALResourceFormat::RGBAFloat;
      }
    }
    m_hInstancingDataTransient = m_pDevice->CreateBuffer(transientDesc);

    // UAV variant
    WGALBufferCreationDescription uavDesc = desc;
    uavDesc.m_uiTotalSize = 8 * sizeof(WTestShaderData);
    uavDesc.m_BufferFlags |= WGALBufferUsageFlags::UnorderedAccess;
    m_hInstancingDataUAV = m_pDevice->CreateBuffer(uavDesc);
  }


  if (iIdentifier == SubTests::ST_CustomVertexStreams)
  {
    // Same as ST_StructuredBuffer, but we put the data in a vertex buffer.
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(WTestShaderData);
    desc.m_uiTotalSize = 16 * desc.m_uiStructSize;
    desc.m_BufferFlags = WGALBufferUsageFlags::VertexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    WTempHybridArray<WTestShaderData, 16> instanceData;
    WRendererTestUtils::FillStructuredBuffer(instanceData);
    m_hInstancingDataVertexStream = m_pDevice->CreateBuffer(desc, instanceData.GetByteArrayPtr());

    {
      WResourceLock<WMeshBufferResource> pMeshBuffer(m_hTriangleMesh, WResourceAcquireMode::BlockTillLoaded);
      m_VertexAttributes = pMeshBuffer->GetVertexAttributes();
    }

    auto& color = m_VertexAttributes.ExpandAndGetRef();
    color.m_eSemantic = WGALVertexAttributeSemantic::Color4;
    color.m_eFormat = WGALResourceFormat::XYZWFloat;
    color.m_uiOffset = sizeof(WVec4) * 0;
    color.m_uiVertexBufferSlot = 5;

    auto& r0 = m_VertexAttributes.ExpandAndGetRef();
    r0.m_eSemantic = WGALVertexAttributeSemantic::Color5;
    r0.m_eFormat = WGALResourceFormat::XYZWFloat;
    r0.m_uiOffset = sizeof(WVec4) * 1;
    r0.m_uiVertexBufferSlot = 5;

    auto& r1 = m_VertexAttributes.ExpandAndGetRef();
    r1.m_eSemantic = WGALVertexAttributeSemantic::Color6;
    r1.m_eFormat = WGALResourceFormat::XYZWFloat;
    r1.m_uiOffset = sizeof(WVec4) * 2;
    r1.m_uiVertexBufferSlot = 5;

    auto& r2 = m_VertexAttributes.ExpandAndGetRef();
    r2.m_eSemantic = WGALVertexAttributeSemantic::Color7;
    r2.m_eFormat = WGALResourceFormat::XYZWFloat;
    r2.m_uiOffset = sizeof(WVec4) * 3;
    r2.m_uiVertexBufferSlot = 5;
  }

  {
    // Texture2D
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = 8;
    desc.m_uiHeight = 8;
    desc.m_uiMipLevelCount = 4;
    desc.m_Format = WGALResourceFormat::BGRAUByteNormalizedsRGB;

    WImage coloredMips;
    WRendererTestUtils::CreateImage(coloredMips, desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, true);

    if (iIdentifier == SubTests::ST_GenerateMipMaps)
    {
      // Clear all mips except the fist one and let them be regenerated.
      desc.m_ResourceAccess.m_bImmutable = false;
      desc.m_TextureFlags.Add(WGALTextureUsageFlags::RenderTarget);
      for (WUInt32 m = 1; m < desc.m_uiMipLevelCount; m++)
      {
        const WUInt32 uiHeight = coloredMips.GetHeight(m);
        const WUInt32 uiWidth = coloredMips.GetWidth(m);
        for (WUInt32 y = 0; y < uiHeight; y++)
        {
          for (WUInt32 x = 0; x < uiWidth; x++)
          {
            WRendererTestUtils::ImgColor* pColor = coloredMips.GetPixelPointer<WRendererTestUtils::ImgColor>(m, 0u, 0u, x, y);
            pColor->a = 255;
            pColor->b = 0;
            pColor->g = 0;
            pColor->r = 0;
          }
        }
      }
    }

    WTempHybridArray<WGALSystemMemoryDescription, 4> initialData;
    initialData.SetCount(desc.m_uiMipLevelCount);
    for (WUInt32 m = 0; m < desc.m_uiMipLevelCount; m++)
    {
      WGALSystemMemoryDescription& memoryDesc = initialData[m];
      memoryDesc.m_pData = coloredMips.GetSubImageView(m).GetByteBlobPtr();
      memoryDesc.m_uiRowPitch = static_cast<WUInt32>(coloredMips.GetRowPitch(m));
      memoryDesc.m_uiSlicePitch = static_cast<WUInt32>(coloredMips.GetDepthPitch(m));
    }
    m_hTexture2D = m_pDevice->CreateTexture(desc, initialData);
  }

  {
    // Texture2DArray
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = 8;
    desc.m_uiHeight = 8;
    desc.m_uiMipLevelCount = 4;
    desc.m_uiArraySize = 2;
    desc.m_Type = WGALTextureType::Texture2DArray;
    desc.m_Format = WGALResourceFormat::BGRAUByteNormalizedsRGB;

    WImage coloredMips[2];
    WRendererTestUtils::CreateImage(coloredMips[0], desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, false, 0);
    WRendererTestUtils::CreateImage(coloredMips[1], desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, false, 255);

    WTempHybridArray<WGALSystemMemoryDescription, 8> initialData;
    initialData.SetCount(desc.m_uiArraySize * desc.m_uiMipLevelCount);
    for (WUInt32 l = 0; l < desc.m_uiArraySize; l++)
    {
      for (WUInt32 m = 0; m < desc.m_uiMipLevelCount; m++)
      {
        WGALSystemMemoryDescription& memoryDesc = initialData[m + l * desc.m_uiMipLevelCount];

        memoryDesc.m_pData = coloredMips[l].GetSubImageView(m).GetByteBlobPtr();
        memoryDesc.m_uiRowPitch = static_cast<WUInt32>(coloredMips[l].GetRowPitch(m));
        memoryDesc.m_uiSlicePitch = static_cast<WUInt32>(coloredMips[l].GetDepthPitch(m));
      }
    }
    m_hTexture2DArray = m_pDevice->CreateTexture(desc, initialData);
  }

  switch (iIdentifier)
  {
    case SubTests::ST_MostBasicShader:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_ViewportScissor:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_VertexBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_IndexBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_ConstantBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_StructuredBuffer:
    case SubTests::ST_TexelBuffer:
    case SubTests::ST_ByteAddressBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_InitialData);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_UpdateForNextFrame);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_UpdateForNextFrame2);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_Transient1);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_Transient2);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_UAV);
      break;
    case SubTests::ST_GenerateMipMaps:
    case SubTests::ST_Texture2D:
    {
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Texture2D.WShader");
    }
    break;
    case SubTests::ST_Texture2DArray:
    {
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Texture2DArray.WShader");
    }
    break;
    case SubTests::ST_PushConstants:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_BindGroups:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/BindGroups.WShader");
      break;
    case SubTests::ST_Timestamps:
    case SubTests::ST_OcclusionQueries:
      break;
    case SubTests::ST_CustomVertexStreams:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::CustomVertexStreams_Offsets);
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return W_SUCCESS;
}

WResult WRendererTestPipelineStates::DeInitializeSubTest(WInt32 iIdentifier)
{
  // Clean up per-sub-test resources (shaders and meshes shared across sub-tests are cleaned up in DeInitializeTest)
  m_hInstancingShader.Invalidate();
  m_hCopyBufferShader.Invalidate();

  if (!m_hInstancingData.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hInstancingData);
    m_hInstancingData.Invalidate();
  }
  if (!m_hInstancingDataTransient.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hInstancingDataTransient);
    m_hInstancingDataTransient.Invalidate();
  }
  if (!m_hInstancingDataUAV.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hInstancingDataUAV);
    m_hInstancingDataUAV.Invalidate();
  }

  if (!m_hInstancingDataVertexStream.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hInstancingDataVertexStream);
    m_hInstancingDataVertexStream.Invalidate();
  }
  m_VertexAttributes.Clear();

  if (!m_hTexture2D.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2D);
    m_hTexture2D.Invalidate();
  }
  if (!m_hTexture2DArray.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2DArray);
    m_hTexture2DArray.Invalidate();
  }
  m_hShader.Invalidate();

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_queries); i++)
  {
    m_queries[i] = {};
  }
  m_hFence = {};

  // Don't call parent's DeInitializeSubTest - renderer shutdown happens in DeInitializeTest

  return W_SUCCESS;
}

WTestAppRun WRendererTestPipelineStates::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;

  if (iIdentifier == SubTests::ST_StructuredBuffer || iIdentifier == SubTests::ST_TexelBuffer || iIdentifier == SubTests::ST_ByteAddressBuffer)
  {
    StructuredBufferTestUpload();
  }

  BeginFrame();

  switch (iIdentifier)
  {
    case SubTests::ST_MostBasicShader:
      MostBasicTriangleTest();
      break;
    case SubTests::ST_ViewportScissor:
      ViewportScissorTest();
      break;
    case SubTests::ST_VertexBuffer:
      VertexBufferTest();
      break;
    case SubTests::ST_IndexBuffer:
      IndexBufferTest();
      break;
    case SubTests::ST_ConstantBuffer:
      ConstantBufferTest();
      break;
    case SubTests::ST_StructuredBuffer:
      StructuredBufferTest(WGALShaderResourceType::StructuredBuffer);
      break;
    case SubTests::ST_TexelBuffer:
      StructuredBufferTest(WGALShaderResourceType::TexelBuffer);
      break;
    case SubTests::ST_ByteAddressBuffer:
      StructuredBufferTest(WGALShaderResourceType::ByteAddressBuffer);
      break;
    case SubTests::ST_Texture2D:
      Texture2D();
      break;
    case SubTests::ST_Texture2DArray:
      Texture2DArray();
      break;
    case SubTests::ST_GenerateMipMaps:
      GenerateMipMaps();
      break;
    case SubTests::ST_PushConstants:
      PushConstantsTest();
      break;
    case SubTests::ST_BindGroups:
      BindGroupsTest();
      break;
    case SubTests::ST_Timestamps:
    {
      auto res = Timestamps();
      EndFrame();
      return res;
    }
    break;
    case SubTests::ST_OcclusionQueries:
    {
      auto res = OcclusionQueries();
      EndFrame();
      return res;
    }
    break;
    case SubTests::ST_CustomVertexStreams:
      CustomVertexStreams();
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

void WRendererTestPipelineStates::MapImageNumberToString(const char* szTestName, const WSubTestEntry& subTest, WUInt32 uiImageNumber, WStringBuilder& out_sString) const
{
  if (subTest.m_iSubTestIdentifier == ST_ByteAddressBuffer || subTest.m_iSubTestIdentifier == ST_TexelBuffer)
  {
    out_sString.SetFormat("{0}_{1}_{2}", szTestName, GetSubTestName(ST_StructuredBuffer), WArgI(uiImageNumber, 3, true));
    out_sString.ReplaceAll(" ", "_");
  }
  else
  {
    WGraphicsTest::MapImageNumberToString(szTestName, subTest, uiImageNumber, out_sString);
  }
}

void WRendererTestPipelineStates::RenderBlock(WMeshBufferResourceHandle mesh, WColor clearColor, WUInt32 uiRenderTargetClearMask, WRectFloat* pViewport, WRectU32* pScissor)
{
  BeginCommands("MostBasicTriangle");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
    BeginRendering(clearColor, uiRenderTargetClearMask, pViewport, pScissor);
    {

      if (mesh.IsValid())
      {
        WRenderContext::GetDefaultInstance()->BindShader(m_hNDCPositionOnlyShader);
        WRenderContext::GetDefaultInstance()->BindMeshBuffer(mesh);
        WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();
      }
      else
      {
        WRenderContext::GetDefaultInstance()->BindShader(m_hMostBasicTriangleShader);
        WRenderContext::GetDefaultInstance()->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);
        WRenderContext::GetDefaultInstance()->DrawMeshBuffer(1).AssertSuccess();
      }
    }

    EndRendering();
    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
      W_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}



void WRendererTestPipelineStates::MostBasicTriangleTest()
{
  m_bCaptureImage = true;
  RenderBlock({}, WColor::RebeccaPurple);
}

void WRendererTestPipelineStates::ViewportScissorTest()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const WUInt32 uiColumns = 2;
  const WUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  WRectFloat viewport = WRectFloat(0, 0, fElementWidth, fElementHeight);
  RenderBlock({}, WColor::CornflowerBlue, 0xFFFFFFFF, &viewport);

  viewport = WRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
  RenderBlock({}, WColor::Green, 0, &viewport);

  viewport = WRectFloat(0, 0, fElementWidth, fHeight);
  WRectU32 scissor = WRectU32(0, (WUInt32)fElementHeight, (WUInt32)fElementWidth, (WUInt32)fElementHeight);
  RenderBlock({}, WColor::Green, 0, &viewport, &scissor);

  m_bCaptureImage = true;
  viewport = WRectFloat(0, 0, fWidth, fHeight);
  scissor = WRectU32((WUInt32)fElementWidth, 0, (WUInt32)fElementWidth, (WUInt32)fElementHeight);
  RenderBlock({}, WColor::Green, 0, &viewport, &scissor);
}

void WRendererTestPipelineStates::VertexBufferTest()
{
  m_bCaptureImage = true;
  RenderBlock(m_hTriangleMesh, WColor::RebeccaPurple);
}

void WRendererTestPipelineStates::IndexBufferTest()
{
  m_bCaptureImage = true;
  RenderBlock(m_hSphereMesh, WColor::Orange);
}

void WRendererTestPipelineStates::PushConstantsTest()
{
  const WUInt32 uiColumns = 4;
  const WUInt32 uiRows = 2;

  BeginCommands("PushConstantsTest");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

    BeginRendering(WColor::CornflowerBlue, 0xFFFFFFFF);
    WRenderContext* pContext = WRenderContext::GetDefaultInstance();
    {
      pContext->BindShader(m_hPushConstantsShader);
      pContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

      for (WUInt32 x = 0; x < uiColumns; ++x)
      {
        for (WUInt32 y = 0; y < uiRows; ++y)
        {
          WTestData constants;
          WTransform t = WRendererTestUtils::CreateTransform(uiColumns, uiRows, x, y);
          constants.Vertex0 = (t * WVec3(1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
          constants.Vertex1 = (t * WVec3(-1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
          constants.Vertex2 = (t * WVec3(-0.f, 1.f, 0.0f)).GetAsVec4(1.0f);
          constants.VertexColor = WColorScheme::LightUI(float(x * uiRows + y) / (uiColumns * uiRows)).GetAsVec4();

          pContext->SetPushConstants("WTestData", constants);
          pContext->DrawMeshBuffer(1).AssertSuccess();
        }
      }
    }
    EndRendering();
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
      W_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

void WRendererTestPipelineStates::BindGroupsTest()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const WUInt32 uiColumns = 2;
  const WUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const WMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);

  auto constants = WRenderContext::GetConstantBufferData<WTestPerFrame>(m_hTestPerFrameConstantBuffer);
  constants->Time = 1.0f;
  WRenderContext* pContext = WRenderContext::GetDefaultInstance();
  {
    WBindGroupBuilder& bindGroupFrame = pContext->GetBindGroup(W_GAL_BIND_GROUP_FRAME);
    bindGroupFrame.BindBuffer("WTestPerFrame", m_hTestPerFrameConstantBuffer);
  }
  auto renderCube = [&](WRectFloat viewport, WMat4 mMVP, WUInt32 uiRenderTargetClearMask, WGALTextureHandle hTexture, const WGALTextureRange& textureRange)
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
    BeginRendering(WColor::RebeccaPurple, uiRenderTargetClearMask, &viewport);
    {
      WBindGroupBuilder& bindGroupDraw = WRenderContext::GetDefaultInstance()->GetBindGroup(W_GAL_BIND_GROUP_DRAW_CALL);
      bindGroupDraw.BindTexture("DiffuseTexture", hTexture, textureRange);

      WRenderContext::GetDefaultInstance()->BindShader(m_hShader, WShaderBindFlags::None);

      ObjectCB* ocb = WRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
      ocb->m_MVP = mMVP;
      ocb->m_Color = WColor(1, 1, 1, 1);

      WBindGroupBuilder& bindGroupMaterial = WRenderContext::GetDefaultInstance()->GetBindGroup(W_GAL_BIND_GROUP_MATERIAL);
      bindGroupMaterial.BindBuffer("PerObject", m_hObjectTransformCB);

      WRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hCubeUV);
      WRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();
    }
    EndRendering();
  };

  BeginCommands("SetsSlots");
  {
    WRectFloat viewport = WRectFloat(0, 0, fElementWidth, fElementHeight);
    renderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D, WGALTextureRange::MakeFromMipRange(0, 1));
    viewport = WRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    renderCube(viewport, mMVP, 0, m_hTexture2D, WGALTextureRange::MakeFromMipRange(1, 1));
    viewport = WRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    renderCube(viewport, mMVP, 0, m_hTexture2D, WGALTextureRange::MakeFromMipRange(2, 1));
    m_bCaptureImage = true;
    viewport = WRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    renderCube(viewport, mMVP, 0, m_hTexture2D, WGALTextureRange::MakeFromMipRange(3, 1));

    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
      W_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

void WRendererTestPipelineStates::ConstantBufferTest()
{
  const WUInt32 uiColumns = 4;
  const WUInt32 uiRows = 2;

  BeginCommands("ConstantBufferTest");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

    BeginRendering(WColor::CornflowerBlue, 0xFFFFFFFF);
    WRenderContext* pContext = WRenderContext::GetDefaultInstance();
    {
      WBindGroupBuilder& bindGroupTest = pContext->GetBindGroup();
      bindGroupTest.BindBuffer("WTestColors", m_hTestColorsConstantBuffer);
      bindGroupTest.BindBuffer("WTestPositions", m_hTestPositionsConstantBuffer);
      pContext->BindShader(m_hConstantBufferShader);
      pContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, 1);

      for (WUInt32 x = 0; x < uiColumns; ++x)
      {
        for (WUInt32 y = 0; y < uiRows; ++y)
        {
          {
            auto constants = WRenderContext::GetConstantBufferData<WTestColors>(m_hTestColorsConstantBuffer);
            constants->VertexColor = WColorScheme::LightUI(float(x * uiRows + y) / (uiColumns * uiRows)).GetAsVec4();
          }
          {
            WTransform t = WRendererTestUtils::CreateTransform(uiColumns, uiRows, x, y);
            auto constants = WRenderContext::GetConstantBufferData<WTestPositions>(m_hTestPositionsConstantBuffer);
            constants->Vertex0 = (t * WVec3(1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
            constants->Vertex1 = (t * WVec3(-1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
            constants->Vertex2 = (t * WVec3(-0.f, 1.f, 0.0f)).GetAsVec4(1.0f);
          }
          pContext->DrawMeshBuffer(1).AssertSuccess();
        }
      }
    }
    EndRendering();
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
      W_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

void WRendererTestPipelineStates::StructuredBufferTestUpload()
{
  if (m_iFrame == ImageCaptureFrames::StructuredBuffer_UpdateForNextFrame)
  {
    // Replace the elements at [0, 3] with more green ones by offsetting the color by 16.
    WTempHybridArray<WTestShaderData, 16> instanceData;
    WRendererTestUtils::FillStructuredBuffer(instanceData, 16 /*green*/);
    m_pDevice->UpdateBufferForNextFrame(m_hInstancingData, instanceData.GetArrayPtr().GetSubArray(0, 4).ToByteArray());
  }
  if (m_iFrame == ImageCaptureFrames::StructuredBuffer_UpdateForNextFrame2)
  {
    // Replace the elements at [8, 15] with the same data as the original 8 elements. We will render these afterwards using custom buffer views.
    WTempHybridArray<WTestShaderData, 16> instanceData;
    WRendererTestUtils::FillStructuredBuffer(instanceData);
    m_pDevice->UpdateBufferForNextFrame(m_hInstancingData, instanceData.GetArrayPtr().GetSubArray(0, 8).ToByteArray(), sizeof(WTestShaderData) * 8);
  }
}

void WRendererTestPipelineStates::StructuredBufferTest(WGALShaderResourceType::Enum bufferType)
{
  BeginCommands("InstancingTest");
  {
    WRenderContext* pContext = WRenderContext::GetDefaultInstance();

    if (m_iFrame == ImageCaptureFrames::StructuredBuffer_UAV)
    {
      pContext->BeginCompute("ComputeCopyData");

      pContext->BindShader(m_hCopyBufferShader);
      // Copy [12, 17] to the front (green)
      WBindGroupBuilder& bindGroupTest = pContext->GetBindGroup();
      bindGroupTest.BindBuffer("instancingData", m_hInstancingData);
      bindGroupTest.BindBuffer("instancingTarget", m_hInstancingDataUAV, {0, 4 * sizeof(WTestShaderData)});
      pContext->Dispatch(4, 1, 1).AssertSuccess();
      // Copy [8, 11] to the back (red)
      bindGroupTest.BindBuffer("instancingData", m_hInstancingData, {12 * sizeof(WTestShaderData), 4 * sizeof(WTestShaderData)});
      bindGroupTest.BindBuffer("instancingTarget", m_hInstancingDataUAV, {4 * sizeof(WTestShaderData), 4 * sizeof(WTestShaderData)});
      pContext->Dispatch(4, 1, 1).AssertSuccess();

      pContext->EndCompute();
    }

    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
    TransitionBuffer(m_hInstancingDataUAV, WGALResourceState::ShaderResource);
    WGALCommandEncoder* pCommandEncoder = BeginRendering(WColor::CornflowerBlue, 0xFFFFFFFF);
    {
      pContext->BindShader(m_hInstancingShader);
      pContext->BindMeshBuffer(m_hTriangleMesh);
      WBindGroupBuilder& bindGroupTest = pContext->GetBindGroup();
      if (m_iFrame <= ImageCaptureFrames::StructuredBuffer_UpdateForNextFrame)
      {
        bindGroupTest.BindBuffer("instancingData", m_hInstancingData);
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
      else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_UpdateForNextFrame2)
      {
        // Use the second half of the buffer to render the 8 triangles using two draw calls.
        bindGroupTest.BindBuffer("instancingData", m_hInstancingData, {8 * sizeof(WTestShaderData), 4 * sizeof(WTestShaderData)});
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
        bindGroupTest.BindBuffer("instancingData", m_hInstancingData, {12 * sizeof(WTestShaderData), 4 * sizeof(WTestShaderData)});
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
      }
      else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_Transient1)
      {
        WTempHybridArray<WTestShaderData, 16> instanceData;
        WRendererTestUtils::FillStructuredBuffer(instanceData, 16 /*green*/);
        // Update the entire buffer in lots of little upload calls with greener versions.
        for (WUInt32 i = 0; i < 16; i++)
        {
          pCommandEncoder->UpdateBuffer(m_hInstancingDataTransient, i * sizeof(WTestShaderData), instanceData.GetArrayPtr().GetSubArray(i, 1).ToByteArray(), WGALUpdateMode::AheadOfTime);
        }

        bindGroupTest.BindBuffer("instancingData", m_hInstancingDataTransient);
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
      else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_Transient2)
      {
        WTempHybridArray<WTestShaderData, 16> instanceData;
        WRendererTestUtils::FillStructuredBuffer(instanceData);
        // Update with one single update call for the first 8 elements matching the initial state.
        pCommandEncoder->UpdateBuffer(m_hInstancingDataTransient, 0, instanceData.GetArrayPtr().GetSubArray(0, 8).ToByteArray(), WGALUpdateMode::AheadOfTime);
        bindGroupTest.BindBuffer("instancingData", m_hInstancingDataTransient);
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
      else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_UAV)
      {
        bindGroupTest.BindBuffer("instancingData", m_hInstancingDataUAV);
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
    }
    EndRendering();
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
      W_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

void WRendererTestPipelineStates::Texture2D()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const WUInt32 uiColumns = 2;
  const WUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const WMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);

  BeginCommands("Texture2D");
  {
    WRectFloat viewport = WRectFloat(0, 0, fElementWidth, fElementHeight);
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D, WGALTextureRange::MakeFromMipRange(0, 1));
    viewport = WRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D, WGALTextureRange::MakeFromMipRange(1, 1));
    viewport = WRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D, WGALTextureRange::MakeFromMipRange(2, 1));
    viewport = WRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D, WGALTextureRange::MakeFromMipRange(3, 1));

    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
      W_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

void WRendererTestPipelineStates::Texture2DArray()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const WUInt32 uiColumns = 2;
  const WUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const WMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);

  BeginCommands("Texture2DArray");
  {
    WRectFloat viewport = WRectFloat(0, 0, fElementWidth, fElementHeight);
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
    TransitionTexture(m_hTexture2DArray, WGALResourceState::ShaderResource, {0, 1, 0, 1});
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2DArray, {0, 1, 0, 1});
    viewport = WRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    TransitionTexture(m_hTexture2DArray, WGALResourceState::ShaderResource, {0, 1, 1, 1});
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray, {0, 1, 1, 1});
    viewport = WRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    TransitionTexture(m_hTexture2DArray, WGALResourceState::ShaderResource, {1, 1, 0, 1});
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray, {1, 1, 0, 1});
    viewport = WRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    TransitionTexture(m_hTexture2DArray, WGALResourceState::ShaderResource, {1, 1, 1, 1});
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray, {1, 1, 1, 1});

    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
      W_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

void WRendererTestPipelineStates::GenerateMipMaps()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const WUInt32 uiColumns = 2;
  const WUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const WMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  BeginCommands("GenerateMipMaps");
  {
    WRectFloat viewport = WRectFloat(0, 0, fElementWidth, fElementHeight);
    {
      auto pGraph = WRenderGraphManager::CreateRenderGraph("GenerateMipMaps");
      WRenderGraphUtils::GenerateMipMaps(m_hTexture2D, {}, *pGraph);
      pGraph->Compile().AssertSuccess();
      pGraph->ComputeBarriers(*GetResourceStateTracker());

      WRenderGraphContext ctx(m_pEncoder, m_pDevice, WRenderContext::GetDefaultInstance());
      pGraph->Execute(ctx);
    }

    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
    TransitionTexture(m_hTexture2D, WGALResourceState::ShaderResource, WGALTextureRange::MakeFromMipRange(0, 1));
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D, WGALTextureRange::MakeFromMipRange(0, 1));
    viewport = WRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    TransitionTexture(m_hTexture2D, WGALResourceState::ShaderResource, WGALTextureRange::MakeFromMipRange(1, 1));
    RenderCube(viewport, mMVP, 0, m_hTexture2D, WGALTextureRange::MakeFromMipRange(1, 1));
    viewport = WRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    TransitionTexture(m_hTexture2D, WGALResourceState::ShaderResource, WGALTextureRange::MakeFromMipRange(2, 1));
    RenderCube(viewport, mMVP, 0, m_hTexture2D, WGALTextureRange::MakeFromMipRange(2, 1));
    viewport = WRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    TransitionTexture(m_hTexture2D, WGALResourceState::ShaderResource, WGALTextureRange::MakeFromMipRange(3, 1));
    RenderCube(viewport, mMVP, 0, m_hTexture2D, WGALTextureRange::MakeFromMipRange(3, 1));

    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
      W_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

WTestAppRun WRendererTestPipelineStates::Timestamps()
{
  BeginCommands("Timestamps");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

    WGALCommandEncoder* pCommandEncoder = BeginRendering(WColor::RebeccaPurple, 0xFFFFFFFF);

    if (m_iFrame == 2)
    {
      m_CPUTime[0] = WTime::Now();
      m_timestamps[0] = pCommandEncoder->InsertTimestamp();
    }
    WRenderContext::GetDefaultInstance()->BindShader(m_hNDCPositionOnlyShader);
    WRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hSphereMesh);
    WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    if (m_iFrame == 2)
      m_timestamps[1] = pCommandEncoder->InsertTimestamp();

    EndRendering();

    if (m_iFrame == 2)
    {
      m_hFence = pCommandEncoder->InsertFence();
      pCommandEncoder->Flush();
    }
  }
  EndCommands();


  if (m_iFrame >= 2)
  {
    // #TODO_VULKAN Our CPU / GPU timestamp calibration is not precise enough to allow comparing between zones reliably. Need to implement VK_KHR_calibrated_timestamps.
    const WTime epsilon = WTime::MakeFromMilliseconds(16);
    WEnum<WGALAsyncResult> fenceResult = m_pDevice->GetFenceResult(m_hFence);
    if (fenceResult == WGALAsyncResult::Ready)
    {
      if (m_pDevice->GetTimestampResult(m_timestamps[0], m_GPUTime[0]) == WGALAsyncResult::Ready && m_pDevice->GetTimestampResult(m_timestamps[1], m_GPUTime[1]) == WGALAsyncResult::Ready)
      {
        m_CPUTime[1] = WTime::Now();
        W_TEST_BOOL_MSG(m_CPUTime[0] <= (m_GPUTime[0] + epsilon), "%.4f < %.4f", m_CPUTime[0].GetMilliseconds(), m_GPUTime[0].GetMilliseconds());
        W_TEST_BOOL_MSG(m_GPUTime[0] <= m_GPUTime[1], "%.4f < %.4f", m_GPUTime[0].GetMilliseconds(), m_GPUTime[1].GetMilliseconds());
        W_TEST_BOOL_MSG(m_GPUTime[1] <= (m_CPUTime[1] + epsilon), "%.4f < %.4f", m_GPUTime[1].GetMilliseconds(), m_CPUTime[1].GetMilliseconds());
        WTestFramework::GetInstance()->Output(WTestOutput::Message, "Timestamp results received after %d frames or %.2f ms (%d frames after fence)", m_iFrame - 2, (WTime::Now() - m_CPUTime[0]).GetMilliseconds(), m_iDelay);
        return WTestAppRun::Quit;
      }
      else
      {
        m_iDelay++;
      }
    }
  }

  if (m_iFrame >= 100)
  {
    WLog::Error("Timestamp results did not complete in 100 frames / {} seconds", (WTime::Now() - m_CPUTime[0]).AsFloatInSeconds());
    return WTestAppRun::Quit;
  }
  return WTestAppRun::Continue;
}

WTestAppRun WRendererTestPipelineStates::OcclusionQueries()
{
  BeginCommands("OcclusionQueries");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

    WGALCommandEncoder* pCommandEncoder = BeginRendering(WColor::RebeccaPurple, 0xFFFFFFFF);

    // #TODO_VULKAN Vulkan will assert if we don't render something bogus here. The reason is that occlusion queries must be started and stopped within the same render pass. However, as we start the render pass lazily within WGALCommandEncoderImplVulkan::FlushDeferredStateChanges, the BeginOcclusionQuery call is actually still outside the render pass.
    WRenderContext::GetDefaultInstance()->BindShader(m_hNDCPositionOnlyShader);
    WRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hTriangleMesh);
    WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    if (m_iFrame == 2)
    {
      W_TEST_BOOL(m_queries[0].IsInvalidated());
      m_queries[0] = pCommandEncoder->BeginOcclusionQuery(WGALQueryType::NumSamplesPassed);
      W_TEST_BOOL(!m_queries[0].IsInvalidated());
      pCommandEncoder->EndOcclusionQuery(m_queries[0]);

      W_TEST_BOOL(m_queries[1].IsInvalidated());
      m_queries[1] = pCommandEncoder->BeginOcclusionQuery(WGALQueryType::AnySamplesPassed);
      W_TEST_BOOL(!m_queries[1].IsInvalidated());
      pCommandEncoder->EndOcclusionQuery(m_queries[1]);

      m_queries[2] = pCommandEncoder->BeginOcclusionQuery(WGALQueryType::NumSamplesPassed);
    }
    else if (m_iFrame == 3)
    {
      m_queries[3] = pCommandEncoder->BeginOcclusionQuery(WGALQueryType::AnySamplesPassed);
    }

    WRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hSphereMesh);
    WRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    if (m_iFrame == 2)
    {
      pCommandEncoder->EndOcclusionQuery(m_queries[2]);
    }
    else if (m_iFrame == 3)
    {
      pCommandEncoder->EndOcclusionQuery(m_queries[3]);
    }
    EndRendering();

    if (m_iFrame == 3)
    {
      m_CPUTime[0] = WTime::Now();
      m_hFence = pCommandEncoder->InsertFence();
      pCommandEncoder->Flush();
    }
  }
  EndCommands();

  if (m_iFrame >= 3)
  {
    WEnum<WGALAsyncResult> fenceResult = m_pDevice->GetFenceResult(m_hFence);
    if (fenceResult == WGALAsyncResult::Ready)
    {
      WEnum<WGALAsyncResult> queryResults[4];
      WUInt64 queryValues[4];
      for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_queries); i++)
      {
        queryResults[i] = m_pDevice->GetOcclusionQueryResult(m_queries[i], queryValues[i]);
        if (!W_TEST_BOOL(queryResults[i] != WGALAsyncResult::Expired))
        {
          return WTestAppRun::Quit;
        }
      }

      bool bAllReady = true;
      for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_queries); i++)
      {
        if (queryResults[i] != WGALAsyncResult::Ready)
          bAllReady = false;
      }

      if (bAllReady)
      {
        WTestFramework::GetInstance()->Output(WTestOutput::Message, "Occlusion query results received after %d frames or %.2f ms (%d frames after fence)", m_iFrame - 3, (WTime::Now() - m_CPUTime[0]).GetMilliseconds(), m_iDelay);

        W_TEST_INT(queryValues[0], 0);
        W_TEST_INT(queryValues[1], 0);

        W_TEST_BOOL(queryValues[2] >= 1);
        W_TEST_BOOL(queryValues[3] >= 1);
        return WTestAppRun::Quit;
      }
      else
      {
        m_iDelay++;
      }
    }
  }

  if (m_iFrame >= 100)
  {
    WLog::Error("Occlusion query results did not complete in 100 frames / {} seconds", (WTime::Now() - m_CPUTime[0]).AsFloatInSeconds());
    return WTestAppRun::Quit;
  }

  return WTestAppRun::Continue;
}

void WRendererTestPipelineStates::CustomVertexStreams()
{
  BeginCommands("InstancingTest");
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

    WGALCommandEncoder* pCommandEncoder = BeginRendering(WColor::CornflowerBlue, 0xFFFFFFFF);

    WRenderContext* pContext = WRenderContext::GetDefaultInstance();
    {
      pContext->BindShader(m_hCustomVertexStreamShader);
      pContext->BindMeshBuffer(m_hTriangleMesh);
      pContext->SetVertexAttributes(m_VertexAttributes);

      if (m_iFrame <= ImageCaptureFrames::DefaultCapture)
      {
        pContext->BindVertexBuffer(m_hInstancingDataVertexStream, 5, WGALVertexBindingRate::Instance, 0);
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
      else if (m_iFrame >= ImageCaptureFrames::CustomVertexStreams_Offsets)
      {
        // Render the same image but this time using two draw calls with offsets.
        pContext->BindVertexBuffer(m_hInstancingDataVertexStream, 5, WGALVertexBindingRate::Instance, 0);
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
        pContext->BindVertexBuffer(m_hInstancingDataVertexStream, 5, WGALVertexBindingRate::Instance, 4 * sizeof(WTestShaderData));
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
      }
    }
    EndRendering();
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
      W_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

static WRendererTestPipelineStates g_PipelineStatesTest;
