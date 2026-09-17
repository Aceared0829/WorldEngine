#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/RefCounted.h>

struct W_CORE_DLL WConsoleString
{
  enum class Type : WUInt8
  {
    Default,
    Error,
    SeriousWarning,
    Warning,
    Note,
    Success,
    Executed,
    VarName,
    FuncName,
    Dev,
    Debug,
  };

  Type m_Type = Type::Default;
  WString m_sText;
  WColor GetColor() const;

  bool operator<(const WConsoleString& rhs) const { return m_sText < rhs.m_sText; }
};

struct W_CORE_DLL WCommandInterpreterState
{
  WStringBuilder m_sInput;
  WHybridArray<WConsoleString, 16> m_sOutput;

  void AddOutputLine(const WFormatString& text, WConsoleString::Type type = WConsoleString::Type::Default);
};

class W_CORE_DLL WCommandInterpreter : public WRefCounted
{
public:
  virtual void Interpret(WCommandInterpreterState& inout_state) = 0;

  virtual void AutoComplete(WCommandInterpreterState& inout_state);

  /// Iterates over all cvars and finds all that start with the string \a szVariable.
  static void FindPossibleCVars(WStringView sVariable, WDeque<WString>& ref_commonStrings, WDeque<WConsoleString>& ref_consoleStrings);

  /// Iterates over all console functions and finds all that start with the string \a szVariable.
  static void FindPossibleFunctions(WStringView sVariable, WDeque<WString>& ref_commonStrings, WDeque<WConsoleString>& ref_consoleStrings);

  /// Returns the prefix string that is common to all strings in the \a vStrings array.
  static const WString FindCommonString(const WDeque<WString>& strings);

  /// \name Helpers
  /// @{

  /// Returns a nice string containing all the important information about the cvar.
  static WString GetFullInfoAsString(WCVar* pCVar);

  /// Returns the value of the cvar as a string.
  static const WString GetValueAsString(WCVar* pCVar);

  /// @}
};
