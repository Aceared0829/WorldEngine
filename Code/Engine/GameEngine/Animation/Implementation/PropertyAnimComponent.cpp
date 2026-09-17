#include <GameEngine/GameEnginePCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/Animation/PropertyAnimComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WPropertyAnimComponent, 3, WComponentMode::Dynamic)
  {
    W_BEGIN_PROPERTIES
    {
      W_RESOURCE_MEMBER_PROPERTY("Animation", m_hPropertyAnim)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Property_Animation"), new WRequiredAttribute()),
      W_MEMBER_PROPERTY("Playing", m_bPlaying)->AddAttributes(new WDefaultValueAttribute(true)),
      W_ENUM_MEMBER_PROPERTY("Mode", WPropertyAnimMode, m_AnimationMode),
      W_MEMBER_PROPERTY("RandomOffset", m_RandomOffset)->AddAttributes(new WClampValueAttribute(WTime::MakeFromSeconds(0), WVariant())),
      W_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(-10.0f, +10.0f)),
      W_MEMBER_PROPERTY("RangeLow", m_AnimationRangeLow)->AddAttributes(new WClampValueAttribute(WTime(), WVariant())),
      W_MEMBER_PROPERTY("RangeHigh", m_AnimationRangeHigh)->AddAttributes(new WClampValueAttribute(WTime(), WVariant()), new WDefaultValueAttribute(WTime::MakeFromSeconds(60 * 60))),
    }
    W_END_PROPERTIES;
    W_BEGIN_ATTRIBUTES
    {
      new WCategoryAttribute("Animation"),
    }
    W_END_ATTRIBUTES;
    W_BEGIN_MESSAGEHANDLERS
    {
      W_MESSAGE_HANDLER(WMsgSetPlaying, OnMsgSetPlaying),
    }
    W_END_MESSAGEHANDLERS;
    W_BEGIN_MESSAGESENDERS
    {
      W_MESSAGE_SENDER(m_EventTrackMsgSender),
      W_MESSAGE_SENDER(m_ReachedEndMsgSender),
    }
    W_END_MESSAGESENDERS;
    W_BEGIN_FUNCTIONS
    {
      W_SCRIPT_FUNCTION_PROPERTY(PlayAnimationRange, In, "RangeLow", In, "RangeHigh")
    }
    W_END_FUNCTIONS;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WPropertyAnimComponent::WPropertyAnimComponent()
{
  m_AnimationRangeHigh = WTime::MakeFromSeconds(60.0 * 60.0);
}

WPropertyAnimComponent::~WPropertyAnimComponent() = default;

void WPropertyAnimComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hPropertyAnim;
  s << m_AnimationMode;
  s << m_RandomOffset;
  s << m_fSpeed;
  s << m_AnimationTime;
  s << m_bReverse;
  s << m_AnimationRangeLow;
  s << m_AnimationRangeHigh;

  s << m_bPlaying;

  /// \todo Somehow store the animation state (not necessary for new scenes, but for quicksaves)
}

void WPropertyAnimComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hPropertyAnim;

  if (uiVersion >= 2)
  {
    s >> m_AnimationMode;
    s >> m_RandomOffset;
    s >> m_fSpeed;
    s >> m_AnimationTime;
    s >> m_bReverse;
    s >> m_AnimationRangeLow;
    s >> m_AnimationRangeHigh;
  }

  if (uiVersion >= 3)
  {
    s >> m_bPlaying;
  }
}

void WPropertyAnimComponent::SetPropertyAnim(const WPropertyAnimResourceHandle& hPropertyAnim)
{
  m_hPropertyAnim = hPropertyAnim;
}

void WPropertyAnimComponent::PlayAnimationRange(WTime rangeLow, WTime rangeHigh)
{
  m_AnimationRangeLow = rangeLow;
  m_AnimationRangeHigh = rangeHigh;

  m_bPlaying = true;

  StartPlayback();
}


void WPropertyAnimComponent::OnMsgSetPlaying(WMsgSetPlaying& ref_msg)
{
  m_bPlaying = ref_msg.m_bPlay;
}

