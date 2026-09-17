#include <RendererCore/RendererCorePCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <RendererFoundation/Shader/Types.h>

using namespace WTokenParseUtils;

namespace
{
  static WMutex s_TableLock;
  static WHashTable<WStringView, const WRTTI*> s_NameToTypeTable;
  static WHashTable<WStringView, WEnum<WGALShaderResourceType>> s_NameToDescriptorTable;
  static WHashTable<WStringView, WEnum<WGALShaderTextureType>> s_NameToTextureTable;
  static WHashTable<WStringView, WEnum<WShaderConstant::Type>> s_NameToShaderConstantTable;
  static WDynamicArray<WUInt32> s_ShaderConstantSize;
  static WDynamicArray<WUInt32> s_ShaderConstantScalarSize;

  void InitializeTables()
  {
    W_LOCK(s_TableLock);

    if (!s_NameToTypeTable.IsEmpty())
      return;

    s_NameToTypeTable.Insert("float", WGetStaticRTTI<float>());
    s_NameToTypeTable.Insert("float2", WGetStaticRTTI<WVec2>());
    s_NameToTypeTable.Insert("float3", WGetStaticRTTI<WVec3>());
    s_NameToTypeTable.Insert("float4", WGetStaticRTTI<WVec4>());
    s_NameToTypeTable.Insert("int", WGetStaticRTTI<int>());
    s_NameToTypeTable.Insert("int2", WGetStaticRTTI<WVec2I32>());
    s_NameToTypeTable.Insert("int3", WGetStaticRTTI<WVec3I32>());
    s_NameToTypeTable.Insert("int4", WGetStaticRTTI<WVec4I32>());
    s_NameToTypeTable.Insert("uint", WGetStaticRTTI<WUInt32>());
    s_NameToTypeTable.Insert("uint2", WGetStaticRTTI<WVec2U32>());
    s_NameToTypeTable.Insert("uint3", WGetStaticRTTI<WVec3U32>());
    s_NameToTypeTable.Insert("uint4", WGetStaticRTTI<WVec4U32>());
    s_NameToTypeTable.Insert("bool", WGetStaticRTTI<bool>());
    s_NameToTypeTable.Insert("Color", WGetStaticRTTI<WColor>());
    /// \todo Are we going to support linear UB colors ?
    s_NameToTypeTable.Insert("Texture2D", WGetStaticRTTI<WString>());
    s_NameToTypeTable.Insert("Texture2DArray", WGetStaticRTTI<WString>());
    s_NameToTypeTable.Insert("Texture3D", WGetStaticRTTI<WString>());
    s_NameToTypeTable.Insert("TextureCube", WGetStaticRTTI<WString>());

    s_NameToDescriptorTable.Insert("cbuffer"_wsv, WGALShaderResourceType::ConstantBuffer);
    s_NameToDescriptorTable.Insert("ConstantBuffer"_wsv, WGALShaderResourceType::ConstantBuffer);
    s_NameToDescriptorTable.Insert("SamplerState"_wsv, WGALShaderResourceType::Sampler);
    s_NameToDescriptorTable.Insert("SamplerComparisonState"_wsv, WGALShaderResourceType::Sampler);
    s_NameToDescriptorTable.Insert("Texture1D"_wsv, WGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Texture1DArray"_wsv, WGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Texture2D"_wsv, WGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Texture2DArray"_wsv, WGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Texture2DMS"_wsv, WGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Texture2DMSArray"_wsv, WGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Texture3D"_wsv, WGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("TextureCube"_wsv, WGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("TextureCubeArray"_wsv, WGALShaderResourceType::Texture);
    s_NameToDescriptorTable.Insert("Buffer"_wsv, WGALShaderResourceType::TexelBuffer);
    s_NameToDescriptorTable.Insert("StructuredBuffer"_wsv, WGALShaderResourceType::StructuredBuffer);
    s_NameToDescriptorTable.Insert("ByteAddressBuffer"_wsv, WGALShaderResourceType::ByteAddressBuffer);
    s_NameToDescriptorTable.Insert("RWTexture1D"_wsv, WGALShaderResourceType::TextureRW);
    s_NameToDescriptorTable.Insert("RWTexture1DArray"_wsv, WGALShaderResourceType::TextureRW);
    s_NameToDescriptorTable.Insert("RWTexture2D"_wsv, WGALShaderResourceType::TextureRW);
    s_NameToDescriptorTable.Insert("RWTexture2DArray"_wsv, WGALShaderResourceType::TextureRW);
    s_NameToDescriptorTable.Insert("RWTexture3D"_wsv, WGALShaderResourceType::TextureRW);
    s_NameToDescriptorTable.Insert("RWBuffer"_wsv, WGALShaderResourceType::TexelBufferRW);
    s_NameToDescriptorTable.Insert("RWStructuredBuffer"_wsv, WGALShaderResourceType::StructuredBufferRW);
    s_NameToDescriptorTable.Insert("RWByteAddressBuffer"_wsv, WGALShaderResourceType::ByteAddressBufferRW);
    s_NameToDescriptorTable.Insert("AppendStructuredBuffer"_wsv, WGALShaderResourceType::StructuredBufferRW);
    s_NameToDescriptorTable.Insert("ConsumeStructuredBuffer"_wsv, WGALShaderResourceType::StructuredBufferRW);

    s_NameToTextureTable.Insert("Texture1D"_wsv, WGALShaderTextureType::Texture1D);
    s_NameToTextureTable.Insert("Texture1DArray"_wsv, WGALShaderTextureType::Texture1DArray);
    s_NameToTextureTable.Insert("Texture2D"_wsv, WGALShaderTextureType::Texture2D);
    s_NameToTextureTable.Insert("Texture2DArray"_wsv, WGALShaderTextureType::Texture2DArray);
    s_NameToTextureTable.Insert("Texture2DMS"_wsv, WGALShaderTextureType::Texture2DMS);
    s_NameToTextureTable.Insert("Texture2DMSArray"_wsv, WGALShaderTextureType::Texture2DMSArray);
    s_NameToTextureTable.Insert("Texture3D"_wsv, WGALShaderTextureType::Texture3D);
    s_NameToTextureTable.Insert("TextureCube"_wsv, WGALShaderTextureType::TextureCube);
    s_NameToTextureTable.Insert("TextureCubeArray"_wsv, WGALShaderTextureType::TextureCubeArray);
    s_NameToTextureTable.Insert("RWTexture1D"_wsv, WGALShaderTextureType::Texture1D);
    s_NameToTextureTable.Insert("RWTexture1DArray"_wsv, WGALShaderTextureType::Texture1DArray);
    s_NameToTextureTable.Insert("RWTexture2D"_wsv, WGALShaderTextureType::Texture2D);
    s_NameToTextureTable.Insert("RWTexture2DArray"_wsv, WGALShaderTextureType::Texture2DArray);
    s_NameToTextureTable.Insert("RWTexture3D"_wsv, WGALShaderTextureType::Texture3D);

    s_NameToShaderConstantTable.Insert("FLOAT1"_wsv, WShaderConstant::Type::Float1);
    s_NameToShaderConstantTable.Insert("FLOAT2"_wsv, WShaderConstant::Type::Float2);
    s_NameToShaderConstantTable.Insert("FLOAT3"_wsv, WShaderConstant::Type::Float3);
    s_NameToShaderConstantTable.Insert("FLOAT4"_wsv, WShaderConstant::Type::Float4);
    s_NameToShaderConstantTable.Insert("INT1"_wsv, WShaderConstant::Type::Int1);
    s_NameToShaderConstantTable.Insert("INT2"_wsv, WShaderConstant::Type::Int2);
    s_NameToShaderConstantTable.Insert("INT3"_wsv, WShaderConstant::Type::Int3);
    s_NameToShaderConstantTable.Insert("INT4"_wsv, WShaderConstant::Type::Int4);
    s_NameToShaderConstantTable.Insert("UINT1"_wsv, WShaderConstant::Type::UInt1);
    s_NameToShaderConstantTable.Insert("UINT2"_wsv, WShaderConstant::Type::UInt2);
    s_NameToShaderConstantTable.Insert("UINT3"_wsv, WShaderConstant::Type::UInt3);
    s_NameToShaderConstantTable.Insert("UINT4"_wsv, WShaderConstant::Type::UInt4);
    s_NameToShaderConstantTable.Insert("MAT3"_wsv, WShaderConstant::Type::Mat3x3);
    s_NameToShaderConstantTable.Insert("MAT4"_wsv, WShaderConstant::Type::Mat4x4);
    s_NameToShaderConstantTable.Insert("TRANSFORM"_wsv, WShaderConstant::Type::Transform);
    s_NameToShaderConstantTable.Insert("COLOR4F"_wsv, WShaderConstant::Type::Float4);
    s_NameToShaderConstantTable.Insert("BOOL1"_wsv, WShaderConstant::Type::Bool);
    // Handled separately
    // s_NameToShaderConstantTable.Insert("PACKEDHALF2"_wsv, WShaderConstant::Type::UInt1);
    // s_NameToShaderConstantTable.Insert("PACKEDCOLOR4H"_wsv, WShaderConstant::Type::Bool);

    s_ShaderConstantSize.SetCount(WShaderConstant::Type::ENUM_COUNT);
    s_ShaderConstantSize[WShaderConstant::Type::Float1] = sizeof(float);
    s_ShaderConstantSize[WShaderConstant::Type::Float2] = sizeof(WVec2);
    s_ShaderConstantSize[WShaderConstant::Type::Float3] = sizeof(WVec3);
    s_ShaderConstantSize[WShaderConstant::Type::Float4] = sizeof(WVec4);
    s_ShaderConstantSize[WShaderConstant::Type::Int1] = sizeof(WInt32);
    s_ShaderConstantSize[WShaderConstant::Type::Int2] = sizeof(WVec2I32);
    s_ShaderConstantSize[WShaderConstant::Type::Int3] = sizeof(WVec3I32);
    s_ShaderConstantSize[WShaderConstant::Type::Int4] = sizeof(WVec4I32);
    s_ShaderConstantSize[WShaderConstant::Type::UInt1] = sizeof(WUInt32);
    s_ShaderConstantSize[WShaderConstant::Type::UInt2] = sizeof(WVec2U32);
    s_ShaderConstantSize[WShaderConstant::Type::UInt3] = sizeof(WVec3U32);
    s_ShaderConstantSize[WShaderConstant::Type::UInt4] = sizeof(WVec4U32);
    s_ShaderConstantSize[WShaderConstant::Type::Mat3x3] = sizeof(WShaderMat3);
    s_ShaderConstantSize[WShaderConstant::Type::Mat4x4] = sizeof(WShaderMat4);
    s_ShaderConstantSize[WShaderConstant::Type::Transform] = sizeof(WShaderTransform);
    s_ShaderConstantSize[WShaderConstant::Type::Bool] = sizeof(WShaderBool);
    s_ShaderConstantSize[WShaderConstant::Type::Struct] = 0;

    s_ShaderConstantScalarSize.SetCount(WShaderConstant::Type::ENUM_COUNT);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Float1] = sizeof(float);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Float2] = sizeof(float);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Float3] = sizeof(float);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Float4] = sizeof(float);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Int1] = sizeof(WInt32);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Int2] = sizeof(WInt32);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Int3] = sizeof(WInt32);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Int4] = sizeof(WInt32);
    s_ShaderConstantScalarSize[WShaderConstant::Type::UInt1] = sizeof(WUInt32);
    s_ShaderConstantScalarSize[WShaderConstant::Type::UInt2] = sizeof(WUInt32);
    s_ShaderConstantScalarSize[WShaderConstant::Type::UInt3] = sizeof(WUInt32);
    s_ShaderConstantScalarSize[WShaderConstant::Type::UInt4] = sizeof(WUInt32);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Mat3x3] = sizeof(float);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Mat4x4] = sizeof(float);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Transform] = sizeof(float);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Bool] = sizeof(WShaderBool);
    s_ShaderConstantScalarSize[WShaderConstant::Type::Struct] = 0;
  }

  const WRTTI* GetType(const char* szType)
  {
    InitializeTables();

    const WRTTI* pType = nullptr;
    s_NameToTypeTable.TryGetValue(szType, pType);
    return pType;
  }

  WVariant ParseValue(const TokenStream& tokens, WUInt32& ref_uiCurToken)
  {
    WUInt32 uiValueToken = ref_uiCurToken;

    if (Accept(tokens, ref_uiCurToken, WTokenType::String1, &uiValueToken) || Accept(tokens, ref_uiCurToken, WTokenType::String2, &uiValueToken))
    {
      WStringBuilder sValue = tokens[uiValueToken]->m_DataView;
      sValue.Trim("\"'");

      return WVariant(sValue.GetData());
    }

    bool bValueIsNegative = false;
    if (Accept(tokens, ref_uiCurToken, "-"))
    {
      bValueIsNegative = true;
    }

    if (Accept(tokens, ref_uiCurToken, WTokenType::Integer, &uiValueToken))
    {
      WString sValue = tokens[uiValueToken]->m_DataView;

      WInt64 iValue = 0;
      if (sValue.StartsWith_NoCase("0x"))
      {
        WUInt32 uiValue32 = 0;
        WConversionUtils::ConvertHexStringToUInt32(sValue, uiValue32).IgnoreResult();

        iValue = uiValue32;
      }
      else
      {
        WConversionUtils::StringToInt64(sValue, iValue).IgnoreResult();
      }

      return WVariant(bValueIsNegative ? -iValue : iValue);
    }

    if (Accept(tokens, ref_uiCurToken, WTokenType::Float, &uiValueToken))
    {
      WString sValue = tokens[uiValueToken]->m_DataView;

      double fValue = 0;
      WConversionUtils::StringToFloat(sValue, fValue).IgnoreResult();

      return WVariant(bValueIsNegative ? -fValue : fValue);
    }

    if (Accept(tokens, ref_uiCurToken, "true", &uiValueToken) || Accept(tokens, ref_uiCurToken, "false", &uiValueToken))
    {
      bool bValue = tokens[uiValueToken]->m_DataView == "true";
      return WVariant(bValue);
    }

    auto& dataView = tokens[ref_uiCurToken]->m_DataView;
    if (tokens[ref_uiCurToken]->m_iType == WTokenType::Identifier && WStringUtils::IsValidIdentifierName(dataView.GetStartPointer(), dataView.GetEndPointer()))
    {
      // complex type constructor
      const WRTTI* pType = nullptr;
      if (!s_NameToTypeTable.TryGetValue(dataView, pType))
      {
        WLog::Error("Invalid type name '{}'", dataView);
        return WVariant();
      }

      ++ref_uiCurToken;
      Accept(tokens, ref_uiCurToken, "(");

      WTempHybridArray<WVariant, 8> constructorArgs;

      while (!Accept(tokens, ref_uiCurToken, ")"))
      {
        WVariant value = ParseValue(tokens, ref_uiCurToken);
        if (value.IsValid())
        {
          constructorArgs.PushBack(value);
        }
        else
        {
          WLog::Error("Invalid arguments for constructor '{}'", pType->GetTypeName());
          return W_FAILURE;
        }

        Accept(tokens, ref_uiCurToken, ",");
      }

      // find matching constructor
      auto functions = pType->GetFunctions();
      for (auto pFunc : functions)
      {
        if (pFunc->GetFunctionType() == WFunctionType::Constructor && pFunc->GetArgumentCount() == constructorArgs.GetCount())
        {
          WTempHybridArray<WVariant, 8> convertedArgs;
          bool bAllArgsValid = true;

          for (WUInt32 uiArg = 0; uiArg < pFunc->GetArgumentCount(); ++uiArg)
          {
            const WRTTI* pArgType = pFunc->GetArgumentType(uiArg);
            WResult conversionResult = W_FAILURE;
            convertedArgs.PushBack(constructorArgs[uiArg].ConvertTo(pArgType->GetVariantType(), &conversionResult));
            if (conversionResult.Failed())
            {
              bAllArgsValid = false;
              break;
            }
          }

          if (bAllArgsValid)
          {
            WVariant result;
            pFunc->Execute(nullptr, convertedArgs, result);

            if (result.IsValid())
            {
              return result;
            }
          }
        }
      }
    }

    return WVariant();
  }

  WResult ParseAttribute(const TokenStream& tokens, WUInt32& ref_uiCurToken, WShaderParser::ParameterDefinition& out_parameterDefinition)
  {
    if (!Accept(tokens, ref_uiCurToken, "@"))
    {
      return W_FAILURE;
    }

    WUInt32 uiTypeToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, WTokenType::Identifier, &uiTypeToken))
    {
      return W_FAILURE;
    }

    WShaderParser::AttributeDefinition& attributeDef = out_parameterDefinition.m_Attributes.ExpandAndGetRef();
    attributeDef.m_sType = tokens[uiTypeToken]->m_DataView;

    Accept(tokens, ref_uiCurToken, "(");

    while (!Accept(tokens, ref_uiCurToken, ")"))
    {
      WVariant value = ParseValue(tokens, ref_uiCurToken);
      if (value.IsValid())
      {
        attributeDef.m_Values.PushBack(value);
      }
      else
      {
        WLog::Error("Invalid arguments for attribute '{}'", attributeDef.m_sType);
        return W_FAILURE;
      }

      Accept(tokens, ref_uiCurToken, ",");
    }

    return W_SUCCESS;
  }

  WResult ParseParameter(const TokenStream& tokens, WUInt32& ref_uiCurToken, WShaderParser::ParameterDefinition& out_parameterDefinition)
  {
    WUInt32 uiTypeToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, WTokenType::Identifier, &uiTypeToken))
    {
      return W_FAILURE;
    }

    out_parameterDefinition.m_sType = tokens[uiTypeToken]->m_DataView;
    out_parameterDefinition.m_pType = GetType(out_parameterDefinition.m_sType);

    WUInt32 uiNameToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, WTokenType::Identifier, &uiNameToken))
    {
      return W_FAILURE;
    }

    out_parameterDefinition.m_sName = tokens[uiNameToken]->m_DataView;

    while (!Accept(tokens, ref_uiCurToken, ";"))
    {
      if (ParseAttribute(tokens, ref_uiCurToken, out_parameterDefinition).Failed())
      {
        return W_FAILURE;
      }
    }

    return W_SUCCESS;
  }

  WResult ParseEnum(const TokenStream& tokens, WUInt32& ref_uiCurToken, WShaderParser::EnumDefinition& out_enumDefinition, bool bCheckPrefix)
  {
    if (!Accept(tokens, ref_uiCurToken, "enum"))
    {
      return W_FAILURE;
    }

    WUInt32 uiNameToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, WTokenType::Identifier, &uiNameToken))
    {
      return W_FAILURE;
    }

    out_enumDefinition.m_sName = tokens[uiNameToken]->m_DataView;
    WStringBuilder sEnumPrefix(out_enumDefinition.m_sName, "_");

    if (!Accept(tokens, ref_uiCurToken, "{"))
    {
      WLog::Error("Opening bracket expected for enum definition.");
      return W_FAILURE;
    }

    WUInt32 uiDefaultValue = 0;
    WUInt32 uiCurrentValue = 0;

    while (true)
    {
      WUInt32 uiValueNameToken = ref_uiCurToken;
      if (!Accept(tokens, ref_uiCurToken, WTokenType::Identifier, &uiValueNameToken))
      {
        return W_FAILURE;
      }

      WStringView sValueName = tokens[uiValueNameToken]->m_DataView;

      if (Accept(tokens, ref_uiCurToken, "="))
      {
        WUInt32 uiValueToken = ref_uiCurToken;
        Accept(tokens, ref_uiCurToken, WTokenType::Integer, &uiValueToken);

        WInt32 iValue = 0;
        if (WConversionUtils::StringToInt(tokens[uiValueToken]->m_DataView, iValue).Succeeded() && iValue >= 0)
        {
          uiCurrentValue = iValue;
        }
        else
        {
          WLog::Error("Invalid enum value '{0}'. Only positive numbers are allowed.", tokens[uiValueToken]->m_DataView);
        }
      }

      if (sValueName.IsEqual_NoCase("default"))
      {
        uiDefaultValue = uiCurrentValue;
      }
      else
      {
        if (bCheckPrefix && !sValueName.StartsWith(sEnumPrefix))
        {
          WLog::Error("Enum value does not start with the expected enum name as prefix: '{0}'", sEnumPrefix);
        }

        auto& ev = out_enumDefinition.m_Values.ExpandAndGetRef();

        const WStringBuilder sFinalName = sValueName;
        ev.m_sValueName.Assign(sFinalName.GetData());
        ev.m_iValueValue = static_cast<WInt32>(uiCurrentValue);
      }

      if (Accept(tokens, ref_uiCurToken, ","))
      {
        ++uiCurrentValue;
      }
      else
      {
        break;
      }

      if (Accept(tokens, ref_uiCurToken, "}"))
        goto after_braces;
    }

    if (!Accept(tokens, ref_uiCurToken, "}"))
    {
      WLog::Error("Closing bracket expected for enum definition.");
      return W_FAILURE;
    }

  after_braces:

    out_enumDefinition.m_uiDefaultValue = uiDefaultValue;

    Accept(tokens, ref_uiCurToken, ";");

    return W_SUCCESS;
  }

  void SkipWhitespace(WStringView& s)
  {
    while (s.IsValid() && WStringUtils::IsWhiteSpace(s.GetCharacter()))
    {
      ++s;
    }
  }

  void SkipWhitespaceAndComments(WStringView& s)
  {
    while (true)
    {
      SkipWhitespace(s);

      if (s.StartsWith("//"))
      {
        while (s.IsValid() && s.GetCharacter() != '\n')
          ++s;
      }
      else if (s.StartsWith("/*"))
      {
        s.Shrink(2, 0);

        while (s.IsValid() && !s.StartsWith("*/"))
          ++s;

        if (s.StartsWith("*/"))
          s.Shrink(2, 0);
      }
      else
      {
        return;
      }
    }
  }
} // namespace

