#pragma once

#include <Foundation/IO/DependencyFile.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

/// Descriptor for GPU pipeline state associated with a shader.
///
/// Contains blend, depth-stencil, and rasterizer state descriptions.
/// Parsed from shader source and serialized with shader permutations.
struct W_RENDERERCORE_DLL WShaderStateResourceDescriptor
{
  WGALBlendStateCreationDescription m_BlendDesc;
  WGALDepthStencilStateCreationDescription m_DepthStencilDesc;
  WGALRasterizerStateCreationDescription m_RasterizerDesc;

  WUInt8 m_uiShaderStencilRef = 0;       ///< Stencil reference value for stencil test comparison
  bool m_bUseUserStencilRefValue = false; ///< Whether to use the stencil ref value, that is provided externally

  /// Parses state descriptions from shader source text.
  WResult Parse(const char* szSource);

  void Load(WStreamReader& inout_stream);
  void Save(WStreamWriter& inout_stream) const;

  /// Calculates a hash of all state descriptions for comparison.
  WUInt32 CalculateHash() const;
};

/// Serialized state of a shader permutation.
///
/// Used by WShaderPermutationResourceLoader to convert into an WShaderPermutationResource.
/// Contains hashes to shader stage binaries, pipeline state, dependencies, and permutation variable values.
class W_RENDERERCORE_DLL WShaderPermutationBinary
{
public:
  WShaderPermutationBinary();

  WResult Write(WStreamWriter& inout_stream);
  WResult Read(WStreamReader& inout_stream, bool& out_bOldVersion);

  /// Hashes of compiled shader stage binaries for each shader stage.
  ///
  /// The actual binary will be loaded from the hash via WShaderStageBinary::LoadStageBinary
  /// to produce WShaderStageBinary objects.
  WUInt32 m_uiShaderStageHashes[WGALShaderStage::ENUM_COUNT];

  WDependencyFile m_DependencyFile;                     ///< File dependencies for hot-reloading.

  WShaderStateResourceDescriptor m_StateDescriptor;     ///< Pipeline state (blend, depth-stencil, rasterizer).

  WHybridArray<WPermutationVar, 16> m_PermutationVars; ///< Values of permutation variables for this permutation.
};
