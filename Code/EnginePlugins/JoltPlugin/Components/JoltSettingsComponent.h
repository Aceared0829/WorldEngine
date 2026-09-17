#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <JoltPlugin/Declarations.h>

using WJoltSettingsComponentManager = WSettingsComponentManager<class WJoltSettingsComponent>;

class W_JOLTPLUGIN_DLL WJoltSettingsComponent : public WSettingsComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltSettingsComponent, WSettingsComponent, WJoltSettingsComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // WJoltSettingsComponent

public:
  WJoltSettingsComponent();
  ~WJoltSettingsComponent();

  const WJoltSettings& GetSettings() const { return m_Settings; }

  const WVec3& GetObjectGravity() const { return m_Settings.m_vObjectGravity; }           // [ property ]
  void SetObjectGravity(const WVec3& v);                                                  // [ property ]

  const WVec3& GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; }     // [ property ]
  void SetCharacterGravity(const WVec3& v);                                               // [ property ]

  WJoltSteppingMode::Enum GetSteppingMode() const { return m_Settings.m_SteppingMode; }   // [ property ]
  void SetSteppingMode(WJoltSteppingMode::Enum mode);                                     // [ property ]

  float GetFixedFrameRate() const { return m_Settings.m_fFixedFrameRate; }                 // [ property ]
  void SetFixedFrameRate(float fFixedFrameRate);                                           // [ property ]

  WUInt32 GetMaxSubSteps() const { return m_Settings.m_uiMaxSubSteps; }                   // [ property ]
  void SetMaxSubSteps(WUInt32 uiMaxSubSteps);                                             // [ property ]

  WUInt32 GetMaxBodies() const { return m_Settings.m_uiMaxBodies; }                       // [ property ]
  void SetMaxBodies(WUInt32 uiMaxBodies);                                                 // [ property ]

  float GetSleepVelocityThreshold() const { return m_Settings.m_fSleepVelocityThreshold; } // [ property ]
  void SetSleepVelocityThreshold(float fSleepVelocityThreshold);                           // [ property ]

protected:
  WJoltSettings m_Settings;
};
