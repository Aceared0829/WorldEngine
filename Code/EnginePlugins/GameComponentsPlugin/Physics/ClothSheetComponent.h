#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>
#include <GameEngine/Physics/ClothSheetSimulator.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;
using WDynamicMeshBufferResourceHandle = WTypedResourceHandle<class WDynamicMeshBufferResource>;

//////////////////////////////////////////////////////////////////////////

class W_GAMECOMPONENTS_DLL WClothSheetComponentManager : public WComponentManager<class WClothSheetComponent, WBlockStorageType::FreeList>
{
public:
  WClothSheetComponentManager(WWorld* pWorld);
  ~WClothSheetComponentManager();

  virtual void Initialize() override;

private:
  void Update(const WWorldModule::UpdateContext& context);
  void UpdateBounds(const WWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

/// Flags for how a piece of cloth should be simulated.
struct W_GAMECOMPONENTS_DLL WClothSheetFlags
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

W_DECLARE_REFLECTABLE_TYPE(W_GAMECOMPONENTS_DLL, WClothSheetFlags);

/// Simulates a rectangular piece of cloth.
///
/// The cloth doesn't interact with the environment and doesn't collide with any geometry.
/// The component samples the wind simulation and applies wind forces to the cloth.
///
/// Cloth sheets can be used as decorative elements like flags that blow in the wind.
class W_GAMECOMPONENTS_DLL WClothSheetComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WClothSheetComponent, WRenderComponent, WClothSheetComponentManager);

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
  // WClothSheetComponent

public:
  WClothSheetComponent();
  ~WClothSheetComponent();

  /// Sets the world-space size of the cloth.
  void SetSize(WVec2 vVal);                 // [ property ]
  WVec2 GetSize() const { return m_vSize; } // [ property ]

  /// Sets of how many pieces the cloth is made up.
  ///
  /// More pieces cost more performance to simulate the cloth.
  /// A size of 32x32 is already quite performance intensive. USe as few segments as possible.
  /// For many cases 8x8 or 12x12 should already be good enough.
  /// Also the more segments there are, the more the cloth will sag.
  void SetSegments(WVec2U32 vVal);                     // [ property ]
  WVec2U32 GetSegments() const { return m_vSegments; } // [ property ]

  /// How much sag the cloth should have along each axis.
  void SetSlack(WVec2 vVal);                  // [ property ]
  WVec2 GetSlack() const { return m_vSlack; } // [ property ]

  /// A factor to tweak how strong the wind can push the cloth.
  float m_fWindInfluence = 0.3f; // [ property ]

  /// Damping slows down cloth movement over time. Higher values make it stop sooner and also improve performance.
  float m_fDamping = 0.5f; // [ property ]

  /// Tint color for the cloth material.
  WColor m_Color = WColor::White; // [ property ]

  /// Sets where the cloth is attached to the world.
  void SetFlags(WBitflags<WClothSheetFlags> flags);                // [ property ]
  WBitflags<WClothSheetFlags> GetFlags() const { return m_Flags; } // [ property ]

  WMaterialResourceHandle m_hMaterial;                              // [ property ]

private:
  void Update();
  void UpdateClothMesh();
  void SetupCloth();

  WVec2 m_vSize;
  WVec2 m_vSlack;
  WVec2U32 m_vSegments;
  WBitflags<WClothSheetFlags> m_Flags;

  WUInt8 m_uiSleepCounter = 0;
  WUInt8 m_uiCheckEquilibriumCounter = 0;
  mutable WInstanceDataOffset m_InstanceDataOffset;

  WClothSimulator m_Simulator;

  WBoundingBox m_Bbox;
  WDynamicMeshBufferResourceHandle m_hDynamicMeshBuffer;
};
