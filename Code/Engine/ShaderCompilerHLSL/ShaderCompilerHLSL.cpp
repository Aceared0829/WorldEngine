#include <ShaderCompilerHLSL/ShaderCompilerHLSL.h>
#include <d3dcompiler.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WShaderCompilerHLSL, 1, WRTTIDefaultAllocator<WShaderCompilerHLSL>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult CompileDXShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, WDynamicArray<WUInt8>& out_byteCode)
{
  out_byteCode.Clear();

  ID3DBlob* pResultBlob = nullptr;
  ID3DBlob* pErrorBlob = nullptr;

  const char* szCompileSource = szSource;
  WStringBuilder sDebugSource;
  UINT flags1 = 0;
  if (bDebug)
  {
    flags1 = D3DCOMPILE_DEBUG | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = szSource;
    sDebugSource.ReplaceAll("#line ", "//ine ");
    szCompileSource = sDebugSource;
  }

  if (FAILED(D3DCompile(szCompileSource, strlen(szCompileSource), szFile, nullptr, nullptr, szEntryPoint, szProfile, flags1, 0, &pResultBlob, &pErrorBlob)))
  {
    if (bDebug)
    {
      // Try again with '#line' intact to get correct error messages with file and line info.
      pErrorBlob->Release();
      pErrorBlob = nullptr;
      W_VERIFY(FAILED(D3DCompile(szSource, strlen(szSource), szFile, nullptr, nullptr, szEntryPoint, szProfile, flags1, 0, &pResultBlob, &pErrorBlob)), "Debug compilation with commented out '#line' failed but original version did not.");
    }

    const char* szError = static_cast<const char*>(pErrorBlob->GetBufferPointer());

    W_LOG_BLOCK("Shader Compilation Failed", szFile);

    WLog::Error("Could not compile shader '{0}' for profile '{1}'", szFile, szProfile);
    WLog::Error("{0}", szError);

    pErrorBlob->Release();
    return W_FAILURE;
  }

  if (pErrorBlob != nullptr)
  {
    const char* szError = static_cast<const char*>(pErrorBlob->GetBufferPointer());

    W_LOG_BLOCK("Shader Compilation Error Message", szFile);
    WLog::Dev("{0}", szError);

    pErrorBlob->Release();
  }

  if (pResultBlob != nullptr)
  {
    out_byteCode.SetCountUninitialized((WUInt32)pResultBlob->GetBufferSize());
    WMemoryUtils::Copy(out_byteCode.GetData(), static_cast<WUInt8*>(pResultBlob->GetBufferPointer()), out_byteCode.GetCount());
    pResultBlob->Release();
  }

  return W_SUCCESS;
}

