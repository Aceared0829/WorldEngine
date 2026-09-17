#pragma once

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

class WRandomPrefabComponent;

class W_GAMECOMPONENTS_DLL WRandomPrefabComponentManager : public WComponentManager<WRandomPrefabComponent, WBlockStorageType::Compact>
{
public:
  WRandomPrefabComponentManager(WWorld* pWorld);
  ~WRandomPrefabComponentManager();

  virtual void Initialize() override;

  void Update(const WWorldModule::UpdateContext& context);
  void AddToUpdateList(WRandomPrefabComponent* pComponent);

private:
  void ResourceEventHandler(const WResourceEvent& e);

  WHashSet<WComponentHandle> m_ComponentsToUpdate;
};

//////////////////////////////////////////////////////////////////////////

/// Spawns one or multiple prefabs randomly from a collection.
///
/// The component stores a list of prefabs. Upon activation it randomly selects one or multiple to spawn.
/// The location, rotation, size and color may vary within specified limits.
///
/// The randomness is deterministic for each object, but different objects produce different results.
class W_GAMECOMPONENTS_DLL WRandomPrefabComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WRandomPrefabComponent, WComponent, WRandomPrefabComponentManager);

public:
  WRandomPrefabComponent();
  ~WRandomPrefabComponent();

  //////////////////////////////////////////////////////////////////////////
  // WComponent

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WRandomPrefabComponent

public:
  /// Specifies how many objects to spawn.
  void SetCount(WUInt16 uiCount);
  WUInt16 GetCount() const;

  /// Specifies how far along the x, y and z axis the spawned object positions may deviate from the center.
  void SetPositionDeviation(const WVec3& vValue);
  const WVec3& GetPositionDeviation() const { return m_vPositionDeviation; }

  /// Specifies how much the spawned object rotations may deviate.
  void SetRotationDeviation(const WVec3& vValue);
  const WVec3& GetRotationDeviation() const { return m_vRotationDeviation; }

  /// Sets the minimum scale for the spawned objects.
  void SetMinUniformScale(float fValue);
  float GetMinUniformScale() const { return m_fMinUniformScale; }

  /// Sets the maximum scale for the spawned objects.
  void SetMaxUniformScale(float fValue);
  float GetMaxUniformScale() const { return m_fMaxUniformScale; }

  /// If color1 and/or color2 are not white, objects are sent an WMsgSetColor, with a color that is a random interpolation between the two.
  void SetColor1(const WColor& value);
  const WColor& GetColor1() const { return m_Color1; }

  /// If color1 and/or color2 are not white, objects are sent an WMsgSetColor, with a color that is a random interpolation between the two.
  void SetColor2(const WColor& value);
  const WColor& GetColor2() const { return m_Color2; }

  /// If set to true, the spawned objects get attached to the owner of this component. Otherwise they don't have a parent.
  void SetInstantiateAsChildren(bool bValue);
  bool GetInstantiateAsChildren() const { return m_bInstantiateAsChildren; }

  /// Whether to preview the result in editor. Can be disabled to reduce clutter or improve performance during editing.
  void SetPreview(bool bValue);
  bool GetPreview() const { return m_bPreview; }

private:
  void ClearCreatedInstances();
  void InstantiatePrefabs();

  WUInt32 Prefabs_GetCount() const;
  WString Prefabs_GetValue(WUInt32 uiIndex) const;
  void Prefabs_SetValue(WUInt32 uiIndex, WString sValue);
  void Prefabs_Insert(WUInt32 uiIndex, WString sValue);
  void Prefabs_Remove(WUInt32 uiIndex);

  WUInt16 m_uiCount = 1;
  bool m_bInstantiateAsChildren = false;
  bool m_bPreview = true;

  WVec3 m_vPositionDeviation = WVec3(0);
  WVec3 m_vRotationDeviation = WVec3(0);
  float m_fMinUniformScale = 1.0f;
  float m_fMaxUniformScale = 1.0f;

  WColor m_Color1 = WColor::White;
  WColor m_Color2 = WColor::White;

  WHybridArray<WPrefabResourceHandle, 4> m_Prefabs;
};
