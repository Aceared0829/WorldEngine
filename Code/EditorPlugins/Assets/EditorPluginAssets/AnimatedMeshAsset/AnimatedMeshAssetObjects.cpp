#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimatedMeshAssetProperties, 4, WRTTIDefaultAllocator<WAnimatedMeshAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new WFileBrowserAttribute("Select Mesh", WFileBrowserAttribute::MeshesWithAnimations), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("MeshIncludeTags", m_sMeshIncludeTags),
    W_MEMBER_PROPERTY("MeshExcludeTags", m_sMeshExcludeTags)->AddAttributes(new WDefaultValueAttribute("$;UCX_")),
    W_MEMBER_PROPERTY("DefaultSkeleton", m_sDefaultSkeleton)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton"), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals),
    W_MEMBER_PROPERTY("RecalculateTangents", m_bRecalculateTangents)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("HighPrecision", m_bHighPrecision),
    W_ENUM_MEMBER_PROPERTY("VertexColorConversion", WMeshVertexColorConversion, m_VertexColorConversion),
    W_MEMBER_PROPERTY("NormalizeWeights", m_bNormalizeWeights)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials),
    W_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new WContainerAttribute(false, true, true)),
    W_MEMBER_PROPERTY("SimplifyMesh", m_bSimplifyMesh),
    W_MEMBER_PROPERTY("MeshSimplification", m_uiMeshSimplification)->AddAttributes(new WDefaultValueAttribute(50), new WClampValueAttribute(1, 100)),
    W_MEMBER_PROPERTY("MaxSimplificationError", m_uiMaxSimplificationError)->AddAttributes(new WDefaultValueAttribute(5), new WClampValueAttribute(1, 100)),
    W_MEMBER_PROPERTY("NormalWeight", m_fNormalWeight)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1000.0f)),
    W_MEMBER_PROPERTY("AggressiveSimplification", m_bAggressiveSimplification),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAnimatedMeshAssetProperties::WAnimatedMeshAssetProperties() = default;
WAnimatedMeshAssetProperties::~WAnimatedMeshAssetProperties() = default;

void WAnimatedMeshAssetProperties::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WAnimatedMeshAssetProperties>())
  {
    const bool bSimplify = e.m_pObject->GetTypeAccessor().GetValue("SimplifyMesh").ConvertTo<bool>();

    auto& props = *e.m_pPropertyStates;

    props["MeshSimplification"].m_Visibility = bSimplify ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MaxSimplificationError"].m_Visibility = bSimplify ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["NormalWeight"].m_Visibility = bSimplify ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["AggressiveSimplification"].m_Visibility = bSimplify ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
}

//////////////////////////////////////////////////////////////////////////

class WAnimatedMeshAssetPropertiesPatch_2_3 : public WGraphPatch
{
public:
  WAnimatedMeshAssetPropertiesPatch_2_3()
    : WGraphPatch("WAnimatedMeshAssetProperties", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    bool bHighPrecision = false;

    if (auto pProp = pNode->FindProperty("NormalPrecision"))
    {
      if (pProp->m_Value.IsA<WString>() && pProp->m_Value.Get<WString>() != "WMeshNormalPrecision::_10Bit")
      {
        bHighPrecision = true;
      }
    }

    if (auto pProp = pNode->FindProperty("TexCoordPrecision"))
    {
      if (pProp->m_Value.IsA<WString>() && pProp->m_Value.Get<WString>() != "WMeshTexCoordPrecision::_16Bit")
      {
        bHighPrecision = true;
      }
    }

    if (auto pProp = pNode->FindProperty("BoneWeightPrecision"))
    {
      if (pProp->m_Value.IsA<WString>() && pProp->m_Value.Get<WString>() != "WMeshBoneWeigthPrecision::_8Bit")
      {
        bHighPrecision = true;
      }
    }

    pNode->AddProperty("HighPrecision", bHighPrecision);
  }
};

WAnimatedMeshAssetPropertiesPatch_2_3 g_WAnimatedMeshAssetPropertiesPatch_2_3;
