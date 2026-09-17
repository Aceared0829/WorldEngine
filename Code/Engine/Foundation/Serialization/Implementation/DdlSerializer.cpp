#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/GraphVersioning.h>

namespace
{
  WSerializedBlock* FindBlock(WDynamicArray<WSerializedBlock>& ref_blocks, WStringView sName)
  {
    for (auto& block : ref_blocks)
    {
      if (block.m_Name == sName)
      {
        return &block;
      }
    }
    return nullptr;
  }

  WSerializedBlock* FindHeaderBlock(WDynamicArray<WSerializedBlock>& ref_blocks, WInt32& out_iVersion)
  {
    WStringBuilder sHeaderName = "HeaderV";
    out_iVersion = 0;
    for (auto& block : ref_blocks)
    {
      if (block.m_Name.StartsWith(sHeaderName))
      {
        WResult res = WConversionUtils::StringToInt(block.m_Name.GetData() + sHeaderName.GetElementCount(), out_iVersion);
        if (res.Failed())
        {
          WLog::Error("Failed to parse version from header name '{0}'", block.m_Name);
        }
        return &block;
      }
    }
    return nullptr;
  }

  WSerializedBlock* GetOrCreateBlock(WDynamicArray<WSerializedBlock>& ref_blocks, WStringView sName)
  {
    WSerializedBlock* pBlock = FindBlock(ref_blocks, sName);
    if (!pBlock)
    {
      pBlock = &ref_blocks.ExpandAndGetRef();
      pBlock->m_Name = sName;
    }
    if (!pBlock->m_Graph)
    {
      pBlock->m_Graph = W_DEFAULT_NEW(WAbstractObjectGraph);
    }
    return pBlock;
  }
} // namespace

static void WriteGraph(WOpenDdlWriter& ref_writer, const WAbstractObjectGraph* pGraph, const char* szName)
{
  WMap<WStringView, const WVariant*> SortedProperties;

  ref_writer.BeginObject(szName);

  const auto& Nodes = pGraph->GetAllNodes();
  for (auto itNode = Nodes.GetIterator(); itNode.IsValid(); ++itNode)
  {
    const auto& node = *itNode.Value();

    ref_writer.BeginObject("o");

    {

      WOpenDdlUtils::StoreUuid(ref_writer, node.GetGuid(), "id");
      WOpenDdlUtils::StoreString(ref_writer, node.GetType(), "t");
      WOpenDdlUtils::StoreUInt32(ref_writer, node.GetTypeVersion(), "v");

      if (!node.GetNodeName().IsEmpty())
        WOpenDdlUtils::StoreString(ref_writer, node.GetNodeName(), "n");

      ref_writer.BeginObject("p");
      {
        for (const auto& prop : node.GetProperties())
          SortedProperties[prop.m_sPropertyName] = &prop.m_Value;

        for (auto it = SortedProperties.GetIterator(); it.IsValid(); ++it)
        {
          WOpenDdlUtils::StoreVariant(ref_writer, *it.Value(), it.Key());
        }

        SortedProperties.Clear();
      }
      ref_writer.EndObject();
    }
    ref_writer.EndObject();
  }

  ref_writer.EndObject();
}

void WAbstractGraphDdlSerializer::Write(WStreamWriter& inout_stream, const WAbstractObjectGraph* pGraph, const WAbstractObjectGraph* pTypesGraph,
  bool bCompactMmode, WOpenDdlWriter::TypeStringMode typeMode)
{
  WOpenDdlWriter writer;
  writer.SetOutputStream(&inout_stream);
  writer.SetCompactMode(bCompactMmode);
  writer.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Exact);
  writer.SetPrimitiveTypeStringMode(typeMode);

  if (typeMode != WOpenDdlWriter::TypeStringMode::Compliant)
    writer.SetIndentation(-1);

  Write(writer, pGraph, pTypesGraph);
}


void WAbstractGraphDdlSerializer::Write(
  WOpenDdlWriter& ref_writer, const WAbstractObjectGraph* pGraph, const WAbstractObjectGraph* pTypesGraph /*= nullptr*/)
{
  WriteGraph(ref_writer, pGraph, "Objects");
  if (pTypesGraph)
  {
    WriteGraph(ref_writer, pTypesGraph, "Types");
  }
}