void WPropertyAnimComponent::CreatePropertyBindings()
{
  m_ColorBindings.Clear();
  m_ComponentFloatBindings.Clear();
  m_GoFloatBindings.Clear();

  m_pAnimDesc = nullptr;

  if (!m_hPropertyAnim.IsValid())
    return;

  WResourceLock<WPropertyAnimResource> pAnimation(m_hPropertyAnim, WResourceAcquireMode::BlockTillLoaded);

  if (!pAnimation || pAnimation.GetAcquireResult() == WResourceAcquireResult::MissingFallback)
    return;

  m_pAnimDesc = pAnimation->GetDescriptor();

  for (const WFloatPropertyAnimEntry& anim : m_pAnimDesc->m_FloatAnimations)
  {
    WTempHybridArray<WGameObject*, 8> targets;
    GetOwner()->SearchForChildrenByNameSequence(anim.m_sObjectSearchSequence, anim.m_pComponentRtti, targets);

    for (WGameObject* pTargetObject : targets)
    {
      // allow to animate properties on the WGameObject
      if (anim.m_pComponentRtti == nullptr)
      {
        CreateGameObjectBinding(&anim, WGetStaticRTTI<WGameObject>(), pTargetObject, pTargetObject->GetHandle());
      }
      else
      {
        WComponent* pComp;
        if (pTargetObject->TryGetComponentOfBaseType(anim.m_pComponentRtti, pComp))
        {
          CreateFloatPropertyBinding(&anim, pComp->GetDynamicRTTI(), pComp, pComp->GetHandle());
        }
      }
    }
  }

  for (const WColorPropertyAnimEntry& anim : m_pAnimDesc->m_ColorAnimations)
  {
    WTempHybridArray<WGameObject*, 8> targets;
    GetOwner()->SearchForChildrenByNameSequence(anim.m_sObjectSearchSequence, anim.m_pComponentRtti, targets);

    for (WGameObject* pTargetObject : targets)
    {
      WComponent* pComp;
      if (pTargetObject->TryGetComponentOfBaseType(anim.m_pComponentRtti, pComp))
      {
        CreateColorPropertyBinding(&anim, pComp->GetDynamicRTTI(), pComp, pComp->GetHandle());
      }
    }
  }
}

void WPropertyAnimComponent::CreateGameObjectBinding(const WFloatPropertyAnimEntry* pAnim, const WRTTI* pOwnerRtti, void* pObject, const WGameObjectHandle& hGameObject)
{
  if (pAnim->m_Target < WPropertyAnimTarget::Number || pAnim->m_Target > WPropertyAnimTarget::RotationZ)
    return;

  const WAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyPath);

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract == nullptr || pAbstract->GetCategory() != WPropertyCategory::Member)
    return;

  auto pMember = static_cast<const WAbstractMemberProperty*>(pAbstract);

  const WRTTI* pPropRtti = pMember->GetSpecificType();

  if (pAnim->m_Target == WPropertyAnimTarget::Number)
  {
    // Game objects only support to animate Position, Rotation,
    // Non-Uniform Scale, the one single-float Uniform scale value
    // and the active flag
    if (pPropRtti != WGetStaticRTTI<float>() && pPropRtti != WGetStaticRTTI<bool>())
      return;
  }
  else if (pAnim->m_Target >= WPropertyAnimTarget::RotationX && pAnim->m_Target <= WPropertyAnimTarget::RotationZ)
  {
    if (pPropRtti != WGetStaticRTTI<WQuat>())
      return;
  }
  else
  {
    if (pPropRtti != WGetStaticRTTI<WVec2>() && pPropRtti != WGetStaticRTTI<WVec3>() && pPropRtti != WGetStaticRTTI<WVec4>())
      return;
  }

  GameObjectBinding* binding = nullptr;
  for (WUInt32 i = 0; i < m_GoFloatBindings.GetCount(); ++i)
  {
    auto& b = m_GoFloatBindings[i];

    if (b.m_hObject == hGameObject && b.m_pMemberProperty == pMember && b.m_pObject == pObject)
    {
      binding = &b;
      break;
    }
  }

  if (binding == nullptr)
  {
    binding = &m_GoFloatBindings.ExpandAndGetRef();
  }

  binding->m_hObject = hGameObject;
  binding->m_pObject = pObject;
  binding->m_pMemberProperty = pMember;

  // we can store a direct pointer here, because our sharedptr keeps the descriptor alive

  if (pAnim->m_Target >= WPropertyAnimTarget::VectorX && pAnim->m_Target <= WPropertyAnimTarget::VectorW)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)WPropertyAnimTarget::VectorX] = pAnim;
  }
  else if (pAnim->m_Target >= WPropertyAnimTarget::RotationX && pAnim->m_Target <= WPropertyAnimTarget::RotationZ)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)WPropertyAnimTarget::RotationX] = pAnim;
  }
  else if (pAnim->m_Target >= WPropertyAnimTarget::Number)
  {
    binding->m_pAnimation[0] = pAnim;
  }
  else
  {
    W_REPORT_FAILURE("Invalid animation target type '{0}'", WArgEnum(pAnim->m_Target));
  }
}

