#pragma once

#include <Core/Console/Console.h>
#include <GameEngine/GameEngineDLL.h>

struct WLoggingEventData;

/// A Quake-style console for in-game configuration of WCVar and WConsoleFunction.
///
/// The console displays the recent log activity and allows to modify cvars and call console functions.
/// It supports auto-completion of known keywords.
/// Additionally, 'keys' can be bound to arbitrary commands, such that useful commands can be executed
/// easily.
/// The default implementation uses WConsoleInterpreter::Lua as the interpreter for commands typed into it.
/// The interpreter can be replaced with custom implementations.
class W_GAMEENGINE_DLL WQuakeConsole final : public WConsole
{
public:
  WQuakeConsole();
  virtual ~WQuakeConsole();



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

  /// \name Command Processing
  /// @{



  /// Executes the given command using the current command interpreter.
  virtual void ExecuteCommand(WStringView sInput) override;

  /// Binds \a szCommand to \a szKey. Calling ExecuteBoundKey() with this key will then run that command.
  ///
  /// A key can be any arbitrary string. However, it might make sense to either use the standard ASCII characters A-Z and a-z, which allows
  /// to trigger actions by the press of any of those buttons.
  /// You can, however, also use names for input buttons, such as 'Key_Left', but then you also need to call ExecuteBoundKey() with those
  /// names.
  /// If you use such virtual key names, it makes also sense to listen to the auto-complete event and suggest those key names there.
  void BindKey(WStringView sKey, WStringView sCommand);

  /// Removes the key binding.
  void UnbindKey(WStringView sKey);

  /// Executes the command that was bound to this key.
  void ExecuteBoundKey(WStringView sKey);

  /// @}

  /// \name Input Handling
  /// @{

  /// Inserts one character at the caret position into the console input line.
  ///
  /// This function also calls ProcessInputCharacter and FilterInputCharacter. By default this already reacts on Tab, Enter and ESC
  /// and filters out all non ASCII characters.
  void AddInputCharacter(WUInt32 uiChar);

  /// Clears the input line of the console.
  void ClearInputLine();

  /// Returns the current content of the input line.
  WStringView GetInputLine() const { return m_sInputLine; }

  /// Returns the position (in characters) of the caret.
  WInt32 GetCaretPosition() const { return m_iCaretPosition; }

  /// Moves the caret in the text. Its position will be clamped to the length of the current input line text.
  void MoveCaret(WInt32 iMoveOffset);

  /// Deletes the character following the caret position.
  void DeleteNextCharacter();

  /// Scrolls the contents of the console up or down. Will be clamped to the available range.
  void Scroll(WInt32 iLines);

  /// Returns the current scroll position. This must be used during rendering to start with the proper line.
  WUInt32 GetScrollPosition() const { return m_iScrollPosition; }

  /// This function implements input handling (via WInputManager) for the console.
  ///
  /// If the console is 'open' (ie. has full focus), it will handle more input for caret movement etc.
  /// However, in the 'closed' state, it will still execute bound keys and commands from the history.
  /// It is not required to call this function, you can implement input handling entirely outside the console.
  ///
  /// If this function is used, it should be called once per frame and if the console is considered 'open',
  /// no further keyboard input should be processed, as that might lead to confusing behavior when the user types
  /// text into the console.
  ///
  /// The state whether the console is considered open has to be managed by the application.
  virtual void DoDefaultInputHandling(bool bConsoleOpen);

  /// @}

  /// \name Console Content
  /// @{

  /// Adds a string to the console.
  virtual void AddConsoleString(WStringView sText, WConsoleString::Type type = WConsoleString::Type::Default) override;

  /// Returns all current console strings. Use GetScrollPosition() to know which one should be displayed as the first one.
  const WDeque<WConsoleString>& GetConsoleStrings() const;

  /// Deletes all console strings, making the console empty.
  void ClearConsoleStrings();

  /// Display the console state.
  virtual void RenderConsole(bool bIsOpen) override;

  /// Update the console with the latest input.
  virtual void HandleInput(bool bIsOpen) override;

  /// @}

protected:
  /// Deletes the character at the given position in the input line.
  void RemoveCharacter(WUInt32 uiInputLinePosition);

  /// Makes sure the caret position is clamped to the input line length.
  void ClampCaretPosition();


  /// The function that is used to read WGlobalLog messages.
  void LogHandler(const WLoggingEventData& data);

  WInt32 m_iCaretPosition;
  WStringBuilder m_sInputLine;

  virtual bool ProcessInputCharacter(WUInt32 uiChar);
  virtual bool FilterInputCharacter(WUInt32 uiChar);
  virtual void InputStringChanged();

  WDeque<WConsoleString> m_ConsoleStrings;
  bool m_bUseFilteredStrings = false;
  WDeque<WConsoleString> m_FilteredConsoleStrings;
  WUInt32 m_uiMaxConsoleStrings;
  WInt32 m_iScrollPosition;
  bool m_bLogOutputEnabled;
  bool m_bDefaultInputHandlingInitialized;


  WMap<WString, WString> m_BoundKeys;
};
