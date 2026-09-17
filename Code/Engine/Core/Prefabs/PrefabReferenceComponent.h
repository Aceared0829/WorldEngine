#pragma once

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>

class WPrefabReferenceComponent;

class W_CORE_DLL WPrefabReferenceComponentManager : public WComponentManager<WPrefabReferenceComponent, WBlockStorageType::Compact>
{
public:
  WPrefabReferenceComponentManager(WWorld* pWorld);
  ~WPrefabReferenceComponentManager();

  virtual void Initialize() override;

  void Update(const WWorldModule::UpdateContext& context);
  void AddToUpdateList(WPrefabReferenceComponent* pComponent);

private:
  void ResourceEventHandler(const WResourceEvent& e);

  WDeque<WComponentHandle> m_ComponentsToUpdate;
};

/// The central component to instantiate prefabs.
///
/// This component instantiates a prefab and attaches the instantiated objects as children to this object.
/// The component is able to remove and recreate instantiated objects, which is needed at editing time.
/// Whenever the prefab resource changes, this component re-creates the instance.
///
/// It also holds prefab parameters, which are passed through during instantiation.
/// For that it also implements remapping of game object references, so that they can be passed into prefabs during instantiation.
class W_CORE_DLL WPrefabReferenceComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WPrefabReferenceComponent, WComponent, WPrefabReferenceComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // WPrefabReferenceComponent

public:
  WPrefabReferenceComponent();
  ~WPrefabReferenceComponent();

  void SetPrefab(const WPrefabResourceHandle& hPrefab);                                 // [ property ]
  W_ALWAYS_INLINE const WPrefabResourceHandle& GetPrefab() const { return m_hPrefab; } // [ property ]

  void SetShowShapeIcons(bool bShow);                                                    // [ property ]
  bool GetShowShapeIcons() const;                                                        // [ property ]

  const WRangeView<const char*, WUInt32> GetParameters() const;                        // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const WVariant& value);                          // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                                               // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, WVariant& out_value) const;                      // [ property ] (exposed parameter)

  static void SerializePrefabParameters(const WWorld& world, WWorldWriter& inout_stream, WArrayMap<WHashedString, WVariant> parameters);
  static void DeserializePrefabParameters(WArrayMap<WHashedString, WVariant>& out_parameters, WWorldReader& inout_stream);

private:
  void InstantiatePrefab();
  void ClearPreviousInstances();

  WPrefabResourceHandle m_hPrefab;
  WArrayMap<WHashedString, WVariant> m_Parameters;
};
