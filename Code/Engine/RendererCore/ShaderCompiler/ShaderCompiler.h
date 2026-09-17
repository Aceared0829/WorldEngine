#pragma once

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/Declarations.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class WRemoteMessage;

/// Shader compiler interface.
/// Custom shader compiles need to derive from this class and implement the pure virtual interface functions. Instances are created via reflection so each implementation must be properly reflected.
class W_RENDERERCORE_DLL WShaderProgramCompiler : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WShaderProgramCompiler, WReflectedClass);

public:
  /// Returns the platforms that this shader compiler supports.
  /// \param out_platforms Filled with the platforms this compiler supports.
  virtual void GetSupportedPlatforms(WHybridArray<WString, 4>& out_platforms) = 0;

  /// Returns the layout used for material buffers on the given platform
  /// \param sPlatform The platform for which the layout is to be retrieved
  /// \return an WGALBufferLayout value.
  virtual WEnum<WGALBufferLayout> GetMaterialBufferLayout(WStringView sPlatform) const = 0;

  /// Allows the shader compiler to modify the shader source before hashing and compiling. This allows it to implement custom features by injecting code before the compile process. Mostly used to define resource bindings that do not cause conflicts across shader stages.
  /// \param inout_data The state of the shader compiler. Only m_sShaderSource should be modified by the implementation.
  /// \param pLog Logging interface to be used when outputting any errors.
  /// \return Returns whether the shader could be modified. On failure, the shader won't be compiled.
  virtual WResult ModifyShaderSource(WShaderProgramData& inout_data, WLogInterface* pLog) = 0;

  /// Compiles the shader comprised of multiple stages defined in inout_data.
  /// \param inout_data The state of the shader compiler. m_Resources and m_ByteCode should be written to on successful return code.
  /// \param pLog Logging interface to be used when outputting any errors.
  /// \return Returns whether the shader was compiled successfully. On failure, errors should be written to pLog.
  virtual WResult Compile(WShaderProgramData& inout_data, WLogInterface* pLog) = 0;
};

class W_RENDERERCORE_DLL WShaderCompiler
{
public:
  WResult CompileShaderPermutationForPlatforms(WStringView sFile, const WArrayPtr<const WPermutationVar>& permutationVars, WLogInterface* pLog, WStringView sPlatform = "ALL", WTokenizedFileCache* pFileCache = nullptr);

private:
  WResult RunShaderCompiler(WStringView sFile, WStringView sPlatform, WShaderProgramCompiler* pCompiler, WLogInterface* pLog, WTokenizedFileCache* pFileCache = nullptr);

  void WriteFailedShaderSource(WShaderProgramData& spd, WLogInterface* pLog);

  bool PassThroughUnknownCommandCB(WStringView sCmd) { return sCmd == "version"; }

  void ShaderCompileMsg(WRemoteMessage& msg);

  struct WShaderData
  {
    WString m_Platforms;
    WHybridArray<WPermutationVar, 16> m_Permutations;
    WHybridArray<WPermutationVar, 16> m_FixedPermVars;
    WString m_StateSource;
    WString m_ShaderStageSource[WGALShaderStage::ENUM_COUNT];
  };

  WResult FileOpen(WStringView sAbsoluteFile, WDynamicArray<WUInt8>& FileContent, WTimestamp& out_FileModification);

  WSharedPtr<WShaderConstantBufferLayout> m_pMaterialBufferLayout;
  WSet<WString> m_MaterialParameters;
  WStringBuilder m_StageSourceFile[WGALShaderStage::ENUM_COUNT];

  WTokenizedFileCache m_FileCache;
  WShaderData m_ShaderData;

  WSet<WString> m_IncludeFiles;
  bool m_bCompilingShaderRemote = false;
  WResult m_RemoteShaderCompileResult = W_FAILURE;
};
