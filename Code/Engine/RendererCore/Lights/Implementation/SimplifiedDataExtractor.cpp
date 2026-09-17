#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SimplifiedDataExtractor.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightDataSimplified.h>
W_DEFINE_AS_POD_TYPE(WSimplifiedDataConstants);

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSimplifiedDataCPU, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WSimplifiedDataCPU::WSimplifiedDataCPU() = default;
WSimplifiedDataCPU::~WSimplifiedDataCPU() = default;

WSimplifiedDataGPU::WSimplifiedDataGPU()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  WGALBufferCreationDescription desc;
  desc.m_uiStructSize = 0;
  desc.m_uiTotalSize = sizeof(WSimplifiedDataConstants);
  desc.m_BufferFlags = WGALBufferUsageFlags::ConstantBuffer;
  m_hConstantBuffer = pDevice->CreateBuffer(desc);
}

WSimplifiedDataGPU::~WSimplifiedDataGPU()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  pDevice->DestroyBuffer(m_hConstantBuffer);
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSimplifiedDataExtractor, 1, WRTTIDefaultAllocator<WSimplifiedDataExtractor>)
W_END_DYNAMIC_REFLECTED_TYPE;

WSimplifiedDataExtractor::WSimplifiedDataExtractor(const char* szName)
  : WExtractor(szName)
{
  m_DependsOn.PushBack(WMakeHashedString("WVisibleObjectsExtractor"));
}

WSimplifiedDataExtractor::~WSimplifiedDataExtractor() = default;

void WSimplifiedDataExtractor::PostSortAndBatch(
  const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData)
{
  WSimplifiedDataCPU* pData = W_NEW(WFrameAllocator::GetCurrentAllocator(), WSimplifiedDataCPU);

  pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();
  pData->m_cameraUsageHint = view.GetCameraUsageHint();

  UpdateGpuData(view, pData);
  AddGpuData(view, ref_extractedRenderData);

  ref_extractedRenderData.AddFrameData(pData);
}

WResult WSimplifiedDataExtractor::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return W_SUCCESS;
}

WResult WSimplifiedDataExtractor::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  return W_SUCCESS;
}

void WSimplifiedDataExtractor::UpdateGpuData(const WView& view, const WSimplifiedDataCPU* pData)
{
  W_IGNORE_UNUSED(view);

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  m_DataGPU.m_uiSkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
  m_DataGPU.m_cameraUsageHint = pData->m_cameraUsageHint;

  WSimplifiedDataConstants constants = {};
  constants.SkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;

  pDevice->UpdateBufferForNextFrame(m_DataGPU.m_hConstantBuffer, WMakeByteArrayPtr(&constants, 1), 0);
}

void WSimplifiedDataExtractor::AddGpuData(const WView& view, WExtractedRenderData& ref_extractedRenderData)
{
  const WEnum<WCameraUsageHint> cameraUsageHint = view.GetCameraUsageHint();
  // Reflection specular and sky irradiance textures
  {
    WGALTextureHandle hReflSpec = WReflectionPool::GetReflectionSpecularTexture(view.GetWorld()->GetIndex(), cameraUsageHint);
    ref_extractedRenderData.AddViewDependency(hReflSpec, WGALResourceState::ShaderResource, WGALShaderStageFlags::Auto);
    ref_extractedRenderData.AddTextureBinding(WTempHashedString("ReflectionSpecularTexture"), hReflSpec);

    WGALTextureHandle hSkyIrradiance = WReflectionPool::GetSkyIrradianceTexture();
    ref_extractedRenderData.AddViewDependency(hSkyIrradiance, WGALResourceState::ShaderResource, WGALShaderStageFlags::Auto);
    ref_extractedRenderData.AddTextureBinding(WTempHashedString("SkyIrradianceTexture"), hSkyIrradiance);
  }

  ref_extractedRenderData.AddBufferBinding("WSimplifiedDataConstants", m_DataGPU.m_hConstantBuffer);
}

W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SimplifiedDataExtractor);
