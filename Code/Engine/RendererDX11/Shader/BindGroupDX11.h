#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/BindGroup.h>

class WGALBindGroupDX11 : public WGALBindGroup
{
public:
protected:
  friend class WGALDeviceDX11;
  friend class WMemoryUtils;

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;
  virtual void Invalidate(WGALDevice* pDevice) override;
  virtual bool IsInvalidated() const override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

  WGALBindGroupDX11(const WGALBindGroupCreationDescription& Description);

  virtual ~WGALBindGroupDX11();

private:
  bool m_bInvalidated = false;
};