void WShaderCompilerHLSL::ReflectShaderStage(WShaderProgramData& inout_Data, WGALShaderStage::Enum Stage)
{
  ID3D11ShaderReflection* pReflector = nullptr;

  WGALShaderByteCode* pShader = inout_Data.m_ByteCode[Stage];
  D3DReflect(pShader->m_ByteCode.GetData(), pShader->m_ByteCode.GetCount(), IID_ID3D11ShaderReflection, (void**)&pReflector);


  D3D11_SHADER_DESC shaderDesc;
  pReflector->GetDesc(&shaderDesc);

  if (Stage == WGALShaderStage::VertexShader)
  {
    auto& vertexInputAttributes = pShader->m_ShaderVertexInput;
    vertexInputAttributes.Reserve(shaderDesc.InputParameters);
    for (WUInt32 i = 0; i < shaderDesc.InputParameters; ++i)
    {
      D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
      pReflector->GetInputParameterDesc(i, &paramDesc);

      WGALVertexAttributeSemantic::Enum semantic;
      if (!m_VertexInputMapping.TryGetValue(paramDesc.SemanticName, semantic))
      {
        // We ignore all system-value semantics as they are not provided by the user but the system so we don't care to reflect them.
        if (WStringUtils::StartsWith_NoCase(paramDesc.SemanticName, "SV_"))
          continue;

        W_ASSERT_NOT_IMPLEMENTED;
      }
      switch (semantic)
      {
        case WGALVertexAttributeSemantic::Color0:
          W_ASSERT_DEBUG(paramDesc.SemanticIndex <= 7, "Color out of range");
          semantic = static_cast<WGALVertexAttributeSemantic::Enum>((WUInt32)semantic + paramDesc.SemanticIndex);
          break;
        case WGALVertexAttributeSemantic::TexCoord0:
          W_ASSERT_DEBUG(paramDesc.SemanticIndex <= 9, "TexCoord out of range");
          semantic = static_cast<WGALVertexAttributeSemantic::Enum>((WUInt32)semantic + paramDesc.SemanticIndex);
          break;
        case WGALVertexAttributeSemantic::BoneIndices0:
          W_ASSERT_DEBUG(paramDesc.SemanticIndex <= 1, "BoneIndices out of range");
          semantic = static_cast<WGALVertexAttributeSemantic::Enum>((WUInt32)semantic + paramDesc.SemanticIndex);
          break;
        case WGALVertexAttributeSemantic::BoneWeights0:
          W_ASSERT_DEBUG(paramDesc.SemanticIndex <= 1, "BoneWeights out of range");
          semantic = static_cast<WGALVertexAttributeSemantic::Enum>((WUInt32)semantic + paramDesc.SemanticIndex);
          break;
        default:
          break;
      }

      WShaderVertexInputAttribute& attr = vertexInputAttributes.ExpandAndGetRef();
      attr.m_eSemantic = semantic;
      attr.m_eFormat = GetEZFormat(paramDesc);
      attr.m_uiLocation = paramDesc.Register;
    }
  }
  else if (Stage == WGALShaderStage::HullShader)
  {
    pShader->m_uiTessellationPatchControlPoints = shaderDesc.cControlPoints;
  }

  for (WUInt32 r = 0; r < shaderDesc.BoundResources; ++r)
  {
    D3D11_SHADER_INPUT_BIND_DESC shaderInputBindDesc;
    pReflector->GetResourceBindingDesc(r, &shaderInputBindDesc);

    // WLog::Info("Bound Resource: '{0}' at slot {1} (Count: {2}, Flags: {3})", sibd.Name, sibd.BindPoint, sibd.BindCount, sibd.uFlags);
    // #TODO_SHADER remove [x] at the end of the name for arrays
    WShaderResourceBinding shaderResourceBinding;
    shaderResourceBinding.m_iBindGroup = 0;
    shaderResourceBinding.m_iSlot = static_cast<WInt16>(shaderInputBindDesc.BindPoint);
    shaderResourceBinding.m_uiArraySize = shaderInputBindDesc.BindCount;
    shaderResourceBinding.m_sName.Assign(shaderInputBindDesc.Name);
    shaderResourceBinding.m_Stages = WGALShaderStageFlags::MakeFromShaderStage(Stage);

    if (shaderInputBindDesc.Type == D3D_SIT_TEXTURE || shaderInputBindDesc.Type == D3D_SIT_UAV_RWTYPED)
    {
      shaderResourceBinding.m_ResourceType = shaderInputBindDesc.Type == D3D_SIT_TEXTURE ? WGALShaderResourceType::Texture : WGALShaderResourceType::TextureRW;
      switch (shaderInputBindDesc.Dimension)
      {
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
          shaderResourceBinding.m_TextureType = WGALShaderTextureType::Texture1D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
          shaderResourceBinding.m_TextureType = WGALShaderTextureType::Texture1DArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
          shaderResourceBinding.m_TextureType = WGALShaderTextureType::Texture2D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
          shaderResourceBinding.m_TextureType = WGALShaderTextureType::Texture2DArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS:
          shaderResourceBinding.m_TextureType = WGALShaderTextureType::Texture2DMS;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
          shaderResourceBinding.m_TextureType = WGALShaderTextureType::Texture2DMSArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D:
          shaderResourceBinding.m_TextureType = WGALShaderTextureType::Texture3D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE:
          shaderResourceBinding.m_TextureType = WGALShaderTextureType::TextureCube;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
          shaderResourceBinding.m_TextureType = WGALShaderTextureType::TextureCubeArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
          shaderResourceBinding.m_ResourceType = shaderInputBindDesc.Type == D3D_SIT_TEXTURE ? WGALShaderResourceType::TexelBuffer : WGALShaderResourceType::TexelBufferRW;
          shaderResourceBinding.m_TextureType = WGALShaderTextureType::Unknown;
          break;

        default:
          W_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }

    else if (shaderInputBindDesc.Type == D3D_SIT_STRUCTURED)
      shaderResourceBinding.m_ResourceType = WGALShaderResourceType::StructuredBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_BYTEADDRESS)
      shaderResourceBinding.m_ResourceType = WGALShaderResourceType::ByteAddressBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
      shaderResourceBinding.m_ResourceType = WGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS)
      shaderResourceBinding.m_ResourceType = WGALShaderResourceType::ByteAddressBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED)
      shaderResourceBinding.m_ResourceType = WGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED)
      shaderResourceBinding.m_ResourceType = WGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER)
      shaderResourceBinding.m_ResourceType = WGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_CBUFFER)
    {
      shaderResourceBinding.m_ResourceType = WGALShaderResourceType::ConstantBuffer;
      shaderResourceBinding.m_pLayout = ReflectConstantBufferLayout(*inout_Data.m_ByteCode[Stage], pReflector->GetConstantBufferByName(shaderInputBindDesc.Name));
    }
    else if (shaderInputBindDesc.Type == D3D_SIT_SAMPLER)
    {
      shaderResourceBinding.m_ResourceType = WGALShaderResourceType::Sampler;
      if (WStringUtils::EndsWith(shaderInputBindDesc.Name, "_AutoSampler"))
      {
        WStringBuilder sb = shaderInputBindDesc.Name;
        sb.Shrink(0, WStringUtils::GetStringElementCount("_AutoSampler"));
        shaderResourceBinding.m_sName.Assign(sb.GetData());
      }
    }
    else
    {
      shaderResourceBinding.m_ResourceType = WGALShaderResourceType::Enum::Unknown;
    }

    if (shaderResourceBinding.m_ResourceType != WGALShaderResourceType::Unknown)
    {
      inout_Data.m_ByteCode[Stage]->m_ShaderResourceBindings.PushBack(shaderResourceBinding);
    }
  }

  pReflector->Release();
}

