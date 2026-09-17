#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/Components/LightShaftsComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/ClusteredDataExtractor.h>

#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Decals/Implementation/DecalManager.h>
#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
WCVarBool cvar_RenderingLightingVisClusterData("Rendering.Lighting.VisClusterData", false, WCVarFlags::Default, "Enables debug visualization of clustered light data");
WCVarInt cvar_RenderingLightingVisClusterDepthSlice("Rendering.Lighting.VisClusterDepthSlice", -1, WCVarFlags::Default, "Show the debug visualization only for the given depth slice");

namespace
{
  void VisualizeClusteredData(const WView& view, const WClusteredDataCPU* pData, WArrayPtr<WSimdBSphere> boundingSpheres)
  {
    if (!cvar_RenderingLightingVisClusterData)
      return;

    const WCamera* pCamera = view.GetCullingCamera();

    if (pCamera->IsOrthographic())
      return;

    float fAspectRatio = view.GetViewport().width / view.GetViewport().height;

    WMat4 mProj;
    pCamera->GetProjectionMatrix(fAspectRatio, mProj);

    const WMat4& mInvView = pCamera->GetViewMatrix().GetInverse();

    WAngle fFovLeft;
    WAngle fFovRight;
    WAngle fFovBottom;
    WAngle fFovTop;
    WGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop);

    const float fTanLeft = WMath::Tan(fFovLeft);
    const float fTanRight = WMath::Tan(fFovRight);
    const float fTanBottom = WMath::Tan(fFovBottom);
    const float fTanTop = WMath::Tan(fFovTop);

    WColor lineColor = WColor(1.0f, 1.0f, 1.0f, 0.1f);

    const WInt32 debugSlice = cvar_RenderingLightingVisClusterDepthSlice;
    const bool bOnlyOneSlice = debugSlice >= 0;
    const WUInt32 maxSlice = bOnlyOneSlice ? debugSlice + 1 : NUM_CLUSTERS_Z;
    const WUInt32 minSlice = bOnlyOneSlice ? debugSlice : 0;

    bool bDrawBoundingSphere = false;
    WStringBuilder sb;

    for (WUInt32 z = maxSlice; z-- > minSlice;)
    {
      float fZf = GetDepthFromSliceIndex(z);
      float fZn = (z > 0) ? GetDepthFromSliceIndex(z - 1) : 0.0f;
      for (WInt32 y = 0; y < NUM_CLUSTERS_Y; ++y)
      {
        for (WInt32 x = 0; x < NUM_CLUSTERS_X; ++x)
        {
          WUInt32 clusterIndex = GetClusterIndexFromCoord(x, y, z);
          auto& clusterData = pData->m_ClusterData[clusterIndex];

          if (clusterData.counts > 0)
          {
            if (bDrawBoundingSphere)
            {
              WBoundingSphere s = WSimdConversion::ToBSphere(boundingSpheres[clusterIndex]);
              s.TransformFromOrigin(mInvView);
              WDebugRenderer::DrawLineSphere(view.GetHandle(), s, lineColor);
            }
            else
            {
              WVec3 cc[8];
              GetClusterCornerPoints(*pCamera, fZf, fZn, fTanLeft, fTanRight, fTanBottom, fTanTop, x, y, z, cc);

              const float lightCount = (float)GET_LIGHT_INDEX(clusterData.counts);
              const float decalCount = (float)GET_DECAL_INDEX(clusterData.counts);
              const float probeCount = (float)GET_PROBE_INDEX(clusterData.counts);
              const float r = WMath::Clamp(lightCount / 16.0f, 0.0f, 1.0f);
              const float g = WMath::Clamp(decalCount / 16.0f, 0.0f, 1.0f);
              const float b = WMath::Clamp(probeCount / 16.0f, 0.0f, 1.0f);
              const WColor color(r, g, b);

              WDebugRendererTriangle tris[12];
              // back
              tris[0] = WDebugRendererTriangle(cc[0], cc[2], cc[1]);
              tris[1] = WDebugRendererTriangle(cc[2], cc[3], cc[1]);
              // front
              tris[2] = WDebugRendererTriangle(cc[4], cc[5], cc[6]);
              tris[3] = WDebugRendererTriangle(cc[6], cc[5], cc[7]);
              // top
              tris[4] = WDebugRendererTriangle(cc[4], cc[0], cc[5]);
              tris[5] = WDebugRendererTriangle(cc[0], cc[1], cc[5]);
              // bottom
              tris[6] = WDebugRendererTriangle(cc[6], cc[7], cc[2]);
              tris[7] = WDebugRendererTriangle(cc[2], cc[7], cc[3]);
              // left
              tris[8] = WDebugRendererTriangle(cc[4], cc[6], cc[0]);
              tris[9] = WDebugRendererTriangle(cc[0], cc[6], cc[2]);
              // right
              tris[10] = WDebugRendererTriangle(cc[5], cc[1], cc[7]);
              tris[11] = WDebugRendererTriangle(cc[1], cc[3], cc[7]);

              WDebugRenderer::DrawSolidTriangles(view.GetHandle(), tris, color.WithAlpha(0.1f));

              WDebugRendererLine lines[12];
              lines[0] = WDebugRendererLine(cc[4], cc[5]);
              lines[1] = WDebugRendererLine(cc[5], cc[7]);
              lines[2] = WDebugRendererLine(cc[7], cc[6]);
              lines[3] = WDebugRendererLine(cc[6], cc[4]);

              lines[4] = WDebugRendererLine(cc[0], cc[1]);
              lines[5] = WDebugRendererLine(cc[1], cc[3]);
              lines[6] = WDebugRendererLine(cc[3], cc[2]);
              lines[7] = WDebugRendererLine(cc[2], cc[0]);

              lines[8] = WDebugRendererLine(cc[4], cc[0]);
              lines[9] = WDebugRendererLine(cc[5], cc[1]);
              lines[10] = WDebugRendererLine(cc[7], cc[3]);
              lines[11] = WDebugRendererLine(cc[6], cc[2]);

              WDebugRenderer::DrawLines(view.GetHandle(), lines, color);

              if (bOnlyOneSlice)
              {
                sb.SetFormat("L:{}\nD:{}\nR:{}", (WUInt32)lightCount, (WUInt32)decalCount, (WUInt32)probeCount);
                WVec3 textPos = (cc[0] + cc[1] + cc[2] + cc[3] + cc[4] + cc[5] + cc[6] + cc[7]) / 8.0f;
                WDebugRenderer::Draw3DText(view.GetHandle(), sb, textPos, color * 4.0f, 16u, WDebugTextHAlign::Center, WDebugTextVAlign::Center);
              }
            }
          }
        }
      }

      {
        WVec3 leftWidth = pCamera->GetDirRight() * fZf * fTanLeft;
        WVec3 rightWidth = pCamera->GetDirRight() * fZf * fTanRight;
        WVec3 bottomHeight = pCamera->GetDirUp() * fZf * fTanBottom;
        WVec3 topHeight = pCamera->GetDirUp() * fZf * fTanTop;

        WVec3 depthFar = pCamera->GetPosition() + pCamera->GetDirForwards() * fZf;
        WVec3 p0 = depthFar + rightWidth + topHeight;
        WVec3 p1 = depthFar + rightWidth + bottomHeight;
        WVec3 p2 = depthFar + leftWidth + bottomHeight;
        WVec3 p3 = depthFar + leftWidth + topHeight;

        WDebugRendererLine lines[4];
        lines[0] = WDebugRendererLine(p0, p1);
        lines[1] = WDebugRendererLine(p1, p2);
        lines[2] = WDebugRendererLine(p2, p3);
        lines[3] = WDebugRendererLine(p3, p0);

        WDebugRenderer::DrawLines(view.GetHandle(), lines, lineColor);
      }
    }
  }
} // namespace
#endif

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WClusteredDataCPU, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WClusteredDataCPU::WClusteredDataCPU() = default;
WClusteredDataCPU::~WClusteredDataCPU() = default;

