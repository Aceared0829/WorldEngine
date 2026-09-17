#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class W_EDITORFRAMEWORK_DLL WViewActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  enum Flags
  {
    PerspectiveMode = W_BIT(0),
    RenderMode = W_BIT(1),
    ActivateRemoteProcess = W_BIT(2),
  };

  static void MapToolbarActions(WStringView sMapping, WUInt32 uiFlags);

  static WActionDescriptorHandle s_hRenderMode;
  static WActionDescriptorHandle s_hPerspective;
  static WActionDescriptorHandle s_hActivateRemoteProcess;
  static WActionDescriptorHandle s_hLinkDeviceCamera;
};

///
class W_EDITORFRAMEWORK_DLL WRenderModeAction : public WEnumerationMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WRenderModeAction, WEnumerationMenuAction);

public:
  WRenderModeAction(const WActionContext& context, const char* szName, const char* szIconPath);
  virtual WInt64 GetValue() const override;
  virtual void Execute(const WVariant& value) override;
};

///
class W_EDITORFRAMEWORK_DLL WPerspectiveAction : public WEnumerationMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WPerspectiveAction, WEnumerationMenuAction);

public:
  WPerspectiveAction(const WActionContext& context, const char* szName, const char* szIconPath);
  virtual WInt64 GetValue() const override;
  virtual void Execute(const WVariant& value) override;
};

class W_EDITORFRAMEWORK_DLL WViewAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WViewAction, WButtonAction);

public:
  enum class ButtonType
  {
    ActivateRemoteProcess,
    LinkDeviceCamera,
  };

  WViewAction(const WActionContext& context, const char* szName, ButtonType button);
  ~WViewAction();

  virtual void Execute(const WVariant& value) override;

private:
  ButtonType m_ButtonType;
};
