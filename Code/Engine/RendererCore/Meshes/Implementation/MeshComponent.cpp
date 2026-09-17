#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WMeshComponent, 3, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Static"), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new WDefaultValueAttribute(WVec4(0, 1, 0, 1))),
    W_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material")),
    W_ACCESSOR_PROPERTY("SortingDepthOffset", GetSortingDepthOffset, SetSortingDepthOffset),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractGeometry, OnMsgExtractGeometry)
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE
// clang-format on

WMeshComponent::WMeshComponent() = default;
WMeshComponent::~WMeshComponent() = default;

void WMeshComponent::OnMsgExtractGeometry(WMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != WWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  // ignore invalid and created resources
  {
    WMeshResourceHandle hRenderMesh = GetMesh();
    if (!hRenderMesh.IsValid())
      return;

    WResourceLock<WMeshResource> pRenderMesh(hRenderMesh, WResourceAcquireMode::PointerOnly);
    if (pRenderMesh->GetBaseResourceFlags().IsAnySet(WResourceFlags::IsCreatedResource))
      return;
  }

  ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), WResourceManager::LoadResource<WCpuMeshResource>(GetMesh().GetResourceID()));
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WMeshImportTransform, 1)
  W_ENUM_CONSTANT(WMeshImportTransform::Blender_YUp),
  W_ENUM_CONSTANT(WMeshImportTransform::Blender_ZUp),
  W_ENUM_CONSTANT(WMeshImportTransform::Custom),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WBasisAxis::Enum WMeshImportTransform::GetRightDir(WMeshImportTransform::Enum transform, WBasisAxis::Enum dir)
{
  switch (transform)
  {
    case WMeshImportTransform::Blender_YUp:
      return WBasisAxis::NegativeX;
    case WMeshImportTransform::Blender_ZUp:
      return WBasisAxis::NegativeX;
    case WMeshImportTransform::Custom:
      return dir;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return dir;
}

WBasisAxis::Enum WMeshImportTransform::GetUpDir(WMeshImportTransform::Enum transform, WBasisAxis::Enum dir)
{
  switch (transform)
  {
    case WMeshImportTransform::Blender_YUp:
      return WBasisAxis::PositiveY;
    case WMeshImportTransform::Blender_ZUp:
      return WBasisAxis::PositiveZ;
    case WMeshImportTransform::Custom:
      return dir;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return dir;
}

bool WMeshImportTransform::GetFlipForward(WMeshImportTransform::Enum transform, bool bFlip)
{
  switch (transform)
  {
    case WMeshImportTransform::Blender_YUp:
      return false;
    case WMeshImportTransform::Blender_ZUp:
      return false;
    case WMeshImportTransform::Custom:
      return bFlip;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return bFlip;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponent);
