#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <KrautGenerator/Description/SpawnNodeDesc.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Material/MaterialResource.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WKrautAssetMaterial, WNoBase, 1, WRTTIDefaultAllocator<WKrautAssetMaterial>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new WReadOnlyAttribute()),
    W_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material", "Kraut"), new WRequiredAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WKrautBranchTypeMode, 1)
  W_ENUM_CONSTANTS(WKrautBranchTypeMode::Regular, WKrautBranchTypeMode::Umbrella)
W_END_STATIC_REFLECTED_ENUM

W_BEGIN_STATIC_REFLECTED_ENUM(WKrautBranchTargetDir, 1)
  W_ENUM_CONSTANT(WKrautBranchTargetDir::Straight),
  W_ENUM_CONSTANT(WKrautBranchTargetDir::Upwards),
  W_ENUM_CONSTANT(WKrautBranchTargetDir::Degree22),
  W_ENUM_CONSTANT(WKrautBranchTargetDir::Degree45),
  W_ENUM_CONSTANT(WKrautBranchTargetDir::Degree67),
  W_ENUM_CONSTANT(WKrautBranchTargetDir::Degree90),
  W_ENUM_CONSTANT(WKrautBranchTargetDir::Degree112),
  W_ENUM_CONSTANT(WKrautBranchTargetDir::Degree135),
  W_ENUM_CONSTANT(WKrautBranchTargetDir::Degree157),
  W_ENUM_CONSTANT(WKrautBranchTargetDir::Downwards),
W_END_STATIC_REFLECTED_ENUM

W_BEGIN_STATIC_REFLECTED_ENUM(WKrautLeafOrientation, 1)
  W_ENUM_CONSTANT(WKrautLeafOrientation::Upwards),
  W_ENUM_CONSTANT(WKrautLeafOrientation::AlongBranch),
  W_ENUM_CONSTANT(WKrautLeafOrientation::OrthogonalToBranch),
W_END_STATIC_REFLECTED_ENUM

W_BEGIN_STATIC_REFLECTED_ENUM(WKrautFrondContourMode, 1)
  W_ENUM_CONSTANT(WKrautFrondContourMode::Off),
  W_ENUM_CONSTANT(WKrautFrondContourMode::Full),
  W_ENUM_CONSTANT(WKrautFrondContourMode::Symetric),
  W_ENUM_CONSTANT(WKrautFrondContourMode::InverseSymetric),
W_END_STATIC_REFLECTED_ENUM

W_BEGIN_STATIC_REFLECTED_ENUM(WKrautBranchTargetDir2Usage, 1)
  W_ENUM_CONSTANT(WKrautBranchTargetDir2Usage::Off),
  W_ENUM_CONSTANT(WKrautBranchTargetDir2Usage::Relative),
  W_ENUM_CONSTANT(WKrautBranchTargetDir2Usage::Absolute),
W_END_STATIC_REFLECTED_ENUM

W_BEGIN_STATIC_REFLECTED_ENUM(WKrautLodMode, 1)
  W_ENUM_CONSTANT(WKrautLodMode::Full),
  //W_ENUM_CONSTANT(WKrautLodMode::FourQuads),
  //W_ENUM_CONSTANT(WKrautLodMode::TwoQuads),
  //W_ENUM_CONSTANT(WKrautLodMode::Billboard),
  W_ENUM_CONSTANT(WKrautLodMode::Disabled),
W_END_STATIC_REFLECTED_ENUM

W_BEGIN_STATIC_REFLECTED_ENUM(WKrautBranchSpikeTipMode, 1)
  W_ENUM_CONSTANT(WKrautBranchSpikeTipMode::FullDetail),
  W_ENUM_CONSTANT(WKrautBranchSpikeTipMode::SingleTriangle),
  W_ENUM_CONSTANT(WKrautBranchSpikeTipMode::Hole),
W_END_STATIC_REFLECTED_ENUM


