#pragma once

#include <EditorPluginFmod/SoundEventAsset/SoundEventAsset.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QLabel;
class QScrollArea;

class WSoundEventAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WSoundEventAssetDocumentWindow(WDocument* pDocument);
  ~WSoundEventAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "SoundEventAsset"; }

private Q_SLOTS:


private:
  void UpdatePreview();
  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);

  WSoundEventAssetDocument* m_pAssetDoc = nullptr;
};
