#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/// Stores the valid values and names for 'dynamic' enums.
///
/// The names and valid values for dynamic enums may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicEnum() to create or get the WDynamicEnum for a specific type.
class W_GUIFOUNDATION_DLL WDynamicEnum
{
public:
  /// Returns a WDynamicEnum under the given name. Creates a new one, if the name has not been used before.
  static WDynamicEnum& GetDynamicEnum(const char* szEnumName);

  /// Returns all enum values and current names.
  const WMap<WInt32, WString>& GetAllValidValues() const { return m_ValidValues; }

  /// Resets the internal data.
  void Clear();

  /// Sets the name for the given enum value.
  void SetValueAndName(WInt32 iValue, WStringView sNewName);

  /// Removes a certain enum value, if it exists.
  void RemoveValue(WInt32 iValue);

  /// Returns whether a certain value is known.
  bool IsValueValid(WInt32 iValue) const;

  /// Returns the name for the given value. Returns "<invalid value>" if the value is not in use.
  WStringView GetValueName(WInt32 iValue) const;

  /// If specified, the widget shows an "edit" option, which will run WActionManager::ExecuteAction(sCmd, value)
  ///
  /// This is meant to be used to open existing config dialogs.
  /// There is currently no way to report back a selection, so after making changes, the user has to make another selection.
  void SetEditCommand(WStringView sCmd, const WVariant& value);
  WStringView GetEditCommand() const { return m_sEditCommand; }
  const WVariant& GetEditCommandValue() const { return m_EditCommandValue; }


private:
  WMap<WInt32, WString> m_ValidValues;
  WString m_sEditCommand;
  WVariant m_EditCommandValue;

  static WMap<WString, WDynamicEnum> s_DynamicEnums;
};
