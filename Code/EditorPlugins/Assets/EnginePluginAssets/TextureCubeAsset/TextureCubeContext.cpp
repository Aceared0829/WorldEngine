#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeView.h>

#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureCubeContext, 1, WRTTIDefaultAllocator<WTextureCubeContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture Cube"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WTextureCubeContext::WTextureCubeContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
}

void WTextureCubeContext::HandleMessage(const WEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WDocumentConfigMsgToEngine>() && m_hMaterial.IsValid())
  {
    const WDocumentConfigMsgToEngine* pMsg2 = static_cast<const WDocumentConfigMsgToEngine*>(pMsg);

    WResourceLock<WMaterialResource> pMaterial(m_hMaterial, WResourceAcquireMode::AllowLoadingFallback);
    if (pMsg2->m_sWhatToDo == "SetChannelMode")
    {
      pMaterial->SetParameter("ShowChannelMode", pMsg2->m_iValue);
      pMaterial->SetParameter("AlphaThreshold", pMsg2->m_fValue);
    }
    else if (pMsg2->m_sWhatToDo == "SetLodLevel")
    {
      pMaterial->SetParameter("LodLevel", pMsg2->m_iValue);
    }
  }

  WEngineProcessDocumentContext::HandleMessage(pMsg);
}

void WTextureCubeContext::OnInitialize()
{
  const char* szMeshName = "DefaultTextureCubePreviewMesh";
  WStringBuilder sTextureGuid;
  WConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);
  const WStringBuilder sMaterialResource(sTextureGuid.GetData(), " - TextureCube Preview");

  m_hPreviewMeshResource = WResourceManager::GetExistingResource<WMeshResource>(szMeshName);
  m_hMaterial = WResourceManager::GetExistingResource<WMaterialResource>(sMaterialResource);

  m_hTexture = WResourceManager::LoadResource<WTextureCubeResource>(sTextureGuid);
  WGALResourceFormat::Enum textureFormat = WGALResourceFormat::Invalid;
  {
    WResourceLock<WTextureCubeResource> pTexture(m_hTexture, WResourceAcquireMode::PointerOnly);

    textureFormat = pTexture->GetFormat();
    pTexture->m_ResourceEvents.AddEventHandler(WMakeDelegate(&WTextureCubeContext::OnResourceEvent, this), m_TextureResourceEventSubscriber);
  }

  // Preview Mesh
  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultTextureCubePreviewMeshBuffer";

    WMeshBufferResourceHandle hMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      WGeometry geom;
      geom.AddStackedSphere(0.5f, 64, 64);
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
      md.SetMaterial(0, "");
      md.ComputeBounds();

      m_hPreviewMeshResource = WResourceManager::GetOrCreateResource<WMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  // Preview Material
  if (!m_hMaterial.IsValid())
  {
    WMaterialResourceDescriptor md;
    md.m_hBaseMaterial = WResourceManager::LoadResource<WMaterialResource>("Editor/Materials/TextureCubePreview.WMaterial");

    auto& tb = md.m_TextureCubeBindings.ExpandAndGetRef();
    tb.m_Name.Assign("BaseTexture");
    tb.m_Value = m_hTexture;

    auto& param = md.m_Parameters.ExpandAndGetRef();
    param.m_Name.Assign("IsLinear");
    param.m_Value = textureFormat != WGALResourceFormat::Invalid ? !WGALResourceFormat::IsSrgb(textureFormat) : false;

    m_hMaterial = WResourceManager::GetOrCreateResource<WMaterialResource>(sMaterialResource, std::move(md));
  }

  // Preview Object
  {
    W_LOCK(m_pWorld->GetWriteMarker());

    WGameObjectDesc obj;
    WGameObject* pObj;

    obj.m_sName.Assign("TextureCubePreview");
    obj.m_LocalRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(90));
    m_hPreviewObject = m_pWorld->CreateObject(obj, pObj);

    WMeshComponent* pMesh;
    m_hPreviewMesh2D = WMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
    pMesh->SetMaterial(0, m_hMaterial);
  }
}

WEngineProcessViewContext* WTextureCubeContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WTextureCubeViewContext, this);
}

void WTextureCubeContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

void WTextureCubeContext::OnResourceEvent(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUpdated)
  {
    const WTextureCubeResource* pTexture = static_cast<const WTextureCubeResource*>(e.m_pResource);
    if (pTexture->GetFormat() != WGALResourceFormat::Invalid)
    {
      WResourceLock<WMaterialResource> pMaterial(m_hMaterial, WResourceAcquireMode::BlockTillLoaded);
      pMaterial->SetParameter("IsLinear", !WGALResourceFormat::IsSrgb(pTexture->GetFormat()));
    }
  }
}
