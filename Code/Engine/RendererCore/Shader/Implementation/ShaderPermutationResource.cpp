#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Shader/Shader.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WShaderPermutationResource, 1, WRTTIDefaultAllocator<WShaderPermutationResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WShaderPermutationResource);
// clang-format on

static WShaderPermutationResourceLoader g_PermutationResourceLoader;

WShaderPermutationResource::WShaderPermutationResource()
  : WResource(DoUpdate::OnGraphicsResourceThreads, 1)
{
  m_bShaderPermutationValid = false;

  for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    m_ByteCodes[stage] = nullptr;
  }
}

WResourceLoadDesc WShaderPermutationResource::UnloadData(Unload WhatToUnload)
{
  m_bShaderPermutationValid = false;

  auto pDevice = WGALDevice::GetDefaultDevice();

  pDevice->DestroyShader(m_hShader);
  pDevice->DestroyBlendState(m_hBlendState);
  pDevice->DestroyDepthStencilState(m_hDepthStencilState);
  pDevice->DestroyRasterizerState(m_hRasterizerState);

  WResourceLoadDesc res;
  res.m_State = WResourceState::Unloaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  return res;
}

WResourceLoadDesc WShaderPermutationResource::UpdateContent(WStreamReader* Stream)
{
  WUInt32 uiGPUMem = 0;
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  m_bShaderPermutationValid = false;

  WResourceLoadDesc res;
  res.m_State = WResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    WLog::Error("Shader Permutation '{0}': Data is not available", GetResourceID());
    return res;
  }

  WShaderPermutationBinary PermutationBinary;

  bool bOldVersion = false;
  if (PermutationBinary.Read(*Stream, bOldVersion).Failed())
  {
    WLog::Error("Shader Permutation '{0}': Could not read shader permutation binary", GetResourceID());
    return res;
  }

  auto pDevice = WGALDevice::GetDefaultDevice();

  // get the shader render state object
  {
    m_hBlendState = pDevice->CreateBlendState(PermutationBinary.m_StateDescriptor.m_BlendDesc);
    m_hDepthStencilState = pDevice->CreateDepthStencilState(PermutationBinary.m_StateDescriptor.m_DepthStencilDesc);
    m_hRasterizerState = pDevice->CreateRasterizerState(PermutationBinary.m_StateDescriptor.m_RasterizerDesc);
    m_uiShaderStencilRef = PermutationBinary.m_StateDescriptor.m_uiShaderStencilRef;
    m_bUseUserStencilRef = PermutationBinary.m_StateDescriptor.m_bUseUserStencilRefValue;
  }

  WGALShaderCreationDescription ShaderDesc;

  // iterate over all shader stages, add them to the descriptor
  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    const WUInt32 uiStageHash = PermutationBinary.m_uiShaderStageHashes[stage];

    if (uiStageHash == 0) // not used
      continue;

    WShaderStageBinary* pStageBin = WShaderStageBinary::LoadStageBinary((WGALShaderStage::Enum)stage, uiStageHash, WShaderManager::GetActivePlatform());

    if (pStageBin == nullptr)
    {
      WLog::Error("Shader Permutation '{0}': Stage '{1}' could not be loaded", GetResourceID(), WGALShaderStage::Names[stage]);
      return res;
    }

    // store not only the hash but also the pointer to the stage binary
    // since it contains other useful information (resource bindings), that we need for shader binding
    m_ByteCodes[stage] = pStageBin->GetByteCode();

    W_ASSERT_DEV(pStageBin->m_pGALByteCode->m_Stage == stage, "Invalid shader stage! Expected stage '{0}', but loaded data is for stage '{1}'", WGALShaderStage::Names[stage], WGALShaderStage::Names[pStageBin->m_pGALByteCode->m_Stage]);

    ShaderDesc.m_ByteCodes[stage] = pStageBin->m_pGALByteCode;

    uiGPUMem += pStageBin->m_pGALByteCode->m_ByteCode.GetCount();
  }

  m_hShader = pDevice->CreateShader(ShaderDesc);

  if (m_hShader.IsInvalidated())
  {
    WLog::Error("Shader Permutation '{0}': Shader program creation failed", GetResourceID());
    return res;
  }

  pDevice->GetShader(m_hShader)->SetDebugName(GetResourceID());

  m_PermutationVars = PermutationBinary.m_PermutationVars;

  m_bShaderPermutationValid = true;

  ModifyMemoryUsage().m_uiMemoryGPU = uiGPUMem;

  return res;
}

void WShaderPermutationResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WShaderPermutationResource);
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WShaderPermutationResource, WShaderPermutationResourceDescriptor)
{
  WResourceLoadDesc ret;
  ret.m_State = WResourceState::Loaded;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;

  return ret;
}

WResourceTypeLoader* WShaderPermutationResource::GetDefaultResourceTypeLoader() const
{
  return &g_PermutationResourceLoader;
}

struct ShaderPermutationResourceLoadData
{
  ShaderPermutationResourceLoadData()
    : m_Reader(&m_Storage)
  {
  }

