#pragma once

#include <Core/World/Component.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Resources/JoltHeightfieldResource.h>

using WJoltHeightfieldColliderComponentManager = WComponentManager<class WJoltHeightfieldColliderComponent, WBlockStorageType::FreeList>;

/// Manages a Jolt static body with a heightfield shape.
class W_JOLTPLUGIN_DLL WJoltHeightfieldColliderComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltHeightfieldColliderComponent, WComponent, WJoltHeightfieldColliderComponentManager);

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

public:
  WJoltHeightfieldColliderComponent();
  ~WJoltHeightfieldColliderComponent();

  WJoltHeightfieldResourceHandle m_hHeightfield; //< [ property ]

private:
  WUInt32 m_uiJoltBodyID = WInvalidIndex;
  WUInt32 m_uiUserDataIndex = WInvalidIndex;
  WUInt32 m_uiObjectFilterID = WInvalidIndex;
};
