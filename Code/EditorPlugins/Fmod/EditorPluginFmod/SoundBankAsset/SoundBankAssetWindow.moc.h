#pragma once

#include <EditorPluginFmod/SoundBankAsset/SoundBankAsset.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QLabel;
class QScrollArea;
class QtImageWidget;

class WSoundBankAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WSoundBankAssetDocumentWindow(WDocument* pDocument);

  virtual const char* GetGroupName() const { return "SoundBankAsset"; }

private:
  WSoundBankAssetDocument* m_pAssetDoc;
};
