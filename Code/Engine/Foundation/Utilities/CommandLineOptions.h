#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/EnumerableClass.h>

class WStringBuilder;
class WLogInterface;

/// WCommandLineOption (and derived types) are used to define options that the application supports.
///
/// Command line options are created as global variables anywhere throughout the code, wherever they are needed.
/// The point of using them over going through WCommandLineUtils directly, is that the options can be listed automatically
/// and thus an application can print all available options, when the user requests help.
///
/// Consequently, their main purpose is to make options discoverable and to document them in a consistent manner.
///
/// Additionally, classes like WCommandLineOptionEnum add functionality that makes some options easier to setup.
class W_FOUNDATION_DLL WCommandLineOption : public WEnumerable<WCommandLineOption>
{
  W_DECLARE_ENUMERABLE_CLASS(WCommandLineOption);

public:
  enum class LogAvailableModes
  {
    Always,         ///< Logs the available modes no matter what
    IfHelpRequested ///< Only logs the modes, if '-h', '-help', '-?' or something similar was specified
  };

  /// Describes whether the value of an option (and whether something went wrong), should be printed to WLog.
  enum class LogMode
  {
    Never,                ///< Don't log anything.
    FirstTime,            ///< Only print the information the first time a value is accessed.
    FirstTimeIfSpecified, ///< Only on first access and only if the user specified the value on the command line.
    Always,               ///< Always log the options value on access.
    AlwaysIfSpecified,    ///< Always log values, if the user specified non-default ones.
  };

  /// Checks whether a command line was passed that requests help output.
  static bool IsHelpRequested(const WCommandLineUtils* pUtils = WCommandLineUtils::GetGlobalInstance()); // [tested]

  /// Checks whether all required options are passed to the command line.
  ///
  /// The options are passed as a semicolon-separated list (spare spaces are stripped away), for instance "-opt1; -opt2"
  static WResult RequireOptions(WStringView sRequiredOptions, WString* pMissingOption = nullptr, const WCommandLineUtils* pUtils = WCommandLineUtils::GetGlobalInstance()); // [tested]

  /// Prints all available options to the WLog.
  ///
  /// \param szGroupFilter
  ///   If this is empty, all options from all 'sorting groups' are logged.
  ///   If non-empty, only options from sorting groups that appear in this string will be logged.
  static bool LogAvailableOptions(LogAvailableModes mode, WStringView sGroupFilter = {}, const WCommandLineUtils* pUtils = WCommandLineUtils::GetGlobalInstance()); // [tested]

  /// Same as LogAvailableOptions() but captures the output from WLog and returns it in an WStringBuilder.
  static bool LogAvailableOptionsToBuffer(WStringBuilder& out_sBuffer, LogAvailableModes mode, WStringView sGroupFilter = {}, const WCommandLineUtils* pUtils = WCommandLineUtils::GetGlobalInstance()); // [tested]

public:
  /// \param szSortingGroup
  ///   This string is used to sort options. Application options should start with an underscore, such that they appear first
  ///   in the output.
  WCommandLineOption(WStringView sSortingGroup) { m_sSortingGroup = sSortingGroup; }

  /// Writes the sorting group name to 'out'.
  virtual void GetSortingGroup(WStringBuilder& ref_sOut) const;

  /// Writes all the supported options (e.g. '-arg') to 'out'.
  /// If more than one option is allowed, they should be separated with semicolons or pipes.
  virtual void GetOptions(WStringBuilder& ref_sOut) const = 0;

  /// Returns the supported option names (e.g. '-arg') as split strings.
  void GetSplitOptions(WStringBuilder& out_sAll, WDynamicArray<WStringView>& ref_splitOptions) const;

  /// Returns a very short description of the option (type). For example "<int>" or "<enum>".
  virtual void GetParamShortDesc(WStringBuilder& ref_sOut) const = 0;

  /// Returns a very short string for the options default value. For example "0" or "auto".
  virtual void GetParamDefaultValueDesc(WStringBuilder& ref_sOut) const = 0;

  /// Returns a proper description of the option.
  ///
  /// The long description is allowed to contain newlines (\n) and the output will be formatted accordingly.
  virtual void GetLongDesc(WStringBuilder& ref_sOut) const = 0;

  /// Returns a string indicating the exact implementation type.
  virtual WStringView GetType() = 0;

protected:
  WStringView m_sSortingGroup;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// WCommandLineOptionDoc can be used to document a command line option whose logic might be more complex than what the other option types provide.
///
/// This class is meant to be used for options that are actually queried directly through WCommandLineUtils,
/// but should still show up in the command line option documentation, such that the user can discover them.
///
class W_FOUNDATION_DLL WCommandLineOptionDoc : public WCommandLineOption
{
public:
  WCommandLineOptionDoc(WStringView sSortingGroup, WStringView sArgument, WStringView sParamShortDesc, WStringView sLongDesc, WStringView sDefaultValue, bool bCaseSensitive = false);

