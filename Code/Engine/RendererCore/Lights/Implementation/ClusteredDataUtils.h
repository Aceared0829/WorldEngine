#pragma once

#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/FillLightComponent.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>
W_DEFINE_AS_POD_TYPE(WPerLightData);
W_DEFINE_AS_POD_TYPE(WPerDecalData);
W_DEFINE_AS_POD_TYPE(WPerReflectionProbeData);
W_DEFINE_AS_POD_TYPE(WPerClusterData);

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Utilities/GraphicsUtils.h>

namespace
{
  ///\todo Make this configurable.
  static float s_fMinLightDistance = 5.0f;
  static float s_fMaxLightDistance = 500.0f;

  static float s_fDepthSliceScale = (NUM_CLUSTERS_Z - 1) / (WMath::Log2(s_fMaxLightDistance) - WMath::Log2(s_fMinLightDistance));
  static float s_fDepthSliceBias = -s_fDepthSliceScale * WMath::Log2(s_fMinLightDistance) + 1.0f;

  W_ALWAYS_INLINE float GetDepthFromSliceIndex(WUInt32 uiSliceIndex)
  {
    return WMath::Pow(2.0f, (uiSliceIndex - s_fDepthSliceBias + 1.0f) / s_fDepthSliceScale);
  }

  W_ALWAYS_INLINE WUInt32 GetSliceIndexFromDepth(float fLinearDepth)
  {
    return WMath::Clamp((WInt32)(WMath::Log2(fLinearDepth) * s_fDepthSliceScale + s_fDepthSliceBias), 0, NUM_CLUSTERS_Z - 1);
  }

  W_ALWAYS_INLINE WUInt32 GetClusterIndexFromCoord(WUInt32 x, WUInt32 y, WUInt32 z)
  {
    return z * NUM_CLUSTERS_XY + y * NUM_CLUSTERS_X + x;
  }

  // in order: tlf, trf, blf, brf, tln, trn, bln, brn
  W_FORCE_INLINE void GetClusterCornerPoints(
    const WCamera& camera, float fZf, float fZn, float fTanLeft, float fTanRight, float fTanBottom, float fTanTop, WInt32 x, WInt32 y, WInt32 z, WVec3* out_pCorners)
  {
    const WVec3& pos = camera.GetPosition();
    const WVec3& dirForward = camera.GetDirForwards();
    const WVec3& dirRight = camera.GetDirRight();
    const WVec3& dirUp = camera.GetDirUp();

    const float fStartXf = fZf * fTanLeft;
    const float fStartYf = fZf * fTanBottom;
    const float fEndXf = fZf * fTanRight;
    const float fEndYf = fZf * fTanTop;

    float fStepXf = (fEndXf - fStartXf) / NUM_CLUSTERS_X;
    float fStepYf = (fEndYf - fStartYf) / NUM_CLUSTERS_Y;

    float fXf = fStartXf + x * fStepXf;
    float fYf = fStartYf + y * fStepYf;

    out_pCorners[0] = pos + dirForward * fZf + dirRight * fXf - dirUp * fYf;
    out_pCorners[1] = out_pCorners[0] + dirRight * fStepXf;
    out_pCorners[2] = out_pCorners[0] - dirUp * fStepYf;
    out_pCorners[3] = out_pCorners[2] + dirRight * fStepXf;

    const float fStartXn = fZn * fTanLeft;
    const float fStartYn = fZn * fTanBottom;
    const float fEndXn = fZn * fTanRight;
    const float fEndYn = fZn * fTanTop;

    float fStepXn = (fEndXn - fStartXn) / NUM_CLUSTERS_X;
    float fStepYn = (fEndYn - fStartYn) / NUM_CLUSTERS_Y;
    float fXn = fStartXn + x * fStepXn;
    float fYn = fStartYn + y * fStepYn;

    out_pCorners[4] = pos + dirForward * fZn + dirRight * fXn - dirUp * fYn;
    out_pCorners[5] = out_pCorners[4] + dirRight * fStepXn;
    out_pCorners[6] = out_pCorners[4] - dirUp * fStepYn;
    out_pCorners[7] = out_pCorners[6] + dirRight * fStepXn;
  }

