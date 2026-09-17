#include <Mcp/McpPCH.h>

#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>
#include <Mcp/Tools/McpLogTool.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMcpLogTool, 1, WRTTIDefaultAllocator<WMcpLogTool>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// The names used in the tool arguments and in the results. Kept short and lower case, because this
  /// is what the AI has to type.
  WStringView SeverityToString(WLogMsgType::Enum type)
  {
    switch (type)
    {
      case WLogMsgType::ErrorMsg:
        return "error";
      case WLogMsgType::SeriousWarningMsg:
        return "serious-warning";
      case WLogMsgType::WarningMsg:
        return "warning";
      case WLogMsgType::SuccessMsg:
        return "success";
      case WLogMsgType::InfoMsg:
        return "info";
      case WLogMsgType::DevMsg:
        return "dev";
      case WLogMsgType::DebugMsg:
        return "debug";
      default:
        return "other";
    }
  }

  WLogMsgType::Enum SeverityFromString(WStringView sSeverity, WLogMsgType::Enum fallback)
  {
    if (sSeverity.IsEqual_NoCase("error"))
      return WLogMsgType::ErrorMsg;
    if (sSeverity.IsEqual_NoCase("serious-warning"))
      return WLogMsgType::SeriousWarningMsg;
    if (sSeverity.IsEqual_NoCase("warning"))
      return WLogMsgType::WarningMsg;
    if (sSeverity.IsEqual_NoCase("success"))
      return WLogMsgType::SuccessMsg;
    if (sSeverity.IsEqual_NoCase("info"))
      return WLogMsgType::InfoMsg;
    if (sSeverity.IsEqual_NoCase("dev"))
      return WLogMsgType::DevMsg;
    if (sSeverity.IsEqual_NoCase("debug"))
      return WLogMsgType::DebugMsg;

    return fallback;
  }
} // namespace

WMcpLogTool::WMcpLogTool() = default;

WMcpLogTool::~WMcpLogTool()
{
  OnDeactivate();
}

void WMcpLogTool::OnActivate()
{
  if (m_LogSubscription != 0)
    return;

  // Hooked from OnActivate rather than when the server starts, because the provider is instantiated
  // during plugin load and startup messages are exactly the ones worth having captured.
  //
  // This sees only the log of the process it runs in. The editor's engine process is a separate one, so
  // an agent diagnosing something that happens while the game runs has to ask the engine's own server.
  m_LogSubscription = WGlobalLog::AddLogWriter(WMakeDelegate(&WMcpLogTool::LogEventHandler, this));
}

void WMcpLogTool::OnDeactivate()
{
  if (m_LogSubscription == 0)
    return;

  WGlobalLog::RemoveLogWriter(m_LogSubscription);
  m_LogSubscription = 0;
}

void WMcpLogTool::LogEventHandler(const WLoggingEventData& e)
{
  // group markers and flushes carry no text worth keeping
  if (e.m_EventType < WLogMsgType::ErrorMsg || e.m_EventType > WLogMsgType::DebugMsg)
    return;

  W_LOCK(m_Mutex);

  if (m_Entries.GetCount() >= s_uiMaxEntries)
  {
    m_Entries.PopFront();
  }

  Entry& entry = m_Entries.ExpandAndGetRef();
  entry.m_uiId = m_uiNextId++;
  entry.m_Type = e.m_EventType;
  entry.m_sText = e.m_sText;
}

void WMcpLogTool::GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const
{
  {
    WMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "log_write";
    desc.m_sDescription = "Writes a message to this application's log, so that it becomes visible to the user.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("message":{"type":"string","description":"The text to log."},)"
                          R"("severity":{"type":"string","enum":["error","serious-warning","warning","success","info","dev","debug"],"description":"Defaults to 'info'."})"
                          R"(},"required":["message"]})";
  }

  {
    WMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "log_read";
    desc.m_sDescription = "Returns the most recent log messages of THIS process, oldest first. Use this to find out what the "
                          "application reported about an operation, e.g. why an asset failed to transform. Note that the "
                          "editor and the engine process that runs the game are separate processes with separate logs, each "
                          "reachable only through its own MCP server. The response's 'lastId' is "
                          "the id of the newest entry returned; pass it back as 'sinceId' on the next call to only get "
                          "messages that arrived after it, instead of re-reading ones already seen. 'contains' lets 'count' "
                          "reach past the most recent messages to find one further back.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("count":{"type":"number","description":"How many messages to return. Defaults to 50."},)"
                          R"("severity":{"type":"string","enum":["error","serious-warning","warning","success","info","dev","debug"],"description":"Only return messages at least this severe. Defaults to 'debug', i.e. everything."},)"
                          R"("sinceId":{"type":"number","description":"Only return messages with an id greater than this, i.e. logged after a previous log_read whose 'lastId' this is. Omit to read the newest messages regardless of what was read before."},)"
                          R"("contains":{"type":"string","description":"Only return messages whose text contains this, case insensitive. Lets 'count' reach further back than the most recent messages."})"
                          R"(}})";
  }
}