  virtual void GetOptions(WStringBuilder& ref_sOut) const override;               // [tested]

  virtual void GetParamShortDesc(WStringBuilder& ref_sOut) const override;        // [tested]

  virtual void GetParamDefaultValueDesc(WStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetLongDesc(WStringBuilder& ref_sOut) const override;              // [tested]

  /// Returns "Doc"
  virtual WStringView GetType() override { return "Doc"; }

  /// Checks whether any of the option variants is set on the command line, and returns which one. For example '-h' or '-help'.
  bool IsOptionSpecified(WStringBuilder* out_pWhich = nullptr, const WCommandLineUtils* pUtils = WCommandLineUtils::GetGlobalInstance()) const; // [tested]

protected:
  bool ShouldLog(LogMode mode, bool bWasSpecified) const;
  void LogOption(WStringView sOption, WStringView sValue, bool bWasSpecified) const;

  WStringView m_sArgument;
  WStringView m_sParamShortDesc;
  WStringView m_sParamDefaultValue;
  WStringView m_sLongDesc;
  bool m_bCaseSensitive = false;
  mutable bool m_bLoggedOnce = false;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// This command line option exposes simple on/off switches.
class W_FOUNDATION_DLL WCommandLineOptionBool : public WCommandLineOptionDoc
{
public:
  WCommandLineOptionBool(WStringView sSortingGroup, WStringView sArgument, WStringView sLongDesc, bool bDefaultValue, bool bCaseSensitive = false);

  /// Returns the value of this option. Either what was specified on the command line, or the default value.
  bool GetOptionValue(LogMode logMode, const WCommandLineUtils* pUtils = WCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// Modifies the default value
  void SetDefaultValue(bool value)
  {
    m_bDefaultValue = value;
  }

  /// Returns the default value.
  bool GetDefaultValue() const { return m_bDefaultValue; }

  /// Returns "Bool"
  virtual WStringView GetType() override { return "Bool"; }

protected:
  bool m_bDefaultValue = false;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// This command line option exposes integer values, optionally with a min/max range.
///
/// If the user specified a value outside the allowed range, a warning is printed, and the default value is used instead.
/// It is valid for the default value to be outside the min/max range, which can be used to detect whether the user provided any value at all.
class W_FOUNDATION_DLL WCommandLineOptionInt : public WCommandLineOptionDoc
{
public:
  WCommandLineOptionInt(WStringView sSortingGroup, WStringView sArgument, WStringView sLongDesc, int iDefaultValue, int iMinValue = WMath::MinValue<int>(), int iMaxValue = WMath::MaxValue<int>(), bool bCaseSensitive = false);

  virtual void GetParamDefaultValueDesc(WStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetParamShortDesc(WStringBuilder& ref_sOut) const override;        // [tested]

  /// Returns the value of this option. Either what was specified on the command line, or the default value.
  int GetOptionValue(LogMode logMode, const WCommandLineUtils* pUtils = WCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// Modifies the default value
  void SetDefaultValue(WInt32 value)
  {
    m_iDefaultValue = value;
  }

  /// Returns "Int"
  virtual WStringView GetType() override { return "Int"; }

  /// Returns the minimum value.
  WInt32 GetMinValue() const { return m_iMinValue; }

  /// Returns the maximum value.
  WInt32 GetMaxValue() const { return m_iMaxValue; }

  /// Returns the default value.
  WInt32 GetDefaultValue() const { return m_iDefaultValue; }

protected:
  WInt32 m_iDefaultValue = 0;
  WInt32 m_iMinValue = 0;
  WInt32 m_iMaxValue = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// This command line option exposes float values, optionally with a min/max range.
///
/// If the user specified a value outside the allowed range, a warning is printed, and the default value is used instead.
/// It is valid for the default value to be outside the min/max range, which can be used to detect whether the user provided any value at all.
class W_FOUNDATION_DLL WCommandLineOptionFloat : public WCommandLineOptionDoc
{
public:
  WCommandLineOptionFloat(WStringView sSortingGroup, WStringView sArgument, WStringView sLongDesc, float fDefaultValue, float fMinValue = WMath::MinValue<float>(), float fMaxValue = WMath::MaxValue<float>(), bool bCaseSensitive = false);

  virtual void GetParamDefaultValueDesc(WStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetParamShortDesc(WStringBuilder& ref_sOut) const override;        // [tested]

  /// Returns the value of this option. Either what was specified on the command line, or the default value.
  float GetOptionValue(LogMode logMode, const WCommandLineUtils* pUtils = WCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// Modifies the default value
  void SetDefaultValue(float value)
  {
    m_fDefaultValue = value;
  }

  /// Returns "Float"
  virtual WStringView GetType() override { return "Float"; }

  /// Returns the minimum value.
  float GetMinValue() const { return m_fMinValue; }

  /// Returns the maximum value.
  float GetMaxValue() const { return m_fMaxValue; }

  /// Returns the default value.
  float GetDefaultValue() const { return m_fDefaultValue; }

protected:
  float m_fDefaultValue = 0;
  float m_fMinValue = 0;
  float m_fMaxValue = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// This command line option exposes simple string values.
class W_FOUNDATION_DLL WCommandLineOptionString : public WCommandLineOptionDoc
{
public:
  WCommandLineOptionString(WStringView sSortingGroup, WStringView sArgument, WStringView sLongDesc, WStringView sDefaultValue, bool bCaseSensitive = false);

  /// Returns the value of this option. Either what was specified on the command line, or the default value.
  WStringView GetOptionValue(LogMode logMode, const WCommandLineUtils* pUtils = WCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// Modifies the default value
  void SetDefaultValue(WStringView sValue)
  {
    m_sDefaultValue = sValue;
  }

  /// Returns the default value.
  WStringView GetDefaultValue() const { return m_sDefaultValue; }

  /// Returns "String"
  virtual WStringView GetType() override { return "String"; }

protected:
  WStringView m_sDefaultValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// This command line option exposes absolute paths. If the user provides a relative path, it will be concatenated with the current working directory.
class W_FOUNDATION_DLL WCommandLineOptionPath : public WCommandLineOptionDoc
{
public:
  WCommandLineOptionPath(WStringView sSortingGroup, WStringView sArgument, WStringView sLongDesc, WStringView sDefaultValue, bool bCaseSensitive = false);

  /// Returns the value of this option. Either what was specified on the command line, or the default value.
  WString GetOptionValue(LogMode logMode, const WCommandLineUtils* pUtils = WCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// Modifies the default value
  void SetDefaultValue(WStringView sValue)
  {
    m_sDefaultValue = sValue;
  }

  /// Returns the default value.
  WStringView GetDefaultValue() const { return m_sDefaultValue; }

  /// Returns "Path"
  virtual WStringView GetType() override { return "Path"; }

protected:
  WStringView m_sDefaultValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// An 'enum' option is a string option that only allows certain phrases ('keys').
///
/// Each phrase has an integer value, and GetOptionValue() returns the integer value of the selected phrase.
/// It is valid for the default value to be different from all the phrase values,
/// which can be used to detect whether the user provided any phrase at all.
///
/// The allowed values are passed in as a single string, in the form "OptA = 0 | OptB = 1 | ..."
/// Phrase values ("= 0" etc) are optional, and if not given are automatically assigned starting at zero.
/// Multiple phrases may share the same value.
class W_FOUNDATION_DLL WCommandLineOptionEnum : public WCommandLineOptionDoc
{
public:
  WCommandLineOptionEnum(WStringView sSortingGroup, WStringView sArgument, WStringView sLongDesc, WStringView sEnumKeysAndValues, WInt32 iDefaultValue, bool bCaseSensitive = false);

  /// Returns the value of this option. Either what was specified on the command line, or the default value.
  WInt32 GetOptionValue(LogMode logMode, const WCommandLineUtils* pUtils = WCommandLineUtils::GetGlobalInstance()) const; // [tested]

  virtual void GetParamShortDesc(WStringBuilder& ref_sOut) const override;                                                  // [tested]

  virtual void GetParamDefaultValueDesc(WStringBuilder& ref_sOut) const override;                                           // [tested]

  struct EnumKeyValue
  {
    WStringView m_Key;
    WInt32 m_iValue = 0;
  };

  /// Returns the enum keys (names) and values (integers) extracted from the string that was passed to the constructor.
  void GetEnumKeysAndValues(WDynamicArray<EnumKeyValue>& out_keysAndValues) const;

  /// Modifies the default value
  void SetDefaultValue(WInt32 value)
  {
    m_iDefaultValue = value;
  }

  /// Returns the default value.
  WInt32 GetDefaultValue() const { return m_iDefaultValue; }

  /// Returns "Enum"
  virtual WStringView GetType() override { return "Enum"; }

protected:
  WInt32 m_iDefaultValue = 0;
  WStringView m_sEnumKeysAndValues;
};
