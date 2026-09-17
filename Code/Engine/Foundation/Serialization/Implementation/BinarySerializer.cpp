#include <Foundation/FoundationPCH.h>

#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/GraphVersioning.h>

enum WBinarySerializerVersion : WUInt32
{
  InvalidVersion = 0,
  Version1,
  // << insert new versions here >>

  ENUM_COUNT,
  CurrentVersion = ENUM_COUNT - 1 // automatically the highest version number
};

static void WriteGraph(const WAbstractObjectGraph* pGraph, WStreamWriter& inout_stream)
{
  const auto& Nodes = pGraph->GetAllNodes();

  WUInt32 uiNodes = Nodes.GetCount();
  inout_stream << uiNodes;
  for (auto itNode = Nodes.GetIterator(); itNode.IsValid(); ++itNode)
  {
    const auto& node = *itNode.Value();
    inout_stream << node.GetGuid();
    inout_stream << node.GetType();
    inout_stream << node.GetTypeVersion();
    inout_stream << node.GetNodeName();

    const auto& properties = node.GetProperties();
    WUInt32 uiProps = properties.GetCount();
    inout_stream << uiProps;
    for (const WAbstractObjectNode::Property& prop : properties)
    {
      inout_stream << prop.m_sPropertyName;
      inout_stream << prop.m_Value;
    }
  }
}

void WAbstractGraphBinarySerializer::Write(WStreamWriter& inout_stream, const WAbstractObjectGraph* pGraph, const WAbstractObjectGraph* pTypesGraph)
{
  WUInt32 uiVersion = WBinarySerializerVersion::CurrentVersion;
  inout_stream << uiVersion;

  WriteGraph(pGraph, inout_stream);
  if (pTypesGraph)
  {
    WriteGraph(pTypesGraph, inout_stream);
  }
}

static void ReadGraph(WStreamReader& inout_stream, WAbstractObjectGraph* pGraph)
{
  WUInt32 uiNodes = 0;
  inout_stream >> uiNodes;
  for (WUInt32 uiNodeIdx = 0; uiNodeIdx < uiNodes; uiNodeIdx++)
  {
    WUuid guid;
    WUInt32 uiTypeVersion;
    WStringBuilder sType;
    WStringBuilder sNodeName;
    inout_stream >> guid;
    inout_stream >> sType;
    inout_stream >> uiTypeVersion;
    inout_stream >> sNodeName;
    WAbstractObjectNode* pNode = pGraph->AddNode(guid, sType, uiTypeVersion, sNodeName);
    WUInt32 uiProps = 0;
    inout_stream >> uiProps;
    for (WUInt32 propIdx = 0; propIdx < uiProps; ++propIdx)
    {
      WStringBuilder sPropName;
      WVariant value;
      inout_stream >> sPropName;
      inout_stream >> value;
      pNode->AddProperty(sPropName, value);
    }
  }
}

void WAbstractGraphBinarySerializer::Read(
  WStreamReader& inout_stream, WAbstractObjectGraph* pGraph, WAbstractObjectGraph* pTypesGraph, bool bApplyPatches)
{
  WUInt32 uiVersion = 0;
  inout_stream >> uiVersion;
  if (uiVersion != WBinarySerializerVersion::CurrentVersion)
  {
    W_REPORT_FAILURE(
      "Binary serializer version {0} does not match expected version {1}, re-export file.", uiVersion, WBinarySerializerVersion::CurrentVersion);
    return;
  }
  ReadGraph(inout_stream, pGraph);
  if (pTypesGraph)
  {
    ReadGraph(inout_stream, pTypesGraph);
  }

  if (bApplyPatches)
  {
    if (pTypesGraph)
      WGraphVersioning::GetSingleton()->PatchGraph(pTypesGraph);
    WGraphVersioning::GetSingleton()->PatchGraph(pGraph, pTypesGraph);
  }
}
