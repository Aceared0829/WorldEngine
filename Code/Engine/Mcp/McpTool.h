#pragma once

#include <Mcp/McpDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Variant.h>

/// Describes one callable function, as it appears in the MCP client's tool list.
struct W_MCP_DLL WMcpToolDesc
{
  /// The name the client calls. Must be unique across all providers. Convention is lower snake_case,
  /// prefixed with the area, e.g. 'log_write'.
  WString m_sName;

  /// What the tool does. This is the only thing an AI agent has to go by when deciding whether to call
  /// it, so be specific about what it returns and what it modifies.
  WString m_sDescription;

  /// The JSON schema of the arguments, as a raw JSON object. Empty means 'no arguments'.
  /// E.g. R"({"type":"object","properties":{"count":{"type":"number"}}})"
  WString m_sInputSchema;
};

/// The result of one tool call.
struct W_MCP_DLL WMcpToolResult
{
  /// The text handed back to the client. For structured data just write JSON in here - MCP has no
  /// separate typed result, everything is text content.
  WStringBuilder m_sText;

  /// If true, the client is told the call failed. The text should then say why.
  bool m_bIsError = false;

  /// Set this to have the call re-run later instead of answered now.
  ///
  /// For a tool that has to wait for the host to make progress - most obviously 'run for N frames',
  /// which cannot happen while the tool itself is holding the main thread. Setting this leaves the
  /// client waiting on its connection and re-enters the tool with the same arguments the next time the
  /// host pumps, until a call finally leaves it false.
  ///
  /// The tool therefore has to keep its own 'where was I' state across those calls, and has to give up
  /// on its own after a timeout: a host that stops pumping is indistinguishable from one that is still
  /// working, and a wait that never ends looks exactly like a hang.
  ///
  /// Only meaningful in a host that pumps repeatedly. m_sText is ignored while this is true.
  bool m_bNotFinished = false;

  void SetError(WStringView sMessage)
  {
    m_sText = sMessage;
    m_bIsError = true;
  }
};

/// Base class for everything that the MCP server exposes to AI agents.
///
/// One provider offers one or more tools, which is why they are grouped: 'log_write' and 'log_read'
/// share a ring buffer, so they belong to the same object.
///
/// Providers are found through reflection, so **any plugin can add its own** simply by deriving from
/// this class - there is no list to register with. The plugin only has to link against the Mcp library,
/// which works on both sides: an editor plugin adds editor tools, a runtime plugin adds tools to the
/// game process. See WMcpLogTool for the smallest complete example.
///
/// Everything happens on the main thread, so implementations may touch the host directly.
class W_MCP_DLL WMcpToolProvider : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WMcpToolProvider, WReflectedClass);

public:
  /// Called once, right after the provider was instantiated. Use it to hook into the editor.
  virtual void OnActivate() {}

  /// Called once, before the provider is destroyed.
  virtual void OnDeactivate() {}

  /// Appends the description of every tool that this provider implements.
  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const = 0;

  /// Executes one of the tools returned by GetSupportedTools().
  ///
  /// \param sToolName The tool to run. A provider with a single tool may ignore this.
  /// \param arguments The 'arguments' object of the call. Missing and mistyped values are normal -
  ///        the client is an AI and will get this wrong. Validate and report through out_result.
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) = 0;
};
