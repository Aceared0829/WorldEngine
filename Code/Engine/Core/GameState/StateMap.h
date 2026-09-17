#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

/// A simple registry that stores name/value pairs of types that are common to store game state.
///
/// Provides type-safe storage and retrieval of common data types used in game state management.
/// Values are stored by name and can be retrieved with optional default values.
class W_CORE_DLL WStateMap
{
public:
  WStateMap();
  ~WStateMap();

  /// void Load(WStreamReader& stream);
  /// void Save(WStreamWriter& stream) const;
  /// Lock / Unlock

  void Clear();

  void StoreBool(const WTempHashedString& sName, bool value);
  void StoreInteger(const WTempHashedString& sName, WInt64 value);
  void StoreDouble(const WTempHashedString& sName, double value);
  void StoreVec3(const WTempHashedString& sName, const WVec3& value);
  void StoreColor(const WTempHashedString& sName, const WColor& value);
  void StoreString(const WTempHashedString& sName, const WString& value);

  void RetrieveBool(const WTempHashedString& sName, bool& out_bValue, bool bDefaultValue = false);
  void RetrieveInteger(const WTempHashedString& sName, WInt64& out_iValue, WInt64 iDefaultValue = 0);
  void RetrieveDouble(const WTempHashedString& sName, double& out_fValue, double fDefaultValue = 0);
  void RetrieveVec3(const WTempHashedString& sName, WVec3& out_vValue, WVec3 vDefaultValue = WVec3(0));
  void RetrieveColor(const WTempHashedString& sName, WColor& out_value, WColor defaultValue = WColor::White);
  void RetrieveString(const WTempHashedString& sName, WString& out_sValue, WStringView sDefaultValue = {});

private:
  WHashTable<WTempHashedString, bool> m_Bools;
  WHashTable<WTempHashedString, WInt64> m_Integers;
  WHashTable<WTempHashedString, double> m_Doubles;
  WHashTable<WTempHashedString, WVec3> m_Vec3s;
  WHashTable<WTempHashedString, WColor> m_Colors;
  WHashTable<WTempHashedString, WString> m_Strings;
};