WClusteredDataGPU::WClusteredDataGPU()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  {
    WGALBufferCreationDescription desc;

    {
      desc.m_uiStructSize = sizeof(WPerLightData);
      desc.m_uiTotalSize = desc.m_uiStructSize * WClusteredDataCPU::MAX_NUM_LIGHTS;
      desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hLightDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiStructSize = sizeof(WPerDecalData);
      desc.m_uiTotalSize = desc.m_uiStructSize * WClusteredDataCPU::MAX_NUM_DECALS;
      desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hDecalDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiStructSize = sizeof(WPerReflectionProbeData);
      desc.m_uiTotalSize = desc.m_uiStructSize * WClusteredDataCPU::MAX_NUM_REFLECTION_PROBES;
      desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hReflectionProbeDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiStructSize = sizeof(WPerClusterData);
      desc.m_uiTotalSize = desc.m_uiStructSize * NUM_CLUSTERS;

      m_hClusterDataBuffer = pDevice->CreateBuffer(desc);
    }
  }

  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = 0;
    desc.m_uiTotalSize = sizeof(WClusteredDataConstants);
    desc.m_BufferFlags = WGALBufferUsageFlags::ConstantBuffer;
    m_hConstantBuffer = pDevice->CreateBuffer(desc);
  }

  {
    WGALSamplerStateCreationDescription desc;
    desc.m_AddressU = WImageAddressMode::Clamp;
    desc.m_AddressV = WImageAddressMode::Clamp;
    desc.m_AddressW = WImageAddressMode::Clamp;
    desc.m_SampleCompareFunc = WGALCompareFunc::Less;

    m_hShadowSampler = pDevice->CreateSamplerState(desc);
  }

  m_hDecalAtlas = WDecalManager::GetBakedDecalAtlas();

  {
    WGALSamplerStateCreationDescription desc;
    desc.m_AddressU = WImageAddressMode::Clamp;
    desc.m_AddressV = WImageAddressMode::Clamp;
    desc.m_AddressW = WImageAddressMode::Clamp;

    WTextureUtils::ConfigureSampler(WTextureFilterSetting::DefaultQuality, desc);
    desc.m_uiMaxAnisotropy = WMath::Min<WUInt8>(desc.m_uiMaxAnisotropy, 4u);

    m_hDecalAtlasSampler = pDevice->CreateSamplerState(desc);
  }
}

