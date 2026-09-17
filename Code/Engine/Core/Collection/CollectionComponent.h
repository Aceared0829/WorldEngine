#pragma once

#include <Core/Collection/CollectionResource.h>
#include <Core/CoreDLL.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

using WCollectionComponentManager = WComponentManager<class WCollectionComponent, WBlockStorageType::Compact>;

/// An WCollectionComponent references an WCollectionResource and triggers resource preloading when needed
///
/// Placing an WCollectionComponent in a scene or a model makes it possible to tell the engine to preload certain resources
/// that are likely to be needed soon.
///
/// If a deactivated WCollectionComponent is part of the scene, it will not trigger a preload, but will do so once
/// the component is activated.
class W_CORE_DLL WCollectionComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WCollectionComponent, WComponent, WCollectionComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent
public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WCollectionComponent
public:
  WCollectionComponent();
  ~WCollectionComponent();

  void SetCollection(const WCollectionResourceHandle& hPrefab);                                     // [ property ]
  W_ALWAYS_INLINE const WCollectionResourceHandle& GetCollection() const { return m_hCollection; } // [ property ]

protected:
  /// Triggers the preload on the referenced WCollectionResource
  void InitiatePreload();

  bool m_bRegisterNames = false;
  WCollectionResourceHandle m_hCollection;
};
