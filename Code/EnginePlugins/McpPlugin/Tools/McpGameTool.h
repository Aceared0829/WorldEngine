#pragma once

#include <McpPlugin/McpPluginDLL.h>

#include <Foundation/Time/Time.h>
#include <Mcp/McpTool.h>

/// What the game is doing, and letting it run.
///
/// The distinction against app_* is process versus game: the port, the executable and the log belong to
/// the process and are answered by the shared WMcpAppTool, while a world, a game state and a clock only
/// exist here.
///
/// Frame-accurate *stepping* is deliberately absent. WClock could do it - pause, fixed timestep,
/// unpause for N frames - but a frame is the wrong unit for a game whose simulation is not frame driven.
/// A turn based game that interpolates between recorded turns would find that pausing the clock freezes
/// playback and not the simulation, so the useful stepping belongs in that game's own tool provider,
/// which the reflection-based registry already allows any game plugin to add.
class WMcpGameTool : public WMcpToolProvider
{
  W_ADD_DYNAMIC_REFLECTION(WMcpGameTool, WMcpToolProvider);

public:
  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

private:
  void ExecuteInfo(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteWait(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecutePause(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteSpeed(const WVariantDictionary& arguments, WMcpToolResult& out_result);

  /// Writes the clock's state, shared by game_info, game_pause and game_speed.
  static void WriteClockState(WMcpJsonWriter& ref_writer);

  /// game_wait cannot loop - it runs inside a frame, so frames only happen after it returns. It defers
  /// instead (WMcpToolResult::m_bNotFinished) and is re-entered once per frame until the target is
  /// reached; this is what survives across those calls.
  bool m_bWaiting = false;
  WUInt64 m_uiWaitUntilFrame = 0;
  WUInt64 m_uiWaitStartFrame = 0;
  WTime m_WaitStarted;
  WTime m_WaitTimeout;

  /// Frames are cheap but not free, and a caller that asks for a million of them has made a mistake
  /// that would otherwise look exactly like a hang.
  static constexpr WUInt32 s_uiMaxFrames = 10000;

  /// How long to keep waiting when frames are not arriving at all. The case this exists for is the
  /// editor's engine process, which renders only when the editor asks it to: without a limit,
  /// 'wait 10 frames' there would never return.
  static constexpr WTime s_DefaultTimeout = WTime::MakeFromSeconds(30);
};