WClusteredDataGPU::~WClusteredDataGPU()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hLightDataBuffer);
  pDevice->DestroyBuffer(m_hDecalDataBuffer);
  pDevice->DestroyBuffer(m_hReflectionProbeDataBuffer);
  pDevice->DestroyBuffer(m_hClusterDataBuffer);
  pDevice->DestroyBuffer(m_hClusterItemBuffer);
  pDevice->DestroyBuffer(m_hConstantBuffer);
  pDevice->DestroySamplerState(m_hShadowSampler);
  pDevice->DestroySamplerState(m_hDecalAtlasSampler);
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WClusteredDataExtractor, 1, WRTTIDefaultAllocator<WClusteredDataExtractor>)
W_END_DYNAMIC_REFLECTED_TYPE;

WClusteredDataExtractor::WClusteredDataExtractor(const char* szName)
  : WExtractor(szName)
{
  m_DependsOn.PushBack(WMakeHashedString("WVisibleObjectsExtractor"));

  m_TempLightsClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_TempDecalsClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_TempReflectionProbeClusters.SetCountUninitialized(NUM_CLUSTERS);

  WMemoryUtils::ZeroFill(m_TempLightsClusters.GetData(), NUM_CLUSTERS);
  WMemoryUtils::ZeroFill(m_TempDecalsClusters.GetData(), NUM_CLUSTERS);
  WMemoryUtils::ZeroFill(m_TempReflectionProbeClusters.GetData(), NUM_CLUSTERS);

  m_ClusterBoundingSpheres.SetCountUninitialized(NUM_CLUSTERS);
  m_ClusterBoundingSpheresRightEye.SetCountUninitialized(NUM_CLUSTERS);
}

WClusteredDataExtractor::~WClusteredDataExtractor() = default;

