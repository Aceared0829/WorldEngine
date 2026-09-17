#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

#define PREFAB_DEBUG false

WString ToBinary(const WUuid& guid)
{
  WStringBuilder s, sResult;

  WUInt8* pBytes = (WUInt8*)&guid;

  for (WUInt32 i = 0; i < sizeof(WUuid); ++i)
  {
    s.SetFormat("{0}", WArgU((WUInt32)*pBytes, 2, true, 16, true));
    ++pBytes;

    sResult.Append(s.GetData());
  }

  return sResult;
}

void WPrefabUtils::LoadGraph(WAbstractObjectGraph& out_graph, WStringView sGraph)
{
  WPrefabCache::GetSingleton()->LoadGraph(out_graph, WStringView(sGraph));
}


WAbstractObjectNode* WPrefabUtils::GetFirstRootNode(WAbstractObjectGraph& ref_graph)
{
  auto& nodes = ref_graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    if (pNode->GetNodeName() == "ObjectTree")
    {
      for (const auto& ObjectTreeProp : pNode->GetProperties())
      {
        if (ObjectTreeProp.m_sPropertyName == "Children" && ObjectTreeProp.m_Value.IsA<WVariantArray>())
        {
          const WVariantArray& RootChildren = ObjectTreeProp.m_Value.Get<WVariantArray>();

          for (const WVariant& childGuid : RootChildren)
          {
            if (!childGuid.IsA<WUuid>())
              continue;

            const WUuid& rootObjectGuid = childGuid.Get<WUuid>();

            return ref_graph.GetNode(rootObjectGuid);
          }
        }
      }
    }
  }
  return nullptr;
}

void WPrefabUtils::GetRootNodes(WAbstractObjectGraph& ref_graph, WDynamicArray<WAbstractObjectNode*>& out_nodes)
{
  auto& nodes = ref_graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    if (pNode->GetNodeName() == "ObjectTree")
    {
      for (const auto& ObjectTreeProp : pNode->GetProperties())
      {
        if (ObjectTreeProp.m_sPropertyName == "Children" && ObjectTreeProp.m_Value.IsA<WVariantArray>())
        {
          const WVariantArray& RootChildren = ObjectTreeProp.m_Value.Get<WVariantArray>();

          for (const WVariant& childGuid : RootChildren)
          {
            if (!childGuid.IsA<WUuid>())
              continue;

            const WUuid& rootObjectGuid = childGuid.Get<WUuid>();

            out_nodes.PushBack(ref_graph.GetNode(rootObjectGuid));
          }

          return;
        }
      }

      return;
    }
  }
}

WUuid WPrefabUtils::GetPrefabRoot(const WDocumentObject* pObject, const WObjectMetaData<WUuid, WDocumentObjectMetaData>& documentObjectMetaData, WInt32* pDepth)
{
  auto pMeta = documentObjectMetaData.BeginReadMetaData(pObject->GetGuid());
  WUuid source = pMeta->m_CreateFromPrefab;
  documentObjectMetaData.EndReadMetaData();

  if (source.IsValid())
  {
    return pObject->GetGuid();
  }

  if (pObject->GetParent() != nullptr)
  {
    if (pDepth)
      *pDepth += 1;
    return GetPrefabRoot(pObject->GetParent(), documentObjectMetaData);
  }
  return WUuid();
}


WVariant WPrefabUtils::GetDefaultValue(const WAbstractObjectGraph& graph, const WUuid& objectGuid, WStringView sProperty, WVariant index, bool* pValueFound)
{
  if (pValueFound)
    *pValueFound = false;

  const WAbstractObjectNode* pNode = graph.GetNode(objectGuid);
  if (!pNode)
    return WVariant();

  const WAbstractObjectNode::Property* pProp = pNode->FindProperty(sProperty);
  if (pProp)
  {
    const WVariant& value = pProp->m_Value;

    if (value.IsA<WVariantArray>() && index.CanConvertTo<WUInt32>())
    {
      WUInt32 uiIndex = index.ConvertTo<WUInt32>();
      const WVariantArray& valueArray = value.Get<WVariantArray>();
      if (uiIndex < valueArray.GetCount())
      {
        if (pValueFound)
          *pValueFound = true;
        return valueArray[uiIndex];
      }
      return WVariant();
    }
    else if (value.IsA<WVariantDictionary>() && index.CanConvertTo<WString>())
    {
      WString sKey = index.ConvertTo<WString>();
      const WVariantDictionary& valueDict = value.Get<WVariantDictionary>();
      auto it = valueDict.Find(sKey);
      if (it.IsValid())
      {
        if (pValueFound)
          *pValueFound = true;
        return it.Value();
      }
      return WVariant();
    }
    if (pValueFound)
      *pValueFound = true;
    return value;
  }

  return WVariant();
}

void WPrefabUtils::WriteDiff(const WDeque<WAbstractGraphDiffOperation>& mergedDiff, WStringBuilder& out_sText)
{
  for (const auto& diff : mergedDiff)
  {
    WStringBuilder Data = ToBinary(diff.m_Node);

    switch (diff.m_Operation)
    {
      case WAbstractGraphDiffOperation::Op::NodeAdded:
      {
        out_sText.AppendFormat("<add> - {{0}} ({1})\n", Data, diff.m_sProperty);
      }
      break;

      case WAbstractGraphDiffOperation::Op::NodeRemoved:
      {
        out_sText.AppendFormat("<del> - {{0}}\n", Data);
      }
      break;

      case WAbstractGraphDiffOperation::Op::PropertyChanged:
        if (diff.m_Value.CanConvertTo<WString>())
          out_sText.AppendFormat("<set> - {{0}} - \"{1}\" = {2}\n", Data, diff.m_sProperty, diff.m_Value.ConvertTo<WString>());
        else
          out_sText.AppendFormat("<set> - {{0}} - \"{1}\" = xxx\n", Data, diff.m_sProperty);
        break;
    }
  }
}