WSharedPtr<WShaderConstantBufferLayout> WShaderCompilerHLSL::ReflectConstantBufferLayout(WGALShaderByteCode& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection)
{
  D3D11_SHADER_BUFFER_DESC shaderBufferDesc;

  if (FAILED(pConstantBufferReflection->GetDesc(&shaderBufferDesc)))
  {
    return nullptr;
  }

  W_LOG_BLOCK("Constant Buffer Layout", shaderBufferDesc.Name);
  WLog::Debug("Constant Buffer has {0} variables, Size is {1}", shaderBufferDesc.Variables, shaderBufferDesc.Size);

  WSharedPtr<WShaderConstantBufferLayout> pLayout = W_DEFAULT_NEW(WShaderConstantBufferLayout);

  pLayout->m_uiTotalSize = shaderBufferDesc.Size;

  for (WUInt32 var = 0; var < shaderBufferDesc.Variables; ++var)
  {
    ID3D11ShaderReflectionVariable* pVar = pConstantBufferReflection->GetVariableByIndex(var);

    D3D11_SHADER_VARIABLE_DESC svd;
    pVar->GetDesc(&svd);

    W_LOG_BLOCK("Constant", svd.Name);

    D3D11_SHADER_TYPE_DESC std;
    pVar->GetType()->GetDesc(&std);

    WShaderConstant constant;
    constant.m_uiArrayElements = static_cast<WUInt8>(WMath::Max(std.Elements, 1u));
    constant.m_uiOffset = static_cast<WUInt16>(svd.StartOffset);
    constant.m_sName.Assign(svd.Name);

    if (std.Class == D3D_SVC_SCALAR || std.Class == D3D_SVC_VECTOR)
    {
      switch (std.Type)
      {
        case D3D_SVT_FLOAT:
          constant.m_Type = (WShaderConstant::Type::Enum)((WInt32)WShaderConstant::Type::Float1 + std.Columns - 1);
          break;
        case D3D_SVT_INT:
          constant.m_Type = (WShaderConstant::Type::Enum)((WInt32)WShaderConstant::Type::Int1 + std.Columns - 1);
          break;
        case D3D_SVT_UINT:
          constant.m_Type = (WShaderConstant::Type::Enum)((WInt32)WShaderConstant::Type::UInt1 + std.Columns - 1);
          break;
        case D3D_SVT_BOOL:
          if (std.Columns == 1)
          {
            constant.m_Type = WShaderConstant::Type::Bool;
          }
          break;

        default:
          break;
      }
    }
    else if (std.Class == D3D_SVC_MATRIX_COLUMNS)
    {
      if (std.Type != D3D_SVT_FLOAT)
      {
        WLog::Error("Variable '{0}': Only float matrices are supported", svd.Name);
        continue;
      }

      if (std.Columns == 3 && std.Rows == 3)
      {
        constant.m_Type = WShaderConstant::Type::Mat3x3;
      }
      else if (std.Columns == 4 && std.Rows == 4)
      {
        constant.m_Type = WShaderConstant::Type::Mat4x4;
      }
      else
      {
        WLog::Error("Variable '{0}': {1}x{2} matrices are not supported", svd.Name, std.Rows, std.Columns);
        continue;
      }
    }
    else if (std.Class == D3D_SVC_MATRIX_ROWS)
    {
      WLog::Error("Variable '{0}': Row-Major matrices are not supported", svd.Name);
      continue;
    }
    else if (std.Class == D3D_SVC_STRUCT)
    {
      constant.m_Type = WShaderConstant::Type::Struct;
      if (svd.Size == 48 && std.Members == 3)
      {
        WStringView sMember0 = pVar->GetType()->GetMemberTypeName(0);
        WStringView sMember1 = pVar->GetType()->GetMemberTypeName(1);
        WStringView sMember2 = pVar->GetType()->GetMemberTypeName(2);
        if (sMember0 == "r0" && sMember1 == "r1" && sMember2 == "r2")
        {
          constant.m_Type = WShaderConstant::Type::Transform;
        }
      }
    }

    if (constant.m_Type == WShaderConstant::Type::Default)
    {
      WLog::Error("Variable '{0}': Variable type '{1}' is unknown / not supported", svd.Name, std.Class);
      continue;
    }

    pLayout->m_Constants.PushBack(constant);
  }

  return pLayout;
}

