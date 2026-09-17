#pragma once

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/EditorFrameworkDLL.h>

struct W_EDITORFRAMEWORK_DLL WGameObjectContextEvent
{
  enum class Type
  {
    ContextAboutToBeChanged,
    ContextChanged,
  };
  Type m_Type;
};

class W_EDITORFRAMEWORK_DLL WGameObjectContextDocument : public WGameObjectDocument
{
  W_ADD_DYNAMIC_REFLECTION(WGameObjectContextDocument, WGameObjectDocument);

public:
  WGameObjectContextDocument(WStringView sDocumentPath, WDocumentObjectManager* pObjectManager,
    WAssetDocEngineConnection engineConnectionType = WAssetDocEngineConnection::FullObjectMirroring);
  ~WGameObjectContextDocument();

  WStatus SetContext(WUuid documentGuid, WUuid objectGuid);
  WUuid GetContextDocumentGuid() const;
  WUuid GetContextObjectGuid() const;
  const WDocumentObject* GetContextObject() const;

  mutable WEvent<const WGameObjectContextEvent&> m_GameObjectContextEvents;

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

private:
  void ClearContext();

private:
  WUuid m_ContextDocument;
  WUuid m_ContextObject;
};
