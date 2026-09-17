#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/SharedPtr.h>

#include <ParticlePlugin/System/ParticleSystemInstance.h>

/// Pre-computed data for one attractor, written by the main thread and consumed by particle tasks.
struct WParticleAttractorData
{
  W_DECLARE_POD_TYPE();

  WVec3 m_vPosition;
  float m_fStrength;
  float m_fRadius;
  float m_fMinDistance;
  float m_fKillDistance;
};


class WParticleEffectInstance;

class WParticleEffectUpdateTask final : public WTask
{
public:
  WParticleEffectUpdateTask(WParticleEffectInstance* pEffect);

  WTime m_UpdateDiff;

private:
  virtual void Execute() override;

  WParticleEffectInstance* m_pEffect;
};

class W_PARTICLEPLUGIN_DLL WParticleEffectInstance
{
  friend class WParticleWorldModule;
  friend class WParticleEffectUpdateTask;

public:
  WParticleEffectInstance();
  ~WParticleEffectInstance();

  void Construct(WParticleEffectHandle hEffectHandle, const WParticleEffectResourceHandle& hResource, WWorld* pWorld, WParticleWorldModule* pOwnerModule, WUInt64 uiRandomSeed, bool bIsShared, WArrayPtr<WParticleEffectFloatParam> floatParams, WArrayPtr<WParticleEffectColorParam> colorParams);
  void Destruct();

  void Interrupt();

  const WParticleEffectHandle& GetHandle() const { return m_hEffectHandle; }

  void SetEmitterEnabled(bool bEnable);
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void ClearParticleSystems();
  void ClearEventReactions();

  bool IsContinuous() const;

  WWorld* GetWorld() const { return m_pWorld; }
  WParticleWorldModule* GetOwnerWorldModule() const { return m_pOwnerModule; }

  const WParticleEffectResourceHandle& GetResource() const { return m_hResource; }

  const WHybridArray<WParticleSystemInstance*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

  void AddParticleEvent(const WParticleEvent& pe);

  WRandom& GetRNG() { return m_Random; }

  WUInt64 GetRandomSeed() const { return m_uiRandomSeed; }

  void RequestWindSamples();
  void UpdateWindSamples(WTime diff);

  /// Called by a behavior to signal that this effect needs attractor sampling each frame.
  ///
  /// \a uiMaxAttractors limits how many of the closest attractors are tracked.
  void RequestAttractorSamples(WUInt8 uiMaxAttractors);

  /// Refreshes the nearby attractor list and reads live properties.
  ///
  /// Must be called from the main thread before particle tasks are dispatched. The spatial
  /// query (which attractors exist) is repeated ~once per second; position and properties
  /// are resolved from stored handles every frame so moving attractors stay accurate.
  void FindNearbyAttractors(WTime diff);

  /// Returns the pre-computed attractor data, valid for use on worker threads during the current frame.
  ///
  /// Reads from the slot opposite to the one currently being written by the main thread,
  /// matching the double-buffer scheme used by the wind sampling system.
  WArrayPtr<const WParticleAttractorData> GetAttractorData() const;

  /// Returns the number of currently active particles across all systems.
  WUInt64 GetNumActiveParticles() const;

  /// @name Transform Related
  /// @{
public:
  /// Whether the effect is simulated around the origin and thus not affected by instance position and rotation
  bool IsSimulatedInLocalSpace() const { return m_bSimulateInLocalSpace; }

  /// Sets the transformation of this instance
  void SetTransform(const WTransform& transform, const WVec3& vParticleStartVelocity);

  /// Sets the transformation of this instance that should be used next frame.
  /// This function is typically used to set the transformation while the particle simulation is running to prevent race conditions.
  void SetTransformForNextFrame(const WTransform& transform, const WVec3& vParticleStartVelocity);

  /// Returns the transform of the main or shared instance.
  const WTransform& GetTransform() const { return m_Transform; }

  /// For the renderer to know whether the instance transform has to be applied to each particle position.
  bool NeedsToApplyTransform() const { return m_bSimulateInLocalSpace || m_bIsSharedEffect; }

  /// Returns the wind at the given position.
  ///
  /// Returns a zero vector, if no wind value is available (invalid index).
  WSimdVec4f GetWindAt(const WSimdVec4f& vPosition) const;

private:
  void PassTransformToSystems();

  WTransform m_Transform;
  WTransform m_TransformForNextFrame;

  WVec3 m_vVelocity;
  WVec3 m_vVelocityForNextFrame;

  struct WindSampleGrid
  {
    WSimdVec4f m_vMinPos;
    WSimdVec4f m_vMaxPos;
    WSimdVec4f m_vInvCellSize;
    WSmallArray<WSimdVec4f, 16, WAlignedAllocatorWrapper> m_Samples;
  };

  WVec4I32 m_vNumWindSamples; // not unsigned so it can be loaded directly to a SIMD register, w contains total num samples
  mutable WUniquePtr<WindSampleGrid> m_WindSampleGrids[2];

  WUInt8 m_uiAttractorSearchTimer = 0;
  WUInt8 m_uiMaxAttractors = 0;
  WSmallArray<WComponentHandle, 1> m_AttractorHandles;
  WSmallArray<WParticleAttractorData, 1> m_AttractorData[2];

