#include <Mcp/McpPCH.h>

#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>
#include <Mcp/McpServer.h>
#include <Mcp/Tools/McpAppTool.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMcpAppTool, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStringView WMcpAppTool::GetBuildTimestamp() const
{
  // when this library was compiled - a host that ships its tools in a separate, faster moving plugin
  // should override this with that plugin's own timestamp
  return __DATE__ " " __TIME__;
}

void WMcpAppTool::GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const
{
  const WStringView sHost = GetHostNoun();

  {
    WMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "app_quit";

    WStringBuilder sDesc;
    sDesc.SetFormat("Shuts this {0} process down. The response is sent before it exits, but every other tool "
                    "stops answering immediately afterwards - this is the last call to this {0}.\n",
      sHost);
    sDesc.Append(GetRelaunchHint());
    desc.m_sDescription = sDesc;

    desc.m_sInputSchema = R"({"type":"object","properties":{"discardChanges":{"type":"boolean","description":"Quit even though there are unsaved changes, losing them. Default false."}}})";
  }

  {
    WMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "app_ping";

    WStringBuilder sDesc;
    sDesc.SetFormat("Returns immediately, to check that the {0} is still responsive. Answering requires a working main "
                    "thread, so a reply means the {0} is genuinely usable, not merely that the port is still open. Use it "
                    "after a call that may have blocked - a modal dialog or a long running action leaves the process stuck "
                    "with its port bound but nothing being served, and this is the cheapest way to tell that apart from one "
                    "that is simply busy. If it does not answer within a few seconds, the process has to be killed by process "
                    "id; app_quit will not get through either. Get the process id from app_info up front, since it cannot be "
                    "retrieved once the {0} stops answering.",
      sHost);
    desc.m_sDescription = sDesc;

    desc.m_sInputSchema = R"({"type":"object","properties":{}})";
  }

  {
    WMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "app_info";

    WStringBuilder sDesc;
    sDesc.SetFormat("Returns how to talk to and restart this {0}: the port it serves MCP on, the executable that runs it, the "
                    "process id and the command line it was started with. Use it to launch another one the same way, or to "
                    "launch a replacement after app_quit - the executable path is not otherwise discoverable, and a different "
                    "port is what allows a second process to run alongside this one. Also returns where this process writes "
                    "its log on disk, which is worth reading when it stopped answering: log_read only serves messages from a "
                    "live process, and only from this one.",
      sHost);
    desc.m_sDescription = sDesc;

    desc.m_sInputSchema = R"({"type":"object","properties":{}})";
  }

  {
    WMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "app_command_line_options";

    WStringBuilder sDesc;
    sDesc.SetFormat("Lists every command line option this {0} supports, with its type, default value and description. This is "
                    "more complete than running it with '-help': options declared by plugins - the MCP port among them - only "
                    "exist once their plugin is loaded, which happens after '-help' has already printed. Use it to find out "
                    "how to launch a process with a particular configuration.",
      sHost);
    desc.m_sDescription = sDesc;

    desc.m_sInputSchema = R"({"type":"object","properties":{"contains":{"type":"string","description":"Only return options whose name or description contains this text, case insensitive. Omit for all of them."}}})";
  }
}

void WMcpAppTool::Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  if (sToolName == "app_ping")
  {
    ExecutePing(arguments, out_result);
  }
  else if (sToolName == "app_quit")
  {
    ExecuteQuit(arguments, out_result);
  }
  else if (sToolName == "app_info")
  {
    ExecuteInfo(arguments, out_result);
  }
  else if (sToolName == "app_command_line_options")
  {
    ExecuteCommandLineOptions(arguments, out_result);
  }
}

