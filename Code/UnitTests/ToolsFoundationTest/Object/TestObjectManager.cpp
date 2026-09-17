#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundationTest/Object/TestObjectManager.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTestDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WTestDocumentObjectManager::WTestDocumentObjectManager() = default;
WTestDocumentObjectManager::~WTestDocumentObjectManager() = default;

WTestDocument::WTestDocument(WStringView sDocumentPath, bool bUseIPCObjectMirror /*= false*/)
  : WDocument(sDocumentPath, W_DEFAULT_NEW(WTestDocumentObjectManager))
  , m_bUseIPCObjectMirror(bUseIPCObjectMirror)
{
}

WTestDocument::~WTestDocument()
{
  if (m_bUseIPCObjectMirror)
  {
    m_ObjectMirror.Clear();
    m_ObjectMirror.DeInit();
  }
}

void WTestDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  if (m_bUseIPCObjectMirror)
  {
    m_ObjectMirror.InitSender(GetObjectManager());
    m_ObjectMirror.InitReceiver(&m_Context);
    m_ObjectMirror.SendDocument();
  }
}

void WTestDocument::ApplyNativePropertyChangesToObjectManager(WDocumentObject* pObject)
{
  // Create native object graph
  WAbstractObjectGraph graph;
  WAbstractObjectNode* pRootNode = nullptr;
  {
    WRttiConverterWriter rttiConverter(&graph, &m_Context, true, true);
    pRootNode = rttiConverter.AddObjectToGraph(pObject->GetType(), m_ObjectMirror.GetNativeObjectPointer(pObject), "Object");
  }

  // Create object manager graph
  WAbstractObjectGraph origGraph;
  WAbstractObjectNode* pOrigRootNode = nullptr;
  {
    WDocumentObjectConverterWriter writer(&origGraph, GetObjectManager());
    pOrigRootNode = writer.AddObjectToGraph(pObject);
  }

  // Remap native guids so they match the object manager (stuff like embedded classes will not have a guid on the native side).
  graph.ReMapNodeGuidsToMatchGraph(pRootNode, origGraph, pOrigRootNode);
  WDeque<WAbstractGraphDiffOperation> diffResult;

  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  // As we messed up the native side the object mirror is no longer synced and needs to be destroyed.
  m_ObjectMirror.Clear();
  m_ObjectMirror.DeInit();

  // Apply diff while object mirror is down.
  GetObjectAccessor()->StartTransaction("Apply Native Property Changes to Object");
  WDocumentObjectConverterReader::ApplyDiffToObject(GetObjectAccessor(), pObject, diffResult);
  GetObjectAccessor()->FinishTransaction();

  // Restart mirror from scratch.
  m_ObjectMirror.InitSender(GetObjectManager());
  m_ObjectMirror.InitReceiver(&m_Context);
  m_ObjectMirror.SendDocument();
}

WDocumentInfo* WTestDocument::CreateDocumentInfo()
{
  return W_DEFAULT_NEW(WDocumentInfo);
}
