
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class W_RENDERERFOUNDATION_DLL WGALBuffer : public WGALResource<WGALBufferCreationDescription>
{
public:
  W_ALWAYS_INLINE WUInt32 GetSize() const;
  W_ALWAYS_INLINE WGALBufferRange ClampRange(WGALBufferRange range) const;

protected:
  friend class WGALDevice;

  WGALBuffer(const WGALBufferCreationDescription& Description);
  virtual ~WGALBuffer();

  virtual WResult InitPlatform(WGALDevice* pDevice, WArrayPtr<const WUInt8> pInitialData) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;
};

#include <RendererFoundation/Resources/Implementation/Buffer_inl.h>