void WPropertyAnimComponent::CreateFloatPropertyBinding(const WFloatPropertyAnimEntry* pAnim, const WRTTI* pOwnerRtti, void* pObject, const WComponentHandle& hComponent)
{
  if (pAnim->m_Target < WPropertyAnimTarget::Number || pAnim->m_Target > WPropertyAnimTarget::VectorW)
    return;

  const WAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyPath);

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract == nullptr || pAbstract->GetCategory() != WPropertyCategory::Member)
    return;

  auto pMember = static_cast<const WAbstractMemberProperty*>(pAbstract);

  const WRTTI* pPropRtti = pMember->GetSpecificType();

  if (pAnim->m_Target == WPropertyAnimTarget::Number)
  {
    if (pPropRtti != WGetStaticRTTI<float>() && pPropRtti != WGetStaticRTTI<double>() && pPropRtti != WGetStaticRTTI<bool>() && pPropRtti != WGetStaticRTTI<WInt64>() && pPropRtti != WGetStaticRTTI<WInt32>() && pPropRtti != WGetStaticRTTI<WInt16>() &&
        pPropRtti != WGetStaticRTTI<WInt8>() && pPropRtti != WGetStaticRTTI<WUInt64>() && pPropRtti != WGetStaticRTTI<WUInt32>() && pPropRtti != WGetStaticRTTI<WUInt16>() && pPropRtti != WGetStaticRTTI<WUInt8>() && pPropRtti != WGetStaticRTTI<WAngle>() &&
        pPropRtti != WGetStaticRTTI<WTime>())
      return;
  }
  else if (pAnim->m_Target >= WPropertyAnimTarget::VectorX && pAnim->m_Target <= WPropertyAnimTarget::VectorW)
  {
    if (pPropRtti != WGetStaticRTTI<WVec2>() && pPropRtti != WGetStaticRTTI<WVec3>() && pPropRtti != WGetStaticRTTI<WVec4>())
      return;
  }
  else
  {
    // Quaternions are not supported for regular types
    return;
  }

  ComponentFloatBinding* binding = nullptr;
  for (WUInt32 i = 0; i < m_ComponentFloatBindings.GetCount(); ++i)
  {
    auto& b = m_ComponentFloatBindings[i];

    if (b.m_hComponent == hComponent && b.m_pMemberProperty == pMember && b.m_pObject == pObject)
    {
      binding = &b;
      break;
    }
  }

  if (binding == nullptr)
  {
    binding = &m_ComponentFloatBindings.ExpandAndGetRef();
  }

  binding->m_hComponent = hComponent;
  binding->m_pObject = pObject;
  binding->m_pMemberProperty = pMember;

  // we can store a direct pointer here, because our sharedptr keeps the descriptor alive
  if (pAnim->m_Target >= WPropertyAnimTarget::VectorX && pAnim->m_Target <= WPropertyAnimTarget::VectorW)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)WPropertyAnimTarget::VectorX] = pAnim;
  }
  else if (pAnim->m_Target >= WPropertyAnimTarget::RotationX && pAnim->m_Target <= WPropertyAnimTarget::RotationZ)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)WPropertyAnimTarget::RotationX] = pAnim;
  }
  else if (pAnim->m_Target >= WPropertyAnimTarget::Number)
  {
    binding->m_pAnimation[0] = pAnim;
  }
  else
  {
    W_REPORT_FAILURE("Invalid animation target type '{0}'", WArgEnum(pAnim->m_Target));
  }
}

