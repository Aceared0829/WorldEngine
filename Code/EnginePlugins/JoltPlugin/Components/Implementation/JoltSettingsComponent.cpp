#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Components/JoltSettingsComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltSettingsComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("ObjectGravity", GetObjectGravity, SetObjectGravity)->AddAttributes(new WDefaultValueAttribute(WVec3(0, 0, -9.81f))),
    W_ACCESSOR_PROPERTY("CharacterGravity", GetCharacterGravity, SetCharacterGravity)->AddAttributes(new WDefaultValueAttribute(WVec3(0, 0, -12.0f))),
    W_ENUM_ACCESSOR_PROPERTY("SteppingMode", WJoltSteppingMode, GetSteppingMode, SetSteppingMode),
    W_ACCESSOR_PROPERTY("FixedFrameRate", GetFixedFrameRate, SetFixedFrameRate)->AddAttributes(new WDefaultValueAttribute(60.0f), new WClampValueAttribute(1.0f, 1000.0f)),
    W_ACCESSOR_PROPERTY("MaxSubSteps", GetMaxSubSteps, SetMaxSubSteps)->AddAttributes(new WDefaultValueAttribute(4), new WClampValueAttribute(1, 100)),
    W_ACCESSOR_PROPERTY("MaxBodies", GetMaxBodies, SetMaxBodies)->AddAttributes(new WDefaultValueAttribute(10000), new WClampValueAttribute(500, 1000000)),
    W_ACCESSOR_PROPERTY("SleepVelocityThreshold", GetSleepVelocityThreshold, SetSleepVelocityThreshold)->AddAttributes(new WDefaultValueAttribute(0.03f), new WClampValueAttribute(0.0f, WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Physics/Jolt/Misc"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltSettingsComponent::WJoltSettingsComponent() = default;
WJoltSettingsComponent::~WJoltSettingsComponent() = default;

void WJoltSettingsComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_Settings.m_vObjectGravity;
  s << m_Settings.m_vCharacterGravity;
  s << m_Settings.m_SteppingMode;
  s << m_Settings.m_fFixedFrameRate;
  s << m_Settings.m_uiMaxSubSteps;
  s << m_Settings.m_uiMaxBodies;
  s << m_Settings.m_fSleepVelocityThreshold;
}


void WJoltSettingsComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_Settings.m_vObjectGravity;
  s >> m_Settings.m_vCharacterGravity;
  s >> m_Settings.m_SteppingMode;
  s >> m_Settings.m_fFixedFrameRate;
  s >> m_Settings.m_uiMaxSubSteps;
  s >> m_Settings.m_uiMaxBodies;

  if (uiVersion >= 2)
  {
    s >> m_Settings.m_fSleepVelocityThreshold;
  }
}

void WJoltSettingsComponent::SetObjectGravity(const WVec3& v)
{
  m_Settings.m_vObjectGravity = v;
  SetModified(W_BIT(0));
}

void WJoltSettingsComponent::SetCharacterGravity(const WVec3& v)
{
  m_Settings.m_vCharacterGravity = v;
  SetModified(W_BIT(1));
}

void WJoltSettingsComponent::SetSteppingMode(WJoltSteppingMode::Enum mode)
{
  m_Settings.m_SteppingMode = mode;
  SetModified(W_BIT(3));
}

void WJoltSettingsComponent::SetFixedFrameRate(float fFixedFrameRate)
{
  m_Settings.m_fFixedFrameRate = fFixedFrameRate;
  SetModified(W_BIT(4));
}

void WJoltSettingsComponent::SetMaxSubSteps(WUInt32 uiMaxSubSteps)
{
  m_Settings.m_uiMaxSubSteps = uiMaxSubSteps;
  SetModified(W_BIT(5));
}

void WJoltSettingsComponent::SetMaxBodies(WUInt32 uiMaxBodies)
{
  m_Settings.m_uiMaxBodies = uiMaxBodies;
  SetModified(W_BIT(6));
}

void WJoltSettingsComponent::SetSleepVelocityThreshold(float fSleepVelocityThreshold)
{
  m_Settings.m_fSleepVelocityThreshold = fSleepVelocityThreshold;
  SetModified(W_BIT(7));
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltSettingsComponent);