static void ReadGraph(WAbstractObjectGraph* pGraph, const WOpenDdlReaderElement* pRoot)
{
  WStringBuilder tmp, tmp2;
  WVariant varTmp;

  for (const WOpenDdlReaderElement* pObject = pRoot->GetFirstChild(); pObject != nullptr; pObject = pObject->GetSibling())
  {
    const WOpenDdlReaderElement* pGuid = pObject->FindChildOfType(WOpenDdlPrimitiveType::Custom, "id");
    const WOpenDdlReaderElement* pType = pObject->FindChildOfType(WOpenDdlPrimitiveType::String, "t");
    const WOpenDdlReaderElement* pTypeVersion = pObject->FindChildOfType(WOpenDdlPrimitiveType::UInt32, "v");
    const WOpenDdlReaderElement* pName = pObject->FindChildOfType(WOpenDdlPrimitiveType::String, "n");
    const WOpenDdlReaderElement* pProps = pObject->FindChildOfType("p");

    if (pGuid == nullptr || pType == nullptr || pProps == nullptr)
    {
      W_REPORT_FAILURE("Object contains invalid elements");
      continue;
    }

    WUuid guid;
    if (WOpenDdlUtils::ConvertToUuid(pGuid, guid).Failed())
    {
      W_REPORT_FAILURE("Object has an invalid guid");
      continue;
    }

    tmp = pType->GetPrimitivesString()[0];

    if (pName)
      tmp2 = pName->GetPrimitivesString()[0];
    else
      tmp2.Clear();

    WUInt32 uiTypeVersion = 0;
    if (pTypeVersion)
    {
      uiTypeVersion = pTypeVersion->GetPrimitivesUInt32()[0];
    }

    auto* pNode = pGraph->AddNode(guid, tmp, uiTypeVersion, tmp2);

    for (const WOpenDdlReaderElement* pProp = pProps->GetFirstChild(); pProp != nullptr; pProp = pProp->GetSibling())
    {
      if (!pProp->HasName())
        continue;

      if (WOpenDdlUtils::ConvertToVariant(pProp, varTmp).Failed())
        continue;

      pNode->AddProperty(pProp->GetName(), varTmp);
    }
  }
}

WResult WAbstractGraphDdlSerializer::Read(
  WStreamReader& inout_stream, WAbstractObjectGraph* pGraph, WAbstractObjectGraph* pTypesGraph, bool bApplyPatches)
{
  WOpenDdlReader reader;
  if (reader.ParseDocument(inout_stream, 0, WLog::GetThreadLocalLogSystem()).Failed())
  {
    WLog::Error("Failed to parse DDL graph");
    return W_FAILURE;
  }

  return Read(reader.GetRootElement(), pGraph, pTypesGraph, bApplyPatches);
}


WResult WAbstractGraphDdlSerializer::Read(const WOpenDdlReaderElement* pRootElement, WAbstractObjectGraph* pGraph,
  WAbstractObjectGraph* pTypesGraph /*= nullptr*/, bool bApplyPatches /*= true*/)
{
  const WOpenDdlReaderElement* pObjects = pRootElement->FindChildOfType("Objects");
  if (pObjects != nullptr)
  {
    ReadGraph(pGraph, pObjects);
  }
  else
  {
    WLog::Error("DDL graph does not contain an 'Objects' root object");
    return W_FAILURE;
  }

  WAbstractObjectGraph* pTempTypesGraph = pTypesGraph;
  if (pTempTypesGraph == nullptr)
  {
    pTempTypesGraph = W_DEFAULT_NEW(WAbstractObjectGraph);
  }
  const WOpenDdlReaderElement* pTypes = pRootElement->FindChildOfType("Types");
  if (pTypes != nullptr)
  {
    ReadGraph(pTempTypesGraph, pTypes);
  }

  if (bApplyPatches)
  {
    if (pTempTypesGraph)
      WGraphVersioning::GetSingleton()->PatchGraph(pTempTypesGraph);
    WGraphVersioning::GetSingleton()->PatchGraph(pGraph, pTempTypesGraph);
  }

  if (pTypesGraph == nullptr)
    W_DEFAULT_DELETE(pTempTypesGraph);

  return W_SUCCESS;
}

