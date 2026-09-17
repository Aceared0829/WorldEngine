#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <ProcGenPlugin/Tasks/PlacementTask.h>
#include <ProcGenPlugin/Tasks/Utils.h>
#include <RendererCore/Debug/DebugRenderer.h>

using namespace WProcGenInternal;

WCVarInt cvar_ProcGenVisTilePointIndex("ProcGen.VisTiles.PointIndex", -1, WCVarFlags::Default, "Visualize the raycasts for the given point index. Disabled if set to less than 0.");

static_assert(sizeof(PlacementPoint) == 32);
static_assert(sizeof(PlacementTransform) == 64);

PlacementTask::PlacementTask(PlacementData* pData, const char* szName)
  : m_pData(pData)
{
  ConfigureTask(szName, WTaskNesting::Maybe);

  m_VM.RegisterFunction(WExtendedExpressionFunctions::s_SampleCurveFunc);
  m_VM.RegisterFunction(WProcGenExpressionFunctions::s_ApplyVolumesFunc);
  m_VM.RegisterFunction(WProcGenExpressionFunctions::s_GetInstanceSeedFunc);
}

PlacementTask::~PlacementTask() = default;

void PlacementTask::Clear()
{
  m_InputPoints.Clear();
  m_OutputTransforms.Clear();
  m_Density.Clear();
  m_ValidPoints.Clear();
}

void PlacementTask::Execute()
{
  FindPlacementPoints();

  if (!m_InputPoints.IsEmpty())
  {
    ExecuteVM();
  }
}

