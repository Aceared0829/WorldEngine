#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class WMiniAudioSoundComponentDragDropHandler : public WComponentDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WMiniAudioSoundComponentDragDropHandler, WComponentDragDropHandler);

public:
  float CanHandle(const WDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const WDragDropInfo* pInfo) override;
};
