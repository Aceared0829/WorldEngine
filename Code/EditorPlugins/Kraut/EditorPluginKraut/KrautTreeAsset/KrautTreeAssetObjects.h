#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <KrautPlugin/KrautDeclarations.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct WPropertyMetaStateEvent;

/// Display names used for branch type entries in the editor UI.
namespace WKrautBranchTypeNames
{
  constexpr const char* Trunk = "Trunk";
  constexpr const char* MainBranch1 = "Main Branch 1";
  constexpr const char* MainBranch2 = "Main Branch 2";
  constexpr const char* MainBranch3 = "Main Branch 3";
  constexpr const char* SubBranch1 = "Sub Branch 1";
  constexpr const char* SubBranch2 = "Sub Branch 2";
  constexpr const char* SubBranch3 = "Sub Branch 3";
  constexpr const char* Twig1 = "Twig 1";
  constexpr const char* Twig2 = "Twig 2";
  constexpr const char* Twig3 = "Twig 3";
} // namespace WKrautBranchTypeNames

/// Maps a user-visible label to a material asset path for one slot in the tree's material list.
struct WKrautAssetMaterial
{
  WString m_sLabel;
  WString m_sMaterial;
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WKrautAssetMaterial);

/// Controls the overall growth pattern of a branch type.
struct WKrautBranchTypeMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Regular,  ///< Standard outward growth.
    Umbrella, ///< Branches arc upward and then droop outward, forming an umbrella silhouette.

    Default = Regular,
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WKrautBranchTypeMode);

/// Specifies the target growth direction for a branch type, expressed as an angle from straight up.
struct WKrautBranchTargetDir
{
  using StorageType = WUInt8;

  enum Enum
  {
    Straight, ///< Along the start direction.
    Upwards,  ///< Toward positive Z (sky).
    Degree22,
    Degree45,
    Degree67,
    Degree90,
    Degree112,
    Degree135,
    Degree157,
    Downwards, ///< Toward negative Z (ground).

    Default = Straight
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WKrautBranchTargetDir);

/// Controls the orientation of leaf geometry relative to the branch it grows from.
struct WKrautLeafOrientation
{
  using StorageType = WUInt8;

  enum Enum
  {
    Upwards,            ///< Leaf plane faces upward regardless of branch direction.
    AlongBranch,        ///< Leaf plane is aligned with the branch axis.
    OrthogonalToBranch, ///< Leaf plane is perpendicular to the branch axis.

    Default = Upwards,
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WKrautLeafOrientation);

/// Determines how the contour curve is applied to frond geometry.
struct WKrautFrondContourMode
{
  using StorageType = WInt8;

  enum Enum
  {
    Off = -1,        ///< No contour; frond uses a flat rectangular shape.
    Full,            ///< Contour applied to the full frond width.
    Symetric,        ///< Contour mirrored symmetrically around the frond center.
    InverseSymetric, ///< Inverse of Symetric.

    Default = Full
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WKrautFrondContourMode);

/// Controls whether and how a second target direction blends with the primary target direction.
struct WKrautBranchTargetDir2Usage
{
  using StorageType = WUInt8;

  enum Enum
  {
    Off,      ///< Only the primary target direction is used.
    Relative, ///< Secondary direction is relative to the branch's start orientation.
    Absolute, ///< Secondary direction is in world space.

    Default = Off
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WKrautBranchTargetDir2Usage);

/// Selects the geometry representation used for a LOD level.
struct WKrautLodMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Full,      ///< Full triangle mesh for all enabled geometry types.
    FourQuads, ///< Impostor represented by four intersecting quads.
    TwoQuads,  ///< Impostor represented by two intersecting quads.
    Billboard, ///< Single camera-facing billboard quad.
    Disabled,  ///< LOD is not generated.

    Default = Full
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WKrautLodMode);

/// Controls how the tapered tip of a branch segment is tessellated.
struct WKrautBranchSpikeTipMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    FullDetail,     ///< Full vertex ring at the tip.
    SingleTriangle, ///< Tip collapsed to a single triangle fan, reducing vertex count.
    Hole,           ///< Tip is left open (no cap geometry).

