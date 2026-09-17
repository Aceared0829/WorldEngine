#pragma once

#include <Mcp/McpTool.h>

/// Information about the project that the editor currently has open.
///
/// This is what orients an agent that has just connected: every path an other tool returns is
/// relative to the data directories reported here, and asset states depend on the active profile.
class WMcpProjectTool : public WMcpToolProvider
{
  W_ADD_DYNAMIC_REFLECTION(WMcpProjectTool, WMcpToolProvider);

public:
  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

private:
  void ExecuteProjectInfo(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteProjectExport(const WVariantDictionary& arguments, WMcpToolResult& out_result);
};
