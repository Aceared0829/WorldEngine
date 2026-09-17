#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <EditorPluginMcp/McpDocument.h>
#include <EditorPluginMcp/McpTools/LongOpTool.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>

#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMcpLongOpTool, 1, WRTTIDefaultAllocator<WMcpLongOpTool>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// A long op that hasn't finished by then is reported as still running rather than waited for further.
  constexpr WTime TimeOut = WTime::MakeFromMinutes(10);

  /// Writes what identifies one operation: which document and component it belongs to, and its state.
  void WriteOperation(WMcpJsonWriter& ref_writer, const WLongOpControllerManager::ProxyOpInfo& opInfo, WStringView sName = WStringView())
  {
    WStringBuilder sTmp;

    // Inside an array the object has to stay anonymous, inside another object it needs a name.
    ref_writer.BeginObject(sName);

    ref_writer.AddVariableString("guid", WConversionUtils::ToString(opInfo.m_OperationGuid, sTmp));
    ref_writer.AddVariableString("name", opInfo.m_pProxyOp->GetDisplayName());
    ref_writer.AddVariableString("type", opInfo.m_pProxyOp->GetDynamicRTTI()->GetTypeName());
    ref_writer.AddVariableString("component", WConversionUtils::ToString(opInfo.m_ComponentGuid, sTmp));

    if (WDocument* pDoc = WDocumentManager::GetDocumentByGuid(opInfo.m_DocumentGuid))
    {
      ref_writer.AddVariableString("document", pDoc->GetDocumentPath());

      // The component itself carries no name, so the object it sits on is what a user would recognize.
      if (const WDocumentObject* pComponent = pDoc->GetObjectManager()->GetObject(opInfo.m_ComponentGuid))
      {
        ref_writer.AddVariableString("componentType", pComponent->GetType()->GetTypeName());

        if (const WDocumentObject* pOwner = pComponent->GetParent())
        {
          ref_writer.AddVariableString("object", WMcpDocument::GetObjectName(pOwner));
        }
      }
    }

    ref_writer.AddVariableBool("running", opInfo.m_bIsRunning);
    ref_writer.AddVariableFloat("completion", opInfo.m_fCompletion);

    ref_writer.EndObject();
  }
} // namespace

void WMcpLongOpTool::GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const
{
  WMcpToolDesc& list = out_tools.ExpandAndGetRef();
  list.m_sName = "longop_list";
  list.m_sDescription = "Lists the long ops currently available - the entries of the editor's 'Long Ops' panel. One is "
                        "registered automatically for every component in an open scene that offers such an operation, for "
                        "instance WBakedProbesComponent (baking a scene). So a long op only exists while the document "
                        "holding its component is open: add the component with object_modify first if the one you want is "
                        "missing. Each entry "
                        "reports the guid that longop_execute takes, plus whether it is running and how far along it is.";
  list.m_sInputSchema = R"({"type":"object","properties":{)"
                        R"("document":{"type":"string","description":"Guid or path of an open document. Only list the long ops belonging to it."},)"
                        R"("contains":{"type":"string","description":"Only list long ops whose display name or component type contains this text, case insensitive."}}})";

  WMcpToolDesc& run = out_tools.ExpandAndGetRef();
  run.m_sName = "longop_execute";
  run.m_sDescription = "Runs one long op and waits for it to finish, then reports whether it succeeded. This is the "
                       "equivalent of pressing its button in the property grid or the 'Long Ops' panel.\n"
                       "The work happens in the engine process, and what it writes back is applied to the document as a "
                       "single undoable step - so after this returns, object_tree shows the result and object_undo reverts "
                       "it. Nothing is saved: call document_save to keep it.\n"
                       "Unlike action_execute this call is asynchronous internally, so it does not block the editor while "
                       "waiting. It can still take minutes for a large scene, so give curl a timeout of tens of minutes. "
                       "Progress is not reported while waiting; call longop_list from another connection to see it.";
  run.m_sInputSchema = R"({"type":"object","properties":{)"
                       R"("guid":{"type":"string","description":"Guid of the long op to run, as reported by longop_list."},)"
                       R"("document":{"type":"string","description":"Guid or path of an open document. Together with 'componentType' this picks the long op instead of naming its guid."},)"
                       R"("componentType":{"type":"string","description":"Type name of the component the long op belongs to, e.g. 'WBakedProbesComponent'. Needs 'document', and only works when that document has exactly one such component."}}})";
}

