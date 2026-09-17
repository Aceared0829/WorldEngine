#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <RendererCore/Meshes/MeshComponent.h>

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WGizmoRenderData : public WMeshRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WGizmoRenderData, WMeshRenderData);

public:
  WTransform m_GlobalTransform;
  WColor m_GizmoColor;
  WUInt32 m_uiUniqueID;
  bool m_bIsPickable;
};

class WGizmoComponent;
class WGizmoComponentManager : public WComponentManager<WGizmoComponent, WBlockStorageType::FreeList>
{
public:
  WGizmoComponentManager(WWorld* pWorld);

  WUInt32 m_uiHighlightID = 0;
};

/// Used by the editor to render gizmo meshes.
///
/// Gizmos use special shaders to have constant screen-space size and swap geometry towards the viewer,
/// so their culling is non-trivial. This component takes care of that and of the highlight color.
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WGizmoComponent : public WMeshComponent
{
  W_DECLARE_COMPONENT_TYPE(WGizmoComponent, WMeshComponent, WGizmoComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WMeshComponentBase

protected:
  virtual WMeshRenderData* CreateRenderData(const WRenderDataManager* pRenderDataManager) const override;
  virtual WResult GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // WGizmoComponent

public:
  WGizmoComponent();
  ~WGizmoComponent();

  WColor m_GizmoColor = WColor::White;
  bool m_bIsPickable = true;
  WDynamicArray<WVec3> m_Lines;
};
