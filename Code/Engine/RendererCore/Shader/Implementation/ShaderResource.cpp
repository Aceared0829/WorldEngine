#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WShaderResource, 1, WRTTIDefaultAllocator<WShaderResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WShaderResource);
// clang-format on

WShaderResource::WShaderResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
  m_bShaderResourceIsValid = false;
}

WResourceLoadDesc WShaderResource::UnloadData(Unload WhatToUnload)
{
  m_bShaderResourceIsValid = false;
  m_PermutationVarsUsed.Clear();

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WShaderResource::UpdateContent(WStreamReader* stream)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  m_bShaderResourceIsValid = false;

  if (stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  WStringBuilder sAbsFilePath;
  (*stream) >> sAbsFilePath;

  WString sContent;
  sContent.ReadAll(*stream);

  WShaderHelper::WTextSectionizer Sections;
  WShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  WUInt32 uiFirstLine = 0;
  WTempHybridArray<WPermutationVar, 16> fixedPermVars; // ignored here
  WStringView sPermutations = Sections.GetSectionContent(WShaderHelper::WShaderSections::PERMUTATIONS, uiFirstLine);
  WShaderParser::ParsePermutationSection(sPermutations, m_PermutationVarsUsed, fixedPermVars);

  uiFirstLine = 0;
  WStringView sShader = Sections.GetSectionContent(WShaderHelper::WShaderSections::MATERIALCONSTANTS, uiFirstLine);
  if (!sShader.IsEmpty())
  {
    if (WShaderParser::ParseMaterialConstantsSection(sShader, m_pLayout).Succeeded() && m_pLayout != nullptr)
    {
      WShaderParser::LayoutMaterialConstants(*m_pLayout, WGALDevice::GetDefaultDevice()->GetCapabilities().m_materialBufferLayout);
    }
  }

  res.m_State = WResourceState::Loaded;
  m_bShaderResourceIsValid = true;

  return res;
}

void WShaderResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WShaderResource) + (WUInt32)m_PermutationVarsUsed.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WShaderResource, WShaderResourceDescriptor)
{
  WResourceLoadDesc ret;
  ret.m_State = WResourceState::Loaded;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;

  m_bShaderResourceIsValid = false;

  return ret;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderResource);
