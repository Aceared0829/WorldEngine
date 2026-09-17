

#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/ImmutableSamplers.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/ShaderUtils.h>
#include <RendererFoundation/Shader/Types.h>

bool WShaderMat3::TransposeShaderMatrices = false;

WGALShader::WGALShader(const WGALShaderCreationDescription& Description)
  : WGALObject(Description)
{
}

WArrayPtr<const WShaderVertexInputAttribute> WGALShader::GetVertexInputAttributes() const
{
  if (m_Description.HasByteCodeForStage(WGALShaderStage::VertexShader))
  {
    return m_Description.m_ByteCodes[WGALShaderStage::VertexShader]->m_ShaderVertexInput;
  }
  return {};
}

WResult WGALShader::CreateBindingMapping(bool bAllowMultipleBindingPerName)
{
  WTempHybridArray<WArrayPtr<const WShaderResourceBinding>, WGALShaderStage::ENUM_COUNT> resourceBinding;
  resourceBinding.SetCount(WGALShaderStage::ENUM_COUNT);
  for (WUInt32 stage = WGALShaderStage::VertexShader; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_Description.HasByteCodeForStage((WGALShaderStage::Enum)stage))
    {
      resourceBinding[stage] = m_Description.m_ByteCodes[stage]->m_ShaderResourceBindings;
    }
  }
  return WShaderResourceBinding::CreateMergedShaderResourceBinding(resourceBinding, m_BindingMapping, bAllowMultipleBindingPerName);
}

void WGALShader::DestroyBindingMapping()
{
  m_BindingMapping.Clear();
}

WResult WGALShader::CreateLayouts(WGALDevice* pDevice, bool bSupportsImmutableSamplers)
{
  WGALBindGroupLayoutCreationDescription BindGroupLayoutDesc[W_GAL_MAX_BIND_GROUPS];
  WGALPipelineLayoutCreationDescription PipelineLayoutDesc;

  const WGALImmutableSamplers::ImmutableSamplers& immutableSamplers = WGALImmutableSamplers::GetImmutableSamplers();
  for (const WShaderResourceBinding& binding : m_BindingMapping)
  {
    if (binding.m_ResourceType == WGALShaderResourceType::PushConstants)
    {
      if (PipelineLayoutDesc.m_PushConstants.m_uiSize != 0)
      {
        WLog::Error("Only one push constants block is supported per shader.");
        return W_FAILURE;
      }
      PipelineLayoutDesc.m_PushConstants.m_Stages = binding.m_Stages;
      PipelineLayoutDesc.m_PushConstants.m_uiOffset = 0;
      PipelineLayoutDesc.m_PushConstants.m_uiSize = (WUInt16)binding.m_pLayout->m_uiTotalSize;
      continue;
    }

    if (binding.m_iBindGroup >= W_GAL_MAX_BIND_GROUPS)
    {
      WLog::Error("Binding set {} for shader resource '{}' is bigger than W_GAL_MAX_BIND_GROUPS.", binding.m_sName.GetData(), binding.m_iBindGroup);
      return W_FAILURE;
    }

    if (bSupportsImmutableSamplers && binding.m_ResourceType == WGALShaderResourceType::Sampler && immutableSamplers.Contains(binding.m_sName))
    {
      BindGroupLayoutDesc[binding.m_iBindGroup].m_ImmutableSamplers.PushBack(binding);
    }
    else
    {
      BindGroupLayoutDesc[binding.m_iBindGroup].m_ResourceBindings.PushBack(binding);
    }
  }

  // There must always be at least one empty set.
  WUInt32 uiMaxBindGroups = 1;
  for (WUInt32 uiBindGroup = 1; uiBindGroup < W_GAL_MAX_BIND_GROUPS; ++uiBindGroup)
  {
    if (!BindGroupLayoutDesc[uiBindGroup].m_ResourceBindings.IsEmpty())
    {
      uiMaxBindGroups = uiBindGroup + 1;
    }
  }

  m_BindGroupLayouts.SetCount(uiMaxBindGroups);
  for (WUInt32 uiBindGroup = 0; uiBindGroup < uiMaxBindGroups; ++uiBindGroup)
  {
    m_BindGroupLayouts[uiBindGroup] = pDevice->CreateBindGroupLayout(BindGroupLayoutDesc[uiBindGroup]);
    if (m_BindGroupLayouts[uiBindGroup].IsInvalidated())
    {
      return W_FAILURE;
    }
    PipelineLayoutDesc.m_BindGroups[uiBindGroup] = m_BindGroupLayouts[uiBindGroup];
  }
  m_hPipelineLayout = pDevice->CreatePipelineLayout(PipelineLayoutDesc);
  if (m_hPipelineLayout.IsInvalidated())
  {
    return W_FAILURE;
  }
  return W_SUCCESS;
}

void WGALShader::DestroyLayouts(WGALDevice* pDevice)
{
  pDevice->DestroyPipelineLayout(m_hPipelineLayout);
  for (WUInt32 uiBindGroup = 0; uiBindGroup < m_BindGroupLayouts.GetCount(); ++uiBindGroup)
  {
    pDevice->DestroyBindGroupLayout(m_BindGroupLayouts[uiBindGroup]);
  }
  m_BindGroupLayouts.Clear();
}

WArrayPtr<const WShaderResourceBinding> WGALShader::GetBindings(WUInt32 uiBindGroup) const
{
  W_ASSERT_DEBUG(uiBindGroup < GetBindGroupCount(), "Bind group index out of range.");
  return m_pDevice->GetBindGroupLayout(m_BindGroupLayouts[uiBindGroup])->GetDescription().m_ResourceBindings;
}

WGALShader::~WGALShader() = default;

WDelegate<void(WShaderUtils::WBuiltinShaderType type, WShaderUtils::WBuiltinShader& out_shader)> WShaderUtils::g_RequestBuiltinShaderCallback;