W_BEGIN_STATIC_REFLECTED_TYPE(WKrautAssetBranchType, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    // === Administrative ===

    W_MEMBER_PROPERTY("GrowSubBranchType1", m_bGrowSubBranchType1),
    W_MEMBER_PROPERTY("GrowSubBranchType2", m_bGrowSubBranchType2),
    W_MEMBER_PROPERTY("GrowSubBranchType3", m_bGrowSubBranchType3),

    // === Branch Type ===

    // General

    W_MEMBER_PROPERTY("SegmentLength", m_uiSegmentLengthCM)->AddAttributes(new WDefaultValueAttribute(5), new WClampValueAttribute(1, 50), new WSuffixAttribute("cm"), new WGroupAttribute("Branch Type: General Settings")),
    W_ENUM_MEMBER_PROPERTY("BranchType", WKrautBranchTypeMode, m_BranchTypeMode)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("BranchlessPartABS", m_fBranchlessPartABS)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0, 10.0f), new WSuffixAttribute("m")),
    W_MEMBER_PROPERTY("BranchlessPartEndABS", m_fBranchlessPartEndABS)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0, 10.0f), new WSuffixAttribute("m")),
    W_MEMBER_PROPERTY("LowerBound", m_uiLowerBound)->AddAttributes(new WDefaultValueAttribute(0), new WClampValueAttribute(0, 100), new WSuffixAttribute("%")),
    W_MEMBER_PROPERTY("UpperBound", m_uiUpperBound)->AddAttributes(new WDefaultValueAttribute(100), new WClampValueAttribute(0, 100), new WSuffixAttribute("%")),
    W_MEMBER_PROPERTY("MinBranchThickness", m_uiMinBranchThicknessInCM)->AddAttributes(new WDefaultValueAttribute(20), new WClampValueAttribute(1, 100), new WSuffixAttribute("cm")),
    W_MEMBER_PROPERTY("MaxBranchThickness", m_uiMaxBranchThicknessInCM)->AddAttributes(new WDefaultValueAttribute(20), new WClampValueAttribute(1, 100), new WSuffixAttribute("cm")),

    // Spawn Nodes

    W_MEMBER_PROPERTY("MinBranchesPerNode", m_uiMinBranches)->AddAttributes(new WDefaultValueAttribute(4), new WClampValueAttribute(0, 32), new WGroupAttribute("Branch Type: Spawn Nodes")),
    W_MEMBER_PROPERTY("MaxBranchesPerNode", m_uiMaxBranches)->AddAttributes(new WDefaultValueAttribute(4), new WClampValueAttribute(0, 32)),
    W_MEMBER_PROPERTY("NodeSpacingBefore", m_fNodeSpacingBefore)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0, 5.0f), new WSuffixAttribute("m")),
    W_MEMBER_PROPERTY("NodeSpacingAfter", m_fNodeSpacingAfter)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0, 5.0f), new WSuffixAttribute("m")),
    W_MEMBER_PROPERTY("NodeHeight", m_fNodeHeight)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0, 5.0f), new WSuffixAttribute("m")),

    // === Growth ===

    // Start Direction
    W_MEMBER_PROPERTY("MaxRotationalDeviation", m_MaxRotationalDeviation)->AddAttributes(new WDefaultValueAttribute(WAngle()), new WClampValueAttribute(WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(180)), new WGroupAttribute("Growth: Start Direction")),
    W_MEMBER_PROPERTY("BranchAngle", m_BranchAngle)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(90)), new WClampValueAttribute(WAngle::MakeFromDegree(1), WAngle::MakeFromDegree(179))),
    W_MEMBER_PROPERTY("MaxBranchAngleDeviation", m_MaxBranchAngleDeviation)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(10)), new WClampValueAttribute(WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(90))),

    // Target Direction
    W_ENUM_MEMBER_PROPERTY("TargetDirection", WKrautBranchTargetDir, m_TargetDirection)->AddAttributes(new WGroupAttribute("Growth: Target Direction")),
    W_MEMBER_PROPERTY("TargetDirRelative", m_bTargetDirRelative),
    W_MEMBER_PROPERTY("MaxTargetDirDeviation", m_MaxTargetDirDeviation)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(20)), new WClampValueAttribute(WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(90))),
    W_ENUM_MEMBER_PROPERTY("TargetDir2Usage", WKrautBranchTargetDir2Usage, m_TargetDir2Usage),
    W_MEMBER_PROPERTY("TargetDir2Offset", m_fTargetDir2Usage)->AddAttributes(new WDefaultValueAttribute(2.5f), new WClampValueAttribute(0.01f, 5.0f), new WSuffixAttribute("m")),
    W_ENUM_MEMBER_PROPERTY("TargetDirection2", WKrautBranchTargetDir, m_TargetDirection2),

    // Growth
    W_MEMBER_PROPERTY("MinBranchLength", m_uiMinBranchLengthInCM)->AddAttributes(new WDefaultValueAttribute(100), new WClampValueAttribute(1, 10000), new WSuffixAttribute("cm"), new WGroupAttribute("Growth: Branch Behavior")),
    W_MEMBER_PROPERTY("MaxBranchLength", m_uiMaxBranchLengthInCM)->AddAttributes(new WDefaultValueAttribute(100), new WClampValueAttribute(1, 10000), new WSuffixAttribute("cm")),
    W_MEMBER_PROPERTY("MaxBranchLengthParentScale", m_MaxBranchLengthParentScale)->AddAttributes(new WColorAttribute(WColor::Brown), new WCurveExtentsAttribute(0.0, true, 1.0f, true), new WClampValueAttribute(0.0, 1.0), new WDefaultValueAttribute(1.0)),
    W_MEMBER_PROPERTY("TargetDirDeviation", m_GrowMaxTargetDirDeviation)->AddAttributes(new WDefaultValueAttribute(WAngle()), new WClampValueAttribute(WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(180))),
    W_MEMBER_PROPERTY("DirChangePerSegment", m_GrowMaxDirChangePerSegment)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(5)), new WClampValueAttribute(WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(90))),
    W_MEMBER_PROPERTY("OnlyGrowUpAndDown", m_bRestrictGrowthToFrondPlane),

    // Obstacles

    //bool m_bActAsObstacle;
    //bool m_bDoPhysicalSimulation;
    //float m_fPhysicsLookAhead;
    //float m_fPhysicsEvasionAngle;

    // === Appearance ===

    // Branch Mesh

    W_MEMBER_PROPERTY("EnableMesh", m_bEnableMesh)->AddAttributes(new WDefaultValueAttribute(true), new WGroupAttribute("Appearance: Branch Mesh")),
    W_MEMBER_PROPERTY("BranchMaterial", m_sBranchMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material", "Kraut-Stem")),
    W_MEMBER_PROPERTY("BranchContour", m_BranchContour)->AddAttributes(new WColorAttribute(WColor::Brown), new WCurveExtentsAttribute(0.0f, true, 1.0f, true), new WClampValueAttribute(0.1, 1.0), new WDefaultValueAttribute(1.0)),
    W_MEMBER_PROPERTY("Roundness", m_fRoundnessFactor)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("Flares", m_uiFlares)->AddAttributes(new WDefaultValueAttribute(0), new WClampValueAttribute(0, 16)),
    W_MEMBER_PROPERTY("FlareWidth", m_fFlareWidth)->AddAttributes(new WDefaultValueAttribute(2.0f), new WClampValueAttribute(0.0f, 10.0f)),
    W_MEMBER_PROPERTY("FlareWidthCurve", m_FlareWidthCurve)->AddAttributes(new WColorAttribute(WColor::FloralWhite), new WCurveExtentsAttribute(0.0, true, 1.0, true), new WClampValueAttribute(0.0, 1.0), new WDefaultValueAttribute(1.0)),
    W_MEMBER_PROPERTY("FlareRotation", m_FlareRotation)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(0)), new WClampValueAttribute(WAngle::MakeFromDegree(-720), WAngle::MakeFromDegree(720))),
    W_MEMBER_PROPERTY("RotateTexCoords", m_bRotateTexCoords)->AddAttributes(new WDefaultValueAttribute(true)),

    // Fronds

    W_MEMBER_PROPERTY("EnableFronds", m_bEnableFronds)->AddAttributes(new WGroupAttribute("Appearance: Fronds")),
    W_MEMBER_PROPERTY("FrondMaterial", m_sFrondMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material", "Kraut-Frond")),
    W_MEMBER_PROPERTY("NumFronds", m_uiNumFronds)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(1, 16)),
    W_MEMBER_PROPERTY("FrondDetail", m_uiFrondDetail)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(0, 32)),
    W_MEMBER_PROPERTY("FrondWidth", m_fFrondWidth)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 10.0f), new WSuffixAttribute("m")),
    W_MEMBER_PROPERTY("FrondWidthScale", m_FrondWidth)->AddAttributes(new WColorAttribute(WColor::LawnGreen), new WCurveExtentsAttribute(0.0, true, 1.0, true), new WClampValueAttribute(0.0, 1.0), new WDefaultValueAttribute(1.0)),
    W_MEMBER_PROPERTY("TextureRepeat", m_fTextureRepeat)->AddAttributes(new WClampValueAttribute(0.0f, 99.0f)),
    W_ENUM_MEMBER_PROPERTY("FrondUpOrientation", WKrautLeafOrientation, m_FrondUpOrientation),
    W_MEMBER_PROPERTY("FrondOrientationDeviation", m_MaxFrondOrientationDeviation)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(180))),
    W_MEMBER_PROPERTY("AlignFrondsOnSurface", m_bAlignFrondsOnSurface),
    W_ENUM_MEMBER_PROPERTY("FrondContourMode", WKrautFrondContourMode, m_FrondContourMode),
    W_MEMBER_PROPERTY("FrondContour", m_FrondContour)->AddAttributes(new WColorAttribute(WColor::LawnGreen), new WCurveExtentsAttribute(0.0, true, 1.0, true), new WClampValueAttribute(-1.0, 1.0), new WDefaultValueAttribute(1.0)),
    W_MEMBER_PROPERTY("FrondHeight", m_fFrondHeight)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(-10.0f, 10.0f), new WSuffixAttribute("m")),
    W_MEMBER_PROPERTY("FrondHeightScale", m_FrondHeight)->AddAttributes(new WColorAttribute(WColor::LawnGreen), new WCurveExtentsAttribute(0.0, true, 1.0, true), new WClampValueAttribute(-1.0, 1.0)),
    //W_MEMBER_PROPERTY("FrondVariationColor", m_FrondVariationColor)->AddAttributes(new WDefaultValueAttribute(WColor::White)),

    // Leaves

    W_MEMBER_PROPERTY("EnableLeaves", m_bEnableLeaves)->AddAttributes(new WGroupAttribute("Appearance: Leaves")),
    W_MEMBER_PROPERTY("LeafMaterial", m_sLeafMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material", "Kraut-Leaf")),
    W_MEMBER_PROPERTY("BillboardLeaves", m_bBillboardLeaves)->AddAttributes(new WDefaultValueAttribute(true), new WHiddenAttribute()),
    W_MEMBER_PROPERTY("LeafSize", m_fLeafSize)->AddAttributes(new WDefaultValueAttribute(0.25f), new WClampValueAttribute(0.01f, 10.0f), new WSuffixAttribute("m")),
    W_MEMBER_PROPERTY("LeafScale", m_LeafScale)->AddAttributes(new WColorAttribute(WColor::LightGreen), new WCurveExtentsAttribute(0.0, true, 1.0, true), new WClampValueAttribute(0.0, 1.0), new WDefaultValueAttribute(1.0)),
    W_MEMBER_PROPERTY("LeafInterval", m_fLeafInterval)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, 10.0f), new WSuffixAttribute("m")),
    //W_MEMBER_PROPERTY("LeafVariationColor", m_LeafVariationColor)->AddAttributes(new WDefaultValueAttribute(WColor::White)),

    // Shared

    //WUInt8 m_uiTextureTilingX[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
    //WUInt8 m_uiTextureTilingY[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WKrautAssetLod, WNoBase, 1, WRTTIDefaultAllocator<WKrautAssetLod>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Mode", WKrautLodMode, m_Mode)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("TipDetail", m_fTipDetail)->AddAttributes(new WDefaultValueAttribute(0.04f), new WClampValueAttribute(0.01f, 1.0f), new WSuffixAttribute("m")),
    W_MEMBER_PROPERTY("CurvatureThreshold", m_fCurvatureThreshold)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.0f, 90.0f)),
    W_MEMBER_PROPERTY("ThicknessThreshold", m_fThicknessThreshold)->AddAttributes(new WDefaultValueAttribute(0.2f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("VertexRingDetail", m_fVertexRingDetail)->AddAttributes(new WDefaultValueAttribute(0.2f), new WClampValueAttribute(0.01f, 2.0f), new WSuffixAttribute("m")),
    W_BITFLAGS_MEMBER_PROPERTY("AllowBranch", WKrautTreeTypeBits, m_AllowBranch),
    W_BITFLAGS_MEMBER_PROPERTY("AllowFrond", WKrautTreeTypeBits, m_AllowFrond),
    W_BITFLAGS_MEMBER_PROPERTY("AllowLeaf", WKrautTreeTypeBits, m_AllowLeaf),
    W_MEMBER_PROPERTY("MaxFrondDetail", m_iMaxFrondDetail)->AddAttributes(new WDefaultValueAttribute(32), new WClampValueAttribute(0, 64)),
    W_MEMBER_PROPERTY("FrondDetailReduction", m_iFrondDetailReduction)->AddAttributes(new WDefaultValueAttribute(0), new WClampValueAttribute(0, 32)),
    W_MEMBER_PROPERTY("LodDistance", m_uiLodDistance)->AddAttributes(new WDefaultValueAttribute(0), new WClampValueAttribute(0, 1000), new WSuffixAttribute("m")),
    W_ENUM_MEMBER_PROPERTY("BranchSpikeTipMode", WKrautBranchSpikeTipMode, m_BranchSpikeTipMode)->AddAttributes(new WHiddenAttribute()), // effect too rarely useful, should be improved
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WKrautTreeAssetProperties, 1, WRTTIDefaultAllocator<WKrautTreeAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    //W_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    //W_MEMBER_PROPERTY("LodDistanceScale", m_fLodDistanceScale)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("MinAmbientOcclusion", m_fMinAmbientOcclusion)->AddAttributes(new WDefaultValueAttribute(0.7f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("StaticColliderRadius", m_fStaticColliderRadius)->AddAttributes(new WDefaultValueAttribute(0.4f), new WClampValueAttribute(0.0f, 10.0f), new WSuffixAttribute("m")),
    W_MEMBER_PROPERTY("TreeStiffness", m_fTreeStiffness)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(1.0f, 10000.0f)),
    W_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::Package)),
    W_ARRAY_MEMBER_PROPERTY("Materials", m_Materials)->AddAttributes(new WContainerAttribute(false, false, false)),
    W_MEMBER_PROPERTY("DisplayRandomSeed", m_uiRandomSeedForDisplay),
    W_ARRAY_MEMBER_PROPERTY("GoodRandomSeeds", m_GoodRandomSeeds),
    W_MEMBER_PROPERTY("BT_Trunk1", m_BT_Trunk1),
    //W_MEMBER_PROPERTY("BT_Trunk2", m_BT_Trunk2),
    //W_MEMBER_PROPERTY("BT_Trunk3", m_BT_Trunk3),
    W_MEMBER_PROPERTY("BT_MainBranch1", m_BT_MainBranch1),
    W_MEMBER_PROPERTY("BT_MainBranch2", m_BT_MainBranch2),
    W_MEMBER_PROPERTY("BT_MainBranch3", m_BT_MainBranch3),
    W_MEMBER_PROPERTY("BT_SubBranch1", m_BT_SubBranch1),
    W_MEMBER_PROPERTY("BT_SubBranch2", m_BT_SubBranch2),
    W_MEMBER_PROPERTY("BT_SubBranch3", m_BT_SubBranch3),
    W_MEMBER_PROPERTY("BT_Twig1", m_BT_Twig1),
    W_MEMBER_PROPERTY("BT_Twig2", m_BT_Twig2),
    W_MEMBER_PROPERTY("BT_Twig3", m_BT_Twig3),
    W_MEMBER_PROPERTY("LOD0", m_Lod0),
    W_MEMBER_PROPERTY("LOD1", m_Lod1),
    W_MEMBER_PROPERTY("LOD2", m_Lod2),
    W_MEMBER_PROPERTY("LOD3", m_Lod3),
    W_MEMBER_PROPERTY("LOD4", m_Lod4),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WKrautTreeAssetProperties::WKrautTreeAssetProperties()
{
  // LOD 0 is enabled by default, others are disabled
  m_Lod1.m_Mode = WKrautLodMode::Disabled;
  m_Lod2.m_Mode = WKrautLodMode::Disabled;
  m_Lod3.m_Mode = WKrautLodMode::Disabled;
  m_Lod4.m_Mode = WKrautLodMode::Disabled;
}

