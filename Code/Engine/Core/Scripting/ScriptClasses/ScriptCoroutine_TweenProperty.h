#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/CurveFunctions.h>

/// Script coroutine that animates a component property value over time.
///
/// Provides smooth interpolation between the current and target property values
/// using configurable easing curves. Supports any property type that can be
/// represented as a variant and interpolated.
class W_CORE_DLL WScriptCoroutine_TweenProperty : public WTypedScriptCoroutine<WScriptCoroutine_TweenProperty, WComponentHandle, WStringView, WVariant, WTime, WEnum<WCurveFunction>>
{
public:
  /// Initiates the property animation to the specified target value.
  void Start(WComponentHandle hComponent, WStringView sPropertyName, WVariant targetValue, WTime duration, WEnum<WCurveFunction> easing);
  virtual Result Update(WTime deltaTimeSinceLastUpdate) override;

private:
  const WAbstractMemberProperty* m_pProperty = nullptr;
  WComponentHandle m_hComponent;
  WVariant m_SourceValue;
  WVariant m_TargetValue;
  WEnum<WCurveFunction> m_Easing;

  WTime m_Duration;
  WTime m_TimePassed;
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WScriptCoroutine_TweenProperty);