void WPropertyAnimComponent::CreateColorPropertyBinding(const WColorPropertyAnimEntry* pAnim, const WRTTI* pOwnerRtti, void* pObject, const WComponentHandle& hComponent)
{
  if (pAnim->m_Target != WPropertyAnimTarget::Color)
    return;

  const WAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyPath);

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract == nullptr || pAbstract->GetCategory() != WPropertyCategory::Member)
    return;

  auto pMember = static_cast<const WAbstractMemberProperty*>(pAbstract);

  const WRTTI* pPropRtti = pMember->GetSpecificType();

  if (pPropRtti != WGetStaticRTTI<WColor>() && pPropRtti != WGetStaticRTTI<WColorGammaUB>())
    return;

  ColorBinding& binding = m_ColorBindings.ExpandAndGetRef();
  binding.m_hComponent = hComponent;
  binding.m_pObject = pObject;
  binding.m_pAnimation = pAnim; // we can store a direct pointer here, because our SharedPtr keeps the descriptor alive
  binding.m_pMemberProperty = pMember;
}

void WPropertyAnimComponent::ApplyAnimations(const WTime& tDiff)
{
  if (m_fSpeed == 0.0f || m_pAnimDesc == nullptr)
    return;

  const WTime fLookupPos = ComputeAnimationLookup(tDiff);

  for (WUInt32 i = 0; i < m_ComponentFloatBindings.GetCount();)
  {
    const auto& binding = m_ComponentFloatBindings[i];

    // if we have a component handle, use it to check that the component is still alive
    if (!binding.m_hComponent.IsInvalidated())
    {
      WComponent* pComponent;
      if (!GetWorld()->TryGetComponent(binding.m_hComponent, pComponent))
      {
        // remove dead references
        m_ComponentFloatBindings.RemoveAtAndSwap(i);
        continue;
      }

      binding.m_pObject = static_cast<void*>(pComponent);
    }

    ApplyFloatAnimation(m_ComponentFloatBindings[i], fLookupPos);

    ++i;
  }

  for (WUInt32 i = 0; i < m_ColorBindings.GetCount();)
  {
    const auto& binding = m_ColorBindings[i];

    // if we have a component handle, use it to check that the component is still alive
    if (!binding.m_hComponent.IsInvalidated())
    {
      WComponent* pComponent;
      if (!GetWorld()->TryGetComponent(binding.m_hComponent, pComponent))

      {
        // remove dead references
        m_ColorBindings.RemoveAtAndSwap(i);
        continue;
      }

      binding.m_pObject = static_cast<void*>(pComponent);
    }

    ApplyColorAnimation(m_ColorBindings[i], fLookupPos);

    ++i;
  }

  for (WUInt32 i = 0; i < m_GoFloatBindings.GetCount();)
  {
    const auto& binding = m_GoFloatBindings[i];

    // if we have a game object handle, use it to check that the component is still alive
    if (!binding.m_hObject.IsInvalidated())
    {
      WGameObject* pObject;
      if (!GetWorld()->TryGetObject(binding.m_hObject, pObject))
      {
        // remove dead references
        m_GoFloatBindings.RemoveAtAndSwap(i);
        continue;
      }

      binding.m_pObject = static_cast<void*>(pObject);
    }

    ApplyFloatAnimation(m_GoFloatBindings[i], fLookupPos);

    ++i;
  }
}

