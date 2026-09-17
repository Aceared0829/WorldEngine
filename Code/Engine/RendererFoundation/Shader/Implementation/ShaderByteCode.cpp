#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>
#include <RendererFoundation/Shader/Types.h>

WUInt32 WShaderConstant::s_TypeSize[(WUInt32)Type::ENUM_COUNT] = {0, sizeof(float) * 1, sizeof(float) * 2, sizeof(float) * 3, sizeof(float) * 4, sizeof(int) * 1, sizeof(int) * 2, sizeof(int) * 3, sizeof(int) * 4, sizeof(WUInt32) * 1, sizeof(WUInt32) * 2,
  sizeof(WUInt32) * 3, sizeof(WUInt32) * 4, sizeof(WShaderMat3), sizeof(WMat4), sizeof(WShaderTransform), sizeof(WShaderBool)};

void WShaderConstant::CopyDataFromVariant(WUInt8* pDest, const WVariant* pValue) const
{
  W_ASSERT_DEV(m_uiArrayElements == 1, "Array constants are not supported");

  WResult conversionResult = W_FAILURE;

  if (pValue != nullptr)
  {
    switch (m_Type)
    {
      case Type::Float1:
        *reinterpret_cast<float*>(pDest) = pValue->ConvertTo<float>(&conversionResult);
        break;
      case Type::Float2:
        *reinterpret_cast<WVec2*>(pDest) = pValue->Get<WVec2>();
        return;
      case Type::Float3:
        *reinterpret_cast<WVec3*>(pDest) = pValue->Get<WVec3>();
        return;
      case Type::Float4:
        if (pValue->GetType() == WVariant::Type::Color || pValue->GetType() == WVariant::Type::ColorGamma)
        {
          const WColor tmp = pValue->ConvertTo<WColor>();
          *reinterpret_cast<WVec4*>(pDest) = *reinterpret_cast<const WVec4*>(&tmp);
        }
        else
        {
          *reinterpret_cast<WVec4*>(pDest) = pValue->Get<WVec4>();
        }
        return;

      case Type::Int1:
        *reinterpret_cast<WInt32*>(pDest) = pValue->ConvertTo<WInt32>(&conversionResult);
        break;
      case Type::Int2:
        *reinterpret_cast<WVec2I32*>(pDest) = pValue->Get<WVec2I32>();
        return;
      case Type::Int3:
        *reinterpret_cast<WVec3I32*>(pDest) = pValue->Get<WVec3I32>();
        return;
      case Type::Int4:
        *reinterpret_cast<WVec4I32*>(pDest) = pValue->Get<WVec4I32>();
        return;

      case Type::UInt1:
        *reinterpret_cast<WUInt32*>(pDest) = pValue->ConvertTo<WUInt32>(&conversionResult);
        break;
      case Type::UInt2:
        *reinterpret_cast<WVec2U32*>(pDest) = pValue->Get<WVec2U32>();
        return;
      case Type::UInt3:
        *reinterpret_cast<WVec3U32*>(pDest) = pValue->Get<WVec3U32>();
        return;
      case Type::UInt4:
        *reinterpret_cast<WVec4U32*>(pDest) = pValue->Get<WVec4U32>();
        return;

      case Type::Mat3x3:
        *reinterpret_cast<WShaderMat3*>(pDest) = pValue->Get<WMat3>();
        return;
      case Type::Mat4x4:
        *reinterpret_cast<WMat4*>(pDest) = pValue->Get<WMat4>();
        return;
      case Type::Transform:
        *reinterpret_cast<WShaderTransform*>(pDest) = pValue->Get<WTransform>();
        return;

      case Type::Bool:
        *reinterpret_cast<WShaderBool*>(pDest) = pValue->ConvertTo<bool>(&conversionResult);
        break;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (conversionResult.Succeeded())
  {
    return;
  }

  // WLog::Error("Constant '{0}' is not set, invalid or couldn't be converted to target type and will be set to zero.", m_sName);
  const WUInt32 uiSize = s_TypeSize[m_Type];
  WMemoryUtils::ZeroFill(pDest, uiSize);
}

WResult WShaderResourceBinding::CreateMergedShaderResourceBinding(const WArrayPtr<WArrayPtr<const WShaderResourceBinding>>& resourcesPerStage, WDynamicArray<WShaderResourceBinding>& out_bindings, bool bAllowMultipleBindingPerName)
{
  WUInt32 uiSize = 0;
  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    uiSize += resourcesPerStage[stage].GetCount();
  }

  out_bindings.Clear();
  out_bindings.Reserve(uiSize);

  auto EqualBindings = [](const WShaderResourceBinding& a, const WShaderResourceBinding& b) -> bool
  {
    return a.m_sName == b.m_sName && a.m_ResourceType == b.m_ResourceType && a.m_TextureType == b.m_TextureType && a.m_uiArraySize == b.m_uiArraySize && a.m_iBindGroup == b.m_iBindGroup && a.m_iSlot == b.m_iSlot;
  };

  auto AddOrExtendBinding = [&](WGALShaderStage::Enum stage, WUInt32 uiStartIndex, const WShaderResourceBinding& add)
  {
    for (WUInt32 i = uiStartIndex + 1; i < out_bindings.GetCount(); i++)
    {
      if (EqualBindings(out_bindings[i], add))
      {
        out_bindings[i].m_Stages |= WGALShaderStageFlags::MakeFromShaderStage(stage);
        return;
      }
    }
    WShaderResourceBinding& newBinding = out_bindings.ExpandAndGetRef();
    newBinding = add;
    newBinding.m_Stages |= WGALShaderStageFlags::MakeFromShaderStage(stage);
  };

  WMap<WHashedString, WUInt32> nameToIndex;
  WMap<WHashedString, WUInt32> samplerToIndex;
  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (const WShaderResourceBinding& res : resourcesPerStage[stage])
    {
      WHashedString sName = res.m_sName;

      WUInt32 uiIndex = WInvalidIndex;
      if (res.m_ResourceType == WGALShaderResourceType::Sampler)
      {
        // #TODO_SHADER Samplers are special! Since the shader compiler edits the reflection data and renames "*_AutoSampler" to just "*", we generate a naming collision between the texture and the sampler. See WBindGroupBuilder::BindTexture for binding code. For now, we allow this collision, but it will probably bite us later on.
        samplerToIndex.TryGetValue(res.m_sName, uiIndex);
      }
      else
      {
        nameToIndex.TryGetValue(res.m_sName, uiIndex);
      }

      if (uiIndex != WInvalidIndex)
      {
        WShaderResourceBinding& current = out_bindings[uiIndex];
        if (!EqualBindings(current, res))
        {
          if (bAllowMultipleBindingPerName)
          {
            AddOrExtendBinding((WGALShaderStage::Enum)stage, uiIndex, res);
            continue;
          }
          // #TODO_SHADER better error reporting.
          WLog::Error("A shared shader resource '{}' has a mismatching signatures between stages", sName);
          return W_FAILURE;
        }

        current.m_Stages |= WGALShaderStageFlags::MakeFromShaderStage((WGALShaderStage::Enum)stage);
      }
      else
      {
        WShaderResourceBinding& newBinding = out_bindings.ExpandAndGetRef();
        newBinding = res;
        newBinding.m_Stages |= WGALShaderStageFlags::MakeFromShaderStage((WGALShaderStage::Enum)stage);
        if (res.m_ResourceType == WGALShaderResourceType::Sampler)
        {
          samplerToIndex[res.m_sName] = out_bindings.GetCount() - 1;
        }
        else
        {
          nameToIndex[res.m_sName] = out_bindings.GetCount() - 1;
        }
      }
    }
  }
  out_bindings.Sort([](const WShaderResourceBinding& lhs, const WShaderResourceBinding& rhs)
    {
    if (lhs.m_iBindGroup != rhs.m_iBindGroup)
      return lhs.m_iBindGroup < rhs.m_iBindGroup;

    return lhs.m_iSlot < rhs.m_iSlot; });
  return W_SUCCESS;
}

