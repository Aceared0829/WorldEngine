#pragma once

#ifndef W_INCLUDING_BASICS_H
#  error "Please don't include FormatString.h directly, but instead include Foundation/Basics.h"
#endif

class WStringBuilder;

#include <Foundation/Strings/StringView.h>

#include <Foundation/Strings/Implementation/FormatStringArgs.h>

/// Implements formating of strings with placeholders and formatting options.
///
/// WFormatString can be used anywhere where a string should be formatable when passing it into a function.
/// Good examples are WStringBuilder::SetFormat() or WLog::Info().
///
/// A function taking an WFormatString can internally call WFormatString::GetText() to retrieve he formatted result.
/// When calling such a function, one must wrap the parameter into 'WFmt' to enable formatting options, example:
///   void MyFunc(const WFormatString& text);
///   MyFunc(WFmt("Cool Story {}", "Bro"));
///
/// To provide more convenience, one can add a template-function overload like this:
///   template <typename... ARGS>
///   void MyFunc(const char* szFormat, ARGS&&... args)
///   {
///     MyFunc(WFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
///   }
///
/// This allows to call MyFunc() without the 'WFmt' wrapper.
///
///
/// === Formatting ===
///
/// Placeholders for variables are specified using '{}'. These may use numbers from 0 to 9,
/// ie. {0}, {3}, {2}, etc. which allows to change the order or insert duplicates.
/// If no number is provided, each {} instance represents the next argument.
///
/// To specify special formatting, wrap the argument into an WArgXY call:
///   WArgC - for characters
///   WArgI - for integer formatting
///   WArgU - for unsigned integer formatting (e.g. HEX)
///   WArgF - for floating point formatting
///   WArgP - for pointer formatting
///   WArgDateTime - for WDateTime formatting options
///   WArgErrorCode - for Windows error code formatting
///   WArgHumanReadable - for shortening numbers with common abbreviations
///   WArgFileSize - for representing file sizes
///
/// Example:
///   WStringBuilder::SetFormat("HEX: {}", WArgU(1337, 8 /*width*/, true /*pad with zeros*/, 16 /*base16*/, true/*upper case*/));
///
/// Arbitrary other types can support special formatting even without an WArgXY call. E.g. WTime and WAngle do special formatting.
/// WArgXY calls are only necessary if formatting options are needed for a specific formatting should be enforced (e.g. WArgErrorCode
/// would otherwise just use uint32 formatting).
///
/// To implement custom formatting see the various free standing 'BuildString' functions.
class W_FOUNDATION_DLL WFormatString
{
  W_DISALLOW_COPY_AND_ASSIGN(WFormatString); // pass by reference, never pass by value

public:
  /// Maximum number of parameters that a single format string can have. Exceeding this results in a compilation error.
  static constexpr WUInt32 MaxNumParameters = 12;

  W_ALWAYS_INLINE WFormatString() = default;
  W_ALWAYS_INLINE WFormatString(const char* szString) { m_sString = szString; }
  W_ALWAYS_INLINE WFormatString(WStringView sString) { m_sString = sString; }
  WFormatString(const WStringBuilder& s);
  virtual ~WFormatString() = default;

  /// Generates the formatted text. Make sure to only call this function once and only when the formatted string is really needed.
  ///
  /// Requires an WStringBuilder as storage, ie. POTENTIALLY writes the formatted text into it.
  /// However, if no formatting is required, it may not touch the string builder at all and just return a string directly.
  ///
  /// \note Do not assume that the result is stored in \a sb. Always only use the return value. The string builder is only used
  /// when necessary.
  [[nodiscard]] virtual WStringView GetText(WStringBuilder&) const { return m_sString; }

  /// Similar to GetText() but guaranteed to copy the string into the given string builder,
  /// and thus guaranteeing that the generated string is zero terminated.
  virtual const char* GetTextCStr(WStringBuilder& out_sString) const;

  bool IsEmpty() const { return m_sString.IsEmpty(); }

  /// Helper function to build the formatted text with the given arguments.
  ///
  /// \note We can't use WArrayPtr here because of include order.
  WStringView BuildFormattedText(WStringBuilder& ref_sStorage, WStringView* pArgs, WUInt32 uiNumArgs) const;

protected:
  WStringView m_sString;
};

#include <Foundation/Strings/Implementation/FormatStringImpl.h>

template <typename... ARGS>
W_ALWAYS_INLINE WFormatStringImpl<ARGS...> WFmt(const char* szFormat, ARGS&&... args)
{
  return WFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...);
}
