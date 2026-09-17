#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

/// Serializes WGALShaderByteCode and provides access to the shader cache.
///
/// Compiled shader stage binaries are cached on disk and accessed via LoadStageBinary using a hash.
/// This allows shader permutations to share compiled stage binaries when they use the same code.
class W_RENDERERCORE_DLL WShaderStageBinary
{
public:
  /// Serialization version for shader stage binaries.
  enum Version
  {
    Version0,
    Version1,
    Version2,
    Version3, ///< Added Material Parameters
    Version4, ///< Constant buffer layouts
    Version5, ///< Debug flag
    Version6, ///< Rewrite, no backwards compatibility. Moves all data into WGALShaderByteCode.
    Version7, ///< Added tessellation support (m_uiTessellationPatchControlPoints)

    ENUM_COUNT,
    VersionCurrent = ENUM_COUNT - 1
  };

  WShaderStageBinary();
  ~WShaderStageBinary();

  /// Returns the compiled shader bytecode.
  WSharedPtr<const WGALShaderByteCode> GetByteCode() const;

private:
  friend class WRenderContext;
  friend class WShaderCompiler;
  friend class WShaderPermutationResource;
  friend class WShaderPermutationResourceLoader;

  WResult WriteStageBinary(WLogInterface* pLog, WStringView sPlatform) const;
  WResult Write(WStreamWriter& inout_stream) const;
  WResult Read(WStreamReader& inout_stream);
  WResult Write(WStreamWriter& inout_stream, const WShaderConstantBufferLayout& layout) const;
  WResult Read(WStreamReader& inout_stream, WShaderConstantBufferLayout& out_layout);

private:
  WUInt32 m_uiSourceHash = 0;
  WSharedPtr<WGALShaderByteCode> m_pGALByteCode;

private: // statics
  /// Loads a shader stage binary from the cache by hash.
  ///
  /// Returns nullptr if the binary is not in the cache. Binaries are cached per stage and platform.
  static WShaderStageBinary* LoadStageBinary(WGALShaderStage::Enum Stage, WUInt32 uiHash, WStringView sPlatform);

  static void OnEngineShutdown();

  static WMutex s_ShaderStageBinariesLock;
  static WMap<WUInt32, WShaderStageBinary> s_ShaderStageBinaries[WGALShaderStage::ENUM_COUNT];
};
