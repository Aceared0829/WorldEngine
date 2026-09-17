#pragma once

#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/DrawBoxGizmo.h>

struct WGameObjectEvent;
struct WManipulatorManagerEvent;

class W_EDITORPLUGINSCENE_DLL WGreyBoxEditTool : public WGameObjectEditTool
{
  W_ADD_DYNAMIC_REFLECTION(WGreyBoxEditTool, WGameObjectEditTool);

public:
  WGreyBoxEditTool();
  ~WGreyBoxEditTool();

  virtual WEditorInputContext* GetEditorInputContextOverride() override;
  virtual WEditToolSupportedSpaces GetSupportedSpaces() const override;
  virtual bool GetSupportsMoveParentOnly() const override;
  virtual void GetGridSettings(WGridSettingsMsgToEngine& out_gridSettings) override;

protected:
  virtual void OnConfigured() override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  void UpdateGizmoState();
  void GameObjectEventHandler(const WGameObjectEvent& e);
  void ManipulatorManagerEventHandler(const WManipulatorManagerEvent& e);
  void GizmoEventHandler(const WGizmoEvent& e);

  WDrawBoxGizmo m_DrawBoxGizmo;
};
