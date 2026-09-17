#pragma once

#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

struct WVisualShaderPinDescriptor;

/// Visual graph pin for visual shader nodes.
///
/// Extends the base pin class with shader-specific metadata such as data type and tooltip information
/// derived from the pin descriptor.
class WVisualShaderPin : public WVisualGraphPin
{
  W_ADD_DYNAMIC_REFLECTION(WVisualShaderPin, WVisualGraphPin);

public:
  WVisualShaderPin(Type type, const WVisualShaderPinDescriptor* pDescriptor, const WDocumentObject* pObject);

  const WRTTI* GetDataType() const;
  const WString& GetTooltip() const;
  const WVisualShaderPinDescriptor* GetDescriptor() const { return m_pDescriptor; }

private:
  const WVisualShaderPinDescriptor* m_pDescriptor;
};

/// Object manager for visual shader graphs.
///
/// Manages the document representation of visual shader nodes and their connections.
/// Creates pins based on shader node type descriptors and validates connections based on data type compatibility.
/// Enforces constraints such as limiting the number of certain node types in a shader.
class WVisualShaderNodeManager : public WVisualGraphObjectManager
{
public:
  virtual bool InternalIsNode(const WDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const WDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetNodeCreationTemplates(WDynamicArray<WVisualGraphNodeDesc>& out_templates) const override;

  virtual WStatus InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_result) const override;

private:
  virtual WStatus InternalCanAdd(
    const WRTTI* pRtti, const WDocumentObject* pParent, WStringView sParentProperty, const WVariant& index) const override;

  WUInt32 CountNodesOfType(WVisualShaderNodeType::Enum type) const;
};