WResult WShaderCompilerHLSL::AddFakeBindGroupAssignments(WShaderProgramData& inout_Data, WGALShaderStage::Enum Stage, WLogInterface* pLog)
{
  WMap<WHashedString, const WShaderResourceDefinition*> resourceMap;
  for (const WShaderResourceDefinition& resource : inout_Data.m_Resources[(int)Stage])
  {
    bool bExisted = false;
    auto it = resourceMap.FindOrAdd(resource.m_Binding.m_sName, &bExisted);
    if (!bExisted)
    {
      it.Value() = &resource;
    }
    else
    {
      const WInt16 iCurrentBindGroup = it.Value()->m_Binding.m_iBindGroup != -1 ? it.Value()->m_Binding.m_iBindGroup : 0;
      const WInt16 iNewBindGroup = resource.m_Binding.m_iBindGroup != -1 ? resource.m_Binding.m_iBindGroup : 0;
      if (iCurrentBindGroup != iNewBindGroup)
      {
        WLog::Error(pLog, "Two bindings found with same name but different bind group assignment: A: {}, B: {}. Shader is invalid.", it.Value()->m_sDeclarationAndRegister, resource.m_sDeclarationAndRegister);
        return W_FAILURE;
      }
    }
  }

  for (WShaderResourceBinding& binding : inout_Data.m_ByteCode[(int)Stage]->m_ShaderResourceBindings)
  {
    if (auto it = resourceMap.Find(binding.m_sName); it.IsValid())
    {
      binding.m_iBindGroup = it.Value()->m_Binding.m_iBindGroup != -1 ? it.Value()->m_Binding.m_iBindGroup : 0;
    }
  }
  return W_SUCCESS;
}