bool IsRequestedSurface(WSurfaceResourceHandle hRequestedSurface, WSurfaceResourceHandle hHitSurface)
{
  if (hRequestedSurface.IsValid())
  {
    if (!hHitSurface.IsValid())
      return false;

    WResourceLock<WSurfaceResource> hitSurface(hHitSurface, WResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (hitSurface.GetAcquireResult() == WResourceAcquireResult::MissingFallback)
      return false;

    if (!hitSurface->IsBasedOn(hRequestedSurface))
      return false;
  }

  return true;
}

void PlacementTask::FindPlacementPoints()
{
  W_PROFILE_SCOPE("FindPlacementPoints");

  auto pOutput = m_pData->m_pOutput;

  WSimdVec4u seed = WSimdVec4u(m_pData->m_uiTileSeed) + WSimdVec4u(0, 3, 7, 11);

  float fZRange = m_pData->m_TileBoundingBox.GetExtents().z;
  WSimdFloat fZStart = m_pData->m_TileBoundingBox.m_vMax.z;
  WSimdVec4f vXY = WSimdConversion::ToVec3(m_pData->m_TileBoundingBox.m_vMin);
  WSimdVec4f vMinOffset = WSimdConversion::ToVec3(pOutput->m_vMinOffset);
  WSimdVec4f vMaxOffset = WSimdConversion::ToVec3(pOutput->m_vMaxOffset);

  // use center for fixed plane placement
  vXY.SetZ(m_pData->m_TileBoundingBox.GetCenter().z);

  WVec3 rayDir = WVec3(0, 0, -1);
  WUInt32 uiCollisionLayer = pOutput->m_uiCollisionLayer;

  WTempHybridArray<WDebugRendererLine, 16> debugLines;
  WColor hitColor = WColorScheme::LightUI(WColorScheme::Green);
  WColor missColor = WColorScheme::LightUI(WColorScheme::Red);

  auto AddDebugRay = [&](const WVec3& rayStart, const WVec3& rayDir, float fRayDistance, float fHitDistance, bool bHit)
  {
    const float fDistance = bHit ? fHitDistance : fRayDistance;
    const WColor c = bHit ? hitColor : missColor;
    debugLines.PushBack(WDebugRendererLine(rayStart, rayStart + rayDir * fDistance, c));
  };

  auto& patternPoints = pOutput->m_pPattern->m_Points;
  for (WUInt32 i = 0; i < patternPoints.GetCount(); ++i)
  {
    const bool bShouldVisualize = m_pData->m_bDebugVisualization && (cvar_ProcGenVisTilePointIndex == i);

    auto& patternPoint = patternPoints[i];
    WSimdVec4f patternCoords = WSimdVec4f(patternPoint.x, patternPoint.y, 0.0f);

    WPhysicsCastResult hitResult;

    if (m_pData->m_pPhysicsModule != nullptr &&
        (pOutput->m_Mode == WProcPlacementMode::Raycast ||
          pOutput->m_Mode == WProcPlacementMode::RaycastHighQuality))
    {
      WSimdVec4f rayStart = (vXY + patternCoords * pOutput->m_fFootprint);
      rayStart += WSimdRandom::FloatMinMax(WSimdVec4i(i), vMinOffset, vMaxOffset, seed);
      rayStart.SetZ(fZStart);

      WPhysicsQueryParameters queryParams(uiCollisionLayer, WPhysicsShapeType::Static);

      {
        const WVec3 vRayStart = WSimdConversion::ToVec3(rayStart);
        bool bHit = m_pData->m_pPhysicsModule->Raycast(hitResult, vRayStart, rayDir, fZRange, queryParams);
        if (bHit)
        {
          bHit = IsRequestedSurface(pOutput->m_hSurface, hitResult.m_hSurface);
        }

        if (bShouldVisualize)
        {
          AddDebugRay(vRayStart, rayDir, fZRange, hitResult.m_fDistance, bHit);
        }

        if (!bHit)
          continue;
      }

      if (pOutput->m_Mode == WProcPlacementMode::RaycastHighQuality)
      {
        const WUInt32 uiNumAdditionalRays = WMath::Max<WUInt32>(pOutput->m_uiNumAdditionalRays, 3);
        const WAngle angleStep = WAngle::MakeFromDegree(360.0f / uiNumAdditionalRays);
        const float fSpread = WMath::Max(pOutput->m_fRaySpread * pOutput->m_fFootprint, 0.01f);

        WTempHybridArray<WVec3, 32> hitPositions;

        bool bAllValid = true;
        for (WUInt32 i = 0; i < uiNumAdditionalRays; ++i)
        {
          const WAngle angle = angleStep * float(i);
          const WSimdVec4f offset = WSimdVec4f(WMath::Cos(angle), WMath::Sin(angle), 0.0f) * fSpread;
          const WSimdVec4f rayStartOffset = rayStart + offset;

          WPhysicsCastResult offsetHitResult;
          bool bHit = m_pData->m_pPhysicsModule->Raycast(offsetHitResult, WSimdConversion::ToVec3(rayStartOffset), rayDir, fZRange, queryParams);
          if (bHit)
          {
            bHit = IsRequestedSurface(pOutput->m_hSurface, offsetHitResult.m_hSurface);
          }

          if (bShouldVisualize)
          {
            AddDebugRay(WSimdConversion::ToVec3(rayStartOffset), rayDir, fZRange, offsetHitResult.m_fDistance, bHit);
          }

          if (!bHit)
          {
            bAllValid = false;
            break;
          }

          hitPositions.PushBack(offsetHitResult.m_vPosition);
        }

        if (!bAllValid)
          continue;

        const WSimdVec4f up = WSimdVec4f(0, 0, 1);
        WSimdVec4f avgNormal = WSimdVec4f::MakeZero();
        for (auto& pos : hitPositions)
        {
          // Do not normalize dirToP, so that points that are further away contribute more to the normal
          const WSimdVec4f dirToP = (WSimdConversion::ToVec3(hitResult.m_vPosition) - WSimdConversion::ToVec3(pos));
          const WSimdVec4f rightDir = up.CrossRH(dirToP);
          const WSimdVec4f normal = dirToP.CrossRH(rightDir);
          avgNormal += normal;
        }

        hitResult.m_vNormal = WSimdConversion::ToVec3(avgNormal.GetNormalized<3>());
      }
    }
    else if (pOutput->m_Mode == WProcPlacementMode::Fixed)
    {
      WSimdVec4f rayStart = (vXY + patternCoords * pOutput->m_fFootprint);
      rayStart += WSimdRandom::FloatMinMax(WSimdVec4i(i), vMinOffset, vMaxOffset, seed);

      hitResult.m_vPosition = WSimdConversion::ToVec3(rayStart);
      hitResult.m_fDistance = 0;
      hitResult.m_vNormal.Set(0, 0, 1);
    }

    bool bInBoundingBox = false;
    WSimdVec4f hitPosition = WSimdConversion::ToVec3(hitResult.m_vPosition);
    WSimdVec4f allOne = WSimdVec4f(1.0f);
    for (auto& globalToLocalBox : m_pData->m_GlobalToLocalBoxTransforms)
    {
      WSimdVec4f localHitPosition = globalToLocalBox.TransformPosition(hitPosition).Abs();
      if ((localHitPosition <= allOne).AllSet<3>())
      {
        bInBoundingBox = true;
        break;
      }
    }

    if (bInBoundingBox)
    {
      PlacementPoint& placementPoint = m_InputPoints.ExpandAndGetRef();
      placementPoint.m_vPosition = hitResult.m_vPosition;
      placementPoint.m_fScale = 1.0f;
      placementPoint.m_vNormal = hitResult.m_vNormal;
      placementPoint.m_uiColorIndex = 0;
      placementPoint.m_uiObjectIndex = 0;
      placementPoint.m_uiPointIndex = static_cast<WUInt16>(i);
    }
  }

  if (m_pData->m_bDebugVisualization && !debugLines.IsEmpty())
  {
    WDebugRenderer::AddPersistentLines(m_pData->m_pWorld, debugLines, WColor::White, WTransform::MakeIdentity(), WTime::MakeFromSeconds(20.0f));

    if (cvar_ProcGenVisTilePointIndex >= 0)
    {
      for (auto& inputPoint : m_InputPoints)
      {
        WLog::Info("Placement Point #{}: Pos: {}, Normal: {}", inputPoint.m_uiPointIndex, inputPoint.m_vPosition, inputPoint.m_vNormal);
      }
    }
  }
}

void PlacementTask::ExecuteVM()
{
  auto pOutput = m_pData->m_pOutput;

  // Execute bytecode
  if (pOutput->m_pByteCode != nullptr)
  {
    W_PROFILE_SCOPE("ExecuteVM");

    WUInt32 uiNumInstances = m_InputPoints.GetCount();
    m_Density.SetCountUninitialized(uiNumInstances);

    WTempHybridArray<WProcessingStream, 8> inputs;
    {
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPositionX, offsetof(PlacementPoint, m_vPosition.x)));
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPositionY, offsetof(PlacementPoint, m_vPosition.y)));
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPositionZ, offsetof(PlacementPoint, m_vPosition.z)));

      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sNormalX, offsetof(PlacementPoint, m_vNormal.x)));
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sNormalY, offsetof(PlacementPoint, m_vNormal.y)));
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sNormalZ, offsetof(PlacementPoint, m_vNormal.z)));

      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPointIndex, offsetof(PlacementPoint, m_uiPointIndex), WProcessingStream::DataType::Short));
    }

    WTempHybridArray<WProcessingStream, 8> outputs;
    {
      outputs.PushBack(WProcessingStream(ExpressionOutputs::s_sOutDensity, m_Density.GetByteArrayPtr(), WProcessingStream::DataType::Float));
      outputs.PushBack(MakeOutputStream(ExpressionOutputs::s_sOutScale, offsetof(PlacementPoint, m_fScale)));
      outputs.PushBack(MakeOutputStream(ExpressionOutputs::s_sOutColorIndex, offsetof(PlacementPoint, m_uiColorIndex), WProcessingStream::DataType::Byte));
      outputs.PushBack(MakeOutputStream(ExpressionOutputs::s_sOutObjectIndex, offsetof(PlacementPoint, m_uiObjectIndex), WProcessingStream::DataType::Byte));
    }

    // Execute expression bytecode
    if (m_VM.Execute(*(pOutput->m_pByteCode), inputs, outputs, uiNumInstances, m_pData->m_GlobalData, WExpressionVM::Flags::BestPerformance).Failed())
    {
      return;
    }

    // Test density against point threshold and fill remaining input point data from expression
    const Pattern* pPattern = pOutput->m_pPattern;
    for (WUInt32 i = 0; i < uiNumInstances; ++i)
    {
      auto& inputPoint = m_InputPoints[i];
      const WUInt32 uiPointIndex = inputPoint.m_uiPointIndex;
      const float fThreshold = pPattern->m_Points[uiPointIndex].threshold;

      if (m_Density[i] >= fThreshold && pOutput->m_ObjectsToPlace[inputPoint.m_uiObjectIndex].IsValid())
      {
        m_ValidPoints.PushBack(i);
      }
    }
  }

  if (m_ValidPoints.IsEmpty())
  {
    return;
  }

  W_PROFILE_SCOPE("Construct final transforms");

  m_OutputTransforms.SetCountUninitialized(m_ValidPoints.GetCount());

  WSimdVec4u seed = WSimdVec4u(m_pData->m_uiTileSeed) + WSimdVec4u(13, 17, 31, 79);

  float fMinAngle = 0.0f;
  float fMaxAngle = WMath::Pi<float>() * 2.0f;

  WSimdVec4f vMinValue = WSimdVec4f(fMinAngle, pOutput->m_vMinOffset.z, 0.0f);
  WSimdVec4f vMaxValue = WSimdVec4f(fMaxAngle, pOutput->m_vMaxOffset.z, 0.0f);
  WSimdVec4f vYawRotationSnap = WSimdVec4f(pOutput->m_YawRotationSnap);
  WSimdVec4f vUp = WSimdVec4f(0, 0, 1);
  WSimdVec4f vHalf = WSimdVec4f(0.5f);
  WSimdVec4f vAlignToNormal = WSimdVec4f(pOutput->m_fAlignToNormal);
  WSimdVec4f vMinScale = WSimdConversion::ToVec3(pOutput->m_vMinScale);
  WSimdVec4f vMaxScale = WSimdConversion::ToVec3(pOutput->m_vMaxScale);

  const WColorGradient* pColorGradient = nullptr;
  if (pOutput->m_hColorGradient.IsValid())
  {
    WResourceLock<WColorGradientResource> pColorGradientResource(pOutput->m_hColorGradient, WResourceAcquireMode::BlockTillLoaded);
    pColorGradient = &(pColorGradientResource->GetDescriptor().m_Gradient);
  }

  for (WUInt32 i = 0; i < m_ValidPoints.GetCount(); ++i)
  {
    WUInt32 uiInputPointIndex = m_ValidPoints[i];
    auto& placementPoint = m_InputPoints[uiInputPointIndex];
    auto& placementTransform = m_OutputTransforms[i];

    WSimdVec4f random = WSimdRandom::FloatMinMax(WSimdVec4i(placementPoint.m_uiPointIndex), vMinValue, vMaxValue, seed);

    WSimdVec4f offset = WSimdVec4f::MakeZero();
    offset.SetZ(random.y());
    placementTransform.m_Transform.m_Position = WSimdConversion::ToVec3(placementPoint.m_vPosition) + offset;

    WSimdVec4f yaw = WSimdVec4f(random.x());
    WSimdVec4f roundedYaw = (yaw.CompDiv(vYawRotationSnap) + vHalf).Floor().CompMul(vYawRotationSnap);
    yaw = WSimdVec4f::Select(vYawRotationSnap == WSimdVec4f::MakeZero(), yaw, roundedYaw);

    WSimdQuat qYawRot = WSimdQuat::MakeFromAxisAndAngle(vUp, yaw.x());
    WSimdVec4f vNormal = WSimdConversion::ToVec3(placementPoint.m_vNormal);
    WSimdQuat qToNormalRot = WSimdQuat::MakeShortestRotation(vUp, WSimdVec4f::Lerp(vUp, vNormal, vAlignToNormal));
    placementTransform.m_Transform.m_Rotation = qToNormalRot * qYawRot;

    WSimdVec4f scale = WSimdVec4f(WMath::Clamp(placementPoint.m_fScale, 0.0f, 1.0f));
    placementTransform.m_Transform.m_Scale = WSimdVec4f::Lerp(vMinScale, vMaxScale, scale);

    placementTransform.m_ObjectColor = WColor::MakeZero();
    placementTransform.m_uiPointIndex = placementPoint.m_uiPointIndex;
    placementTransform.m_uiObjectIndex = placementPoint.m_uiObjectIndex;
    placementTransform.m_bHasValidColor = false;

    if (pColorGradient != nullptr)
    {
      float colorIndex = WMath::ColorByteToFloat(placementPoint.m_uiColorIndex);

      WColor objectColor;
      WUInt8 alpha;
      float intensity = 1.0f;
      pColorGradient->EvaluateColor(colorIndex, objectColor);
      pColorGradient->EvaluateIntensity(colorIndex, intensity);
      pColorGradient->EvaluateAlpha(colorIndex, alpha);
      objectColor.r *= intensity;
      objectColor.g *= intensity;
      objectColor.b *= intensity;
      objectColor.a = WMath::ColorByteToFloat(alpha);

      placementTransform.m_ObjectColor = objectColor;
      placementTransform.m_bHasValidColor = true;
    }
  }
}


W_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Tasks_Implementation_PlacementTask);
