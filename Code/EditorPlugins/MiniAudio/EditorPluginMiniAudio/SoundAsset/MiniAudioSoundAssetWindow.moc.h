#pragma once

#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundAsset.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QLabel;
class QScrollArea;
class QtImageWidget;

class WMiniAudioSoundAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WMiniAudioSoundAssetDocumentWindow(WDocument* pDocument);

  virtual const char* GetGroupName() const { return "MiniAudioSoundAsset"; }

private:
  WMiniAudioSoundAssetDocument* m_pAssetDoc;
};