const char* GetProfileName(WStringView sPlatform, WGALShaderStage::Enum stage)
{
  if (sPlatform == "DX11_SM40_93")
  {
    switch (stage)
    {
      case WGALShaderStage::VertexShader:
        return "vs_4_0_level_9_3";
      case WGALShaderStage::PixelShader:
        return "ps_4_0_level_9_3";
      default:
        break;
    }
  }

  if (sPlatform == "DX11_SM40")
  {
    switch (stage)
    {
      case WGALShaderStage::VertexShader:
        return "vs_4_0";
      case WGALShaderStage::GeometryShader:
        return "gs_4_0";
      case WGALShaderStage::PixelShader:
        return "ps_4_0";
      case WGALShaderStage::ComputeShader:
        return "cs_4_0";
      default:
        break;
    }
  }

  if (sPlatform == "DX11_SM41")
  {
    switch (stage)
    {
      case WGALShaderStage::GeometryShader:
        return "gs_4_0";
      case WGALShaderStage::VertexShader:
        return "vs_4_1";
      case WGALShaderStage::PixelShader:
        return "ps_4_1";
      case WGALShaderStage::ComputeShader:
        return "cs_4_1";
      default:
        break;
    }
  }

  if (sPlatform == "DX11_SM50")
  {
    switch (stage)
    {
      case WGALShaderStage::VertexShader:
        return "vs_5_0";
      case WGALShaderStage::HullShader:
        return "hs_5_0";
      case WGALShaderStage::DomainShader:
        return "ds_5_0";
      case WGALShaderStage::GeometryShader:
        return "gs_5_0";
      case WGALShaderStage::PixelShader:
        return "ps_5_0";
      case WGALShaderStage::ComputeShader:
        return "cs_5_0";
      default:
        break;
    }
  }

  W_REPORT_FAILURE("Unknown Platform '{0}' or Stage {1}", sPlatform, stage);
  return "";
}

WResult WShaderCompilerHLSL::ModifyShaderSource(WShaderProgramData& inout_data, WLogInterface* pLog)
{
  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    WShaderParser::ParseShaderResources(inout_data.m_sShaderSource[stage], inout_data.m_Resources[stage]);

    // Force material parameter into the material bind group
    for (WShaderResourceDefinition& def : inout_data.m_Resources[stage])
    {
      if (inout_data.m_MaterialParameters.Contains(def.m_Binding.m_sName))
      {
        def.m_Binding.m_iBindGroup = W_GAL_BIND_GROUP_MATERIAL;
      }
    }
  }

  WHashTable<WHashedString, WShaderResourceBinding> bindings;
  W_SUCCEED_OR_RETURN(WShaderParser::MergeShaderResourceBindings(inout_data, bindings, pLog));
  W_SUCCEED_OR_RETURN(DefineShaderResourceBindings(inout_data, bindings, pLog));

  for (auto it : bindings)
  {
    if (it.Value().m_ResourceType == WGALShaderResourceType::ConstantBuffer && it.Value().m_iSlot >= W_GAL_MAX_CONSTANT_BUFFER_COUNT)
    {
      WLog::Error(pLog, "Shader constant buffer resource '{}' has slot index {}. W only supports up to {} slots.", it.Key(), it.Value().m_iSlot, W_GAL_MAX_CONSTANT_BUFFER_COUNT);
      return W_FAILURE;
    }
    if (it.Value().m_ResourceType == WGALShaderResourceType::Sampler && it.Value().m_iSlot >= W_GAL_MAX_SAMPLER_COUNT)
    {
      WLog::Error(pLog, "Shader sampler resource '{}' has slot index {}. W only supports up to {} slots.", it.Key(), it.Value().m_iSlot, W_GAL_MAX_SAMPLER_COUNT);
      return W_FAILURE;
    }
  }

  // Apply shader resource bindings
  WStringBuilder sNewShaderCode;
  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_data.m_sShaderSource[stage].IsEmpty())
      continue;
    WShaderParser::ApplyShaderResourceBindings(inout_data.m_sPlatform, inout_data.m_sShaderSource[stage], inout_data.m_Resources[stage], bindings, WMakeDelegate(&WShaderCompilerHLSL::CreateNewShaderResourceDeclaration, this), sNewShaderCode);
    inout_data.m_sShaderSource[stage] = sNewShaderCode;
  }
  return W_SUCCESS;
}

