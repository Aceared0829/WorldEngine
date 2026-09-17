#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation3D/VoxelNavigation.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

using WAiVoxelPathTestComponentManager = WComponentManagerSimple<class WAiVoxelPathTestComponent, WComponentUpdateType::WhenSimulating>;

/// Used to test path-finding through voxel grids.
///
/// The component takes a reference to another game object as the destination and recomputes a path
/// towards it every frame using WAiVoxelNavigation. The path can be visualized, coloring segments
/// that go through a voxel grid differently from straight-line segments that cross free space not
/// covered by any grid.
///
/// Repathing every frame is intentionally wasteful - this is a debugging aid, not something to run
/// in a shipping scene.
///
/// This component should be used in the editor, to test whether the scene's voxel grids produce the
/// desired paths.
class W_AIPLUGIN_DLL WAiVoxelPathTestComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WAiVoxelPathTestComponent, WComponent, WAiVoxelPathTestComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  //  WAiVoxelPathTestComponent

public:
  WAiVoxelPathTestComponent();
  ~WAiVoxelPathTestComponent();

  void SetPathEndReference(const char* szReference); // [ property ]
  void SetPathEnd(WGameObjectHandle hObject);

  /// Render the smoothed path, i.e. the one actually used for navigation.
  bool m_bVisualizeSmoothedPath = true; // [ property ]

  /// Render text describing the path search result.
  bool m_bVisualizePathState = true; // [ property ]

  /// Passed through to WAiVoxelNavigation::FindPath().
  float m_fSearchMargin = 5.0f; // [ property ]

  /// Passed through to WAiVoxelNavigation::FindPath().
  WUInt32 m_uiMaxIterationsPerHop = 10000; // [ property ]

  /// Passed through to WAiVoxelNavigation::FindPath().
  WUInt32 m_uiMaxHops = 16; // [ property ]

protected:
  void Update();

  WGameObjectHandle m_hPathEnd;
  WAiVoxelNavigation m_Navigation;

private:
  const char* DummyGetter() const { return nullptr; }
};
