#include <RendererTest/RendererTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/RendererReflection.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererTest/Basics/ReadbackBuffer.h>

WResult WRendererTestReadbackBuffer::InitializeTest()
{
  WStartup::StartupCoreSystems();

  if (SetupRenderer().Failed())
    return W_FAILURE;

  W_SUCCEED_OR_RETURN(CreateWindow(320, 240));

  return W_SUCCESS;
}

WResult WRendererTestReadbackBuffer::DeInitializeTest()
{
  DestroyWindow();
  ShutdownRenderer();
  WStartup::ShutdownCoreSystems();
  WMemoryTracker::DumpMemoryLeaks();

  return W_SUCCESS;
}

void WRendererTestReadbackBuffer::SetupSubTests()
{
  const WGALDeviceCapabilities& caps = GetDeviceCapabilities();

  AddSubTest("VertexBuffer", SubTests::ST_VertexBuffer);
  AddSubTest("IndexBuffer", SubTests::ST_IndexBuffer);
  if (caps.m_bSupportsTexelBuffer)
  {
    AddSubTest("TexelBuffer", SubTests::ST_TexelBuffer);
  }
  AddSubTest("StructuredBuffer", SubTests::ST_StructuredBuffer);
  AddSubTest("ByteAddressBuffer", SubTests::ST_ByteAddressBuffer);
}

WResult WRendererTestReadbackBuffer::InitializeSubTest(WInt32 iIdentifier)
{
  m_iFrame = -1;
  m_bCaptureImage = false;
  m_ImgCompFrames.Clear();
  m_bReadbackInProgress = true;

  // m_hUVColorShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/ReadbackFloat.WShader");

  switch (iIdentifier)
  {
    case ST_VertexBuffer:
    {
      WDynamicArray<float> data;
      data.SetCountUninitialized(128);
      for (WUInt32 i = 0; i < 128; i++)
      {
        data[i] = (float)i;
      }
      m_BufferData = data.GetByteArrayPtr();

      m_hBufferReadback = m_pDevice->CreateVertexBuffer(4 * sizeof(float), 32, m_BufferData.GetByteArrayPtr());
      W_ASSERT_DEBUG(!m_hBufferReadback.IsInvalidated(), "Failed to create buffer");
    }
    break;
    case ST_IndexBuffer:
    {
      WDynamicArray<WUInt32> data;
      data.SetCountUninitialized(128);
      for (WUInt32 i = 0; i < 128; i++)
      {
        data[i] = i;
      }
      m_BufferData = data.GetByteArrayPtr();

      m_hBufferReadback = m_pDevice->CreateIndexBuffer(WGALIndexType::UInt, 128, m_BufferData.GetByteArrayPtr());
      W_ASSERT_DEBUG(!m_hBufferReadback.IsInvalidated(), "Failed to create buffer");
    }
    break;
    case ST_TexelBuffer:
    {
      WDynamicArray<WUInt32> data;
      data.SetCountUninitialized(128);
      for (WUInt32 i = 0; i < 128; i++)
      {
        data[i] = i;
      }
      m_BufferData = data.GetByteArrayPtr();

      WGALBufferCreationDescription desc;
      desc.m_uiStructSize = 0;
      desc.m_uiTotalSize = 128 * WGALResourceFormat::GetBitsPerElement(WGALResourceFormat::RGBAUByteNormalized) / 8;
      desc.m_BufferFlags = WGALBufferUsageFlags::TexelBuffer | WGALBufferUsageFlags::ShaderResource;
      desc.m_Format = WGALResourceFormat::RGBAUByteNormalized;
      desc.m_ResourceAccess.m_bImmutable = false;
      m_hBufferReadback = m_pDevice->CreateBuffer(desc, m_BufferData.GetByteArrayPtr());
      W_ASSERT_DEBUG(!m_hBufferReadback.IsInvalidated(), "Failed to create buffer");
    }
    break;
    case ST_StructuredBuffer:
    {
      WDynamicArray<WColor> data;
      data.SetCountUninitialized(128);
      for (WUInt32 i = 0; i < 128; i++)
      {
        data[i] = WColor::MakeFromKelvin(1000 + 10 * i);
      }
      m_BufferData = data.GetByteArrayPtr();

      WGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(WColor);
      desc.m_uiTotalSize = 128 * desc.m_uiStructSize;
      desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
      desc.m_ResourceAccess.m_bImmutable = false;
      m_hBufferReadback = m_pDevice->CreateBuffer(desc, m_BufferData.GetByteArrayPtr());
      W_ASSERT_DEBUG(!m_hBufferReadback.IsInvalidated(), "Failed to create buffer");
    }
    break;
    case ST_ByteAddressBuffer:
    {
      WDynamicArray<WUInt8> data;
      data.SetCountUninitialized(128);
      for (WUInt8 i = 0; i < 128; i++)
      {
        data[i] = i;
      }
      m_BufferData = data.GetByteArrayPtr();

      WGALBufferCreationDescription desc;
      desc.m_uiStructSize = 0;
      desc.m_uiTotalSize = 128 * sizeof(WUInt8);
      desc.m_BufferFlags = WGALBufferUsageFlags::ByteAddressBuffer | WGALBufferUsageFlags::ShaderResource;
      desc.m_ResourceAccess.m_bImmutable = false;
      m_hBufferReadback = m_pDevice->CreateBuffer(desc, m_BufferData.GetByteArrayPtr());
      W_ASSERT_DEBUG(!m_hBufferReadback.IsInvalidated(), "Failed to create buffer");
    }
    break;
    default:
      break;
  }

  return W_SUCCESS;
}

