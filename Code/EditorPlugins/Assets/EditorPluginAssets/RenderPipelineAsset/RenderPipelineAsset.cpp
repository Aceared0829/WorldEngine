#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>

#include <ToolsFoundation/VisualGraph/VisualGraphCommandAccessor.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelinePassGraph.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WRenderPipelineResourceType, 1)
  W_ENUM_CONSTANTS(WRenderPipelineResourceType::Texture, WRenderPipelineResourceType::Buffer)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineAssetPinInfo, WNoBase, 2, WRTTIDefaultAllocator<WRenderPipelineAssetPinInfo>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("ResourceType", WRenderPipelineResourceType, m_ResourceType),
    W_MEMBER_PROPERTY("Name", m_sName),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRenderPipelineAssetMetaData, 1, WRTTIDefaultAllocator<WRenderPipelineAssetMetaData>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Inputs", m_Inputs),
    W_ARRAY_MEMBER_PROPERTY("Outputs", m_Outputs),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRenderPipelineNodeGraphPin, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRenderPipelineAssetDocument, 6, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  WUInt64 ComputeRenderPipelineMetaDataHash(const WRenderPipelineAssetMetaData* pMetaData)
  {
    auto HashPin = [](const WRenderPipelineAssetPinInfo& pin, WUInt64& ref_uiHash)
    {
      const WUInt8 uiResourceType = pin.m_ResourceType.GetValue();
      ref_uiHash = WHashingUtils::xxHash64(&uiResourceType, sizeof(uiResourceType), ref_uiHash);
      ref_uiHash = WHashingUtils::xxHash64String(pin.m_sName, ref_uiHash);
    };

    if (pMetaData == nullptr)
      return 0;

    WUInt64 uiHash = 0;
    const WUInt32 uiInputCount = pMetaData->m_Inputs.GetCount();
    uiHash = WHashingUtils::xxHash64(&uiInputCount, sizeof(uiInputCount), uiHash);
    for (const WRenderPipelineAssetPinInfo& pin : pMetaData->m_Inputs)
    {
      HashPin(pin, uiHash);
    }

    const WUInt32 uiOutputCount = pMetaData->m_Outputs.GetCount();
    uiHash = WHashingUtils::xxHash64(&uiOutputCount, sizeof(uiOutputCount), uiHash);
    for (const WRenderPipelineAssetPinInfo& pin : pMetaData->m_Outputs)
    {
      HashPin(pin, uiHash);
    }

    return uiHash;
  }

  WColor GetPinColor(bool bIsBuffer, WStringView sName)
  {
    if (bIsBuffer)
      return WColorScheme::DarkUI(WColorScheme::Teal);

    if (sName == "DepthStencil")
      return WColorScheme::DarkUI(WColorScheme::Pink);

    return WColorScheme::DarkUI(WColorScheme::Blue);
  }
} // namespace

WRenderPipelineNodeGraphPin::WRenderPipelineNodeGraphPin(WVisualGraphPin::Type type, const char* szName, const WColorGammaUB& color, const WDocumentObject* pObject, WRenderPipelineResourceType::Enum resourceType)
  : WVisualGraphPin(type, szName, color, pObject)
  , m_ResourceType(resourceType)
{
}

WRenderPipelineNodeGraphPin::~WRenderPipelineNodeGraphPin() = default;

//////////////////////////////////////////////////////////////////////////

WRenderPipelineNodeManager::WRenderPipelineNodeManager()
{
  WAssetCurator::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WRenderPipelineNodeManager::AssetCuratorEventHandler, this));
  m_NodeEvents.AddEventHandler(WMakeDelegate(&WRenderPipelineNodeManager::NodeEventHandler, this));
}

WRenderPipelineNodeManager::~WRenderPipelineNodeManager()
{
  m_NodeEvents.RemoveEventHandler(WMakeDelegate(&WRenderPipelineNodeManager::NodeEventHandler, this));
  WAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WRenderPipelineNodeManager::AssetCuratorEventHandler, this));
}

