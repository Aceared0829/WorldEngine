#include <AiPlugin/Navigation/NavigationConfig.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

WAiNavigationConfig::WAiNavigationConfig()
{
  m_GroundTypes[0].m_bUsed = true;
  m_GroundTypes[0].m_sName = "<None>";
  m_GroundTypes[1].m_bUsed = true;
  m_GroundTypes[1].m_sName = "<Default>";

  WStringBuilder tmp;
  for (WUInt32 i = 2; i < WAiNumGroundTypes; ++i)
  {
    tmp.SetFormat("Custom Ground Type {}", i + 1);
    m_GroundTypes[i].m_sName = tmp;
  }

  for (WUInt32 i = 0; i < WAiNumGroundTypes / 8; ++i)
  {
    const float f = 1.0f + i * 0.5f;
    m_GroundTypes[(i * 8) + 0].m_Color = WColor::RosyBrown.GetDarker(f);
    m_GroundTypes[(i * 8) + 1].m_Color = WColor::CornflowerBlue.GetDarker(f);
    m_GroundTypes[(i * 8) + 2].m_Color = WColor::Crimson.GetDarker(f);
    m_GroundTypes[(i * 8) + 3].m_Color = WColor::Cyan.GetDarker(f);
    m_GroundTypes[(i * 8) + 4].m_Color = WColor::DarkSeaGreen.GetDarker(f);
    m_GroundTypes[(i * 8) + 5].m_Color = WColor::DarkViolet.GetDarker(f);
    m_GroundTypes[(i * 8) + 6].m_Color = WColor::HoneyDew.GetDarker(f);
    m_GroundTypes[(i * 8) + 7].m_Color = WColor::LightSalmon.GetDarker(f);
  }
}

WResult WAiNavigationConfig::Save(WStringView sFile) const
{
  WFileWriter file;
  if (file.Open(sFile).Failed())
    return W_FAILURE;

  Save(file);

  return W_SUCCESS;
}

WResult WAiNavigationConfig::Load(WStringView sFile)
{
  WFileReader file;
  if (file.Open(sFile).Failed())
    return W_FAILURE;

  Load(file);
  return W_SUCCESS;
}

void WAiNavigationConfig::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 2;

  inout_stream << uiVersion;

  const WUInt8 numGroundTypes = WAiNumGroundTypes;
  inout_stream << numGroundTypes;

  for (WUInt32 i = 0; i < numGroundTypes; ++i)
  {
    const auto& gt = m_GroundTypes[i];
    inout_stream << gt.m_bUsed;
    inout_stream << gt.m_sName;
    inout_stream << gt.m_Color;
  }

  const WUInt8 numSearchTypes = m_PathSearchConfigs.GetCount();
  inout_stream << numSearchTypes;

  for (WUInt32 i = 0; i < numSearchTypes; ++i)
  {
    const auto& cfg = m_PathSearchConfigs[i];

    inout_stream << cfg.m_sName;
    inout_stream.WriteArray(cfg.m_fGroundTypeCost).AssertSuccess();
    inout_stream.WriteArray(cfg.m_bGroundTypeAllowed).AssertSuccess();
  }

  const WUInt8 numNavmeshTypes = m_NavmeshConfigs.GetCount();
  inout_stream << numNavmeshTypes;

  for (WUInt32 i = 0; i < numNavmeshTypes; ++i)
  {
    const auto& nc = m_NavmeshConfigs[i];

    inout_stream << nc.m_sName;

    inout_stream << nc.m_uiNumSectorsX;
    inout_stream << nc.m_uiNumSectorsY;
    inout_stream << nc.m_fSectorSize;

    inout_stream << nc.m_uiCollisionLayer;
    inout_stream << nc.m_fCellSize;
    inout_stream << nc.m_fCellHeight;
    inout_stream << nc.m_fAgentRadius;
    inout_stream << nc.m_fAgentHeight;
    inout_stream << nc.m_fAgentStepHeight;
    inout_stream << nc.m_WalkableSlope;

    inout_stream << nc.m_fMaxEdgeLength;
    inout_stream << nc.m_fMaxSimplificationError;
    inout_stream << nc.m_fMinRegionSize;
    inout_stream << nc.m_fRegionMergeSize;
    inout_stream << nc.m_fDetailMeshSampleDistanceFactor;
    inout_stream << nc.m_fDetailMeshSampleErrorFactor;
  }
}

void WAiNavigationConfig::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= 2, "Invalid version {0} for WAiNavigationConfig file", uiVersion);

  WUInt8 numGroundTypes = 0;
  inout_stream >> numGroundTypes;

  numGroundTypes = WMath::Min<WUInt8>(numGroundTypes, WAiNumGroundTypes);

  for (WUInt32 i = 0; i < numGroundTypes; ++i)
  {
    auto& gt = m_GroundTypes[i];
    inout_stream >> gt.m_bUsed;
    inout_stream >> gt.m_sName;
    inout_stream >> gt.m_Color;
  }

  WUInt8 numSearchTypes = 0;
  inout_stream >> numSearchTypes;

  m_PathSearchConfigs.Clear();
  m_PathSearchConfigs.Reserve(numSearchTypes);

  for (WUInt32 i = 0; i < numSearchTypes; ++i)
  {
    auto& cfg = m_PathSearchConfigs.ExpandAndGetRef();

    inout_stream >> cfg.m_sName;
    inout_stream.ReadArray(cfg.m_fGroundTypeCost).AssertSuccess();
    inout_stream.ReadArray(cfg.m_bGroundTypeAllowed).AssertSuccess();
  }

  WUInt8 numNavmeshTypes = 0;
  inout_stream >> numNavmeshTypes;

  m_NavmeshConfigs.Clear();
  m_NavmeshConfigs.Reserve(numNavmeshTypes);

  for (WUInt32 i = 0; i < numNavmeshTypes; ++i)
  {
    auto& nc = m_NavmeshConfigs.ExpandAndGetRef();

    inout_stream >> nc.m_sName;

    if (uiVersion >= 2)
    {
      inout_stream >> nc.m_uiNumSectorsX;
      inout_stream >> nc.m_uiNumSectorsY;
      inout_stream >> nc.m_fSectorSize;
    }

    inout_stream >> nc.m_uiCollisionLayer;
    inout_stream >> nc.m_fCellSize;
    inout_stream >> nc.m_fCellHeight;
    inout_stream >> nc.m_fAgentRadius;
    inout_stream >> nc.m_fAgentHeight;
    inout_stream >> nc.m_fAgentStepHeight;
    inout_stream >> nc.m_WalkableSlope;

    inout_stream >> nc.m_fMaxEdgeLength;
    inout_stream >> nc.m_fMaxSimplificationError;
    inout_stream >> nc.m_fMinRegionSize;
    inout_stream >> nc.m_fRegionMergeSize;
    inout_stream >> nc.m_fDetailMeshSampleDistanceFactor;
    inout_stream >> nc.m_fDetailMeshSampleErrorFactor;
  }
}

WAiPathSearchConfig::WAiPathSearchConfig()
{
  for (WUInt32 i = 0; i < WAiNumGroundTypes; ++i)
  {
    m_fGroundTypeCost[i] = 1.0f;
    m_bGroundTypeAllowed[i] = true;
  }
}