WResult WAbstractGraphDdlSerializer::ReadBlocks(WStreamReader& stream, WDynamicArray<WSerializedBlock>& blocks)
{
  WOpenDdlReader reader;
  if (reader.ParseDocument(stream, 0, WLog::GetThreadLocalLogSystem()).Failed())
  {
    WLog::Error("Failed to parse DDL graph");
    return W_FAILURE;
  }

  const WOpenDdlReaderElement* pRoot = reader.GetRootElement();
  for (const WOpenDdlReaderElement* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    WSerializedBlock* pBlock = GetOrCreateBlock(blocks, pChild->GetCustomType());
    ReadGraph(pBlock->m_Graph.Borrow(), pChild);
  }
  return W_SUCCESS;
}

#define W_DOCUMENT_VERSION 2

void WAbstractGraphDdlSerializer::WriteDocument(WStreamWriter& inout_stream, const WAbstractObjectGraph* pHeader, const WAbstractObjectGraph* pGraph,
  const WAbstractObjectGraph* pTypes, bool bCompactMode, WOpenDdlWriter::TypeStringMode typeMode)
{
  WOpenDdlWriter writer;
  writer.SetOutputStream(&inout_stream);
  writer.SetCompactMode(bCompactMode);
  writer.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Exact);
  writer.SetPrimitiveTypeStringMode(typeMode);

  if (typeMode != WOpenDdlWriter::TypeStringMode::Compliant)
    writer.SetIndentation(-1);

  WStringBuilder sHeaderVersion;
  sHeaderVersion.SetFormat("HeaderV{0}", (int)W_DOCUMENT_VERSION);
  WriteGraph(writer, pHeader, sHeaderVersion);
  WriteGraph(writer, pGraph, "Objects");
  WriteGraph(writer, pTypes, "Types");
}

WResult WAbstractGraphDdlSerializer::ReadDocument(WStreamReader& inout_stream, WUniquePtr<WAbstractObjectGraph>& ref_pHeader,
  WUniquePtr<WAbstractObjectGraph>& ref_pGraph, WUniquePtr<WAbstractObjectGraph>& ref_pTypes, bool bApplyPatches)
{
  WTempHybridArray<WSerializedBlock, 3> blocks;
  if (ReadBlocks(inout_stream, blocks).Failed())
  {
    return W_FAILURE;
  }

  WInt32 iVersion = 2;
  WSerializedBlock* pHB = FindHeaderBlock(blocks, iVersion);
  WSerializedBlock* pOB = FindBlock(blocks, "Objects");
  WSerializedBlock* pTB = FindBlock(blocks, "Types");
  if (!pOB)
  {
    WLog::Error("No 'Objects' block in document");
    return W_FAILURE;
  }
  if (!pTB && !pHB)
  {
    iVersion = 0;
  }
  else if (!pHB)
  {
    iVersion = 1;
  }
  if (iVersion < 2)
  {
    // Move header into its own graph.
    WStringBuilder sHeaderVersion;
    sHeaderVersion.SetFormat("HeaderV{0}", iVersion);
    pHB = GetOrCreateBlock(blocks, sHeaderVersion);
    WAbstractObjectGraph& graph = *pOB->m_Graph.Borrow();
    if (auto* pHeaderNode = graph.GetNodeByName("Header"))
    {
      WAbstractObjectGraph& headerGraph = *pHB->m_Graph.Borrow();
      /*auto* pNewHeaderNode =*/headerGraph.CopyNodeIntoGraph(pHeaderNode);
      // pNewHeaderNode->AddProperty("DocVersion", iVersion);
      graph.RemoveNode(pHeaderNode->GetGuid());
    }
  }

  if (bApplyPatches && pTB)
  {
    WGraphVersioning::GetSingleton()->PatchGraph(pTB->m_Graph.Borrow());
    WGraphVersioning::GetSingleton()->PatchGraph(pHB->m_Graph.Borrow(), pTB->m_Graph.Borrow());
    WGraphVersioning::GetSingleton()->PatchGraph(pOB->m_Graph.Borrow(), pTB->m_Graph.Borrow());
  }

  ref_pHeader = std::move(pHB->m_Graph);
  ref_pGraph = std::move(pOB->m_Graph);
  if (pTB)
  {
    ref_pTypes = std::move(pTB->m_Graph);
  }

  return W_SUCCESS;
}

