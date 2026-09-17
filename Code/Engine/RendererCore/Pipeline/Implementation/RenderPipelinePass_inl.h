#pragma once

WRenderPipelinePinConnection::WRenderPipelinePinConnection(Connectivity connectivity)
  : m_Connectivity(connectivity)
  , m_TextureHandle()
{
}

WRenderPipelinePinConnection::WRenderPipelinePinConnection(Connectivity connectivity, WRenderGraphTextureHandle hTextureHandle)
  : m_Connectivity(connectivity)
  , m_TextureHandle(hTextureHandle)
{
}

WRenderPipelinePinConnection::WRenderPipelinePinConnection(Connectivity connectivity, WRenderGraphBufferHandle hBufferHandle)
  : m_Connectivity(connectivity)
  , m_BufferHandle(hBufferHandle)
{
}

WRenderPipelinePinConnection::WRenderPipelinePinConnection(const WRenderPipelinePinConnection& other)
  : m_Connectivity(other.m_Connectivity)
{
  switch (other.m_Connectivity)
  {
    case Connectivity::Texture:
      m_TextureHandle = other.m_TextureHandle;
      break;
    case Connectivity::Buffer:
      m_BufferHandle = other.m_BufferHandle;
      break;
    default:
      break;
  }
}

WRenderPipelinePinConnection& WRenderPipelinePinConnection::operator=(const WRenderPipelinePinConnection& other)
{
  if (this != &other)
  {
    const_cast<Connectivity&>(m_Connectivity) = other.m_Connectivity;
    switch (m_Connectivity)
    {
      case Connectivity::Texture:
        m_TextureHandle = other.m_TextureHandle;
        break;
      case Connectivity::Buffer:
        m_BufferHandle = other.m_BufferHandle;
        break;
      default:
        m_TextureHandle = WRenderGraphTextureHandle();
        break;
    }
  }
  return *this;
}
