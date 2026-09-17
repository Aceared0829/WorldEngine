#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderPipelineNode.h>

// clang-format off
W_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(WRenderPipelineNode, 1)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineNodePin, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_ATTRIBUTES
  {
   new WHiddenAttribute(),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineNodeInputPin, WRenderPipelineNodePin, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineNodeOutputPin, WRenderPipelineNodePin, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineNodeInputProviderPin, WRenderPipelineNodeInputPin, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineNodeOutputProviderPin, WRenderPipelineNodeOutputPin, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineNodePassThroughPin, WRenderPipelineNodePin, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineNodeBufferInputPin, WRenderPipelineNodePin, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineNodeBufferOutputPin, WRenderPipelineNodePin, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineNodeBufferInputProviderPin, WRenderPipelineNodeBufferInputPin, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineNodeBufferOutputProviderPin, WRenderPipelineNodeBufferOutputPin, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WRenderPipelineNodeBufferPassThroughPin, WRenderPipelineNodePin, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

void WRenderPipelineNode::InitializePins()
{
  m_InputPins.Clear();
  m_OutputPins.Clear();
  m_NameToPin.Clear();

  const WRTTI* pType = GetDynamicRTTI();

  WTempHybridArray<const WAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != WPropertyCategory::Member || !pProp->GetSpecificType()->IsDerivedFrom(WGetStaticRTTI<WRenderPipelineNodePin>()))
      continue;

    auto pPinProp = static_cast<const WAbstractMemberProperty*>(pProp);
    WRenderPipelineNodePin* pPin = static_cast<WRenderPipelineNodePin*>(pPinProp->GetPropertyPointer(this));

    pPin->m_pParent = this;
    const bool bMoreThanOneType = ((WInt32)pPin->m_Type.IsSet(WRenderPipelineNodePin::Type::PassThrough) + (WInt32)pPin->m_Type.IsSet(WRenderPipelineNodePin::Type::Input) + (WInt32)pPin->m_Type.IsSet(WRenderPipelineNodePin::Type::Output)) > 1;
    const bool bProviderOnPassThrough = pPin->m_Type.IsSet(WRenderPipelineNodePin::Type::PassThrough) && pPin->m_Type.IsSet(WRenderPipelineNodePin::Type::TextureProvider);
    if (bMoreThanOneType || bProviderOnPassThrough)
    {
      W_REPORT_FAILURE("Pin '{0}' has an invalid type. Do not use WRenderPipelineNodePin directly as member but one of its derived types", pProp->GetPropertyName());
      continue;
    }

    if (pPin->m_Type.IsAnySet(WRenderPipelineNodePin::Type::Input | WRenderPipelineNodePin::Type::PassThrough))
    {
      pPin->m_uiInputIndex = static_cast<WUInt8>(m_InputPins.GetCount());
      m_InputPins.PushBack(pPin);
    }
    if (pPin->m_Type.IsAnySet(WRenderPipelineNodePin::Type::Output | WRenderPipelineNodePin::Type::PassThrough))
    {
      pPin->m_uiOutputIndex = static_cast<WUInt8>(m_OutputPins.GetCount());
      m_OutputPins.PushBack(pPin);
    }

    WHashedString sHashedName;
    sHashedName.Assign(pProp->GetPropertyName());
    m_NameToPin.Insert(sHashedName, pPin);
  }

  AddDynamicPins(m_NameToPin);
}

WHashedString WRenderPipelineNode::GetPinName(const WRenderPipelineNodePin* pPin) const
{
  for (auto it = m_NameToPin.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value() == pPin)
    {
      return it.Key();
    }
  }
  return WHashedString();
}

const WRenderPipelineNodePin* WRenderPipelineNode::GetPinByName(WTempHashedString sName) const
{
  const WRenderPipelineNodePin* pin;
  if (m_NameToPin.TryGetValue(sName, pin))
  {
    return pin;
  }

  return nullptr;
}


W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineNode);
