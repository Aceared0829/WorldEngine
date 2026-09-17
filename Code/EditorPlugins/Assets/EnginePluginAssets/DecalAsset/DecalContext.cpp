#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDecalContext, 1, WRTTIDefaultAllocator<WDecalContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Decal"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WDecalContext::WDecalContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
}

void WDecalContext::OnInitialize()
{
  const char* szMeshName = "DefaultDecalPreviewMesh";
  m_hPreviewMeshResource = WResourceManager::GetExistingResource<WMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultDecalPreviewMeshBuffer";

    WMeshBufferResourceHandle hMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      WGeometry geom;
      WGeometry::GeoOptions opt;

      geom.AddBox(WVec3(0.5f, 1.0f, 1.0f), true);

      WMat4 t, r;
      t = WMat4::MakeTranslation(WVec3(0, 1.5f, 0));
      r = WMat4::MakeRotationZ(WAngle::MakeFromDegree(90));
      opt.m_Transform = t * r;
      geom.AddStackedSphere(0.5f, 64, 64, opt);

      t.SetTranslationVector(WVec3(0, -1.5f, 0));
      r = WMat4::MakeRotationY(WAngle::MakeFromDegree(90));
      opt.m_Transform = t * r;
      geom.AddTorus(0.1f, 0.5f, 32, 64, true, opt);

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
      md.SetMaterial(0, "Materials/Common/TestBricks.WMaterial");
      md.ComputeBounds();

      m_hPreviewMeshResource = WResourceManager::GetOrCreateResource<WMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  auto pWorld = m_pWorld;
  W_LOCK(pWorld->GetWriteMarker());

  WGameObjectDesc obj;
  WGameObject* pObj;

  // Preview Mesh that the decals get projected onto
  {
    obj.m_sName.Assign("DecalPreview");
    pWorld->CreateObject(obj, pObj);

    WMeshComponent* pMesh;
    WMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
  }

  // decals
  {
    WStringBuilder sDecalGuid;
    WConversionUtils::ToString(GetDocumentGuid(), sDecalGuid);

    // box
    {
      obj.m_sName.Assign("Decal1");
      obj.m_LocalPosition.Set(-0.25f, 0, 0);
      pWorld->CreateObject(obj, pObj);

      WDecalComponent* pDecal;
      WDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // torus
    {
      obj.m_sName.Assign("Decal2");
      obj.m_LocalPosition.Set(-0.2f, -1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      WDecalComponent* pDecal;
      WDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // sphere
    {
      obj.m_sName.Assign("Decal3");
      obj.m_LocalPosition.Set(-0.5f, 1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      WDecalComponent* pDecal;
      WDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }


    // box
    {
      obj.m_sName.Assign("Decal4");
      obj.m_LocalRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(180));
      obj.m_LocalPosition.Set(0.25f, 0, 0);
      pWorld->CreateObject(obj, pObj);

      WDecalComponent* pDecal;
      WDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(WVec3(2));
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // torus
    {
      obj.m_sName.Assign("Decal5");
      obj.m_LocalRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(180));
      obj.m_LocalPosition.Set(0.2f, -1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      WDecalComponent* pDecal;
      WDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(WVec3(2));
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // sphere
    {
      obj.m_sName.Assign("Decal6");
      obj.m_LocalRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(180));
      obj.m_LocalPosition.Set(0.5f, 1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      WDecalComponent* pDecal;
      WDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(WVec3(2));
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }
  }
}

WEngineProcessViewContext* WDecalContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WDecalViewContext, this);
}

void WDecalContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}