void WRenderPipelineNodeManager::AssetCuratorEventHandler(const WAssetCuratorEvent& e)
{
  if (e.m_Type != WAssetCuratorEvent::Type::AssetUpdated || e.m_pInfo == nullptr || e.m_pInfo->m_pAssetInfo->GetManager() != GetDocument()->GetDocumentManager())
    return;

  const WRenderPipelineAssetMetaData* pMetaData = e.m_pInfo->m_pAssetInfo->m_Info->GetMetaInfo<WRenderPipelineAssetMetaData>();
  const WUInt64 uiMetaDataHash = ComputeRenderPipelineMetaDataHash(pMetaData);

  WHybridArray<const WDocumentObject*, 4> subGraphs;
  // Find sub-graphs that match the changed asset and have a different meta-data hash.
  for (auto it = m_SubGraphs.GetIterator(); it.IsValid(); ++it)
  {
    const SubGraphCache& cache = it.Value();
    if (cache.m_SourceAssetGuid == e.m_AssetGuid && cache.m_uiMetaDataHash != uiMetaDataHash)
      subGraphs.PushBack(cache.m_pObject);
  }

  if (!subGraphs.IsEmpty())
  {
    // As we have to modify the document, we need to create a transaction. Undoing this one might fail though but still better than clearing the undo stack.
    auto pAccessor = static_cast<WVisualGraphCommandAccessor*>(GetDocument()->GetObjectAccessor());
    pAccessor->StartTransaction("Update Sub-Graph");
    for (const WDocumentObject* pObject : subGraphs)
    {
      WTempHybridArray<WVisualGraphCommandAccessor::ConnectionInfo, 16> oldConnections;
      WStatus res = pAccessor->DisconnectAllPins(pObject, oldConnections);
      if (res.Failed())
      {
        WLog::Warning("Failed to DisconnectAllPins while a pipeline sub-graph was changed: {}", res.GetMessageString());
      }

      TryRecreatePins(pObject);

      res = pAccessor->TryReconnectAllPins(pObject, oldConnections);
      if (res.Failed())
      {
        WLog::Warning("Failed to TryReconnectAllPins while a pipeline sub-graph was changed: {}", res.GetMessageString());
      }
    }
    pAccessor->FinishTransaction();
  }
}

void WRenderPipelineNodeManager::NodeEventHandler(const WVisualGraphObjectManagerEvent& e)
{
  if (e.m_EventType == WVisualGraphObjectManagerEvent::Type::BeforeNodeRemoved)
  {
    m_SubGraphs.Remove(e.m_pObject->GetGuid());
  }
}

bool WRenderPipelineNodeManager::InternalIsNode(const WDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<WRenderPipelineNode>() || pType->IsDerivedFrom<WExtractor>();
}

