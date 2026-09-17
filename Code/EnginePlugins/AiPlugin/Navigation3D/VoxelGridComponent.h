#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation3D/VoxelGrid.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/SpatialData.h>

using WAiVoxelGridComponentManager = WComponentManager<class WAiVoxelGridComponent, WBlockStorageType::Compact>;

/// Drop this component into a scene to add a 3D voxel navigation grid.
///
/// The grid is centered on this component's game object position.
class W_AIPLUGIN_DLL WAiVoxelGridComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WAiVoxelGridComponent, WComponent, WAiVoxelGridComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WAiVoxelGridComponent

public:
  WAiVoxelGridComponent();
  ~WAiVoxelGridComponent();

  const WVec3& GetSize() const { return m_vSize; }
  void SetSize(const WVec3& vSize);

  float GetVoxelSize() const { return m_fVoxelSize; }               // [ property ]
  void SetVoxelSize(float fValue);                                  // [ property ]

  WUInt32 GetCollisionLayer() const { return m_uiCollisionLayer; } // [ property ]
  void SetCollisionLayer(WUInt32 uiValue);                         // [ property ]

  /// If true, or if the AI.VoxelGrid.Visualize CVar is set, the grid is drawn using the debug renderer.
  bool m_bVisualize = false; // [ property ]

  void VoxelizeWorld();

  const WVoxelGrid& GetStaticVoxelGrid() const { return m_StaticGrid; }

  /// Spatial data category used to register grid components with the world's spatial system.
  ///
  /// Used by WAiVoxelWorldModule::FindGridsInBox() to efficiently query grid components in an area.
  static WSpatialData::Category SpatialDataCategory;

protected:
  void OnMsgUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const; // [ msg handler ]

private:
  WVec3 m_vSize = WVec3(32.0f);
  float m_fVoxelSize = 0.5f;
  WUInt32 m_uiCollisionLayer = 0;

  WVoxelGrid m_StaticGrid;

  bool m_bNeedsVoxelization = true;
};
