#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/LodComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/LodMeshComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WLodMeshLod, WNoBase, 2, WRTTIDefaultAllocator<WLodMeshLod>)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("Mesh", m_hMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Static"), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("Threshold", m_fThreshold)
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WLodMeshComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new WDefaultValueAttribute(WVec4(0, 1, 0, 1))),
    W_ACCESSOR_PROPERTY("SortingDepthOffset", GetSortingDepthOffset, SetSortingDepthOffset),
    W_MEMBER_PROPERTY("BoundsOffset", m_vBoundsOffset),
    W_MEMBER_PROPERTY("BoundsRadius", m_fBoundsRadius)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.01f, 100.0f)),
    W_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    W_ACCESSOR_PROPERTY("OverlapRanges", GetOverlapRanges, SetOverlapRanges)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ARRAY_MEMBER_PROPERTY("Meshes", m_Meshes),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering"),
    new WSphereVisualizerAttribute("BoundsRadius", WColor::MediumVioletRed, nullptr, WVisualizerAnchor::Center, WVec3(1.0f), "BoundsOffset"),
    new WTransformManipulatorAttribute("BoundsOffset"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgSetColor, OnMsgSetColor),
    W_MESSAGE_HANDLER(WMsgSetCustomData, OnMsgSetCustomData),
    W_MESSAGE_HANDLER(WMsgExtractGeometry, OnMsgExtractGeometry),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE;
// clang-format on

struct LodMeshCompFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    OverlapRanges = 1,
  };
};

WLodMeshComponent::WLodMeshComponent() = default;
WLodMeshComponent::~WLodMeshComponent() = default;

void WLodMeshComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(LodMeshCompFlags::ShowDebugInfo, bShow);
}

bool WLodMeshComponent::GetShowDebugInfo() const
{
  return GetUserFlag(LodMeshCompFlags::ShowDebugInfo);
}

void WLodMeshComponent::SetOverlapRanges(bool bShow)
{
  SetUserFlag(LodMeshCompFlags::OverlapRanges, bShow);
}

bool WLodMeshComponent::GetOverlapRanges() const
{
  return GetUserFlag(LodMeshCompFlags::OverlapRanges);
}

void WLodMeshComponent::OnDeactivated()
{
  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  SUPER::OnDeactivated();
}

void WLodMeshComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_Meshes.GetCount();
  for (const auto& mesh : m_Meshes)
  {
    s << mesh.m_hMesh;
    s << mesh.m_fThreshold;
  }

  s << m_Color;
  s << m_fSortingDepthOffset;

  s << m_vBoundsOffset;
  s << m_fBoundsRadius;

  s << m_vCustomData;
}

void WLodMeshComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  WUInt32 uiMeshes = 0;
  s >> uiMeshes;

  m_Meshes.SetCount(uiMeshes);

  for (auto& mesh : m_Meshes)
  {
    s >> mesh.m_hMesh;
    s >> mesh.m_fThreshold;
  }

  s >> m_Color;
  s >> m_fSortingDepthOffset;

  s >> m_vBoundsOffset;
  s >> m_fBoundsRadius;

  if (uiVersion >= 2)
  {
    s >> m_vCustomData;
  }
}

WResult WLodMeshComponent::GetLocalBounds(WBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  out_bounds = WBoundingSphere::MakeFromCenterAndRadius(m_vBoundsOffset, m_fBoundsRadius);
  out_bAlwaysVisible = false;
  return W_SUCCESS;
}

void WLodMeshComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (m_Meshes.IsEmpty())
    return;

  if (msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::EditorView || msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::MainView)
  {
    UpdateSelectedLod(*msg.m_pView);
  }

  if (m_iCurLod >= (WInt32)m_Meshes.GetCount())
    return;

  auto hMesh = m_Meshes[m_iCurLod].m_hMesh;

  if (!hMesh.IsValid())
    return;

  // Force dynamic instance data buffer since the render data is not cached, so we would trash the static instance data buffer every frame.
  const bool bDynamic = true;
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color, m_vCustomData);

  WResourceLock<WMeshResource> pMesh(hMesh, WResourceAcquireMode::AllowLoadingFallback);
  WArrayPtr<const WMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (WUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const WUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    WMaterialResourceHandle hMaterial;

    hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    WMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WMeshRenderData>(GetOwner());
    pRenderData->m_fSortingDepthOffset = m_fSortingDepthOffset;
    pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
    pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, hMesh, uiMaterialIndex, uiPartIndex);

    WRenderData::Category category = WMaterialResource::GetRenderDataCategory(hMaterial);

    msg.AddRenderData(pRenderData, category, WRenderData::Caching::Never);
  }
}

