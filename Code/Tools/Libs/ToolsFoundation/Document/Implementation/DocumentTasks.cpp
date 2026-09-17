#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Document/DocumentTasks.h>

WSaveDocumentTask::WSaveDocumentTask()
{
  ConfigureTask("WSaveDocumentTask", WTaskNesting::Maybe);
}

WSaveDocumentTask::~WSaveDocumentTask() = default;

void WSaveDocumentTask::Execute()
{
  WAbstractGraphDdlSerializer::WriteDocument(file, &headerGraph, &objectGraph, &typesGraph, false);

  if (file.Close() == W_FAILURE)
  {
    m_document->m_LastSaveResult = WStatus(WFmt("Unable to open file '{0}' for writing!", m_document->m_sDocumentPath));
  }
  else
  {
    m_document->m_LastSaveResult = WStatus(W_SUCCESS);
  }
}

WAfterSaveDocumentTask::WAfterSaveDocumentTask()
{
  ConfigureTask("WAfterSaveDocumentTask", WTaskNesting::Maybe);
}

WAfterSaveDocumentTask::~WAfterSaveDocumentTask() = default;

void WAfterSaveDocumentTask::Execute()
{
  if (m_document->m_LastSaveResult.Succeeded())
  {
    WDocumentEvent e;
    e.m_pDocument = m_document;
    e.m_Type = WDocumentEvent::Type::DocumentSaved;
    m_document->m_EventsOne.Broadcast(e);
    m_document->s_EventsAny.Broadcast(e);

    m_document->SetModified(false);

    // after saving once, this information is pointless
    m_document->m_uiUnknownObjectTypeInstances = 0;
    m_document->m_UnknownObjectTypes.Clear();
  }

  if (m_document->m_LastSaveResult.Succeeded())
  {
    m_document->InternalAfterSaveDocument();
  }
  if (m_callback.IsValid())
  {
    m_callback(m_document, m_document->m_LastSaveResult);
  }
  m_document->m_ActiveSaveTask.Invalidate();
}
