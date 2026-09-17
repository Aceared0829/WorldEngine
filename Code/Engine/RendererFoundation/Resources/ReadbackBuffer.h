#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class WGALDevice;

/// Allows for a GPU buffer to be read back to the CPU.
/// Uses the same WGALBufferCreationDescription as a normal buffer for convenience. While most of the properties may be irrelevant for this purpose, the user should not have to care about that and just request a readback buffer that can read back a buffer of the given description.
class W_RENDERERFOUNDATION_DLL WGALReadbackBuffer : public WGALResource<WGALBufferCreationDescription>
{
public:
  W_ALWAYS_INLINE WUInt32 GetSize() const { return m_Description.m_uiTotalSize; }

protected:
  friend class WGALDevice;

  WGALReadbackBuffer(const WGALBufferCreationDescription& Description);
  virtual ~WGALReadbackBuffer();

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;
};
