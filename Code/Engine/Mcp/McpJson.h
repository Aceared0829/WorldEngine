#pragma once

#include <Mcp/McpDLL.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Variant.h>

/// Reads values out of the arguments of a tool call.
///
/// Every accessor takes a fallback and never fails, because the caller is a language model: arguments
/// go missing, arrive as the wrong type, or are spelled differently than the schema asked for. A tool
/// that wants to reject bad input has to compare against the fallback and say so in its result.
///
/// For producing JSON see WMcpJsonWriter.
struct W_MCP_DLL WMcpJson
{
  /// Returns the value of a string member, or sFallback if it is missing or not a string.
  static WStringView GetString(const WVariantDictionary& dict, WStringView sKey, WStringView sFallback = {});

  /// Returns the value of a number member, or iFallback if it is missing or not a number.
  ///
  /// The JSON parser turns every number into a double, and AI clients happily send "10" as a string,
  /// so this accepts anything convertible.
  static WInt64 GetInt(const WVariantDictionary& dict, WStringView sKey, WInt64 iFallback);

  /// Returns the value of a bool member, or bFallback if it is missing or not a bool.
  ///
  /// Also accepts the strings "true"/"false" and numbers, because clients send all three.
  static bool GetBool(const WVariantDictionary& dict, WStringView sKey, bool bFallback);

  /// Returns a nested object member, or nullptr.
  static const WVariantDictionary* GetDict(const WVariantDictionary& dict, WStringView sKey);

  /// Returns a member that is a raw JSON array, or nullptr if it is missing or not an array.
  ///
  /// Unlike GetStringArray() this does not convert or filter elements - use it when the array holds
  /// objects (e.g. a list of steps), and inspect each WVariant yourself.
  static const WVariantArray* GetArray(const WVariantDictionary& dict, WStringView sKey);

  /// Collects a member that is a list of strings into out_values.
  ///
  /// Accepts a JSON array, and also a single string as a list of one - a client asked for one element
  /// tends to send it bare. Non-string array elements are converted where they can be and skipped
  /// otherwise, so one malformed entry does not discard the rest. Returns false when the member is
  /// missing entirely, which is what distinguishes 'not given' from 'given as empty'.
  static bool GetStringArray(const WVariantDictionary& dict, WStringView sKey, WDynamicArray<WString>& out_values);
};
