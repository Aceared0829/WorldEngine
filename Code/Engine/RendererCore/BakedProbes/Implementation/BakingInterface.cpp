#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/BakedProbes/BakingInterface.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WBakingSettings, WNoBase, 1, WRTTIDefaultAllocator<WBakingSettings>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ProbeSpacing", m_vProbeSpacing)->AddAttributes(new WDefaultValueAttribute(WVec3(4)), new WClampValueAttribute(WVec3(0.1f), WVariant())),
    W_MEMBER_PROPERTY("NumSamplesPerProbe", m_uiNumSamplesPerProbe)->AddAttributes(new WDefaultValueAttribute(128), new WClampValueAttribute(32, 1024)),
    W_MEMBER_PROPERTY("MaxRayDistance", m_fMaxRayDistance)->AddAttributes(new WDefaultValueAttribute(1000), new WClampValueAttribute(1, WVariant())),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

static WTypeVersion s_BakingSettingsVersion = 1;
WResult WBakingSettings::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_BakingSettingsVersion);

  inout_stream << m_vProbeSpacing;
  inout_stream << m_uiNumSamplesPerProbe;
  inout_stream << m_fMaxRayDistance;

  return W_SUCCESS;
}

WResult WBakingSettings::Deserialize(WStreamReader& inout_stream)
{
  const WTypeVersion version = inout_stream.ReadVersion(s_BakingSettingsVersion);
  W_IGNORE_UNUSED(version);

  inout_stream >> m_vProbeSpacing;
  inout_stream >> m_uiNumSamplesPerProbe;
  inout_stream >> m_fMaxRayDistance;

  return W_SUCCESS;
}


W_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakingInterface);