void WClusteredDataExtractor::PostSortAndBatch(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData)
{
  W_PROFILE_SCOPE("PostSortAndBatch");

  const WCamera* pCamera = view.GetCullingCamera();
  const float fAspectRatio = view.GetViewport().width / view.GetViewport().height;
  const bool bIsStereo = pCamera->IsStereoscopic();

  WMat4 mProj;
  pCamera->GetProjectionMatrix(fAspectRatio, mProj, WCameraEye::Left);
  if (m_mProjection != mProj)
  {
    m_mProjection = mProj;

    FillClusterBoundingSpheres(*pCamera, mProj, m_ClusterBoundingSpheres);
  }

  // For stereo rendering, also compute right eye cluster bounding spheres
  WMat4 mProjRight;
  if (bIsStereo)
  {
    pCamera->GetProjectionMatrix(fAspectRatio, mProjRight, WCameraEye::Right);
    if (m_mProjectionRightEye != mProjRight)
    {
      m_mProjectionRightEye = mProjRight;

      FillClusterBoundingSpheres(*pCamera, mProjRight, m_ClusterBoundingSpheresRightEye);
    }
  }

  WClusteredDataCPU* pData = W_NEW(WFrameAllocator::GetCurrentAllocator(), WClusteredDataCPU);
  pData->m_ClusterData = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WPerClusterData, NUM_CLUSTERS);

  WMat4 tmp = pCamera->GetViewMatrix(WCameraEye::Left);
  WSimdMat4f viewMatrix = WSimdConversion::ToMat4(tmp);

  pCamera->GetProjectionMatrix(fAspectRatio, tmp, WCameraEye::Left);
  WSimdMat4f projectionMatrix = WSimdConversion::ToMat4(tmp);
  WSimdMat4f viewProjectionMatrix = projectionMatrix * viewMatrix;

  // For stereo, also prepare the right eye matrices
  WSimdMat4f viewMatrixRight;
  WSimdMat4f projectionMatrixRight;
  WSimdMat4f viewProjectionMatrixRight;
  if (bIsStereo)
  {
    tmp = pCamera->GetViewMatrix(WCameraEye::Right);
    viewMatrixRight = WSimdConversion::ToMat4(tmp);

    pCamera->GetProjectionMatrix(fAspectRatio, tmp, WCameraEye::Right);
    projectionMatrixRight = WSimdConversion::ToMat4(tmp);
    viewProjectionMatrixRight = projectionMatrixRight * viewMatrixRight;
  }

  // Lights
  {
    W_PROFILE_SCOPE("Lights");
    m_TempLightData.Clear();

    WUInt32 uiBrightestDirectionalLightIndex = WInvalidIndex;
    float fBrightestDirectionalLightIntensity = 0.0f;

    auto batchList = ref_extractedRenderData.GetRenderDataBatchesWithCategory(WDefaultRenderDataCategories::Light);
    const WUInt32 uiBatchCount = batchList.GetBatchCount();
    for (WUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const WRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<WRenderData>(); it.IsValid(); ++it)
      {
        const WUInt32 uiLightIndex = m_TempLightData.GetCount();

        if (uiLightIndex == WClusteredDataCPU::MAX_NUM_LIGHTS)
        {
          WLog::Warning("Maximum number of lights reached ({0}). Further lights will be discarded.", WClusteredDataCPU::MAX_NUM_LIGHTS);
          break;
        }

        if (auto pPointLightRenderData = WDynamicCast<const WPointLightRenderData*>(it))
        {
          FillPointLightData(m_TempLightData.ExpandAndGetRef(), pPointLightRenderData);

          WSimdBSphere pointLightSphere = WSimdBSphere(WSimdConversion::ToVec3(pPointLightRenderData->m_vGlobalPosition), pPointLightRenderData->m_fRange + pPointLightRenderData->m_fLength * 0.5f);
          RasterizeSphere(pointLightSphere, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheres.GetData());

          // For stereo, also rasterize against right eye clusters (union of both eyes)
          if (bIsStereo)
          {
            RasterizeSphere(pointLightSphere, uiLightIndex, viewMatrixRight, projectionMatrixRight, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheresRightEye.GetData());
          }

          if (false)
          {
            WSimdBSphere viewSpaceSphere(viewMatrix.TransformPosition(pointLightSphere.GetCenter()), pointLightSphere.GetRadius());
            WSimdBBox ssb = GetScreenSpaceBounds(viewSpaceSphere, projectionMatrix);
            float minX = ((float)ssb.m_Min.x() * 0.5f + 0.5f) * view.GetViewport().width;
            float maxX = ((float)ssb.m_Max.x() * 0.5f + 0.5f) * view.GetViewport().width;
            float minY = ((float)ssb.m_Max.y() * -0.5f + 0.5f) * view.GetViewport().height;
            float maxY = ((float)ssb.m_Min.y() * -0.5f + 0.5f) * view.GetViewport().height;

            WRectFloat rect(minX, minY, maxX - minX, maxY - minY);
            WDebugRenderer::Draw2DRectangle(view.GetHandle(), rect, 0.0f, WColor::Blue.WithAlpha(0.3f));
          }
        }
        else if (auto pSpotLightRenderData = WDynamicCast<const WSpotLightRenderData*>(it))
        {
          FillSpotLightData(m_TempLightData.ExpandAndGetRef(), pSpotLightRenderData);

          WAngle halfAngle = pSpotLightRenderData->m_OuterSpotAngle / 2.0f;

          BoundingCone cone;
          cone.m_PositionAndRange = WSimdConversion::ToVec3(pSpotLightRenderData->m_vGlobalPosition);
          cone.m_PositionAndRange.SetW(pSpotLightRenderData->m_fRange);
          cone.m_ForwardDir = WSimdConversion::ToVec3(pSpotLightRenderData->m_qGlobalRotation * WVec3(1.0f, 0.0f, 0.0f));
          cone.m_SinCosAngle = WSimdVec4f(WMath::Sin(halfAngle), WMath::Cos(halfAngle), 0.0f);
          RasterizeSpotLight(cone, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheres.GetData());

          // For stereo, also rasterize against right eye clusters (union of both eyes)
          if (bIsStereo)
          {
            RasterizeSpotLight(cone, uiLightIndex, viewMatrixRight, projectionMatrixRight, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheresRightEye.GetData());
          }
        }
        else if (auto pDirLightRenderData = WDynamicCast<const WDirectionalLightRenderData*>(it))
        {
          FillDirLightData(m_TempLightData.ExpandAndGetRef(), pDirLightRenderData);

          RasterizeDirLight(pDirLightRenderData, uiLightIndex, m_TempLightsClusters.GetArrayPtr());
          // Note: Directional lights affect all clusters, so no need for separate stereo handling

          const float fIntensity = pDirLightRenderData->m_fIntensity * WColor(pDirLightRenderData->m_LightColor).GetLuminance();
          if (fIntensity > fBrightestDirectionalLightIntensity)
          {
            uiBrightestDirectionalLightIndex = uiLightIndex;
            fBrightestDirectionalLightIntensity = fIntensity;
          }
        }
        else if (auto pFillLightRenderData = WDynamicCast<const WFillLightRenderData*>(it))
        {
          FillFillLightData(m_TempLightData.ExpandAndGetRef(), pFillLightRenderData);

          WSimdBSphere fillLightSphere = WSimdBSphere(WSimdConversion::ToVec3(pFillLightRenderData->m_vGlobalPosition), pFillLightRenderData->m_fRange);
          RasterizeSphere(fillLightSphere, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheres.GetData());

          // For stereo, also rasterize against right eye clusters (union of both eyes)
          if (bIsStereo)
          {
            RasterizeSphere(fillLightSphere, uiLightIndex, viewMatrixRight, projectionMatrixRight, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheresRightEye.GetData());
          }
        }
        else if (auto pFogRenderData = WDynamicCast<const WFogRenderData*>(it))
        {
          const float fogBaseHeight = pFogRenderData->m_fBaseHeight;
          float fogHeightFalloff = pFogRenderData->m_fHeightFalloff > 0.0f ? WMath::Ln(0.0001f) / pFogRenderData->m_fHeightFalloff : 0.0f;

          const float fogAtCameraPos = fogHeightFalloff * (pCamera->GetPosition().z - fogBaseHeight);
          if (fogAtCameraPos >= 80.0f) // Prevent infs
          {
            fogHeightFalloff = 0.0f;
          }

          pData->m_fFogHeight = -fogHeightFalloff * fogBaseHeight;
          pData->m_fFogHeightFalloff = fogHeightFalloff;
          pData->m_fFogDensityAtCameraPos = WMath::Exp(WMath::Clamp(fogAtCameraPos, -80.0f, 80.0f)); // Prevent infs
          pData->m_fFogDensity = pFogRenderData->m_fDensity;
          pData->m_fFogInvSkyDistance = pFogRenderData->m_fInvSkyDistance;
          pData->m_fFogStartDistance = pFogRenderData->m_fFogStartDistance;

          pData->m_FogColor = pFogRenderData->m_Color;
        }
        else if (auto pLightShaftsRenderData = WDynamicCast<const WLightShaftsRenderData*>(it))
        {
          pData->m_vLightShaftsDirection = pLightShaftsRenderData->m_vDirection;
          pData->m_fLightShaftsIntensity = pLightShaftsRenderData->m_fIntensity;
          pData->m_fLightShaftsMaxBrightness = pLightShaftsRenderData->m_fMaxBrightness;
          pData->m_fLightShaftsBrightnessThreshold = pLightShaftsRenderData->m_fBrightnessThreshold;
          pData->m_fLightShaftsDiskMaskRadius = pLightShaftsRenderData->m_fDiskMaskRadius;
          pData->m_LightShaftsTintColor = pLightShaftsRenderData->m_TintColor;
        }
        else
        {
          WLog::Warning("Unhandled render data type '{}' in 'Light' category", it->GetDynamicRTTI()->GetTypeName());
        }
      }
    }

    pData->m_LightData = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WPerLightData, m_TempLightData.GetCount());
    pData->m_LightData.CopyFrom(m_TempLightData);

    pData->m_uiBrightestDirectionalLightIndex = uiBrightestDirectionalLightIndex;
    pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();
    pData->m_cameraUsageHint = view.GetCameraUsageHint();
  }

  // Decals
  {
    W_PROFILE_SCOPE("Decals");
    m_TempDecalData.Clear();

    auto batchList = ref_extractedRenderData.GetRenderDataBatchesWithCategory(WDefaultRenderDataCategories::Decal);
    const WUInt32 uiBatchCount = batchList.GetBatchCount();
    for (WUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const WRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<WRenderData>(); it.IsValid(); ++it)
      {
        const WUInt32 uiDecalIndex = m_TempDecalData.GetCount();

        if (uiDecalIndex == WClusteredDataCPU::MAX_NUM_DECALS)
        {
          WLog::Warning("Maximum number of decals reached ({0}). Further decals will be discarded.", WClusteredDataCPU::MAX_NUM_DECALS);
          break;
        }

        if (auto pDecalRenderData = WDynamicCast<const WDecalRenderData*>(it))
        {
          FillDecalData(m_TempDecalData.ExpandAndGetRef(), pDecalRenderData);

          const WVec4 rotationValues = pDecalRenderData->m_qGlobalRotation;
          const WQuat rotation(rotationValues.x, rotationValues.y, rotationValues.z, rotationValues.w);
          const WTransform decalTransform = WTransform::Make(pDecalRenderData->m_vGlobalPosition, rotation, pDecalRenderData->m_vGlobalScale);
          RasterizeBox(decalTransform, uiDecalIndex, viewMatrix, viewProjectionMatrix, m_TempDecalsClusters.GetData(), m_ClusterBoundingSpheres.GetData());

          // For stereo, also rasterize against right eye clusters (union of both eyes)
          if (bIsStereo)
          {
            RasterizeBox(decalTransform, uiDecalIndex, viewMatrixRight, viewProjectionMatrixRight, m_TempDecalsClusters.GetData(), m_ClusterBoundingSpheresRightEye.GetData());
          }
        }
        else
        {
          WLog::Warning("Unhandled render data type '{}' in 'Decal' category", it->GetDynamicRTTI()->GetTypeName());
        }
      }
    }

    pData->m_DecalData = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WPerDecalData, m_TempDecalData.GetCount());
    pData->m_DecalData.CopyFrom(m_TempDecalData);
  }

  // Reflection Probes
  {
    W_PROFILE_SCOPE("Probes");
    m_TempReflectionProbeData.Clear();

    auto batchList = ref_extractedRenderData.GetRenderDataBatchesWithCategory(WDefaultRenderDataCategories::ReflectionProbe);
    const WUInt32 uiBatchCount = batchList.GetBatchCount();
    for (WUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const WRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<WRenderData>(); it.IsValid(); ++it)
      {
        const WUInt32 uiProbeIndex = m_TempReflectionProbeData.GetCount();

        if (uiProbeIndex == WClusteredDataCPU::MAX_NUM_REFLECTION_PROBES)
        {
          WLog::Warning("Maximum number of reflection probes reached ({0}). Further reflection probes will be discarded.", WClusteredDataCPU::MAX_NUM_REFLECTION_PROBES);
          break;
        }

        if (auto pReflectionProbeRenderData = WDynamicCast<const WReflectionProbeRenderData*>(it))
        {
          auto& probeData = m_TempReflectionProbeData.ExpandAndGetRef();
          FillReflectionProbeData(probeData, pReflectionProbeRenderData);

          const WVec3 vFullScale = pReflectionProbeRenderData->m_vHalfExtents.CompMul(pReflectionProbeRenderData->m_GlobalTransform.m_vScale);

          bool bRasterizeSphere = false;
          float fMaxRadius = 0.0f;
          if (pReflectionProbeRenderData->m_uiIndex & REFLECTION_PROBE_IS_SPHERE)
          {
            constexpr float fSphereConstant = (4.0f / 3.0f) * WMath::Pi<float>();
            fMaxRadius = WMath::Max(WMath::Max(WMath::Abs(vFullScale.x), WMath::Abs(vFullScale.y)), WMath::Abs(vFullScale.z));
            const float fSphereVolume = fSphereConstant * WMath::Pow(fMaxRadius, 3.0f);
            const float fBoxVolume = WMath::Abs(vFullScale.x * vFullScale.y * vFullScale.z * 8);
            if (fSphereVolume < fBoxVolume)
            {
              bRasterizeSphere = true;
            }
          }


          if (bRasterizeSphere)
          {
            WSimdBSphere pointLightSphere =
              WSimdBSphere(WSimdConversion::ToVec3(pReflectionProbeRenderData->m_GlobalTransform.m_vPosition), fMaxRadius);
            RasterizeSphere(
              pointLightSphere, uiProbeIndex, viewMatrix, projectionMatrix, m_TempReflectionProbeClusters.GetData(), m_ClusterBoundingSpheres.GetData());

            // For stereo, also rasterize against right eye clusters (union of both eyes)
            if (bIsStereo)
            {
              RasterizeSphere(
                pointLightSphere, uiProbeIndex, viewMatrixRight, projectionMatrixRight, m_TempReflectionProbeClusters.GetData(), m_ClusterBoundingSpheresRightEye.GetData());
            }
          }
          else
          {
            WTransform transform = pReflectionProbeRenderData->m_GlobalTransform;
            transform.m_vScale = vFullScale.CompMul(probeData.InfluenceScale.GetAsVec3());
            transform.m_vPosition += transform.m_qRotation * vFullScale.CompMul(probeData.InfluenceShift.GetAsVec3());

            // const WBoundingBox aabb(WVec3(-1.0f), WVec3(1.0f));
            // WDebugRenderer::DrawLineBox(view.GetHandle(), aabb, WColor::DarkBlue, transform);

            RasterizeBox(transform, uiProbeIndex, viewMatrix, viewProjectionMatrix, m_TempReflectionProbeClusters.GetData(), m_ClusterBoundingSpheres.GetData());

            // For stereo, also rasterize against right eye clusters (union of both eyes)
            if (bIsStereo)
            {
              RasterizeBox(transform, uiProbeIndex, viewMatrixRight, viewProjectionMatrixRight, m_TempReflectionProbeClusters.GetData(), m_ClusterBoundingSpheresRightEye.GetData());
            }
          }
        }
        else
        {
          WLog::Warning("Unhandled render data type '{}' in 'ReflectionProbe' category", it->GetDynamicRTTI()->GetTypeName());
        }
      }
    }

    pData->m_ReflectionProbeData = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WPerReflectionProbeData, m_TempReflectionProbeData.GetCount());
    pData->m_ReflectionProbeData.CopyFrom(m_TempReflectionProbeData);
  }

  FillItemListAndClusterData(pData);

  UpdateGpuData(view, pData);
  AddGpuData(view, ref_extractedRenderData);

  ref_extractedRenderData.AddFrameData(pData);


