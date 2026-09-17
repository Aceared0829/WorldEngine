#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;
using WDynamicMeshBufferResourceHandle = WTypedResourceHandle<class WDynamicMeshBufferResource>;

//////////////////////////////////////////////////////////////////////////

class W_JOLTPLUGIN_DLL WJoltClothSheetComponentManager : public WComponentManager<class WJoltClothSheetComponent, WBlockStorageType::FreeList>
{
public:
  WJoltClothSheetComponentManager(WWorld* pWorld);
  ~WJoltClothSheetComponentManager();

  virtual void Initialize() override;

private:
  WUInt64 m_uiLastJoltUpdateCounter = 0;
  void UpdatePreAsync(const WWorldModule::UpdateContext& context);
  void UpdatePostAsync(const WWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

/// Flags for how a piece of cloth should be simulated.
struct W_JOLTPLUGIN_DLL WJoltClothSheetFlags
{
  using StorageType = WUInt16;

  enum Enum
  {
    FixedCornerTopLeft = W_BIT(0),     ///< This corner can't move.
    FixedCornerTopRight = W_BIT(1),    ///< This corner can't move.
    FixedCornerBottomRight = W_BIT(2), ///< This corner can't move.
    FixedCornerBottomLeft = W_BIT(3),  ///< This corner can't move.
    FixedEdgeTop = W_BIT(4),           ///< This entire edge can't move.
    FixedEdgeRight = W_BIT(5),         ///< This entire edge can't move.
    FixedEdgeBottom = W_BIT(6),        ///< This entire edge can't move.
    FixedEdgeLeft = W_BIT(7),          ///< This entire edge can't move.

    Default = FixedEdgeTop
  };

  struct Bits
  {
    StorageType FixedCornerTopLeft : 1;
    StorageType FixedCornerTopRight : 1;
    StorageType FixedCornerBottomRight : 1;
    StorageType FixedCornerBottomLeft : 1;
    StorageType FixedEdgeTop : 1;
    StorageType FixedEdgeRight : 1;
    StorageType FixedEdgeBottom : 1;
    StorageType FixedEdgeLeft : 1;
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WJoltClothSheetFlags);

/// Simulates a rectangular piece of cloth.
///
/// The cloth doesn't interact with the environment and doesn't collide with any geometry.
/// The component samples the wind simulation and applies wind forces to the cloth.
///
/// Cloth sheets can be used as decorative elements like flags that blow in the wind.
class W_JOLTPLUGIN_DLL WJoltClothSheetComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltClothSheetComponent, WRenderComponent, WJoltClothSheetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;

private:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // WJoltClothSheetComponent

public:
  WJoltClothSheetComponent();
  ~WJoltClothSheetComponent();

  /// Sets the world-space size of the cloth.
  void SetSize(WVec2 vVal);                 // [ property ]
  WVec2 GetSize() const { return m_vSize; } // [ property ]

  /// Sets of how many pieces the cloth is made up.
  ///
  /// More pieces cost more performance to simulate the cloth.
  /// A size of 32x32 is already quite performance intensive. Use as few segments as possible.
  /// For many cases 8x8 or 12x12 should already be good enough.
  /// Also the more segments there are, the more the cloth will sag.
  void SetSegments(WVec2U32 vVal);                     // [ property ]
  WVec2U32 GetSegments() const { return m_vNumVertices; } // [ property ]

  /// The collision layer determines with which other actors this actor collides. \see WJoltActorComponent
  WUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// Adjusts how strongly gravity affects the soft body.
  float m_fGravityFactor = 1.0f; // [ property ]

  /// A factor to tweak how strong the wind can push the cloth.
  float m_fWindInfluence = 0.3f; // [ property ]

  /// Damping slows down cloth movement over time. Higher values make it stop sooner and also improve performance.
  float m_fDamping = 0.5f; // [ property ]

  /// How thick the cloth is, to prevent it from intersecting with other geometry.
  float m_fThickness = 0.05f; // [ property ]

  /// Tint color for the cloth material.
  WColor m_Color = WColor::White; // [ property ]

  /// Sets where the cloth is attached to the world.
  void SetFlags(WBitflags<WJoltClothSheetFlags> flags);                // [ property ]
  WBitflags<WJoltClothSheetFlags> GetFlags() const { return m_Flags; } // [ property ]

  WMaterialResourceHandle m_hMaterial;                                  // [ property ]

private:
  void UpdatePreAsync();
  void UpdatePostAsync();

  void ApplyWind();

  void SetupCloth();
  void RemoveBody();

  WVec2 m_vSize = WVec2(1.0f, 1.0f);
  WVec2 m_vTextureScale = WVec2(1.0f);
  WVec2U32 m_vNumVertices = WVec2U32(16, 16);
  WBitflags<WJoltClothSheetFlags> m_Flags;
  mutable WRenderData::Category m_RenderDataCategory;
  WUInt8 m_uiSleepCounter = 0;
  WUInt32 m_uiObjectFilterID = WInvalidIndex;
  WUInt32 m_uiUserDataIndex = WInvalidIndex;
  WUInt32 m_uiJoltBodyID = WInvalidIndex;

  WBoundingSphere m_BSphere;
  WTransform m_BodyGlobalTransform = WTransform::MakeIdentity();

  mutable WInstanceDataOffset m_InstanceDataOffset;

  WDynamicMeshBufferResourceHandle m_hDynamicMeshBuffer;
};
