#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WLUTAssetDocument;

class WQtLUTAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtLUTAssetDocumentWindow(WLUTAssetDocument* pDocument);
};

class WLUTAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sMapping);
};
