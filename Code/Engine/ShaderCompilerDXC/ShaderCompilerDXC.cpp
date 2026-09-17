#include <ShaderCompilerDXC/ShaderCompilerDXC.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>

#include <SPIRV-Reflect/spirv_reflect.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)
#  include <d3dcompiler.h>
#endif

#include <dxcapi.h>

template <typename T>
struct WComPtr
{
public:
  WComPtr() = default;
  ~WComPtr()
  {
    if (m_pPtr != nullptr)
    {
      m_pPtr->Release();
      m_pPtr = nullptr;
    }
  }

  WComPtr(const WComPtr& other)
    : m_pPtr(other.m_pPtr)
  {
    if (m_pPtr)
    {
      m_pPtr->AddRef();
    }
  }

  T* operator->() { return m_pPtr; }
  T* const operator->() const { return m_pPtr; }

  T** put()
  {
    W_ASSERT_DEV(m_pPtr == nullptr, "Can only put into an empty WComPtr");
    return &m_pPtr;
  }

  bool operator==(nullptr_t)
  {
    return m_pPtr == nullptr;
  }

  bool operator!=(nullptr_t)
  {
    return m_pPtr != nullptr;
  }

private:
  T* m_pPtr = nullptr;
};

WComPtr<IDxcUtils> s_pDxcUtils;
WComPtr<IDxcCompiler3> s_pDxcCompiler;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(ShaderCompilerDXC, ShaderCompilerDXCPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(s_pDxcUtils.put()));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(s_pDxcCompiler.put()));
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pDxcUtils = {};
    s_pDxcCompiler = {};
  }

W_END_SUBSYSTEM_DECLARATION;

W_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(WShaderCompilerDXC, 1)
W_END_ABSTRACT_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStringView WShaderCompilerDXC::GetProfileName(WStringView sPlatform, WGALShaderStage::Enum Stage)
{
  switch (Stage)
  {
    case WGALShaderStage::VertexShader:
      return "vs_6_0";
    case WGALShaderStage::HullShader:
      return "hs_6_0";
    case WGALShaderStage::DomainShader:
      return "ds_6_0";
    case WGALShaderStage::GeometryShader:
      return "gs_6_0";
    case WGALShaderStage::PixelShader:
      return "ps_6_0";
    case WGALShaderStage::ComputeShader:
      return "cs_6_0";
    default:
      break;
  }

  W_REPORT_FAILURE("Unknown Platform '{}' or Stage {}", sPlatform, Stage);
  return "";
}

WResult WShaderCompilerDXC::Initialize()
{
  if (m_VertexInputMapping.IsEmpty())
  {
    m_VertexInputMapping["in.var.POSITION"] = WGALVertexAttributeSemantic::Position;
    m_VertexInputMapping["in.var.NORMAL"] = WGALVertexAttributeSemantic::Normal;
    m_VertexInputMapping["in.var.TANGENT"] = WGALVertexAttributeSemantic::Tangent;

    m_VertexInputMapping["in.var.COLOR0"] = WGALVertexAttributeSemantic::Color0;
    m_VertexInputMapping["in.var.COLOR1"] = WGALVertexAttributeSemantic::Color1;
    m_VertexInputMapping["in.var.COLOR2"] = WGALVertexAttributeSemantic::Color2;
    m_VertexInputMapping["in.var.COLOR3"] = WGALVertexAttributeSemantic::Color3;
    m_VertexInputMapping["in.var.COLOR4"] = WGALVertexAttributeSemantic::Color4;
    m_VertexInputMapping["in.var.COLOR5"] = WGALVertexAttributeSemantic::Color5;
    m_VertexInputMapping["in.var.COLOR6"] = WGALVertexAttributeSemantic::Color6;
    m_VertexInputMapping["in.var.COLOR7"] = WGALVertexAttributeSemantic::Color7;

    m_VertexInputMapping["in.var.TEXCOORD0"] = WGALVertexAttributeSemantic::TexCoord0;
    m_VertexInputMapping["in.var.TEXCOORD1"] = WGALVertexAttributeSemantic::TexCoord1;
    m_VertexInputMapping["in.var.TEXCOORD2"] = WGALVertexAttributeSemantic::TexCoord2;
    m_VertexInputMapping["in.var.TEXCOORD3"] = WGALVertexAttributeSemantic::TexCoord3;
    m_VertexInputMapping["in.var.TEXCOORD4"] = WGALVertexAttributeSemantic::TexCoord4;
    m_VertexInputMapping["in.var.TEXCOORD5"] = WGALVertexAttributeSemantic::TexCoord5;
    m_VertexInputMapping["in.var.TEXCOORD6"] = WGALVertexAttributeSemantic::TexCoord6;
    m_VertexInputMapping["in.var.TEXCOORD7"] = WGALVertexAttributeSemantic::TexCoord7;
    m_VertexInputMapping["in.var.TEXCOORD8"] = WGALVertexAttributeSemantic::TexCoord8;
    m_VertexInputMapping["in.var.TEXCOORD9"] = WGALVertexAttributeSemantic::TexCoord9;

    m_VertexInputMapping["in.var.BITANGENT"] = WGALVertexAttributeSemantic::BiTangent;
    m_VertexInputMapping["in.var.BONEINDICES0"] = WGALVertexAttributeSemantic::BoneIndices0;
    m_VertexInputMapping["in.var.BONEINDICES1"] = WGALVertexAttributeSemantic::BoneIndices1;
    m_VertexInputMapping["in.var.BONEWEIGHTS0"] = WGALVertexAttributeSemantic::BoneWeights0;
    m_VertexInputMapping["in.var.BONEWEIGHTS1"] = WGALVertexAttributeSemantic::BoneWeights1;

    m_VertexInputMapping["in.var.DATAOFFSETS"] = WGALVertexAttributeSemantic::DataOffsets;
  }

  W_ASSERT_DEV(s_pDxcUtils != nullptr && s_pDxcCompiler != nullptr, "ShaderCompiler SubSystem init should have initialized library pointers.");
  return W_SUCCESS;
}

