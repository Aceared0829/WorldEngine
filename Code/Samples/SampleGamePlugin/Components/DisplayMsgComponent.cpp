#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <SampleGamePlugin/Components/DisplayMsgComponent.h>
#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(DisplayMsgComponent, 1, WComponentMode::Static /* this component does not move the owner node */)
{
  //W_BEGIN_PROPERTIES
  //{
  //}
  //W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("SampleGamePlugin"),
  }
  W_END_ATTRIBUTES;

  // BEGIN-DOCS-CODE-SNIPPET: message-handler-block
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgSetText, OnSetText),
    W_MESSAGE_HANDLER(WMsgSetColor, OnSetColor)
  }
  W_END_MESSAGEHANDLERS;
  // END-DOCS-CODE-SNIPPET
}
W_END_COMPONENT_TYPE
// clang-format on

DisplayMsgComponent::DisplayMsgComponent() = default;
DisplayMsgComponent::~DisplayMsgComponent() = default;

void DisplayMsgComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
}

void DisplayMsgComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
}

void DisplayMsgComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
}

void DisplayMsgComponent::Update()
{
  const WTransform ownerTransform = GetOwner()->GetGlobalTransform();

  WDebugRenderer::Draw3DText(GetWorld(), m_sCurrentText.GetData(), ownerTransform.m_vPosition, m_TextColor, 32);
}

// BEGIN-DOCS-CODE-SNIPPET: message-handler-impl
void DisplayMsgComponent::OnSetText(WMsgSetText& msg)
{
  m_sCurrentText = msg.m_sText;
}

void DisplayMsgComponent::OnSetColor(WMsgSetColor& msg)
{
  m_TextColor = msg.m_Color;
}
// END-DOCS-CODE-SNIPPET