void WRenderPipelineNodeManager::InternalCreatePins(const WDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<WRenderPipelineNode>())
    return;

  // SubGraph nodes get their pins from the referenced asset's meta data.
  if (pType == WGetStaticRTTI<WSubGraphNode>())
  {
    SubGraphCache& cache = m_SubGraphs[pObject->GetGuid()];
    cache.m_pObject = pObject;
    cache.m_SourceAssetGuid = WUuid();
    cache.m_uiMetaDataHash = 0;

    const WString sPipeline = pObject->GetTypeAccessor().GetValue("Pipeline").ConvertTo<WString>();

    auto pSubAsset = WAssetCurator::GetSingleton()->FindSubAsset(sPipeline);
    if (!pSubAsset.isValid())
      return;

    const WRenderPipelineAssetMetaData* pMeta = pSubAsset->m_pAssetInfo->m_Info->GetMetaInfo<WRenderPipelineAssetMetaData>();

    cache.m_SourceAssetGuid = pSubAsset->m_Data.m_Guid;
    cache.m_uiMetaDataHash = ComputeRenderPipelineMetaDataHash(pMeta);

    if (pMeta == nullptr)
      return;

    for (const WRenderPipelineAssetPinInfo& pinInfo : pMeta->m_Inputs)
    {
      const WColor pinColor = GetPinColor(pinInfo.m_ResourceType == WRenderPipelineResourceType::Buffer, pinInfo.m_sName);

      auto pPin = W_DEFAULT_NEW(WRenderPipelineNodeGraphPin, WVisualGraphPin::Type::Input, pinInfo.m_sName, pinColor, pObject, pinInfo.m_ResourceType);
      ref_node.m_Inputs.PushBack(std::move(pPin));
    }

    for (const WRenderPipelineAssetPinInfo& pinInfo : pMeta->m_Outputs)
    {
      const WColor pinColor = GetPinColor(pinInfo.m_ResourceType == WRenderPipelineResourceType::Buffer, pinInfo.m_sName);

      auto pPin = W_DEFAULT_NEW(WRenderPipelineNodeGraphPin, WVisualGraphPin::Type::Output, pinInfo.m_sName, pinColor, pObject, pinInfo.m_ResourceType);
      ref_node.m_Outputs.PushBack(std::move(pPin));
    }

    return;
  }

  if (pType->IsDerivedFrom<WSwitchBasePass>())
  {
    WDynamicArray<WString> inputNames;
    GetDynamicPinNames(pObject, "Values", "", inputNames);

    const bool bBuffer = pType->IsDerivedFrom<WBufferSwitchPass>();
    const WRenderPipelineResourceType::Enum resourceType = bBuffer ? WRenderPipelineResourceType::Buffer : WRenderPipelineResourceType::Texture;

    for (const WString& sInputName : inputNames)
    {
      const WColor pinColor = GetPinColor(bBuffer, sInputName);

      auto pPin = W_DEFAULT_NEW(WRenderPipelineNodeGraphPin, WVisualGraphPin::Type::Input, sInputName, pinColor, pObject, resourceType);
      ref_node.m_Inputs.PushBack(std::move(pPin));
    }
  }

  WTempHybridArray<const WAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != WPropertyCategory::Member)
      continue;

    if (pProp->GetAttributeByType<WHiddenAttribute>() != nullptr)
      continue;

    if (!pProp->GetSpecificType()->IsDerivedFrom<WRenderPipelineNodePin>())
      continue;

    const WRTTI* pPinType = pProp->GetSpecificType();
    const bool bBuffer = pPinType->IsDerivedFrom<WRenderPipelineNodeBufferInputPin>() ||
                         pPinType->IsDerivedFrom<WRenderPipelineNodeBufferOutputPin>() ||
                         pPinType->IsDerivedFrom<WRenderPipelineNodeBufferPassThroughPin>();
    const WRenderPipelineResourceType::Enum resourceType = bBuffer ? WRenderPipelineResourceType::Buffer : WRenderPipelineResourceType::Texture;

    WColor pinColor;
    if (const WColorAttribute* pAttr = pProp->GetAttributeByType<WColorAttribute>())
    {
      pinColor = pAttr->GetColor();
    }
    else
    {
      pinColor = GetPinColor(bBuffer, pProp->GetPropertyName());
    }

    if (pPinType->IsDerivedFrom<WRenderPipelineNodeInputPin>() || pPinType->IsDerivedFrom<WRenderPipelineNodeBufferInputPin>())
    {
      auto pPin = W_DEFAULT_NEW(WRenderPipelineNodeGraphPin, WVisualGraphPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject, resourceType);
      ref_node.m_Inputs.PushBack(std::move(pPin));
    }
    else if (pPinType->IsDerivedFrom<WRenderPipelineNodeOutputPin>() || pPinType->IsDerivedFrom<WRenderPipelineNodeBufferOutputPin>())
    {
      auto pPin = W_DEFAULT_NEW(WRenderPipelineNodeGraphPin, WVisualGraphPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject, resourceType);
      ref_node.m_Outputs.PushBack(std::move(pPin));
    }
    else if (pPinType->IsDerivedFrom<WRenderPipelineNodePassThroughPin>() || pPinType->IsDerivedFrom<WRenderPipelineNodeBufferPassThroughPin>())
    {
      auto pInputPin = W_DEFAULT_NEW(WRenderPipelineNodeGraphPin, WVisualGraphPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject, resourceType);
      ref_node.m_Inputs.PushBack(std::move(pInputPin));

      auto pOutputPin = W_DEFAULT_NEW(WRenderPipelineNodeGraphPin, WVisualGraphPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject, resourceType);
      ref_node.m_Outputs.PushBack(std::move(pOutputPin));
    }
  }
}

void WRenderPipelineNodeManager::GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  WRTTI::ForEachDerivedType<WRenderPipelineNode>(
    [&](const WRTTI* pRtti)
    { out_types.PushBack(pRtti); },
    WRTTI::ForEachOptions::ExcludeAbstract);

  WRTTI::ForEachDerivedType<WExtractor>(
    [&](const WRTTI* pRtti)
    { out_types.PushBack(pRtti); },
    WRTTI::ForEachOptions::ExcludeAbstract);
}

WStatus WRenderPipelineNodeManager::InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_result) const
{
  const WRenderPipelineNodeGraphPin& sourcePin = WStaticCast<const WRenderPipelineNodeGraphPin&>(source);
  const WRenderPipelineNodeGraphPin& targetPin = WStaticCast<const WRenderPipelineNodeGraphPin&>(target);

  out_result = CanConnectResult::ConnectNever;

  if (sourcePin.m_ResourceType != targetPin.m_ResourceType)
    return WStatus("Can't connect texture and buffer pins");

  if (WouldConnectionCreateCircle(source, target))
    return WStatus("Connecting these pins would create a circle in the graph.");

  out_result = CanConnectResult::ConnectNto1;
  return WStatus(W_SUCCESS);
}

