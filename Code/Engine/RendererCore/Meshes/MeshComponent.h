#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

struct WMsgExtractGeometry;
using WMeshComponentManager = WComponentManager<class WMeshComponent, WBlockStorageType::Compact>;

/// Renders a single instance of a static mesh.
///
/// This is the main component to use for rendering regular meshes.
class W_RENDERERCORE_DLL WMeshComponent : public WMeshComponentBase
{
  W_DECLARE_COMPONENT_TYPE(WMeshComponent, WMeshComponentBase, WMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WMeshComponent

public:
  WMeshComponent();
  ~WMeshComponent();

  /// Extracts the render geometry for export etc.
  void OnMsgExtractGeometry(WMsgExtractGeometry& ref_msg) const; // [ msg handler ]
};
