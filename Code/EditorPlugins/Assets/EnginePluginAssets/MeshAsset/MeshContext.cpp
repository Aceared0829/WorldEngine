#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>

#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshContext, 1, WRTTIDefaultAllocator<WMeshContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Mesh"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WMeshContext::WMeshContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMeshObject = nullptr;
}

void WMeshContext::HandleMessage(const WEditorEngineDocumentMsg* pDocMsg)
{
  if (auto* pMsg = WDynamicCast<const WEditorEngineSetMaterialsMsg*>(pDocMsg))
  {
    m_SlotNames = pMsg->m_SlotNames;

    WMeshComponent* pMesh;
    if (m_pMeshObject && m_pMeshObject->TryGetComponentOfBaseType(pMesh))
    {
      for (WUInt32 i = 0; i < pMsg->m_Materials.GetCount(); ++i)
      {
        WMaterialResourceHandle hMat;

        if (!pMsg->m_Materials[i].IsEmpty())
        {
          hMat = WResourceManager::LoadResource<WMaterialResource>(pMsg->m_Materials[i]);
        }

        pMesh->SetMaterial(i, hMat);
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

void WMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  W_LOCK(pWorld->GetWriteMarker());

  WGameObjectDesc obj;
  WMeshComponent* pMesh;

  // Preview Mesh
  {
    obj.m_sName.Assign("MeshPreview");
    pWorld->CreateObject(obj, m_pMeshObject);

    const WTag& tagCastShadows = WTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMeshObject->SetTag(tagCastShadows);

    WMeshComponent::CreateComponent(m_pMeshObject, pMesh);
    WStringBuilder sMeshGuid;
    WConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMesh = WResourceManager::LoadResource<WMeshResource>(sMeshGuid);
    pMesh->SetMesh(m_hMesh);

    {
      WResourceLock<WMeshResource> pMeshRes(m_hMesh, WResourceAcquireMode::PointerOnly);
      pMeshRes->m_ResourceEvents.AddEventHandler(WMakeDelegate(&WMeshContext::OnResourceEvent, this), m_MeshResourceEventSubscriber);
    }
  }
}

WEngineProcessViewContext* WMeshContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WMeshViewContext, this);
}

void WMeshContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

bool WMeshContext::UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext)
{
  if (m_bBoundsDirty)
  {
    W_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
    m_bBoundsDirty = false;
  }
  WBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  WMeshViewContext* pMeshViewContext = static_cast<WMeshViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void WMeshContext::QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg)
{
  if (m_pMeshObject == nullptr)
    return;

  WBoundingBoxSphere bounds = WBoundingBoxSphere::MakeInvalid();

  {
    W_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
    m_bBoundsDirty = false;
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

void WMeshContext::OnResourceEvent(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUpdated)
  {
    m_bBoundsDirty = true;
  }
}
