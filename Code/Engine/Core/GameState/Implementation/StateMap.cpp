#include <Core/CorePCH.h>

#include <Core/GameState/StateMap.h>

WStateMap::WStateMap() = default;
WStateMap::~WStateMap() = default;


void WStateMap::Clear()
{
  m_Bools.Clear();
  m_Integers.Clear();
  m_Doubles.Clear();
  m_Vec3s.Clear();
  m_Colors.Clear();
  m_Strings.Clear();
}

void WStateMap::StoreBool(const WTempHashedString& sName, bool value)
{
  m_Bools[sName] = value;
}

void WStateMap::StoreInteger(const WTempHashedString& sName, WInt64 value)
{
  m_Integers[sName] = value;
}

void WStateMap::StoreDouble(const WTempHashedString& sName, double value)
{
  m_Doubles[sName] = value;
}

void WStateMap::StoreVec3(const WTempHashedString& sName, const WVec3& value)
{
  m_Vec3s[sName] = value;
}

void WStateMap::StoreColor(const WTempHashedString& sName, const WColor& value)
{
  m_Colors[sName] = value;
}

void WStateMap::StoreString(const WTempHashedString& sName, const WString& value)
{
  m_Strings[sName] = value;
}

void WStateMap::RetrieveBool(const WTempHashedString& sName, bool& out_bValue, bool bDefaultValue /*= false*/)
{
  if (!m_Bools.TryGetValue(sName, out_bValue))
  {
    out_bValue = bDefaultValue;
  }
}

void WStateMap::RetrieveInteger(const WTempHashedString& sName, WInt64& out_iValue, WInt64 iDefaultValue /*= 0*/)
{
  if (!m_Integers.TryGetValue(sName, out_iValue))
  {
    out_iValue = iDefaultValue;
  }
}

void WStateMap::RetrieveDouble(const WTempHashedString& sName, double& out_fValue, double fDefaultValue /*= 0*/)
{
  if (!m_Doubles.TryGetValue(sName, out_fValue))
  {
    out_fValue = fDefaultValue;
  }
}

void WStateMap::RetrieveVec3(const WTempHashedString& sName, WVec3& out_vValue, WVec3 vDefaultValue /*= WVec3(0)*/)
{
  if (!m_Vec3s.TryGetValue(sName, out_vValue))
  {
    out_vValue = vDefaultValue;
  }
}

void WStateMap::RetrieveColor(const WTempHashedString& sName, WColor& out_value, WColor defaultValue /*= WColor::White*/)
{
  if (!m_Colors.TryGetValue(sName, out_value))
  {
    out_value = defaultValue;
  }
}

void WStateMap::RetrieveString(const WTempHashedString& sName, WString& out_sValue, WStringView sDefaultValue /*= {} */)
{
  if (!m_Strings.TryGetValue(sName, out_sValue))
  {
    out_sValue = sDefaultValue;
  }
}