    Default = FullDetail
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WKrautBranchSpikeTipMode);

/// Authoring data for a single Kraut branch type (trunk, main branch, sub-branch, or twig).
///
/// Each field group maps to a section in the Kraut tree editor's property panel.
/// Instances of this struct are embedded in WKrautTreeAssetProperties, one per active branch type slot.
struct WKrautAssetBranchType
{
  // === Administrative ===

  // bool m_bVisible = true;

  bool m_bGrowSubBranchType1 = false;
  bool m_bGrowSubBranchType2 = false;
  bool m_bGrowSubBranchType3 = false;

  // === Branch Type ===

  // General

  WUInt8 m_uiSegmentLengthCM = 5;
  WEnum<WKrautBranchTypeMode> m_BranchTypeMode = WKrautBranchTypeMode::Regular;
  float m_fBranchlessPartABS = 0.0f;
  float m_fBranchlessPartEndABS = 0.0f;
  WUInt8 m_uiLowerBound = 0;
  WUInt8 m_uiUpperBound = 100;
  WUInt16 m_uiMinBranchThicknessInCM = 20;
  WUInt16 m_uiMaxBranchThicknessInCM = 20;

  // Spawn Nodes

  WUInt8 m_uiMinBranches = 4;
  WUInt8 m_uiMaxBranches = 4;
  float m_fNodeSpacingBefore = 0.5f;
  float m_fNodeSpacingAfter = 0.0f;
  float m_fNodeHeight = 0.0f;


  // === Growth ===

  // Start Direction

  WAngle m_MaxRotationalDeviation = {};
  WAngle m_BranchAngle = WAngle::MakeFromDegree(90);
  WAngle m_MaxBranchAngleDeviation = WAngle::MakeFromDegree(10);

  // Target Direction

  WEnum<WKrautBranchTargetDir> m_TargetDirection = WKrautBranchTargetDir::Straight;
  bool m_bTargetDirRelative = false;
  WEnum<WKrautBranchTargetDir2Usage> m_TargetDir2Usage = WKrautBranchTargetDir2Usage::Off;
  float m_fTargetDir2Usage = 2.5f;
  WEnum<WKrautBranchTargetDir> m_TargetDirection2 = WKrautBranchTargetDir::Upwards;
  WAngle m_MaxTargetDirDeviation = WAngle::MakeFromDegree(20);

  // Growth

  WUInt16 m_uiMinBranchLengthInCM = 100;
  WUInt16 m_uiMaxBranchLengthInCM = 100;
  WSingleCurveData m_MaxBranchLengthParentScale;
  WAngle m_GrowMaxTargetDirDeviation = {};
  WAngle m_GrowMaxDirChangePerSegment = WAngle::MakeFromDegree(5);
  bool m_bRestrictGrowthToFrondPlane = false;

  // Obstacles

  // bool m_bActAsObstacle = false;
  // bool m_bDoPhysicalSimulation = false;
  // float m_fPhysicsLookAhead = 1.5f;
  // WAngle m_PhysicsEvasionAngle = WAngle::MakeFromDegree(30);


  // === Appearance ===

  // Branch Mesh

  bool m_bEnableMesh = true;
  WString m_sBranchMaterial;
  WSingleCurveData m_BranchContour;
  float m_fRoundnessFactor = 0.5f;
  WUInt8 m_uiFlares = 0;
  float m_fFlareWidth = 2.0f;
  WSingleCurveData m_FlareWidthCurve;
  WAngle m_FlareRotation = {};
  bool m_bRotateTexCoords = true;

  // Fronds

  bool m_bEnableFronds = true;
  WString m_sFrondMaterial;
  float m_fTextureRepeat = 0.0f;
  WEnum<WKrautLeafOrientation> m_FrondUpOrientation = WKrautLeafOrientation::Upwards;
  WAngle m_MaxFrondOrientationDeviation = {};
  WUInt8 m_uiNumFronds = 1;
  bool m_bAlignFrondsOnSurface = false;
  WUInt8 m_uiFrondDetail = 1;
  WSingleCurveData m_FrondContour;
  WEnum<WKrautFrondContourMode> m_FrondContourMode = WKrautFrondContourMode::Full;
  float m_fFrondHeight = 0.5f;
  WSingleCurveData m_FrondHeight;
  float m_fFrondWidth = 0.5f;
  WSingleCurveData m_FrondWidth;
  // WColor m_FrondVariationColor;// currently done through the material

