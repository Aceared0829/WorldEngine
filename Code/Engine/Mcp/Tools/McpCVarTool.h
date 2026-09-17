#pragma once

#include <Mcp/McpTool.h>

class WCVar;
class WMcpJsonWriter;

/// Reading and writing the CVars this process has registered.
///
/// CVars are where the engine and its plugins already expose their debug switches - render passes,
/// physics visualisation, AI overlays, resource management - so this reaches a large amount of existing
/// behaviour for very little code. Nothing has to be written per switch: a plugin that declares a CVar
/// is reachable the moment it is loaded.
///
/// Host independent, hence concrete and living in the Mcp library. It is most useful in a game process,
/// where the interesting switches are, but the editor registers its own and answers the same way.
class WMcpCVarTool : public WMcpToolProvider
{
  W_ADD_DYNAMIC_REFLECTION(WMcpCVarTool, WMcpToolProvider);

public:
  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

private:
  void ExecuteList(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteSet(const WVariantDictionary& arguments, WMcpToolResult& out_result);

  /// Writes one CVar as an object: name, type, current value, and what differs from it.
  static void WriteCVar(WMcpJsonWriter& ref_writer, const WCVar* pCVar);

  /// The 'Current' value of any CVar type, as the JSON type that matches it.
  static void WriteValue(WMcpJsonWriter& ref_writer, WStringView sFieldName, const WCVar* pCVar, WUInt32 uiWhichValue);

  /// The cap exists for the same reason as everywhere else - a process can register hundreds and an
  /// unfiltered dump would bury the answer.
  static constexpr WUInt32 s_uiMaxResults = 200;
};