WResult WShaderCompilerDXC::Compile(WShaderProgramData& inout_data, WLogInterface* pLog)
{
  W_SUCCEED_OR_RETURN(Initialize());

  for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_data.m_uiSourceHash[stage] == 0)
      continue;
    if (inout_data.m_bWriteToDisk[stage] == false)
    {
      WLog::Debug("Shader for stage '{}' is already compiled.", WGALShaderStage::Names[stage]);
      continue;
    }

    const WStringBuilder sShaderSource = inout_data.m_sShaderSource[stage];

    if (!sShaderSource.IsEmpty() && sShaderSource.FindSubString("main") != nullptr)
    {
      const WStringBuilder sSourceFile = inout_data.m_sSourceFile;

      if (CompileSPIRVShader(sSourceFile, sShaderSource, inout_data.m_Flags.IsSet(WShaderCompilerFlags::Debug), GetProfileName(inout_data.m_sPlatform, (WGALShaderStage::Enum)stage), "main", inout_data.m_ByteCode[stage]->m_ByteCode).Succeeded())
      {
        W_SUCCEED_OR_RETURN(ReflectShaderStage(inout_data, (WGALShaderStage::Enum)stage));
      }
      else
      {
        return W_FAILURE;
      }
    }
  }

  return W_SUCCESS;
}

void WShaderCompilerDXC::ConfigureDxcArgs(WDynamicArray<WStringWChar>& inout_Args)
{
  inout_Args.PushBack(L"-spirv");
  inout_Args.PushBack(L"-fvk-use-dx-position-w");
  inout_Args.PushBack(L"-fspv-target-env=vulkan1.1");
  // inout_Args.PushBack(L"-fvk-use-dx-layout");
}

WResult WShaderCompilerDXC::CompileSPIRVShader(WStringView sFile, WStringView sSource, bool bDebug, WStringView sProfile, WStringView sEntryPoint, WDynamicArray<WUInt8>& out_ByteCode)
{
  out_ByteCode.Clear();

  WStringView sCompileSource = sSource;
  WStringBuilder sDebugSource;

  WDynamicArray<WStringWChar> args;
  args.PushBack(WStringWChar(sFile));
  args.PushBack(L"-E");
  args.PushBack(WStringWChar(sEntryPoint));
  args.PushBack(L"-T");
  args.PushBack(WStringWChar(sProfile));

  ConfigureDxcArgs(args);

  if (bDebug)
  {
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = sSource;
    sDebugSource.ReplaceAll("#line ", "//ine ");
    sCompileSource = sDebugSource;

    // WLog::Warning("Vulkan DEBUG shader support not really implemented.");

    args.PushBack(L"-Zi"); // Enable debug information.
    // args.PushBack(L"-Fo"); // Optional. Stored in the pdb.
    // args.PushBack(L"myshader.bin");
    // args.PushBack(L"-Fd"); // The file name of the pdb.
    // args.PushBack(L"myshader.pdb");
  }

  WComPtr<IDxcBlobEncoding> pSource;
  s_pDxcUtils->CreateBlob(sCompileSource.GetStartPointer(), sCompileSource.GetElementCount(), DXC_CP_UTF8, pSource.put());

  DxcBuffer Source;
  Source.Ptr = pSource->GetBufferPointer();
  Source.Size = pSource->GetBufferSize();
  Source.Encoding = DXC_CP_UTF8;

  WHybridArray<LPCWSTR, 16> pszArgs;
  pszArgs.SetCount(args.GetCount());
  for (WUInt32 i = 0; i < args.GetCount(); ++i)
  {
    pszArgs[i] = args[i].GetData();
  }

  WComPtr<IDxcResult> pResults;
  s_pDxcCompiler->Compile(&Source, pszArgs.GetData(), pszArgs.GetCount(), nullptr, IID_PPV_ARGS(pResults.put()));

  WComPtr<IDxcBlobUtf8> pErrors;
  pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.put()), nullptr);

  HRESULT hrStatus;
  pResults->GetStatus(&hrStatus);
  if (FAILED(hrStatus))
  {
    WLog::Error("SPIR-V shader compilation failed.");

    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      WLog::Error("{}", WStringUtf8(pErrors->GetStringPointer()).GetData());
    }

    return W_FAILURE;
  }
  else
  {
    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      WLog::Warning("{}", WStringUtf8(pErrors->GetStringPointer()).GetData());
    }
  }

  WComPtr<IDxcBlob> pShader;
  WComPtr<IDxcBlobWide> pShaderName;
  pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pShader.put()), pShaderName.put());

  if (pShader == nullptr)
  {
    WLog::Error("No SPIR-V bytecode was generated.");
    return W_FAILURE;
  }

  out_ByteCode.SetCountUninitialized(static_cast<WUInt32>(pShader->GetBufferSize()));

  WMemoryUtils::Copy(out_ByteCode.GetData(), reinterpret_cast<WUInt8*>(pShader->GetBufferPointer()), out_ByteCode.GetCount());

  return W_SUCCESS;
}

