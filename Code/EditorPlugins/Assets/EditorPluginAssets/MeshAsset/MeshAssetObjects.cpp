#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshAssetProperties, 6, WRTTIDefaultAllocator<WMeshAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("PrimitiveType", WMeshPrimitive, m_PrimitiveType),
    W_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new WFileBrowserAttribute("Select Mesh", WFileBrowserAttribute::Meshes)),
    W_MEMBER_PROPERTY("MeshIncludeTags", m_sMeshIncludeTags),
    W_MEMBER_PROPERTY("MeshExcludeTags", m_sMeshExcludeTags)->AddAttributes(new WDefaultValueAttribute("$;UCX_")),
    W_ENUM_MEMBER_PROPERTY("ImportTransform", WMeshImportTransform, m_ImportTransform),
    W_ENUM_MEMBER_PROPERTY("RightDir", WBasisAxis, m_RightDir)->AddAttributes(new WDefaultValueAttribute((int)WBasisAxis::NegativeX)),
    W_ENUM_MEMBER_PROPERTY("UpDir", WBasisAxis, m_UpDir)->AddAttributes(new WDefaultValueAttribute((int)WBasisAxis::PositiveY)),
    W_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    W_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    W_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0001f, 10000.0f)),
    W_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals),
    W_MEMBER_PROPERTY("RecalculateTangents", m_bRecalculateTangents)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("HighPrecision", m_bHighPrecision),
    W_ENUM_MEMBER_PROPERTY("VertexColorConversion", WMeshVertexColorConversion, m_VertexColorConversion),
    W_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials),
    W_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("Radius2", m_fRadius2)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("Detail", m_uiDetail)->AddAttributes(new WDefaultValueAttribute(0), new WClampValueAttribute(0, 128)),
    W_MEMBER_PROPERTY("Detail2", m_uiDetail2)->AddAttributes(new WDefaultValueAttribute(0), new WClampValueAttribute(0, 128)),
    W_MEMBER_PROPERTY("Cap", m_bCap)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("Cap2", m_bCap2)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(360.0f)), new WClampValueAttribute(WAngle::MakeFromDegree(0.0f), WAngle::MakeFromDegree(360.0f))),
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

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WMeshPrimitive, 1)
  W_ENUM_CONSTANT(WMeshPrimitive::File),
  W_ENUM_CONSTANT(WMeshPrimitive::Box),
  W_ENUM_CONSTANT(WMeshPrimitive::Rect),
  W_ENUM_CONSTANT(WMeshPrimitive::Cylinder),
  W_ENUM_CONSTANT(WMeshPrimitive::Cone),
  W_ENUM_CONSTANT(WMeshPrimitive::Pyramid),
  W_ENUM_CONSTANT(WMeshPrimitive::Sphere),
  W_ENUM_CONSTANT(WMeshPrimitive::HalfSphere),
  W_ENUM_CONSTANT(WMeshPrimitive::GeodesicSphere),
  W_ENUM_CONSTANT(WMeshPrimitive::Capsule),
  W_ENUM_CONSTANT(WMeshPrimitive::Torus),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WMeshAssetProperties::WMeshAssetProperties() = default;
WMeshAssetProperties::~WMeshAssetProperties() = default;


