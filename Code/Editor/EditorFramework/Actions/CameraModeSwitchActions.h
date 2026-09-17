#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class W_EDITORFRAMEWORK_DLL WCameraModeSwitchActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  /// Maps the camera mode dropdown to the toolbar identified by \a szMapping.
  static void MapToolbarActions(const char* szMapping);

  static WActionDescriptorHandle s_hCameraMode;
};

class W_EDITORFRAMEWORK_DLL WCameraModeSwitchAction : public WDynamicMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WCameraModeSwitchAction, WDynamicMenuAction);

public:
  WCameraModeSwitchAction(const WActionContext& context, const char* szName, const char* szIconPath);
  virtual void GetEntries(WDynamicArray<Item>& out_entries) override;
  virtual void Execute(const WVariant& value) override;
};