WResult WShaderCompilerDXC::ModifyShaderSource(WShaderProgramData& inout_data, WLogInterface* pLog)
{
  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    WShaderParser::ParseShaderResources(inout_data.m_sShaderSource[stage], inout_data.m_Resources[stage]);
  }

  WHashTable<WHashedString, WShaderResourceBinding> bindings;
  W_SUCCEED_OR_RETURN(WShaderParser::MergeShaderResourceBindings(inout_data, bindings, pLog));
  W_SUCCEED_OR_RETURN(DefineShaderResourceBindings(inout_data, bindings, pLog));
  W_SUCCEED_OR_RETURN(WShaderParser::SanityCheckShaderResourceBindings(bindings, pLog));

  // Apply shader resource bindings
  WStringBuilder sNewShaderCode;
  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_data.m_sShaderSource[stage].IsEmpty())
      continue;
    WShaderParser::ApplyShaderResourceBindings(inout_data.m_sPlatform, inout_data.m_sShaderSource[stage], inout_data.m_Resources[stage], bindings, WMakeDelegate(&WShaderCompilerDXC::CreateNewShaderResourceDeclaration, this), sNewShaderCode);
    inout_data.m_sShaderSource[stage] = sNewShaderCode;
  }
  return W_SUCCESS;
}

