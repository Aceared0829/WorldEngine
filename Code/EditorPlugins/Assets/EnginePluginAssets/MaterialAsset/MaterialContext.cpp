#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMaterialContext, 1, WRTTIDefaultAllocator<WMaterialContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Material"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WMaterialContext::WMaterialContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
}

void WMaterialContext::HandleMessage(const WEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WCreateThumbnailMsgToEngine>())
  {
    WResourceManager::RestoreResource(m_hMaterial);
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WDocumentConfigMsgToEngine>())
  {
    const WDocumentConfigMsgToEngine* pMsg2 = static_cast<const WDocumentConfigMsgToEngine*>(pMsg);

    if (pMsg2->m_sWhatToDo == "InvalidateCache")
    {
      // make sure all scenes etc rebuild their render cache
      WRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg2->m_sWhatToDo == "PreviewModel" && m_PreviewModel != (PreviewModel)pMsg2->m_iValue)
    {
      m_PreviewModel = (PreviewModel)pMsg2->m_iValue;

      auto pWorld = m_pWorld;
      W_LOCK(pWorld->GetWriteMarker());

      WMeshComponent* pMeshComp = nullptr;
      if (pWorld->TryGetComponent(m_hMeshComponent, pMeshComp))
      {
        switch (m_PreviewModel)
        {
          case PreviewModel::Ball:
            pMeshComp->SetMesh(m_hBallMesh);
            break;
          case PreviewModel::Sphere:
            pMeshComp->SetMesh(m_hSphereMesh);
            break;
          case PreviewModel::Box:
            pMeshComp->SetMesh(m_hBoxMesh);
            break;
          case PreviewModel::Plane:
            pMeshComp->SetMesh(m_hPlaneMesh);
            break;
        }
      }
    }
  }

  WEngineProcessDocumentContext::HandleMessage(pMsg);
}

void WMaterialContext::OnInitialize()
{
  {
    const char* szSphereMeshName = "SphereMaterialPreviewMesh";
    m_hSphereMesh = WResourceManager::GetExistingResource<WMeshResource>(szSphereMeshName);

    if (!m_hSphereMesh.IsValid())
    {
      const char* szMeshBufferName = "SphereMaterialPreviewMeshBuffer";

      WMeshBufferResourceHandle hMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        // Build geometry
        WGeometry geom;

        WGeometry::GeoOptions opt;
        opt.m_Color = WColor::Red;
        opt.m_Transform = WMat4::MakeRotationZ(WAngle::MakeFromDegree(90));
        geom.AddStackedSphere(1.0f, 64, 64, opt);
        geom.ComputeTangents();

        WMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AddStream(WMeshVertexStreamType::TexCoord1);
        desc.AddStream(WMeshVertexStreamType::Color0);
        desc.AddStream(WMeshVertexStreamType::Color1);
        desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

        hMeshBuffer = WResourceManager::GetOrCreateResource<WMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }

      {
        WResourceLock<WMeshBufferResource> pMeshBuffer(hMeshBuffer, WResourceAcquireMode::AllowLoadingFallback);

        WMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hSphereMesh = WResourceManager::GetOrCreateResource<WMeshResource>(szSphereMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  {
    const char* szBoxMeshName = "BoxMaterialPreviewMesh";
    m_hBoxMesh = WResourceManager::GetExistingResource<WMeshResource>(szBoxMeshName);

    if (!m_hBoxMesh.IsValid())
    {
      const char* szMeshBufferName = "BoxMaterialPreviewMeshBuffer";

      WMeshBufferResourceHandle hMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        WGeometry::GeoOptions opt;
        opt.m_Color = WColor::Red;

        // Build geometry
        WGeometry geom;

        geom.AddBox(WVec3(1.5f), true, opt);
        geom.ComputeTangents();

        WMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AddStream(WMeshVertexStreamType::TexCoord1);
        desc.AddStream(WMeshVertexStreamType::Color0);
        desc.AddStream(WMeshVertexStreamType::Color1);
        desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

        hMeshBuffer = WResourceManager::GetOrCreateResource<WMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }

      {
        WResourceLock<WMeshBufferResource> pMeshBuffer(hMeshBuffer, WResourceAcquireMode::AllowLoadingFallback);

        WMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hBoxMesh = WResourceManager::GetOrCreateResource<WMeshResource>(szBoxMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  {
    const char* szPlaneMeshName = "PlaneMaterialPreviewMesh";
    m_hPlaneMesh = WResourceManager::GetExistingResource<WMeshResource>(szPlaneMeshName);

    if (!m_hPlaneMesh.IsValid())
    {
      const char* szMeshBufferName = "PlaneMaterialPreviewMeshBuffer";

      WMeshBufferResourceHandle hMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        // Build geometry
        WGeometry geom;

        WGeometry::GeoOptions opt;
        opt.m_Color = WColor::Red;
        opt.m_Transform = WMat4::MakeRotationZ(WAngle::MakeFromDegree(-90));
        geom.AddRect(WVec2(2.0f), 64, 64, opt);
        geom.ComputeTangents();

        WMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AddStream(WMeshVertexStreamType::TexCoord1);
        desc.AddStream(WMeshVertexStreamType::Color0);
        desc.AddStream(WMeshVertexStreamType::Color1);
        desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

        hMeshBuffer = WResourceManager::GetOrCreateResource<WMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }

      {
        WResourceLock<WMeshBufferResource> pMeshBuffer(hMeshBuffer, WResourceAcquireMode::AllowLoadingFallback);

        WMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hPlaneMesh = WResourceManager::GetOrCreateResource<WMeshResource>(szPlaneMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  {
    m_hBallMesh = WResourceManager::LoadResource<WMeshResource>("Editor/Meshes/MaterialBall.WBinMesh");
  }

  auto pWorld = m_pWorld;
  W_LOCK(pWorld->GetWriteMarker());

  WGameObjectDesc obj;
  WGameObject* pObj;

  // Preview Mesh
  {
    obj.m_sName.Assign("MaterialPreview");
    m_hMeshObject = pWorld->CreateObject(obj, pObj);

    WMeshComponent* pMesh;
    m_hMeshComponent = WMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hBallMesh);
    WStringBuilder sMaterialGuid;
    WConversionUtils::ToString(GetDocumentGuid(), sMaterialGuid);
    m_hMaterial = WResourceManager::LoadResource<WMaterialResource>(sMaterialGuid);

    // 20 material overrides should be enough for any mesh.
    for (WUInt32 i = 0; i < 20; ++i)
    {
      pMesh->SetMaterial(i, m_hMaterial);
    }
  }
}

WEngineProcessViewContext* WMaterialContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WMaterialViewContext, this);
}

void WMaterialContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

bool WMaterialContext::UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext)
{
  WMaterialViewContext* pMaterialViewContext = static_cast<WMaterialViewContext*>(pThumbnailViewContext);
  pMaterialViewContext->PositionThumbnailCamera();
  return true;
}