  // Leaves

  bool m_bEnableLeaves = true;
  WString m_sLeafMaterial;
  bool m_bBillboardLeaves = true;
  float m_fLeafSize = 0.25f;
  WSingleCurveData m_LeafScale;
  float m_fLeafInterval = 0;
  // WColor m_LeafVariationColor;// currently done through the material

  // Shared

  // WUInt8 m_uiTextureTilingX[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
  // WUInt8 m_uiTextureTilingY[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WKrautAssetBranchType);


/// Authoring data for a single LOD level in a Kraut tree asset.
///
/// Controls which geometry types are included in the LOD (via the Allow* bitfields)
/// and how much mesh detail is generated. m_uiLodDistance sets the camera distance
/// at which the engine switches to this LOD.
struct WKrautAssetLod
{
  WEnum<WKrautLodMode> m_Mode = WKrautLodMode::Full;
  float m_fTipDetail = 0.04f;
  float m_fCurvatureThreshold = 5.0f;
  float m_fThicknessThreshold = 0.2f;
  float m_fVertexRingDetail = 0.2f;
  WBitflags<WKrautTreeTypeBits> m_AllowBranch = WKrautTreeTypeBits::Default; ///< Branch types for which branch mesh geometry is generated in this LOD.
  WBitflags<WKrautTreeTypeBits> m_AllowFrond = WKrautTreeTypeBits::Default;  ///< Branch types for which frond geometry is generated in this LOD.
  WBitflags<WKrautTreeTypeBits> m_AllowLeaf = WKrautTreeTypeBits::Default;   ///< Branch types for which leaf geometry is generated in this LOD.
  WInt8 m_iMaxFrondDetail = 32;
  WInt8 m_iFrondDetailReduction = 0;
  WUInt32 m_uiLodDistance = 0;                                                 ///< Camera distance in meters at which the engine transitions to this LOD.
  WEnum<WKrautBranchSpikeTipMode> m_BranchSpikeTipMode = WKrautBranchSpikeTipMode::FullDetail;
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WKrautAssetLod);

/// The editable property object for a Kraut tree asset document.
///
/// Aggregates all branch type descriptors (one per active slot), up to five LOD descriptors,
/// the material list, and global tree properties. The document serializes this object and
/// uses it to drive WKrautGeneratorResourceDescriptor creation at transform time.
class WKrautTreeAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WKrautTreeAssetProperties, WReflectedClass);

public:
  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

  WKrautTreeAssetProperties();
  ~WKrautTreeAssetProperties();

  // float m_fUniformScaling = 1.0f;
  // float m_fLodDistanceScale = 1.0f;
  float m_fStaticColliderRadius = 0.4f;
  float m_fTreeStiffness = 10.0f;
  float m_fMinAmbientOcclusion = 0.7f;
  WString m_sSurface;

  WHybridArray<WKrautAssetMaterial, 8> m_Materials;
  WKrautAssetBranchType m_BT_Trunk1;
  // WKrautAssetBranchType m_BT_Trunk2;
  // WKrautAssetBranchType m_BT_Trunk3;
  WKrautAssetBranchType m_BT_MainBranch1;
  WKrautAssetBranchType m_BT_MainBranch2;
  WKrautAssetBranchType m_BT_MainBranch3;
  WKrautAssetBranchType m_BT_SubBranch1;
  WKrautAssetBranchType m_BT_SubBranch2;
  WKrautAssetBranchType m_BT_SubBranch3;
  WKrautAssetBranchType m_BT_Twig1;
  WKrautAssetBranchType m_BT_Twig2;
  WKrautAssetBranchType m_BT_Twig3;

  WKrautAssetLod m_Lod0;
  WKrautAssetLod m_Lod1;
  WKrautAssetLod m_Lod2;
  WKrautAssetLod m_Lod3;
  WKrautAssetLod m_Lod4;

  /// Seed used to generate the tree shown in the asset editor preview.
  WUInt16 m_uiRandomSeedForDisplay = 0;

  /// List of seeds that produce good-looking results for this tree type.
  /// WKrautTreeComponent uses this list when a variation index is specified.
  WHybridArray<WUInt16, 16> m_GoodRandomSeeds;
};