  WContiguousMemoryStreamStorage m_Storage;
  WMemoryStreamReader m_Reader;
};

WResult WShaderPermutationResourceLoader::RunCompiler(const WResource* pResource, WShaderPermutationBinary& BinaryInfo, bool bForce)
{
  if (WShaderManager::IsRuntimeCompilationEnabled())
  {
    if (!bForce)
    {
      // check whether any dependent file has changed, and trigger a recompilation if necessary
      if (BinaryInfo.m_DependencyFile.HasAnyFileChanged())
      {
        bForce = true;
      }
    }

    if (!bForce) // no recompilation necessary
      return W_SUCCESS;

    WStringBuilder sPermutationFile = pResource->GetResourceID();

    sPermutationFile.ChangeFileExtension("");
    sPermutationFile.Shrink(WShaderManager::GetCacheDirectory().GetCharacterCount() + WShaderManager::GetActivePlatform().GetCharacterCount() + 2, 1);

    sPermutationFile.Shrink(0, 9); // remove underscore and the hash at the end
    sPermutationFile.Append(".WShader");

    WArrayPtr<const WPermutationVar> permutationVars = static_cast<const WShaderPermutationResource*>(pResource)->GetPermutationVars();

    WShaderCompiler sc;
    return sc.CompileShaderPermutationForPlatforms(sPermutationFile, permutationVars, WLog::GetThreadLocalLogSystem(), WShaderManager::GetActivePlatform());
  }
  else
  {
    if (bForce)
    {
      WLog::Error("Shader was forced to be compiled, but runtime shader compilation is not available");
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

bool WShaderPermutationResourceLoader::IsResourceOutdated(const WResource* pResource) const
{
  // don't try to reload a file that cannot be found
  WStringBuilder sAbs;
  if (WFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
    return false;

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    WFileStats stat;
    if (WFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    if (!stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), WTimestamp::CompareMode::FileTimeEqual))
      return true;
  }

#endif

  WDependencyFile dep;
  if (dep.ReadDependencyFile(pResource->GetResourceID()).Failed())
    return true;

  return dep.HasAnyFileChanged();
}

WResourceLoadData WShaderPermutationResourceLoader::OpenDataStream(const WResource* pResource)
{
  WResourceLoadData res;

  WShaderPermutationBinary permutationBinary;

  bool bNeedsCompilation = true;
  bool bOldVersion = false;

  {
    WFileReader File;
    if (File.Open(pResource->GetResourceID()).Failed())
    {
      WLog::Debug("Shader Permutation '{0}' does not exist, triggering recompile.", pResource->GetResourceID());

      bNeedsCompilation = false;
      if (RunCompiler(pResource, permutationBinary, true).Failed())
        return res;

      // try again
      if (File.Open(pResource->GetResourceID()).Failed())
      {
        WLog::Debug("Shader Permutation '{0}' still does not exist after recompile.", pResource->GetResourceID());
        return res;
      }
    }

    res.m_sResourceDescription = File.GetFilePathRelative().GetData();

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
    WFileStats stat;
    if (WFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
    {
      res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
    }
#endif

    if (permutationBinary.Read(File, bOldVersion).Failed())
    {
      WLog::Error("Shader Permutation '{0}': Could not read shader permutation binary", pResource->GetResourceID());

      bNeedsCompilation = true;
    }

    if (bOldVersion)
    {
      WLog::Dev("Shader Permutation Binary version is outdated, recompiling shader.");
      bNeedsCompilation = true;
    }
  }

  if (bNeedsCompilation)
  {
    if (RunCompiler(pResource, permutationBinary, false).Failed())
      return res;

    WFileReader File;

    if (File.Open(pResource->GetResourceID()).Failed())
    {
      WLog::Error("Shader Permutation '{0}': Failed to open the file", pResource->GetResourceID());
      return res;
    }

    if (permutationBinary.Read(File, bOldVersion).Failed())
    {
      WLog::Error("Shader Permutation '{0}': Binary data could not be read", pResource->GetResourceID());
      return res;
    }

    File.Close();
  }



  ShaderPermutationResourceLoadData* pData = W_DEFAULT_NEW(ShaderPermutationResourceLoadData);

  WMemoryStreamWriter w(&pData->m_Storage);

  // preload the files that are referenced in the .WPermutation file
  {
    // write the permutation file info back to the output stream, so that the resource can read it as well
    permutationBinary.Write(w).IgnoreResult();

    for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
    {
      const WUInt32 uiStageHash = permutationBinary.m_uiShaderStageHashes[stage];

      if (uiStageHash == 0) // not used
        continue;

      // this is where the preloading happens
      WShaderStageBinary::LoadStageBinary((WGALShaderStage::Enum)stage, uiStageHash, WShaderManager::GetActivePlatform());
    }
  }

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void WShaderPermutationResourceLoader::CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData)
{
  ShaderPermutationResourceLoadData* pData = static_cast<ShaderPermutationResourceLoadData*>(loaderData.m_pCustomLoaderData);

  W_DEFAULT_DELETE(pData);
}



W_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderPermutationResource);