  void FillClusterBoundingSpheres(const WCamera& camera, const WMat4& mProj, WArrayPtr<WSimdBSphere> clusterBoundingSpheres)
  {
    W_PROFILE_SCOPE("FillClusterBoundingSpheres");

    ///\todo proper implementation for orthographic views
    if (camera.IsOrthographic())
      return;

    WSimdVec4f stepScale;
    WSimdVec4f tanLBLB;
    {
      WAngle fFovLeft;
      WAngle fFovRight;
      WAngle fFovBottom;
      WAngle fFovTop;
      WGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop);

      const float fTanLeft = WMath::Tan(fFovLeft);
      const float fTanRight = WMath::Tan(fFovRight);
      const float fTanBottom = WMath::Tan(fFovBottom);
      const float fTanTop = WMath::Tan(fFovTop);

      float fStepXf = (fTanRight - fTanLeft) / NUM_CLUSTERS_X;
      float fStepYf = (fTanTop - fTanBottom) / NUM_CLUSTERS_Y;

      stepScale = WSimdVec4f(fStepXf, fStepYf, fStepXf, fStepYf);
      tanLBLB = WSimdVec4f(fTanLeft, fTanBottom, fTanLeft, fTanBottom);
    }

    const WSimdVec4f dirForward = WSimdVec4f(0, 0, 1, 0);
    const WSimdVec4f dirRight = WSimdVec4f(1, 0, 0, 0);
    const WSimdVec4f dirUp = WSimdVec4f(0, 1, 0, 0);


    WSimdVec4f fZn = WSimdVec4f::MakeZero();
    WSimdVec4f cc[8];

