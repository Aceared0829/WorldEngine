#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAsset.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphQt.h>
#include <Foundation/Math/ColorScheme.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Output/PoseResultAnimNode.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Pose/SampleFrameAnimNode.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>
#include <ToolsFoundation/VisualGraph/VisualGraphCommandAccessor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationGraphAssetDocument, 5, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationGraphNodePin, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationGraphAssetProperties, 1, WRTTIDefaultAllocator<WAnimationGraphAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("IncludeGraphs", m_IncludeGraphs)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Keyframe_Graph")),
    W_ARRAY_MEMBER_PROPERTY("AnimationClipMapping", m_AnimationClipMapping),
  }
    W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, AnimationGraph)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WQtVisualGraphScene::GetNodeFactory().RegisterCreator(WGetStaticRTTI<WAnimGraphNode>(), [](const WRTTI* pRtti)->WQtVisualGraphNode* { return new WQtAnimationGraphNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WQtVisualGraphScene::GetNodeFactory().UnregisterCreator(WGetStaticRTTI<WAnimGraphNode>());
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool WAnimationGraphNodeManager::InternalIsNode(const WDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<WAnimGraphNode>();
}

void WAnimationGraphNodeManager::InternalCreatePins(const WDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<WAnimGraphNode>())
    return;

  WTempHybridArray<const WAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  const WColor triggerPinColor = WColorScheme::DarkUI(WColorScheme::Yellow);
  const WColor numberPinColor = WColorScheme::DarkUI(WColorScheme::Lime);
  const WColor boolPinColor = WColorScheme::LightUI(WColorScheme::Lime);
  const WColor weightPinColor = WColorScheme::DarkUI(WColorScheme::Teal);
  const WColor localPosePinColor = WColorScheme::DarkUI(WColorScheme::Blue);
  const WColor modelPosePinColor = WColorScheme::DarkUI(WColorScheme::Grape);
  // EXTEND THIS if a new type is introduced

  WTempHybridArray<WString, 16> pinNames;

  for (auto pProp : properties)
  {
    if (!pProp->GetSpecificType()->IsDerivedFrom<WAnimGraphPin>())
      continue;

    pinNames.Clear();

    if (pProp->GetCategory() == WPropertyCategory::Array)
    {
      if (const WDynamicPinAttribute* pDynPin = pProp->GetAttributeByType<WDynamicPinAttribute>())
      {
        GetDynamicPinNames(pObject, pDynPin->GetProperty(), pProp->GetPropertyName(), pinNames);
      }
    }
    else if (pProp->GetCategory() == WPropertyCategory::Member)
    {
      pinNames.PushBack(pProp->GetPropertyName());
    }

    for (WUInt32 i = 0; i < pinNames.GetCount(); ++i)
    {
      const auto& pinName = pinNames[i];

      if (pProp->GetSpecificType()->IsDerivedFrom<WAnimGraphTriggerInputPin>())
      {
        auto pPin = W_DEFAULT_NEW(WAnimationGraphNodePin, WVisualGraphPin::Type::Input, pinName, triggerPinColor, pObject);
        pPin->m_DataType = WAnimGraphPin::Trigger;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<WAnimGraphTriggerOutputPin>())
      {
        auto pPin = W_DEFAULT_NEW(WAnimationGraphNodePin, WVisualGraphPin::Type::Output, pinName, triggerPinColor, pObject);
        pPin->m_DataType = WAnimGraphPin::Trigger;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<WAnimGraphNumberInputPin>())
      {
        auto pPin = W_DEFAULT_NEW(WAnimationGraphNodePin, WVisualGraphPin::Type::Input, pinName, numberPinColor, pObject);
        pPin->m_DataType = WAnimGraphPin::Number;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<WAnimGraphNumberOutputPin>())
      {
        auto pPin = W_DEFAULT_NEW(WAnimationGraphNodePin, WVisualGraphPin::Type::Output, pinName, numberPinColor, pObject);
        pPin->m_DataType = WAnimGraphPin::Number;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<WAnimGraphBoolInputPin>())
      {
        auto pPin = W_DEFAULT_NEW(WAnimationGraphNodePin, WVisualGraphPin::Type::Input, pinName, boolPinColor, pObject);
        pPin->m_DataType = WAnimGraphPin::Bool;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<WAnimGraphBoolOutputPin>())
      {
        auto pPin = W_DEFAULT_NEW(WAnimationGraphNodePin, WVisualGraphPin::Type::Output, pinName, boolPinColor, pObject);
        pPin->m_DataType = WAnimGraphPin::Bool;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<WAnimGraphBoneWeightsInputPin>())
      {
        auto pPin = W_DEFAULT_NEW(WAnimationGraphNodePin, WVisualGraphPin::Type::Input, pinName, weightPinColor, pObject);
        pPin->m_DataType = WAnimGraphPin::BoneWeights;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<WAnimGraphBoneWeightsOutputPin>())
      {
        auto pPin = W_DEFAULT_NEW(WAnimationGraphNodePin, WVisualGraphPin::Type::Output, pinName, weightPinColor, pObject);
        pPin->m_DataType = WAnimGraphPin::BoneWeights;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<WAnimGraphLocalPoseInputPin>())
      {
        auto pPin = W_DEFAULT_NEW(WAnimationGraphNodePin, WVisualGraphPin::Type::Input, pinName, localPosePinColor, pObject);
        pPin->m_DataType = WAnimGraphPin::LocalPose;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<WAnimGraphLocalPoseOutputPin>())
      {
        auto pPin = W_DEFAULT_NEW(WAnimationGraphNodePin, WVisualGraphPin::Type::Output, pinName, localPosePinColor, pObject);
        pPin->m_DataType = WAnimGraphPin::LocalPose;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else
      {
        // EXTEND THIS if a new type is introduced
        W_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }
}

void WAnimationGraphNodeManager::GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  WRTTI::ForEachDerivedType<WAnimGraphNode>(
    [&](const WRTTI* pRtti)
    { out_types.PushBack(pRtti); },
    WRTTI::ForEachOptions::ExcludeAbstract);
}

WStatus WAnimationGraphNodeManager::InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_result) const
{
  const WAnimationGraphNodePin& sourcePin = WStaticCast<const WAnimationGraphNodePin&>(source);
  const WAnimationGraphNodePin& targetPin = WStaticCast<const WAnimationGraphNodePin&>(target);

  out_result = CanConnectResult::ConnectNever;

  if (sourcePin.m_DataType != targetPin.m_DataType)
    return WStatus("Can't connect pins of different data types");

  if (sourcePin.GetType() == targetPin.GetType())
    return WStatus("Can only connect input pins with output pins.");

  switch (sourcePin.m_DataType)
  {
    case WAnimGraphPin::Trigger:
      out_result = CanConnectResult::ConnectNtoN;
      break;

    case WAnimGraphPin::Number:
      out_result = CanConnectResult::ConnectNto1;
      break;

    case WAnimGraphPin::Bool:
      out_result = CanConnectResult::ConnectNto1;
      break;

    case WAnimGraphPin::BoneWeights:
      out_result = CanConnectResult::ConnectNto1;
      break;

    case WAnimGraphPin::LocalPose:
      if (targetPin.m_bMultiInputPin)
        out_result = CanConnectResult::ConnectNtoN;
      else
        out_result = CanConnectResult::ConnectNto1;
      break;

    case WAnimGraphPin::ModelPose:
      out_result = CanConnectResult::ConnectNto1;
      break;

      // EXTEND THIS if a new type is introduced
      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (out_result != CanConnectResult::ConnectNever && WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return WStatus("Connecting these pins would create a circle in the graph.");
  }

  return WStatus(W_SUCCESS);
}

bool WAnimationGraphNodeManager::InternalIsDynamicPinProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp) const
{
  return pProp->GetAttributeByType<WDynamicPinAttribute>() != nullptr;
}

WAnimationGraphAssetDocument::WAnimationGraphAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WAnimationGraphAssetProperties>(W_DEFAULT_NEW(WAnimationGraphNodeManager), sDocumentPath, WAssetDocEngineConnection::None)
{
  m_pObjectAccessor = W_DEFAULT_NEW(WVisualGraphCommandAccessor, GetCommandHistory());
}

WTransformStatus WAnimationGraphAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  const auto* pNodeManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());

  auto pProp = GetProperties();

  {
    stream.WriteVersion(2);
    stream.WriteArray(pProp->m_IncludeGraphs).AssertSuccess();

    const WUInt32 uiNum = pProp->m_AnimationClipMapping.GetCount();
    stream << uiNum;

    for (WUInt32 i = 0; i < uiNum; ++i)
    {
      stream << pProp->m_AnimationClipMapping[i].m_sClipName;
      stream << pProp->m_AnimationClipMapping[i].m_hClip;
    }
  }

  // find all 'nodes'
  WDynamicArray<const WDocumentObject*> allNodes;
  for (auto pNode : pNodeManager->GetRootObject()->GetChildren())
  {
    if (!pNodeManager->IsNode(pNode))
      continue;

    allNodes.PushBack(pNode);
  }

  WAnimGraph animGraph;

  WMap<const WDocumentObject*, WAnimGraphNode*> docNodeToRuntimeNode;

  // create all nodes in the WAnimGraph
  {
    for (const WDocumentObject* pNode : allNodes)
    {
      WAnimGraphNode* pNewNode = animGraph.AddNode(pNode->GetType()->GetAllocator()->Allocate<WAnimGraphNode>());

      // copy all the non-hidden properties
      WToolsSerializationUtils::CopyProperties(pNode, GetObjectManager(), pNewNode, pNewNode->GetDynamicRTTI(), [](const WAbstractProperty* p)
        { return p->GetAttributeByType<WHiddenAttribute>() == nullptr; });

      docNodeToRuntimeNode[pNode] = pNewNode;
    }
  }

  // add all node connections to the WAnimGraph
  {
    for (WUInt32 nodeIdx = 0; nodeIdx < allNodes.GetCount(); ++nodeIdx)
    {
      const WDocumentObject* pNode = allNodes[nodeIdx];

      const auto outputPins = pNodeManager->GetOutputPins(pNode);

      for (auto& pPin : outputPins)
      {
        for (const WVisualGraphConnection* pCon : pNodeManager->GetConnections(*pPin))
        {
          const WAnimGraphNode* pSrcNode = docNodeToRuntimeNode[pCon->GetSourcePin().GetParent()];
          WAnimGraphNode* pDstNode = docNodeToRuntimeNode[pCon->GetTargetPin().GetParent()];

          animGraph.AddConnection(pSrcNode, pCon->GetSourcePin().GetName(), pDstNode, pCon->GetTargetPin().GetName());
        }
      }
    }
  }

  W_SUCCEED_OR_RETURN(animGraph.Serialize(stream));

  return WTransformStatus(W_SUCCESS);
}

void WAnimationGraphAssetDocument::InternalGetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const
{
  // without this, changing connections only (no property value) may not result in a different asset document hash and therefore no transform

  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void WAnimationGraphAssetDocument::AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void WAnimationGraphAssetDocument::RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}



void WAnimationGraphAssetDocument::GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/WEditor.AnimationGraphGraph");
}

bool WAnimationGraphAssetDocument::CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_MimeType) const
{
  out_MimeType = "application/WEditor.AnimationGraphGraph";

  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool WAnimationGraphAssetDocument::Paste(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType)
{
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, WQtVisualGraphScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

WAnimationGraphNodePin::WAnimationGraphNodePin(Type type, const char* szName, const WColorGammaUB& color, const WDocumentObject* pObject)
  : WVisualGraphPin(type, szName, color, pObject)
{
}

WAnimationGraphNodePin::~WAnimationGraphNodePin() = default;
