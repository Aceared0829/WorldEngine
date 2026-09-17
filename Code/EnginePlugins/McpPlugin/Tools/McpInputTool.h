#pragma once

#include <McpPlugin/McpPluginDLL.h>

#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Variant.h>
#include <Mcp/McpTool.h>

class WMcpInputDevice;

/// Pressing keys and moving the mouse, as the game sees it.
///
/// Owns the synthetic WInputDevice that does the actual writing - see WMcpInputDevice for why it is a
/// device rather than injected values. The provider's lifetime is the plugin's, so the device exists for
/// as long as there is a server to ask for input.
///
/// Deliberately shaped to grow: an agent driving a game will eventually want recorded sequences and
/// timed scripts, and those are new tool names on this provider writing to the same device, not a
/// different mechanism.
class WMcpInputTool : public WMcpToolProvider
{
  W_ADD_DYNAMIC_REFLECTION(WMcpInputTool, WMcpToolProvider);

public:
  WMcpInputTool();
  ~WMcpInputTool();

  virtual void OnActivate() override;
  virtual void OnDeactivate() override;

  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

private:
  void ExecuteSlots(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteSet(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteSequence(const WVariantDictionary& arguments, WMcpToolResult& out_result);

  /// Writes every slot in \a slots via m_pDevice, the same way ExecuteSet() does. Shared with
  /// ExecuteSequence()'s 'set' steps. Returns false (and has already set out_result) on the first
  /// value that is not a number.
  bool ApplySlots(const WVariantDictionary& slots, WUInt32 uiFrames, WMcpToolResult& out_result);

  WUniquePtr<WMcpInputDevice> m_pDevice;

  /// Same reason as everywhere else: a process registers hundreds of slots and an unfiltered dump of
  /// them buries whatever the caller was looking for.
  static constexpr WUInt32 s_uiMaxResults = 200;

  /// input_sequence cannot loop across a 'wait' step - like game_wait, it defers instead
  /// (WMcpToolResult::m_bNotFinished) and is re-entered once per frame until the wait is over. This is
  /// what survives across those re-entries: which step is in flight, and until when.
  bool m_bSequenceActive = false;
  bool m_bSequenceWaiting = false;
  WUInt32 m_uiSequenceStep = 0;
  WUInt64 m_uiSequenceWaitUntilFrame = 0;
  WTime m_SequenceWaitStarted;
  WTime m_SequenceWaitTimeout;

  /// A caller that asks for a sequence of a million steps has made a mistake that would otherwise look
  /// exactly like a hang - same reasoning as game_wait's s_uiMaxFrames.
  static constexpr WUInt32 s_uiMaxSteps = 200;
  static constexpr WUInt32 s_uiMaxWaitFrames = 10000;
  static constexpr WTime s_DefaultWaitTimeout = WTime::MakeFromSeconds(30);
};