WResult WShaderParser::PreprocessSection(WStringView sSectionContent, WArrayPtr<WString> customDefines, WStringBuilder& out_sResult)
{
  WPreprocessor pp;
  pp.SetPassThroughPragma(false);
  pp.SetPassThroughLine(false);

  // setup defines
  {
    W_SUCCEED_OR_RETURN(pp.AddCustomDefine("TRUE 1"));
    W_SUCCEED_OR_RETURN(pp.AddCustomDefine("FALSE 0"));
    W_SUCCEED_OR_RETURN(pp.AddCustomDefine("PLATFORM_SHADER ="));

    for (auto& sDefine : customDefines)
    {
      W_SUCCEED_OR_RETURN(pp.AddCustomDefine(sDefine));
    }
  }

  pp.SetFileOpenFunction([&](WStringView sAbsoluteFile, WDynamicArray<WUInt8>& out_fileContent, WTimestamp& out_fileModification)
    {
        if (sAbsoluteFile == "SectionContent")
        {
          out_fileContent.PushBackRange(WMakeArrayPtr((const WUInt8*)sSectionContent.GetStartPointer(), sSectionContent.GetElementCount()));
          return W_SUCCESS;
        }

        WFileReader r;
        if (r.Open(sAbsoluteFile).Failed())
        {
          WLog::Error("Could not find include file '{0}'", sAbsoluteFile);
          return W_FAILURE;
        }

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
        WFileStats stats;
        if (WFileSystem::GetFileStats(sAbsoluteFile, stats).Succeeded())
        {
          out_fileModification = stats.m_LastModificationTime;
        }
#endif

        WUInt8 Temp[4096];
        while (WUInt64 uiRead = r.ReadBytes(Temp, 4096))
        {
          out_fileContent.PushBackRange(WArrayPtr<WUInt8>(Temp, (WUInt32)uiRead));
        }

        return W_SUCCESS; });

  bool bFoundUndefinedVars = false;
  pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars](const WPreprocessor::ProcessingEvent& e)
    {
        if (e.m_Type == WPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          WLog::Error("Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}. Only material permutation variables are allowed in material config sections.", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        } });

  if (pp.Process("SectionContent", out_sResult, false).Failed() || bFoundUndefinedVars)
  {
    return W_FAILURE;
  }

  return W_SUCCESS;
}