#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  VisualizeClusteredData(view, pData, m_ClusterBoundingSpheres);
#endif
}

WResult WClusteredDataExtractor::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return W_SUCCESS;
}


WResult WClusteredDataExtractor::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  return W_SUCCESS;
}

namespace
{
  W_FORCE_INLINE WUInt32 MakeDecalIndex(WUInt32 uiDecalIndex)
  {
    return uiDecalIndex << DECAL_SHIFT;
  }

  W_FORCE_INLINE WUInt32 MakeProbeIndex(WUInt32 uiReflectionProbeIndex)
  {
    return uiReflectionProbeIndex << PROBE_SHIFT;
  }
} // namespace

void WClusteredDataExtractor::FillItemListAndClusterData(WClusteredDataCPU* pData)
{
  W_PROFILE_SCOPE("FillItemListAndClusterData");
  m_TempClusterItemList.Clear();

  const WUInt32 uiNumLights = m_TempLightData.GetCount();
  const WUInt32 uiMaxLightBlockIndex = (uiNumLights + 31) / 32;

  const WUInt32 uiNumDecals = m_TempDecalData.GetCount();
  const WUInt32 uiMaxDecalBlockIndex = (uiNumDecals + 31) / 32;

  const WUInt32 uiNumReflectionProbes = m_TempReflectionProbeData.GetCount();
  const WUInt32 uiMaxReflectionProbeBlockIndex = (uiNumReflectionProbes + 31) / 32;

  const WUInt32 uiWorstCase = WMath::Max(uiNumLights, uiNumDecals, uiNumReflectionProbes);
  for (WUInt32 i = 0; i < NUM_CLUSTERS; ++i)
  {
    const WUInt32 uiOffset = m_TempClusterItemList.GetCount();
    WUInt32 uiLightCount = 0;

    // We expand m_TempClusterItemList by the worst case this loop can produce and then cut it down again to the actual size once we have filled the data. This makes sure we do not waste time on boundary checks or potential out of line calls like PushBack or PushBackUnchecked.
    m_TempClusterItemList.SetCountUninitialized(uiOffset + uiWorstCase);
    WUInt32* pTempClusterItemListRange = m_TempClusterItemList.GetData() + uiOffset;

    // Lights
    {
      auto& tempCluster = m_TempLightsClusters[i];
      for (WUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxLightBlockIndex; ++uiBlockIndex)
      {
        WUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          WUInt32 uiLightIndex = WMath::FirstBitLow(mask);
          mask &= mask - 1;

          uiLightIndex += uiBlockIndex * 32;
          pTempClusterItemListRange[uiLightCount] = uiLightIndex;
          ++uiLightCount;
        }

        tempCluster.m_BitMask[uiBlockIndex] = 0;
      }
    }

    WUInt32 uiDecalCount = 0;

    // Decals
    {
      auto& tempCluster = m_TempDecalsClusters[i];
      for (WUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxDecalBlockIndex; ++uiBlockIndex)
      {
        WUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          WUInt32 uiDecalIndex = WMath::FirstBitLow(mask);
          mask &= mask - 1;

          uiDecalIndex += uiBlockIndex * 32;

          const WUInt32 item = pTempClusterItemListRange[uiDecalCount];
          pTempClusterItemListRange[uiDecalCount] = (uiDecalCount < uiLightCount ? item : 0) | MakeDecalIndex(uiDecalIndex);

          ++uiDecalCount;
        }

        tempCluster.m_BitMask[uiBlockIndex] = 0;
      }
    }

    WUInt32 uiReflectionProbeCount = 0;
    const WUInt32 uiMaxUsed = WMath::Max(uiLightCount, uiDecalCount);
    // Reflection Probes
    {
      auto& tempCluster = m_TempReflectionProbeClusters[i];
      for (WUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxReflectionProbeBlockIndex; ++uiBlockIndex)
      {
        WUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          WUInt32 uiReflectionProbeIndex = WMath::FirstBitLow(mask);
          mask &= mask - 1;

          uiReflectionProbeIndex += uiBlockIndex * 32;

          const WUInt32 item = pTempClusterItemListRange[uiReflectionProbeCount];
          pTempClusterItemListRange[uiReflectionProbeCount] = (uiReflectionProbeCount < uiMaxUsed ? item : 0) | MakeProbeIndex(uiReflectionProbeIndex);

          ++uiReflectionProbeCount;
        }

        tempCluster.m_BitMask[uiBlockIndex] = 0;
      }
    }

    // Cut down the array to the actual number of elements we have written.
    const WUInt32 uiActualCase = WMath::Max(uiLightCount, uiDecalCount, uiReflectionProbeCount);
    m_TempClusterItemList.SetCountUninitialized(uiOffset + uiActualCase);

    auto& clusterData = pData->m_ClusterData[i];
    clusterData.offset = uiOffset;
    clusterData.counts = uiLightCount | MakeDecalIndex(uiDecalCount) | MakeProbeIndex(uiReflectionProbeCount);
  }

  pData->m_ClusterItemList = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WUInt32, m_TempClusterItemList.GetCount());
  pData->m_ClusterItemList.CopyFrom(m_TempClusterItemList);
}