WResult WShaderCompilerDXC::DefineShaderResourceBindings(const WShaderProgramData& data, WHashTable<WHashedString, WShaderResourceBinding>& inout_resourceBinding, WLogInterface* pLog)
{
  // Force material parameter into the material bind group
  for (const WString& sMaterialParameter : data.m_MaterialParameters)
  {
    WShaderResourceBinding* pBinding = nullptr;
    if (inout_resourceBinding.TryGetValue(WTempHashedString(sMaterialParameter.GetView()), pBinding))
    {
      pBinding->m_iBindGroup = W_GAL_BIND_GROUP_MATERIAL;
    }
  }

  // Determine which indices are hard-coded in the shader already.
  WHybridArray<WHybridBitfield<64>, 4> slotInUseInSet;
  for (auto it : inout_resourceBinding)
  {
    WInt16& iSet = it.Value().m_iBindGroup;
    if (iSet == -1)
      iSet = 0;

    slotInUseInSet.EnsureCount(iSet + 1);

    if (it.Value().m_iSlot != -1)
    {
      slotInUseInSet[iSet].SetCount(WMath::Max(slotInUseInSet[iSet].GetCount(), static_cast<WUInt32>(it.Value().m_iSlot + 1)));
      slotInUseInSet[iSet].SetBit(it.Value().m_iSlot);
    }
  }

  // Create stable oder of resources in each set.
  WHybridArray<WHybridArray<WHashedString, 16>, 4> orderInSet;
  orderInSet.SetCount(slotInUseInSet.GetCount());
  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (data.m_sShaderSource[stage].IsEmpty())
      continue;

    for (const auto& res : data.m_Resources[stage])
    {
      const WInt16 iSet = res.m_Binding.m_iBindGroup < 0 ? (WInt16)0 : res.m_Binding.m_iBindGroup;
      if (!orderInSet[iSet].Contains(res.m_Binding.m_sName))
      {
        orderInSet[iSet].PushBack(res.m_Binding.m_sName);
      }
    }
  }

  // Do we have _AutoSampler in use? Combine them!
  struct TextureAndSamplerTuple
  {
    WHashTable<WHashedString, WShaderResourceBinding>::Iterator itSampler;
    WHashTable<WHashedString, WShaderResourceBinding>::Iterator itTexture;
  };
  WHybridArray<TextureAndSamplerTuple, 2> autoSamplers;

  if (AllowCombinedImageSamplers())
  {
    for (auto itSampler : inout_resourceBinding)
    {
      if (itSampler.Value().m_ResourceType != WGALShaderResourceType::Sampler || !itSampler.Key().GetView().EndsWith("_AutoSampler"))
        continue;

      WStringBuilder sb = itSampler.Key().GetString();
      sb.TrimWordEnd("_AutoSampler");
      auto itTexture = inout_resourceBinding.Find(WTempHashedString(sb));
      if (!itTexture.IsValid())
        continue;

      if (itSampler.Value().m_iBindGroup != itTexture.Value().m_iBindGroup || itSampler.Value().m_iSlot != itTexture.Value().m_iSlot)
        continue;

      itSampler.Value().m_ResourceType = WGALShaderResourceType::TextureAndSampler;
      itTexture.Value().m_ResourceType = WGALShaderResourceType::TextureAndSampler;
      // Sampler will match the slot of the texture at the end
      orderInSet[itSampler.Value().m_iBindGroup].RemoveAndCopy(itSampler.Key());
      autoSamplers.PushBack({itSampler, itTexture});
    }
  }

  // Assign slot to each resource in each set.
  for (WInt16 iSet = 0; iSet < (WInt16)slotInUseInSet.GetCount(); ++iSet)
  {
    WUInt32 uiCurrentSlot = 0;
    for (const auto& sName : orderInSet[iSet])
    {
      WInt16& iSlot = inout_resourceBinding[sName].m_iSlot;
      if (iSlot != -1)
        continue;
      while (uiCurrentSlot < slotInUseInSet[iSet].GetCount() && slotInUseInSet[iSet].IsBitSet(uiCurrentSlot))
      {
        uiCurrentSlot++;
      }
      iSlot = static_cast<WInt16>(uiCurrentSlot);
      slotInUseInSet[iSet].SetCount(WMath::Max(slotInUseInSet[iSet].GetCount(), uiCurrentSlot + 1));
      slotInUseInSet[iSet].SetBit(uiCurrentSlot);
    }
  }

  // Copy texture assignments to the samplers.
  for (TextureAndSamplerTuple& tas : autoSamplers)
  {
    tas.itSampler.Value().m_iSlot = tas.itTexture.Value().m_iSlot;
  }
  return W_SUCCESS;
}

