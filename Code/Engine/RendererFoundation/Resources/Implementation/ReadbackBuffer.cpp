#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ReadbackBuffer.h>

WGALReadbackBuffer::WGALReadbackBuffer(const WGALBufferCreationDescription& Description)
  : WGALResource(Description)
{
}

WGALReadbackBuffer::~WGALReadbackBuffer() = default;
