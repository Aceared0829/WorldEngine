#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ReadbackTexture.h>

WGALReadbackTexture::WGALReadbackTexture(const WGALTextureCreationDescription& Description)
  : WGALResource(Description)
{
}

WGALReadbackTexture::~WGALReadbackTexture() = default;
