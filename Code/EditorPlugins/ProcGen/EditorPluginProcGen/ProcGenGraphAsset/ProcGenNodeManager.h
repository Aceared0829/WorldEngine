#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/VisualGraph/Connection.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>

/// Visual graph pin for procedural generation nodes.
///
/// Basic pin implementation for procedural generation graphs without additional metadata.
class WProcGenPin : public WVisualGraphPin
{
  W_ADD_DYNAMIC_REFLECTION(WProcGenPin, WVisualGraphPin);

public:
  using WVisualGraphPin::WVisualGraphPin;
};

/// Object manager for procedural generation graphs.
///
/// Manages nodes and connections for procedural generation systems, such as terrain generation or placement rules.
/// Validates connections between different types of procedural generation nodes.
class WProcGenNodeManager : public WVisualGraphObjectManager
{
public:
  virtual bool InternalIsNode(const WDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const WDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const override;

  virtual WStatus InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_result) const override;
};
