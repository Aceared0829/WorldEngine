#include <EnginePluginKraut/EnginePluginKrautPCH.h>

#include <EnginePluginKraut/KrautTreeAsset/KrautTreeContext.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeView.h>

#include <GameEngine/Effects/Wind/SimpleWindComponent.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Components/SkyBoxComponent.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WKrautTreeContext, 1, WRTTIDefaultAllocator<WKrautTreeContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Kraut Tree"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WKrautTreeContext::WKrautTreeContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMainObject = nullptr;
}

void WKrautTreeContext::HandleMessage(const WEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = WDynamicCast<const WQuerySelectionBBoxMsgToEngine*>(pMsg0))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = WDynamicCast<const WSimpleDocumentConfigMsgToEngine*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "UpdateTree" && !m_hKrautComponent.IsInvalidated())
    {
      W_LOCK(m_pWorld->GetWriteMarker());

      WKrautTreeComponent* pTree = nullptr;
      if (!m_pWorld->TryGetComponent(m_hKrautComponent, pTree))
        return;

      if (pMsg->m_sPayload == "DisplayRandomSeed")
      {
        m_uiDisplayRandomSeed = pMsg->m_PayloadValue.ConvertTo<WUInt32>();

        pTree->SetCustomRandomSeed(m_uiDisplayRandomSeed);
      }
    }

    if (pMsg->m_sWhatToDo == "SetWindStrength" && !m_hWindComponent.IsInvalidated())
    {
      W_LOCK(m_pWorld->GetWriteMarker());

      WSimpleWindComponent* pWind = nullptr;
      if (!m_pWorld->TryGetComponent(m_hWindComponent, pWind))
        return;

      const WInt32 windStrength = pMsg->m_PayloadValue.ConvertTo<WInt32>();

      switch (windStrength)
      {
        case 0: // Off
          pWind->m_MinWindStrength = WWindStrength::None;
          pWind->m_MaxWindStrength = WWindStrength::None;
          break;

        case 1: // Light
          pWind->m_MinWindStrength = WWindStrength::Calm;
          pWind->m_MaxWindStrength = WWindStrength::GentleBreeze;
          break;

        case 2: // Moderate
          pWind->m_MinWindStrength = WWindStrength::GentleBreeze;
          pWind->m_MaxWindStrength = WWindStrength::StrongBreeze;
          break;

        case 3: // Strong
          pWind->m_MinWindStrength = WWindStrength::Storm;
          pWind->m_MaxWindStrength = WWindStrength::Storm;
          break;
      }
    }

    if (pMsg->m_sWhatToDo == "SetLodOverride" && !m_hKrautComponent.IsInvalidated())
    {
      W_LOCK(m_pWorld->GetWriteMarker());

      WKrautTreeComponent* pTree = nullptr;
      if (!m_pWorld->TryGetComponent(m_hKrautComponent, pTree))
        return;

      const WInt8 iLodOverride = static_cast<WInt8>(pMsg->m_PayloadValue.ConvertTo<WInt32>());
      pTree->m_iLodOverride = iLodOverride;
    }

    if (pMsg->m_sWhatToDo == "SetShowFrondsLeaves" && !m_hKrautComponent.IsInvalidated())
    {
      W_LOCK(m_pWorld->GetWriteMarker());

      WKrautTreeComponent* pTree = nullptr;
      if (!m_pWorld->TryGetComponent(m_hKrautComponent, pTree))
        return;

      pTree->m_bHideFrondsAndLeafs = !pMsg->m_PayloadValue.ConvertTo<bool>();
    }

    return;
  }

  if (auto pMsg = WDynamicCast<const WViewRedrawMsgToEngine*>(pMsg0))
  {
    SendLodStats(pMsg->m_DocumentGuid);
  }

  WEngineProcessDocumentContext::HandleMessage(pMsg0);
}