// static
void WShaderParser::ParseMaterialParameterSection(WStringView sSection, WDynamicArray<ParameterDefinition>& out_parameter, WDynamicArray<EnumDefinition>& out_enumDefinitions)
{
  WStringBuilder sPreprocessedSection;
  if (WShaderParser::PreprocessSection(sSection, WArrayPtr<WString>(), sPreprocessedSection).Failed())
  {
    WLog::Error("Preprocessing material parameter section failed.");
    return;
  }

  WTokenizer tokenizer;
  tokenizer.Tokenize(WMakeArrayPtr((const WUInt8*)sPreprocessedSection.GetData(), sPreprocessedSection.GetElementCount()), WLog::GetThreadLocalLogSystem(), false);

  TokenStream tokens;
  tokenizer.GetAllLines(tokens);

  WUInt32 uiCurToken = 0;

  while (!Accept(tokens, uiCurToken, WTokenType::EndOfFile))
  {
    EnumDefinition enumDef;
    if (ParseEnum(tokens, uiCurToken, enumDef, false).Succeeded())
    {
      W_ASSERT_DEV(!enumDef.m_sName.IsEmpty(), "");

      out_enumDefinitions.PushBack(std::move(enumDef));
      continue;
    }

    ParameterDefinition paramDef;
    if (ParseParameter(tokens, uiCurToken, paramDef).Succeeded())
    {
      out_parameter.PushBack(std::move(paramDef));
      continue;
    }

    WLog::Error("Invalid token in material parameter section '{}'", tokens[uiCurToken]->m_DataView);
    break;
  }
}