void WMcpLongOpTool::Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  if (sToolName == "longop_list")
  {
    ExecuteList(arguments, out_result);
  }
  else if (sToolName == "longop_execute")
  {
    ExecuteRun(arguments, out_result);
  }
}

void WMcpLongOpTool::ExecuteList(const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  const WStringView sDocument = WMcpJson::GetString(arguments, "document");
  const WStringView sContains = WMcpJson::GetString(arguments, "contains");

  WDocument* pDocument = nullptr;

  if (!sDocument.IsEmpty())
  {
    pDocument = WMcpDocument::Find(sDocument);

    if (pDocument == nullptr)
    {
      WMcpDocument::SetNotOpenError(out_result, sDocument);
      return;
    }
  }

  WLongOpControllerManager* pManager = WLongOpControllerManager::GetSingleton();

  if (pManager == nullptr)
  {
    out_result.SetError("The long op controller is not available.");
    return;
  }

  WMcpJsonWriter writer;
  writer.BeginObject();

  WUInt32 uiCount = 0;

  {
    W_LOCK(pManager->m_Mutex);

    writer.BeginArray("longOps");

    for (const auto& pOpInfo : pManager->GetOperations())
    {
      if (pDocument != nullptr && pOpInfo->m_DocumentGuid != pDocument->GetGuid())
        continue;

      if (!sContains.IsEmpty())
      {
        WStringBuilder sHaystack = pOpInfo->m_pProxyOp->GetDisplayName();

        if (WDocument* pDoc = WDocumentManager::GetDocumentByGuid(pOpInfo->m_DocumentGuid))
        {
          if (const WDocumentObject* pComponent = pDoc->GetObjectManager()->GetObject(pOpInfo->m_ComponentGuid))
          {
            sHaystack.Append(" ", pComponent->GetType()->GetTypeName());
          }
        }

        if (sHaystack.FindSubString_NoCase(sContains) == nullptr)
          continue;
      }

      WriteOperation(writer, *pOpInfo);
      ++uiCount;
    }

    writer.EndArray();
  }

  writer.AddVariableUInt32("count", uiCount);

  if (uiCount == 0)
  {
    writer.AddVariableString("note", "No long op matched. They only exist for components in open documents, so open the "
                                     "document and make sure it has a component that offers one.");
  }

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

WResult WMcpLongOpTool::ResolveOperation(WLongOpControllerManager& ref_manager, const WVariantDictionary& arguments, WUuid& out_opGuid, WMcpToolResult& out_result)
{
  const WStringView sGuid = WMcpJson::GetString(arguments, "guid");
  const WStringView sDocument = WMcpJson::GetString(arguments, "document");
  const WStringView sComponentType = WMcpJson::GetString(arguments, "componentType");

  if (!sGuid.IsEmpty())
  {
    if (WConversionUtils::TryConvertStringToUuid(sGuid, out_opGuid).Failed())
    {
      out_result.SetError("'guid' is not a valid guid. Take it from longop_list.");
      return W_FAILURE;
    }

    return W_SUCCESS;
  }

  if (sDocument.IsEmpty() || sComponentType.IsEmpty())
  {
    out_result.SetError("Pass either 'guid', or both 'document' and 'componentType', to say which long op to run. "
                        "longop_list reports both.");
    return W_FAILURE;
  }

  WDocument* pDocument = WMcpDocument::Find(sDocument);

  if (pDocument == nullptr)
  {
    WMcpDocument::SetNotOpenError(out_result, sDocument);
    return W_FAILURE;
  }

  WUInt32 uiMatches = 0;

  {
    W_LOCK(ref_manager.m_Mutex);

    for (const auto& pOpInfo : ref_manager.GetOperations())
    {
      if (pOpInfo->m_DocumentGuid != pDocument->GetGuid())
        continue;

      const WDocumentObject* pComponent = pDocument->GetObjectManager()->GetObject(pOpInfo->m_ComponentGuid);

      if (pComponent == nullptr || pComponent->GetType()->GetTypeName() != sComponentType)
        continue;

      out_opGuid = pOpInfo->m_OperationGuid;
      ++uiMatches;
    }
  }

  WStringBuilder sMsg;

  if (uiMatches == 0)
  {
    sMsg.SetFormat("No long op for a component of type '{}' in that document. Use longop_list to see what is there.", sComponentType);
    out_result.SetError(sMsg);
    return W_FAILURE;
  }

  if (uiMatches > 1)
  {
    sMsg.SetFormat("The document has {} components of type '{}', so 'componentType' does not identify one. Pass 'guid' instead.", uiMatches, sComponentType);
    out_result.SetError(sMsg);
    return W_FAILURE;
  }

  return W_SUCCESS;
}

void WMcpLongOpTool::ExecuteRun(const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  WLongOpControllerManager* pManager = WLongOpControllerManager::GetSingleton();

  if (pManager == nullptr)
  {
    out_result.SetError("The long op controller is not available.");
    return;
  }

  WUuid opGuid;
  if (ResolveOperation(*pManager, arguments, opGuid, out_result).Failed())
    return;

  // A re-entered call carries the same arguments, so it resolves to the operation that is already
  // being waited for and only has to look at its state. A different one is a second, concurrent
  // request, which this tool cannot serve because it keeps the wait state only once.
  if (m_WaitingForOp.IsValid())
  {
    if (m_WaitingForOp != opGuid)
    {
      out_result.SetError("Another long op is currently being waited for. Only one longop_execute can be in flight at a "
                          "time - watch the running one with longop_list and try again once it is done.");
      return;
    }

    bool bStillRunning = false;

    {
      W_LOCK(pManager->m_Mutex);

      if (auto pOpInfo = pManager->GetOperation(m_WaitingForOp))
      {
        bStillRunning = pOpInfo->m_bIsRunning;
      }
      else
      {
        // The operation disappeared, which happens when its component or document went away mid-run.
        m_WaitingForOp = WUuid();
        out_result.SetError("The long op disappeared while it was running. Its component or document was probably closed.");
        return;
      }
    }

    if (bStillRunning)
    {
      if (WTime::Now() - m_WaitStarted > TimeOut)
      {
        m_WaitingForOp = WUuid();
        out_result.SetError("Timed out waiting for the long op to finish. It is still running - use longop_list to watch it.");
        return;
      }

      out_result.m_bNotFinished = true;
      return;
    }

    // Finished. The proxy op has applied its result to the document by now, since that happens in
    // Finalize(), which runs before m_bIsRunning goes back to false.
    WMcpJsonWriter writer;
    writer.BeginObject();

    {
      W_LOCK(pManager->m_Mutex);

      if (auto pOpInfo = pManager->GetOperation(m_WaitingForOp))
      {
        WriteOperation(writer, *pOpInfo, "longOp");
        writer.AddVariableDouble("durationSeconds", pOpInfo->m_StartOrDuration.GetSeconds());
      }
    }

    writer.AddVariableString("note", "The long op finished. Whether it produced anything is up to the operation - check the "
                                     "editor log with log_read, and the document with object_tree. Nothing is saved yet.");
    writer.EndObject();

    m_WaitingForOp = WUuid();
    out_result.m_sText = writer.GetResult();
    return;
  }

  {
    W_LOCK(pManager->m_Mutex);

    auto pOpInfo = pManager->GetOperation(opGuid);

    if (pOpInfo == nullptr)
    {
      out_result.SetError("No long op with that guid. They are per editor session, so a guid from an earlier run is stale - "
                          "call longop_list again.");
      return;
    }

    if (pOpInfo->m_bIsRunning)
    {
      out_result.SetError("That long op is already running. Wait for it to finish, watching it with longop_list.");
      return;
    }
  }

  pManager->StartOperation(opGuid);

  m_WaitingForOp = opGuid;
  m_WaitStarted = WTime::Now();

  // Answer only once the operation is done, which needs the host to pump in between.
  out_result.m_bNotFinished = true;
}
