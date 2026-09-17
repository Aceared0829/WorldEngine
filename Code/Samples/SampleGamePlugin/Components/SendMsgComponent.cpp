#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <SampleGamePlugin/Components/SendMsgComponent.h>
#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(SendMsgComponent, 1, WComponentMode::Static /* this component does not move the owner node */)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Strings", m_TextArray)
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("SampleGamePlugin"),
  }
  W_END_ATTRIBUTES;

  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgComponentInternalTrigger, OnSendText)
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE
// clang-format on

SendMsgComponent::SendMsgComponent() = default;
SendMsgComponent::~SendMsgComponent() = default;

void SendMsgComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s.WriteArray(m_TextArray).IgnoreResult();
}

void SendMsgComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s.ReadArray(m_TextArray).IgnoreResult();
}

static WHashedString s_sSendNextString = WMakeHashedString("SendNextString");

void SendMsgComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // start sending strings shortly
  WMsgComponentInternalTrigger msg;
  msg.m_sMessage = s_sSendNextString;
  PostMessage(msg, WTime::MakeFromMilliseconds(100));
}

void SendMsgComponent::OnSendText(WMsgComponentInternalTrigger& msg)
{
  // Note: We don't need to take care to stop when the component gets deactivated
  // because messages are only delivered to active components.
  // However, if the component got deactivated and activated again within the 2 second
  // message delay, OnSimulationStarted() above could queue a second message, and now
  // both of them would arrive. We don't handle that case here.
  // if (!IsActiveAndSimulating())
  //  return;

  if (msg.m_sMessage == s_sSendNextString)
  {
    if (!m_TextArray.IsEmpty())
    {
      const WUInt32 idx = m_uiNextString % m_TextArray.GetCount();

      // send the message to all components on this node and all child nodes

      WGameObject* pGameObject = GetOwner();

      // BEGIN-DOCS-CODE-SNIPPET: message-send-direct
      WMsgSetText textMsg;
      textMsg.m_sText = m_TextArray[idx];
      pGameObject->SendMessageRecursive(textMsg);
      // END-DOCS-CODE-SNIPPET

      m_uiNextString++;
    }


    // send the next string in a second
    PostMessage(msg, WTime::MakeFromSeconds(2));
  }
}
