#pragma once

#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>
#include <Mcp/McpTool.h>

/// Tools for listing and running long ops - the operations behind the "Long Ops" panel.
///
/// A long op is registered automatically for every component in a scene that has an WLongOpAttribute,
/// for instance baking a scene or placing reflection probes. They run in the engine process, so unlike
/// an editor action they are asynchronous: longop_execute therefore waits for the operation to finish
/// before it answers, which needs the host to keep pumping in between.
class WMcpLongOpTool : public WMcpToolProvider
{
  W_ADD_DYNAMIC_REFLECTION(WMcpLongOpTool, WMcpToolProvider);

public:
  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

private:
  void ExecuteList(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteRun(const WVariantDictionary& arguments, WMcpToolResult& out_result);

  /// Works out which operation the arguments name, either directly by guid or by document + component type.
  ///
  /// Writes the reason into out_result and fails when the arguments don't identify exactly one operation.
  /// The result is only a guid, so it stays valid to call repeatedly for the same (re-entered) request.
  static WResult ResolveOperation(class WLongOpControllerManager& ref_manager, const WVariantDictionary& arguments, WUuid& out_opGuid, WMcpToolResult& out_result);

  /// Guid of the operation that ExecuteRun() is currently waiting for, invalid while nothing is running.
  WUuid m_WaitingForOp;

  /// When the wait started, so that a long op that never finishes doesn't block the caller forever.
  WTime m_WaitStarted;
};