WTime WPropertyAnimComponent::ComputeAnimationLookup(WTime tDiff)
{
  m_AnimationRangeLow = WMath::Clamp(m_AnimationRangeLow, WTime::MakeZero(), m_pAnimDesc->m_AnimationDuration);
  m_AnimationRangeHigh = WMath::Clamp(m_AnimationRangeHigh, m_AnimationRangeLow, m_pAnimDesc->m_AnimationDuration);

  const WTime duration = m_AnimationRangeHigh - m_AnimationRangeLow;

  if (duration.IsZero())
  {
    m_bPlaying = false;
    return m_AnimationRangeLow;
  }

  tDiff = m_fSpeed * tDiff;

  WMsgAnimationReachedEnd reachedEndMsg;
  WTime tStart = m_AnimationTime;

  if (m_AnimationMode == WPropertyAnimMode::Once)
  {
    m_AnimationTime += tDiff;

    if (m_fSpeed > 0 && m_AnimationTime >= m_AnimationRangeHigh)
    {
      m_AnimationTime = m_AnimationRangeHigh;
      m_bPlaying = false;

      m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());
    }
    else if (m_fSpeed < 0 && m_AnimationTime <= m_AnimationRangeLow)
    {
      m_AnimationTime = m_AnimationRangeLow;
      m_bPlaying = false;

      m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());
    }

    EvaluateEventTrack(tStart, m_AnimationTime);
  }
  else if (m_AnimationMode == WPropertyAnimMode::Loop)
  {
    m_AnimationTime += tDiff;

    while (m_AnimationTime > m_AnimationRangeHigh)
    {
      m_AnimationTime -= duration;

      m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());

      EvaluateEventTrack(tStart, m_AnimationRangeHigh);
      tStart = m_AnimationRangeLow;
    }

    while (m_AnimationTime < m_AnimationRangeLow)
    {
      m_AnimationTime += duration;

      m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());

      EvaluateEventTrack(tStart, m_AnimationRangeLow);
      tStart = m_AnimationRangeHigh;
    }

    EvaluateEventTrack(tStart, m_AnimationTime);
  }
  else if (m_AnimationMode == WPropertyAnimMode::BackAndForth)
  {
    const bool bReverse = m_fSpeed < 0 ? !m_bReverse : m_bReverse;

    if (bReverse)
      m_AnimationTime -= tDiff;
    else
      m_AnimationTime += tDiff;

    // ping pong back and forth as long as the current animation time is outside the valid range
    while (true)
    {
      if (m_AnimationTime > m_AnimationRangeHigh)
      {
        m_AnimationTime = m_AnimationRangeHigh - (m_AnimationTime - m_AnimationRangeHigh);
        m_bReverse = true;

        EvaluateEventTrack(tStart, m_AnimationRangeHigh);
        tStart = m_AnimationRangeHigh;

        m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());
      }
      else if (m_AnimationTime < m_AnimationRangeLow)
      {
        m_AnimationTime = m_AnimationRangeLow + (m_AnimationRangeLow - m_AnimationTime);
        m_bReverse = false;

        EvaluateEventTrack(tStart, m_AnimationRangeLow);
        tStart = m_AnimationRangeLow;

        m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());
      }
      else
      {
        EvaluateEventTrack(tStart, m_AnimationTime);
        break;
      }
    }
  }

  return m_AnimationTime;
}

void WPropertyAnimComponent::EvaluateEventTrack(WTime startTime, WTime endTime)
{
  const WEventTrack& et = m_pAnimDesc->m_EventTrack;

  if (et.IsEmpty())
    return;

  WTempHybridArray<WHashedString, 8> events;
  et.Sample(startTime, endTime, events);

  for (const WHashedString& sEvent : events)
  {
    WMsgGenericEvent msg;
    msg.m_sMessage = sEvent;
    m_EventTrackMsgSender.SendEventMessage(msg, this, GetOwner());
  }
}

void WPropertyAnimComponent::OnSimulationStarted()
{
  CreatePropertyBindings();

  StartPlayback();
}

