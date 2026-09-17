#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/// Stores the valid values and names for 'dynamic' bitflags.
///
/// The names and valid values for dynamic bitflags may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicBitflags() to create or get the WDynamicBitflags for a specific type.
class W_GUIFOUNDATION_DLL WDynamicBitflags
{
public:
  /// Returns a WDynamicBitflags under the given name. Creates a new one, if the name has not been used before.
  static WDynamicBitflags& GetDynamicBitflags(WStringView sName);

  /// Returns all bitflag values and current names.
  const WMap<WUInt64, WString>& GetAllValidValues() const { return m_ValidValues; }

  /// Resets stored values.
  void Clear();

  /// Sets the name for the given bit position.
  void SetValueAndName(WUInt32 uiBitPos, WStringView sName);

  /// Removes a value, if it exists.
  void RemoveValue(WUInt32 uiBitPos);

  /// Returns whether a certain value is known.
  bool IsValueValid(WUInt32 uiBitPos) const;

  /// Returns the name for the given value
  bool TryGetValueName(WUInt32 uiBitPos, WStringView& out_sName) const;

private:
  WMap<WUInt64, WString> m_ValidValues;

  static WMap<WString, WDynamicBitflags> s_DynamicBitflags;
};
