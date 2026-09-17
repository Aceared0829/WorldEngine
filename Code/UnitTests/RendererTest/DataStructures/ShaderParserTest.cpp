#include <RendererTest/TestClass/SimpleRendererTest.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP) || W_ENABLED(W_PLATFORM_LINUX)

#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileWriter.h>
#  include <RendererCore/Shader/ShaderPermutationResource.h>
#  include <RendererCore/ShaderCompiler/ShaderManager.h>
#  include <RendererCore/ShaderCompiler/ShaderParser.h>
#  include <RendererFoundation/Shader/Shader.h>

void CompareLayouts(const WShaderConstantBufferLayout& layoutA, const WShaderConstantBufferLayout& layoutB)
{
  if (W_TEST_INT(layoutA.m_Constants.GetCount(), layoutB.m_Constants.GetCount()))
  {
    for (WUInt32 i = 0; i < layoutA.m_Constants.GetCount(); ++i)
    {
      const auto& constantA = layoutA.m_Constants[i];
      const auto& constantB = layoutB.m_Constants[i];

      W_TEST_STRING(constantA.m_sName.GetData(), constantB.m_sName.GetData());
      if (constantA.m_Type == WShaderConstant::Type::Bool || constantB.m_Type == WShaderConstant::Type::Bool)
      {
        W_TEST_BOOL(constantA.m_Type == WShaderConstant::Type::Bool || constantA.m_Type == WShaderConstant::Type::UInt1);
        W_TEST_BOOL(constantB.m_Type == WShaderConstant::Type::Bool || constantB.m_Type == WShaderConstant::Type::UInt1);
      }
      else
      {
        W_TEST_INT(constantA.m_Type.GetValue(), constantB.m_Type.GetValue());
      }
      W_TEST_INT(constantA.m_uiArrayElements, constantB.m_uiArrayElements);
      W_TEST_INT(constantA.m_uiOffset, constantB.m_uiOffset);
    }
  }
  W_TEST_INT(layoutA.m_uiTotalSize, layoutB.m_uiTotalSize);
}

void TestMaterialConstants(WStringView sMaterialConstants, WStringView sMaterialUsage, WStringView sShaderName, WUInt32 uiParameterCount)
{
  // Read template
  WStringBuilder sEzFileContent;
  {
    WFileReader fileEz;
    if (!W_TEST_RESULT(fileEz.Open("RendererTest/Shaders/ShaderParserTest.WShader.template")))
    {
      return;
    }
    sEzFileContent.ReadAll(fileEz);
  }

  // Insert shaderSection
  sEzFileContent.ReplaceFirst("{{MATERIAL_SECTION}}", sMaterialConstants);
  sEzFileContent.ReplaceFirst("{{MATERIAL_USAGE}}", sMaterialUsage);

  // Write temp shader
  WStringBuilder sTempFile(":imgout/", sShaderName);
  {
    WFileWriter TempFile;
    W_TEST_BOOL(TempFile.Open(sTempFile) == W_SUCCESS);
    TempFile.WriteBytes(sEzFileContent.GetData(), sEzFileContent.GetElementCount()).IgnoreResult();
    TempFile.Close();
  }

  // Load / compile shader permutation
  auto m_hUVColorShader = WResourceManager::LoadResource<WShaderResource>(sShaderName);
  WResourceLock<WShaderResource> pShaderResource(m_hUVColorShader, WResourceAcquireMode::BlockTillLoaded);

  WHashTable<WHashedString, WHashedString> m_PermutationVariables;
  WShaderPermutationResourceHandle m_hActiveShaderPermutation = WShaderManager::PreloadSinglePermutation(m_hUVColorShader, m_PermutationVariables, false);

  if (!W_TEST_BOOL(m_hActiveShaderPermutation.IsValid()))
    return;

  WShaderPermutationResource* pShaderPermutation = WResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, WResourceAcquireMode::BlockTillLoaded);
  const WGALShader* pGalShader = WGALDevice::GetDefaultDevice()->GetShader(pShaderPermutation->GetGALShader());

  if (W_TEST_BOOL(pGalShader))
  {
    WTempHashedString sConstantBufferName("materialData");
    W_TEST_BOOL(pGalShader->GetBindGroupCount() > W_GAL_BIND_GROUP_MATERIAL);
    WArrayPtr<const WShaderResourceBinding> bindings = pGalShader->GetBindings(W_GAL_BIND_GROUP_MATERIAL);

    const WShaderResourceBinding* pBinding = nullptr;
    for (const WShaderResourceBinding& binding : bindings)
    {
      if (binding.m_sName == sConstantBufferName)
      {
        pBinding = &binding;
        break;
      }
    }
    W_TEST_BOOL(pBinding != nullptr);

    // Compared parsed vs compiled layout
    if (W_TEST_BOOL(pBinding && pBinding->m_pLayout && pShaderResource->GetMaterialLayout()))
    {
      W_TEST_INT(uiParameterCount, pBinding->m_pLayout->m_Constants.GetCount());
      W_TEST_INT(uiParameterCount, pShaderResource->GetMaterialLayout()->m_Constants.GetCount());
      CompareLayouts(*pShaderResource->GetMaterialLayout(), *pBinding->m_pLayout);
    }
  }
  WResourceManager::EndAcquireResource(pShaderPermutation);
}