// static
void WShaderParser::ParsePermutationSection(WStringView s, WDynamicArray<WHashedString>& out_permVars, WDynamicArray<WPermutationVar>& out_fixedPermVars)
{
  out_permVars.Clear();
  out_fixedPermVars.Clear();

  WTokenizer tokenizer;
  tokenizer.Tokenize(WArrayPtr<const WUInt8>((const WUInt8*)s.GetStartPointer(), s.GetElementCount()), WLog::GetThreadLocalLogSystem(), false);

  enum class State
  {
    Idle,
    HasName,
    HasEqual,
    HasValue
  };

  State state = State::Idle;
  WStringBuilder sToken, sVarName;

  for (const auto& token : tokenizer.GetTokens())
  {
    if (token.m_iType == WTokenType::Whitespace || token.m_iType == WTokenType::BlockComment || token.m_iType == WTokenType::LineComment)
      continue;

    if (token.m_iType == WTokenType::String1 || token.m_iType == WTokenType::String2 || token.m_iType == WTokenType::RawString1)
    {
      sToken = token.m_DataView;
      WLog::Error("Strings are not allowed in the permutation section: '{0}'", sToken);
      return;
    }

    if (token.m_iType == WTokenType::Newline || token.m_iType == WTokenType::EndOfFile)
    {
      if (state == State::HasEqual)
      {
        WLog::Error("Missing assignment value in permutation section");
        return;
      }

      if (state == State::HasName)
      {
        out_permVars.ExpandAndGetRef().Assign(sVarName.GetData());
      }

      state = State::Idle;
      continue;
    }

    sToken = token.m_DataView;

    if (token.m_iType == WTokenType::NonIdentifier)
    {
      if (sToken == "=" && state == State::HasName)
      {
        state = State::HasEqual;
        continue;
      }
    }
    else if (token.m_iType == WTokenType::Identifier)
    {
      if (state == State::Idle)
      {
        sVarName = sToken;
        state = State::HasName;
        continue;
      }

      if (state == State::HasEqual)
      {
        auto& res = out_fixedPermVars.ExpandAndGetRef();
        res.m_sName.Assign(sVarName.GetData());
        res.m_sValue.Assign(sToken.GetData());
        state = State::HasValue;
        continue;
      }
    }

    WLog::Error("Invalid permutation section at token '{0}'", sToken);
  }
}

