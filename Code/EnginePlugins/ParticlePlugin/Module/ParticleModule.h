#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class WProcessingStream;

/// Base class for all particle system modules
///
/// Modules process particle data through streams.
/// Derived types include emitters, initializers, behaviors, and finalizers.
class W_PARTICLEPLUGIN_DLL WParticleModule : public WProcessingStreamProcessor
{
  W_ADD_DYNAMIC_REFLECTION(WParticleModule, WProcessingStreamProcessor);

  friend class WParticleSystemInstance;

public:
  virtual void CreateRequiredStreams() = 0;
  virtual void QueryOptionalStreams() {}

  void Reset(WParticleSystemInstance* pOwner)
  {
    m_pOwnerSystem = pOwner;
    m_StreamBinding.Clear();

    OnReset();
  }

  /// Called after everything is set up.
  virtual void OnFinalize() {}

  WParticleSystemInstance* GetOwnerSystem() { return m_pOwnerSystem; }

  const WParticleSystemInstance* GetOwnerSystem() const { return m_pOwnerSystem; }

  WParticleEffectInstance* GetOwnerEffect() const { return m_pOwnerSystem->GetOwnerEffect(); }

  /// Override this to cache world module pointers for later access.
  ///
  /// Cached modules can be retrieved via WParticleWorldModule::GetCachedWorldModule().
  virtual void RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule) {}

protected:
  /// Called by Reset() to perform custom cleanup
  virtual void OnReset() {}

  void CreateStream(const char* szName, WProcessingStream::DataType Type, WProcessingStream** ppStream, bool bWillInitializeStream)
  {
    m_pOwnerSystem->CreateStream(szName, Type, ppStream, m_StreamBinding, bWillInitializeStream);
  }

  virtual WResult UpdateStreamBindings() final override
  {
    m_StreamBinding.UpdateBindings(m_pStreamGroup);
    return W_SUCCESS;
  }

  WRandom& GetRNG() const { return GetOwnerEffect()->GetRNG(); }

private:
  WParticleSystemInstance* m_pOwnerSystem;
  WParticleStreamBinding m_StreamBinding;
};