WKrautTreeAssetProperties::~WKrautTreeAssetProperties() = default;

void WKrautTreeAssetProperties::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WKrautAssetLod>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool bDisabled = e.m_pObject->GetTypeAccessor().GetValue("Mode").ConvertTo<WInt32>() == WKrautLodMode::Disabled;
    const WPropertyUiState::Visibility lodVis = bDisabled ? WPropertyUiState::Disabled : WPropertyUiState::Default;

    props["TipDetail"].m_Visibility = lodVis;
    props["CurvatureThreshold"].m_Visibility = lodVis;
    props["ThicknessThreshold"].m_Visibility = lodVis;
    props["VertexRingDetail"].m_Visibility = lodVis;
    props["AllowBranch"].m_Visibility = lodVis;
    props["AllowFrond"].m_Visibility = lodVis;
    props["AllowLeaf"].m_Visibility = lodVis;
    props["MaxFrondDetail"].m_Visibility = lodVis;
    props["FrondDetailReduction"].m_Visibility = lodVis;
    props["LodDistance"].m_Visibility = lodVis;
    props["BranchSpikeTipMode"].m_Visibility = lodVis;
  }

  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WKrautAssetBranchType>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool bEnableMesh = e.m_pObject->GetTypeAccessor().GetValue("EnableMesh").ConvertTo<bool>();
    const bool bEnableFronds = e.m_pObject->GetTypeAccessor().GetValue("EnableFronds").ConvertTo<bool>();
    const bool bEnableLeaves = e.m_pObject->GetTypeAccessor().GetValue("EnableLeaves").ConvertTo<bool>();
    const bool bHasFlares = e.m_pObject->GetTypeAccessor().GetValue("Flares").ConvertTo<WUInt32>() > 0;
    const bool bFrondContourOff = e.m_pObject->GetTypeAccessor().GetValue("FrondContourMode").ConvertTo<WInt32>() == WKrautFrondContourMode::Off;
    const bool bIsTrunk = e.m_pObject->GetParentProperty().StartsWith("BT_Trunk");
    const bool bTargetDir2Off = e.m_pObject->GetTypeAccessor().GetValue("TargetDir2Usage").ConvertTo<WInt32>() == WKrautBranchTargetDir2Usage::Off;

    // Branch Mesh properties
    props["BranchMaterial"].m_Visibility = bEnableMesh ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["BranchContour"].m_Visibility = bEnableMesh ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["Roundness"].m_Visibility = bEnableMesh ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["Flares"].m_Visibility = bEnableMesh ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["FlareWidth"].m_Visibility = (bEnableMesh && bHasFlares) ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["FlareWidthCurve"].m_Visibility = (bEnableMesh && bHasFlares) ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["FlareRotation"].m_Visibility = (bEnableMesh && bHasFlares) ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["RotateTexCoords"].m_Visibility = (bEnableMesh && bHasFlares) ? WPropertyUiState::Default : WPropertyUiState::Disabled;

    props["MaxBranchLengthParentScale"].m_Visibility = bIsTrunk ? WPropertyUiState::Disabled : WPropertyUiState::Default;
    props["LowerBound"].m_Visibility = bIsTrunk ? WPropertyUiState::Disabled : WPropertyUiState::Default;
    props["UpperBound"].m_Visibility = bIsTrunk ? WPropertyUiState::Disabled : WPropertyUiState::Default;
    props["TargetDir2Offset"].m_Visibility = bTargetDir2Off ? WPropertyUiState::Disabled : WPropertyUiState::Default;
    props["TargetDirection2"].m_Visibility = bTargetDir2Off ? WPropertyUiState::Disabled : WPropertyUiState::Default;

    // Frond properties
    props["FrondMaterial"].m_Visibility = bEnableFronds ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["TextureRepeat"].m_Visibility = bEnableFronds ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["FrondUpOrientation"].m_Visibility = bEnableFronds ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["FrondOrientationDeviation"].m_Visibility = bEnableFronds ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["NumFronds"].m_Visibility = bEnableFronds ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["AlignFrondsOnSurface"].m_Visibility = bEnableFronds ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["FrondDetail"].m_Visibility = bEnableFronds ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["FrondContourMode"].m_Visibility = bEnableFronds ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["FrondContour"].m_Visibility = (bEnableFronds && !bFrondContourOff) ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["FrondHeight"].m_Visibility = (bEnableFronds && !bFrondContourOff) ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["FrondHeightScale"].m_Visibility = (bEnableFronds && !bFrondContourOff) ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["FrondWidth"].m_Visibility = bEnableFronds ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["FrondWidthScale"].m_Visibility = bEnableFronds ? WPropertyUiState::Default : WPropertyUiState::Disabled;

    // Leaf properties
    props["LeafMaterial"].m_Visibility = bEnableLeaves ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["BillboardLeaves"].m_Visibility = bEnableLeaves ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["LeafSize"].m_Visibility = bEnableLeaves ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["LeafScale"].m_Visibility = bEnableLeaves ? WPropertyUiState::Default : WPropertyUiState::Disabled;
    props["LeafInterval"].m_Visibility = bEnableLeaves ? WPropertyUiState::Default : WPropertyUiState::Disabled;

    WStringBuilder tmp1, tmp2, tmp3;

    // Sub-branch type labels: reflect the names of the branch types that would grow from this one
    const WStringView sParentProp = e.m_pObject->GetParentProperty();
    if (bIsTrunk)
    {
      tmp1.Set("Grow ", WKrautBranchTypeNames::MainBranch1);
      tmp2.Set("Grow ", WKrautBranchTypeNames::MainBranch2);
      tmp3.Set("Grow ", WKrautBranchTypeNames::MainBranch3);
    }
    else if (sParentProp.StartsWith("BT_MainBranch"))
    {
      tmp1.Set("Grow ", WKrautBranchTypeNames::SubBranch1);
      tmp2.Set("Grow ", WKrautBranchTypeNames::SubBranch2);
      tmp3.Set("Grow ", WKrautBranchTypeNames::SubBranch3);
    }
    else if (sParentProp.StartsWith("BT_SubBranch"))
    {
      tmp1.Set("Grow ", WKrautBranchTypeNames::Twig1);
      tmp2.Set("Grow ", WKrautBranchTypeNames::Twig2);
      tmp3.Set("Grow ", WKrautBranchTypeNames::Twig3);
    }

    if (!tmp1.IsEmpty())
    {
      props["GrowSubBranchType1"].m_sNewLabelText = tmp1;
      props["GrowSubBranchType2"].m_sNewLabelText = tmp2;
      props["GrowSubBranchType3"].m_sNewLabelText = tmp3;
    }
  }
}

