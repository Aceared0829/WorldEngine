#pragma once

#include <Foundation/Types/Status.h>
#include <RendererCore/Shader/ShaderHelper.h>
#include <RendererCore/ShaderCompiler/Declarations.h>
#include <RendererFoundation/Device/DeviceCapabilities.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>

class WPropertyAttribute;

class W_RENDERERCORE_DLL WShaderParser
{
public:
  struct AttributeDefinition
  {
    WString m_sType;
    WHybridArray<WVariant, 8> m_Values;
  };

  struct ParameterDefinition
  {
    const WRTTI* m_pType = nullptr;
    WString m_sType;
    WString m_sName;

    WHybridArray<AttributeDefinition, 4> m_Attributes;
  };

  struct EnumValue
  {
    WHashedString m_sValueName;
    WInt32 m_iValueValue = 0;
  };

  struct EnumDefinition
  {
    WString m_sName;
    WUInt32 m_uiDefaultValue = 0;
    WHybridArray<EnumValue, 16> m_Values;
  };

  static WResult PreprocessSection(WStringView sSectionContent, WArrayPtr<WString> customDefines, WStringBuilder& out_sResult);

  static void ParseMaterialParameterSection(WStringView sSection, WDynamicArray<ParameterDefinition>& out_parameter, WDynamicArray<EnumDefinition>& out_enumDefinitions);

  static void ParsePermutationSection(WStringView sPermutationSection, WDynamicArray<WHashedString>& out_permVars, WDynamicArray<WPermutationVar>& out_fixedPermVars);

  static WStatus ParseMaterialConstantsSection(WStringView sMaterialConstantsSection, WSharedPtr<WShaderConstantBufferLayout>& out_pMaterialConstantBufferLayout);
  static void LayoutMaterialConstants(WShaderConstantBufferLayout& ref_materialConstantBufferLayout, WEnum<WGALBufferLayout> layout);

  static void ParsePermutationVarConfig(WStringView sPermutationVarConfig, WVariant& out_defaultValue, EnumDefinition& out_enumDefinition);



  /// Tries to find shader resource declarations inside the shader source.
  ///
  /// Used by the shader compiler implementations to generate resource mappings to sets/slots without creating conflicts across shader stages. For a list of supported resource declarations and possible pitfalls, please refer to https://ezengine.net/pages/docs/graphics/shaders/shader-resources.html.
  /// \param sShaderStageSource The shader source to parse.
  /// \param out_Resources The shader resources found inside the source.
  static void ParseShaderResources(WStringView sShaderStageSource, WDynamicArray<WShaderResourceDefinition>& out_resources);

  /// Delegate to creates a new declaration and register binding for a specific shader WShaderResourceDefinition.
  /// \param sPlatform The platform for which the shader is being compiled. Will be one of the values returned by GetSupportedPlatforms.
  /// \param sDeclaration The shader resource declaration without any attributes, e.g. "Texture2D DiffuseTexture"
  /// \param binding The binding that needs to be set on the output out_sDeclaration.
  /// \param out_sDeclaration The new declaration that changes sDeclaration according to the provided 'binding', e.g. "Texture2D DiffuseTexture : register(t0, space5)"
  using CreateResourceDeclaration = WDelegate<void(WStringView, WStringView, const WShaderResourceBinding&, WStringBuilder&)>;

  /// Merges the shader resource bindings of all used shader stages.
  ///
  /// The function can fail if a shader resource of the same name has different signatures in two stages. E.g. the type, slot or set is different. Shader resources must be uniquely identified via name.
  /// \param spd The shader currently being processed.
  /// \param out_bindings A hashmap from shader resource name to shader resource binding. If a binding is used in multiple stages, WShaderResourceBinding::m_Stages will be the combination of all used stages.
  /// \param pLog Log interface to write errors to.
  /// \return Returns failure if the shader stages could not be merged.
  static WResult MergeShaderResourceBindings(const WShaderProgramData& spd, WHashTable<WHashedString, WShaderResourceBinding>& out_bindings, WLogInterface* pLog);

  /// Makes sure that bindings fulfills the basic requirements that WorldEngine has for resource bindings in a shader, e.g. that each binding has a set / slot set.
  static WResult SanityCheckShaderResourceBindings(const WHashTable<WHashedString, WShaderResourceBinding>& bindings, WLogInterface* pLog);

  /// Creates a new shader source code that patches all shader resources to contain fixed set / slot bindings.
  /// \param sPlatform The platform for which the shader should be patched.
  /// \param sShaderStageSource The original shader source code that should be patched.
  /// \param resources A list of all shader resources that need to be patched within sShaderStageSource.
  /// \param bindings The binding information that each shader resource should have after patching. These bindings must have unique set / slots combinations for each resource.
  /// \param createDeclaration The callback to be called to generate the new shader resource declaration.
  /// \param out_shaderStageSource The new shader source code after patching.
  static void ApplyShaderResourceBindings(WStringView sPlatform, WStringView sShaderStageSource, const WDynamicArray<WShaderResourceDefinition>& resources, const WHashTable<WHashedString, WShaderResourceBinding>& bindings, const CreateResourceDeclaration& createDeclaration, WStringBuilder& out_sShaderStageSource);
};