WResult WShaderCompilerHLSL::Compile(WShaderProgramData& inout_data, WLogInterface* pLog)
{
  Initialize();
  WStringBuilder sFile, sSource;

  for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    // Shader stage not used.
    if (inout_data.m_uiSourceHash[stage] == 0)
      continue;

    // Shader already compiled.
    if (inout_data.m_bWriteToDisk[stage] == false)
    {
      WLog::Debug("Shader for stage '{0}' is already compiled.", WGALShaderStage::Names[stage]);
      continue;
    }

    WStringView sShaderSource = inout_data.m_sShaderSource[stage];
    const WUInt32 uiLength = sShaderSource.GetElementCount();

    if (uiLength > 0 && sShaderSource.FindSubString("main") != nullptr)
    {
      if (CompileDXShader(inout_data.m_sSourceFile.GetData(sFile), sShaderSource.GetData(sSource), inout_data.m_Flags.IsSet(WShaderCompilerFlags::Debug), GetProfileName(inout_data.m_sPlatform, (WGALShaderStage::Enum)stage), "main", inout_data.m_ByteCode[stage]->m_ByteCode).Succeeded())
      {
        ReflectShaderStage(inout_data, (WGALShaderStage::Enum)stage);
        W_SUCCEED_OR_RETURN(AddFakeBindGroupAssignments(inout_data, (WGALShaderStage::Enum)stage, pLog));
      }
      else
      {
        return W_FAILURE;
      }
    }
  }

  return W_SUCCESS;
}

namespace
{
  struct DX11ResourceCategory
  {
    using StorageType = WUInt8;
    static constexpr int ENUM_COUNT = 4;
    enum Enum : WUInt8
    {
      Sampler = W_BIT(0),
      ConstantBuffer = W_BIT(1),
      SRV = W_BIT(2),
      UAV = W_BIT(3),
      Default = 0
    };

    struct Bits
    {
      StorageType Sampler : 1;
      StorageType ConstantBuffer : 1;
      StorageType SRV : 1;
      StorageType UAV : 1;
    };

    static WBitflags<DX11ResourceCategory> MakeFromShaderDescriptorType(WGALShaderResourceType::Enum type);
  };

  W_DECLARE_FLAGS_OPERATORS(DX11ResourceCategory);
} // namespace

inline WBitflags<DX11ResourceCategory> DX11ResourceCategory::MakeFromShaderDescriptorType(WGALShaderResourceType::Enum type)
{
  switch (type)
  {
    case WGALShaderResourceType::Sampler:
      return DX11ResourceCategory::Sampler;
    case WGALShaderResourceType::ConstantBuffer:
    case WGALShaderResourceType::PushConstants:
      return DX11ResourceCategory::ConstantBuffer;
    case WGALShaderResourceType::Texture:
    case WGALShaderResourceType::TexelBuffer:
    case WGALShaderResourceType::StructuredBuffer:
    case WGALShaderResourceType::ByteAddressBuffer:
      return DX11ResourceCategory::SRV;
    case WGALShaderResourceType::TextureRW:
    case WGALShaderResourceType::TexelBufferRW:
    case WGALShaderResourceType::StructuredBufferRW:
    case WGALShaderResourceType::ByteAddressBufferRW:
      return DX11ResourceCategory::UAV;
    case WGALShaderResourceType::TextureAndSampler:
      return DX11ResourceCategory::SRV | DX11ResourceCategory::Sampler;
    default:
      W_REPORT_FAILURE("Missing enum");
      return {};
  }
}

