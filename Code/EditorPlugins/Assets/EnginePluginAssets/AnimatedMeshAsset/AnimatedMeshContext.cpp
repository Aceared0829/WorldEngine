#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshContext.h>
#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshView.h>

#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimatedMeshContext, 1, WRTTIDefaultAllocator<WAnimatedMeshContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Animated Mesh"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAnimatedMeshContext::WAnimatedMeshContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pAnimatedMeshObject = nullptr;
}

void WAnimatedMeshContext::HandleMessage(const WEditorEngineDocumentMsg* pDocMsg)
{
  if (auto* pMsg = WDynamicCast<const WEditorEngineSetMaterialsMsg*>(pDocMsg))
  {
    WAnimatedMeshComponent* pAnimatedMesh;
    if (m_pAnimatedMeshObject && m_pAnimatedMeshObject->TryGetComponentOfBaseType(pAnimatedMesh))
    {
      for (WUInt32 i = 0; i < pMsg->m_Materials.GetCount(); ++i)
      {
        WMaterialResourceHandle hMat;

        if (!pMsg->m_Materials[i].IsEmpty())
        {
          hMat = WResourceManager::LoadResource<WMaterialResource>(pMsg->m_Materials[i]);
        }

        pAnimatedMesh->SetMaterial(i, hMat);
      }
    }

    return;
  }

  if (auto* pMsg = WDynamicCast<const WQuerySelectionBBoxMsgToEngine*>(pDocMsg))
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
  }

  WEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void WAnimatedMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  W_LOCK(pWorld->GetWriteMarker());

  WAnimatedMeshComponent* pAnimatedMesh;

  // Preview AnimatedMesh
  {
    WGameObjectDesc obj;
    obj.m_bDynamic = true;
    obj.m_sName.Assign("AnimatedMeshPreview");
    pWorld->CreateObject(obj, m_pAnimatedMeshObject);

    const WTag& tagCastShadows = WTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pAnimatedMeshObject->SetTag(tagCastShadows);

    WAnimatedMeshComponent::CreateComponent(m_pAnimatedMeshObject, pAnimatedMesh);
    WStringBuilder sAnimatedMeshGuid;
    WConversionUtils::ToString(GetDocumentGuid(), sAnimatedMeshGuid);
    m_hAnimatedMesh = WResourceManager::LoadResource<WMeshResource>(sAnimatedMeshGuid);
    pAnimatedMesh->SetMesh(m_hAnimatedMesh);
  }
}

WEngineProcessViewContext* WAnimatedMeshContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WAnimatedMeshViewContext, this);
}

void WAnimatedMeshContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

bool WAnimatedMeshContext::UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext)
{
  WBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  WAnimatedMeshViewContext* pAnimatedMeshViewContext = static_cast<WAnimatedMeshViewContext*>(pThumbnailViewContext);
  return pAnimatedMeshViewContext->UpdateThumbnailCamera(bounds);
}


void WAnimatedMeshContext::QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg)
{
  if (m_pAnimatedMeshObject == nullptr)
    return;

  WBoundingBoxSphere bounds = WBoundingBoxSphere::MakeInvalid();

  {
    W_LOCK(m_pWorld->GetWriteMarker());

    m_pAnimatedMeshObject->UpdateLocalBounds();
    m_pAnimatedMeshObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pAnimatedMeshObject->GetGlobalBounds();

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