void WShaderCompilerDXC::CreateNewShaderResourceDeclaration(WStringView sPlatform, WStringView sDeclaration, const WShaderResourceBinding& binding, WStringBuilder& out_sDeclaration)
{
  WBitflags<WGALShaderResourceCategory> type = WGALShaderResourceCategory::MakeFromShaderDescriptorType(binding.m_ResourceType);
  WStringView sResourcePrefix;

  // The only descriptor that can have more than one shader resource type is TextureAndSampler.
  // There will be two declarations in the HLSL code, the sampler and the texture.
  if (binding.m_ResourceType == WGALShaderResourceType::TextureAndSampler)
  {
    type = binding.m_TextureType == WGALShaderTextureType::Unknown ? WGALShaderResourceCategory::Sampler : WGALShaderResourceCategory::TextureSRV;
  }

  switch (type.GetValue())
  {
    case WGALShaderResourceCategory::Sampler:
      sResourcePrefix = "s"_wsv;
      break;
    case WGALShaderResourceCategory::ConstantBuffer:
      sResourcePrefix = "b"_wsv;
      break;
    case WGALShaderResourceCategory::TextureSRV:
    case WGALShaderResourceCategory::BufferSRV:
      sResourcePrefix = "t"_wsv;
      break;
    case WGALShaderResourceCategory::TextureUAV:
    case WGALShaderResourceCategory::BufferUAV:
      sResourcePrefix = "u"_wsv;
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  if (binding.m_ResourceType == WGALShaderResourceType::TextureAndSampler)
  {
    out_sDeclaration.SetFormat("[[vk::combinedImageSampler]] {} : register({}{}, space{})", sDeclaration, sResourcePrefix, binding.m_iSlot, binding.m_iBindGroup);
  }
  else
  {
    out_sDeclaration.SetFormat("{} : register({}{}, space{})", sDeclaration, sResourcePrefix, binding.m_iSlot, binding.m_iBindGroup);
  }
}

WResult WShaderCompilerDXC::FillResourceBinding(WShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.resource_type & SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV)
  {
    return FillSRVResourceBinding(binding, info);
  }
  else if (info.resource_type & SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_UAV)
  {
    return FillUAVResourceBinding(binding, info);
  }
  else if (info.resource_type & SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_CBV)
  {
    binding.m_ResourceType = WGALShaderResourceType::ConstantBuffer;
    binding.m_pLayout = ReflectConstantBufferLayout(info.name, info.block);

    return W_SUCCESS;
  }
  else if (info.resource_type & SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
  {
    binding.m_ResourceType = WGALShaderResourceType::Sampler;

    if (AllowCombinedImageSamplers())
    {
      if (binding.m_sName.GetString().EndsWith("_AutoSampler"))
      {
        WStringBuilder sb = binding.m_sName.GetString();
        sb.TrimWordEnd("_AutoSampler");
        binding.m_sName.Assign(sb);
      }
    }

    return W_SUCCESS;
  }

  WLog::Error("Resource '{}': Unsupported resource type.", info.name);
  return W_FAILURE;
}

WGALShaderTextureType::Enum WShaderCompilerDXC::GetTextureType(const SpvReflectDescriptorBinding& info)
{
  switch (info.image.dim)
  {
    case SpvDim::SpvDim1D:
    {
      if (info.image.ms == 0)
      {
        if (info.image.arrayed > 0)
        {
          return WGALShaderTextureType::Texture1DArray;
        }
        else
        {
          return WGALShaderTextureType::Texture1D;
        }
      }

      break;
    }

    case SpvDim::SpvDim2D:
    {
      if (info.image.ms == 0)
      {
        if (info.image.arrayed > 0)
        {
          return WGALShaderTextureType::Texture2DArray;
        }
        else
        {
          return WGALShaderTextureType::Texture2D;
        }
      }
      else
      {
        if (info.image.arrayed > 0)
        {
          return WGALShaderTextureType::Texture2DMSArray;
        }
        else
        {
          return WGALShaderTextureType::Texture2DMS;
        }
      }

      break;
    }

    case SpvDim::SpvDim3D:
    {
      if (info.image.ms == 0 && info.image.arrayed == 0)
      {
        return WGALShaderTextureType::Texture3D;
      }

      break;
    }

    case SpvDim::SpvDimCube:
    {
      if (info.image.ms == 0)
      {
        if (info.image.arrayed == 0)
        {
          return WGALShaderTextureType::TextureCube;
        }
        else
        {
          return WGALShaderTextureType::TextureCubeArray;
        }
      }

      break;
    }

    case SpvDim::SpvDimBuffer:
      W_ASSERT_NOT_IMPLEMENTED;
      // binding.m_TextureType = WGALShaderTextureType::GenericBuffer;
      return WGALShaderTextureType::Unknown;

    case SpvDim::SpvDimRect:
      W_ASSERT_NOT_IMPLEMENTED;
      return WGALShaderTextureType::Unknown;

    case SpvDim::SpvDimSubpassData:
      W_ASSERT_NOT_IMPLEMENTED;
      return WGALShaderTextureType::Unknown;

    case SpvDim::SpvDimMax:
      W_ASSERT_DEV(false, "Invalid enum value");
      break;

    default:
      break;
  }
  return WGALShaderTextureType::Unknown;
}

WResult WShaderCompilerDXC::FillSRVResourceBinding(WShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    if (info.type_description->op == SpvOp::SpvOpTypeStruct && WStringUtils::StartsWith(info.type_description->type_name, "type.StructuredBuffer"))
    {
      binding.m_pLayout = ReflectStructuredBufferLayout(info.name, info.block);
      binding.m_ResourceType = WGALShaderResourceType::StructuredBuffer;
      return W_SUCCESS;
    }
    else if (info.type_description->op == SpvOp::SpvOpTypeStruct && WStringUtils::StartsWith(info.type_description->type_name, "type.ByteAddressBuffer"))
    {
      binding.m_ResourceType = WGALShaderResourceType::ByteAddressBuffer;
      return W_SUCCESS;
    }
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
  {
    binding.m_ResourceType = WGALShaderResourceType::Texture;
    binding.m_TextureType = GetTextureType(info);
    return W_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
  {
    W_ASSERT_DEV(!binding.m_sName.GetString().EndsWith("_AutoSampler"), "Combined image sampler should have taken the name from the image part");
    binding.m_ResourceType = WGALShaderResourceType::TextureAndSampler;
    binding.m_TextureType = GetTextureType(info);
    return W_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_ResourceType = WGALShaderResourceType::TexelBuffer;
      return W_SUCCESS;
    }

    WLog::Error("Resource '{}': Unsupported texel buffer SRV type.", info.name);
    return W_FAILURE;
  }

  WLog::Error("Resource '{}': Unsupported SRV type.", info.name);
  return W_FAILURE;
}

WResult WShaderCompilerDXC::FillUAVResourceBinding(WShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE)
  {
    binding.m_ResourceType = WGALShaderResourceType::TextureRW;
    binding.m_TextureType = GetTextureType(info);
    return W_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_ResourceType = WGALShaderResourceType::TexelBufferRW;
      return W_SUCCESS;
    }

    WLog::Error("Resource '{}': Unsupported texel buffer UAV type.", info.name);
    return W_FAILURE;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    if (info.type_description->op == SpvOp::SpvOpTypeStruct && WStringUtils::StartsWith(info.type_description->type_name, "type.RWStructuredBuffer"))
    {
      binding.m_pLayout = ReflectStructuredBufferLayout(info.name, info.block);
      binding.m_ResourceType = WGALShaderResourceType::StructuredBufferRW;
      return W_SUCCESS;
    }
    else if (info.type_description->op == SpvOp::SpvOpTypeStruct && WStringUtils::StartsWith(info.type_description->type_name, "type.RWByteAddressBuffer"))
    {
      binding.m_ResourceType = WGALShaderResourceType::ByteAddressBufferRW;
      return W_SUCCESS;
    }
    else if (info.type_description->op == SpvOp::SpvOpTypeStruct && WStringUtils::StartsWith(info.type_description->type_name, "type.AppendStructuredBuffer"))
    {
      // #TODO_VULKAN AppendStructuredBuffer support
      binding.m_ResourceType = WGALShaderResourceType::StructuredBufferRW;
      return W_SUCCESS;
    }
    else if (info.type_description->op == SpvOp::SpvOpTypeStruct && WStringUtils::StartsWith(info.type_description->type_name, "type.ConsumeStructuredBuffer"))
    {
      // #TODO_VULKAN ConsumeStructuredBuffer support
      binding.m_ResourceType = WGALShaderResourceType::StructuredBufferRW;
      return W_SUCCESS;
    }
    else if (info.image.dim == SpvDim::SpvDim1D)
    {
      // #TODO_VULKAN AppendStructuredBuffer / ConsumeStructuredBuffer counter support
      binding.m_ResourceType = WGALShaderResourceType::StructuredBufferRW;
      return W_SUCCESS;
    }
  }

  WLog::Error("Resource '{}': Unsupported UAV type.", info.name);
  return W_FAILURE;
}

