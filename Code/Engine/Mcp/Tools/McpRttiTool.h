#pragma once

#include <Mcp/McpTool.h>

class WRTTI;
class WMcpJsonWriter;
class WAbstractProperty;
class WAbstractFunctionProperty;

/// Exposes the reflection data, so an agent can find out which types exist and what they look like.
///
/// Split into four narrow tools rather than one, because the codebase has well over a thousand reflected
/// types: a single 'dump everything' call would cost more tokens than a client can spend and would bury
/// whatever was actually asked for. The intended flow is rtti_find_types to narrow down to a few names,
/// then the detail tools for those.
///
/// Host independent, hence concrete and living in the Mcp library: reflection is the same system in a
/// game as in the editor, and it is how an agent finds out what a component or a game's own types look
/// like. Whatever the host has registered by the time of the call is what gets reported - in the editor
/// that includes the phantom types WPhantomRttiManager puts into WRTTI, which the same traversal picks
/// up without having to know about them.
class WMcpRttiTool : public WMcpToolProvider
{
  W_ADD_DYNAMIC_REFLECTION(WMcpRttiTool, WMcpToolProvider);

public:
  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

private:
  void ExecuteFindTypes(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteTypeInfo(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteTypeProperties(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteDerivedTypes(const WVariantDictionary& arguments, WMcpToolResult& out_result);

  static void WriteTypeFlags(WMcpJsonWriter& ref_writer, const WRTTI* pType);
  static void WritePropertyFlags(WMcpJsonWriter& ref_writer, WStringView sName, WBitflags<WPropertyFlags> flags);
  static void WriteAttributes(WMcpJsonWriter& ref_writer, WArrayPtr<const WPropertyAttribute* const> attributes);

  /// Writes one attribute as an object: its concrete type plus its reflected members.
  static void WriteAttribute(WMcpJsonWriter& ref_writer, const WPropertyAttribute* pAttr);

  /// Returns the attribute a variant points at, or nullptr if it does not hold one. Attributes nested
  /// inside another attribute arrive as a TypedPointer and would otherwise lose their contents.
  static const WPropertyAttribute* GetAttributeFromVariant(const WVariant& value);
  /// \param pOwnerType The type the property was listed for. Only used to look up its translations, which
  ///        are keyed on the declaring type - WAbstractProperty does not know which type that is.
  static void WriteProperty(WMcpJsonWriter& ref_writer, const WRTTI* pOwnerType, const WAbstractProperty* pProp);
  static void WriteFunction(WMcpJsonWriter& ref_writer, const WAbstractFunctionProperty* pFunc);
};
