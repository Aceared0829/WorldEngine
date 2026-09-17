#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Communication/Event.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

#include <QPointer>

class WImageDataAssetDocument;
struct WImageDataAssetEvent;

class WQtImageDataAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtImageDataAssetDocumentWindow(WImageDataAssetDocument* pDocument);

private:
  void ImageDataAssetEventHandler(const WImageDataAssetEvent& e);
  WEvent<const WImageDataAssetEvent&>::Unsubscriber m_EventUnsubscriper;

  void UpdatePreview();

  QPointer<WQtImageWidget> m_pImageWidget;
};