void WMcpLogTool::Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  if (sToolName == "log_write")
  {
    ExecuteWrite(arguments, out_result);
  }
  else if (sToolName == "log_read")
  {
    ExecuteRead(arguments, out_result);
  }
}

void WMcpLogTool::ExecuteWrite(const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  const WStringView sMessage = WMcpJson::GetString(arguments, "message");

  if (sMessage.IsEmpty())
  {
    out_result.SetError("No 'message' argument given.");
    return;
  }

  const WLogMsgType::Enum severity = SeverityFromString(WMcpJson::GetString(arguments, "severity"), WLogMsgType::InfoMsg);

  // the tag makes it obvious in the log where this came from
  switch (severity)
  {
    case WLogMsgType::ErrorMsg:
      WLog::Error("MCP: {}", sMessage);
      break;
    case WLogMsgType::SeriousWarningMsg:
      WLog::SeriousWarning("MCP: {}", sMessage);
      break;
    case WLogMsgType::WarningMsg:
      WLog::Warning("MCP: {}", sMessage);
      break;
    case WLogMsgType::SuccessMsg:
      WLog::Success("MCP: {}", sMessage);
      break;
    case WLogMsgType::DevMsg:
      WLog::Dev("MCP: {}", sMessage);
      break;
    case WLogMsgType::DebugMsg:
      WLog::Debug("MCP: {}", sMessage);
      break;
    default:
      WLog::Info("MCP: {}", sMessage);
      break;
  }

  out_result.m_sText = "ok";
}

void WMcpLogTool::ExecuteRead(const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  const WInt64 iCount = WMath::Clamp<WInt64>(WMcpJson::GetInt(arguments, "count", 50), 1, s_uiMaxEntries);
  const WLogMsgType::Enum minSeverity = SeverityFromString(WMcpJson::GetString(arguments, "severity"), WLogMsgType::DebugMsg);
  const WUInt64 uiSinceId = static_cast<WUInt64>(WMath::Max<WInt64>(WMcpJson::GetInt(arguments, "sinceId", 0), 0));
  const WStringView sContains = WMcpJson::GetString(arguments, "contains");

  W_LOCK(m_Mutex);

  // collect from the back, so 'count' means 'the newest count that pass the filter'
  WHybridArray<const Entry*, 64> selected;

  for (WUInt32 i = m_Entries.GetCount(); i > 0 && selected.GetCount() < iCount; --i)
  {
    const Entry& entry = m_Entries[i - 1];

    if (entry.m_uiId <= uiSinceId)
      break; // entries only get older from here, nothing further back can pass either

    // lower enum value means more severe
    if (entry.m_Type > minSeverity)
      continue;

    if (!sContains.IsEmpty() && entry.m_sText.FindSubString_NoCase(sContains) == nullptr)
      continue;

    selected.PushBack(&entry);
  }

  // the newest id actually seen by this call, i.e. what a follow-up call should pass as 'sinceId';
  // falls back to whatever was asked for so a client that already knows it stays in sync
  const WUInt64 uiLastId = selected.IsEmpty() ? uiSinceId : selected[0]->m_uiId;

  WMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableInt64("lastId", static_cast<WInt64>(uiLastId));
  writer.BeginArray("messages");

  // 'selected' is newest first, but the result reads better oldest first
  for (WUInt32 i = selected.GetCount(); i > 0; --i)
  {
    const Entry& entry = *selected[i - 1];

    writer.BeginObject();
    writer.AddVariableInt64("id", static_cast<WInt64>(entry.m_uiId));
    writer.AddVariableString("severity", SeverityToString(entry.m_Type));
    writer.AddVariableString("text", entry.m_sText);
    writer.EndObject();
  }

  writer.EndArray();
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}
