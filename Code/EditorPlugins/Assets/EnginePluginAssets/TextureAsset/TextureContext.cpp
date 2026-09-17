#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>

#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureContext, 1, WRTTIDefaultAllocator<WTextureContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture 2D;Render Target;Substance Package"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static void CreatePreviewRect(WGeometry& ref_geom)
{
  const WMat4 mTransform = WMat4::MakeIdentity();
  const WVec2 size(1.0f);
  const WColor color = WColor::White;

  const WVec2 halfSize = size * 0.5f;

  WUInt32 idx[4];

  idx[0] = ref_geom.AddVertex(mTransform, WVec3(-halfSize.x, 0, -halfSize.y), WVec3(-1, 0, 0), WVec2(-1, 2), color);
  idx[1] = ref_geom.AddVertex(mTransform, WVec3(halfSize.x, 0, -halfSize.y), WVec3(-1, 0, 0), WVec2(2, 2), color);
  idx[2] = ref_geom.AddVertex(mTransform, WVec3(halfSize.x, 0, halfSize.y), WVec3(-1, 0, 0), WVec2(2, -1), color);
  idx[3] = ref_geom.AddVertex(mTransform, WVec3(-halfSize.x, 0, halfSize.y), WVec3(-1, 0, 0), WVec2(-1, -1), color);

  ref_geom.AddPolygon(idx, false);
}

WTextureContext::WTextureContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
}

void WTextureContext::HandleMessage(const WEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WDocumentConfigMsgToEngine>() && !m_SlicePreviews.IsEmpty())
  {
    const WDocumentConfigMsgToEngine* pMsg2 = static_cast<const WDocumentConfigMsgToEngine*>(pMsg);

    if (pMsg2->m_sWhatToDo == "SetChannelMode")
    {
      m_iChannelMode = pMsg2->m_iValue;
      m_fAlphaThreshold = pMsg2->m_fValue;

      for (auto& slice : m_SlicePreviews)
      {
        WResourceLock<WMaterialResource> pMaterial(slice.m_hMaterial, WResourceAcquireMode::AllowLoadingFallback);
        if (pMaterial.GetAcquireResult() != WResourceAcquireResult::Final)
        {
          // Would modify the loading fallback; retry once the real material is loaded.
          m_bSliceMaterialsDirty = true;
          break;
        }

        pMaterial->SetParameter("ShowChannelMode", m_iChannelMode);
        pMaterial->SetParameter("AlphaThreshold", m_fAlphaThreshold);
      }
    }
    else if (pMsg2->m_sWhatToDo == "SetLodLevel")
    {
      if (pMsg2->m_iValue != m_iLodLevel)
      {
        m_iLodLevel = pMsg2->m_iValue;
        for (auto& slice : m_SlicePreviews)
        {
          WResourceLock<WMaterialResource> pMaterial(slice.m_hMaterial, WResourceAcquireMode::AllowLoadingFallback);
          if (pMaterial.GetAcquireResult() != WResourceAcquireResult::Final)
          {
            m_bSliceMaterialsDirty = true;
            break;
          }

          pMaterial->SetParameter("LodLevel", m_iLodLevel);
        }
      }
    }
    else if (pMsg2->m_sWhatToDo == "SetTexture" && pMsg2->m_sValue.IsEmpty() == false)
    {
      SetTexture(pMsg2->m_sValue);
    }
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WViewRedrawMsgToEngine>())
  {
    // Non-blocking: check current array size and rebuild objects if it changed.
    // Defaults to 1 if the texture is not yet loaded.
    WUInt32 uiArraySlices = 1;
    if (m_hTexture.IsValid())
    {
      WResourceLock<WTexture2DResource> pTex(m_hTexture, WResourceAcquireMode::PointerOnly);
      if (pTex->GetFormat() != WGALResourceFormat::Invalid && pTex->GetType() == WGALTextureType::Texture2DArray)
      {
        const WGALTexture* pGALTex = WGALDevice::GetDefaultDevice()->GetTexture(pTex->GetGALTexture());
        if (pGALTex != nullptr)
          uiArraySlices = pGALTex->GetDescription().m_uiArraySize;
      }
    }
    RebuildPreviewObjects(uiArraySlices);

    // Applying the material parameters can fail while the materials are still loading (a loading
    // fallback would be modified instead of the real material). In that case retry on the next redraw.
    if (m_bSliceMaterialsDirty)
    {
      ApplySliceMaterialParameters();
    }
  }

  WEngineProcessDocumentContext::HandleMessage(pMsg);
}

void WTextureContext::OnInitialize()
{
  // Preview Mesh
  const char* szMeshName = "DefaultTexturePreviewMesh";
  m_hPreviewMeshResource = WResourceManager::GetExistingResource<WMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultTexturePreviewMeshBuffer";

    WMeshBufferResourceHandle hMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      WGeometry geom;
      CreatePreviewRect(geom);
      geom.ComputeTangents();

      WMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

      hMeshBuffer = WResourceManager::GetOrCreateResource<WMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
    }

    WResourceLock<WMeshBufferResource> pMeshBuffer(hMeshBuffer, WResourceAcquireMode::AllowLoadingFallback);

    WMeshResourceDescriptor md;
    md.UseExistingMeshBuffer(hMeshBuffer);
    md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
    md.SetMaterial(0, "");
    md.ComputeBounds();

    m_hPreviewMeshResource = WResourceManager::GetOrCreateResource<WMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
  }

  WStringBuilder sTextureGuid;
  WConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);
  SetTexture(sTextureGuid);
}