void WPropertyAnimComponent::StartPlayback()
{
  if (m_pAnimDesc == nullptr)
    return;

  m_AnimationRangeLow = WMath::Clamp(m_AnimationRangeLow, WTime::MakeZero(), m_pAnimDesc->m_AnimationDuration);
  m_AnimationRangeHigh = WMath::Clamp(m_AnimationRangeHigh, m_AnimationRangeLow, m_pAnimDesc->m_AnimationDuration);

  // when starting with a negative speed, start at the end of the animation and play backwards
  // important for play-once mode
  if (m_fSpeed < 0.0f)
  {
    m_AnimationTime = m_AnimationRangeHigh;
  }
  else
  {
    m_AnimationTime = m_AnimationRangeLow;
  }

  if (!m_RandomOffset.IsZero() && m_pAnimDesc->m_AnimationDuration.IsPositive())
  {
    // should the random offset also be scaled by the speed factor? I guess not
    m_AnimationTime += WMath::Abs(m_fSpeed) * WTime::MakeFromSeconds(GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, m_RandomOffset.GetSeconds()));

    const WTime duration = m_AnimationRangeHigh - m_AnimationRangeLow;

    if (duration.IsZeroOrNegative())
    {
      m_AnimationTime = m_AnimationRangeLow;
    }
    else
    {
      // adjust current time to be inside the valid range
      // do not clamp, as that would give a skewed random chance
      while (m_AnimationTime > m_AnimationRangeHigh)
      {
        m_AnimationTime -= duration;
      }

      while (m_AnimationTime < m_AnimationRangeLow)
      {
        m_AnimationTime += duration;
      }
    }
  }
}

void WPropertyAnimComponent::ApplySingleFloatAnimation(const FloatBinding& binding, WTime lookupTime)
{
  const WRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  double fFinalValue = 0;
  {
    const WCurve1D& curve = binding.m_pAnimation[0]->m_Curve;

    if (curve.IsEmpty())
      return;

    fFinalValue = curve.Evaluate(lookupTime.GetSeconds());
  }

  if (pRtti == WGetStaticRTTI<bool>())
  {
    auto pTyped = static_cast<const WTypedMemberProperty<bool>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, fFinalValue > 0.99); // this is close to what WVariant does (not identical, that does an int cast != 0), but faster to evaluate
    return;
  }
  else if (pRtti == WGetStaticRTTI<WAngle>())
  {
    auto pTyped = static_cast<const WTypedMemberProperty<WAngle>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, WAngle::MakeFromDegree((float)fFinalValue));
    return;
  }
  else if (pRtti == WGetStaticRTTI<WTime>())
  {
    auto pTyped = static_cast<const WTypedMemberProperty<WTime>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, WTime::MakeFromSeconds(fFinalValue));
    return;
  }

  // this handles float, double, all int types, etc.
  WVariant value = fFinalValue;
  if (pRtti->GetVariantType() != WVariantType::Invalid && value.CanConvertTo(pRtti->GetVariantType()))
  {
    WReflectionUtils::SetMemberPropertyValue(binding.m_pMemberProperty, binding.m_pObject, value);
  }
}

