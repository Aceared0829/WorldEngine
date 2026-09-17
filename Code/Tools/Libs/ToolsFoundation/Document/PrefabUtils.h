#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WDocumentObject;

class W_TOOLSFOUNDATION_DLL WPrefabUtils
{
public:
  static void LoadGraph(WAbstractObjectGraph& out_graph, WStringView sGraph);

  static WAbstractObjectNode* GetFirstRootNode(WAbstractObjectGraph& ref_graph);

  static void GetRootNodes(WAbstractObjectGraph& ref_graph, WDynamicArray<WAbstractObjectNode*>& out_nodes);

  static WUuid GetPrefabRoot(const WDocumentObject* pObject, const WObjectMetaData<WUuid, WDocumentObjectMetaData>& documentObjectMetaData, WInt32* pDepth = nullptr);

  static WVariant GetDefaultValue(
    const WAbstractObjectGraph& graph, const WUuid& objectGuid, WStringView sProperty, WVariant index = WVariant(), bool* pValueFound = nullptr);

  static void WriteDiff(const WDeque<WAbstractGraphDiffOperation>& mergedDiff, WStringBuilder& out_sText);

  /// Merges diffs of left and right graphs relative to their base graph. Conflicts prefer the right graph.
  static void Merge(const WAbstractObjectGraph& baseGraph, const WAbstractObjectGraph& leftGraph, const WAbstractObjectGraph& rightGraph,
    WDeque<WAbstractGraphDiffOperation>& out_mergedDiff);

  /// Merges diffs of left and right graphs relative to their base graph. Conflicts prefer the right graph. Base and left are provided as
  /// serialized DDL graphs and the right graph is build directly from pRight and its PrefabSeed.
  static void Merge(WStringView sBase, WStringView sLeft, WDocumentObject* pRight, bool bRightIsNotPartOfPrefab, const WUuid& prefabSeed,
    WStringBuilder& out_sNewGraph);

  static WString ReadDocumentAsString(WStringView sFile);
};
