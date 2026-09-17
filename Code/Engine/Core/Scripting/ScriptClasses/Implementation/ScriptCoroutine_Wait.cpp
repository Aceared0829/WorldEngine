#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_Wait.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WScriptCoroutine_Wait, WScriptCoroutine, 1, WRTTIDefaultAllocator<WScriptCoroutine_Wait>)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(Start, In, "Timeout"),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Coroutine::Wait {Timeout}"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

void WScriptCoroutine_Wait::Start(WTime timeout)
{
  m_TimeRemaing = timeout;
}

WScriptCoroutine::Result WScriptCoroutine_Wait::Update(WTime deltaTimeSinceLastUpdate)
{
  m_TimeRemaing -= deltaTimeSinceLastUpdate;
  if (m_TimeRemaing.IsPositive())
  {
    // Don't wait for the full remaining time to prevent oversleeping due to scheduling precision.
    return Result::Running(m_TimeRemaing * 0.8);
  }

  return Result::Completed();
}


W_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptCoroutine_Wait);