WResult WShaderCompilerHLSL::DefineShaderResourceBindings(const WShaderProgramData& data, WHashTable<WHashedString, WShaderResourceBinding>& inout_resourceBinding, WLogInterface* pLog)
{
  WHybridBitfield<64> indexInUse[DX11ResourceCategory::ENUM_COUNT];
  for (auto it : inout_resourceBinding)
  {
    const WBitflags<DX11ResourceCategory> type = DX11ResourceCategory::MakeFromShaderDescriptorType(it.Value().m_ResourceType);
    // Convert bit to index. We know that only one bit can be set in DX11 as TextureAndSampler is not supported.
    const WUInt32 uiIndex = WMath::FirstBitLow((WUInt32)type.GetValue());
    const WInt16 iSlot = it.Value().m_iSlot;
    if (iSlot != -1)
    {
      indexInUse[uiIndex].SetCount(WMath::Max(indexInUse[uiIndex].GetCount(), static_cast<WUInt32>(iSlot + 1)));
      indexInUse[uiIndex].SetBit(iSlot);
    }
  }

  // Create stable order of resources
  WTempHybridArray<WHashedString, 16> order[DX11ResourceCategory::ENUM_COUNT];
  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (data.m_sShaderSource[stage].IsEmpty())
      continue;

    for (const auto& res : data.m_Resources[stage])
    {
      const WBitflags<DX11ResourceCategory> type = DX11ResourceCategory::MakeFromShaderDescriptorType(res.m_Binding.m_ResourceType);
      const WUInt32 uiIndex = WMath::FirstBitLow((WUInt32)type.GetValue());
      if (!order[uiIndex].Contains(res.m_Binding.m_sName))
      {
        order[uiIndex].PushBack(res.m_Binding.m_sName);
      }
    }
  }

  // W: We only allow constant buffers to be bound globally, so they must all have unique indices.
  // DX11: UAV are bound globally
  // DX11: SRV, Samplers can be bound by stage, so indices can be re-used. Thus, we don't set an index for any of them and let the compiler choose.
  for (auto type : WBitflags<DX11ResourceCategory>(DX11ResourceCategory::UAV | DX11ResourceCategory::ConstantBuffer))
  {
    const WUInt32 uiIndex = WMath::FirstBitLow((WUInt32)type);
    WUInt32 uiCurrentIndex = 0;
    // Workaround for this: error X4509: UAV registers live in the same name space as outputs, so they must be bound to at least u1, manual bind to slot u0 failed
    if (type == DX11ResourceCategory::UAV)
      uiCurrentIndex = 1;

    for (const auto& sName : order[uiIndex])
    {
      while (uiCurrentIndex < indexInUse[uiIndex].GetCount() && indexInUse[uiIndex].IsBitSet(uiCurrentIndex))
      {
        uiCurrentIndex++;
      }
      inout_resourceBinding[sName].m_iSlot = static_cast<WInt16>(uiCurrentIndex);
      indexInUse[uiIndex].SetCount(WMath::Max(indexInUse[uiIndex].GetCount(), uiCurrentIndex + 1));
      indexInUse[uiIndex].SetBit(uiCurrentIndex);
    }
  }

  return W_SUCCESS;
}

