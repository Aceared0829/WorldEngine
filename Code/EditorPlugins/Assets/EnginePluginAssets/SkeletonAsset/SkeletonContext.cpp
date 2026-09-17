#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonView.h>

#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/SkeletonComponent.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSkeletonContext, 1, WRTTIDefaultAllocator<WSkeletonContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Skeleton"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSkeletonContext::WSkeletonContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
}

void WSkeletonContext::HandleMessage(const WEditorEngineDocumentMsg* pDocMsg)
{
  if (auto pMsg = WDynamicCast<const WQuerySelectionBBoxMsgToEngine*>(pDocMsg))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = WDynamicCast<const WSimpleDocumentConfigMsgToEngine*>(pDocMsg))
  {
    if (pMsg->m_sWhatToDo == "CommonAssetUiState")
    {
      if (pMsg->m_sPayload == "Grid")
      {
        m_bDisplayGrid = pMsg->m_PayloadValue.ConvertTo<float>() > 0;
        return;
      }
    }
    else if (pMsg->m_sWhatToDo == "HighlightBones")
    {
      W_LOCK(m_pWorld->GetWriteMarker());

      WSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->SetBonesToHighlight(pMsg->m_sPayload);
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderBones")
    {
      W_LOCK(m_pWorld->GetWriteMarker());

      WSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeBones = pMsg->m_PayloadValue.Get<bool>();
      }

      // resend the pose every frame (this config message is send every frame)
      // this ensures that changing any of the visualization states in the skeleton component displays correctly
      // a bit hacky and should be cleaned up, but this way the skeleton component doesn't need to keep a copy of the last pose (maybe it should)
      WSkeletonPoseComponent* pPoseSkeleton;
      if (m_pWorld->TryGetComponent(m_hPoseComponent, pPoseSkeleton))
      {
        pPoseSkeleton->ResendPose();
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderColliders")
    {
      W_LOCK(m_pWorld->GetWriteMarker());

      WSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeColliders = pMsg->m_PayloadValue.Get<bool>();
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderJoints")
    {
      W_LOCK(m_pWorld->GetWriteMarker());

      WSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeJoints = pMsg->m_PayloadValue.Get<bool>();
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderSwingLimits")
    {
      W_LOCK(m_pWorld->GetWriteMarker());

      WSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeSwingLimits = pMsg->m_PayloadValue.Get<bool>();
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderTwistLimits")
    {
      W_LOCK(m_pWorld->GetWriteMarker());

      WSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeTwistLimits = pMsg->m_PayloadValue.Get<bool>();
      }
    }
    else if (pMsg->m_sWhatToDo == "PreviewMesh" && m_sAnimatedMeshToUse != pMsg->m_sPayload)
    {
      m_sAnimatedMeshToUse = pMsg->m_sPayload;

      auto pWorld = m_pWorld;
      W_LOCK(pWorld->GetWriteMarker());

      WAnimatedMeshComponent* pAnimMesh;
      if (pWorld->TryGetComponent(m_hAnimMeshComponent, pAnimMesh))
      {
        m_hAnimMeshComponent.Invalidate();
        pAnimMesh->DeleteComponent();
      }

      if (!m_sAnimatedMeshToUse.IsEmpty())
      {
        WMaterialResourceHandle hMat = WResourceManager::LoadResource<WMaterialResource>("Editor/Materials/SkeletonPreviewMesh.WMaterial");

        m_hAnimMeshComponent = WAnimatedMeshComponent::CreateComponent(m_pGameObject, pAnimMesh);
        pAnimMesh->SetMeshFile(m_sAnimatedMeshToUse);

        for (int i = 0; i < 10; ++i)
        {
          pAnimMesh->SetMaterial(i, hMat);
        }
      }
    }
  }

  WEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void WSkeletonContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  W_LOCK(pWorld->GetWriteMarker());

  WGameObjectDesc obj;
  WSkeletonComponent* pVisSkeleton;
  WSkeletonPoseComponent* pPoseSkeleton;

  // Preview Mesh
  {
    obj.m_sName.Assign("SkeletonPreview");
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, m_pGameObject);

    m_hSkeletonComponent = WSkeletonComponent::CreateComponent(m_pGameObject, pVisSkeleton);
    WStringBuilder sSkeletonGuid;
    WConversionUtils::ToString(GetDocumentGuid(), sSkeletonGuid);
    m_hSkeleton = WResourceManager::LoadResource<WSkeletonResource>(sSkeletonGuid);
    pVisSkeleton->SetSkeleton(m_hSkeleton);
    pVisSkeleton->m_bVisualizeColliders = true;

    m_hPoseComponent = WSkeletonPoseComponent::CreateComponent(m_pGameObject, pPoseSkeleton);
    pPoseSkeleton->SetSkeleton(m_hSkeleton);
    pPoseSkeleton->SetPoseMode(WSkeletonPoseMode::RestPose);
  }
}

WEngineProcessViewContext* WSkeletonContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WSkeletonViewContext, this);
}

void WSkeletonContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

bool WSkeletonContext::UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext)
{
  WBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  WSkeletonViewContext* pMeshViewContext = static_cast<WSkeletonViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void WSkeletonContext::QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg)
{
  if (m_pGameObject == nullptr)
    return;

  WBoundingBoxSphere bounds = WBoundingBoxSphere::MakeInvalid();

  {
    W_LOCK(m_pWorld->GetWriteMarker());

    m_pGameObject->UpdateLocalBounds();
    m_pGameObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pGameObject->GetGlobalBounds();

    if (b.IsValid())
      bounds.ExpandToInclude(b);
  }

  const WQuerySelectionBBoxMsgToEngine* msg = static_cast<const WQuerySelectionBBoxMsgToEngine*>(pMsg);

  WQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID = msg->m_uiViewID;
  res.m_iPurpose = msg->m_iPurpose;
  res.m_vCenter = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtents;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}
