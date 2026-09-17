#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

using WNavMeshPathTestComponentManager = WComponentManagerSimple<class WAiNavMeshPathTestComponent, WComponentUpdateType::WhenSimulating>;

/// Used to test path-finding through a navmesh.
///
/// The component takes a reference to another game object as the destination
/// and then requests a path from the navmesh.
/// Various aspects of the path can be visualized for inspection.
///
/// This component should be used in the editor, to test whether the scene navmesh behaves as desired.
class W_AIPLUGIN_DLL WAiNavMeshPathTestComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WAiNavMeshPathTestComponent, WComponent, WNavMeshPathTestComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  //  WAiNavMeshPathTestComponent

public:
  WAiNavMeshPathTestComponent();
  ~WAiNavMeshPathTestComponent();

  void SetPathEndReference(const char* szReference); // [ property ]
  void SetPathEnd(WGameObjectHandle hObject);

  /// Render the navmesh polygons, through which the path goes.
  bool m_bVisualizePathCorridor = true; // [ property ]

  /// Render a line for the shortest path through the corridor.
  bool m_bVisualizePathLine = true; // [ property ]

  /// Render text describing what went wrong during path search.
  bool m_bVisualizePathState = true; // [ property ]

  /// Name of the WAiNavmeshConfig to use. See WAiNavigationConfig.
  WHashedString m_sNavmeshConfig; // [ property ]

  /// Name of the WAiPathSearchConfig to use. See WAiNavigationConfig.
  WHashedString m_sPathSearchConfig; // [ property ]

protected:
  void Update();

  WGameObjectHandle m_hPathEnd;
  WAiNavigation m_Navigation;

private:
  const char* DummyGetter() const { return nullptr; }
};