void WPrefabUtils::Merge(const WAbstractObjectGraph& baseGraph, const WAbstractObjectGraph& leftGraph, const WAbstractObjectGraph& rightGraph, WDeque<WAbstractGraphDiffOperation>& out_mergedDiff)
{
  // debug output
  if (PREFAB_DEBUG)
  {
    {
      WFileWriter file;
      file.Open("C:\\temp\\Prefab - base.txt").IgnoreResult();
      WAbstractGraphDdlSerializer::Write(file, &baseGraph, nullptr, false, WOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }

    {
      WFileWriter file;
      file.Open("C:\\temp\\Prefab - template.txt").IgnoreResult();
      WAbstractGraphDdlSerializer::Write(file, &leftGraph, nullptr, false, WOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }

    {
      WFileWriter file;
      file.Open("C:\\temp\\Prefab - instance.txt").IgnoreResult();
      WAbstractGraphDdlSerializer::Write(file, &rightGraph, nullptr, false, WOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }
  }

  WDeque<WAbstractGraphDiffOperation> LeftToBase;
  leftGraph.CreateDiffWithBaseGraph(baseGraph, LeftToBase);
  WDeque<WAbstractGraphDiffOperation> RightToBase;
  rightGraph.CreateDiffWithBaseGraph(baseGraph, RightToBase);

  baseGraph.MergeDiffs(LeftToBase, RightToBase, out_mergedDiff);

  // debug output
  if (PREFAB_DEBUG)
  {
    WFileWriter file;
    file.Open("C:\\temp\\Prefab - diff.txt").IgnoreResult();

    WStringBuilder sDiff;
    sDiff.Append("######## Template To Base #######\n");
    WPrefabUtils::WriteDiff(LeftToBase, sDiff);
    sDiff.Append("\n\n######## Instance To Base #######\n");
    WPrefabUtils::WriteDiff(RightToBase, sDiff);
    sDiff.Append("\n\n######## Merged Diff #######\n");
    WPrefabUtils::WriteDiff(out_mergedDiff, sDiff);


    file.WriteBytes(sDiff.GetData(), sDiff.GetElementCount()).IgnoreResult();
  }
}

void WPrefabUtils::Merge(WStringView sBase, WStringView sLeft, WDocumentObject* pRight, bool bRightIsNotPartOfPrefab, const WUuid& prefabSeed, WStringBuilder& out_sNewGraph)
{
  // prepare the original prefab as a graph
  WAbstractObjectGraph baseGraph;
  WPrefabUtils::LoadGraph(baseGraph, sBase);
  if (auto pHeader = baseGraph.GetNodeByName("Header"))
  {
    baseGraph.RemoveNode(pHeader->GetGuid());
  }

  {
    // read the new template as a graph
    WAbstractObjectGraph leftGraph;
    WPrefabUtils::LoadGraph(leftGraph, sLeft);
    if (auto pHeader = leftGraph.GetNodeByName("Header"))
    {
      leftGraph.RemoveNode(pHeader->GetGuid());
    }

    // prepare the current state as a graph
    WAbstractObjectGraph rightGraph;
    {
      WDocumentObjectConverterWriter writer(&rightGraph, pRight->GetDocumentObjectManager());

      WVariantArray children;
      if (bRightIsNotPartOfPrefab)
      {
        for (WDocumentObject* pChild : pRight->GetChildren())
        {
          writer.AddObjectToGraph(pChild);
          children.PushBack(pChild->GetGuid());
        }
      }
      else
      {
        writer.AddObjectToGraph(pRight);
        children.PushBack(pRight->GetGuid());
      }

      rightGraph.ReMapNodeGuids(prefabSeed, true);
      // just take the entire ObjectTree node as is TODO: this may cause a crash if the root object is replaced
      WAbstractObjectNode* pRightObjectTree = rightGraph.CopyNodeIntoGraph(leftGraph.GetNodeByName("ObjectTree"));
      // The root node should always have a property 'children' where all the root objects are attached to. We need to replace that property's value as the prefab instance graph can have less or more objects than the template.
      WAbstractObjectNode::Property* pChildrenProp = pRightObjectTree->FindProperty("Children");
      pChildrenProp->m_Value = children;
    }

    // Merge diffs relative to base
    WDeque<WAbstractGraphDiffOperation> mergedDiff;
    WPrefabUtils::Merge(baseGraph, leftGraph, rightGraph, mergedDiff);


    {
      // Apply merged diff to base.
      baseGraph.ApplyDiff(mergedDiff);

      WContiguousMemoryStreamStorage stor;
      WMemoryStreamWriter sw(&stor);

      WAbstractGraphDdlSerializer::Write(sw, &baseGraph, nullptr, true, WOpenDdlWriter::TypeStringMode::Shortest);

      out_sNewGraph.SetSubString_ElementCount((const char*)stor.GetData(), stor.GetStorageSize32());
    }

    // debug output
    if (PREFAB_DEBUG)
    {
      WFileWriter file;
      file.Open("C:\\temp\\Prefab - result.txt").IgnoreResult();
      WAbstractGraphDdlSerializer::Write(file, &baseGraph, nullptr, false, WOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }
  }
}

WString WPrefabUtils::ReadDocumentAsString(WStringView sFile)
{
  WFileReader file;
  if (file.Open(sFile) == W_FAILURE)
  {
    WLog::Error("Failed to open document file '{0}'", sFile);
    return WString();
  }

  WStringBuilder sGraph;
  sGraph.ReadAll(file);

  return sGraph;
}
