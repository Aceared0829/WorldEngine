#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/RendererCoreDLL.h>

using WShaderResourceHandle = WTypedResourceHandle<class WShaderResource>;

/// Descriptor for creating a shader resource.
///
/// Currently empty as shaders are typically loaded from files rather than created from descriptors.
struct WShaderResourceDescriptor
{
};

class WShaderConstantBufferLayout;

/// Represents a shader resource loaded from an WShader file.
///
/// This resource stores metadata about a shader including which permutation variables it uses
/// and the layout of material constants. The actual compiled shader code is stored in
/// WShaderPermutationResource instances.
///
/// Shader resources are parsed from .WShader files which contain sections like [PERMUTATIONS]
/// and [MATERIALCONSTANTS]. The resource itself does not contain GPU shader bytecode.
class W_RENDERERCORE_DLL WShaderResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WShaderResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WShaderResource);
  W_RESOURCE_DECLARE_CREATEABLE(WShaderResource, WShaderResourceDescriptor);

public:
  WShaderResource();
  ~WShaderResource() = default;

  /// Returns whether the shader resource was loaded and parsed successfully.
  bool IsShaderValid() const { return m_bShaderResourceIsValid; }

  /// Returns the list of permutation variables that this shader declares.
  ///
  /// These are parsed from the [PERMUTATIONS] section of the .WShader file.
  /// The system uses this information to generate shader permutations for different
  /// combinations of these variables.
  WArrayPtr<const WHashedString> GetUsedPermutationVars() const { return m_PermutationVarsUsed; }

  /// Returns the layout of material constants for this shader.
  ///
  /// This describes the structure and types of constants that can be set per-material.
  /// Returns nullptr if the shader has no [MATERIALCONSTANTS] section.
  const WSharedPtr<WShaderConstantBufferLayout>& GetMaterialLayout() const { return m_pLayout; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  WHybridArray<WHashedString, 16> m_PermutationVarsUsed;
  WSharedPtr<WShaderConstantBufferLayout> m_pLayout;
  bool m_bShaderResourceIsValid = false;
};
