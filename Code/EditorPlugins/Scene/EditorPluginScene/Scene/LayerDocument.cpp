#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <SharedPluginScene/Common/Messages.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLayerDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WLayerDocument::WLayerDocument(WStringView sDocumentPath, WScene2Document* pParentScene)
  : WSceneDocument(sDocumentPath, WSceneDocument::DocumentType::Layer)
{
  m_pHostDocument = pParentScene;
}

WLayerDocument::~WLayerDocument() = default;

void WLayerDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

WVariant WLayerDocument::GetCreateEngineMetaData() const
{
  return m_pHostDocument->GetGuid();
}
