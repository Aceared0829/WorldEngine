
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class W_RENDERERFOUNDATION_DLL WGALBlendState : public WGALObject<WGALBlendStateCreationDescription>
{
public:
protected:
  WGALBlendState(const WGALBlendStateCreationDescription& Description);

  virtual ~WGALBlendState();

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;
};

class W_RENDERERFOUNDATION_DLL WGALDepthStencilState : public WGALObject<WGALDepthStencilStateCreationDescription>
{
public:
protected:
  WGALDepthStencilState(const WGALDepthStencilStateCreationDescription& Description);

  virtual ~WGALDepthStencilState();

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;
};

class W_RENDERERFOUNDATION_DLL WGALRasterizerState : public WGALObject<WGALRasterizerStateCreationDescription>
{
public:
protected:
  WGALRasterizerState(const WGALRasterizerStateCreationDescription& Description);

  virtual ~WGALRasterizerState();

  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;

  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;
};

class W_RENDERERFOUNDATION_DLL WGALSamplerState : public WGALResource<WGALSamplerStateCreationDescription>
{
public:
protected:
  WGALSamplerState(const WGALSamplerStateCreationDescription& Description);

  virtual ~WGALSamplerState();

  virtual void SetDebugNamePlatform(const char* szName) const override { W_IGNORE_UNUSED(szName); };
  virtual WResult InitPlatform(WGALDevice* pDevice) = 0;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) = 0;
};
