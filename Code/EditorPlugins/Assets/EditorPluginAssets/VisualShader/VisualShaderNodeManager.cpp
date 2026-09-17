#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>

//////////////////////////////////////////////////////////////////////////
// WVisualShaderPin
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualShaderPin, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WVisualShaderPin::WVisualShaderPin(Type type, const WVisualShaderPinDescriptor* pDescriptor, const WDocumentObject* pObject)
  : WVisualGraphPin(type, pDescriptor->m_sName, pDescriptor->m_Color, pObject)
{
  m_pDescriptor = pDescriptor;
}

const WRTTI* WVisualShaderPin::GetDataType() const
{
  return m_pDescriptor->m_pDataType;
}

const WString& WVisualShaderPin::GetTooltip() const
{
  return m_pDescriptor->m_sTooltip;
}

//////////////////////////////////////////////////////////////////////////
// WVisualShaderNodeManager
//////////////////////////////////////////////////////////////////////////

bool WVisualShaderNodeManager::InternalIsNode(const WDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(WVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType());
}

void WVisualShaderNodeManager::InternalCreatePins(const WDocumentObject* pObject, NodeInternal& ref_node)
{
  const auto* pDesc = WVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

  if (pDesc == nullptr)
    return;

  ref_node.m_Inputs.Reserve(pDesc->m_InputPins.GetCount());
  ref_node.m_Outputs.Reserve(pDesc->m_OutputPins.GetCount());

  for (const auto& pin : pDesc->m_InputPins)
  {
    auto pPin = W_DEFAULT_NEW(WVisualShaderPin, WVisualGraphPin::Type::Input, &pin, pObject);
    ref_node.m_Inputs.PushBack(pPin);
  }

  for (const auto& pin : pDesc->m_OutputPins)
  {
    auto pPin = W_DEFAULT_NEW(WVisualShaderPin, WVisualGraphPin::Type::Output, &pin, pObject);
    ref_node.m_Outputs.PushBack(pPin);
  }
}

void WVisualShaderNodeManager::GetNodeCreationTemplates(WDynamicArray<WVisualGraphNodeDesc>& out_templates) const
{
  const WRTTI* pNodeBaseType = WVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

  WRTTI::ForEachDerivedType(
    pNodeBaseType,
    [&](const WRTTI* pRtti)
    {
      auto& nodeTemplate = out_templates.ExpandAndGetRef();
      nodeTemplate.m_pType = pRtti;

      if (const WVisualShaderNodeDescriptor* pDesc = WVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti))
      {
        nodeTemplate.m_sCategory = pDesc->m_sCategory;
      }
    },
    WRTTI::ForEachOptions::ExcludeAbstract);
}

WStatus WVisualShaderNodeManager::InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_result) const
{
  const WVisualShaderPin& pinSource = WStaticCast<const WVisualShaderPin&>(source);
  const WVisualShaderPin& pinTarget = WStaticCast<const WVisualShaderPin&>(target);

  const WRTTI* pSamplerType = WVisualShaderTypeRegistry::GetSingleton()->GetPinSamplerType();
  const WRTTI* pStringType = WGetStaticRTTI<WString>();

  if ((pinSource.GetDataType() == pSamplerType && pinTarget.GetDataType() != pSamplerType) || (pinSource.GetDataType() != pSamplerType && pinTarget.GetDataType() == pSamplerType))
  {
    out_result = CanConnectResult::ConnectNever;
    return WStatus("Pin of type 'sampler' cannot be connected with a pin of a different type.");
  }

  if ((pinSource.GetDataType() == pStringType && pinTarget.GetDataType() != pStringType) || (pinSource.GetDataType() != pStringType && pinTarget.GetDataType() == pStringType))
  {
    out_result = CanConnectResult::ConnectNever;
    return WStatus("Pin of type 'string' cannot be connected with a pin of a different type.");
  }

  if (WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return WStatus("Connecting these pins would create a circle in the shader graph.");
  }

  out_result = CanConnectResult::ConnectNto1;
  return WStatus(W_SUCCESS);
}


WStatus WVisualShaderNodeManager::InternalCanAdd(const WRTTI* pRtti, const WDocumentObject* pParent, WStringView sParentProperty, const WVariant& index) const
{
  auto pDesc = WVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti);

  if (pDesc)
  {
    if (pDesc->m_NodeType == WVisualShaderNodeType::Main && CountNodesOfType(WVisualShaderNodeType::Main) > 0)
    {
      return WStatus("The shader may only contain a single output node");
    }

    /// \todo This is an arbitrary limit and it does not count how many nodes reference the same texture
    static constexpr WUInt32 uiMaxTextures = 16;
    if (pDesc->m_NodeType == WVisualShaderNodeType::Texture && CountNodesOfType(WVisualShaderNodeType::Texture) >= uiMaxTextures)
    {
      return WStatus(WFmt("The maximum number of texture nodes is {0}", uiMaxTextures));
    }
  }

  return WStatus(W_SUCCESS);
}

WUInt32 WVisualShaderNodeManager::CountNodesOfType(WVisualShaderNodeType::Enum type) const
{
  WUInt32 count = 0;

  const WVisualShaderTypeRegistry* pRegistry = WVisualShaderTypeRegistry::GetSingleton();

  const auto& children = GetRootObject()->GetChildren();
  for (WUInt32 i = 0; i < children.GetCount(); ++i)
  {
    auto pDesc = pRegistry->GetDescriptorForType(children[i]->GetType());

    if (pDesc && pDesc->m_NodeType == type)
    {
      ++count;
    }
  }

  return count;
}
