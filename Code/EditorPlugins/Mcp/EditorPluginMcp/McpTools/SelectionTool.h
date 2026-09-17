#pragma once

#include <Mcp/McpTool.h>

class WDocumentObject;
class WMcpJsonWriter;

/// Reads and sets the selection of an open document.
///
/// The selection is the one piece of editor state that is visible to both sides: it is what a user
/// means by "this object", and setting it is how an agent points at something instead of describing
/// it. It is per document and is not saved.
///
/// Selecting an object does not require its window to exist, but a document that was opened without
/// one shows nothing, so a selection meant for the user to see belongs to a document they have open.
class WMcpSelectionTool : public WMcpToolProvider
{
  W_ADD_DYNAMIC_REFLECTION(WMcpSelectionTool, WMcpToolProvider);

public:
  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

private:
  void ExecuteGet(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteSet(const WVariantDictionary& arguments, WMcpToolResult& out_result);

  /// Writes guid, name and type - enough to recognise an object without a second call per entry.
  static void WriteObject(WMcpJsonWriter& ref_writer, const WDocumentObject* pObject);
};
