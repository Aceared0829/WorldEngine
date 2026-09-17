#pragma once

#include <EditorFramework/EditTools/GizmoEditTool.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>

class WQtGameObjectDocumentWindow;
class WPreferences;

class W_EDITORFRAMEWORK_DLL WTranslateGizmoEditTool : public WGameObjectGizmoEditTool
{
  W_ADD_DYNAMIC_REFLECTION(WTranslateGizmoEditTool, WGameObjectGizmoEditTool);

public:
  WTranslateGizmoEditTool();
  ~WTranslateGizmoEditTool();

  virtual WEditToolSupportedSpaces GetSupportedSpaces() const override { return WEditToolSupportedSpaces::LocalAndWorldSpace; }
  virtual bool GetSupportsMoveParentOnly() const override { return true; }
  virtual void GetGridSettings(WGridSettingsMsgToEngine& out_gridSettings) override;

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const WTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const WGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  void OnPreferenceChange(WPreferences* pref);

  WTranslateGizmo m_TranslateGizmo;
  enum GridPlane
  {
    X,
    Y,
    Z
  };

  GridPlane m_GridPlane = GridPlane::Z;
};

//////////////////////////////////////////////////////////////////////////

class W_EDITORFRAMEWORK_DLL WRotateGizmoEditTool : public WGameObjectGizmoEditTool
{
  W_ADD_DYNAMIC_REFLECTION(WRotateGizmoEditTool, WGameObjectGizmoEditTool);

public:
  WRotateGizmoEditTool();
  ~WRotateGizmoEditTool();

  virtual WEditToolSupportedSpaces GetSupportedSpaces() const override { return WEditToolSupportedSpaces::LocalAndWorldSpace; }
  virtual bool GetSupportsMoveParentOnly() const override { return true; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const WTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const WGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  WRotateGizmo m_RotateGizmo;
};

//////////////////////////////////////////////////////////////////////////

class W_EDITORFRAMEWORK_DLL WScaleGizmoEditTool : public WGameObjectGizmoEditTool
{
  W_ADD_DYNAMIC_REFLECTION(WScaleGizmoEditTool, WGameObjectGizmoEditTool);

public:
  WScaleGizmoEditTool();
  ~WScaleGizmoEditTool();

  virtual WEditToolSupportedSpaces GetSupportedSpaces() const override { return WEditToolSupportedSpaces::LocalSpaceOnly; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const WTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const WGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  WScaleGizmo m_ScaleGizmo;
};

//////////////////////////////////////////////////////////////////////////

class W_EDITORFRAMEWORK_DLL WDragToPositionGizmoEditTool : public WGameObjectGizmoEditTool
{
  W_ADD_DYNAMIC_REFLECTION(WDragToPositionGizmoEditTool, WGameObjectGizmoEditTool);

public:
  WDragToPositionGizmoEditTool();
  ~WDragToPositionGizmoEditTool();

  virtual WEditToolSupportedSpaces GetSupportedSpaces() const override { return WEditToolSupportedSpaces::LocalSpaceOnly; }
  virtual bool GetSupportsMoveParentOnly() const override { return true; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const WTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const WGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  WDragToPositionGizmo m_DragToPosGizmo;
};