void WKrautTreeContext::SendLodStats(const WUuid& documentGuid)
{
  if (m_hKrautComponent.IsInvalidated())
    return;

  W_LOCK(m_pWorld->GetWriteMarker());

  WKrautTreeComponent* pTree = nullptr;
  if (!m_pWorld->TryGetComponent(m_hKrautComponent, pTree))
    return;

  if (!pTree->GetKrautTreeResource().IsValid())
    return;

  WResourceLock<WKrautTreeResource> pResource(pTree->GetKrautTreeResource(), WResourceAcquireMode::AllowLoadingFallback);
  if (pResource.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  const auto treeLods = pResource->GetTreeLODs();
  const WInt8 iLodIdx = pTree->m_iLodOverride;

  if (iLodIdx < 0 || iLodIdx >= static_cast<WInt8>(treeLods.GetCount()))
    return;

  const auto& lod = treeLods[iLodIdx];

  LodStats stats;
  stats.m_iLodIndex = iLodIdx;
  stats.m_uiNumBones = lod.m_uiNumBones;
  stats.m_uiNumTrianglesTotal = lod.m_uiNumTrianglesBranch + lod.m_uiNumTrianglesFrond + lod.m_uiNumTrianglesLeaf;
  stats.m_uiNumTrianglesBranch = lod.m_uiNumTrianglesBranch;
  stats.m_uiNumTrianglesFrond = lod.m_uiNumTrianglesFrond;
  stats.m_uiNumTrianglesLeaf = lod.m_uiNumTrianglesLeaf;

  if (stats.m_iLodIndex == m_LastSentLodStats.m_iLodIndex &&
      stats.m_uiNumBones == m_LastSentLodStats.m_uiNumBones &&
      stats.m_uiNumTrianglesTotal == m_LastSentLodStats.m_uiNumTrianglesTotal &&
      stats.m_uiNumTrianglesBranch == m_LastSentLodStats.m_uiNumTrianglesBranch &&
      stats.m_uiNumTrianglesFrond == m_LastSentLodStats.m_uiNumTrianglesFrond &&
      stats.m_uiNumTrianglesLeaf == m_LastSentLodStats.m_uiNumTrianglesLeaf)
    return;

  m_LastSentLodStats = stats;

  WStringBuilder sPayload;
  sPayload.SetFormat("{};{};{};{};{}",
    stats.m_uiNumBones,
    stats.m_uiNumTrianglesTotal,
    stats.m_uiNumTrianglesBranch,
    stats.m_uiNumTrianglesFrond,
    stats.m_uiNumTrianglesLeaf);

  WSimpleDocumentConfigMsgToEditor msg;
  msg.m_DocumentGuid = documentGuid;
  msg.m_sWhatToDo = "LODStats";
  msg.m_sPayload = sPayload;
  SendProcessMessage(&msg);
}

void WKrautTreeContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  W_LOCK(pWorld->GetWriteMarker());


  WKrautTreeComponent* pTree = nullptr;

  // Preview Mesh
  {
    WGameObjectDesc obj;
    obj.m_sName.Assign("KrautTreePreview");
    // TODO: making the object dynamic is a workaround!
    // without it, shadows keep disappearing when switching between tree documents
    // triggering resource reload will also trigger WKrautTreeComponent::OnMsgExtractRenderData,
    // which fixes the shadows for a while, but not caching the render-data (WRenderData::Caching::IfStatic)
    // 'solves' the issue in the preview
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, m_pMainObject);

    const WTag& tagCastShadows = WTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMainObject->SetTag(tagCastShadows);

    m_hKrautComponent = WKrautTreeComponent::CreateComponent(m_pMainObject, pTree);
    WStringBuilder sMeshGuid;
    WConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMainResource = WResourceManager::LoadResource<WKrautGeneratorResource>(sMeshGuid);
    pTree->SetVariationIndex(0xFFFF); // takes the 'display seed'
    pTree->SetKrautGeneratorResource(m_hMainResource);
    pTree->m_bForceGenerateImmediate = true;
  }


  // Wind
  {
    WGameObjectDesc obj;
    obj.m_sName.Assign("Wind");

    WGameObject* pObj;
    pWorld->CreateObject(obj, pObj);

    WSimpleWindComponent* pWind = nullptr;
    m_hWindComponent = WSimpleWindComponent::CreateComponent(pObj, pWind);

    pWind->m_Deviation = WAngle::MakeFromDegree(180);
    pWind->m_MinWindStrength = WWindStrength::LightBreeze;
    pWind->m_MaxWindStrength = WWindStrength::GentleBreeze;
  }

  // ground
  {
    const char* szMeshName = "KrautPreviewGroundMesh";
    m_hPreviewMeshResource = WResourceManager::GetExistingResource<WMeshResource>(szMeshName);

    if (!m_hPreviewMeshResource.IsValid())
    {
      const char* szMeshBufferName = "KrautPreviewGroundMeshBuffer";

      WMeshBufferResourceHandle hMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        // Build geometry
        WGeometry::GeoOptions opt;
        opt.m_Transform = WMat4::MakeTranslation(WVec3(0, 0, -0.05f));

        WGeometry geom;
        geom.AddCylinder(8.0f, 7.9f, 0.05f, 0.05f, true, true, 32, opt);
        geom.TriangulatePolygons();
        geom.ComputeTangents();

        WMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

        hMeshBuffer = WResourceManager::GetOrCreateResource<WMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }
      {
        WResourceLock<WMeshBufferResource> pMeshBuffer(hMeshBuffer, WResourceAcquireMode::AllowLoadingFallback);

        WMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Pattern.WMaterialAsset
        md.ComputeBounds();

        m_hPreviewMeshResource = WResourceManager::GetOrCreateResource<WMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }

    // Ground Mesh Component
    {
      WGameObjectDesc obj;
      obj.m_sName.Assign("KrautGround");

      WGameObject* pObj;
      pWorld->CreateObject(obj, pObj);

      WMeshComponent* pMesh;
      WMeshComponent::CreateComponent(pObj, pMesh);
      pMesh->SetMesh(m_hPreviewMeshResource);
    }
  }
}

