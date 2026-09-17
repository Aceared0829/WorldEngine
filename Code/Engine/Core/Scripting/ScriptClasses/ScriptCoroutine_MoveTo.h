#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/CurveFunctions.h>

/// Script coroutine that smoothly moves a game object to a target position over time.
///
/// Provides interpolated movement with configurable easing curves for animation effects.
/// The object's position is updated each frame until the target is reached or the duration expires.
class W_CORE_DLL WScriptCoroutine_MoveTo : public WTypedScriptCoroutine<WScriptCoroutine_MoveTo, WGameObjectHandle, WVec3, WTime, WEnum<WCurveFunction>>
{
public:
  /// Initiates the move operation to the specified target position.
  void Start(WGameObjectHandle hObject, const WVec3& vTargetPos, WTime duration, WEnum<WCurveFunction> easing);
  virtual Result Update(WTime deltaTimeSinceLastUpdate) override;

private:
  WGameObjectHandle m_hObject;
  WVec3 m_vSourcePos;
  WVec3 m_vTargetPos;
  WEnum<WCurveFunction> m_Easing;

  WTime m_Duration;
  WTime m_TimePassed;
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WScriptCoroutine_MoveTo);
