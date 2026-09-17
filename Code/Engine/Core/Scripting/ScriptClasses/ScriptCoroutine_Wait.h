#pragma once

#include <Core/Scripting/ScriptCoroutine.h>

/// Script coroutine that pauses execution for a specified duration.
///
/// Simple timing coroutine that delays script execution for a given time period.
/// Useful for creating delays in script sequences or implementing timed behaviors.
class W_CORE_DLL WScriptCoroutine_Wait : public WTypedScriptCoroutine<WScriptCoroutine_Wait, WTime>
{
public:
  /// Initiates the wait period for the specified duration.
  void Start(WTime timeout);
  virtual Result Update(WTime deltaTimeSinceLastUpdate) override;

private:
  WTime m_TimeRemaing;
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WScriptCoroutine_Wait);
