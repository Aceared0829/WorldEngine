#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_MoveTo.h>
#include <Core/World/World.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WScriptCoroutine_MoveTo, WScriptCoroutine, 1, WRTTIDefaultAllocator<WScriptCoroutine_MoveTo>)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(Start, In, "Object", In, "TargetPos", In, "Duration", In, "Easing"),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Coroutine::MoveTo {TargetPos}"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

void WScriptCoroutine_MoveTo::Start(WGameObjectHandle hObject, const WVec3& vTargetPos, WTime duration, WEnum<WCurveFunction> easing)
{
  WGameObject* pObject = nullptr;
  if (WWorld::GetWorld(hObject)->TryGetObject(hObject, pObject) == false)
  {
    WLog::Error("MoveTo: The given game object was not found.");
    return;
  }

  m_hObject = hObject;
  m_vSourcePos = pObject->GetLocalPosition();
  m_vTargetPos = vTargetPos;
  m_Easing = easing;

  m_Duration = duration;
  m_TimePassed = WTime::MakeZero();
}

WScriptCoroutine::Result WScriptCoroutine_MoveTo::Update(WTime deltaTimeSinceLastUpdate)
{
  if (deltaTimeSinceLastUpdate.IsPositive())
  {
    WGameObject* pObject = nullptr;
    if (WWorld::GetWorld(m_hObject)->TryGetObject(m_hObject, pObject) == false)
    {
      return Result::Failed();
    }

    m_TimePassed += deltaTimeSinceLastUpdate;

    const double fDuration = m_Duration.GetSeconds();
    double fCurrentX = WMath::Min(fDuration > 0 ? m_TimePassed.GetSeconds() / fDuration : 1.0, 1.0);
    fCurrentX = WCurveFunction::GetValue(m_Easing, fCurrentX);

    WVec3 vCurrentPos = WMath::Lerp(m_vSourcePos, m_vTargetPos, static_cast<float>(fCurrentX));
    pObject->SetLocalPosition(vCurrentPos);
  }

  if (m_TimePassed < m_Duration)
  {
    return Result::Running();
  }

  return Result::Completed();
}


W_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptCoroutine_MoveTo);
