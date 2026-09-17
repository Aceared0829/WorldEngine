#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class WSceneDocument;

class WQtScenegraphModel : public WQtGameObjectModel
{
  Q_OBJECT

public:
  WQtScenegraphModel(const WDocumentObjectManager* pObjectManager, const WUuid& root = WUuid());
  ~WQtScenegraphModel();
};
