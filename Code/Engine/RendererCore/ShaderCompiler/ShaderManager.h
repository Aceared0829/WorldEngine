#pragma once

#include <Foundation/Containers/HashTable.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

/// Manages shader compilation, permutations, and caching.
///
/// Handles shader permutation variable configurations, validates permutation combinations,
/// and manages shader compilation both at runtime and from cache. Platform-specific shader
/// compilation is supported through the active platform setting.
class W_RENDERERCORE_DLL WShaderManager
{
public:
  /// Configures the shader manager with platform and compilation settings.
  ///
  /// Must be called during initialization. The active platform determines which shader binaries
  /// to load. Runtime compilation allows shaders to be compiled on demand if not found in cache.
  static void Configure(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory = ":shadercache/ShaderCache",
    const char* szPermVarSubDirectory = "Shaders/PermutationVars");

  static const WString& GetPermutationVarSubDirectory() { return s_sPermVarSubDir; }
  static const WString& GetActivePlatform() { return s_sPlatform; }
  static const WString& GetCacheDirectory() { return s_sShaderCacheDirectory; }
  static bool IsRuntimeCompilationEnabled() { return s_bEnableRuntimeCompilation; }

  /// Reloads the permutation variable configuration for a specific variable.
  static void ReloadPermutationVarConfig(const char* szName, const WTempHashedString& sHashedName);

  /// Checks if a permutation variable value is valid according to its configuration.
  ///
  /// Returns false if the value is not allowed for the given variable.
  static bool IsPermutationValueAllowed(const char* szName, const WTempHashedString& sHashedName, const WTempHashedString& sValue,
    WHashedString& out_sName, WHashedString& out_sValue);

  static bool IsPermutationValueAllowed(const WHashedString& sName, const WHashedString& sValue);

  /// If the given permutation variable is an enum variable, this returns the possible values.
  /// Returns an empty array for other types of permutation variables.
  static WArrayPtr<const WShaderParser::EnumValue> GetPermutationEnumValues(const WHashedString& sName);

  /// Same as GetPermutationEnumValues() but also returns values for other types of variables.
  /// E.g. returns TRUE and FALSE for boolean variables.
  static void GetPermutationValues(const WHashedString& sName, WDynamicArray<WHashedString>& out_values);

  /// Begins preloading multiple shader permutations asynchronously.
  ///
  /// The permutations should be available within the specified time. This is a hint for the
  /// resource system to prioritize loading.
  static void PreloadPermutations(
    WShaderResourceHandle hShader, const WHashTable<WHashedString, WHashedString>& permVars, WTime shouldBeAvailableIn);

  /// Preloads a single shader permutation and returns its resource handle.
  ///
  /// If bAllowFallback is true and the exact permutation is not available, a fallback
  /// permutation may be returned.
  static WShaderPermutationResourceHandle PreloadSinglePermutation(
    WShaderResourceHandle hShader, const WHashTable<WHashedString, WHashedString>& permVars, bool bAllowFallback);

private:
  static WUInt32 FilterPermutationVars(WArrayPtr<const WHashedString> usedVars, const WHashTable<WHashedString, WHashedString>& permVars,
    WDynamicArray<WPermutationVar>& out_FilteredPermutationVariables);
  static WShaderPermutationResourceHandle PreloadSinglePermutationInternal(WStringView sResourceId, WUInt64 uiResourceIdHash, WUInt32 uiPermutationHash, WArrayPtr<WPermutationVar> filteredPermutationVariables);

  static bool s_bEnableRuntimeCompilation;
  static WString s_sPlatform;
  static WString s_sPermVarSubDir;
  static WString s_sShaderCacheDirectory;
};
