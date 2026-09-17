#include <RendererTest/RendererTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/RendererReflection.h>
#include <RendererTest/Basics/ShaderCompilerTest.h>

void WRendererTestShaderCompiler::SetupSubTests()
{
  AddSubTest("Shader Resources", SubTests::ST_ShaderResources);
}

WResult WRendererTestShaderCompiler::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(WGraphicsTest::InitializeSubTest(iIdentifier));

  m_hUVColorShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/ShaderCompilerTest.WShader");

  return W_SUCCESS;
}

WResult WRendererTestShaderCompiler::DeInitializeSubTest(WInt32 iIdentifier)
{
  // m_hShader.Invalidate();
  m_hUVColorShader.Invalidate();

  if (WGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WTestAppRun WRendererTestShaderCompiler::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  WHashTable<WHashedString, WHashedString> m_PermutationVariables;
  WShaderPermutationResourceHandle m_hActiveShaderPermutation = WShaderManager::PreloadSinglePermutation(m_hUVColorShader, m_PermutationVariables, false);

  if (!W_TEST_BOOL(m_hActiveShaderPermutation.IsValid()))
    return WTestAppRun::Quit;

  {
    WResourceLock<WShaderPermutationResource> pResource(m_hActiveShaderPermutation, WResourceAcquireMode::BlockTillLoaded);

    WArrayPtr<const WPermutationVar> permutationVars = static_cast<const WShaderPermutationResource*>(pResource.GetPointer())->GetPermutationVars();

    const WGALShaderByteCode* pVertex = pResource->GetShaderByteCode(WGALShaderStage::VertexShader);
    W_TEST_BOOL(pVertex);
    const auto& vertexDecl = pVertex->m_ShaderVertexInput;
    if (W_TEST_INT(vertexDecl.GetCount(), 12))
    {
      auto CheckVertexDecl = [&](WGALVertexAttributeSemantic::Enum semantic, WGALResourceFormat::Enum format, WUInt8 uiLocation)
      {
        W_TEST_INT(vertexDecl[uiLocation].m_eSemantic, semantic);
        W_TEST_INT(vertexDecl[uiLocation].m_eFormat, format);
        W_TEST_INT(vertexDecl[uiLocation].m_uiLocation, uiLocation);
      };
      CheckVertexDecl(WGALVertexAttributeSemantic::Position, WGALResourceFormat::RGBAFloat, 0);
      CheckVertexDecl(WGALVertexAttributeSemantic::Normal, WGALResourceFormat::RGBFloat, 1);
      CheckVertexDecl(WGALVertexAttributeSemantic::Tangent, WGALResourceFormat::RGFloat, 2);
      CheckVertexDecl(WGALVertexAttributeSemantic::Color0, WGALResourceFormat::RFloat, 3);
      CheckVertexDecl(WGALVertexAttributeSemantic::Color7, WGALResourceFormat::RGBAUInt, 4);
      CheckVertexDecl(WGALVertexAttributeSemantic::TexCoord0, WGALResourceFormat::RGBUInt, 5);
      CheckVertexDecl(WGALVertexAttributeSemantic::TexCoord9, WGALResourceFormat::RGUInt, 6);
      CheckVertexDecl(WGALVertexAttributeSemantic::BiTangent, WGALResourceFormat::RUInt, 7);
      CheckVertexDecl(WGALVertexAttributeSemantic::BoneWeights0, WGALResourceFormat::RGBAInt, 8);
      CheckVertexDecl(WGALVertexAttributeSemantic::BoneWeights1, WGALResourceFormat::RGBInt, 9);
      CheckVertexDecl(WGALVertexAttributeSemantic::BoneIndices0, WGALResourceFormat::RGInt, 10);
      CheckVertexDecl(WGALVertexAttributeSemantic::BoneIndices1, WGALResourceFormat::RInt, 11);
    }

    const WGALShaderByteCode* pPixel = pResource->GetShaderByteCode(WGALShaderStage::PixelShader);
    W_TEST_BOOL(pPixel);
    const WHybridArray<WShaderResourceBinding, 8>& bindings = pPixel->m_ShaderResourceBindings;
    {
      auto CheckBinding = [&](WStringView sName, WGALShaderResourceType::Enum descriptorType, WGALShaderTextureType::Enum textureType = WGALShaderTextureType::Unknown, WBitflags<WGALShaderStageFlags> stages = WGALShaderStageFlags::PixelShader, WUInt32 uiArraySize = 1)
      {
        for (WUInt32 i = 0; i < bindings.GetCount(); ++i)
        {
          if (bindings[i].m_sName.GetView() == sName)
          {
            W_TEST_INT(bindings[i].m_ResourceType, descriptorType);
            W_TEST_INT(bindings[i].m_TextureType, textureType);
            W_TEST_INT(bindings[i].m_Stages.GetValue(), stages.GetValue());
            W_TEST_INT(bindings[i].m_uiArraySize, uiArraySize);
            return;
          }
        }
        W_TEST_BOOL_MSG(false, "Shader resource not found in binding list");
      };
      const WGALDeviceCapabilities& caps = WGALDevice::GetDefaultDevice()->GetCapabilities();
      CheckBinding("PointClampSampler"_wsv, WGALShaderResourceType::Sampler);
      CheckBinding("PerFrame"_wsv, WGALShaderResourceType::ConstantBuffer);
      // CheckBinding("RES_Texture1D"_wsv, WGALShaderResourceType::Texture, WGALShaderTextureType::Texture1D);
      // CheckBinding("RES_Texture1DArray"_wsv, WGALShaderResourceType::Texture, WGALShaderTextureType::Texture1DArray);
      CheckBinding("RES_Texture2D"_wsv, WGALShaderResourceType::Texture, WGALShaderTextureType::Texture2D);
      CheckBinding("RES_Texture2DArray"_wsv, WGALShaderResourceType::Texture, WGALShaderTextureType::Texture2DArray);
      CheckBinding("RES_Texture2DMS"_wsv, WGALShaderResourceType::Texture, WGALShaderTextureType::Texture2DMS);
      if (caps.m_bSupportsMultiSampledArrays)
      {
        CheckBinding("RES_Texture2DMSArray"_wsv, WGALShaderResourceType::Texture, WGALShaderTextureType::Texture2DMSArray);
      }
      CheckBinding("RES_Texture3D"_wsv, WGALShaderResourceType::Texture, WGALShaderTextureType::Texture3D);
      CheckBinding("RES_TextureCube"_wsv, WGALShaderResourceType::Texture, WGALShaderTextureType::TextureCube);
      CheckBinding("RES_TextureCubeArray"_wsv, WGALShaderResourceType::Texture, WGALShaderTextureType::TextureCubeArray);

      if (caps.m_bSupportsTexelBuffer)
      {
        CheckBinding("RES_Buffer"_wsv, WGALShaderResourceType::TexelBuffer);
      }
      CheckBinding("RES_StructuredBuffer"_wsv, WGALShaderResourceType::StructuredBuffer);
      CheckBinding("RES_ByteAddressBuffer"_wsv, WGALShaderResourceType::ByteAddressBuffer);

      // CheckBinding("RES_RWTexture1D"_wsv, WGALShaderResourceType::TextureRW, WGALShaderTextureType::Texture1D);
      // CheckBinding("RES_RWTexture1DArray"_wsv, WGALShaderResourceType::TextureRW, WGALShaderTextureType::Texture1DArray);
      CheckBinding("RES_RWTexture2D"_wsv, WGALShaderResourceType::TextureRW, WGALShaderTextureType::Texture2D);
      CheckBinding("RES_RWTexture2DArray"_wsv, WGALShaderResourceType::TextureRW, WGALShaderTextureType::Texture2DArray);
      CheckBinding("RES_RWTexture3D"_wsv, WGALShaderResourceType::TextureRW, WGALShaderTextureType::Texture3D);
      if (caps.m_bSupportsTexelBuffer)
      {
        CheckBinding("RES_RWBuffer"_wsv, WGALShaderResourceType::TexelBufferRW);
      }
      CheckBinding("RES_RWStructuredBuffer"_wsv, WGALShaderResourceType::StructuredBufferRW);
      CheckBinding("RES_RWByteAddressBuffer"_wsv, WGALShaderResourceType::ByteAddressBufferRW);

      CheckBinding("RES_AppendStructuredBuffer"_wsv, WGALShaderResourceType::StructuredBufferRW);
      CheckBinding("RES_ConsumeStructuredBuffer"_wsv, WGALShaderResourceType::StructuredBufferRW);
    }
  }

  return WTestAppRun::Quit;
}


static WRendererTestShaderCompiler g_ShaderCompilerTest;