void WPropertyAnimComponent::ApplyFloatAnimation(const FloatBinding& binding, WTime lookupTime)
{
  if (binding.m_pAnimation[0] != nullptr && binding.m_pAnimation[0]->m_Target == WPropertyAnimTarget::Number)
  {
    ApplySingleFloatAnimation(binding, lookupTime);
    return;
  }

  const WRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  float fCurValue[4] = {0, 0, 0, 0};

  if (pRtti == WGetStaticRTTI<WVec2>())
  {
    auto pTyped = static_cast<const WTypedMemberProperty<WVec2>*>(binding.m_pMemberProperty);
    const WVec2 value = pTyped->GetValue(binding.m_pObject);

    fCurValue[0] = value.x;
    fCurValue[1] = value.y;
  }
  else if (pRtti == WGetStaticRTTI<WVec3>())
  {
    auto pTyped = static_cast<const WTypedMemberProperty<WVec3>*>(binding.m_pMemberProperty);
    const WVec3 value = pTyped->GetValue(binding.m_pObject);

    fCurValue[0] = value.x;
    fCurValue[1] = value.y;
    fCurValue[2] = value.z;
  }
  else if (pRtti == WGetStaticRTTI<WVec4>())
  {
    auto pTyped = static_cast<const WTypedMemberProperty<WVec4>*>(binding.m_pMemberProperty);
    const WVec4 value = pTyped->GetValue(binding.m_pObject);

    fCurValue[0] = value.x;
    fCurValue[1] = value.y;
    fCurValue[2] = value.z;
    fCurValue[3] = value.w;
  }
  else if (pRtti == WGetStaticRTTI<WQuat>())
  {
    auto pTyped = static_cast<const WTypedMemberProperty<WQuat>*>(binding.m_pMemberProperty);
    const WQuat value = pTyped->GetValue(binding.m_pObject);

    WAngle euler[3];
    value.GetAsEulerAngles(euler[0], euler[1], euler[2]);
    fCurValue[0] = euler[0].GetDegree();
    fCurValue[1] = euler[1].GetDegree();
    fCurValue[2] = euler[2].GetDegree();
  }

  // evaluate all available curves
  for (WUInt32 i = 0; i < 4; ++i)
  {
    if (binding.m_pAnimation[i] != nullptr)
    {
      const WCurve1D& curve = binding.m_pAnimation[i]->m_Curve;

      if (!curve.IsEmpty())
      {
        fCurValue[i] = (float)curve.Evaluate(lookupTime.GetSeconds());
      }
    }
  }

  if (pRtti == WGetStaticRTTI<WVec2>())
  {
    auto pTyped = static_cast<const WTypedMemberProperty<WVec2>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, WVec2(fCurValue[0], fCurValue[1]));
  }
  else if (pRtti == WGetStaticRTTI<WVec3>())
  {
    auto pTyped = static_cast<const WTypedMemberProperty<WVec3>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, WVec3(fCurValue[0], fCurValue[1], fCurValue[2]));
  }
  else if (pRtti == WGetStaticRTTI<WVec4>())
  {
    auto pTyped = static_cast<const WTypedMemberProperty<WVec4>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, WVec4(fCurValue[0], fCurValue[1], fCurValue[2], fCurValue[3]));
  }
  else if (pRtti == WGetStaticRTTI<WQuat>())
  {
    auto pTyped = static_cast<const WTypedMemberProperty<WQuat>*>(binding.m_pMemberProperty);

    WQuat rot = WQuat::MakeFromEulerAngles(WAngle::MakeFromDegree(fCurValue[0]), WAngle::MakeFromDegree(fCurValue[1]), WAngle::MakeFromDegree(fCurValue[2]));

    pTyped->SetValue(binding.m_pObject, rot);
  }
}

void WPropertyAnimComponent::ApplyColorAnimation(const ColorBinding& binding, WTime lookupTime)
{
  const WRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  if (pRtti == WGetStaticRTTI<WColorGammaUB>())
  {
    WColorGammaUB gamma;
    float intensity;
    binding.m_pAnimation->m_Gradient.Evaluate(lookupTime.AsFloatInSeconds(), gamma, intensity);
    binding.m_pMemberProperty->SetValuePtr(binding.m_pObject, &gamma);
    return;
  }

  if (pRtti == WGetStaticRTTI<WColor>())
  {
    WColorGammaUB gamma;
    float intensity;
    binding.m_pAnimation->m_Gradient.Evaluate(lookupTime.AsFloatInSeconds(), gamma, intensity);

    WColor finalColor = gamma;
    finalColor.ScaleRGB(intensity);
    binding.m_pMemberProperty->SetValuePtr(binding.m_pObject, &finalColor);
    return;
  }
}

void WPropertyAnimComponent::Update()
{
  if (m_bPlaying == false || !m_hPropertyAnim.IsValid())
    return;

  if (m_pAnimDesc == nullptr)
  {
    CreatePropertyBindings();
  }

  ApplyAnimations(GetWorld()->GetClock().GetTimeDiff());
}



W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_PropertyAnimComponent);
