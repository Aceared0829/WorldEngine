#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Buffer.h>

WGALBuffer::WGALBuffer(const WGALBufferCreationDescription& Description)
  : WGALResource(Description)
{
}

WGALBuffer::~WGALBuffer() = default;
