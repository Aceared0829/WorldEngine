#pragma once

#include <Mcp/McpTool.h>

class WDocument;
class WMcpJsonWriter;
struct WDocumentTypeDescriptor;

/// Lists, opens, creates, saves, closes and focuses editor documents.
///
/// 'document_types' is the type-level view - what can be created and with which file extension - and
/// is the call that has to come first, because every other tool here names a document type or a path
/// whose extension decides the type.
///
/// The tools that change something are deliberately explicit about it: closing discards unsaved work
/// unless told otherwise, and deleting rewrites other documents. Neither can ask, so both take the
/// decision as an argument (see Status.md, 'A tool must never wait for a human').
class WMcpDocumentTool : public WMcpToolProvider
{
  W_ADD_DYNAMIC_REFLECTION(WMcpDocumentTool, WMcpToolProvider);

public:
  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

private:
  void ExecuteListTypes(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteList(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteOpen(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteCreate(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteSave(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteClose(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteFocus(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteDelete(const WVariantDictionary& arguments, WMcpToolResult& out_result);

  /// Writes guid, path, type and modified state of one open document. Every tool here returns this
  /// for the document it acted on, so a caller never has to follow up with 'document_list' to learn
  /// the guid of something it just created.
  static void WriteDocumentIdentity(WMcpJsonWriter& ref_writer, const WDocument& document);
};