WStatus ParseShaderConstant(const TokenStream& tokens, WUInt32& ref_uiCurToken, WShaderConstantBufferLayout& ref_materialConstantBufferLayout)
{
  WTempHybridArray<WUInt32, 8> acceptedTokens;
  TokenMatch constantPattern[] = {WTokenType::Identifier, "("_wsv, WTokenType::Identifier, ")"_wsv, ";"_wsv};
  TokenMatch packedhalf2Pattern[] = {WTokenType::Identifier, "("_wsv, WTokenType::Identifier, ","_wsv, WTokenType::Identifier, ","_wsv, WTokenType::Identifier, ")"_wsv, ";"_wsv};
  const WUInt32 uiStartToken = ref_uiCurToken;
  if (Accept(tokens, ref_uiCurToken, constantPattern, &acceptedTokens))
  {
    const WUInt32 uiNameToken = acceptedTokens[2];
    const WUInt32 uiTypeToken = acceptedTokens[0];

    WEnum<WShaderConstant::Type> type;
    if (s_NameToShaderConstantTable.TryGetValue(tokens[uiTypeToken]->m_DataView, type))
    {
      WShaderConstant& out_shaderConstant = ref_materialConstantBufferLayout.m_Constants.ExpandAndGetRef();
      out_shaderConstant.m_Type = type;
      out_shaderConstant.m_sName.Assign(tokens[uiNameToken]->m_DataView);
      out_shaderConstant.m_uiArrayElements = 1;
      out_shaderConstant.m_uiOffset = 0;
      return WStatus(W_SUCCESS);
    }
    else if (tokens[uiNameToken]->m_DataView == "PACKEDCOLOR4H")
    {
      {
        WShaderConstant& out_shaderConstant = ref_materialConstantBufferLayout.m_Constants.ExpandAndGetRef();
        out_shaderConstant.m_Type = WShaderConstant::Type::UInt1;
        WStringBuilder sNameRG(tokens[uiNameToken]->m_DataView, "RG"_wsv);
        out_shaderConstant.m_sName.Assign(sNameRG.GetView());
        out_shaderConstant.m_uiArrayElements = 1;
        out_shaderConstant.m_uiOffset = 0;
      }
      {
        WShaderConstant& out_shaderConstant = ref_materialConstantBufferLayout.m_Constants.ExpandAndGetRef();
        out_shaderConstant.m_Type = WShaderConstant::Type::UInt1;
        WStringBuilder sNameRG(tokens[uiNameToken]->m_DataView, "GB"_wsv);
        out_shaderConstant.m_sName.Assign(sNameRG.GetView());
        out_shaderConstant.m_uiArrayElements = 1;
        out_shaderConstant.m_uiOffset = 0;
      }
      return WStatus(W_SUCCESS);
    }

    return WStatus(WFmt("Unknown shader constant type: {}", tokens[uiTypeToken]->m_DataView));
  }
  else if (Accept(tokens, ref_uiCurToken, packedhalf2Pattern, &acceptedTokens))
  {
    const WUInt32 uiNameToken = acceptedTokens[6];

    WShaderConstant& out_shaderConstant = ref_materialConstantBufferLayout.m_Constants.ExpandAndGetRef();
    out_shaderConstant.m_Type = WShaderConstant::Type::UInt1;
    out_shaderConstant.m_sName.Assign(tokens[uiNameToken]->m_DataView);
    out_shaderConstant.m_uiArrayElements = 1;
    out_shaderConstant.m_uiOffset = 0;
    return WStatus(W_SUCCESS);
  }
  return WStatus(WFmt("Unknown shader constant"));
}