WEngineProcessViewContext* WKrautTreeContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WKrautTreeViewContext, this);
}

void WKrautTreeContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

bool WKrautTreeContext::UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext)
{
  {
    W_LOCK(m_pWorld->GetWriteMarker());

    m_pMainObject->UpdateLocalBounds();
    m_pMainObject->UpdateGlobalTransformAndBounds();
  }

  WBoundingBoxSphere bounds = m_pMainObject->GetGlobalBounds();

  // Bounds are only valid once the base-data task has run and SetDetails() was called.
  // Until then return false so the convergence counter doesn't start and the camera isn't
  // focused on an invalid bounding sphere.
  if (!bounds.IsValid())
    return false;

  // undo the artificial bounds scale to get a tight bbox for better thumbnails
  const float fAdditionalZoom = 1.5f;
  bounds.m_fSphereRadius /= WKrautTreeComponent::s_iLocalBoundsScale * fAdditionalZoom;
  bounds.m_vBoxHalfExtents /= WKrautTreeComponent::s_iLocalBoundsScale * fAdditionalZoom;

  WKrautTreeViewContext* pMeshViewContext = static_cast<WKrautTreeViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void WKrautTreeContext::QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg)
{
  if (m_pMainObject == nullptr)
    return;

  WBoundingBoxSphere bounds = WBoundingBoxSphere::MakeInvalid();

  {
    W_LOCK(m_pWorld->GetWriteMarker());

    m_pMainObject->UpdateLocalBounds();
    m_pMainObject->UpdateGlobalTransformAndBounds();
    auto b = m_pMainObject->GetGlobalBounds();

    if (b.IsValid())
    {
      b.m_fSphereRadius /= WKrautTreeComponent::s_iLocalBoundsScale;
      b.m_vBoxHalfExtents /= (float)WKrautTreeComponent::s_iLocalBoundsScale;

      bounds.ExpandToInclude(b);
    }
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
