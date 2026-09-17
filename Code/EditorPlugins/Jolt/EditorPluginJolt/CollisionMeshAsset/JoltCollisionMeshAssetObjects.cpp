#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WJoltSurfaceResourceSlot, WNoBase, 1, WRTTIDefaultAllocator<WJoltSurfaceResourceSlot>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new WReadOnlyAttribute()),
    W_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),
    W_MEMBER_PROPERTY("Exclude", m_bExclude),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WJoltCollisionMeshType, 2)
  W_ENUM_CONSTANT(WJoltCollisionMeshType::ConvexHull),
  W_ENUM_CONSTANT(WJoltCollisionMeshType::TriangleMesh),
  W_ENUM_CONSTANT(WJoltCollisionMeshType::Cylinder),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WJoltConvexCollisionMeshType, 1)
  W_ENUM_CONSTANT(WJoltConvexCollisionMeshType::ConvexHull),
  W_ENUM_CONSTANT(WJoltConvexCollisionMeshType::Cylinder),
  W_ENUM_CONSTANT(WJoltConvexCollisionMeshType::ConvexDecomposition),
  W_ENUM_CONSTANT(WJoltConvexCollisionMeshType::ConvexHullGroup),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WJoltCollisionMeshAssetProperties, 3, WRTTIDefaultAllocator<WJoltCollisionMeshAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("ImportTransform", WMeshImportTransform, m_ImportTransform),
    W_ENUM_MEMBER_PROPERTY("RightDir", WBasisAxis, m_RightDir)->AddAttributes(new WDefaultValueAttribute((int)WBasisAxis::NegativeX)),
    W_ENUM_MEMBER_PROPERTY("UpDir", WBasisAxis, m_UpDir)->AddAttributes(new WDefaultValueAttribute((int)WBasisAxis::PositiveY)),
    W_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    W_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    W_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("IsConvexMesh", m_bIsConvexMesh)->AddAttributes(new WHiddenAttribute()),
    W_ENUM_MEMBER_PROPERTY("ConvexMeshType", WJoltConvexCollisionMeshType, m_ConvexMeshType),
    W_MEMBER_PROPERTY("MaxConvexPieces", m_uiMaxConvexPieces)->AddAttributes(new WDefaultValueAttribute(5)),
    W_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("Radius2", m_fRadius2)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("Detail", m_uiDetail)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(0, 32)),
    W_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new WFileBrowserAttribute("Select Mesh", WFileBrowserAttribute::Meshes), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("MeshIncludeTags", m_sMeshIncludeTags),
    W_MEMBER_PROPERTY("MeshExcludeTags", m_sMeshExcludeTags),
    W_ARRAY_MEMBER_PROPERTY("Surfaces", m_Slots)->AddAttributes(new WContainerAttribute(false, false, true)),
    W_MEMBER_PROPERTY("Surface", m_sConvexMeshSurface)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),

    W_MEMBER_PROPERTY("SimplifyMesh", m_bSimplifyMesh),
    W_MEMBER_PROPERTY("MeshSimplification", m_uiMeshSimplification)->AddAttributes(new WDefaultValueAttribute(50), new WClampValueAttribute(1, 100)),
    W_MEMBER_PROPERTY("MaxSimplificationError", m_uiMaxSimplificationError)->AddAttributes(new WDefaultValueAttribute(20), new WClampValueAttribute(1, 100)),
    W_MEMBER_PROPERTY("NormalWeight", m_fNormalWeight)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1000.0f)),
    W_MEMBER_PROPERTY("AggressiveSimplification", m_bAggressiveSimplification),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltCollisionMeshAssetProperties::WJoltCollisionMeshAssetProperties() = default;
WJoltCollisionMeshAssetProperties::~WJoltCollisionMeshAssetProperties() = default;

void WJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() != WGetStaticRTTI<WJoltCollisionMeshAssetProperties>())
    return;

  const bool bSimplify = e.m_pObject->GetTypeAccessor().GetValue("SimplifyMesh").ConvertTo<bool>();
  const bool isConvex = e.m_pObject->GetTypeAccessor().GetValue("IsConvexMesh").ConvertTo<bool>();
  const WInt64 meshType = e.m_pObject->GetTypeAccessor().GetValue("ConvexMeshType").ConvertTo<WInt64>();

  auto& props = *e.m_pPropertyStates;

  props["Radius"].m_Visibility = WPropertyUiState::Invisible;
  props["Radius2"].m_Visibility = WPropertyUiState::Invisible;
  props["Height"].m_Visibility = WPropertyUiState::Invisible;
  props["Detail"].m_Visibility = WPropertyUiState::Invisible;
  props["MeshFile"].m_Visibility = WPropertyUiState::Invisible;
  props["MeshIncludeTags"].m_Visibility = WPropertyUiState::Invisible;
  props["MeshExcludeTags"].m_Visibility = WPropertyUiState::Invisible;
  props["ConvexMeshType"].m_Visibility = WPropertyUiState::Invisible;
  props["MaxConvexPieces"].m_Visibility = WPropertyUiState::Invisible;
  props["MaxConvexPieces"].m_Visibility = WPropertyUiState::Invisible;
  props["Surfaces"].m_Visibility = WPropertyUiState::Invisible;
  props["SimplifyMesh"].m_Visibility = WPropertyUiState::Invisible;
  props["MeshSimplification"].m_Visibility = WPropertyUiState::Invisible;
  props["MaxSimplificationError"].m_Visibility = WPropertyUiState::Invisible;
  props["NormalWeight"].m_Visibility = WPropertyUiState::Invisible;
  props["AggressiveSimplification"].m_Visibility = WPropertyUiState::Invisible;
  props["Surface"].m_Visibility = WPropertyUiState::Invisible;

  const WInt64 importTransform = e.m_pObject->GetTypeAccessor().GetValue("ImportTransform").ConvertTo<WInt64>();
  const bool bCustomTransform = importTransform == 127;
  props["RightDir"].m_Visibility = bCustomTransform ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  props["UpDir"].m_Visibility = bCustomTransform ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  props["FlipForwardDir"].m_Visibility = bCustomTransform ? WPropertyUiState::Default : WPropertyUiState::Invisible;

  if (!isConvex)
  {
    props["MeshFile"].m_Visibility = WPropertyUiState::Default;
    props["MeshIncludeTags"].m_Visibility = WPropertyUiState::Default;
    props["MeshExcludeTags"].m_Visibility = WPropertyUiState::Default;
    props["Surfaces"].m_Visibility = WPropertyUiState::Default;
    props["SimplifyMesh"].m_Visibility = WPropertyUiState::Default;

    props["MeshSimplification"].m_Visibility = bSimplify ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MaxSimplificationError"].m_Visibility = bSimplify ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["NormalWeight"].m_Visibility = bSimplify ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["AggressiveSimplification"].m_Visibility = bSimplify ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
  else
  {
    props["ConvexMeshType"].m_Visibility = WPropertyUiState::Default;

    switch (meshType)
    {
      case WJoltConvexCollisionMeshType::ConvexHull:
        props["MeshFile"].m_Visibility = WPropertyUiState::Default;
        props["MeshIncludeTags"].m_Visibility = WPropertyUiState::Default;
        props["MeshExcludeTags"].m_Visibility = WPropertyUiState::Default;
        props["Surface"].m_Visibility = WPropertyUiState::Default;
        break;

      case WJoltConvexCollisionMeshType::ConvexDecomposition:
        props["MeshFile"].m_Visibility = WPropertyUiState::Default;
        props["MeshIncludeTags"].m_Visibility = WPropertyUiState::Default;
        props["MeshExcludeTags"].m_Visibility = WPropertyUiState::Default;
        props["Surface"].m_Visibility = WPropertyUiState::Default;
        props["MaxConvexPieces"].m_Visibility = WPropertyUiState::Default;
        break;

      case WJoltConvexCollisionMeshType::ConvexHullGroup:
        props["MeshFile"].m_Visibility = WPropertyUiState::Default;
        props["MeshIncludeTags"].m_Visibility = WPropertyUiState::Default;
        props["MeshExcludeTags"].m_Visibility = WPropertyUiState::Default;
        props["Surfaces"].m_Visibility = WPropertyUiState::Default;
        break;

      case WJoltConvexCollisionMeshType::Cylinder:
        props["Radius"].m_Visibility = WPropertyUiState::Default;
        props["Radius2"].m_Visibility = WPropertyUiState::Default;
        props["Height"].m_Visibility = WPropertyUiState::Default;
        props["Detail"].m_Visibility = WPropertyUiState::Default;
        props["Surface"].m_Visibility = WPropertyUiState::Default;
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WJoltCollisionMeshAssetProperties_1_2 : public WGraphPatch
{
public:
  WJoltCollisionMeshAssetProperties_1_2()
    : WGraphPatch("WJoltCollisionMeshAssetProperties", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("ImportTransform", 127);
  }
};

WJoltCollisionMeshAssetProperties_1_2 g_WJoltCollisionMeshAssetProperties_1_2;
