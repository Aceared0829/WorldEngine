#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class WVisualShaderActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sMapping);

  static WActionDescriptorHandle s_hVisualShaderCategory;
  static WActionDescriptorHandle s_hCleanGraph;
};

class WVisualShaderAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WVisualShaderAction, WButtonAction);

public:
  WVisualShaderAction(const WActionContext& context, const char* szName);
  ~WVisualShaderAction();

  virtual void Execute(const WVariant& value) override;

private:
  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
};