WStatus WRenderPipelineNodeManager::InternalCanAdd(const WRTTI* pRtti, const WDocumentObject* pParent, WStringView sParentProperty, const WVariant& index) const
{
  if (pRtti->IsDerivedFrom<WExtractor>())
  {
    for (const WDocumentObject* pObject : GetRootObject()->GetChildren())
    {
      if (pObject->GetType() == pRtti)
        return WStatus(WFmt("The pipeline may only contain one extractor of type '{}'.", pRtti->GetTypeName()));
    }
  }

  return WStatus(W_SUCCESS);
}

bool WRenderPipelineNodeManager::InternalIsDynamicPinProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp) const
{
  if (pObject->GetTypeAccessor().GetType()->IsDerivedFrom<WSwitchBasePass>())
  {
    return WStringUtils::IsEqual(pProp->GetPropertyName(), "Values");
  }

  if (pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WSubGraphNode>())
  {
    return WStringUtils::IsEqual(pProp->GetPropertyName(), "Pipeline");
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////

WRenderPipelineAssetDocument::WRenderPipelineAssetDocument(WStringView sDocumentPath)
  : WAssetDocument(sDocumentPath, W_DEFAULT_NEW(WRenderPipelineNodeManager), WAssetDocEngineConnection::FullObjectMirroring)
{
  m_pObjectAccessor = W_DEFAULT_NEW(WVisualGraphCommandAccessor, GetCommandHistory());
}

WRenderPipelineAssetDocument::~WRenderPipelineAssetDocument() = default;

WStatus WRenderPipelineAssetDocument::Validate() const
{
  const WRTTI* pInputTypes[] = {WGetStaticRTTI<WSubGraphTextureInputNode>(), WGetStaticRTTI<WSubGraphBufferInputNode>()};
  const WRTTI* pOutputTypes[] = {WGetStaticRTTI<WSubGraphTextureOutputNode>(), WGetStaticRTTI<WSubGraphBufferOutputNode>()};
  const WRenderPipelineNodeManager* pManager = static_cast<const WRenderPipelineNodeManager*>(GetObjectManager());

  WSet<WString> inputNames, outputNames;

  for (const WDocumentObject* pObject : GetObjectManager()->GetRootObject()->GetChildren())
  {
    const WRTTI* pType = pObject->GetTypeAccessor().GetType();

    if (pType->IsDerivedFrom<WSwitchBasePass>())
    {
      WSet<WInt32> uniqueValues;
      const WVariantArray& values = pObject->GetTypeAccessor().GetValue("Values").Get<WVariantArray>();
      for (const WVariant& value : values)
      {
        const WInt32 iValue = value.ConvertTo<WInt32>();
        if (uniqueValues.Contains(iValue))
          return WStatus(WFmt("Switch '{}' contains duplicate value '{}'.", pObject->GetTypeAccessor().GetValue("Name"), iValue));

        uniqueValues.Insert(iValue);
      }
    }

    bool bIsInput = (pType == pInputTypes[0] || pType == pInputTypes[1]);
    bool bIsOutput = (pType == pOutputTypes[0] || pType == pOutputTypes[1]);

    if (!bIsInput && !bIsOutput)
      continue;

    const WString sName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<WString>();

    if (sName.IsEmpty())
    {
      return WStatus(WFmt("{} node '{}' has an empty name.", bIsInput ? "Input" : "Output", pType->GetTypeName()));
    }

    WSet<WString>& names = bIsInput ? inputNames : outputNames;
    if (names.Contains(sName))
    {
      return WStatus(WFmt("{} node '{}' has a duplicate name '{}'.", bIsInput ? "Input" : "Output", pType->GetTypeName(), sName));
    }
    names.Insert(sName);
  }

  for (const WDocumentObject* pObject : GetObjectManager()->GetRootObject()->GetChildren())
  {
    const bool isNode = pManager->IsNode(pObject);
    if (!isNode)
      continue;

    for (const WUniquePtr<const WVisualGraphPin>& pOutputPin : pManager->GetOutputPins(pObject))
    {
      WUInt32 uiPassThroughConnections = 0;
      for (const WVisualGraphConnection* pConnection : pManager->GetConnections(*pOutputPin))
      {
        const WVisualGraphPin& targetPin = pConnection->GetTargetPin();
        const WRTTI* pTargetType = targetPin.GetParent()->GetType();

        bool bIsPassThrough = pTargetType->IsDerivedFrom<WSwitchBasePass>();
        if (const WAbstractProperty* pProperty = pTargetType->FindPropertyByName(targetPin.GetName()))
        {
          const WRTTI* pPinType = pProperty->GetSpecificType();
          bIsPassThrough |= pPinType->IsDerivedFrom<WRenderPipelineNodePassThroughPin>() || pPinType->IsDerivedFrom<WRenderPipelineNodeBufferPassThroughPin>();
        }

        if (bIsPassThrough && ++uiPassThroughConnections > 1)
        {
          return WStatus(WFmt("Output pin '{}.{}' is connected to more than one pass-through input.", pObject->GetTypeAccessor().GetValue("Name"), pOutputPin->GetName()));
        }
      }
    }
  }

  return WStatus(W_SUCCESS);
}

WTransformStatus WRenderPipelineAssetDocument::InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
  const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  W_SUCCEED_OR_RETURN(Validate());

  if (!GetLoadingErrors().IsEmpty())
  {
    WStringBuilder s("Cannot transform document because it had errors during loading:\n\n");
    for (const WString& err : GetLoadingErrors())
    {
      s.Append(err, "\n");
    }
    return WTransformStatus(s.GetView());
  }

  return WAssetDocument::RemoteExport(AssetHeader, szTargetFile);
}

WTransformStatus WRenderPipelineAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  W_REPORT_FAILURE("Should not be called");
  return WTransformStatus();
}

void WRenderPipelineAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  WRenderPipelineAssetMetaData* pMeta = W_DEFAULT_NEW(WRenderPipelineAssetMetaData);

  struct BoundaryNodeType
  {
    const WRTTI* m_pType;
    bool m_bIsInput;
    WRenderPipelineResourceType::Enum m_ResourceType;
  };

  const BoundaryNodeType boundaryTypes[] = {
    {WGetStaticRTTI<WSubGraphTextureInputNode>(), true, WRenderPipelineResourceType::Texture},
    {WGetStaticRTTI<WSubGraphBufferInputNode>(), true, WRenderPipelineResourceType::Buffer},
    {WGetStaticRTTI<WSubGraphTextureOutputNode>(), false, WRenderPipelineResourceType::Texture},
    {WGetStaticRTTI<WSubGraphBufferOutputNode>(), false, WRenderPipelineResourceType::Buffer},
  };

  for (const WDocumentObject* pObject : GetObjectManager()->GetRootObject()->GetChildren())
  {
    const WRTTI* pType = pObject->GetTypeAccessor().GetType();

    for (const BoundaryNodeType& boundary : boundaryTypes)
    {
      if (boundary.m_pType != pType)
        continue;

      auto& info = (boundary.m_bIsInput ? pMeta->m_Inputs : pMeta->m_Outputs).ExpandAndGetRef();
      info.m_ResourceType = boundary.m_ResourceType;
      info.m_sName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<WString>();
      break;
    }
  }

  auto sortByTypeAndName = [](const WRenderPipelineAssetPinInfo& a, const WRenderPipelineAssetPinInfo& b)
  {
    if (a.m_ResourceType != b.m_ResourceType)
      return a.m_ResourceType < b.m_ResourceType;

    return a.m_sName.Compare(b.m_sName) < 0;
  };
  pMeta->m_Inputs.Sort(sortByTypeAndName);
  pMeta->m_Outputs.Sort(sortByTypeAndName);

  // pInfo takes ownership
  pInfo->m_MetaInfo.PushBack(pMeta);
}

void WRenderPipelineAssetDocument::InternalGetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const
{
  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void WRenderPipelineAssetDocument::AttachMetaDataBeforeSaving(WAbstractObjectGraph& ref_graph) const
{
  SUPER::AttachMetaDataBeforeSaving(ref_graph);
  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(ref_graph);
}

void WRenderPipelineAssetDocument::RestoreMetaDataAfterLoading(const WAbstractObjectGraph& ref_graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(ref_graph, bUndoable);
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(ref_graph, bUndoable);
}

void WRenderPipelineAssetDocument::GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/WEditor.RenderPipelineGraph");
}

bool WRenderPipelineAssetDocument::CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_MimeType) const
{
  out_MimeType = "application/WEditor.RenderPipelineGraph";

  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool WRenderPipelineAssetDocument::Paste(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType)
{
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, WQtVisualGraphScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
