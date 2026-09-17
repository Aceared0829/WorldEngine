#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtCollectionAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtCollectionAssetDocumentWindow(WDocument* pDocument);
  ~WQtCollectionAssetDocumentWindow();
};
