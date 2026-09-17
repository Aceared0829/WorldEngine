#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GameEngine/Physics/CollisionFilter.h>

WCollisionFilterConfig::WCollisionFilterConfig()
{
  for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_GroupMasks); ++i)
  {
    m_GroupMasks[i] = 0;
  }
}

WCollisionFilterConfig::~WCollisionFilterConfig() = default;

void WCollisionFilterConfig::SetGroupName(WUInt32 uiGroup, WStringView sName)
{
  m_GroupNames[uiGroup] = sName;
}

WStringView WCollisionFilterConfig::GetGroupName(WUInt32 uiGroup) const
{
  return m_GroupNames[uiGroup];
}

void WCollisionFilterConfig::EnableCollision(WUInt32 uiGroup1, WUInt32 uiGroup2, bool bEnable)
{
  if (bEnable)
  {
    m_GroupMasks[uiGroup1] |= W_BIT(uiGroup2);
    m_GroupMasks[uiGroup2] |= W_BIT(uiGroup1);
  }
  else
  {
    m_GroupMasks[uiGroup1] &= ~W_BIT(uiGroup2);
    m_GroupMasks[uiGroup2] &= ~W_BIT(uiGroup1);
  }
}

bool WCollisionFilterConfig::IsCollisionEnabled(WUInt32 uiGroup1, WUInt32 uiGroup2) const
{
  return (m_GroupMasks[uiGroup1] & W_BIT(uiGroup2)) != 0;
}

WUInt32 WCollisionFilterConfig::GetNumNamedGroups() const
{
  WUInt32 count = 0;

  for (WUInt32 i = 0; i < 32; ++i)
  {
    if (!m_GroupNames[i].IsEmpty())
      ++count;
  }

  return count;
}

WUInt32 WCollisionFilterConfig::GetNamedGroupIndex(WUInt32 uiGroup) const
{
  for (WUInt32 i = 0; i < 32; ++i)
  {
    if (!m_GroupNames[i].IsEmpty())
    {
      if (uiGroup == 0)
        return i;

      --uiGroup;
    }
  }

  W_REPORT_FAILURE("Invalid index, there are not so many named collision filter groups");
  return WInvalidIndex;
}

WUInt32 WCollisionFilterConfig::GetFilterGroupByName(WStringView sName) const
{
  for (WUInt32 i = 0; i < 32; ++i)
  {
    if (sName.IsEqual_NoCase(m_GroupNames[i]))
      return i;
  }

  return WInvalidIndex;
}

WUInt32 WCollisionFilterConfig::FindUnnamedGroup() const
{
  for (WUInt32 i = 0; i < 32; ++i)
  {
    if (m_GroupNames[i].IsEmpty())
      return i;
  }

  return WInvalidIndex;
}

WResult WCollisionFilterConfig::Save(WStringView sFile) const
{
  WFileWriter file;
  if (file.Open(sFile).Failed())
    return W_FAILURE;

  Save(file);

  return W_SUCCESS;
}

WResult WCollisionFilterConfig::Load(WStringView sFile)
{
  WFileReader file;
  if (file.Open(sFile).Failed())
    return W_FAILURE;

  Load(file);
  return W_SUCCESS;
}

void WCollisionFilterConfig::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 2;

  inout_stream << uiVersion;

  inout_stream.WriteBytes(m_GroupMasks, sizeof(WUInt32) * 32).AssertSuccess();

  for (WUInt32 i = 0; i < 32; ++i)
  {
    inout_stream << m_GroupNames[i];
  }
}


void WCollisionFilterConfig::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion == 1 || uiVersion == 2, "Invalid version {0} for WCollisionFilterConfig file", uiVersion);

  inout_stream.ReadBytes(m_GroupMasks, sizeof(WUInt32) * 32);

  for (WUInt32 i1 = 0; i1 < 32; ++i1)
  {
    for (WUInt32 i2 = 0; i2 < 32; ++i2)
    {
      const bool b1 = (m_GroupMasks[i1] & W_BIT(i2)) != 0;
      const bool b2 = (m_GroupMasks[i2] & W_BIT(i1)) != 0;

      if (b1 != b2)
      {
        // reset group masks that differ due to previously uninitialized memory
        m_GroupMasks[i1] &= ~W_BIT(i2);
        m_GroupMasks[i2] &= ~W_BIT(i1);
      }
    }
  }

  if (uiVersion == 1)
  {
    char groupNames[32][32];
    inout_stream.ReadBytes(groupNames, sizeof(char) * 32 * 32);

    for (WUInt32 i = 0; i < 32; ++i)
    {
      m_GroupNames[i] = groupNames[i];
    }
  }
  else
  {
    for (WUInt32 i = 0; i < 32; ++i)
    {
      inout_stream >> m_GroupNames[i];
    }
  }
}
