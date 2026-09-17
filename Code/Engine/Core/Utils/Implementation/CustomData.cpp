#include <Core/CorePCH.h>

#include <Core/Utils/CustomData.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCustomData, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WCustomData::Load(WAbstractObjectGraph& ref_graph, WRttiConverterContext& ref_context, const WAbstractObjectNode* pRootNode)
{
  WRttiConverterReader convRead(&ref_graph, &ref_context);
  convRead.ApplyPropertiesToObject(pRootNode, GetDynamicRTTI(), this);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCustomDataResourceBase, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCustomDataResourceBase::WCustomDataResourceBase()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WCustomDataResourceBase::~WCustomDataResourceBase() = default;

WResourceLoadDesc WCustomDataResourceBase::UnloadData(Unload WhatToUnload)
{
  W_IGNORE_UNUSED(WhatToUnload);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;
  return res;
}

WResourceLoadDesc WCustomDataResourceBase::UpdateContent_Internal(WStreamReader* Stream, const WRTTI& rtti)
{
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  WAbstractObjectGraph graph;
  WRttiConverterContext context;

  WAbstractGraphBinarySerializer::Read(*Stream, &graph);

  const WAbstractObjectNode* pRootNode = graph.GetNodeByName("root");

  if (pRootNode != nullptr && pRootNode->GetType() != rtti.GetTypeName())
  {
    WLog::Error("Expected WCustomData type '{}' but resource is of type '{}' ('{}')", rtti.GetTypeName(), pRootNode->GetType(), GetResourceIdOrDescription());

    // make sure we create a default-initialized object and don't deserialize data that happens to match
    pRootNode = nullptr;
  }

  CreateAndLoadData(graph, context, pRootNode);

  res.m_State = WResourceState::Loaded;
  return res;
}

W_STATICLINK_FILE(Core, Core_Utils_Implementation_CustomData);
