#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Declarations.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>

/// Output of ParseShaderResources. A shader resource definition found inside the shader source code.
struct WShaderResourceDefinition
{
  /// Just the declaration inside the shader source, e.g. "Texture1D Texture".
  WStringView m_sDeclaration;
  /// The declaration with any optional register mappings, e.g. "Texture1D Texture : register(12t, space3)"
  WStringView m_sDeclarationAndRegister;
  /// The extracted reflection of the resource containing type, slot, set etc.
  WShaderResourceBinding m_Binding;
};

/// Flags that affect the compilation process of a shader
struct WShaderCompilerFlags
{
  using StorageType = WUInt8;
  enum Enum
  {
    Debug = W_BIT(0),
    Default = 0,
  };

  struct Bits
  {
    StorageType Debug : 1;
  };
};
W_DECLARE_FLAGS_OPERATORS(WShaderCompilerFlags);

/// Storage used during the shader compilation process.
struct W_RENDERERCORE_DLL WShaderProgramData
{
  WShaderProgramData()
  {
    m_sPlatform = {};
    m_sSourceFile = {};

    for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
    {
      m_bWriteToDisk[stage] = true;
      m_sShaderSource[stage].Clear();
      m_Resources[stage].Clear();
      m_uiSourceHash[stage] = 0;
      m_ByteCode[stage].Clear();
    }
  }

  WBitflags<WShaderCompilerFlags> m_Flags;
  WStringView m_sPlatform;
  WStringView m_sSourceFile;
  WString m_sShaderSource[WGALShaderStage::ENUM_COUNT];
  WHybridArray<WShaderResourceDefinition, 8> m_Resources[WGALShaderStage::ENUM_COUNT];
  WUInt32 m_uiSourceHash[WGALShaderStage::ENUM_COUNT];
  WSharedPtr<WGALShaderByteCode> m_ByteCode[WGALShaderStage::ENUM_COUNT];
  bool m_bWriteToDisk[WGALShaderStage::ENUM_COUNT];
  WSet<WString> m_MaterialParameters; ///< Any resource matching these names will be forced into the material bind group.
};
