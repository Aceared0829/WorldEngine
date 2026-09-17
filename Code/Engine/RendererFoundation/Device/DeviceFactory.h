#pragma once

#include <Foundation/Types/Delegate.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct W_RENDERERFOUNDATION_DLL WGALDeviceFactory
{
  using CreatorFunc = WDelegate<WInternal::NewInstance<WGALDevice>(WAllocator*, const WGALDeviceCreationDescription&)>;

  static WInternal::NewInstance<WGALDevice> CreateDevice(WStringView sRendererName, WAllocator* pAllocator, const WGALDeviceCreationDescription& desc);

  static void GetShaderModelAndCompiler(WStringView sRendererName, const char*& ref_szShaderModel, const char*& ref_szShaderCompiler);

  static void RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler);
  static void UnregisterCreatorFunc(const char* szRendererName);
};