W_CREATE_SIMPLE_RENDERER_TEST(DataStructures, ShaderParser)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Default")
  {
    WStringView shaderSection =
      "  COLOR4F(BaseColor);\n"
      "  COLOR4F(EmissiveColor);\n"
      "  FLOAT1(MetallicValue);\n"
      "  FLOAT1(RoughnessValue);\n"
      "  FLOAT1(MaskThreshold);\n"
      "  BOOL1(UseBaseTexture);\n"
      "  BOOL1(UseNormalTexture);\n"
      "  BOOL1(UseRoughnessTexture);\n"
      "  BOOL1(UseMetallicTexture);\n"
      "  BOOL1(UseEmissiveTexture);\n"
      "  BOOL1(UseOcclusionTexture);\n"
      "  BOOL1(UseOrmTexture);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(BaseColor).r", "Temp_Default.WShader", 12);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Zoo")
  {
    WStringView shaderSection =
      "  FLOAT1(Value1);\n"
      "  FLOAT2(Value2);\n"
      "  FLOAT3(Value3);\n"
      "  FLOAT4(Value4);\n"
      "  INT1(Value5);\n"
      "  INT2(Value6);\n"
      "  INT3(Value7);\n"
      "  INT4(Value8);\n"
      "  UINT1(Value9);\n"
      "  UINT2(Value10);\n"
      "  UINT3(Value11);\n"
      "  UINT4(Value12);\n"
      "  MAT3(Value13);\n"
      "  MAT4(Value14);\n"
      "  TRANSFORM(Value15);\n"
      "  COLOR4F(Value16);\n"
      "  BOOL1(Value17);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_Zoo.WShader", 17);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PoorPacking1")
  {
    WStringView shaderSection =
      "  FLOAT1(Value1);\n"
      "  BOOL1(Value2);\n"
      "  COLOR4F(Value3);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_PoorPacking1.WShader", 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PoorPacking2")
  {
    WStringView shaderSection =
      "  FLOAT1(Value1);\n"
      "  COLOR4F(Value2);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_PoorPacking2.WShader", 2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PoorPacking3")
  {
    WStringView shaderSection =
      "  FLOAT1(Value1);\n"
      "  MAT3(Value2);\n"
      "  FLOAT2(Value3);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_PoorPacking3.WShader", 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PoorPacking4")
  {
    WStringView shaderSection =
      "  FLOAT1(Value1);\n"
      "  MAT4(Value2);\n"
      "  INT1(Value3);\n"
      "  UINT2(Value4);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_PoorPacking4.WShader", 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PoorPacking5")
  {
    WStringView shaderSection =
      "  UINT2(Value1);\n"
      "  TRANSFORM(Value2);\n"
      "  INT2(Value3);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_PoorPacking5.WShader", 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Float2a")
  {
    WStringView shaderSection =
      "  FLOAT2(Value1);\n"
      "  UINT2(Value2);\n"
      "  FLOAT1(Value3);\n"
      "  INT1(Value4);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_Float2a.WShader", 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Float2b")
  {
    WStringView shaderSection =
      "  FLOAT2(Value1);\n"
      "  FLOAT1(Value2);\n"
      "  FLOAT2(Value3);\n"
      "  FLOAT1(Value4);\n"
      "  FLOAT1(Value5);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_Float2b.WShader", 5);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Float2c")
  {
    WStringView shaderSection =
      "  FLOAT1(Value1);\n"
      "  FLOAT1(Value2);\n"
      "  FLOAT2(Value3);\n"
      "  FLOAT2(Value4);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_Float2c.WShader", 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Float3")
  {
    WStringView shaderSection =
      "  FLOAT3(Value1);\n"
      "  FLOAT3(Value2);\n"
      "  FLOAT3(Value3);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_Float3.WShader", 3);
  }
}
#endif
