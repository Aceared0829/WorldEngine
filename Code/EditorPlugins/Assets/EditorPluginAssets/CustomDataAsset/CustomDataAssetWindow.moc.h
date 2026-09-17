#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtCustomDataAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtCustomDataAssetDocumentWindow(WDocument* pDocument);
};
