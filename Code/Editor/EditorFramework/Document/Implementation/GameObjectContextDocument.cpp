#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <EditorFramework/Preferences/GameObjectContextPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameObjectContextDocument, 2, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WGameObjectContextDocument::WGameObjectContextDocument(
  WStringView sDocumentPath, WDocumentObjectManager* pObjectManager, WAssetDocEngineConnection engineConnectionType)
  : WGameObjectDocument(sDocumentPath, pObjectManager, engineConnectionType)
{
}

WGameObjectContextDocument::~WGameObjectContextDocument() = default;

WStatus WGameObjectContextDocument::SetContext(WUuid documentGuid, WUuid objectGuid)
{
  if (!documentGuid.IsValid())
  {
    {
      WGameObjectContextEvent e;
      e.m_Type = WGameObjectContextEvent::Type::ContextAboutToBeChanged;
      m_GameObjectContextEvents.Broadcast(e);
    }
    ClearContext();
    {
      WGameObjectContextEvent e;
      e.m_Type = WGameObjectContextEvent::Type::ContextChanged;
      m_GameObjectContextEvents.Broadcast(e);
    }
    return WStatus(W_SUCCESS);
  }

  const WAbstractObjectGraph* pPrefab = WPrefabCache::GetSingleton()->GetCachedPrefabGraph(documentGuid);
  if (!pPrefab)
    return WStatus("Context document could not be loaded.");

  {
    WGameObjectContextEvent e;
    e.m_Type = WGameObjectContextEvent::Type::ContextAboutToBeChanged;
    m_GameObjectContextEvents.Broadcast(e);
  }
  ClearContext();
  WAbstractObjectGraph graph;
  pPrefab->Clone(graph);

  WRttiConverterContext context;
  WRttiConverterReader rttiConverter(&graph, &context);
  WDocumentObjectConverterReader objectConverter(&graph, GetObjectManager(), WDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  {
    W_PROFILE_SCOPE("Restoring Objects");
    auto* pRootNode = graph.GetNodeByName("ObjectTree");
    W_ASSERT_DEV(pRootNode->FindProperty("TempObjects") == nullptr, "TempObjects should not be serialized.");
    pRootNode->RenameProperty("Children", "TempObjects");
    objectConverter.ApplyPropertiesToObject(pRootNode, GetObjectManager()->GetRootObject());
  }
  {
    W_PROFILE_SCOPE("Restoring Meta-Data");
    RestoreMetaDataAfterLoading(graph, false);
  }
  {
    WGameObjectContextPreferencesUser* pPreferences = WPreferences::QueryPreferences<WGameObjectContextPreferencesUser>(this);
    m_ContextDocument = documentGuid;
    pPreferences->SetContextDocument(m_ContextDocument);

    const WDocumentObject* pContextObject = GetObjectManager()->GetObject(objectGuid);
    m_ContextObject = pContextObject ? objectGuid : WUuid();
    pPreferences->SetContextObject(m_ContextObject);
  }
  {
    WGameObjectContextEvent e;
    e.m_Type = WGameObjectContextEvent::Type::ContextChanged;
    m_GameObjectContextEvents.Broadcast(e);
  }
  return WStatus(W_SUCCESS);
}

WUuid WGameObjectContextDocument::GetContextDocumentGuid() const
{
  return m_ContextDocument;
}

WUuid WGameObjectContextDocument::GetContextObjectGuid() const
{
  return m_ContextObject;
}

const WDocumentObject* WGameObjectContextDocument::GetContextObject() const
{
  if (m_ContextDocument.IsValid())
  {
    if (m_ContextObject.IsValid())
    {
      return GetObjectManager()->GetObject(m_ContextObject);
    }
    return GetObjectManager()->GetRootObject();
  }
  return nullptr;
}

void WGameObjectContextDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  WGameObjectContextPreferencesUser* pPreferences = WPreferences::QueryPreferences<WGameObjectContextPreferencesUser>(this);
  SetContext(pPreferences->GetContextDocument(), pPreferences->GetContextObject()).LogFailure();
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

void WGameObjectContextDocument::ClearContext()
{
  m_ContextDocument = WUuid();
  m_ContextObject = WUuid();
  WDocumentObject* pRoot = GetObjectManager()->GetRootObject();
  WTempHybridArray<WVariant, 16> values;
  GetObjectAccessor()->GetValuesByName(pRoot, "TempObjects", values).AssertSuccess();
  for (WInt32 i = (WInt32)values.GetCount() - 1; i >= 0; --i)
  {
    WDocumentObject* pChild = GetObjectManager()->GetObject(values[i].Get<WUuid>());
    GetObjectManager()->RemoveObject(pChild);
    GetObjectManager()->DestroyObject(pChild);
  }
}
