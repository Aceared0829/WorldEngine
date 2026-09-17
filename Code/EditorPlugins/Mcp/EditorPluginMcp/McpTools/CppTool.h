#pragma once

#include <Mcp/McpTool.h>

class WCppSettings;

/// The project's C++ plugin: whether it exists, generating it and compiling it.
///
/// Covers what the 'C++ Project' dialog and the Cpp actions in the project menu do, none of which are
/// reachable through action_execute, because they all end in a modal dialog. Everything here goes
/// through the WCppProject statics, which are dialog free.
class WMcpCppTool : public WMcpToolProvider
{
  W_ADD_DYNAMIC_REFLECTION(WMcpCppTool, WMcpToolProvider);

public:
  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

private:
  void ExecuteStatus(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteGenerate(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteBuild(const WVariantDictionary& arguments, WMcpToolResult& out_result);

  /// Common preconditions: a project has to be open and the C++ settings have to load.
  ///
  /// Returns false and fills out_result with the reason when a tool cannot run at all.
  static bool PrepareSettings(WCppSettings& out_settings, WMcpToolResult& out_result);

  /// The name the plugin binary is built under, which is not stored anywhere: an empty setting
  /// means the project name is used.
  static WString GetEffectivePluginName(const WCppSettings& settings);

  /// Absolute path of the built plugin library, i.e. what the editor loads.
  static WString GetPluginBinaryPath(const WCppSettings& settings);
};
