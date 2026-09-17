#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerDXC/ShaderCompilerDXC.h>
#include <ShaderCompilerVulkan/ShaderCompilerVulkanDLL.h>

class W_SHADERCOMPILERVULKAN_DLL WShaderCompilerVulkan : public WShaderCompilerDXC
{
  W_ADD_DYNAMIC_REFLECTION(WShaderCompilerVulkan, WShaderCompilerDXC);

public:
  virtual void GetSupportedPlatforms(WHybridArray<WString, 4>& out_platforms) override
  {
    out_platforms.PushBack("VULKAN");
  }

  virtual WEnum<WGALBufferLayout> GetMaterialBufferLayout(WStringView sPlatform) const override
  {
    return WGALBufferLayout::Vulkan_Std430_relaxed;
  }
};