// This is a handcrafted DDL reader that ignores everything that is not an 'AssetInfo' object
// The purpose is to speed up reading asset information by skipping everything else
//
// Version 0 and 1:
// The reader 'knows' the file format details and uses them.
// Top-level (ie. depth 0) there is an "Objects" object -> we need to enter that
// Inside that (depth 1) there is the "AssetInfo" object -> need to enter that as well
// All objects inside that must be stored
// Once the AssetInfo object is left everything else can be skipped
//
// Version 2:
// The very first top level object is "Header" and only that is read and parsing is stopped afterwards.
class HeaderReader : public WOpenDdlReader
{
public:
  HeaderReader() = default;

  bool m_bHasHeader = false;
  WInt32 m_iDepth = 0;

  virtual void OnBeginObject(WStringView sType, WStringView sName, bool bGlobalName) override
  {
    //////////////////////////////////////////////////////////////////////////
    // New document format has header block.
    if (m_iDepth == 0 && sType.StartsWith("HeaderV"))
    {
      m_bHasHeader = true;
    }
    if (m_bHasHeader)
    {
      ++m_iDepth;
      WOpenDdlReader::OnBeginObject(sType, sName, bGlobalName);
      return;
    }

    //////////////////////////////////////////////////////////////////////////
    // Old header is stored in the object block.
    // not yet entered the "Objects" group
    if (m_iDepth == 0 && sType == "Objects")
    {
      ++m_iDepth;

      WOpenDdlReader::OnBeginObject(sType, sName, bGlobalName);
      return;
    }

    // not yet entered the "AssetInfo" group, but inside "Objects"
    if (m_iDepth == 1 && sType == "AssetInfo")
    {
      ++m_iDepth;

      WOpenDdlReader::OnBeginObject(sType, sName, bGlobalName);
      return;
    }

    // inside "AssetInfo"
    if (m_iDepth > 1)
    {
      ++m_iDepth;
      WOpenDdlReader::OnBeginObject(sType, sName, bGlobalName);
      return;
    }

    // ignore everything else
    SkipRestOfObject();
  }


  virtual void OnEndObject() override
  {
    --m_iDepth;
    if (m_bHasHeader)
    {
      if (m_iDepth == 0)
      {
        m_iDepth = -1;
        StopParsing();
      }
    }
    else
    {
      if (m_iDepth <= 1)
      {
        // we were inside "AssetInfo" or "Objects" and returned from it, so now skip the rest
        m_iDepth = -1;
        StopParsing();
      }
    }
    WOpenDdlReader::OnEndObject();
  }
};

WResult WAbstractGraphDdlSerializer::ReadHeader(WStreamReader& inout_stream, WAbstractObjectGraph* pGraph)
{
  HeaderReader reader;
  if (reader.ParseDocument(inout_stream, 0, WLog::GetThreadLocalLogSystem()).Failed())
  {
    W_REPORT_FAILURE("Failed to parse DDL graph");
    return W_FAILURE;
  }

  const WOpenDdlReaderElement* pObjects = nullptr;
  if (reader.m_bHasHeader)
  {
    pObjects = reader.GetRootElement()->GetFirstChild();
  }
  else
  {
    pObjects = reader.GetRootElement()->FindChildOfType("Objects");
  }

  if (pObjects != nullptr)
  {
    ReadGraph(pGraph, pObjects);
  }
  else
  {
    W_REPORT_FAILURE("DDL graph does not contain an 'Objects' root object");
    return W_FAILURE;
  }
  return W_SUCCESS;
}
