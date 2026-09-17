#pragma once

#include <Core/World/World.h>
#include <GameEngine/Volumes/VolumeSampler.h>

struct WVolumeSamplerValue
{
  WHashedString m_sName;
  WVariant m_DefaultValue;
  WTime m_InterpolationDuration;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WVolumeSamplerValue);

using WVolumeSamplerComponentManager = WComponentManagerSimple<class WVolumeSamplerComponent, WComponentUpdateType::Always, WBlockStorageType::Compact, WWorldUpdatePhase::Async>;

/// A component that wraps an WVolumeSampler to sample arbitrary values from volumes at the owner's position (or the main camera's position).
///
/// The sampled values can then be queried via script or C++ code and used for things like gameplay logic, audio modulation, etc.
class W_GAMEENGINE_DLL WVolumeSamplerComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WVolumeSamplerComponent, WComponent, WVolumeSamplerComponentManager);

public:
  WVolumeSamplerComponent();
  WVolumeSamplerComponent(WVolumeSamplerComponent&& other);
  ~WVolumeSamplerComponent();
  WVolumeSamplerComponent& operator=(WVolumeSamplerComponent&& other);

  virtual void OnActivated() override;

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  /// Sets which volume type to sample from.
  void SetVolumeType(const char* szType); // [ property ]
  const char* GetVolumeType() const;      // [ property ]

  /// If enabled, the sampling position will be the main camera's position. Otherwise, the owner's position is used.
  void SetAttachToMainCamera(bool bAttach);                            // [ property ]
  bool GetAttachToMainCamera() const { return m_bAttachToMainCamera; } // [ property ]

  /// If enabled, the sampled values will also be written to a blackboard.
  /// See WBlackboardComponent::FindBlackboard for details on how blackboards are found.
  void SetWriteToBlackboard(bool bWriteToBlackboard);                // [ property ]
  bool GetWriteToBlackboard() const { return m_bWriteToBlackboard; } // [ property ]

  /// The name of the blackboard to write to. Only relevant if WriteToBlackboard is enabled.
  /// See WBlackboardComponent::FindBlackboard for details on how blackboards are found.
  void SetBlackboardName(const WHashedString& sName);                          // [ property ]
  const WHashedString& GetBlackboardName() const { return m_sBlackboardName; } // [ property ]

  /// Registers a new value to be sampled from the volumes. This registration is only done at runtime and not serialized.
  void RegisterValue(const WHashedString& sName, const WVariant& defaultValue, WTime interpolationDuration = WTime::MakeZero()); // [ scriptable ]

  /// Get the latest sampled value for the given name.
  WVariant GetValue(const WHashedString& sName) const;                                                   // [ scriptable ]
  float GetFloatValue(const WHashedString& sName, float fFallbackValue = 0.0f) const;                     // [ scriptable ]
  WColor GetColorValue(const WHashedString& sName, const WColor& fallbackValue = WColor::White) const; // [ scriptable ]

private:
  WUInt32 Values_GetCount() const { return m_Values.GetCount(); }                                         // [ property ]
  const WVolumeSamplerValue& Values_GetMapping(WUInt32 i) const { return m_Values[i]; }                  // [ property ]
  void Values_SetMapping(WUInt32 i, const WVolumeSamplerValue& mapping);                                 // [ property ]
  void Values_Insert(WUInt32 uiIndex, const WVolumeSamplerValue& mapping);                               // [ property ]
  void Values_Remove(WUInt32 uiIndex);                                                                    // [ property ]

  void RegisterSamplerValues(WArrayPtr<const WVolumeSamplerValue> values);
  void Update();

  WDynamicArray<WVolumeSamplerValue> m_Values;
  WUniquePtr<WVolumeSampler> m_pSampler;
  WSpatialData::Category m_SpatialCategory = WInvalidSpatialDataCategory;
  bool m_bAttachToMainCamera = false;
  bool m_bWriteToBlackboard = false;

  WHashedString m_sBlackboardName;

  WSharedPtr<WBlackboard> m_pBlackboard;
};
