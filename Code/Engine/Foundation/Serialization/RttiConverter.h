#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class WAbstractObjectGraph;
class WAbstractObjectNode;

/// Simple wrapper that pairs a runtime type with an object instance pointer.
///
/// This structure is used throughout the RTTI converter system to maintain type safety
/// when working with void pointers. It ensures that object pointers are always associated
/// with their correct runtime type information.
struct W_FOUNDATION_DLL WRttiConverterObject
{
  WRttiConverterObject()
    : m_pType(nullptr)
    , m_pObject(nullptr)
  {
  }
  WRttiConverterObject(const WRTTI* pType, void* pObject)
    : m_pType(pType)
    , m_pObject(pObject)
  {
  }

  W_DECLARE_POD_TYPE();

  const WRTTI* m_pType; ///< Runtime type information for the object
  void* m_pObject;       ///< Pointer to the actual object instance
};


/// Context object that manages object lifetime and relationships during RTTI-based conversion.
///
/// This class provides the infrastructure for converting between native objects and abstract
/// object graphs. It handles object creation, deletion, GUID management, and type resolution
/// during both serialization and deserialization processes.
///
/// Key responsibilities:
/// - Object lifecycle management (creation, registration, deletion)
/// - GUID generation and object-to-GUID mapping
/// - Type resolution and unknown type handling
/// - Object queuing for deferred processing
/// - Cross-reference resolution during deserialization
///
/// The context can be customized by overriding virtual methods to implement:
/// - Custom GUID generation strategies
/// - Alternative object creation patterns
/// - Specialized type resolution logic
/// - Custom error handling for unknown types
class W_FOUNDATION_DLL WRttiConverterContext
{
public:
  /// Clears all cached objects and resets the context state.
  virtual void Clear();

  /// Generates a guid for a new object. Default implementation generates stable guids derived from
  /// parentGuid + property name + index and ignores the address of pObject.
  virtual WUuid GenerateObjectGuid(const WUuid& parentGuid, const WAbstractProperty* pProp, WVariant index, void* pObject) const;

  virtual WInternal::NewInstance<void> CreateObject(const WUuid& guid, const WRTTI* pRtti);
  virtual void DeleteObject(const WUuid& guid);

  virtual void RegisterObject(const WUuid& guid, const WRTTI* pRtti, void* pObject);
  virtual void UnregisterObject(const WUuid& guid);

  virtual WRttiConverterObject GetObjectByGUID(const WUuid& guid) const;
  virtual WUuid GetObjectGUID(const WRTTI* pRtti, const void* pObject) const;

  virtual const WRTTI* FindTypeByName(WStringView sName) const;

  template <typename T>
  void GetObjectsByType(WDynamicArray<T*>& out_objects, WDynamicArray<WUuid>* out_pUuids = nullptr)
  {
    for (auto it : m_GuidToObject)
    {
      if (it.Value().m_pType->IsDerivedFrom(WGetStaticRTTI<T>()))
      {
        out_objects.PushBack(static_cast<T*>(it.Value().m_pObject));
        if (out_pUuids)
        {
          out_pUuids->PushBack(it.Key());
        }
      }
    }
  }

  virtual WUuid EnqueObject(const WUuid& guid, const WRTTI* pRtti, void* pObject);
  virtual WRttiConverterObject DequeueObject();

  virtual void OnUnknownTypeError(WStringView sTypeName);

protected:
  WHashTable<WUuid, WRttiConverterObject> m_GuidToObject;
  mutable WHashTable<const void*, WUuid> m_ObjectToGuid;
  WSet<WUuid> m_QueuedObjects;
};


/// Converts native objects to abstract object graph representation using reflection.
///
/// This class traverses object hierarchies using RTTI and converts them into abstract
/// object graphs that can be serialized to various formats. It handles object references,
/// inheritance hierarchies, and complex property types automatically.
class W_FOUNDATION_DLL WRttiConverterWriter
{
public:
  /// Filter function type for controlling which properties are serialized.
  ///
  /// Return true to include the property, false to skip it. Allows fine-grained control
  /// over what gets serialized based on object state, property attributes, or other criteria.
  using FilterFunction = WDelegate<bool(const void* pObject, const WAbstractProperty* pProp)>;

  /// Constructs a writer with boolean flags for common filtering options.
  ///
  /// \param bSerializeReadOnly If true, includes read-only properties in the output
  /// \param bSerializeOwnerPtrs If true, serializes objects pointed to by owner pointers
  WRttiConverterWriter(WAbstractObjectGraph* pGraph, WRttiConverterContext* pContext, bool bSerializeReadOnly, bool bSerializeOwnerPtrs);

  /// Constructs a writer with a custom filter function for maximum control.
  ///
  /// The filter function is called for each property and can implement complex logic
  /// to determine what should be serialized.
  WRttiConverterWriter(WAbstractObjectGraph* pGraph, WRttiConverterContext* pContext, FilterFunction filter);

  WAbstractObjectNode* AddObjectToGraph(WReflectedClass* pObject, const char* szNodeName = nullptr)
  {
    return AddObjectToGraph(pObject->GetDynamicRTTI(), pObject, szNodeName);
  }
  WAbstractObjectNode* AddObjectToGraph(const WRTTI* pRtti, const void* pObject, const char* szNodeName = nullptr);

  void AddProperty(WAbstractObjectNode* pNode, const WAbstractProperty* pProp, const void* pObject);
  void AddProperties(WAbstractObjectNode* pNode, const WRTTI* pRtti, const void* pObject);

  WAbstractObjectNode* AddSubObjectToGraph(const WRTTI* pRtti, const void* pObject, const WUuid& guid, const char* szNodeName);

private:
  WRttiConverterContext* m_pContext = nullptr;
  WAbstractObjectGraph* m_pGraph = nullptr;
  FilterFunction m_Filter;
};

/// Converts abstract object graphs back to native objects using reflection.
///
/// This class performs the reverse operation of WRttiConverterWriter, reconstructing
/// native object hierarchies from abstract object graphs. It handles object creation,
/// property restoration, and reference resolution automatically.
class W_FOUNDATION_DLL WRttiConverterReader
{
public:
  /// Constructs a reader for the given object graph and context.
  WRttiConverterReader(const WAbstractObjectGraph* pGraph, WRttiConverterContext* pContext);

  WInternal::NewInstance<void> CreateObjectFromNode(const WAbstractObjectNode* pNode);
  void ApplyPropertiesToObject(const WAbstractObjectNode* pNode, const WRTTI* pRtti, void* pObject);

private:
  void ApplyProperty(void* pObject, const WAbstractProperty* pProperty, const WAbstractObjectNode::Property* pSource);
  void CallOnObjectCreated(const WAbstractObjectNode* pNode, const WRTTI* pRtti, void* pObject);

  WRttiConverterContext* m_pContext = nullptr;
  const WAbstractObjectGraph* m_pGraph = nullptr;
};
