#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GameEngine/Physics/ImpulseType.h>

WImpulseTypeConfig::WImpulseTypeConfig() = default;
WImpulseTypeConfig::~WImpulseTypeConfig() = default;

WUInt8 WImpulseTypeConfig::FindByName(WTempHashedString sName) const
{
  m_Types.Sort();

  for (WUInt32 idx = 0; idx < m_Types.GetCount(); ++idx)
  {
    const auto& item = m_Types.GetValue(idx);
    if (item.m_sName == sName)
    {
      return m_Types.GetKey(idx);
    }
  }

  return InvalidKey;
}

WUInt8 WImpulseTypeConfig::GetFreeKey() const
{
  m_Types.Sort();
  for (WUInt8 idx = FirstValidKey; idx < 250; ++idx)
  {
    if (!m_Types.Contains(idx))
      return idx;
  }

  return InvalidKey;
}

WResult WImpulseTypeConfig::Save(WStringView sFile /*= s_sConfigFile*/) const
{
  WFileWriter file;
  if (file.Open(sFile).Failed())
    return W_FAILURE;

  Save(file);

  return W_SUCCESS;
}

WResult WImpulseTypeConfig::Load(WStringView sFile /*= s_sConfigFile*/)
{
  WFileReader file;
  if (file.Open(sFile).Failed())
    return W_FAILURE;

  Load(file);
  return W_SUCCESS;
}

void WImpulseTypeConfig::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  m_Types.Sort();
  const WUInt16 uiNumCats = m_Types.GetCount();

  inout_stream << uiNumCats;

  for (WUInt32 i = 0; i < uiNumCats; ++i)
  {
    const auto& cat = m_Types.GetPair(i);

    inout_stream << cat.key;
    inout_stream << cat.value.m_sName;
    inout_stream << cat.value.m_fDefaultValue;
    inout_stream << cat.value.m_sDescription;

    cat.value.m_WeightOverrides.Sort();
    inout_stream << cat.value.m_WeightOverrides.GetCount();
    for (const auto& f : cat.value.m_WeightOverrides)
    {
      inout_stream << f.key;
      inout_stream << f.value;
    }
  }
}

void WImpulseTypeConfig::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= 1, "Invalid version '{0}' for WImpulseTypeConfig file", uiVersion);

  WUInt16 uiNumCats = 0;
  inout_stream >> uiNumCats;

  m_Types.Clear();
  m_Types.Reserve(uiNumCats);

  for (WUInt32 i = 0; i < uiNumCats; ++i)
  {
    WUInt8 idx = 0;
    inout_stream >> idx;

    auto& item = m_Types[idx];
    inout_stream >> item.m_sName;

    inout_stream >> item.m_fDefaultValue;
    inout_stream >> item.m_sDescription;

    WUInt32 uiNum = 0;
    inout_stream >> uiNum;

    for (WUInt32 i = 0; i < uiNum; ++i)
    {
      WUInt8 uiKey;
      float fForce;

      inout_stream >> uiKey;
      inout_stream >> fForce;

      item.m_WeightOverrides.Insert(uiKey, fForce);
    }

    item.m_WeightOverrides.Sort();
  }

  m_Types.Sort();
}

float WImpulseTypeConfig::GetImpulseForWeight(WUInt8 uiImpulseType, WUInt8 uiWeightCategory) const
{
  if (uiImpulseType == WImpulseTypeConfig::NoValueKey)
    return 0.0f;

  if (uiImpulseType == WImpulseTypeConfig::CustomValueKey)
    return 1.0f;

  // if the impulse is given as a type, look up the WeightCategory-specific impulse
  const WUInt32 impIdx = m_Types.Find(uiImpulseType);
  if (impIdx == WInvalidIndex)
    return 1.0f;

  const auto& impulse = m_Types.GetValue(impIdx);

  const WUInt32 weightIdx = impulse.m_WeightOverrides.Find(uiWeightCategory);
  if (weightIdx == WInvalidIndex)
    return impulse.m_fDefaultValue;

  // override the impulse for this weight category
  return impulse.m_WeightOverrides.GetValue(weightIdx);
}
