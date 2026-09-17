#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Timestamp.h>

struct WPhantomRttiManagerEvent;
class WExposedParameters;
struct WAssetCuratorEvent;

/// Lazily converts WExposedParameters into phantom types.
/// Call GetExposedParametersType to create a type for a sub-asset ID.
class WExposedParametersTypeRegistry
{
  W_DECLARE_SINGLETON(WExposedParametersTypeRegistry);

public:
  WExposedParametersTypeRegistry();
  ~WExposedParametersTypeRegistry();
  /// Returns null if the curator can find the asset or if the asset
  /// does not have any WExposedParameters meta data.
  const WRTTI* GetExposedParametersType(const char* szResource);
  /// All exposed parameter types derive from this.
  const WRTTI* GetExposedParametersBaseType() const { return m_pBaseType; }

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, ExposedParametersTypeRegistry);

  struct ParamData
  {
    ParamData()

      = default;

    WUuid m_SubAssetGuid;
    bool m_bUpToDate = true;
    const WRTTI* m_pType = nullptr;
  };
  void UpdateExposedParametersType(ParamData& data, const WExposedParameters& params);
  void AssetCuratorEventHandler(const WAssetCuratorEvent& e);
  void PhantomTypeRegistryEventHandler(const WPhantomRttiManagerEvent& e);

  WMap<WUuid, ParamData> m_ShaderTypes;
  const WRTTI* m_pBaseType;
  ParamData* m_pAboutToBeRegistered = nullptr;
};