void WClusteredDataExtractor::UpdateGpuData(const WView& view, const WClusteredDataCPU* pData)
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  m_DataGPU.m_uiSkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
  m_DataGPU.m_cameraUsageHint = pData->m_cameraUsageHint;

  if (!pData->m_ClusterItemList.IsEmpty())
  {
    if (!pData->m_LightData.IsEmpty())
    {
      pDevice->UpdateBufferForNextFrame(m_DataGPU.m_hLightDataBuffer, pData->m_LightData.ToByteArray(), 0);
    }

    if (!pData->m_DecalData.IsEmpty())
    {
      pDevice->UpdateBufferForNextFrame(m_DataGPU.m_hDecalDataBuffer, pData->m_DecalData.ToByteArray(), 0);
    }

    if (!pData->m_ReflectionProbeData.IsEmpty())
    {
      pDevice->UpdateBufferForNextFrame(m_DataGPU.m_hReflectionProbeDataBuffer, pData->m_ReflectionProbeData.ToByteArray(), 0);
    }

    if (m_DataGPU.m_hClusterItemBuffer.IsInvalidated() == false)
    {
      auto& bufferDesc = pDevice->GetBuffer(m_DataGPU.m_hClusterItemBuffer)->GetDescription();
      if (bufferDesc.m_uiTotalSize < pData->m_ClusterItemList.ToByteArray().GetCount())
      {
        pDevice->DestroyBuffer(m_DataGPU.m_hClusterItemBuffer);
      }
    }

    if (m_DataGPU.m_hClusterItemBuffer.IsInvalidated())
    {
      const WUInt32 uiNumItems = WMemoryUtils::AlignSize(pData->m_ClusterItemList.GetCount(), WMath::PowerOfTwo_Ceil(WUInt32(NUM_CLUSTERS)));

      WGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(WUInt32);
      desc.m_uiTotalSize = uiNumItems * desc.m_uiStructSize;
      desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
      desc.m_ResourceAccess.m_bImmutable = false;
      m_DataGPU.m_hClusterItemBuffer = pDevice->CreateBuffer(desc);
    }

    pDevice->UpdateBufferForNextFrame(m_DataGPU.m_hClusterItemBuffer, pData->m_ClusterItemList.ToByteArray(), 0);
  }

  pDevice->UpdateBufferForNextFrame(m_DataGPU.m_hClusterDataBuffer, pData->m_ClusterData.ToByteArray(), 0);

  WClusteredDataConstants constants = {};
  constants.DepthSliceScale = s_fDepthSliceScale;
  constants.DepthSliceBias = s_fDepthSliceBias;
  constants.InvTileSize = WVec2(NUM_CLUSTERS_X / view.GetViewport().width, NUM_CLUSTERS_Y / view.GetViewport().height);
  constants.NumLights = pData->m_LightData.GetCount();
  constants.NumDecals = pData->m_DecalData.GetCount();

  constants.BrightestDirectionalLightIndex = pData->m_uiBrightestDirectionalLightIndex;
  constants.SkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;

  constants.FogHeight = pData->m_fFogHeight;
  constants.FogHeightFalloff = pData->m_fFogHeightFalloff;
  constants.FogDensityAtCameraPos = pData->m_fFogDensityAtCameraPos;
  constants.FogDensity = pData->m_fFogDensity;
  constants.FogColor = pData->m_FogColor;
  constants.FogInvSkyDistance = pData->m_fFogInvSkyDistance;
  constants.FogStartDistance = pData->m_fFogStartDistance;

  pDevice->UpdateBufferForNextFrame(m_DataGPU.m_hConstantBuffer, WMakeByteArrayPtr(&constants, 1), 0);
}