WEngineProcessViewContext* WTextureContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WTextureViewContext, this);
}

void WTextureContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

void WTextureContext::RebuildPreviewObjects(WUInt32 uiNumArraySlices)
{
  if (uiNumArraySlices == 0)
    uiNumArraySlices = 1;

  if (uiNumArraySlices == m_uiNumArraySlices && !m_bPreviewObjectsDirty)
    return;

  m_uiNumArraySlices = uiNumArraySlices;
  m_bPreviewObjectsDirty = false;

  // The slice materials are created resources, so they have no data loader and can never be reloaded
  // once their data was unloaded (which happens when the document is closed and the material loses its
  // last reference). Reusing such a name would yield a material without base material or shader, which
  // renders as the fallback texture. A process wide counter keeps every batch of materials distinct.
  static WAtomicInteger32 s_uiNextMaterialGeneration = 0;
  m_uiMaterialGeneration = (WUInt32)s_uiNextMaterialGeneration.Increment();

  WStringBuilder sTextureGuid;
  WConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);

  W_LOCK(m_pWorld->GetWriteMarker());

  for (auto& slice : m_SlicePreviews)
    m_pWorld->DeleteObjectDelayed(slice.m_hObject);
  m_SlicePreviews.Clear();

  // Each subsequent slice is placed slightly behind (positive X) and shifted in YZ
  // so its edges peek out from behind the previous slice (deck-of-cards effect).
  // The preview quad lies in the YZ plane after the 90 degree Z rotation on the game object.
  const float fOffsetYZ = 0.25f;
  const float fOffsetX = 0.2f;

  for (WUInt32 i = 0; i < uiNumArraySlices; ++i)
  {
    // Per-slice material
    WStringBuilder sMaterialName;
    sMaterialName.SetFormat("{}-{} - Texture Preview Slice {}", sTextureGuid, m_uiMaterialGeneration, i);

    WMaterialResourceDescriptor md;
    md.m_hBaseMaterial = WResourceManager::LoadResource<WMaterialResource>("Editor/Materials/TexturePreview.WMaterial");
    WMaterialResourceHandle hMaterial = WResourceManager::GetOrCreateResource<WMaterialResource>(sMaterialName, std::move(md));

    // Game object at stacked position
    WGameObjectDesc obj;
    WGameObject* pObj = nullptr;
    obj.m_sName.Assign("TexturePreview");
    obj.m_LocalRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(90));
    obj.m_LocalPosition = (float)i * WVec3(fOffsetX, fOffsetYZ, -fOffsetYZ);
    WGameObjectHandle hObject = m_pWorld->CreateObject(obj, pObj);

    WMeshComponent* pMesh = nullptr;
    WComponentHandle hMeshComp = WMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
    pMesh->SetMaterial(0, hMaterial);

    auto& slice = m_SlicePreviews.ExpandAndGetRef();
    slice.m_hObject = hObject;
    slice.m_hMeshComponent = hMeshComp;
    slice.m_hMaterial = hMaterial;
  }

  m_bSliceMaterialsDirty = true;
  ApplySliceMaterialParameters();
}

void WTextureContext::ApplySliceMaterialParameters()
{
  WUInt32 uiArrayIndex = 0;

  for (auto& slice : m_SlicePreviews)
  {
    WResourceLock<WMaterialResource> pMaterial(slice.m_hMaterial, WResourceAcquireMode::AllowLoadingFallback);

    if (pMaterial.GetAcquireResult() != WResourceAcquireResult::Final)
      return;

    pMaterial->SetParameter("ArrayIndex", (int)uiArrayIndex);
    pMaterial->SetParameter("LodLevel", m_iLodLevel);
    pMaterial->SetParameter("ShowChannelMode", m_iChannelMode);
    pMaterial->SetParameter("AlphaThreshold", m_fAlphaThreshold);

    if (m_hTexture.IsValid())
    {
      pMaterial->SetTexture2DBinding("BaseTexture", m_hTexture);

      WResourceLock<WTexture2DResource> pTexture(m_hTexture, WResourceAcquireMode::PointerOnly);
      if (pTexture->GetFormat() != WGALResourceFormat::Invalid)
      {
        pMaterial->SetParameter("IsLinear", !WGALResourceFormat::IsSrgb(pTexture->GetFormat()));
      }
    }

    ++uiArrayIndex;
  }

  m_bSliceMaterialsDirty = false;
}

void WTextureContext::SetTexture(WStringView sTextureFile)
{
  if (m_hTexture.IsValid() && m_hTexture.GetResourceID() == sTextureFile)
    return;

  m_hTexture = WResourceManager::LoadResource<WTexture2DResource>(sTextureFile);

  {
    WResourceLock<WTexture2DResource> pTexture(m_hTexture, WResourceAcquireMode::PointerOnly);
    pTexture->m_ResourceEvents.AddEventHandler(WMakeDelegate(&WTextureContext::OnResourceEvent, this), m_TextureResourceEventSubscriber);
  }

  m_bPreviewObjectsDirty = true;
  m_bSliceMaterialsDirty = true;
}

void WTextureContext::OnResourceEvent(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUpdated)
  {
    const WTexture2DResource* pTexture = static_cast<const WTexture2DResource*>(e.m_pResource);
    if (pTexture->GetFormat() != WGALResourceFormat::Invalid)
    {
      m_bPreviewObjectsDirty = true;
      m_bSliceMaterialsDirty = true;
    }
  }
}