WUInt32 AlignSize(WUInt32 uiValue, WUInt32 uiAlignment)
{
  const WUInt32 uiRemainder = uiValue % uiAlignment;
  return uiRemainder == 0 ? uiValue : uiValue + uiAlignment - uiRemainder;
}

void AlignConstantBufferDX(WShaderConstantBufferLayout& ref_materialConstantBufferLayout)
{
  WUInt32 uiCurrentOffset = 0;
  for (WShaderConstant& constant : ref_materialConstantBufferLayout.m_Constants)
  {
    const WUInt32 uiScalarSize = s_ShaderConstantScalarSize[constant.m_Type];
    const WUInt32 uiConstantSize = s_ShaderConstantSize[constant.m_Type];
    uiCurrentOffset = AlignSize(uiCurrentOffset, uiScalarSize);
    const WUInt32 uiStartBucket = uiCurrentOffset / 16;
    const WUInt32 uiEndBucket = (uiCurrentOffset + uiConstantSize - 1) / 16;
    // Check if the constant is crossing a 16 byte boundary
    if (uiStartBucket != uiEndBucket)
    {
      uiCurrentOffset = AlignSize(uiCurrentOffset, 16);
    }
    constant.m_uiOffset = uiCurrentOffset;
    uiCurrentOffset += uiConstantSize;
  }

  uiCurrentOffset = AlignSize(uiCurrentOffset, 16);
  ref_materialConstantBufferLayout.m_uiTotalSize = uiCurrentOffset;
}

void AlignStructuredBufferDX(WShaderConstantBufferLayout& ref_materialConstantBufferLayout)
{
  WUInt32 uiCurrentOffset = 0;
  for (WShaderConstant& constant : ref_materialConstantBufferLayout.m_Constants)
  {
    const WUInt32 uiScalarSize = s_ShaderConstantScalarSize[constant.m_Type];
    const WUInt32 uiConstantSize = s_ShaderConstantSize[constant.m_Type];
    uiCurrentOffset = AlignSize(uiCurrentOffset, uiScalarSize);
    constant.m_uiOffset = uiCurrentOffset;
    uiCurrentOffset += uiConstantSize;
  }
  ref_materialConstantBufferLayout.m_uiTotalSize = uiCurrentOffset;
}

void AlignStructuredBufferStd430Relaxed(WShaderConstantBufferLayout& ref_materialConstantBufferLayout)
{
  WUInt32 uiCurrentOffset = 0;
  for (WShaderConstant& constant : ref_materialConstantBufferLayout.m_Constants)
  {
    const WUInt32 uiScalarSize = s_ShaderConstantScalarSize[constant.m_Type];
    const WUInt32 uiConstantSize = s_ShaderConstantSize[constant.m_Type];
    uiCurrentOffset = AlignSize(uiCurrentOffset, uiScalarSize);
    const WUInt32 uiStartBucket = uiCurrentOffset / 16;
    const WUInt32 uiEndBucket = (uiCurrentOffset + uiConstantSize - 1) / 16;
    // Check if the constant is crossing a 16 byte boundary
    if (uiStartBucket != uiEndBucket)
    {
      uiCurrentOffset = AlignSize(uiCurrentOffset, 16);
    }
    constant.m_uiOffset = uiCurrentOffset;
    uiCurrentOffset += uiConstantSize;
  }
  ref_materialConstantBufferLayout.m_uiTotalSize = uiCurrentOffset;
}

WStatus ParseMaterialConstants(const TokenStream& tokens, WUInt32& ref_uiCurToken, WShaderConstantBufferLayout& ref_materialConstantBufferLayout)
{
  while (!Accept(tokens, ref_uiCurToken, WTokenType::EndOfFile))
  {
    WStatus parseResult = ParseShaderConstant(tokens, ref_uiCurToken, ref_materialConstantBufferLayout);
    if (!parseResult.Succeeded())
    {
      return parseResult;
    }
  }

  return WStatus(W_SUCCESS);
}

WStatus WShaderParser::ParseMaterialConstantsSection(WStringView sMaterialConstantsSection, WSharedPtr<WShaderConstantBufferLayout>& out_pMaterialConstantBufferLayout)
{
  InitializeTables();

  WTokenizer tokenizer;
  tokenizer.Tokenize(WArrayPtr<const WUInt8>((const WUInt8*)sMaterialConstantsSection.GetStartPointer(), sMaterialConstantsSection.GetElementCount()), WLog::GetThreadLocalLogSystem(), false);

  TokenStream tokens;
  tokenizer.GetAllLines(tokens);

  WUInt32 uiCurToken = 0;
  WTempHybridArray<WUInt32, 8> acceptedTokens;

  out_pMaterialConstantBufferLayout = W_DEFAULT_NEW(WShaderConstantBufferLayout);
  const WStatus res = ParseMaterialConstants(tokens, uiCurToken, *out_pMaterialConstantBufferLayout);
  if (res.Failed() || out_pMaterialConstantBufferLayout->m_Constants.IsEmpty())
  {
    out_pMaterialConstantBufferLayout = nullptr;
    return res;
  }

  return WStatus(W_SUCCESS);
}

