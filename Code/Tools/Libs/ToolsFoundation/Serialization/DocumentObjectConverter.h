#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WObjectAccessorBase;

/// Writes the state of an WDocumentObject to an abstract graph.
///
/// This information can then be applied to another WDocument object through WDocumentObjectConverterReader,
/// or to entirely different class using WRttiConverterReader.
class W_TOOLSFOUNDATION_DLL WDocumentObjectConverterWriter
{
public:
  using FilterFunction = WDelegate<bool(const WDocumentObject*, const WAbstractProperty*)>;
  WDocumentObjectConverterWriter(WAbstractObjectGraph* pGraph, const WDocumentObjectManager* pManager, FilterFunction filter = FilterFunction())
  {
    m_pGraph = pGraph;
    m_pManager = pManager;
    m_Filter = filter;
  }

  WAbstractObjectNode* AddObjectToGraph(const WDocumentObject* pObject, WStringView sNodeName = nullptr);

private:
  void AddProperty(WAbstractObjectNode* pNode, const WAbstractProperty* pProp, const WDocumentObject* pObject);
  void AddProperties(WAbstractObjectNode* pNode, const WDocumentObject* pObject);

  WAbstractObjectNode* AddSubObjectToGraph(const WDocumentObject* pObject, WStringView sNodeName);

  const WDocumentObjectManager* m_pManager;
  WAbstractObjectGraph* m_pGraph;
  FilterFunction m_Filter;
  WSet<const WDocumentObject*> m_QueuedObjects;
};


/// Reads document objects from an abstract graph and reconstructs them in a document.
class W_TOOLSFOUNDATION_DLL WDocumentObjectConverterReader
{
public:
  enum class Mode
  {
    CreateOnly,
    CreateAndAddToDocument,
  };
  WDocumentObjectConverterReader(const WAbstractObjectGraph* pGraph, WDocumentObjectManager* pManager, Mode mode);

  WDocumentObject* CreateObjectFromNode(const WAbstractObjectNode* pNode);
  void ApplyPropertiesToObject(const WAbstractObjectNode* pNode, WDocumentObject* pObject);

  WUInt32 GetNumUnknownObjectCreations() const { return m_uiUnknownTypeInstances; }
  const WSet<WString>& GetUnknownObjectTypes() const { return m_UnknownTypes; }

  static void ApplyDiffToObject(WObjectAccessorBase* pObjectAccessor, const WDocumentObject* pObject, WDeque<WAbstractGraphDiffOperation>& ref_diff);

private:
  void AddObject(WDocumentObject* pObject, WDocumentObject* pParent, WStringView sParentProperty, WVariant index);
  void ApplyProperty(WDocumentObject* pObject, const WAbstractProperty* pProp, const WAbstractObjectNode::Property* pSource);
  static void ApplyDiff(WObjectAccessorBase* pObjectAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp,
    WAbstractGraphDiffOperation& op, WDeque<WAbstractGraphDiffOperation>& diff);

  Mode m_Mode;
  WDocumentObjectManager* m_pManager;
  const WAbstractObjectGraph* m_pGraph;
  WSet<WString> m_UnknownTypes;
  WUInt32 m_uiUnknownTypeInstances;
};