  /// @}
  /// @name Updates
  /// @{

public:
  /// Returns false when the effect is finished.
  bool Update(const WTime& diff);

  /// Returns the total (game) time that the effect is alive and has been updated.
  ///
  /// Use this time, instead of a world clock, for time-dependent calculations. It is mostly tied to the world clock (game update),
  /// but additionally includes pre-simulation timings, which would otherwise be left out which can break some calculations.
  WTime GetTotalEffectLifeTime() const { return m_TotalEffectLifeTime; }

private: // friend WParticleWorldModule
  /// Whether this instance is in a state where its update task should be run
  bool ShouldBeUpdated() const;

  /// Returns the task that is used to update the effect
  const WSharedPtr<WTask>& GetUpdateTask() { return m_pTask; }

private: // friend WParticleEffectUpdateTask
  friend class WParticleEffectController;
  /// If the effect wants to skip all the initial behavior, this simulates it multiple times before it is shown the first time.
  void PreSimulate();

  /// Applies a given time step, without any restrictions.
  bool StepSimulation(const WTime& tDiff);

private:
  WTime m_TotalEffectLifeTime = WTime::MakeZero();
  WTime m_ElapsedTimeSinceUpdate = WTime::MakeZero();


  /// @}
  /// @name Shared Instances
  /// @{
public:
  /// Returns true, if this effect is configured to be simulated once per frame, but rendered by multiple instances.
  bool IsSharedEffect() const { return m_bIsSharedEffect; }

private: // friend WParticleWorldModule
  void AddSharedInstance(const void* pSharedInstanceOwner);
  void RemoveSharedInstance(const void* pSharedInstanceOwner);

private:
  bool m_bIsSharedEffect = false;

  /// @}
  /// \name Visibility and Culling
  /// @{
public:
  /// Marks this effect as visible from at least one view.
  /// This affects simulation update rates.
  void SetIsVisible() const;

  void SetVisibleIf(WParticleEffectInstance* pOtherVisible);

  /// Whether the effect has been marked as visible recently.
  bool IsVisible() const;

  /// Returns the bounding volume of the effect.
  /// The volume is in the local space of the effect.
  void GetBoundingVolume(WBoundingBoxSphere& ref_volume) const;

private:
  void CombineSystemBoundingVolumes();

  WBoundingBoxSphere m_BoundingVolume;
  mutable WTime m_EffectIsVisible;
  WParticleEffectInstance* m_pVisibleIf = nullptr;
  WEnum<WEffectInvisibleUpdateRate> m_InvisibleUpdateRate;
  WUInt64 m_uiRandomSeed = 0;

  /// @}
  /// \name Effect Parameters
  /// @{
public:
  void SetParameter(const WTempHashedString& sName, float value);
  void SetParameter(const WTempHashedString& sName, const WColor& value);

  WInt32 FindFloatParameter(const WTempHashedString& sName) const;
  float GetFloatParameter(const WTempHashedString& sName, float fDefaultValue) const;
  float GetFloatParameter(WUInt32 uiIdx) const { return m_FloatParameters[uiIdx].m_fValue; }

  WInt32 FindColorParameter(const WTempHashedString& sName) const;
  const WColor& GetColorParameter(const WTempHashedString& sName, const WColor& defaultValue) const;
  const WColor& GetColorParameter(WUInt32 uiIdx) const { return m_ColorParameters[uiIdx].m_Value; }


private:
  struct FloatParameter
  {
    W_DECLARE_POD_TYPE();
    WUInt64 m_uiNameHash;
    float m_fValue;
  };

  struct ColorParameter
  {
    W_DECLARE_POD_TYPE();
    WUInt64 m_uiNameHash;
    WColor m_Value;
  };

  WHybridArray<FloatParameter, 2> m_FloatParameters;
  WHybridArray<ColorParameter, 2> m_ColorParameters;

  /// @}


private:
  void Reconfigure(bool bFirstTime, WArrayPtr<WParticleEffectFloatParam> floatParams, WArrayPtr<WParticleEffectColorParam> colorParams);
  void ClearParticleSystem(WUInt32 index);
  void ProcessEventQueues();

  // for deterministic randomness
  WRandom m_Random;

  WHashSet<const void*> m_SharedInstances;
  WParticleEffectHandle m_hEffectHandle;
  bool m_bEmitterEnabled = true;
  bool m_bSimulateInLocalSpace = false;
  bool m_bIsFinishing = false;
  WUInt8 m_uiReviveTimeout = 3;
  WInt8 m_iMinSimStepsToDo = 0;
  float m_fApplyInstanceVelocity = 0;
  WTime m_PreSimulateDuration;
  WParticleEffectResourceHandle m_hResource;

  WParticleWorldModule* m_pOwnerModule = nullptr;
  WWorld* m_pWorld = nullptr;
  WHybridArray<WParticleSystemInstance*, 4> m_ParticleSystems;
  WHybridArray<WParticleEventReaction*, 4> m_EventReactions;

  WSharedPtr<WTask> m_pTask;

  WStaticArray<WParticleEvent, 16> m_EventQueue;
};
