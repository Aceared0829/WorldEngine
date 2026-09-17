#pragma once

#include <Foundation/Serialization/RttiConverter.h>

/// Specialized context for tracking and applying native object changes to abstract object graphs.
///
/// This context enables a sophisticated bidirectional synchronization workflow between native
/// C++ objects and their serialized representations in WAbstractObjectGraph form. It ensures
/// that modifications made to native objects can be properly tracked and applied back to the
/// abstract representation while maintaining object identity through consistent GUID generation.
///
/// The key capability is generating GUIDs for native objects that exactly match the GUIDs
/// used in the original abstract object graph. This allows the system to correlate changes
/// made to native objects with their counterparts in the serialized form.
///
/// Typical workflow:
/// 1. Deserialize an abstract object graph to native objects
/// 2. Create this context to track GUID relationships
/// 3. Modify the native objects through normal C++ operations
/// 4. Use the context to detect and apply changes back to the abstract graph
/// 5. Serialize the updated abstract graph for persistence
///
/// This is particularly useful for:
/// - Editor scenarios where objects are modified through UI and need to be saved
/// - Undo/redo systems that operate on abstract object graphs
/// - Network synchronization where changes need to be transmitted efficiently
/// - Asset pipeline where native modifications need to be persisted
///
/// \sa WAbstractObjectGraph::ModifyNodeViaNativeCounterpart
class W_FOUNDATION_DLL WApplyNativePropertyChangesContext : public WRttiConverterContext
{
public:
  WApplyNativePropertyChangesContext(WRttiConverterContext& ref_source, const WAbstractObjectGraph& originalGraph);

  virtual WUuid GenerateObjectGuid(const WUuid& parentGuid, const WAbstractProperty* pProp, WVariant index, void* pObject) const override;

private:
  WRttiConverterContext& m_NativeContext;
  const WAbstractObjectGraph& m_OriginalGraph;
};