static void CopyKrautCurve(Kraut::Curve& ref_dst, const WSingleCurveData& src, WUInt32 uiNumSamples, float fDefaultValue)
{
  if (src.m_ControlPoints.IsEmpty())
  {
    ref_dst.Initialize(1, fDefaultValue, 0.0f, 1.0f);
  }
  else
  {
    WCurve1D c;
    src.ConvertToRuntimeData(c);
    c.SortControlPoints();
    c.CreateLinearApproximation();

    ref_dst.Initialize(uiNumSamples, fDefaultValue, 0.0f, 1.0f);

    const double invSam = 1.0 / (uiNumSamples - 1);
    for (WUInt32 i = 0; i < uiNumSamples; ++i)
    {
      const double val = c.Evaluate(i * invSam);
      ref_dst.m_Values[i] = (float)val;
    }
  }
}

void CopyKrautConfig(Kraut::SpawnNodeDesc& ref_node, const WKrautAssetBranchType& bt, WDynamicArray<WKrautMaterialDescriptor>& ref_materials, WKrautBranchType branchType)
{
  // === Administrative ===

  ref_node.m_bAllowSubType[0] = bt.m_bGrowSubBranchType1;
  ref_node.m_bAllowSubType[1] = bt.m_bGrowSubBranchType2;
  ref_node.m_bAllowSubType[2] = bt.m_bGrowSubBranchType3;

  ref_node.m_bEnable[Kraut::BranchGeometryType::Branch] = bt.m_bEnableMesh;
  ref_node.m_bEnable[Kraut::BranchGeometryType::Frond] = bt.m_bEnableFronds;
  ref_node.m_bEnable[Kraut::BranchGeometryType::Leaf] = bt.m_bEnableLeaves;

  if (bt.m_bEnableMesh && !bt.m_sBranchMaterial.IsEmpty())
  {
    auto& m = ref_materials.ExpandAndGetRef();
    m.m_BranchType = branchType;
    m.m_MaterialType = WKrautMaterialType::Branch;
    m.m_hMaterial = WResourceManager::LoadResource<WMaterialResource>(bt.m_sBranchMaterial);
  }

  if (bt.m_bEnableFronds && !bt.m_sFrondMaterial.IsEmpty())
  {
    auto& m = ref_materials.ExpandAndGetRef();
    m.m_BranchType = branchType;
    m.m_MaterialType = WKrautMaterialType::Frond;
    m.m_hMaterial = WResourceManager::LoadResource<WMaterialResource>(bt.m_sFrondMaterial);
    // m.m_VariationColor = bt.m_FrondVariationColor;// currently done through the material
  }

  if (bt.m_bEnableLeaves && !bt.m_sLeafMaterial.IsEmpty())
  {
    auto& m = ref_materials.ExpandAndGetRef();
    m.m_BranchType = branchType;
    m.m_MaterialType = WKrautMaterialType::Leaf;
    m.m_hMaterial = WResourceManager::LoadResource<WMaterialResource>(bt.m_sLeafMaterial);
    // m.m_VariationColor = bt.m_LeafVariationColor;// currently done through the material
  }

  // === Branch Type ===

  // General

  ref_node.m_iSegmentLengthCM = WMath::Clamp<WInt8>(bt.m_uiSegmentLengthCM, 1, 50);
  ref_node.m_BranchTypeMode = (Kraut::BranchTypeMode::Enum)bt.m_BranchTypeMode.GetValue();
  ref_node.m_fBranchlessPartABS = bt.m_fBranchlessPartABS;
  ref_node.m_fBranchlessPartEndABS = bt.m_fBranchlessPartEndABS;
  ref_node.m_uiLowerBound = WMath::Clamp<WUInt8>(bt.m_uiLowerBound, 0, 100);
  ref_node.m_uiUpperBound = WMath::Clamp<WUInt8>(bt.m_uiUpperBound, ref_node.m_uiLowerBound, 100);
  ref_node.m_uiMinBranchThicknessInCM = WMath::Clamp<WUInt16>(bt.m_uiMinBranchThicknessInCM, 1, 100);
  ref_node.m_uiMaxBranchThicknessInCM = WMath::Clamp<WUInt16>(bt.m_uiMaxBranchThicknessInCM, ref_node.m_uiMinBranchThicknessInCM, 100);

  // Spawn Nodes

  ref_node.m_uiMinBranches = WMath::Clamp<WUInt16>(bt.m_uiMinBranches, 0, 32);
  ref_node.m_uiMaxBranches = WMath::Clamp<WUInt16>(bt.m_uiMaxBranches, ref_node.m_uiMinBranches, 32);
  ref_node.m_fNodeSpacingBefore = bt.m_fNodeSpacingBefore;
  ref_node.m_fNodeSpacingAfter = bt.m_fNodeSpacingAfter;
  ref_node.m_fNodeHeight = bt.m_fNodeHeight;


  // === Growth ===

  // Start Direction

  ref_node.m_fMaxRotationalDeviation = bt.m_MaxRotationalDeviation.GetDegree();
  ref_node.m_fBranchAngle = bt.m_BranchAngle.GetDegree();
  ref_node.m_fMaxBranchAngleDeviation = bt.m_MaxBranchAngleDeviation.GetDegree();

  // Target Direction

  ref_node.m_TargetDirection = (Kraut::BranchTargetDir::Enum)bt.m_TargetDirection.GetValue();
  ref_node.m_bTargetDirRelative = bt.m_bTargetDirRelative;
  ref_node.m_TargetDir2Usage = (Kraut::BranchTargetDir2Usage::Enum)bt.m_TargetDir2Usage.GetValue();
  ref_node.m_fTargetDir2Usage = bt.m_fTargetDir2Usage;
  ref_node.m_TargetDirection2 = (Kraut::BranchTargetDir::Enum)bt.m_TargetDirection2.GetValue();
  ref_node.m_fMaxTargetDirDeviation = bt.m_MaxTargetDirDeviation.GetDegree();

  // Growth

  ref_node.m_uiMinBranchLengthInCM = WMath::Clamp<WUInt16>(bt.m_uiMinBranchLengthInCM, 1, 10000);
  ref_node.m_uiMaxBranchLengthInCM = WMath::Clamp<WUInt16>(bt.m_uiMaxBranchLengthInCM, ref_node.m_uiMinBranchLengthInCM, 10000);
  CopyKrautCurve(ref_node.m_MaxBranchLengthParentScale, bt.m_MaxBranchLengthParentScale, 20, 1.0f);
  ref_node.m_fGrowMaxTargetDirDeviation = bt.m_GrowMaxTargetDirDeviation.GetDegree();
  ref_node.m_fGrowMaxDirChangePerSegment = bt.m_GrowMaxDirChangePerSegment.GetDegree();
  ref_node.m_bRestrictGrowthToFrondPlane = bt.m_bRestrictGrowthToFrondPlane;

  // Obstacles

  // bool m_bActAsObstacle;
  // bool m_bDoPhysicalSimulation;
  // float m_fPhysicsLookAhead;
  // float m_fPhysicsEvasionAngle;


  // === Appearance ===

  // Branch Mesh

  CopyKrautCurve(ref_node.m_BranchContour, bt.m_BranchContour, 50, 1.0f);
  ref_node.m_fRoundnessFactor = bt.m_fRoundnessFactor;
  ref_node.m_uiFlares = bt.m_uiFlares;
  ref_node.m_fFlareWidth = bt.m_fFlareWidth;
  CopyKrautCurve(ref_node.m_FlareWidthCurve, bt.m_FlareWidthCurve, 50, 1.0f);
  ref_node.m_fFlareRotation = bt.m_FlareRotation.GetDegree();
  ref_node.m_bRotateTexCoords = bt.m_bRotateTexCoords;

  // Fronds

  ref_node.m_fTextureRepeat = bt.m_fTextureRepeat;
  ref_node.m_FrondUpOrientation = (Kraut::LeafOrientation::Enum)bt.m_FrondUpOrientation.GetValue();
  ref_node.m_uiMaxFrondOrientationDeviation = (WUInt8)bt.m_MaxFrondOrientationDeviation.GetDegree();
  ref_node.m_uiNumFronds = bt.m_uiNumFronds;
  ref_node.m_bAlignFrondsOnSurface = bt.m_bAlignFrondsOnSurface;
  ref_node.m_uiFrondDetail = bt.m_uiFrondDetail;
  CopyKrautCurve(ref_node.m_FrondContour, bt.m_FrondContour, 40, 1.0f);
  ref_node.m_FrondContourMode = (Kraut::SpawnNodeDesc::FrondContourMode)bt.m_FrondContourMode.GetValue();
  ref_node.m_fFrondHeight = (bt.m_FrondContourMode == WKrautFrondContourMode::Off) ? 0.0f : -bt.m_fFrondHeight;
  CopyKrautCurve(ref_node.m_FrondHeight, bt.m_FrondHeight, 50, 0.0f);
  ref_node.m_fFrondWidth = bt.m_fFrondWidth;
  CopyKrautCurve(ref_node.m_FrondWidth, bt.m_FrondWidth, 50, 1.0f);

  // Leaves

  ref_node.m_bBillboardLeaves = bt.m_bBillboardLeaves;
  ref_node.m_fLeafSize = bt.m_fLeafSize;
  CopyKrautCurve(ref_node.m_LeafScale, bt.m_LeafScale, 25, 1.0f);
  ref_node.m_fLeafInterval = bt.m_fLeafInterval;

  // Shared
  // WUInt8 m_uiTextureTilingX[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
  // WUInt8 m_uiTextureTilingY[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
}
