#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Strings/String.h>

static constexpr WUInt32 WAiNumGroundTypes = 32;

struct W_AIPLUGIN_DLL WAiNavmeshConfig
{
  WString m_sName;

  WUInt16 m_uiNumSectorsX = 64;
  WUInt16 m_uiNumSectorsY = 64;

  float m_fSectorSize = 32.0f;

  /// The physics collision layer to use for building this navmesh (retrieving the physics geometry).
  WUInt8 m_uiCollisionLayer = 0;

  float m_fCellSize = 0.2f;
  float m_fCellHeight = 0.2f;

  float m_fAgentRadius = 0.2f;
  float m_fAgentHeight = 1.5f;
  float m_fAgentStepHeight = 0.6f;

  WAngle m_WalkableSlope = WAngle::MakeFromDegree(45);

  float m_fMaxEdgeLength = 4.0f;
  float m_fMaxSimplificationError = 1.3f;
  float m_fMinRegionSize = 0.5f;
  float m_fRegionMergeSize = 5.0f;
  float m_fDetailMeshSampleDistanceFactor = 1.0f;
  float m_fDetailMeshSampleErrorFactor = 1.0f;
};

struct W_AIPLUGIN_DLL WAiPathSearchConfig
{
  WAiPathSearchConfig();

  WString m_sName;
  float m_fGroundTypeCost[WAiNumGroundTypes];   // = 1.0f
  bool m_bGroundTypeAllowed[WAiNumGroundTypes]; // = true
};

struct W_AIPLUGIN_DLL WAiNavigationConfig
{
  WAiNavigationConfig();

  struct GroundType
  {
    bool m_bUsed = false;
    WString m_sName;
    WColorGammaUB m_Color;
  };

  GroundType m_GroundTypes[WAiNumGroundTypes];

  WDynamicArray<WAiPathSearchConfig> m_PathSearchConfigs;
  WDynamicArray<WAiNavmeshConfig> m_NavmeshConfigs;

  static constexpr const WStringView s_sConfigFile = ":project/RuntimeConfigs/AiPluginConfig.cfg"_wsv;

  WResult Save(WStringView sFile = s_sConfigFile) const;
  WResult Load(WStringView sFile = s_sConfigFile);

  void Save(WStreamWriter& inout_stream) const;
  void Load(WStreamReader& inout_stream);
};
