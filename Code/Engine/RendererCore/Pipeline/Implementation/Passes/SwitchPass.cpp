#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/SwitchPass.h>

// clang-format off
W_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(WSwitchBasePass, 1)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("BlackboardProperty", m_sBlackboardProperty),
    W_ARRAY_MEMBER_PROPERTY("Values", m_Values)->AddAttributes(new WMaxArraySizeAttribute(WSwitchBasePass::s_uiMaxInputs), new WNoTemporaryTransactionsAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_ABSTRACT_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureSwitchPass, 1, WRTTIDefaultAllocator<WTextureSwitchPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Output", m_Output),
    W_MEMBER_PROPERTY("Input0", m_Input0)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input1", m_Input1)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input2", m_Input2)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input3", m_Input3)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input4", m_Input4)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input5", m_Input5)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input6", m_Input6)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input7", m_Input7)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Texture Switch: {Name}"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Blue)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBufferSwitchPass, 1, WRTTIDefaultAllocator<WBufferSwitchPass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Output", m_Output),
    W_MEMBER_PROPERTY("Input0", m_Input0)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input1", m_Input1)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input2", m_Input2)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input3", m_Input3)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input4", m_Input4)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input5", m_Input5)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input6", m_Input6)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Input7", m_Input7)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Buffer Switch: {Name}"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Teal)),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSwitchBasePass::WSwitchBasePass(const char* szName)
  : WRenderPipelinePass(szName)
{
}

bool WSwitchBasePass::SetSwitchValue(WInt32 iValue)
{
  WUInt8 uiNewIndex = 0;
  for (WUInt32 i = 0; i < WMath::Min(m_Values.GetCount(), s_uiMaxInputs); ++i)
  {
    if (iValue == m_Values[i])
    {
      uiNewIndex = static_cast<WUInt8>(i);
      break;
    }
  }

  const bool bChanged = m_uiSelectedValueIndex != uiNewIndex;
  m_uiSelectedValueIndex = uiNewIndex;
  return bChanged;
}

WStatus WSwitchBasePass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  if (m_uiSelectedValueIndex >= inputs.GetCount() || outputs.IsEmpty())
    return WStatus(WFmt("Switch '{}' has an invalid selected input.", GetName()));

  outputs[0] = inputs[m_uiSelectedValueIndex];
  return W_SUCCESS;
}

WResult WSwitchBasePass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_sBlackboardProperty;
  W_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Values));
  return W_SUCCESS;
}

WResult WSwitchBasePass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_ASSERT_DEBUG(uiVersion == 1, "Unknown version encountered");
  W_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_sBlackboardProperty;
  W_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Values));
  return W_SUCCESS;
}

void WSwitchBasePass::AddDynamicInputPins(WArrayPtr<const WRenderPipelineNodePin* const> pins, WHashTable<WHashedString, const WRenderPipelineNodePin*>& ref_nameToPin)
{
  const WUInt32 uiCount = WMath::Min(m_Values.GetCount(), pins.GetCount());

  WStringBuilder sName;
  for (WUInt32 i = 0; i < uiCount; ++i)
  {
    sName.SetFormat("{}", m_Values[i]);

    WHashedString sHashedName;
    sHashedName.Assign(sName);

    if (ref_nameToPin.Contains(sHashedName))
    {
      WLog::Error("Switch '{}' uses the value '{}' more than once, only the first input pin with that value is reachable.", GetName(), m_Values[i]);
      continue;
    }

    ref_nameToPin.Insert(sHashedName, pins[i]);
  }
}

WTextureSwitchPass::WTextureSwitchPass()
  : WSwitchBasePass("TextureSwitchPass")
{
}

void WTextureSwitchPass::AddDynamicPins(WHashTable<WHashedString, const WRenderPipelineNodePin*>& ref_nameToPin)
{
  const WRenderPipelineNodePin* pins[] = {&m_Input0, &m_Input1, &m_Input2, &m_Input3, &m_Input4, &m_Input5, &m_Input6, &m_Input7};
  static_assert(W_ARRAY_SIZE(pins) == s_uiMaxInputs);

  AddDynamicInputPins(pins, ref_nameToPin);
}

WBufferSwitchPass::WBufferSwitchPass()
  : WSwitchBasePass("BufferSwitchPass")
{
}

void WBufferSwitchPass::AddDynamicPins(WHashTable<WHashedString, const WRenderPipelineNodePin*>& ref_nameToPin)
{
  const WRenderPipelineNodePin* pins[] = {&m_Input0, &m_Input1, &m_Input2, &m_Input3, &m_Input4, &m_Input5, &m_Input6, &m_Input7};
  static_assert(W_ARRAY_SIZE(pins) == s_uiMaxInputs);

  AddDynamicInputPins(pins, ref_nameToPin);
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SwitchPass);