void WShaderParser::LayoutMaterialConstants(WShaderConstantBufferLayout& ref_materialConstantBufferLayout, WEnum<WGALBufferLayout> layout)
{
  switch (layout)
  {
    case WGALBufferLayout::Vulkan_Std430_relaxed:
      AlignStructuredBufferStd430Relaxed(ref_materialConstantBufferLayout);
      break;
    case WGALBufferLayout::DirectX_StructuredButter:
      AlignStructuredBufferDX(ref_materialConstantBufferLayout);
      break;
    case WGALBufferLayout::DirectX_ConstantButter:
      AlignConstantBufferDX(ref_materialConstantBufferLayout);
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }
}

// static
void WShaderParser::ParsePermutationVarConfig(WStringView s, WVariant& out_defaultValue, EnumDefinition& out_enumDefinition)
{
  SkipWhitespaceAndComments(s);

  WStringBuilder name;

  if (s.StartsWith("bool"))
  {
    bool bDefaultValue = false;

    const char* szDefaultValue = s.FindSubString("=");
    if (szDefaultValue != nullptr)
    {
      name.SetSubString_FromTo(s.GetStartPointer() + 4, szDefaultValue);

      ++szDefaultValue;
      WConversionUtils::StringToBool(szDefaultValue, bDefaultValue).IgnoreResult();
    }
    else
    {
      name.SetSubString_FromTo(s.GetStartPointer() + 4, s.GetEndPointer());
    }

    name.Trim(" \t\r\n");
    out_enumDefinition.m_sName = name;
    out_defaultValue = bDefaultValue;
  }
  else if (s.StartsWith("enum"))
  {
    WTokenizer tokenizer;
    tokenizer.Tokenize(WArrayPtr<const WUInt8>((const WUInt8*)s.GetStartPointer(), s.GetElementCount()), WLog::GetThreadLocalLogSystem());

    TokenStream tokens;
    tokenizer.GetAllLines(tokens);

    WUInt32 uiCurToken = 0;
    if (ParseEnum(tokens, uiCurToken, out_enumDefinition, true).Failed())
    {
      WLog::Error("Invalid enum PermutationVar definition.");
    }
    else
    {
      W_ASSERT_DEV(!out_enumDefinition.m_sName.IsEmpty(), "");

      out_defaultValue = out_enumDefinition.m_uiDefaultValue;
    }
  }
  else
  {
    WLog::Error("Unknown permutation var type");
  }
}

WResult ParseResource(const TokenStream& tokens, WUInt32& ref_uiCurToken, WShaderResourceDefinition& out_resourceDefinition)
{
  // Match type
  WUInt32 uiTypeToken = ref_uiCurToken;
  if (!Accept(tokens, ref_uiCurToken, WTokenType::Identifier, &uiTypeToken))
  {
    return W_FAILURE;
  }
  if (!s_NameToDescriptorTable.TryGetValue(tokens[uiTypeToken]->m_DataView, out_resourceDefinition.m_Binding.m_ResourceType))
    return W_FAILURE;
  s_NameToTextureTable.TryGetValue(tokens[uiTypeToken]->m_DataView, out_resourceDefinition.m_Binding.m_TextureType);

  // Skip optional template
  TokenMatch templatePattern[] = {"<"_wsv, WTokenType::Identifier, ">"_wsv};
  WTempHybridArray<WUInt32, 8> acceptedTokens;
  Accept(tokens, ref_uiCurToken, templatePattern, &acceptedTokens);

  // Match name
  WUInt32 uiNameToken = ref_uiCurToken;
  if (!Accept(tokens, ref_uiCurToken, WTokenType::Identifier, &uiNameToken))
  {
    return W_FAILURE;
  }
  out_resourceDefinition.m_Binding.m_sName.Assign(tokens[uiNameToken]->m_DataView);
  WUInt32 uiEndToken = uiNameToken;

  // Match optional array
  TokenMatch arrayPattern[] = {"["_wsv, WTokenType::Integer, "]"_wsv};
  TokenMatch bindlessPattern[] = {"["_wsv, "]"_wsv};
  if (Accept(tokens, ref_uiCurToken, arrayPattern, &acceptedTokens))
  {
    WConversionUtils::StringToUInt(tokens[acceptedTokens[1]]->m_DataView, out_resourceDefinition.m_Binding.m_uiArraySize).AssertSuccess("Tokenizer error");
    uiEndToken = acceptedTokens.PeekBack();
  }
  else if (Accept(tokens, ref_uiCurToken, bindlessPattern, &acceptedTokens))
  {
    out_resourceDefinition.m_Binding.m_uiArraySize = 0;
    uiEndToken = acceptedTokens.PeekBack();
  }
  out_resourceDefinition.m_sDeclaration = WStringView(tokens[uiTypeToken]->m_DataView.GetStartPointer(), tokens[uiEndToken]->m_DataView.GetEndPointer());

  // Match optional register
  TokenMatch slotPattern[] = {":"_wsv, "register"_wsv, "("_wsv, WTokenType::Identifier, ")"_wsv};
  TokenMatch slotAndSetPattern[] = {":"_wsv, "register"_wsv, "("_wsv, WTokenType::Identifier, ","_wsv, WTokenType::Identifier, ")"_wsv};
  if (Accept(tokens, ref_uiCurToken, slotPattern, &acceptedTokens))
  {
    WStringView sSlot = tokens[acceptedTokens[3]]->m_DataView;
    sSlot.Trim("tsubx");
    if (sSlot.IsEqual_NoCase("AUTO")) // See shader macros in StandardMacros.h
    {
      out_resourceDefinition.m_Binding.m_iSlot = -1;
    }
    else
    {
      WInt32 iSlot;
      WConversionUtils::StringToInt(sSlot, iSlot).AssertSuccess("Failed to parse slot index of shader resource");
      out_resourceDefinition.m_Binding.m_iSlot = static_cast<WInt16>(iSlot);
    }
    uiEndToken = acceptedTokens.PeekBack();
  }
  else if (Accept(tokens, ref_uiCurToken, slotAndSetPattern, &acceptedTokens))
  {
    WStringView sSlot = tokens[acceptedTokens[3]]->m_DataView;
    sSlot.Trim("tsubx");
    if (sSlot.IsEqual_NoCase("AUTO")) // See shader macros in StandardMacros.h
    {
      out_resourceDefinition.m_Binding.m_iSlot = -1;
    }
    else
    {
      WInt32 iSlot;
      WConversionUtils::StringToInt(sSlot, iSlot).AssertSuccess("Failed to parse slot index of shader resource");
      out_resourceDefinition.m_Binding.m_iSlot = static_cast<WInt16>(iSlot);
    }
    WStringView sSet = tokens[acceptedTokens[5]]->m_DataView;
    sSet.TrimWordStart("space"_wsv);
    WInt32 iSet;
    WConversionUtils::StringToInt(sSet, iSet).AssertSuccess("Failed to parse set index of shader resource");
    out_resourceDefinition.m_Binding.m_iBindGroup = static_cast<WInt16>(iSet);
    uiEndToken = acceptedTokens.PeekBack();
  }

  out_resourceDefinition.m_sDeclarationAndRegister = WStringView(tokens[uiTypeToken]->m_DataView.GetStartPointer(), tokens[uiEndToken]->m_DataView.GetEndPointer());
  // Match ; (resource declaration done) or { (constant buffer member declaration starts)
  if (!Accept(tokens, ref_uiCurToken, ";"_wsv) && !Accept(tokens, ref_uiCurToken, "{"_wsv))
    return W_FAILURE;

  return W_SUCCESS;
}