WGALResourceFormat::Enum GetEZFormat(SpvReflectFormat format)
{
  switch (format)
  {
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_UINT:
      return WGALResourceFormat::RUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SINT:
      return WGALResourceFormat::RInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SFLOAT:
      return WGALResourceFormat::RFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_UINT:
      return WGALResourceFormat::RGUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SINT:
      return WGALResourceFormat::RGInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SFLOAT:
      return WGALResourceFormat::RGFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_UINT:
      return WGALResourceFormat::RGBUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SINT:
      return WGALResourceFormat::RGBInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
      return WGALResourceFormat::RGBFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
      return WGALResourceFormat::RGBAUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
      return WGALResourceFormat::RGBAInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
      return WGALResourceFormat::RGBAFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_UNDEFINED:
    default:
      return WGALResourceFormat::Invalid;
  }
}

WResult WShaderCompilerDXC::ReflectShaderStage(WShaderProgramData& inout_Data, WGALShaderStage::Enum Stage)
{
  W_LOG_BLOCK("ReflectShaderStage", inout_Data.m_sSourceFile);

  WGALShaderByteCode* pShader = inout_Data.m_ByteCode[Stage];
  const auto& bytecode = pShader->m_ByteCode;

  SpvReflectShaderModule module;

  if (spvReflectCreateShaderModule(bytecode.GetCount(), bytecode.GetData(), &module) != SPV_REFLECT_RESULT_SUCCESS)
  {
    WLog::Error("Extracting shader reflection information failed.");
    return W_FAILURE;
  }

  W_SCOPE_EXIT(spvReflectDestroyShaderModule(&module));

  //
  auto& vertexInputAttributes = pShader->m_ShaderVertexInput;
  if (Stage == WGALShaderStage::VertexShader)
  {
    WUInt32 uiNumVars = 0;
    if (spvReflectEnumerateInputVariables(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      WLog::Error("Failed to retrieve number of input variables.");
      return W_FAILURE;
    }
    WDynamicArray<SpvReflectInterfaceVariable*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumerateInputVariables(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      WLog::Error("Failed to retrieve input variables.");
      return W_FAILURE;
    }

    vertexInputAttributes.Reserve(vars.GetCount());

    for (WUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      SpvReflectInterfaceVariable* pVar = vars[i];
      if (pVar->name != nullptr)
      {
        WShaderVertexInputAttribute& attr = vertexInputAttributes.ExpandAndGetRef();
        attr.m_uiLocation = static_cast<WUInt8>(pVar->location);

        WGALVertexAttributeSemantic::Enum* pVAS = m_VertexInputMapping.GetValue(pVar->name);
        W_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input sematic found: {}", pVar->name);
        attr.m_eSemantic = *pVAS;
        attr.m_eFormat = GetEZFormat(pVar->format);
        W_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input format found: {}", pVar->format);
      }
    }
  }
  else if (Stage == WGALShaderStage::HullShader)
  {
    pShader->m_uiTessellationPatchControlPoints = module.entry_points[0].output_vertices;
  }

  // descriptor bindings
  {
    WUInt32 uiNumVars = 0;
    if (spvReflectEnumerateDescriptorBindings(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      WLog::Error("Failed to retrieve number of descriptor bindings.");
      return W_FAILURE;
    }

    WDynamicArray<SpvReflectDescriptorBinding*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumerateDescriptorBindings(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      WLog::Error("Failed to retrieve descriptor bindings.");
      return W_FAILURE;
    }

    for (WUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      auto& info = *vars[i];

      WLog::Info("Bound Resource: '{}' at slot {} (Count: {})", info.name, info.binding, info.count);

      WShaderResourceBinding shaderResourceBinding;
      shaderResourceBinding.m_iBindGroup = static_cast<WInt16>(info.set);
      shaderResourceBinding.m_iSlot = static_cast<WInt16>(info.binding);
      shaderResourceBinding.m_uiArraySize = info.count;
      shaderResourceBinding.m_sName.Assign(info.name);
      shaderResourceBinding.m_Stages = WGALShaderStageFlags::MakeFromShaderStage(Stage);

      if (FillResourceBinding(shaderResourceBinding, info).Failed())
        continue;

      W_ASSERT_DEV(shaderResourceBinding.m_ResourceType != WGALShaderResourceType::Unknown, "FillResourceBinding should have failed.");

      inout_Data.m_ByteCode[Stage]->m_ShaderResourceBindings.PushBack(shaderResourceBinding);
    }
  }

  // Push Constants
  {
    WUInt32 uiNumVars = 0;
    if (spvReflectEnumeratePushConstantBlocks(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      WLog::Error("Failed to retrieve number of descriptor bindings.");
      return W_FAILURE;
    }

    if (uiNumVars > 1)
    {
      WLog::Error("Only one push constant block is supported right now.");
      return W_FAILURE;
    }

    WDynamicArray<SpvReflectBlockVariable*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumeratePushConstantBlocks(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      WLog::Error("Failed to retrieve descriptor bindings.");
      return W_FAILURE;
    }

    for (WUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      auto& info = *vars[i];

      WStringBuilder sName = info.name;
      sName.TrimWordStart("type.PushConstant.");
      sName.TrimWordEnd("_PushConstants");

      WLog::Info("Push Constants: '{}', Offset: {}, Size: {}", sName, info.offset, info.padded_size);

      if (info.offset != 0)
      {
        WLog::Error("The push constant block '{}' has an offset of '{}', only a zero offset is supported right now. This should be the case if only one block exists", sName, info.offset);
        return W_FAILURE;
      }

      WShaderResourceBinding shaderResourceBinding;
      shaderResourceBinding.m_ResourceType = WGALShaderResourceType::PushConstants;
      shaderResourceBinding.m_iBindGroup = -1;
      shaderResourceBinding.m_iSlot = -1;
      shaderResourceBinding.m_uiArraySize = 1;

      shaderResourceBinding.m_sName.Assign(sName);
      shaderResourceBinding.m_Stages = WGALShaderStageFlags::MakeFromShaderStage(Stage);
      shaderResourceBinding.m_pLayout = ReflectConstantBufferLayout(info.name, info);
      inout_Data.m_ByteCode[Stage]->m_ShaderResourceBindings.PushBack(shaderResourceBinding);
    }
  }

  return W_SUCCESS;
}

WSharedPtr<WShaderConstantBufferLayout> WShaderCompilerDXC::ReflectStructuredBufferLayout(WStringView sName, const SpvReflectBlockVariable& block)
{
  W_LOG_BLOCK("Structured Buffer Layout", sName);
  SpvReflectBlockVariable& innerBlock = block.members[0];
  WLog::Debug("Structured Buffer has {} variables, Size is {}", innerBlock.member_count, innerBlock.padded_size);
  return ReflectBufferLayout(innerBlock);
}

WSharedPtr<WShaderConstantBufferLayout> WShaderCompilerDXC::ReflectConstantBufferLayout(WStringView sName, const SpvReflectBlockVariable& block)
{
  W_LOG_BLOCK("Constant Buffer Layout", sName);
  WLog::Debug("Constant Buffer has {} variables, Size is {}", block.member_count, block.padded_size);
  return ReflectBufferLayout(block);
}

WSharedPtr<WShaderConstantBufferLayout> WShaderCompilerDXC::ReflectBufferLayout(const SpvReflectBlockVariable& block)
{
  WSharedPtr<WShaderConstantBufferLayout> pLayout = W_DEFAULT_NEW(WShaderConstantBufferLayout);

  pLayout->m_uiTotalSize = block.padded_size;

  for (WUInt32 var = 0; var < block.member_count; ++var)
  {
    const auto& svd = block.members[var];

    WShaderConstant constant;
    constant.m_sName.Assign(svd.name);
    constant.m_uiOffset = svd.offset; // TODO: or svd.absolute_offset ??
    constant.m_uiArrayElements = 1;

    WUInt32 uiFlags = svd.type_description->type_flags;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY;

      if (svd.array.dims_count != 1)
      {
        WLog::Error("Variable '{}': Multi-dimensional arrays are not supported.", constant.m_sName);
        continue;
      }

      constant.m_uiArrayElements = svd.array.dims[0];
    }

    WUInt32 uiComponents = 0;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR;

      uiComponents = svd.numeric.vector.component_count;
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL;

      // TODO: unfortunately this never seems to be set, 'bool' types are always exposed as 'int'
      W_ASSERT_NOT_IMPLEMENTED;

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = WShaderConstant::Type::Bool;
          break;

        default:
          WLog::Error("Variable '{}': Multi-component bools are not supported.", constant.m_sName);
          continue;
      }
    }
    else if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT;

      const bool bSigned = svd.type_description->traits.numeric.scalar.signedness != 0;
      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = bSigned ? WShaderConstant::Type::Int1 : WShaderConstant::Type::UInt1;
          break;
        case 2:
          constant.m_Type = bSigned ? WShaderConstant::Type::Int2 : WShaderConstant::Type::UInt2;
          break;
        case 3:
          constant.m_Type = bSigned ? WShaderConstant::Type::Int3 : WShaderConstant::Type::UInt3;
          break;
        case 4:
          constant.m_Type = bSigned ? WShaderConstant::Type::Int4 : WShaderConstant::Type::UInt4;
          break;
      }
    }
    else if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT;

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = WShaderConstant::Type::Float1;
          break;
        case 2:
          constant.m_Type = WShaderConstant::Type::Float2;
          break;
        case 3:
          constant.m_Type = WShaderConstant::Type::Float3;
          break;
        case 4:
          constant.m_Type = WShaderConstant::Type::Float4;
          break;
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX;

      constant.m_Type = WShaderConstant::Type::Default;

      const WUInt32 rows = svd.type_description->traits.numeric.matrix.row_count;
      const WUInt32 columns = svd.type_description->traits.numeric.matrix.column_count;

      if ((svd.type_description->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT) == 0)
      {
        WLog::Error("Variable '{}': Only float matrices are supported", constant.m_sName);
        continue;
      }

      if (columns == 3 && rows == 3)
      {
        constant.m_Type = WShaderConstant::Type::Mat3x3;
      }
      else if (columns == 4 && rows == 4)
      {
        constant.m_Type = WShaderConstant::Type::Mat4x4;
      }
      else
      {
        WLog::Error("Variable '{}': {}x{} matrices are not supported", constant.m_sName, rows, columns);
        continue;
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT;
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK;

      if (svd.size == 48 && svd.member_count == 3 && "r0"_wsv == svd.members[0].name && "r1"_wsv == svd.members[1].name && "r2"_wsv == svd.members[2].name)
      {
        constant.m_Type = WShaderConstant::Type::Transform;
      }
      else
      {
        constant.m_Type = WShaderConstant::Type::Struct;
      }
    }

    if (uiFlags != 0)
    {
      WLog::Error("Variable '{}': Unknown additional type flags '{}'", constant.m_sName, uiFlags);
    }

    if (constant.m_Type == WShaderConstant::Type::Default)
    {
      WLog::Error("Variable '{}': Variable type is unknown / not supported", constant.m_sName);
      continue;
    }

    const char* typeNames[] = {
      "Default",
      "Float1",
      "Float2",
      "Float3",
      "Float4",
      "Int1",
      "Int2",
      "Int3",
      "Int4",
      "UInt1",
      "UInt2",
      "UInt3",
      "UInt4",
      "Mat3x3",
      "Mat4x4",
      "Transform",
      "Bool",
      "Struct",
    };

    if (constant.m_uiArrayElements > 1)
    {
      WLog::Debug("{1} {3}[{2}] {0}", constant.m_sName, WArgU(constant.m_uiOffset, 3, true), constant.m_uiArrayElements, typeNames[constant.m_Type]);
    }
    else
    {
      WLog::Debug("{1} {3} {0}", constant.m_sName, WArgU(constant.m_uiOffset, 3, true), constant.m_uiArrayElements, typeNames[constant.m_Type]);
    }

    pLayout->m_Constants.PushBack(constant);
  }

  return pLayout;
}