WResult WRendererTestReadbackBuffer::DeInitializeSubTest(WInt32 iIdentifier)
{
  m_Readback.Reset();
  if (!m_hBufferReadback.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hBufferReadback);
    m_hBufferReadback.Invalidate();
  }

  m_hComputeShader.Invalidate();

  // Don't call parent's DeInitializeSubTest - renderer shutdown happens in DeInitializeTest

  return W_SUCCESS;
}

WTestAppRun WRendererTestReadbackBuffer::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;
  BeginFrame();
  WTestAppRun res = ReadbackBuffer(uiInvocationCount);
  EndFrame();
  return res;
}


WTestAppRun WRendererTestReadbackBuffer::ReadbackBuffer(WUInt32 uiInvocationCount)
{
  if (m_iFrame == 1)
  {
    BeginCommands("Readback Buffer");
    // Queue readback
    {
      TransitionBuffer(m_hBufferReadback, WGALResourceState::CopySource);
      m_bReadbackInProgress = true;
      m_Readback.ReadbackBuffer(*m_pEncoder, m_hBufferReadback);
    }

    // Wait for results
    {
      WEnum<WGALAsyncResult> res = m_Readback.GetReadbackResult(WTime::MakeFromHours(1));
      W_ASSERT_ALWAYS(res == WGALAsyncResult::Ready, "Readback of texture failed");
    }

    // Readback result
    {
      m_bReadbackInProgress = false;
      {
        WArrayPtr<const WUInt8> memory;
        WReadbackBufferLock lock = m_Readback.LockBuffer(memory);
        W_ASSERT_ALWAYS(lock, "Failed to lock readback buffer");

        if (W_TEST_INT(memory.GetCount(), m_BufferData.GetCount()))
        {
          W_TEST_INT(WMemoryUtils::Compare(memory.GetPtr(), m_BufferData.GetData(), memory.GetCount()), 0);
        }
      }
    }
    EndCommands();
  }

  return m_bReadbackInProgress ? WTestAppRun::Continue : WTestAppRun::Quit;
}

static WRendererTestReadbackBuffer g_ReadbackBufferTest;
