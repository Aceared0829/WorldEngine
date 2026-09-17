#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GameEngine/Physics/WeightCategory.h>

WWeightCategoryConfig::WWeightCategoryConfig() = default;
WWeightCategoryConfig::~WWeightCategoryConfig() = default;

WUInt8 WWeightCategoryConfig::FindByName(WTempHashedString sName) const
{
  m_Categories.Sort();

  for (WUInt32 idx = 0; idx < m_Categories.GetCount(); ++idx)
  {
    const auto& item = m_Categories.GetValue(idx);
    if (item.m_sName == sName)
    {
      return m_Categories.GetKey(idx);
    }
  }

  return InvalidKey;
}

WUInt8 WWeightCategoryConfig::GetFreeKey() const
{
  m_Categories.Sort();
  for (WUInt8 idx = FirstValidKey; idx < 250; ++idx)
  {
    if (!m_Categories.Contains(idx))
      return idx;
  }

  return InvalidKey;
}

WResult WWeightCategoryConfig::Save(WStringView sFile /*= s_sConfigFile*/) const
{
  WFileWriter file;
  if (file.Open(sFile).Failed())
    return W_FAILURE;

  Save(file);

  return W_SUCCESS;
}

WResult WWeightCategoryConfig::Load(WStringView sFile /*= s_sConfigFile*/)
{
  WFileReader file;
  if (file.Open(sFile).Failed())
    return W_FAILURE;

  Load(file);
  return W_SUCCESS;
}

void WWeightCategoryConfig::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  m_Categories.Sort();
  const WUInt16 uiNumCats = m_Categories.GetCount();

  inout_stream << uiNumCats;

  for (WUInt32 i = 0; i < uiNumCats; ++i)
  {
    const auto& cat = m_Categories.GetPair(i);

    inout_stream << cat.key;
    inout_stream << cat.value.m_sName;
    inout_stream << cat.value.m_fMass;
    inout_stream << cat.value.m_sDescription;
  }
}

void WWeightCategoryConfig::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= 1, "Invalid version '{0}' for WWeightCategoryConfig file", uiVersion);

  WUInt16 uiNumCats = 0;
  inout_stream >> uiNumCats;

  m_Categories.Clear();
  m_Categories.Reserve(uiNumCats);

  for (WUInt32 i = 0; i < uiNumCats; ++i)
  {
    WUInt8 idx = 0;
    inout_stream >> idx;

    auto& item = m_Categories[idx];
    inout_stream >> item.m_sName;

    inout_stream >> item.m_fMass;
    inout_stream >> item.m_sDescription;
  }

  m_Categories.Sort();
}

float WWeightCategoryConfig::GetMassForWeightCategory(WUInt8 uiWeightCategory, float fDefaultMass, float fCustomMass, float fWeightScale, float fMinMass, float fMaxMass) const
{
  if (uiWeightCategory == WWeightCategoryConfig::DefaultValueKey)
    return fDefaultMass;

  if (uiWeightCategory == WWeightCategoryConfig::CustomMassKey)
    return fCustomMass;

  if (uiWeightCategory == WWeightCategoryConfig::CustomDensityKey)
    return 0.0f; // use zero mass, to calculate it from density instead

  auto& cat = m_Categories;
  WUInt32 idx = cat.Find(uiWeightCategory);

  if (idx != WInvalidIndex)
  {
    return WMath::Clamp(cat.GetValue(idx).m_fMass * fWeightScale, fMinMass, fMaxMass);
  }

  return fDefaultMass;
}
