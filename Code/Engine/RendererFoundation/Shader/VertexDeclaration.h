
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class W_RENDERERFOUNDATION_DLL WGALVertexDeclaration : public WGALObject<WGALVertexDeclarationCreationDescription>
{
public:
protected:
  friend class WGALDevice;

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;

  WGALVertexDeclaration(const WGALVertexDeclarationCreationDescription& Description);

  virtual ~WGALVertexDeclaration();
};
