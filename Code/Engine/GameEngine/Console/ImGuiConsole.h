#pragma once

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/Console/Console.h>
#  include <Core/Input/Declarations.h>
#  include <Foundation/Containers/IdTable.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Types/Delegate.h>
#  include <GameEngine/GameEngineDLL.h>

struct WLoggingEventData;

/// Callback signature for registered ImGui windows. The bool ref controls the window's open state.
using WImGuiWindowCallback = WDelegate<void(bool&)>;
using WImGuiRegisteredWndHandleData = WGenericId<16, 16>;
class WImGuiRegisteredWndHandle
{
  W_DECLARE_HANDLE_TYPE(WImGuiRegisteredWndHandle, WImGuiRegisteredWndHandleData);
};


/// An ImGui-based console for in-game display of logs, configuration of WCVars, and more.
///
/// The console displays the recent log activity, allows to modify CVars and call console functions.
/// It supports auto-completion and provides a GUI using ImGui.
/// The default implementation uses WConsoleInterpreter::Lua as the interpreter for commands typed into it.
/// The interpreter can be replaced with custom implementations.
class W_GAMEENGINE_DLL WImGuiConsole final : public WConsole
{
public:
  WImGuiConsole();
  ~WImGuiConsole();

  /// \name Configuration
  /// @{

  /// Adjusts how many strings the console will keep in memory at maximum.
  void SetMaxConsoleStrings(WUInt32 uiMax) { m_uiMaxConsoleStrings = WMath::Clamp<WUInt32>(uiMax, 0, 100000); }

  /// Returns how many strings the console will keep in memory at maximum.
  WUInt32 GetMaxConsoleStrings() const { return m_uiMaxConsoleStrings; }

  /// Enables or disables that the output from WGlobalLog is displayed in the console. Enabled by default.
  void EnableLogOutput(bool bEnable);

  /// Writes the state of the console (history, bound keys) to the stream.
  virtual void SaveState(WStreamWriter& inout_stream) const;

  /// Reads the state of the console (history, bound keys) from the stream.
  virtual void LoadState(WStreamReader& inout_stream);

  /// @}
  /// \name Console Content
  /// @{

  /// Adds a string to the console.
  virtual void AddConsoleString(WStringView sText, WConsoleString::Type type = WConsoleString::Type::Default) override;

  /// @}
  /// \name Updates
  /// @{

  /// Display the console state.
  virtual void RenderConsole(bool bIsOpen) override;

  /// Update the console with the latest input.
  virtual void HandleInput(bool bIsOpen) override;

  /// @}
  /// \name Custom Windows
  /// @{

  /// Register an additional ImGui window that will be rendered when the console is open.
  /// The window appears in the Commands menu bar under "Windows".
  static WImGuiRegisteredWndHandle RegisterWindow(WStringView sName, WImGuiWindowCallback callback);

  /// Unregister a previously registered window by name.
  static void UnregisterWindow(WImGuiRegisteredWndHandle hWindow);

  /// @}

protected:
  struct CVarTreeNode
  {
    WString m_sName;
    WCVar* m_pCVar = nullptr; // nullptr for parent nodes, valid for leaf nodes
    WMap<WString, CVarTreeNode> m_Children;
    bool m_bExpanded = false;
  };

  struct CustomConsoleWindow
  {
    WString m_sName;
    WImGuiWindowCallback m_Callback;
    bool m_bOpen = false;
  };

  void LogHandler(const WLoggingEventData& data);
  void ClearLogStrings();
  void RenderMenuBar();
  void RenderCommandWindow(bool bSetFocus);
  void RenderCVarWindow();
  void RenderStatsWindow(bool bFull);
  void RenderLogWindow(bool bFull);
  void HandleAutoComplete();
  int InputTextCallback(struct ImGuiInputTextCallbackData* data);
  void BuildCVarTree(CVarTreeNode& root);
  void RenderCVarTreeNode(const WString& sNodeName, CVarTreeNode& node);
  void RenderCVarValue(WCVar* pCVar);
  bool CVarNamePassesFilter(WStringView sCVarName) const;
  bool CVarTreeNodeHasMatchingDescendant(const CVarTreeNode& node) const;
  void BuildFilteredLogStrings();
  bool FilterLogString(const WConsoleString& entry) const;
  WUInt64 CalculateTotalMemoryUsage();
  void UpdateFrameTimes();
  void UpdateMemoryUsage();

  static WIdTable<WImGuiRegisteredWndHandleData, WUniquePtr<CustomConsoleWindow>> s_CustomWindows;

  WDeque<WConsoleString> m_LogStrings;           // For log messages (used by log window)
  WDeque<WConsoleString> m_CommandOutputStrings; // For command input/output (used by console window)
  WDeque<WConsoleString> m_FilteredLogStrings;
  WUInt32 m_uiMaxConsoleStrings = 1000;
  WUInt32 m_uiMaxCommandOutputStrings = 1000;
  bool m_bLogOutputEnabled = false;
  bool m_bScrollLogToBottom = false;
  bool m_bScrollCommandsToBottom = false; // For console window auto-scroll

  bool m_bPinStatsWindow = false;
  bool m_bPinLogWindow = false;
  bool m_bStatsWindowOpen = true;
  bool m_bLogWindowOpen = true;
  bool m_bCVarWindowOpen = true;

  // Log filtering
  WStringBuilder m_sLogFilter;
  WStringBuilder m_sCommandText;
  bool m_bFilterLog = false;
  bool m_bLogFilterChanged = false;
  WLogMsgType::Enum m_LogLevel = WLogMsgType::DebugMsg;

  // CVar filtering
  WStringBuilder m_sCVarFilter;

  // Input handling
  WUInt8 m_uiForceFocus = 3;
  bool m_bWasOpen = false;
  bool m_bDefaultInputHandlingInitialized = false;
  bool m_bExecutingCommand = false;
  WUInt8 m_uiResetLayout = 0;

  // While the console is open, the OS cursor has to be visible, no matter what the game wants,
  // and a custom ('software') cursor must not be used, because it may not indicate where clicks go.
  WMouseCursorOverrideRequest m_CursorOverride;

  // Frame time tracking for statistics
  float m_FrameTimeHistory[30 * 10];
  WUInt32 m_uiFrameTimeHistoryIndex = 0;
  float m_fCurrentBinMaxFrameTime = 0.0f;
  WTime m_LastBinTime;
  bool m_bTimeTrackingVisible = true;

  // Memory tracking for statistics
  float m_MemoryHistory[30 * 10]; // Memory usage in MB
  WUInt32 m_uiMemoryHistoryIndex = 0;
  float m_fCurrentBinMaxMemory = 0.0f;
  WTime m_LastMemoryBinTime;
  bool m_bMemoryTrackingVisible = true;

  // Stats window size tracking for overlay/full mode switching
  WVec2 m_vStatsWindowSavedSize = WVec2(0, 0);
  bool m_bStatsWasInFullMode = true;
};

#endif // BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
