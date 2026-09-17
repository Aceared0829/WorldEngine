#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>

WQtScenegraphModel::WQtScenegraphModel(const WDocumentObjectManager* pObjectManager, const WUuid& root)
  : WQtGameObjectModel(pObjectManager, root)
{
}

WQtScenegraphModel::~WQtScenegraphModel() = default;
