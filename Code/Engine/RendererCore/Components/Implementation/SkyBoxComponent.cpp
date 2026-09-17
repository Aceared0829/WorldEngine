#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/TextureCubeResource.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSkyBoxComponent, 4, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("CubeMap", GetCubeMap, SetCubeMap)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_Cube"), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("ExposureBias", GetExposureBias, SetExposureBias)->AddAttributes(new WClampValueAttribute(-32.0f, 32.0f)),
    W_ACCESSOR_PROPERTY("InverseTonemap", GetInverseTonemap, SetInverseTonemap),
    W_ACCESSOR_PROPERTY("UseFog", GetUseFog, SetUseFog)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("VirtualDistance", GetVirtualDistance, SetVirtualDistance)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1000.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE;
// clang-format on

WSkyBoxComponent::WSkyBoxComponent() = default;
WSkyBoxComponent::~WSkyBoxComponent() = default;

void WSkyBoxComponent::Initialize()
{
  SUPER::Initialize();

  const char* szBufferResourceName = "SkyBoxBuffer";
  WMeshBufferResourceHandle hMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>(szBufferResourceName);
  if (!hMeshBuffer.IsValid())
  {
    WGeometry geom;
    geom.AddRect(WVec2(2.0f));

    WMeshBufferResourceDescriptor desc;
    desc.AddStream(WMeshVertexStreamType::Position);
    desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

    hMeshBuffer = WResourceManager::GetOrCreateResource<WMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
  }

  const char* szMeshResourceName = "SkyBoxMesh";
  m_hMesh = WResourceManager::GetExistingResource<WMeshResource>(szMeshResourceName);
  if (!m_hMesh.IsValid())
  {
    WMeshResourceDescriptor desc;
    desc.UseExistingMeshBuffer(hMeshBuffer);
    desc.AddSubMesh(2, 0, 0);
    desc.ComputeBounds();

    m_hMesh = WResourceManager::GetOrCreateResource<WMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
  }

  WStringBuilder cubeMapMaterialName = "SkyBoxMaterial_CubeMap";
  cubeMapMaterialName.AppendFormat("_{0}", WArgP(GetWorld())); // make the resource unique for each world

  m_hCubeMapMaterial = WResourceManager::GetExistingResource<WMaterialResource>(cubeMapMaterialName);
  if (!m_hCubeMapMaterial.IsValid())
  {
    WMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = WResourceManager::LoadResource<WMaterialResource>("{ b4b75b1c-c2c8-4a0e-8076-780bdd46d18b }"); // Sky.WMaterialAsset

    m_hCubeMapMaterial = WResourceManager::CreateResource<WMaterialResource>(cubeMapMaterialName, std::move(desc), cubeMapMaterialName);
  }

  UpdateMaterials();
}

void WSkyBoxComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateMaterials();
}

void WSkyBoxComponent::OnDeactivated()
{
  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  SUPER::OnDeactivated();
}

WResult WSkyBoxComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return W_SUCCESS;
}

void WSkyBoxComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  // Don't extract sky render data for selection or in orthographic views.
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory || msg.m_pView->GetCamera()->IsOrthographic())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  WTransform globalTransform = GetOwner()->GetGlobalTransform();
  globalTransform.m_vPosition.SetZero(); // skybox should always be at the origin
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, globalTransform, m_InstanceDataOffset, GetUniqueIdForRendering());

  WMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WMeshRenderData>(GetOwner());
  pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, m_hCubeMapMaterial, m_hMesh);

  msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::Sky, WRenderData::Caching::IfStatic);
}

void WSkyBoxComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_fExposureBias;
  s << m_bInverseTonemap;
  s << m_bUseFog;
  s << m_fVirtualDistance;
  s << m_hCubeMap;
}

void WSkyBoxComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  s >> m_fExposureBias;
  s >> m_bInverseTonemap;

  if (uiVersion >= 4)
  {
    s >> m_bUseFog;
    s >> m_fVirtualDistance;
  }

  if (uiVersion >= 3)
  {
    s >> m_hCubeMap;
  }
  else
  {
    WTexture2DResourceHandle dummyHandle;
    for (int i = 0; i < 6; i++)
    {
      s >> dummyHandle;
    }
  }
}

void WSkyBoxComponent::SetExposureBias(float fExposureBias)
{
  m_fExposureBias = fExposureBias;

  UpdateMaterials();
}

void WSkyBoxComponent::SetInverseTonemap(bool bInverseTonemap)
{
  m_bInverseTonemap = bInverseTonemap;

  UpdateMaterials();
}

void WSkyBoxComponent::SetUseFog(bool bUseFog)
{
  m_bUseFog = bUseFog;

  UpdateMaterials();
}

void WSkyBoxComponent::SetVirtualDistance(float fVirtualDistance)
{
  m_fVirtualDistance = fVirtualDistance;

  UpdateMaterials();
}

void WSkyBoxComponent::SetCubeMap(const WTextureCubeResourceHandle& hCubeMap)
{
  m_hCubeMap = hCubeMap;
  UpdateMaterials();
}

const WTextureCubeResourceHandle& WSkyBoxComponent::GetCubeMap() const
{
  return m_hCubeMap;
}

void WSkyBoxComponent::UpdateMaterials()
{
  if (m_hCubeMapMaterial.IsValid())
  {
    WResourceLock<WMaterialResource> pMaterial(m_hCubeMapMaterial, WResourceAcquireMode::AllowLoadingFallback);

    pMaterial->SetParameter("ExposureBias", m_fExposureBias);
    pMaterial->SetParameter("InverseTonemap", m_bInverseTonemap);
    pMaterial->SetParameter("UseFog", m_bUseFog);
    pMaterial->SetParameter("VirtualDistance", m_fVirtualDistance);
    pMaterial->SetTextureCubeBinding("CubeMap", m_hCubeMap);

    pMaterial->PreserveCurrentDesc();
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WSkyBoxComponentPatch_1_2 : public WGraphPatch
{
public:
  WSkyBoxComponentPatch_1_2()
    : WGraphPatch("WSkyBoxComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Exposure Bias", "ExposureBias");
    pNode->RenameProperty("Inverse Tonemap", "InverseTonemap");
    pNode->RenameProperty("Left Texture", "LeftTexture");
    pNode->RenameProperty("Front Texture", "FrontTexture");
    pNode->RenameProperty("Right Texture", "RightTexture");
    pNode->RenameProperty("Back Texture", "BackTexture");
    pNode->RenameProperty("Up Texture", "UpTexture");
    pNode->RenameProperty("Down Texture", "DownTexture");
  }
};

WSkyBoxComponentPatch_1_2 g_WSkyBoxComponentPatch_1_2;



W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SkyBoxComponent);