void WLodMeshComponent::OnMsgExtractGeometry(WMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != WWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  for (WUInt32 i = m_Meshes.GetCount(); i > 0; --i)
  {
    const WMeshResourceHandle& hMesh = m_Meshes[i - 1].m_hMesh;

    if (!hMesh.IsValid())
      continue;

    // A procedurally created mesh only exists on the GPU side, there is no file to load a CPU mesh from.
    {
      WResourceLock<WMeshResource> pMesh(hMesh, WResourceAcquireMode::PointerOnly);
      if (pMesh->GetBaseResourceFlags().IsAnySet(WResourceFlags::IsCreatedResource))
        continue;
    }

    ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), WResourceManager::LoadResource<WCpuMeshResource>(hMesh.GetResourceID()));
    return;
  }
}

void WLodMeshComponent::SetColor(const WColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const WColor& WLodMeshComponent::GetColor() const
{
  return m_Color;
}

void WLodMeshComponent::SetCustomData(const WVec4& vData)
{
  m_vCustomData = vData;

  InvalidateCachedRenderData();
}

const WVec4& WLodMeshComponent::GetCustomData() const
{
  return m_vCustomData;
}

void WLodMeshComponent::SetSortingDepthOffset(float fOffset)
{
  m_fSortingDepthOffset = fOffset;

  InvalidateCachedRenderData();
}

float WLodMeshComponent::GetSortingDepthOffset() const
{
  return m_fSortingDepthOffset;
}

void WLodMeshComponent::OnMsgSetColor(WMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

void WLodMeshComponent::OnMsgSetCustomData(WMsgSetCustomData& ref_msg)
{
  m_vCustomData = ref_msg.m_vData;

  InvalidateCachedRenderData();
}

static float CalculateSphereScreenSpaceCoverage(const WBoundingSphere& sphere, const WCamera& camera)
{
  if (camera.IsPerspective())
  {
    return WGraphicsUtils::CalculateSphereScreenCoverage(sphere, camera.GetCenterPosition(), camera.GetFovY(1.0f));
  }
  else
  {
    return WGraphicsUtils::CalculateSphereScreenCoverage(sphere.m_fRadius, camera.GetDimensionY(1.0f));
  }
}

void WLodMeshComponent::UpdateSelectedLod(const WView& view) const
{
  const WInt32 iNumLods = (WInt32)m_Meshes.GetCount();

  const WVec3 vScale = GetOwner()->GetGlobalScaling();
  const float fScale = WMath::Max(vScale.x, vScale.y, vScale.z);
  const WVec3 vCenter = GetOwner()->GetGlobalTransform() * m_vBoundsOffset;

  const float fCoverage = CalculateSphereScreenSpaceCoverage(WBoundingSphere::MakeFromCenterAndRadius(vCenter, fScale * m_fBoundsRadius), *view.GetLodCamera()) * WMath::Max(0.0f, (float)cvar_RenderingLodCoverageScale);

  // clamp the input value, this is to prevent issues while editing the threshold array
  WInt32 iNewLod = WMath::Clamp<WInt32>(m_iCurLod, 0, iNumLods);

  float fCoverageP = 1;
  float fCoverageN = 0;

  if (iNewLod > 0)
  {
    fCoverageP = m_Meshes[iNewLod - 1].m_fThreshold;
  }

  if (iNewLod < iNumLods)
  {
    fCoverageN = m_Meshes[iNewLod].m_fThreshold;
  }

  if (GetOverlapRanges())
  {
    const float fLodRangeOverlap = 0.40f;

    if (iNewLod + 1 < iNumLods)
    {
      float range = (fCoverageN - m_Meshes[iNewLod + 1].m_fThreshold);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
    else
    {
      float range = (fCoverageN - 0.0f);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
  }

  if (fCoverage < fCoverageN)
  {
    ++iNewLod;
  }
  else if (fCoverage > fCoverageP)
  {
    --iNewLod;
  }

  iNewLod = WMath::Clamp(iNewLod, 0, iNumLods);

  if (cvar_RenderingLodForce >= 0)
  {
    iNewLod = WMath::Min<WInt32>(cvar_RenderingLodForce, iNumLods - 1);
  }

  m_iCurLod = iNewLod;

  if (GetShowDebugInfo())
  {
    WStringBuilder sb;
    sb.SetFormat("Coverage: {}\nLOD {}\nRange: {} - {}", WArgF(fCoverage, 3), iNewLod, WArgF(fCoverageP, 3), WArgF(fCoverageN, 3));
    WDebugRenderer::Draw3DText(view.GetHandle(), sb, GetOwner()->GetGlobalPosition(), WColor::White);
  }
}


W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_LodMeshComponent);
