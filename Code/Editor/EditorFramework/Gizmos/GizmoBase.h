#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WCamera;

struct WGizmoEvent
{
  enum class Type
  {
    BeginInteractions,
    EndInteractions,
    Interaction,
    CancelInteractions,
  };

  const WEditorInputContext* m_pGizmo = nullptr;
  Type m_Type;
};

class W_EDITORFRAMEWORK_DLL WGizmo : public WEditorInputContext
{
  W_ADD_DYNAMIC_REFLECTION(WGizmo, WEditorInputContext);

public:
  WGizmo();

  void SetVisible(bool bVisible);
  bool IsVisible() const { return m_bVisible; }

  void SetTransformation(const WTransform& transform);
  const WTransform& GetTransformation() const { return m_Transformation; }

  void ConfigureInteraction(WGizmoHandle* pHandle, const WCamera* pCamera, const WVec3& vInteractionPivot, const WVec2I32& vViewport)
  {
    m_pInteractionGizmoHandle = pHandle;
    m_pCamera = pCamera;
    m_vInteractionPivot = vInteractionPivot;
    m_vViewport = vViewport;
  }

  WEvent<const WGizmoEvent&> m_GizmoEvents;

protected:
  virtual void OnVisibleChanged(bool bVisible) = 0;
  virtual void OnTransformationChanged(const WTransform& transform) = 0;

  void GetInverseViewProjectionMatrix(WMat4& out_mInvViewProj) const;
  WResult GetPointOnPlane(const WPlane& plane, const WVec2I32& vScreenPos, const WMat4& mInvViewProj, WVec3& out_Result) const;
  WResult GetPointOnAxis(const WVec3& vStartPos, const WVec3& vAxis, const WVec2I32& vScreenPos, const WMat4& mInvViewProj, WVec3& out_Result, float* out_pProjectedLength = nullptr) const;

  const WCamera* m_pCamera = nullptr;
  WGizmoHandle* m_pInteractionGizmoHandle = nullptr;
  WVec3 m_vInteractionPivot;
  WVec2I32 m_vViewport;

private:
  bool m_bVisible = false;
  WTransform m_Transformation;
};
