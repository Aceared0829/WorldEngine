#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshContext.h>
#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshView.h>

#include <JoltPlugin/Components/JoltVisColMeshComponent.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WJoltCollisionMeshContext, 1, WRTTIDefaultAllocator<WJoltCollisionMeshContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Jolt_Colmesh_Triangle;Jolt_Colmesh_Convex"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltCollisionMeshContext::WJoltCollisionMeshContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMeshObject = nullptr;
}

void WJoltCollisionMeshContext::HandleMessage(const WEditorEngineDocumentMsg* pDocMsg)
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
  }

  WEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void WJoltCollisionMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  W_LOCK(pWorld->GetWriteMarker());

  WGameObjectDesc obj;
  WJoltVisColMeshComponent* pMesh = nullptr;

  // Preview Mesh
  {
    obj.m_sName.Assign("MeshPreview");
    pWorld->CreateObject(obj, m_pMeshObject);

    const WTag& tagCastShadows = WTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMeshObject->SetTag(tagCastShadows);

    WJoltVisColMeshComponent::CreateComponent(m_pMeshObject, pMesh);
    WStringBuilder sMeshGuid;
    WConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMesh = WResourceManager::LoadResource<WJoltMeshResource>(sMeshGuid);
    pMesh->SetMesh(m_hMesh);
  }
}

WEngineProcessViewContext* WJoltCollisionMeshContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WJoltCollisionMeshViewContext, this);
}

void WJoltCollisionMeshContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

bool WJoltCollisionMeshContext::UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext)
{
  WBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  WJoltCollisionMeshViewContext* pMeshViewContext = static_cast<WJoltCollisionMeshViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void WJoltCollisionMeshContext::QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg)
{
  if (m_pMeshObject == nullptr)
    return;

  WBoundingBoxSphere bounds = WBoundingBoxSphere::MakeInvalid();

  {
    W_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pMeshObject->GetGlobalBounds();

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