void WMcpAppTool::ExecutePing(const WVariantDictionary& /*arguments*/, WMcpToolResult& out_result)
{
  WMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableBool("responsive", true);

  // Included so that a caller which pings once at the start has the process id on hand later, when a
  // blocked process cannot be asked for anything at all and killing it is the only option left.
  writer.AddVariableUInt32("processId", WProcess::GetCurrentProcessID());

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void WMcpAppTool::ExecuteCommandLineOptions(const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  const WStringView sContains = WMcpJson::GetString(arguments, "contains");

  WMcpJsonWriter writer;
  writer.BeginObject();

  WUInt32 uiTotalOptions = 0;
  WUInt32 uiReturned = 0;

  WStringBuilder sOptions, sShortDesc, sDefaultValue, sLongDesc, sGroup;

  writer.BeginArray("options");

  // The options register themselves into this list from wherever they are declared, including plugin
  // DLLs, so anything loaded by now is covered.
  for (WCommandLineOption* pOpt = WCommandLineOption::GetFirstInstance(); pOpt != nullptr; pOpt = pOpt->GetNextInstance())
  {
    pOpt->GetOptions(sOptions);
    pOpt->GetLongDesc(sLongDesc);
    sLongDesc.Trim(" \t\n\r");

    ++uiTotalOptions;

    if (!sContains.IsEmpty())
    {
      if (sOptions.FindSubString_NoCase(sContains) == nullptr && sLongDesc.FindSubString_NoCase(sContains) == nullptr)
        continue;
    }

    pOpt->GetParamShortDesc(sShortDesc);
    pOpt->GetParamDefaultValueDesc(sDefaultValue);
    pOpt->GetSortingGroup(sGroup);

    writer.BeginObject();

    // One option can have several spellings, e.g. '-h;-help;-?', so they are reported as a list
    // rather than as the raw separated string.
    WTempHybridArray<WStringView, 4> names;
    sOptions.Split(false, names, ";", "|");

    writer.BeginArray("names");
    for (WStringView sName : names)
    {
      writer.WriteString(sName);
    }
    writer.EndArray();

    writer.AddVariableString("type", pOpt->GetType());

    if (!sShortDesc.IsEmpty())
    {
      writer.AddVariableString("parameter", sShortDesc);
    }

    if (!sDefaultValue.IsEmpty())
    {
      writer.AddVariableString("default", sDefaultValue);
    }

    if (!sLongDesc.IsEmpty())
    {
      writer.AddVariableString("description", sLongDesc);
    }

    if (!sGroup.IsEmpty())
    {
      writer.AddVariableString("group", sGroup);
    }

    writer.EndObject();

    ++uiReturned;
  }

  writer.EndArray();

  // Not capped, unlike the listing tools: the number of options is bounded by what is compiled in and
  // is small, so there is nothing to truncate and no 'truncated' flag to report. 'totalOptions' is
  // still worth having, because it tells a filtered call how much it filtered away.
  writer.AddVariableUInt32("returned", uiReturned);
  writer.AddVariableUInt32("totalOptions", uiTotalOptions);

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void WMcpAppTool::ExecuteInfo(const WVariantDictionary& /*arguments*/, WMcpToolResult& out_result)
{
  const WMcpServer* pServer = WMcpServer::GetInstance();
  const WUInt16 uiPort = pServer != nullptr ? pServer->GetPort() : 0;

  WMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("executable", WOSFile::GetApplicationPath());
  writer.AddVariableUInt32("processId", WProcess::GetCurrentProcessID());

  // Compare this against when the source last changed before concluding that a tool is missing: a
  // binary that predates the feature reports exactly the same tool list as one that never had it.
  writer.AddVariableString("buildTimestamp", GetBuildTimestamp());

  writer.AddVariableUInt32("mcpPort", uiPort);

  WStringBuilder sUrl;
  sUrl.SetFormat("http://127.0.0.1:{}/mcp", uiPort);
  writer.AddVariableString("mcpUrl", sUrl);

  // The command line this process was started with, so that another one can be launched the same way.
  // Whether the executable itself appears as the first entry is platform dependent, which is why it
  // is also reported on its own above.
  const WCommandLineUtils* pCmd = WCommandLineUtils::GetGlobalInstance();

  writer.BeginArray("commandLine");
  for (WUInt32 i = 0; i < pCmd->GetParameterCount(); ++i)
  {
    writer.WriteString(pCmd->GetParameter(i));
  }
  writer.EndArray();

  AddHostInfo(writer);

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void WMcpAppTool::ExecuteQuit(const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  const bool bDiscardChanges = WMcpJson::GetBool(arguments, "discardChanges", false);

  const bool bRefused = CanQuit(bDiscardChanges).Failed();

  WMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableBool("quitting", !bRefused);

  if (bRefused)
  {
    AddQuitRefusalInfo(writer);
    writer.EndObject();

    // an error, so the agent cannot mistake this for a successful shutdown and stop waiting
    out_result.m_sText = writer.GetResult();
    out_result.m_bIsError = true;
    return;
  }

  AddQuitInfo(writer);
  writer.EndObject();
  out_result.m_sText = writer.GetResult();

  RequestQuit(bDiscardChanges);
}
