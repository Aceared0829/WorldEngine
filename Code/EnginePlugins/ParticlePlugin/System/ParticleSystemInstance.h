#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct WMsgExtractRenderData;

/// A particle system stores all data for one 'layer' of a running particle effect
class W_PARTICLEPLUGIN_DLL WParticleSystemInstance
{
public:
  WParticleSystemInstance();

  void Construct(WUInt32 uiMaxParticles, WWorld* pWorld, WParticleEffectInstance* pOwnerEffect, float fSpawnCountMultiplier);
  void Destruct();

  bool IsVisible() const { return m_bVisible; }

  void SetEmitterEnabled(bool bEnable) { m_bEmitterEnabled = bEnable; }
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void ConfigureFromTemplate(const WParticleSystemDescriptor* pTemplate);
  void Finalize();

  void ReinitializeStreamProcessors(const WParticleSystemDescriptor* pTemplate);

  void CreateStreamProcessors(const WParticleSystemDescriptor* pTemplate);

  void SetupOptionalStreams();

  void SetTransform(const WTransform& transform, const WVec3& vParticleStartVelocity);
  const WTransform& GetTransform() const { return m_Transform; }
  const WVec3& GetParticleStartVelocity() const { return m_vParticleStartVelocity; }

  WParticleSystemState::Enum Update(const WTime& diff);

  WWorld* GetWorld() const { return m_pWorld; }

  WUInt64 GetMaxParticles() const { return m_StreamGroup.GetNumElements(); }
  WUInt64 GetNumActiveParticles() const { return m_StreamGroup.GetNumActiveElements(); }



  /// Returns the desired stream, if it already exists, nullptr otherwise.
  WProcessingStream* QueryStream(WTempHashedString sName, WProcessingStream::DataType type) const;

  /// Returns the desired stream, if it already exists, creates it otherwise.
  void CreateStream(WStringView sName, WProcessingStream::DataType type, WProcessingStream** pStream, WParticleStreamBinding& ref_binding, bool bExpectInitializedValue);

  void ProcessEventQueue(WParticleEventQueue queue);

  WParticleEffectInstance* GetOwnerEffect() const { return m_pOwnerEffect; }
  WParticleWorldModule* GetOwnerWorldModule() const;

  void ExtractSystemRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const;

  using ParticleDeathHandler = WEvent<const WStreamGroupElementRemovedEvent&>::Handler;

  void AddParticleDeathEventHandler(ParticleDeathHandler handler);
  void RemoveParticleDeathEventHandler(ParticleDeathHandler handler);

  void SetBoundingVolume(const WBoundingBoxSphere& volume, float fMaxParticleSize);
  const WBoundingBoxSphere& GetBoundingVolume() const { return m_BoundingVolume; }

  bool IsContinuous() const;

  float GetSpawnCountMultiplier() const { return m_fSpawnCountMultiplier; }

private:
  bool IsEmitterConfigEqual(const WParticleSystemDescriptor* pTemplate) const;
  bool IsInitializerConfigEqual(const WParticleSystemDescriptor* pTemplate) const;
  bool IsBehaviorConfigEqual(const WParticleSystemDescriptor* pTemplate) const;
  bool IsTypeConfigEqual(const WParticleSystemDescriptor* pTemplate) const;
  bool IsFinalizerConfigEqual(const WParticleSystemDescriptor* pTemplate) const;

  void CreateStreamZeroInitializers();

  WSmallArray<WParticleEmitter*, 2> m_Emitters;
  WSmallArray<WParticleInitializer*, 6> m_Initializers;
  WSmallArray<WParticleBehavior*, 6> m_Behaviors;
  WSmallArray<WParticleFinalizer*, 2> m_Finalizers;
  WSmallArray<WParticleType*, 2> m_Types;

  bool m_bVisible; // typically used in editor to hide a system
  bool m_bEmitterEnabled;
  WParticleEffectInstance* m_pOwnerEffect;
  WWorld* m_pWorld;
  WTransform m_Transform;
  WVec3 m_vParticleStartVelocity;
  float m_fSpawnCountMultiplier = 1.0f;

  WProcessingStreamGroup m_StreamGroup;

  struct StreamInfo
  {
    WHashedString m_sName;
    bool m_bGetsInitialized = false;
    bool m_bInUse = false;
    WProcessingStreamProcessor* m_pDefaultInitializer = nullptr;
  };

  WSmallArray<StreamInfo, 16> m_StreamInfo;

  // culling data
  WBoundingBoxSphere m_BoundingVolume;
};
