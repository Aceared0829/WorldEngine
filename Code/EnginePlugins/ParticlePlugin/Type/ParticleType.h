#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct WMsgExtractRenderData;

/// Sorting key values used to order particles during rendering.
enum WParticleTypeSortingKey
{
  Opaque,
  BlendedBackground,
  Additive,
  Blended,
  BlendedForeground,
};

/// Factory for creating particle type instances.
///
/// Each particle type factory stores the configuration for a particle type
/// and can create instances of that type for particle system instances.
class W_PARTICLEPLUGIN_DLL WParticleTypeFactory : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypeFactory, WReflectedClass);

public:
  virtual const WRTTI* GetTypeType() const = 0;
  virtual void CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const = 0;

  WParticleType* CreateType(WParticleSystemInstance* pOwner) const;

  /// Allows the type to register any finalizers it depends on.
  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const {}

  virtual void Save(WStreamWriter& inout_stream) const = 0;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) = 0;
};

/// Base class for particle types that define how particles are rendered.
///
/// Each particle type handles a specific rendering method such as billboards,
/// trails, meshes, or lights. Types process particle data each frame and
/// generate render data for the renderer.
class W_PARTICLEPLUGIN_DLL WParticleType : public WParticleModule
{
  W_ADD_DYNAMIC_REFLECTION(WParticleType, WParticleModule);

  friend class WParticleSystemInstance;

public:
  /// Returns the maximum radius a particle can occupy for culling purposes.
  ///
  /// Used to compute bounding volumes for frustum culling. The default implementation
  /// returns half the particle size, assuming spherical particles.
  virtual float GetMaxParticleRadius(float fParticleSize) const { return fParticleSize * 0.5f; }

  /// Generates render data for all active particles.
  ///
  /// Called during render data extraction to create render objects for this particle type.
  virtual void ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const = 0;

protected:
  WParticleType();

  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const WTime& tDiff, WUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  static WUInt32 ComputeSortingKey(WParticleTypeRenderMode::Enum mode, WUInt64 uiResource1Hash, WUInt64 uiResource2Hash);

  WTime m_TimeDiff;
  mutable WUInt64 m_uiLastExtractedFrame;
};