    for (WInt32 z = 0; z < NUM_CLUSTERS_Z; z++)
    {
      WSimdVec4f fZf = WSimdVec4f(GetDepthFromSliceIndex(z));
      WSimdVec4f zff_znn = fZf.GetCombined<WSwizzle::XXXX>(fZn);
      WSimdVec4f steps = zff_znn.CompMul(stepScale);

      WSimdVec4f depthF = dirForward * fZf.x();
      WSimdVec4f depthN = dirForward * fZn.x();

      WSimdVec4f startLBLB = zff_znn.CompMul(tanLBLB);

      for (WInt32 y = 0; y < NUM_CLUSTERS_Y; y++)
      {
        for (WInt32 x = 0; x < NUM_CLUSTERS_X; x++)
        {
          WSimdVec4f xyxy = WSimdVec4i(x, y, x, y).ToFloat();
          WSimdVec4f xfyf = startLBLB + (xyxy).CompMul(steps);

          cc[0] = depthF + dirRight * xfyf.x() - dirUp * xfyf.y();
          cc[1] = cc[0] + dirRight * steps.x();
          cc[2] = cc[0] - dirUp * steps.y();
          cc[3] = cc[2] + dirRight * steps.x();

          cc[4] = depthN + dirRight * xfyf.z() - dirUp * xfyf.w();
          cc[5] = cc[4] + dirRight * steps.z();
          cc[6] = cc[4] - dirUp * steps.w();
          cc[7] = cc[6] + dirRight * steps.z();

          clusterBoundingSpheres[GetClusterIndexFromCoord(x, y, z)] = WSimdBSphere::MakeFromPoints(cc, 8);
        }
      }

      fZn = fZf;
    }
  }

  W_ALWAYS_INLINE void FillLightData(WPerLightData& out_perLightData, const WLightRenderData* pLightRenderData, WUInt8 uiType)
  {
    WMemoryUtils::ZeroFill(&out_perLightData, 1);

    WColorLinearUB lightColor = pLightRenderData->m_LightColor;
    lightColor.a = uiType;

    out_perLightData.colorAndType = *reinterpret_cast<WUInt32*>(&lightColor.r);
    out_perLightData.intensity = pLightRenderData->m_fIntensity;
    out_perLightData.specularMultiplierAndRadius = WShaderUtils::Float2ToRG16F(WVec2(pLightRenderData->m_fSpecularMultiplier, pLightRenderData->m_fRadius));
    out_perLightData.shadowDataOffsetAndFadeOut = pLightRenderData->m_uiShadowDataOffsetAndFadeOut;
  }

  void FillPointLightData(WPerLightData& out_perLightData, const WPointLightRenderData* pPointLightRenderData)
  {
    FillLightData(out_perLightData, pPointLightRenderData, LIGHT_TYPE_POINT);

    out_perLightData.position = pPointLightRenderData->m_vGlobalPosition;
    out_perLightData.invSqrAttRadius = 1.0f / (pPointLightRenderData->m_fRange * pPointLightRenderData->m_fRange);

    // Tube axis direction (X axis of rotation)
    const WVec3 axisDir = pPointLightRenderData->m_qGlobalRotation * WVec3(1.0f, 0.0f, 0.0f);
    out_perLightData.direction = WShaderUtils::Float3ToRGB10(axisDir);

    // Pack length and half length as fp16
    out_perLightData.auxParams = WShaderUtils::Float2ToRG16F(WVec2(pPointLightRenderData->m_fLength, pPointLightRenderData->m_fLength * 0.5f));

    // Pack a perpendicular direction (Y axis) for orientation recovery on GPU
    const WVec3 rightDir = pPointLightRenderData->m_qGlobalRotation * WVec3(0.0f, 1.0f, 0.0f);
    out_perLightData.cookieParams0 = WFloat16(rightDir.z).GetRawData() << 16;
    out_perLightData.cookieParams1 = WShaderUtils::Float2ToRG16F(rightDir.GetAsVec2());
  }

  void FillSpotLightData(WPerLightData& out_perLightData, const WSpotLightRenderData* pSpotLightRenderData)
  {
    FillLightData(out_perLightData, pSpotLightRenderData, LIGHT_TYPE_SPOT);

    out_perLightData.direction = WShaderUtils::Float3ToRGB10(pSpotLightRenderData->m_qGlobalRotation * WVec3(-1, 0, 0));
    out_perLightData.position = pSpotLightRenderData->m_vGlobalPosition;
    out_perLightData.invSqrAttRadius = 1.0f / (pSpotLightRenderData->m_fRange * pSpotLightRenderData->m_fRange);

    const float fCosInner = WMath::Cos(pSpotLightRenderData->m_InnerSpotAngle * 0.5f);
    const float fCosOuter = WMath::Cos(pSpotLightRenderData->m_OuterSpotAngle * 0.5f);
    const float fSpotParamScale = 1.0f / WMath::Max(0.001f, (fCosInner - fCosOuter));
    const float fSpotParamOffset = -fCosOuter * fSpotParamScale;
    out_perLightData.auxParams = WShaderUtils::Float2ToRG16F(WVec2(fSpotParamScale, fSpotParamOffset));

    if (!pSpotLightRenderData->m_CookieId.IsInvalidated())
    {
      const float fScale = 1.0f / WMath::Max(0.001f, WMath::Tan(pSpotLightRenderData->m_OuterSpotAngle * 0.5f));
      const WVec3 cookieRightDir = pSpotLightRenderData->m_qGlobalRotation * WVec3(0, fScale, 0);

      // Set bit 15 as marker bit to indicate that we have a cookie.
      // The shader checks for (cookieParams0 & 0xFFFF) != 0 which would not work in case the cookie id is 0.
      out_perLightData.cookieParams0 = (pSpotLightRenderData->m_CookieId.m_InstanceIndex & 0x7FFF) | (1 << 15) | (WFloat16(cookieRightDir.z).GetRawData() << 16);
      out_perLightData.cookieParams1 = WShaderUtils::Float2ToRG16F(cookieRightDir.GetAsVec2());
    }
  }

  void FillDirLightData(WPerLightData& out_perLightData, const WDirectionalLightRenderData* pDirLightRenderData)
  {
    FillLightData(out_perLightData, pDirLightRenderData, LIGHT_TYPE_DIR);

    out_perLightData.direction = WShaderUtils::Float3ToRGB10(pDirLightRenderData->m_vDirection);
    out_perLightData.auxParams = pDirLightRenderData->m_bScreenSpaceShadows ? 1 : 0;
  }

  void FillFillLightData(WPerLightData& out_perLightData, const WFillLightRenderData* pFillLightRenderData)
  {
    WMemoryUtils::ZeroFill(&out_perLightData, 1);

    WColorLinearUB lightColor = pFillLightRenderData->m_LightColor;
    out_perLightData.intensity = pFillLightRenderData->m_fIntensity;

    switch (pFillLightRenderData->m_LightMode)
    {
      case WFillLightMode::Additive:
        lightColor.a = LIGHT_TYPE_FILL_ADDITIVE;
        break;
      case WFillLightMode::Subtractive:
        lightColor.a = LIGHT_TYPE_FILL_ADDITIVE;
        out_perLightData.intensity = -out_perLightData.intensity;
        break;
      case WFillLightMode::ModulateIndirect:
        lightColor.a = LIGHT_TYPE_FILL_MODULATE_INDIRECT;
        out_perLightData.intensity = WMath::Saturate(out_perLightData.intensity);
        break;
    }

    out_perLightData.colorAndType = *reinterpret_cast<WUInt32*>(&lightColor.r);
    out_perLightData.specularMultiplierAndRadius = 0; // no specular for fill lights

    out_perLightData.position = pFillLightRenderData->m_vGlobalPosition;
    out_perLightData.invSqrAttRadius = 1.0f / pFillLightRenderData->m_fRange;

    const float fFalloffExponent = WMath::Max(pFillLightRenderData->m_fFalloffExponent, 0.001f);
    out_perLightData.auxParams = WShaderUtils::Float2ToRG16F(WVec2(fFalloffExponent, pFillLightRenderData->m_fDirectionality));
  }

  void FillDecalData(WPerDecalData& out_perDecalData, const WDecalRenderData* pDecalRenderData)
  {
    const WVec4 rotationValues = pDecalRenderData->m_qGlobalRotation;
    const WQuat rotation(rotationValues.x, rotationValues.y, rotationValues.z, rotationValues.w);

    const WVec3 position = pDecalRenderData->m_vGlobalPosition;
    const WVec3 dirForwards = rotation * WVec3(1.0f, 0.0, 0.0f);
    const WVec3 dirUp = rotation * WVec3(0.0f, 0.0, 1.0f);
    WVec3 scale = pDecalRenderData->m_vGlobalScale;

    // the CompMax prevents division by zero (thus inf, thus NaN later, then crash)
    // if negative scaling should be allowed, this would need to be changed
    scale = WVec3(1.0f).CompDiv(scale.CompMax(WVec3(0.00001f)));

    const WMat4 lookAt = WGraphicsUtils::CreateLookAtViewMatrix(position, position + dirForwards, dirUp);
    WMat4 scaleMat = WMat4::MakeScaling(WVec3(scale.y, -scale.z, scale.x));

    out_perDecalData.worldToDecalMatrix = scaleMat * lookAt;
    out_perDecalData.applyOnlyToId = pDecalRenderData->m_uiApplyOnlyToId;
    out_perDecalData.decalFlags = pDecalRenderData->m_uiFlags;
    out_perDecalData.angleFadeParams = pDecalRenderData->m_uiAngleFadeParams;
    out_perDecalData.baseColor = *reinterpret_cast<const WUInt32*>(&pDecalRenderData->m_BaseColor.r);
    out_perDecalData.emissiveColorRG = WShaderUtils::PackFloat16intoUint(pDecalRenderData->m_EmissiveColor.r, pDecalRenderData->m_EmissiveColor.g);
    out_perDecalData.emissiveColorBA = WShaderUtils::PackFloat16intoUint(pDecalRenderData->m_EmissiveColor.b, pDecalRenderData->m_EmissiveColor.a);
    out_perDecalData.baseColorAtlasScale = pDecalRenderData->m_uiBaseColorAtlasScale;
    out_perDecalData.baseColorAtlasOffset = pDecalRenderData->m_uiBaseColorAtlasOffset;
    out_perDecalData.normalAtlasScale = pDecalRenderData->m_uiNormalAtlasScale;
    out_perDecalData.normalAtlasOffset = pDecalRenderData->m_uiNormalAtlasOffset;
    out_perDecalData.ormAtlasScale = pDecalRenderData->m_uiORMAtlasScale;
    out_perDecalData.ormAtlasOffset = pDecalRenderData->m_uiORMAtlasOffset;
  }

  void FillReflectionProbeData(WPerReflectionProbeData& out_perReflectionProbeData, const WReflectionProbeRenderData* pReflectionProbeRenderData)
  {
    WVec3 position = pReflectionProbeRenderData->m_GlobalTransform.m_vPosition;
    WVec3 scale = pReflectionProbeRenderData->m_GlobalTransform.m_vScale.CompMul(pReflectionProbeRenderData->m_vHalfExtents);

    // We store scale separately so we easily transform into probe projection space (with scale), influence space (scale + offset) and cube map space (no scale).
    auto trans = pReflectionProbeRenderData->m_GlobalTransform;
    trans.m_vScale = WVec3(1.0f, 1.0f, 1.0f);
    auto inverse = trans.GetAsMat4().GetInverse();

    // the CompMax prevents division by zero (thus inf, thus NaN later, then crash)
    // if negative scaling should be allowed, this would need to be changed
    scale = WVec3(1.0f).CompDiv(scale.CompMax(WVec3(0.00001f)));
    out_perReflectionProbeData.WorldToProbeProjectionMatrix = inverse;

    out_perReflectionProbeData.ProbePosition = pReflectionProbeRenderData->m_vGlobalPosition.GetAsVec4(1.0f); // W isn't used.
    out_perReflectionProbeData.Scale = scale.GetAsVec4(0.0f);                                                 // W isn't used.

    out_perReflectionProbeData.InfluenceScale = pReflectionProbeRenderData->m_vInfluenceScale.GetAsVec4(0.0f);
    out_perReflectionProbeData.InfluenceShift = pReflectionProbeRenderData->m_vInfluenceShift.CompMul(WVec3(1.0f) - pReflectionProbeRenderData->m_vInfluenceScale).GetAsVec4(0.0f);

    out_perReflectionProbeData.PositiveFalloff = pReflectionProbeRenderData->m_vPositiveFalloff.GetAsVec4(0.0f);
    out_perReflectionProbeData.NegativeFalloff = pReflectionProbeRenderData->m_vNegativeFalloff.GetAsVec4(0.0f);
    out_perReflectionProbeData.Index = pReflectionProbeRenderData->m_uiIndex;
  }


  W_FORCE_INLINE WSimdBBox GetScreenSpaceBounds(const WSimdBSphere& viewSpaceSphere, const WSimdMat4f& mProjectionMatrix)
  {
    WSimdVec4f viewSpaceCenter = viewSpaceSphere.GetCenter();
    WSimdFloat depth = viewSpaceCenter.z();
    WSimdFloat radius = viewSpaceSphere.GetRadius();

    WSimdVec4f mi;
    WSimdVec4f ma;

    if (viewSpaceCenter.GetLength<3>() > radius && depth > radius)
    {
      WSimdVec4f one = WSimdVec4f(1.0f);
      WSimdVec4f oneNegOne = WSimdVec4f(1.0f, -1.0f, 1.0f, -1.0f);

      WSimdVec4f pRadius = WSimdVec4f(radius / depth);
      WSimdVec4f pRadius2 = pRadius.CompMul(pRadius);

      WSimdVec4f xy = viewSpaceCenter / depth;
      WSimdVec4f xxyy = xy.Get<WSwizzle::XXYY>();
      WSimdVec4f nom = (pRadius2.CompMul(xxyy.CompMul(xxyy) - pRadius2 + one)).GetSqrt() - xxyy.CompMul(oneNegOne);
      WSimdVec4f denom = pRadius2 - one;

      WSimdVec4f projection = mProjectionMatrix.m_col0.GetCombined<WSwizzle::XXYY>(mProjectionMatrix.m_col1);
      WSimdVec4f minXmaxX_minYmaxY = nom.CompDiv(denom).CompMul(oneNegOne).CompMul(projection);

      mi = minXmaxX_minYmaxY.Get<WSwizzle::XZXX>();
      ma = minXmaxX_minYmaxY.Get<WSwizzle::YWYY>();
    }
    else
    {
      mi = WSimdVec4f(-1.0f);
      ma = WSimdVec4f(1.0f);
    }

    mi.SetZ(depth - radius);
    ma.SetZ(depth + radius);

    return WSimdBBox(mi, ma);
  }

  template <typename Cluster, typename IntersectionFunc>
  W_FORCE_INLINE void FillCluster(const WSimdBBox& screenSpaceBounds, WUInt32 uiBlockIndex, WUInt32 uiMask, Cluster* pClusters, IntersectionFunc func)
  {
    WSimdVec4f scale = WSimdVec4f(0.5f * NUM_CLUSTERS_X, -0.5f * NUM_CLUSTERS_Y, 1.0f, 1.0f);
    WSimdVec4f bias = WSimdVec4f(0.5f * NUM_CLUSTERS_X, 0.5f * NUM_CLUSTERS_Y, 0.0f, 0.0f);

    WSimdVec4f mi = WSimdVec4f::MulAdd(screenSpaceBounds.m_Min, scale, bias);
    WSimdVec4f ma = WSimdVec4f::MulAdd(screenSpaceBounds.m_Max, scale, bias);

    WSimdVec4i minXY_maxXY = WSimdVec4i::Truncate(mi.GetCombined<WSwizzle::XYXY>(ma));

    WSimdVec4i maxClusterIndex = WSimdVec4i(NUM_CLUSTERS_X, NUM_CLUSTERS_Y, NUM_CLUSTERS_X, NUM_CLUSTERS_Y);
    minXY_maxXY = minXY_maxXY.CompMin(maxClusterIndex - WSimdVec4i(1));
    minXY_maxXY = minXY_maxXY.CompMax(WSimdVec4i::MakeZero());

    WUInt32 xMin = minXY_maxXY.x();
    WUInt32 yMin = minXY_maxXY.w();

    WUInt32 xMax = minXY_maxXY.z();
    WUInt32 yMax = minXY_maxXY.y();

    WUInt32 zMin = GetSliceIndexFromDepth(screenSpaceBounds.m_Min.z());
    WUInt32 zMax = GetSliceIndexFromDepth(screenSpaceBounds.m_Max.z());

    for (WUInt32 z = zMin; z <= zMax; ++z)
    {
      for (WUInt32 y = yMin; y <= yMax; ++y)
      {
        for (WUInt32 x = xMin; x <= xMax; ++x)
        {
          WUInt32 uiClusterIndex = GetClusterIndexFromCoord(x, y, z);
          if (func(uiClusterIndex))
          {
            pClusters[uiClusterIndex].m_BitMask[uiBlockIndex] |= uiMask;
          }
        }
      }
    }
  }

  template <typename Cluster>
  void RasterizeSphere(const WSimdBSphere& pointLightSphere, WUInt32 uiLightIndex, const WSimdMat4f& mViewMatrix,
    const WSimdMat4f& mProjectionMatrix, Cluster* pClusters, WSimdBSphere* pClusterBoundingSpheres)
  {
    WSimdBSphere viewSpaceSphere(mViewMatrix.TransformPosition(pointLightSphere.GetCenter()), pointLightSphere.GetRadius());

    WSimdBBox screenSpaceBounds = GetScreenSpaceBounds(viewSpaceSphere, mProjectionMatrix);

    const WUInt32 uiBlockIndex = uiLightIndex / 32;
    const WUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters,
      [&](WUInt32 uiClusterIndex)
      { return viewSpaceSphere.Overlaps(pClusterBoundingSpheres[uiClusterIndex]); });
  }

  struct BoundingCone
  {
    WSimdBSphere m_BoundingSphere;
    WSimdVec4f m_PositionAndRange;
    WSimdVec4f m_ForwardDir;
    WSimdVec4f m_SinCosAngle;
  };

  template <typename Cluster>
  void RasterizeSpotLight(const BoundingCone& spotLightCone, WUInt32 uiLightIndex, const WSimdMat4f& mViewMatrix, const WSimdMat4f& mProjectionMatrix, Cluster* pClusters, const WSimdBSphere* pClusterBoundingSpheres)
  {
    WSimdVec4f position = mViewMatrix.TransformPosition(spotLightCone.m_PositionAndRange);
    WSimdFloat range = spotLightCone.m_PositionAndRange.w();
    WSimdVec4f forwardDir = mViewMatrix.TransformDirection(spotLightCone.m_ForwardDir);
    WSimdFloat sinAngle = spotLightCone.m_SinCosAngle.x();
    WSimdFloat cosAngle = spotLightCone.m_SinCosAngle.y();

    // First calculate a bounding sphere around the cone to get min and max bounds
    WSimdVec4f bSphereCenter;
    WSimdFloat bSphereRadius;
    if (sinAngle > 0.707107f) // sin(45)
    {
      bSphereCenter = position + forwardDir * cosAngle * range;
      bSphereRadius = sinAngle * range;
    }
    else
    {
      bSphereRadius = range / (cosAngle + cosAngle);
      bSphereCenter = position + forwardDir * bSphereRadius;
    }

    WSimdBSphere spotLightSphere(bSphereCenter, bSphereRadius);
    WSimdBBox screenSpaceBounds = GetScreenSpaceBounds(spotLightSphere, mProjectionMatrix);

    const WUInt32 uiBlockIndex = uiLightIndex / 32;
    const WUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters,
      [&](WUInt32 uiClusterIndex)
      {
        WSimdBSphere clusterSphere = pClusterBoundingSpheres[uiClusterIndex];
        WSimdFloat clusterRadius = clusterSphere.GetRadius();

        WSimdVec4f toConePos = clusterSphere.m_CenterAndRadius - position;
        WSimdFloat projected = forwardDir.Dot<3>(toConePos);
        WSimdFloat distToConeSq = toConePos.Dot<3>(toConePos);
        WSimdFloat distClosestP = cosAngle * (distToConeSq - projected * projected).GetSqrt() - projected * sinAngle;

        bool angleCull = distClosestP > clusterRadius;
        bool frontCull = projected > clusterRadius + range;
        bool backCull = projected < -clusterRadius;

        return !(angleCull || frontCull || backCull);
      });
  }

  template <typename Cluster>
  void RasterizeDirLight(const WDirectionalLightRenderData* pDirLightRenderData, WUInt32 uiLightIndex, WArrayPtr<Cluster> clusters)
  {
    const WUInt32 uiBlockIndex = uiLightIndex / 32;
    const WUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    for (WUInt32 i = 0; i < clusters.GetCount(); ++i)
    {
      clusters[i].m_BitMask[uiBlockIndex] |= uiMask;
    }
  }

  template <typename Cluster>
  void RasterizeBox(const WTransform& transform, WUInt32 uiDecalIndex, const WSimdMat4f& mView, const WSimdMat4f& mViewProjection, Cluster* pClusters, const WSimdBSphere* pClusterBoundingSpheres)
  {
    const WSimdVec4f decalHalfExtents = WSimdConversion::ToVec3(transform.m_vScale);
    WSimdBBox localDecalBounds = WSimdBBox(-decalHalfExtents, decalHalfExtents);

    WVec3 corners[8];
    WSimdConversion::ToBBox(localDecalBounds).GetCorners(corners);

    const WSimdTransform boxTransform = WSimdTransform::Make(WSimdConversion::ToVec3(transform.m_vPosition), WSimdConversion::ToQuat(transform.m_qRotation));
    const WSimdMat4f boxToWorld = boxTransform.GetAsMat4();
    const WSimdMat4f decalToScreen = mViewProjection * boxToWorld;

    WSimdBBox screenSpaceBounds = WSimdBBox::MakeInvalid();
    bool bInsideBox = false;
    for (WUInt32 i = 0; i < 8; ++i)
    {
      const WSimdVec4f corner = WSimdConversion::ToVec3(corners[i]);
      WSimdVec4f screenSpaceCorner = decalToScreen.TransformPosition(corner);
      const WSimdFloat depth = screenSpaceCorner.w();
      bInsideBox |= depth < WSimdFloat::MakeZero();

      screenSpaceCorner /= depth;
      screenSpaceCorner = screenSpaceCorner.GetCombined<WSwizzle::XYZW>(WSimdVec4f(depth));

      screenSpaceBounds.m_Min = screenSpaceBounds.m_Min.CompMin(screenSpaceCorner);
      screenSpaceBounds.m_Max = screenSpaceBounds.m_Max.CompMax(screenSpaceCorner);
    }

    if (bInsideBox)
    {
      screenSpaceBounds.m_Min = WSimdVec4f(-1.0f).GetCombined<WSwizzle::XYZW>(screenSpaceBounds.m_Min);
      screenSpaceBounds.m_Max = WSimdVec4f(1.0f).GetCombined<WSwizzle::XYZW>(screenSpaceBounds.m_Max);
    }

    const WUInt32 uiBlockIndex = uiDecalIndex / 32;
    const WUInt32 uiMask = 1 << (uiDecalIndex - uiBlockIndex * 32);

    const WSimdMat4f viewToBox = (mView * boxToWorld).GetInverse();

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters,
      [&](WUInt32 uiClusterIndex)
      {
        WSimdBSphere clusterSphere = pClusterBoundingSpheres[uiClusterIndex];
        clusterSphere.Transform(viewToBox);

        return localDecalBounds.Overlaps(clusterSphere);
      });
  }
} // namespace
