#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class WDocumentObjectManager;
class WDocumentObject;
class WRTTI;

/// Provides helper functions for serializing document object types and copying properties between objects.
///
/// Also check out WToolsReflectionUtils for related functionality.
class W_TOOLSFOUNDATION_DLL WToolsSerializationUtils
{
public:
  using FilterFunction = WDelegate<bool(const WAbstractProperty*)>;

  /// Serializes the given set of types into the provided object graph.
  static void SerializeTypes(const WSet<const WRTTI*>& types, WAbstractObjectGraph& ref_typesGraph);

  /// Copies properties from a source document object to a target object, optionally filtering properties.
  static void CopyProperties(const WDocumentObject* pSource, const WDocumentObjectManager* pSourceManager, void* pTarget, const WRTTI* pTargetType, FilterFunction propertFilter = nullptr);
};
