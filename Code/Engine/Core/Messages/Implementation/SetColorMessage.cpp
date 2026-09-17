#include <Core/CorePCH.h>

#include <Core/Messages/SetColorMessage.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WSetColorMode, 1)
W_ENUM_CONSTANTS(WSetColorMode::SetRGBA, WSetColorMode::SetRGB, WSetColorMode::SetAlpha, WSetColorMode::AlphaBlend, WSetColorMode::Additive, WSetColorMode::Modulate)
W_END_STATIC_REFLECTED_ENUM;

W_IMPLEMENT_MESSAGE_TYPE(WMsgSetColor);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgSetColor, 1, WRTTIDefaultAllocator<WMsgSetColor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_Color),
    W_ENUM_MEMBER_PROPERTY("Mode", WSetColorMode, m_Mode)
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WMsgSetColor::ModifyColor(WColor& ref_color) const
{
  switch (m_Mode)
  {
    case WSetColorMode::SetRGB:
      ref_color.SetRGB(m_Color.r, m_Color.g, m_Color.b);
      break;

    case WSetColorMode::SetAlpha:
      ref_color.a = m_Color.a;
      break;

    case WSetColorMode::AlphaBlend:
      ref_color = WMath::Lerp(ref_color, m_Color, m_Color.a);
      break;

    case WSetColorMode::Additive:
      ref_color += m_Color;
      break;

    case WSetColorMode::Modulate:
      ref_color *= m_Color;
      break;

    case WSetColorMode::SetRGBA:
    default:
      ref_color = m_Color;
      break;
  }
}

void WMsgSetColor::ModifyColor(WColorGammaUB& ref_color) const
{
  WColor temp = ref_color;
  ModifyColor(temp);
  ref_color = temp;
}

void WMsgSetColor::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_Color;
  inout_stream << m_Mode;
}

void WMsgSetColor::Deserialize(WStreamReader& inout_stream, WUInt8 uiTypeVersion)
{
  W_IGNORE_UNUSED(uiTypeVersion);

  inout_stream >> m_Color;
  inout_stream >> m_Mode;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgSetCustomData);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgSetCustomData, 1, WRTTIDefaultAllocator<WMsgSetCustomData>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("Data", m_vData),
    } W_END_PROPERTIES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WMsgSetCustomData::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_vData;
}

void WMsgSetCustomData::Deserialize(WStreamReader& inout_stream, WUInt8 uiTypeVersion)
{
  W_IGNORE_UNUSED(uiTypeVersion);

  inout_stream >> m_vData;
}



W_STATICLINK_FILE(Core, Core_Messages_Implementation_SetColorMessage);
