#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Time/Timestamp.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

using WShaderPermutationResourceHandle = WTypedResourceHandle<class WShaderPermutationResource>;
using WShaderStateResourceHandle = WTypedResourceHandle<class WShaderStateResource>;

/// Descriptor for shader permutation resources.
struct WShaderPermutationResourceDescriptor
{
  // empty
};

/// Runtime resource representing a specific shader permutation variant.
///
/// Shaders use permutation variables to create variants for different features (e.g., with/without shadows,
/// skinning, etc.). Each unique combination of permutation values results in a separate compiled shader.
/// This resource holds the compiled shader bytecode, render states, and permutation variable values.
class W_RENDERERCORE_DLL WShaderPermutationResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WShaderPermutationResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WShaderPermutationResource);
  W_RESOURCE_DECLARE_CREATEABLE(WShaderPermutationResource, WShaderPermutationResourceDescriptor);

public:
  WShaderPermutationResource();

  WGALShaderHandle GetGALShader() const { return m_hShader; }
  const WGALShaderByteCode* GetShaderByteCode(WGALShaderStage::Enum stage) const { return m_ByteCodes[stage]; }

  WGALBlendStateHandle GetBlendState() const { return m_hBlendState; }
  WGALDepthStencilStateHandle GetDepthStencilState() const { return m_hDepthStencilState; }
  WGALRasterizerStateHandle GetRasterizerState() const { return m_hRasterizerState; }

  /// Returns the stencil reference value for stencil testing.
  WUInt8 GetShaderStencilRefValue() const { return m_uiShaderStencilRef; }

  /// Returns whether the shader wants to use the user provided stencil reference value.
  bool GetUseUserStencilRefValue() const { return m_bUseUserStencilRef; }

  /// Returns true if the shader compiled successfully.
  bool IsShaderValid() const { return m_bShaderPermutationValid; }

  /// Returns the permutation variable values that define this shader variant.
  WArrayPtr<const WPermutationVar> GetPermutationVars() const { return m_PermutationVars; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual WResourceTypeLoader* GetDefaultResourceTypeLoader() const override;

private:
  friend class WShaderManager;

  WSharedPtr<const WGALShaderByteCode> m_ByteCodes[WGALShaderStage::ENUM_COUNT];

  bool m_bShaderPermutationValid;
  WGALShaderHandle m_hShader;

  WGALBlendStateHandle m_hBlendState;
  WGALDepthStencilStateHandle m_hDepthStencilState;
  WGALRasterizerStateHandle m_hRasterizerState;

  WUInt8 m_uiShaderStencilRef = 0;
  bool m_bUseUserStencilRef = false;

  WHybridArray<WPermutationVar, 16> m_PermutationVars;
};


/// Resource loader for shader permutation resources.
///
/// Handles loading compiled shader bytecode. If the permutation is not found or outdated,
/// it triggers shader compilation on-demand.
class WShaderPermutationResourceLoader : public WResourceTypeLoader
{
public:
  virtual WResourceLoadData OpenDataStream(const WResource* pResource) override;
  virtual void CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData) override;

  virtual bool IsResourceOutdated(const WResource* pResource) const override;

private:
  WResult RunCompiler(const WResource* pResource, WShaderPermutationBinary& BinaryInfo, bool bForce);
};
