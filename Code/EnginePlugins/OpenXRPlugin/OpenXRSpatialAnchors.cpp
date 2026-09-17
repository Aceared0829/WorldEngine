#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Core/World/World.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <OpenXRPlugin/OpenXRSpatialAnchors.h>
#include <OpenXRPlugin/Utils/OpenXRConversionUtils.h>

W_IMPLEMENT_SINGLETON(WOpenXRSpatialAnchors);

WOpenXRSpatialAnchors::WOpenXRSpatialAnchors(WOpenXR* pOpenXR)
  : m_SingletonRegistrar(this)
  , m_pOpenXR(pOpenXR)
{
  W_ASSERT_DEV(m_pOpenXR->m_Extensions.m_bSpatialAnchor, "Spatial anchors not supported");
}

WOpenXRSpatialAnchors::~WOpenXRSpatialAnchors()
{
  for (auto it = m_Anchors.GetIterator(); it.IsValid(); ++it)
  {
    AnchorData anchorData;
    if (m_Anchors.TryGetValue(it.Id(), anchorData))
    {
      XR_LOG_ERROR(m_pOpenXR->m_Extensions.pfn_xrDestroySpatialAnchorMSFT(anchorData.m_Anchor));
      XR_LOG_ERROR(xrDestroySpace(anchorData.m_Space));
    }
  }
  m_Anchors.Clear();
}

WXRSpatialAnchorID WOpenXRSpatialAnchors::CreateAnchor(const WTransform& globalTransform)
{
  WWorld* pWorld = m_pOpenXR->GetWorld();
  if (pWorld == nullptr)
    return WXRSpatialAnchorID();

  WTransform globalStageTransform;
  globalStageTransform.SetIdentity();
  if (const WStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<WStageSpaceComponentManager>())
  {
    if (const WStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
    {
      globalStageTransform = pStage->GetOwner()->GetGlobalTransform();
    }
  }
  WTransform local = WTransform::MakeLocalTransform(globalStageTransform, globalTransform);

  XrSpatialAnchorCreateInfoMSFT createInfo{XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_MSFT};
  createInfo.space = m_pOpenXR->GetBaseSpace();
  createInfo.pose.position = WOpenXRConversionUtils::ConvertPosition(local.m_vPosition);
  createInfo.pose.orientation = WOpenXRConversionUtils::ConvertOrientation(local.m_qRotation);
  createInfo.time = m_pOpenXR->m_FrameState.predictedDisplayTime;

  XrSpatialAnchorMSFT anchor;
  XrResult res = m_pOpenXR->m_Extensions.pfn_xrCreateSpatialAnchorMSFT(m_pOpenXR->m_pSession, &createInfo, &anchor);
  if (res != XrResult::XR_SUCCESS)
    return WXRSpatialAnchorID();

  XrSpatialAnchorSpaceCreateInfoMSFT createSpaceInfo{XR_TYPE_SPATIAL_ANCHOR_SPACE_CREATE_INFO_MSFT};
  createSpaceInfo.anchor = anchor;
  createSpaceInfo.poseInAnchorSpace = WOpenXRConversionUtils::ConvertTransform(WTransform::MakeIdentity());

  XrSpace space;
  res = m_pOpenXR->m_Extensions.pfn_xrCreateSpatialAnchorSpaceMSFT(m_pOpenXR->m_pSession, &createSpaceInfo, &space);

  return m_Anchors.Insert({anchor, space});
}

WResult WOpenXRSpatialAnchors::DestroyAnchor(WXRSpatialAnchorID id)
{
  AnchorData anchorData;
  if (!m_Anchors.TryGetValue(id, anchorData))
    return W_FAILURE;

  XR_LOG_ERROR(m_pOpenXR->m_Extensions.pfn_xrDestroySpatialAnchorMSFT(anchorData.m_Anchor));
  XR_LOG_ERROR(xrDestroySpace(anchorData.m_Space));
  m_Anchors.Remove(id);

  return W_SUCCESS;
}

WResult WOpenXRSpatialAnchors::TryGetAnchorTransform(WXRSpatialAnchorID id, WTransform& out_globalTransform)
{
  WWorld* pWorld = m_pOpenXR->GetWorld();
  if (!pWorld)
    return W_FAILURE;

  AnchorData anchorData;
  if (!m_Anchors.TryGetValue(id, anchorData))
    return W_FAILURE;

  const XrTime time = m_pOpenXR->m_FrameState.predictedDisplayTime;
  XrSpaceLocation viewInScene = {XR_TYPE_SPACE_LOCATION};
  XrResult res = xrLocateSpace(anchorData.m_Space, m_pOpenXR->m_pSceneSpace, time, &viewInScene);
  if (res != XrResult::XR_SUCCESS)
    return W_FAILURE;

  if ((viewInScene.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) ==
      (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
  {
    WTransform globalStageTransform;
    globalStageTransform.SetIdentity();
    if (const WStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<WStageSpaceComponentManager>())
    {
      if (const WStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
      {
        globalStageTransform = pStage->GetOwner()->GetGlobalTransform();
      }
    }
    WTransform local(WOpenXRConversionUtils::ConvertPosition(viewInScene.pose.position), WOpenXRConversionUtils::ConvertOrientation(viewInScene.pose.orientation));
    out_globalTransform = WTransform::MakeGlobalTransform(globalStageTransform, local);

    return W_SUCCESS;
  }
  return W_FAILURE;
}