void WMeshAssetProperties::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WMeshAssetProperties>())
  {
    const WInt64 primType = e.m_pObject->GetTypeAccessor().GetValue("PrimitiveType").ConvertTo<WInt64>();
    const bool bSimplify = e.m_pObject->GetTypeAccessor().GetValue("SimplifyMesh").ConvertTo<bool>();

    auto& props = *e.m_pPropertyStates;

    props["MeshFile"].m_Visibility = WPropertyUiState::Invisible;
    props["MeshIncludeTags"].m_Visibility = WPropertyUiState::Invisible;
    props["MeshExcludeTags"].m_Visibility = WPropertyUiState::Invisible;
    props["Radius"].m_Visibility = WPropertyUiState::Invisible;
    props["Radius2"].m_Visibility = WPropertyUiState::Invisible;
    props["Height"].m_Visibility = WPropertyUiState::Invisible;
    props["Detail"].m_Visibility = WPropertyUiState::Invisible;
    props["Detail2"].m_Visibility = WPropertyUiState::Invisible;
    props["Cap"].m_Visibility = WPropertyUiState::Invisible;
    props["Cap2"].m_Visibility = WPropertyUiState::Invisible;
    props["Angle"].m_Visibility = WPropertyUiState::Invisible;
    props["ImportMaterials"].m_Visibility = WPropertyUiState::Invisible;
    props["RecalculateNormals"].m_Visibility = WPropertyUiState::Invisible;
    props["RecalculateTangents"].m_Visibility = WPropertyUiState::Invisible;
    props["HighPrecision"].m_Visibility = WPropertyUiState::Invisible;
    props["VertexColorConversion"].m_Visibility = WPropertyUiState::Invisible;

    props["MeshSimplification"].m_Visibility = bSimplify ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MaxSimplificationError"].m_Visibility = bSimplify ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["NormalWeight"].m_Visibility = bSimplify ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["AggressiveSimplification"].m_Visibility = bSimplify ? WPropertyUiState::Default : WPropertyUiState::Invisible;

    const WInt64 importTransform = e.m_pObject->GetTypeAccessor().GetValue("ImportTransform").ConvertTo<WInt64>();
    const bool bCustomTransform = importTransform == 127;
    props["RightDir"].m_Visibility = bCustomTransform ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["UpDir"].m_Visibility = bCustomTransform ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["FlipForwardDir"].m_Visibility = bCustomTransform ? WPropertyUiState::Default : WPropertyUiState::Invisible;

    switch (primType)
    {
      case WMeshPrimitive::File:
        props["MeshFile"].m_Visibility = WPropertyUiState::Default;
        props["MeshIncludeTags"].m_Visibility = WPropertyUiState::Default;
        props["MeshExcludeTags"].m_Visibility = WPropertyUiState::Default;
        props["ImportMaterials"].m_Visibility = WPropertyUiState::Default;
        props["RecalculateNormals"].m_Visibility = WPropertyUiState::Default;
        props["RecalculateTangents"].m_Visibility = WPropertyUiState::Default;
        props["HighPrecision"].m_Visibility = WPropertyUiState::Default;
        props["VertexColorConversion"].m_Visibility = WPropertyUiState::Default;
        break;

      case WMeshPrimitive::Box:
        break;

      case WMeshPrimitive::Rect:
        props["Detail"].m_Visibility = WPropertyUiState::Default;
        props["Detail2"].m_Visibility = WPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Rect.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Rect.Detail2";
        break;

      case WMeshPrimitive::Capsule:
        props["Radius"].m_Visibility = WPropertyUiState::Default;
        props["Height"].m_Visibility = WPropertyUiState::Default;
        props["Detail"].m_Visibility = WPropertyUiState::Default;
        props["Detail2"].m_Visibility = WPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Sphere.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Sphere.Detail2";
        break;

      case WMeshPrimitive::Cone:
        props["Radius"].m_Visibility = WPropertyUiState::Default;
        props["Height"].m_Visibility = WPropertyUiState::Default;
        props["Detail"].m_Visibility = WPropertyUiState::Default;
        props["Cap"].m_Visibility = WPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Cylinder.Detail";
        break;

      case WMeshPrimitive::Cylinder:
        props["Radius"].m_Visibility = WPropertyUiState::Default;
        props["Radius2"].m_Visibility = WPropertyUiState::Default;
        props["Height"].m_Visibility = WPropertyUiState::Default;
        props["Detail"].m_Visibility = WPropertyUiState::Default;
        props["Cap"].m_Visibility = WPropertyUiState::Default;
        props["Cap2"].m_Visibility = WPropertyUiState::Default;
        props["Angle"].m_Visibility = WPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Cylinder.Detail";
        props["Radius"].m_sNewLabelText = "Prim.Cylinder.Radius1";
        props["Radius2"].m_sNewLabelText = "Prim.Cylinder.Radius2";
        props["Angle"].m_sNewLabelText = "Prim.Cylinder.Angle";
        props["Cap"].m_sNewLabelText = "Prim.Cylinder.Cap1";
        props["Cap2"].m_sNewLabelText = "Prim.Cylinder.Cap2";
        break;

      case WMeshPrimitive::GeodesicSphere:
        props["Radius"].m_Visibility = WPropertyUiState::Default;
        props["Detail"].m_Visibility = WPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.GeoSphere.Detail";
        break;

      case WMeshPrimitive::HalfSphere:
        props["Radius"].m_Visibility = WPropertyUiState::Default;
        props["Detail"].m_Visibility = WPropertyUiState::Default;
        props["Detail2"].m_Visibility = WPropertyUiState::Default;
        props["Cap"].m_Visibility = WPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Sphere.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Sphere.Detail2";
        break;

      case WMeshPrimitive::Pyramid:
        props["Cap"].m_Visibility = WPropertyUiState::Default;
        break;

      case WMeshPrimitive::Sphere:
        props["Radius"].m_Visibility = WPropertyUiState::Default;
        props["Detail"].m_Visibility = WPropertyUiState::Default;
        props["Detail2"].m_Visibility = WPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Sphere.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Sphere.Detail2";
        break;

      case WMeshPrimitive::Torus:
        props["Radius"].m_Visibility = WPropertyUiState::Default;
        props["Radius2"].m_Visibility = WPropertyUiState::Default;
        props["Detail"].m_Visibility = WPropertyUiState::Default;
        props["Detail2"].m_Visibility = WPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Torus.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Torus.Detail2";
        props["Radius"].m_sNewLabelText = "Prim.Torus.Radius1";
        props["Radius2"].m_sNewLabelText = "Prim.Torus.Radius2";
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

class WMeshAssetPropertiesPatch_1_2 : public WGraphPatch
{
public:
  WMeshAssetPropertiesPatch_1_2()
    : WGraphPatch("WMeshAssetProperties", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Primitive Type", "PrimitiveType");
    pNode->RenameProperty("Forward Dir", "ForwardDir");
    pNode->RenameProperty("Right Dir", "RightDir");
    pNode->RenameProperty("Up Dir", "UpDir");
    pNode->RenameProperty("Uniform Scaling", "UniformScaling");
    pNode->RenameProperty("Non-Uniform Scaling", "NonUniformScaling");
    pNode->RenameProperty("Mesh File", "MeshFile");
    pNode->RenameProperty("Radius 2", "Radius2");
    pNode->RenameProperty("Detail 2", "Detail2");
    pNode->RenameProperty("Cap 2", "Cap2");
    pNode->RenameProperty("Import Materials", "ImportMaterials");
  }
};

WMeshAssetPropertiesPatch_1_2 g_MeshAssetPropertiesPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class WMeshAssetPropertiesPatch_2_3 : public WGraphPatch
{
public:
  WMeshAssetPropertiesPatch_2_3()
    : WGraphPatch("WMeshAssetProperties", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // convert the "Angle" property from float to WAngle
    if (auto pProp = pNode->FindProperty("Angle"))
    {
      if (pProp->m_Value.IsA<float>())
      {
        const float valFloat = pProp->m_Value.Get<float>();
        pProp->m_Value = WAngle::MakeFromDegree(valFloat);
      }
    }
  }
};

WMeshAssetPropertiesPatch_2_3 g_WMeshAssetPropertiesPatch_2_3;

//////////////////////////////////////////////////////////////////////////

class WMeshAssetPropertiesPatch_3_4 : public WGraphPatch
{
public:
  WMeshAssetPropertiesPatch_3_4()
    : WGraphPatch("WMeshAssetProperties", 4)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("ImportTransform", 127);
  }
};

WMeshAssetPropertiesPatch_3_4 g_WMeshAssetPropertiesPatch_3_4;

//////////////////////////////////////////////////////////////////////////

class WMeshAssetPropertiesPatch_4_5 : public WGraphPatch
{
public:
  WMeshAssetPropertiesPatch_4_5()
    : WGraphPatch("WMeshAssetProperties", 5)
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

    pNode->AddProperty("HighPrecision", bHighPrecision);
  }
};

WMeshAssetPropertiesPatch_4_5 g_WMeshAssetPropertiesPatch_4_5;