void WShaderCompilerHLSL::CreateNewShaderResourceDeclaration(WStringView sPlatform, WStringView sDeclaration, const WShaderResourceBinding& binding, WStringBuilder& out_sDeclaration)
{
  const WBitflags<DX11ResourceCategory> type = DX11ResourceCategory::MakeFromShaderDescriptorType(binding.m_ResourceType);
  WStringView sSemicolon = type.GetValue() != DX11ResourceCategory::ConstantBuffer ? ";" : "";

  WStringView sResourcePrefix;
  if (binding.m_iSlot == -1)
  {
    // Let the compiler choose an index.
    out_sDeclaration.SetFormat("{}{} // Bind Group: {}", sDeclaration, sSemicolon, binding.m_iBindGroup);
    return;
  }

  switch (type.GetValue())
  {
    case DX11ResourceCategory::Sampler:
      sResourcePrefix = "s"_wsv;
      break;
    case DX11ResourceCategory::ConstantBuffer:
      sResourcePrefix = "b"_wsv;
      break;
    case DX11ResourceCategory::SRV:
      sResourcePrefix = "t"_wsv;
      break;
    case DX11ResourceCategory::UAV:
      sResourcePrefix = "u"_wsv;
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }
  out_sDeclaration.SetFormat("{} : register({}{}){} // Bind Group: {}", sDeclaration, sResourcePrefix, binding.m_iSlot, sSemicolon, binding.m_iBindGroup);
}

void WShaderCompilerHLSL::Initialize()
{
  if (m_VertexInputMapping.IsEmpty())
  {
    m_VertexInputMapping["POSITION"] = WGALVertexAttributeSemantic::Position;
    m_VertexInputMapping["NORMAL"] = WGALVertexAttributeSemantic::Normal;
    m_VertexInputMapping["TANGENT"] = WGALVertexAttributeSemantic::Tangent;
    m_VertexInputMapping["BITANGENT"] = WGALVertexAttributeSemantic::BiTangent;
    m_VertexInputMapping["COLOR"] = WGALVertexAttributeSemantic::Color0;
    m_VertexInputMapping["TEXCOORD"] = WGALVertexAttributeSemantic::TexCoord0;
    m_VertexInputMapping["BONEINDICES"] = WGALVertexAttributeSemantic::BoneIndices0;
    m_VertexInputMapping["BONEWEIGHTS"] = WGALVertexAttributeSemantic::BoneWeights0;
    m_VertexInputMapping["DATAOFFSETS"] = WGALVertexAttributeSemantic::DataOffsets;
  }
}

WGALResourceFormat::Enum WShaderCompilerHLSL::GetEZFormat(const _D3D11_SIGNATURE_PARAMETER_DESC& paramDesc)
{
  WUInt32 uiComponents = WMath::Log2i(paramDesc.Mask + 1);
  switch (paramDesc.ComponentType)
  {
    case D3D_REGISTER_COMPONENT_UNKNOWN:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
    case D3D_REGISTER_COMPONENT_UINT32:
      switch (uiComponents)
      {
        case 1:
          return WGALResourceFormat::RUInt;
        case 2:
          return WGALResourceFormat::RGUInt;
        case 3:
          return WGALResourceFormat::RGBUInt;
        case 4:
          return WGALResourceFormat::RGBAUInt;
        default:
          W_ASSERT_NOT_IMPLEMENTED;
          break;
      }
      break;
    case D3D_REGISTER_COMPONENT_SINT32:
      switch (uiComponents)
      {
        case 1:
          return WGALResourceFormat::RInt;
        case 2:
          return WGALResourceFormat::RGInt;
        case 3:
          return WGALResourceFormat::RGBInt;
        case 4:
          return WGALResourceFormat::RGBAInt;
        default:
          W_ASSERT_NOT_IMPLEMENTED;
          break;
      }
      break;
    case D3D_REGISTER_COMPONENT_FLOAT32:
      switch (uiComponents)
      {
        case 1:
          return WGALResourceFormat::RFloat;
        case 2:
          return WGALResourceFormat::RGFloat;
        case 3:
          return WGALResourceFormat::RGBFloat;
        case 4:
          return WGALResourceFormat::RGBAFloat;
        default:
          W_ASSERT_NOT_IMPLEMENTED;
          break;
      }
      break;
    default:
      return WGALResourceFormat::Invalid;
  }
  return WGALResourceFormat::Invalid;
}
