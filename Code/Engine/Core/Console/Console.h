#pragma once

#include <Core/Console/CommandInterpreter.h>
#include <Core/Console/ConsoleFunction.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

/// The event data that is broadcast by the console
struct WConsoleEvent
{
  enum class Type : WInt32
  {
    OutputLineAdded, ///< A string was added to the console
  };

  Type m_Type;

  /// The console string that was just added.
  const WConsoleString* m_AddedpConsoleString;
};

/// Base console system for command input, output display, and history management.
///
/// Provides infrastructure for command execution through pluggable interpreters,
/// maintains input history, and broadcasts events when output is added.
/// Thread-safe through internal mutex protection.
///
/// This base class handles core functionality but doesn't store output strings.
/// Derived classes typically provide persistent storage and visual representation.
class W_CORE_DLL WConsole
{
public:
  WConsole();
  virtual ~WConsole();

  /// \name Events
  /// @{

public:
  /// Grants access to subscribe and unsubscribe from console events.
  const WEvent<const WConsoleEvent&>& Events() const { return m_Events; }

protected:
  /// The console event variable, to attach to.
  WEvent<const WConsoleEvent&> m_Events;

  /// @}

  /// \name Helpers
  /// @{

public:
  /// Returns the mutex that's used to prevent multi-threaded access
  WMutex& GetMutex() const { return m_Mutex; }

  static void SetMainConsole(WConsole* pConsole);
  static WConsole* GetMainConsole();

protected:
  mutable WMutex m_Mutex;

private:
  static WConsole* s_pMainConsole;

  /// @}

  /// \name Command Interpreter
  /// @{

public:
  /// Replaces the current command interpreter.
  ///
  /// This base class doesn't set any default interpreter, but derived classes may do so.
  void SetCommandInterpreter(const WSharedPtr<WCommandInterpreter>& pInterpreter) { m_pCommandInterpreter = pInterpreter; }

  /// Returns the currently used command interpreter.
  const WSharedPtr<WCommandInterpreter>& GetCommandInterpreter() const { return m_pCommandInterpreter; }

  /// Auto-completes the given text.
  ///
  /// Returns true, if the string was modified in any way.
  /// Adds additional strings to the console output, if there are further auto-completion suggestions.
  virtual bool AutoComplete(WStringBuilder& ref_sText);

  /// Executes the given input string.
  ///
  /// The command is forwarded to the set command interpreter.
  virtual void ExecuteCommand(WStringView sInput);

protected:
  WSharedPtr<WCommandInterpreter> m_pCommandInterpreter;

  /// @}

  /// \name Console Display
  /// @{

public:
  /// Adds a string to the console.
  ///
  /// The base class only broadcasts an event, but does not store the string anywhere.
  virtual void AddConsoleString(WStringView sText, WConsoleString::Type type = WConsoleString::Type::Default);

  /// Display the console state.
  virtual void RenderConsole(bool bIsOpen) { W_IGNORE_UNUSED(bIsOpen); }

  /// @}

  /// \name Input
  /// @{

public:
  /// Update the console with the latest input.
  virtual void HandleInput(bool bIsOpen) { W_IGNORE_UNUSED(bIsOpen); }

  /// Adds an item to the input history.
  void AddToInputHistory(WStringView sText);

  /// Returns the current input history.
  ///
  /// Make sure to lock the console's mutex while working with the history.
  const WStaticArray<WString, 16>& GetInputHistory() const { return m_InputHistory; }

  /// Replaces the input line by the next (or previous) history item.
  void RetrieveInputHistory(WInt32 iHistoryUp, WStringBuilder& ref_sResult);

  /// Writes the current input history to a text file.
  WResult SaveInputHistory(WStringView sFile);

  /// Reads the text file and appends all lines to the input history.
  void LoadInputHistory(WStringView sFile);

protected:
  WInt32 m_iCurrentInputHistoryElement = -1;
  WStaticArray<WString, 16> m_InputHistory;

  /// @}
};
