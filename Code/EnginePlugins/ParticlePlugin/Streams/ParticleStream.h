#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class WParticleStream;
class WParticleSystemInstance;

/// Base class for all particle stream factories
///
/// Stream factories are responsible for creating and configuring particle streams.
/// Each factory specifies the stream name, data type, and the actual stream class to instantiate.
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory, WReflectedClass);

public:
  WParticleStreamFactory(const char* szStreamName, WProcessingStream::DataType dataType, const WRTTI* pStreamTypeToCreate);

  const WRTTI* GetParticleStreamType() const;
  WProcessingStream::DataType GetStreamDataType() const;
  const char* GetStreamName() const;

  /// Creates and initializes a new particle stream instance for the given particle system.
  WParticleStream* CreateParticleStream(WParticleSystemInstance* pOwner) const;

private:
  const char* m_szStreamName = nullptr;
  WProcessingStream::DataType m_DataType = WProcessingStream::DataType::Float;
  const WRTTI* m_pStreamTypeToCreate = nullptr;
};

/// Base class for all particle streams
///
/// Particle streams store per-particle data like position, velocity, color, or size.
/// Each stream type provides initialization logic for new particles.
/// Streams run with high priority (-1000) to ensure they initialize data before other processors.
class W_PARTICLEPLUGIN_DLL WParticleStream : public WProcessingStreamProcessor
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStream, WProcessingStreamProcessor);

  friend class WParticleSystemInstance;
  friend class WParticleStreamFactory;

protected:
  WParticleStream();

  /// Called once during stream creation to set up any necessary references or state.
  virtual void Initialize(WParticleSystemInstance* pOwner) {}

  virtual WResult UpdateStreamBindings() final override;

  /// Particle streams do not process existing elements, they only initialize new ones.
  virtual void Process(WUInt64 uiNumElements) final override {}

  /// The default implementation initializes all data with zero.
  ///
  /// Override this to provide custom initialization for new particles.
  /// The implementation should initialize elements in the range [uiStartIndex, uiStartIndex + uiNumElements).
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;

  WProcessingStream* m_pStream; ///< The underlying data stream managed by this particle stream

private:
  WParticleStreamBinding m_StreamBinding;
};
