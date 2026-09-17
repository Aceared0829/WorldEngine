#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtBlackboardTemplateAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtBlackboardTemplateAssetDocumentWindow(WDocument* pDocument);
  ~WQtBlackboardTemplateAssetDocumentWindow();

private:
  void UpdatePreview();
  void RestoreResource();

  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);
};
