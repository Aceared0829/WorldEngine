#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>


/// An object change starts at the heap object m_Root (because we can only safely store pointers to those).
///  From this object we follow m_Steps (member arrays, structs) to execute m_Change at the end target.
///
/// In case of an NodeAdded operation, m_GraphData contains the entire subgraph of this node.
class W_TOOLSFOUNDATION_DLL WObjectChange
{
public:
  WObjectChange() = default;
  WObjectChange(const WObjectChange&);
  WObjectChange(WObjectChange&& rhs);
  void operator=(WObjectChange&& rhs);
  void operator=(WObjectChange& rhs);
  void GetGraph(WAbstractObjectGraph& ref_graph) const;
  void SetGraph(WAbstractObjectGraph& ref_graph);

  WUuid m_Root;                                //< The object that is the parent of the op, namely the parent heap object we can store a pointer to.
  WHybridArray<WPropertyPathStep, 2> m_Steps; //< Path from root to target of change.
  WDiffOperation m_Change;                     //< Change at the target.
  WDataBuffer m_GraphData;                     //< In case of ObjectAdded, this holds the binary serialized object graph.
};
W_DECLARE_REFLECTABLE_TYPE(W_TOOLSFOUNDATION_DLL, WObjectChange);


class W_TOOLSFOUNDATION_DLL WDocumentObjectMirror
{
public:
  WDocumentObjectMirror();
  virtual ~WDocumentObjectMirror();

  void InitSender(const WDocumentObjectManager* pManager);
  void InitReceiver(WRttiConverterContext* pContext);
  void DeInit();

  using FilterFunction = WDelegate<bool(const WDocumentObject*, WStringView)>;

  /// \param filter
  ///   Filter that defines whether an object property should be mirrored or not.
  void SetFilterFunction(FilterFunction filter);

  void SendDocument();
  void Clear();

  void TreeStructureEventHandler(const WDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const WDocumentObjectPropertyEvent& e);

  void* GetNativeObjectPointer(const WDocumentObject* pObject);
  const void* GetNativeObjectPointer(const WDocumentObject* pObject) const;

protected:
  bool IsRootObject(const WDocumentObject* pParent);
  bool IsHeapAllocated(const WDocumentObject* pParent, WStringView sParentProperty);
  bool IsDiscardedByFilter(const WDocumentObject* pObject, WStringView sProperty) const;
  static void CreatePath(WObjectChange& out_change, const WDocumentObject* pRoot, WStringView sProperty);
  static WUuid FindRootOpObject(const WDocumentObject* pObject, WDynamicArray<const WDocumentObject*>& out_path);
  static void FlattenSteps(const WArrayPtr<const WDocumentObject* const> path, WDynamicArray<WPropertyPathStep>& out_steps);

  virtual void ApplyOp(WObjectChange& change);
  void ApplyOp(WRttiConverterObject object, const WObjectChange& change);

protected:
  WRttiConverterContext* m_pContext;
  const WDocumentObjectManager* m_pManager;
  FilterFunction m_Filter;
};