void WClusteredDataExtractor::AddGpuData(const WView& view, WExtractedRenderData& ref_extractedRenderData)
{
  const WEnum<WCameraUsageHint> cameraUsageHint = view.GetCameraUsageHint();

  // Shadow atlas texture
  if (cameraUsageHint != WCameraUsageHint::Shadow)
  {
    ref_extractedRenderData.AddViewDependency(WShadowPool::GetShadowAtlasTexture(), WGALResourceState::DepthStencilRead, WGALShaderStageFlags::Auto);
    ref_extractedRenderData.AddTextureBinding(WTempHashedString("ShadowAtlasTexture"), WShadowPool::GetShadowAtlasTexture());
  }

  // Decal runtime atlas texture
  ref_extractedRenderData.AddViewDependency(WDecalManager::GetRuntimeDecalAtlasTexture(), WGALResourceState::ShaderResource, WGALShaderStageFlags::Auto);
  ref_extractedRenderData.AddTextureBinding(WTempHashedString("DecalRuntimeAtlasTexture"), WDecalManager::GetRuntimeDecalAtlasTexture());

  // Reflection specular and sky irradiance textures
  const WGALTextureHandle hReflSpec = WReflectionPool::GetReflectionSpecularTexture(view.GetWorld()->GetIndex(), cameraUsageHint);
  ref_extractedRenderData.AddViewDependency(hReflSpec, WGALResourceState::ShaderResource, WGALShaderStageFlags::Auto);
  ref_extractedRenderData.AddTextureBinding(WTempHashedString("ReflectionSpecularTexture"), hReflSpec);

  const WGALTextureHandle hSkyIrradiance = WReflectionPool::GetSkyIrradianceTexture();
  ref_extractedRenderData.AddViewDependency(hSkyIrradiance, WGALResourceState::ShaderResource, WGALShaderStageFlags::Auto);
  ref_extractedRenderData.AddTextureBinding(WTempHashedString("SkyIrradianceTexture"), hSkyIrradiance);

  ref_extractedRenderData.AddBufferBinding("perLightDataBuffer", m_DataGPU.m_hLightDataBuffer);
  ref_extractedRenderData.AddBufferBinding("perDecalDataBuffer", m_DataGPU.m_hDecalDataBuffer);
  ref_extractedRenderData.AddBufferBinding("perDecalAtlasDataBuffer", WDecalManager::GetDecalAtlasDataBufferForRendering());
  ref_extractedRenderData.AddBufferBinding("perPerReflectionProbeDataBuffer", m_DataGPU.m_hReflectionProbeDataBuffer);
  ref_extractedRenderData.AddBufferBinding("perClusterDataBuffer", m_DataGPU.m_hClusterDataBuffer);
  ref_extractedRenderData.AddBufferBinding("clusterItemBuffer", m_DataGPU.m_hClusterItemBuffer);
  ref_extractedRenderData.AddBufferBinding("shadowDataBuffer", WShadowPool::GetShadowDataBuffer());
  ref_extractedRenderData.AddSamplerBinding("ShadowSampler", m_DataGPU.m_hShadowSampler);
  WResourceLock<WDecalAtlasResource> pDecalAtlas(m_DataGPU.m_hDecalAtlas, WResourceAcquireMode::AllowLoadingFallback);
  ref_extractedRenderData.AddTextureBinding("DecalAtlasBaseColorTexture", pDecalAtlas->GetBaseColorTexture());
  ref_extractedRenderData.AddTextureBinding("DecalAtlasNormalTexture", pDecalAtlas->GetNormalTexture());
  ref_extractedRenderData.AddTextureBinding("DecalAtlasORMTexture", pDecalAtlas->GetORMTexture());
  ref_extractedRenderData.AddSamplerBinding("DecalAtlasSampler", m_DataGPU.m_hDecalAtlasSampler);
  ref_extractedRenderData.AddBufferBinding("WClusteredDataConstants", m_DataGPU.m_hConstantBuffer);
}



W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ClusteredDataExtractor);
