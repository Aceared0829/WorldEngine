#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class WGALDevice;

/// Allows for a GPU texture to be read back to the CPU.
/// Uses the same WGALTextureCreationDescription as a normal texture for convenience. While most of the properties may be irrelevant for this purpose, the user should not have to care about that and just request a readback texture that can read back a texture of the given description.
class W_RENDERERFOUNDATION_DLL WGALReadbackTexture : public WGALResource<WGALTextureCreationDescription>
{
protected:
  friend class WGALDevice;

  WGALReadbackTexture(const WGALTextureCreationDescription& Description);
  virtual ~WGALReadbackTexture();

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;
};
