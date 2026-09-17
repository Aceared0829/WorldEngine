#include <RendererTest/RendererTestPCH.h>

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

W_CREATE_SIMPLE_RENDERER_TEST(DataStructures, TextureDefaultState)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Immutable SRV texture defaults to ShaderResource")
  {
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = 4;
    desc.m_uiHeight = 4;
    desc.m_Format = WGALResourceFormat::RGBAUByteNormalized;
    desc.m_TextureFlags = WGALTextureUsageFlags::ShaderResource;
    desc.m_ResourceAccess.m_bImmutable = true;
    W_TEST_BOOL(desc.GetDefaultState() == WGALResourceState::ShaderResource);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Immutable depth texture defaults to DepthStencilRead")
  {
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = 4;
    desc.m_uiHeight = 4;
    desc.m_Format = WGALResourceFormat::D16;
    desc.m_TextureFlags = WGALTextureUsageFlags::ShaderResource;
    desc.m_ResourceAccess.m_bImmutable = true;
    W_TEST_BOOL(desc.GetDefaultState() == WGALResourceState::DepthStencilRead);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Color render target with SRV defaults to ShaderResource")
  {
    WGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(4, 4, WGALResourceFormat::RGBAUByteNormalized);
    W_TEST_BOOL(desc.GetDefaultState() == WGALResourceState::ShaderResource);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Depth render target with SRV defaults to DepthStencilRead")
  {
    WGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(4, 4, WGALResourceFormat::D16);
    W_TEST_BOOL(desc.GetDefaultState() == WGALResourceState::DepthStencilRead);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Presentable texture defaults to Present")
  {
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = 4;
    desc.m_uiHeight = 4;
    desc.m_Format = WGALResourceFormat::BGRAUByteNormalized;
    desc.m_TextureFlags = WGALTextureUsageFlags::RenderTarget | WGALTextureUsageFlags::Presentable;
    W_TEST_BOOL(desc.GetDefaultState() == WGALResourceState::Present);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "UAV-only texture defaults to UnorderedAccess")
  {
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = 4;
    desc.m_uiHeight = 4;
    desc.m_Format = WGALResourceFormat::RGBAFloat;
    desc.m_TextureFlags = WGALTextureUsageFlags::UnorderedAccess;
    W_TEST_BOOL(desc.GetDefaultState() == WGALResourceState::UnorderedAccess);
  }
}

W_CREATE_SIMPLE_RENDERER_TEST(DataStructures, BufferDefaultState)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Buffer with UAV defaults to UnorderedAccess")
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = 4;
    desc.m_uiTotalSize = 64;
    desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;
    W_TEST_BOOL(desc.GetDefaultState() == WGALResourceState::UnorderedAccess);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Buffer with VB+IB defaults to combined read states")
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = 4;
    desc.m_uiTotalSize = 64;
    desc.m_BufferFlags = WGALBufferUsageFlags::VertexBuffer | WGALBufferUsageFlags::IndexBuffer;
    desc.m_ResourceAccess.m_bImmutable = true;
    auto defaultState = desc.GetDefaultState();
    W_TEST_BOOL(defaultState.IsSet(WGALResourceState::VertexBuffer));
    W_TEST_BOOL(defaultState.IsSet(WGALResourceState::IndexBuffer));
    W_TEST_BOOL(!defaultState.IsAnySet(WGALResourceState::AllWriteStates));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Buffer with SRV-only defaults to ShaderResource")
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = 4;
    desc.m_uiTotalSize = 64;
    desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
    desc.m_ResourceAccess.m_bImmutable = true;
    W_TEST_BOOL(desc.GetDefaultState() == WGALResourceState::ShaderResource);
  }
}
