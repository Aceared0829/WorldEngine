#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerHLSL/ShaderCompilerHLSLDLL.h>

struct ID3D11ShaderReflectionConstantBuffer;
struct _D3D11_SIGNATURE_PARAMETER_DESC;

class W_SHADERCOMPILERHLSL_DLL WShaderCompilerHLSL : public WShaderProgramCompiler
{
  W_ADD_DYNAMIC_REFLECTION(WShaderCompilerHLSL, WShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(WHybridArray<WString, 4>& out_platforms) override
  {
    out_platforms.PushBack("DX11_SM40_93");
    out_platforms.PushBack("DX11_SM40");
    out_platforms.PushBack("DX11_SM41");
    out_platforms.PushBack("DX11_SM50");
  }

  virtual WEnum<WGALBufferLayout> GetMaterialBufferLayout(WStringView sPlatform) const override
  {
    return WGALBufferLayout::DirectX_ConstantButter;
  }

  virtual WResult ModifyShaderSource(WShaderProgramData& inout_data, WLogInterface* pLog) override;
  virtual WResult Compile(WShaderProgramData& inout_data, WLogInterface* pLog) override;

private:
  WResult DefineShaderResourceBindings(const WShaderProgramData& data, WHashTable<WHashedString, WShaderResourceBinding>& inout_resourceBinding, WLogInterface* pLog);

  void CreateNewShaderResourceDeclaration(WStringView sPlatform, WStringView sDeclaration, const WShaderResourceBinding& binding, WStringBuilder& out_sDeclaration);

  void ReflectShaderStage(WShaderProgramData& inout_Data, WGALShaderStage::Enum Stage);
  WSharedPtr<WShaderConstantBufferLayout> ReflectConstantBufferLayout(WGALShaderByteCode& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection);
  WResult AddFakeBindGroupAssignments(WShaderProgramData& inout_Data, WGALShaderStage::Enum Stage, WLogInterface* pLog);
  void Initialize();
  static WGALResourceFormat::Enum GetEZFormat(const _D3D11_SIGNATURE_PARAMETER_DESC& paramDesc);

private:
  WMap<const char*, WGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