WGALShaderByteCode::WGALShaderByteCode() = default;

WGALShaderByteCode::~WGALShaderByteCode() = default;

bool WShaderConstantBufferLayout::operator==(const WShaderConstantBufferLayout& rhs) const
{
  if (m_uiTotalSize != rhs.m_uiTotalSize || m_Constants.GetCount() != rhs.m_Constants.GetCount())
    return false;

  const WUInt32 uiCount = m_Constants.GetCount();
  for (WUInt32 i = 0; i < uiCount; ++i)
  {
    const WShaderConstant& a = m_Constants[i];
    const WShaderConstant& b = rhs.m_Constants[i];

    // Some platforms return bool or uint1 for a bool type in a shader.
    WEnum<WShaderConstant::Type> aType = a.m_Type == WShaderConstant::Type::Bool ? WEnum<WShaderConstant::Type>(WShaderConstant::Type::UInt1) : a.m_Type;
    WEnum<WShaderConstant::Type> bType = b.m_Type == WShaderConstant::Type::Bool ? WEnum<WShaderConstant::Type>(WShaderConstant::Type::UInt1) : b.m_Type;

    if (a.m_sName != b.m_sName || aType != bType || a.m_uiArrayElements != b.m_uiArrayElements || a.m_uiOffset != b.m_uiOffset)
      return false;
  }
  return true;
}
