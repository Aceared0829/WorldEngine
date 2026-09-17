#pragma once

#include <McpPlugin/McpPluginDLL.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Time/Time.h>
#include <Mcp/Tools/McpAppTool.h>
#include <Texture/Image/Image.h>

/// The game process' half of the shared app_* tools, plus taking a screenshot.
///
/// The process level questions - port, executable, command line, process id - are answered by the base
/// class, so an agent driving both an editor and the game it runs asks them the same way in both. What
/// is added here is what only a rendering, frame-stepping process can answer.
class WMcpEngineAppTool : public WMcpAppTool
{
  W_ADD_DYNAMIC_REFLECTION(WMcpEngineAppTool, WMcpAppTool);

public:
  ~WMcpEngineAppTool();

  virtual void OnDeactivate() override;

  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

protected:
  virtual WStringView GetHostNoun() const override { return "game"; }
  virtual WStringView GetRelaunchHint() const override;
  virtual WStringView GetBuildTimestamp() const override;
  virtual void AddHostInfo(WMcpJsonWriter& ref_writer) override;
  virtual void RequestQuit(bool bDiscardChanges) override;

private:
  void ExecuteScreenshot(const WVariantDictionary& arguments, WMcpToolResult& out_result);

  /// Starts the capture. Fills out_result and returns W_FAILURE if it could not be started.
  WResult BeginScreenshot(const WVariantDictionary& arguments, WMcpToolResult& out_result);

  /// Turns a finished capture into a file. Called once m_State is Ready.
  void FinishScreenshot(WMcpToolResult& out_result);

  /// Polls the pending capture, from inside the frame.
  ///
  /// Subscribed to the frame loop rather than done from the tool call, because a capture only completes
  /// once the frame that renders it is about to be presented - which is exactly where the engine polls
  /// its own screenshots (WGameApplication::Run_PresentImage). Polling from the tool call instead, at
  /// the point where MCP requests are pumped, is *before* that frame has rendered, and the capture is
  /// then perpetually pending.
  void ExecutionEventHandler(const WGameApplicationExecutionEvent& e);

  enum class CaptureState
  {
    Idle,
    Pending, ///< Started, waiting for the frame loop to hand back an image.
    Ready,   ///< m_CapturedImage holds it.
    Failed,  ///< Dropped by the renderer - see m_sCaptureError.
  };

  /// Capturing a back buffer is asynchronous by at least one frame, and the tool call runs *inside* a
  /// frame - so the image cannot be ready before that call returns. The request is deferred
  /// (WMcpToolResult::m_bNotFinished) and this is the state that survives across the re-entries. Only
  /// one request is ever in flight, so a single set of members is enough.
  CaptureState m_State = CaptureState::Idle;
  WImage m_CapturedImage;
  WString m_sCaptureError;
  WString m_sCapturePath;
  WUInt32 m_uiCaptureMaxWidth = 0;
  WInt32 m_iCaptureWindow = 0;
  /// Name of the window the capture was started on, so the result can say what was captured. Recorded
  /// up front because a window can go away while the capture is in flight.
  WString m_sCaptureWindowName;
  WTime m_CaptureStarted;
  WEventSubscriptionID m_FrameSubscription = 0;

  /// How long to keep polling before giving up. Generous for a frame or two of latency, short enough
  /// that the answer arrives while an agent is still waiting. The interesting case it bounds is the
  /// editor's engine process, which renders only when the editor asks it to and may never present a
  /// frame at all - without a limit that is indistinguishable from a hang.
  static constexpr WTime s_CaptureTimeout = WTime::MakeFromSeconds(10);
};
