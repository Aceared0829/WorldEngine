#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_TweenProperty.h>
#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WScriptCoroutine_TweenProperty, WScriptCoroutine, 1, WRTTIDefaultAllocator<WScriptCoroutine_TweenProperty>)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(Start, In, "Component", In, "PropertyName", In, "TargetValue", In, "Duration", In, "Easing"),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Coroutine::TweenProperty {PropertyName}"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

void WScriptCoroutine_TweenProperty::Start(WComponentHandle hComponent, WStringView sPropertyName, WVariant targetValue, WTime duration, WEnum<WCurveFunction> easing)
{
  WComponent* pComponent = nullptr;
  if (WWorld::GetWorld(hComponent)->TryGetComponent(hComponent, pComponent) == false)
  {
    WLog::Error("TweenProperty: The given component was not found.");
    return;
  }

  auto pType = pComponent->GetDynamicRTTI();
  auto pProp = pType->FindPropertyByName(sPropertyName);
  if (pProp == nullptr || pProp->GetCategory() != WPropertyCategory::Member)
  {
    WLog::Error("TweenProperty: The given component of type '{}' does not have a member property named '{}'.", pType->GetTypeName(), sPropertyName);
    return;
  }

  WVariantType::Enum variantType = pProp->GetSpecificType()->GetVariantType();
  if (variantType == WVariantType::Invalid)
  {
    WLog::Error("TweenProperty: Can't tween property '{}' of type '{}'.", sPropertyName, pProp->GetSpecificType()->GetTypeName());
    return;
  }

  WResult conversionStatus = W_SUCCESS;
  m_TargetValue = targetValue.ConvertTo(variantType, &conversionStatus);
  if (conversionStatus.Failed())
  {
    WLog::Error("TweenProperty: Can't convert given target value to '{}'.", pProp->GetSpecificType()->GetTypeName());
    return;
  }

  m_pProperty = static_cast<const WAbstractMemberProperty*>(pProp);
  m_hComponent = hComponent;
  m_SourceValue = WReflectionUtils::GetMemberPropertyValue(m_pProperty, pComponent);
  m_Easing = easing;

  m_Duration = duration;
  m_TimePassed = WTime::MakeZero();
}

WScriptCoroutine::Result WScriptCoroutine_TweenProperty::Update(WTime deltaTimeSinceLastUpdate)
{
  if (m_pProperty == nullptr)
  {
    return Result::Failed();
  }

  if (deltaTimeSinceLastUpdate.IsPositive())
  {
    WComponent* pComponent = nullptr;
    if (WWorld::GetWorld(m_hComponent)->TryGetComponent(m_hComponent, pComponent) == false)
    {
      return Result::Failed();
    }

    m_TimePassed += deltaTimeSinceLastUpdate;

    const double fDuration = m_Duration.GetSeconds();
    double fCurrentX = WMath::Min(fDuration > 0 ? m_TimePassed.GetSeconds() / fDuration : 1.0, 1.0);
    fCurrentX = WCurveFunction::GetValue(m_Easing, fCurrentX);
    WVariant currentValue = WMath::Lerp(m_SourceValue, m_TargetValue, fCurrentX);

    WReflectionUtils::SetMemberPropertyValue(m_pProperty, pComponent, currentValue);
  }

  if (m_TimePassed < m_Duration)
  {
    return Result::Running();
  }

  return Result::Completed();
}


W_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptCoroutine_TweenProperty);
