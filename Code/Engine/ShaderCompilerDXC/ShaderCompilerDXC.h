#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerDXC/ShaderCompilerDXCDLL.h>

struct SpvReflectDescriptorBinding;
struct SpvReflectBlockVariable;

class W_SHADERCOMPILERDXC_DLL WShaderCompilerDXC : public WShaderProgramCompiler
{
  W_ADD_DYNAMIC_REFLECTION(WShaderCompilerDXC, WShaderProgramCompiler);

public:
  virtual WResult ModifyShaderSource(WShaderProgramData& inout_data, WLogInterface* pLog) override;
  virtual WResult Compile(WShaderProgramData& inout_data, WLogInterface* pLog) override;

protected:
  virtual void ConfigureDxcArgs(WDynamicArray<WStringWChar>& inout_Args);
  virtual bool AllowCombinedImageSamplers() const { return true; }

private:
  /// Sets fixed set / slot bindings to each resource.
  /// The end result will have these properties:
  /// 1. Every binding name has a unique set / slot.
  /// 2. Bindings that already had a fixed set or slot (e.g. != -1) should not have these changed.
  /// 2. Set / slots can only be the same for two bindings if they have been changed to WGALShaderResourceType::TextureAndSampler.
  WResult DefineShaderResourceBindings(const WShaderProgramData& data, WHashTable<WHashedString, WShaderResourceBinding>& inout_resourceBinding, WLogInterface* pLog);

  void CreateNewShaderResourceDeclaration(WStringView sPlatform, WStringView sDeclaration, const WShaderResourceBinding& binding, WStringBuilder& out_sDeclaration);

  WResult ReflectShaderStage(WShaderProgramData& inout_Data, WGALShaderStage::Enum Stage);
  WSharedPtr<WShaderConstantBufferLayout> ReflectStructuredBufferLayout(WStringView sName, const SpvReflectBlockVariable& block);
  WSharedPtr<WShaderConstantBufferLayout> ReflectConstantBufferLayout(WStringView sName, const SpvReflectBlockVariable& block);
  WSharedPtr<WShaderConstantBufferLayout> ReflectBufferLayout(const SpvReflectBlockVariable& block);
  WResult FillResourceBinding(WShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  WResult FillSRVResourceBinding(WShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  WResult FillUAVResourceBinding(WShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  static WGALShaderTextureType::Enum GetTextureType(const SpvReflectDescriptorBinding& info);
  WResult Initialize();
  WResult CompileSPIRVShader(WStringView sFile, WStringView sSource, bool bDebug, WStringView sProfile, WStringView sEntryPoint, WDynamicArray<WUInt8>& out_ByteCode);
  WStringView GetProfileName(WStringView sPlatform, WGALShaderStage::Enum Stage);

private:
  WMap<const char*, WGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
