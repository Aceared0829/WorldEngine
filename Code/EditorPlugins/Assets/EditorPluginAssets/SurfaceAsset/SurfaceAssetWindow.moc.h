#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtSurfaceAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtSurfaceAssetDocumentWindow(WDocument* pDocument);
};
