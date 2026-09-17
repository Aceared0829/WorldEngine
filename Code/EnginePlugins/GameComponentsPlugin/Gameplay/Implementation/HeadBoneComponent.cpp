#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Gameplay/HeadBoneComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WHeadBoneComponent, 1, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("VerticalRotation", m_MaxVerticalRotation)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(80)), new WClampValueAttribute(WAngle::MakeFromDegree(0.0f), WAngle::MakeFromDegree(89.0f))),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SetVerticalRotation, In, "Radians"),
    W_SCRIPT_FUNCTION_PROPERTY(ChangeVerticalRotation, In, "Radians"),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WHeadBoneComponent::WHeadBoneComponent() = default;
WHeadBoneComponent::~WHeadBoneComponent() = default;

void WHeadBoneComponent::Update()
{
  m_NewVerticalRotation = WMath::Clamp(m_NewVerticalRotation, -m_MaxVerticalRotation, m_MaxVerticalRotation);

  WQuat qOld, qNew;
  qOld = WQuat::MakeFromAxisAndAngle(WVec3(0, 1, 0), m_CurVerticalRotation);
  qNew = WQuat::MakeFromAxisAndAngle(WVec3(0, 1, 0), m_NewVerticalRotation);

  const WQuat qChange = qNew * qOld.GetInverse();

  const WQuat qFinalNew = qChange * GetOwner()->GetLocalRotation();

  GetOwner()->SetLocalRotation(qFinalNew);

  m_CurVerticalRotation = m_NewVerticalRotation;
}

void WHeadBoneComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  // Version 1
  s << m_MaxVerticalRotation;
  s << m_CurVerticalRotation;
}

void WHeadBoneComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  // Version 1
  s >> m_MaxVerticalRotation;
  s >> m_CurVerticalRotation;
}

void WHeadBoneComponent::SetVerticalRotation(float fRadians)
{
  m_NewVerticalRotation = WAngle::MakeFromRadian(fRadians);
}

void WHeadBoneComponent::ChangeVerticalRotation(float fRadians)
{
  m_NewVerticalRotation += WAngle::MakeFromRadian(fRadians);
}


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Gameplay_Implementation_HeadBoneComponent);