void WShaderParser::ParseShaderResources(WStringView sShaderStageSource, WDynamicArray<WShaderResourceDefinition>& out_resources)
{
  if (sShaderStageSource.IsEmpty())
  {
    out_resources.Clear();
    return;
  }

  InitializeTables();

  WTokenizer tokenizer;
  tokenizer.SetTreatHashSignAsLineComment(true);
  tokenizer.Tokenize(WArrayPtr<const WUInt8>((const WUInt8*)sShaderStageSource.GetStartPointer(), sShaderStageSource.GetElementCount()), WLog::GetThreadLocalLogSystem(), false);

  TokenStream tokens;
  tokenizer.GetAllLines(tokens);

  WUInt32 uiCurToken = 0;

  while (!Accept(tokens, uiCurToken, WTokenType::EndOfFile))
  {
    WShaderResourceDefinition resourceDef;
    if (ParseResource(tokens, uiCurToken, resourceDef).Succeeded())
    {
      out_resources.PushBack(std::move(resourceDef));
      continue;
    }
    ++uiCurToken;
  }
}

WResult WShaderParser::MergeShaderResourceBindings(const WShaderProgramData& spd, WHashTable<WHashedString, WShaderResourceBinding>& out_bindings, WLogInterface* pLog)
{
  WUInt32 uiSize = 0;
  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    uiSize += spd.m_Resources[stage].GetCount();
  }

  out_bindings.Clear();
  out_bindings.Reserve(uiSize);

  WMap<WHashedString, const WShaderResourceDefinition*> resourceFirstOccurence;

  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (const WShaderResourceDefinition& res : spd.m_Resources[stage])
    {
      WHashedString sName = res.m_Binding.m_sName;
      auto it = out_bindings.Find(sName);
      if (it.IsValid())
      {
        WShaderResourceBinding& current = it.Value();
        if (current.m_ResourceType != res.m_Binding.m_ResourceType || current.m_TextureType != res.m_Binding.m_TextureType || current.m_uiArraySize != res.m_Binding.m_uiArraySize)
        {
          WLog::Error(pLog, "A shared shader resource '{}' has a mismatching signatures between stages: '{}' vs '{}'", sName, resourceFirstOccurence.Find(sName).Value()->m_sDeclarationAndRegister, res.m_sDeclarationAndRegister);
          return W_FAILURE;
        }

        current.m_Stages |= WGALShaderStageFlags::MakeFromShaderStage((WGALShaderStage::Enum)stage);
      }
      else
      {
        out_bindings.Insert(sName, res.m_Binding);
        resourceFirstOccurence.Insert(sName, &res);
        out_bindings.Find(sName).Value().m_Stages |= WGALShaderStageFlags::MakeFromShaderStage((WGALShaderStage::Enum)stage);
      }
    }
  }
  return W_SUCCESS;
}

WResult WShaderParser::SanityCheckShaderResourceBindings(const WHashTable<WHashedString, WShaderResourceBinding>& bindings, WLogInterface* pLog)
{
  for (auto it : bindings)
  {
    if (it.Value().m_iBindGroup < 0)
    {
      WLog::Error(pLog, "Shader resource '{}' does not have a set defined.", it.Key());
      return W_FAILURE;
    }
    if (it.Value().m_iSlot < 0)
    {
      WLog::Error(pLog, "Shader resource '{}' does not have a slot defined.", it.Key());
      return W_FAILURE;
    }
  }
  return W_SUCCESS;
}

void WShaderParser::ApplyShaderResourceBindings(WStringView sPlatform, WStringView sShaderStageSource, const WDynamicArray<WShaderResourceDefinition>& resources, const WHashTable<WHashedString, WShaderResourceBinding>& bindings, const CreateResourceDeclaration& createDeclaration, WStringBuilder& out_sShaderStageSource)
{
  WDeque<WString> partStorage;
  WTempHybridArray<WStringView, 16> parts;

  WStringBuilder sDeclaration;
  const char* szStart = sShaderStageSource.GetStartPointer();
  for (WUInt32 i = 0; i < resources.GetCount(); ++i)
  {
    parts.PushBack(WStringView(szStart, resources[i].m_sDeclarationAndRegister.GetStartPointer()));

    WShaderResourceBinding* pBinding = nullptr;
    bindings.TryGetValue(resources[i].m_Binding.m_sName, pBinding);

    W_ASSERT_DEV(pBinding != nullptr, "Every resource should be present in the map.");
    createDeclaration(sPlatform, resources[i].m_sDeclaration, *pBinding, sDeclaration);
    WString& sStorage = partStorage.ExpandAndGetRef();
    sStorage = sDeclaration;
    parts.PushBack(sStorage);
    szStart = resources[i].m_sDeclarationAndRegister.GetEndPointer();
  }
  parts.PushBack(WStringView(szStart, sShaderStageSource.GetEndPointer()));

  WUInt32 uiSize = 0;
  for (const WStringView& sPart : parts)
    uiSize += sPart.GetElementCount();

  out_sShaderStageSource.Clear();
  out_sShaderStageSource.Reserve(uiSize);

  for (const WStringView& sPart : parts)
  {
    out_sShaderStageSource.Append(sPart);
  }
}
