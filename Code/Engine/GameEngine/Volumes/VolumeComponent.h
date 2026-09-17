#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>
#include <GameEngine/GameEngineDLL.h>

struct WMsgUpdateLocalBounds;

using WBlackboardTemplateResourceHandle = WTypedResourceHandle<class WBlackboardTemplateResource>;

/// A volume component can hold generic values either from a blackboard template or set directly on the component.
///
/// The values can be sampled with an WVolumeSampler and then used for things like e.g. post-processing, reverb etc.
/// They can also be used to represent knowledge in a scene, like e.g. smell or threat, and can be detected by an WSensorComponent and then processed by AI.
class W_GAMEENGINE_DLL WVolumeComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WVolumeComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WVolumeComponent

public:
  WVolumeComponent();
  ~WVolumeComponent();

  /// Sets the blackboard template to use.
  void SetTemplate(const WBlackboardTemplateResourceHandle& hResource);                        // [ property ]
  const WBlackboardTemplateResourceHandle& GetTemplate() const { return m_hTemplateResource; } // [ property ]

  /// In case two volumes overlap, the one with a higher sort order value has precedence.
  void SetSortOrder(float fOrder);                    // [ property ]
  float GetSortOrder() const { return m_fSortOrder; } // [ property ]

  /// Sets the spatial category under which this volume can be detected.
  void SetVolumeType(const char* szType); // [ property ]
  const char* GetVolumeType() const;      // [ property ]

  /// Adds or replaces a value with a given name.
  void SetValue(const WHashedString& sName, const WVariant& value); // [ scriptable ]
  WVariant GetValue(WTempHashedString sName) const                  // [ scriptable ]
  {
    WVariant v;
    m_Values.TryGetValue(sName, v);
    return v;
  }

protected:
  const WRangeView<const WString&, WUInt32> Reflection_GetKeys() const;
  bool Reflection_GetValue(const char* szName, WVariant& value) const;
  void Reflection_InsertValue(const char* szName, const WVariant& value);
  void Reflection_RemoveValue(const char* szName);

  void InitializeFromTemplate();
  void ReloadTemplate();
  void RemoveReloadFunction();

  WBlackboardTemplateResourceHandle m_hTemplateResource;
  WHashTable<WHashedString, WVariant> m_Values;
  WSmallArray<WHashedString, 1> m_OverwrittenValues; // only used in editor
  float m_fSortOrder = 0.0f;
  WSpatialData::Category m_SpatialCategory = WInvalidSpatialDataCategory;
  bool m_bReloadFunctionAdded = false;
};

//////////////////////////////////////////////////////////////////////////

using WVolumeSphereComponentManager = WComponentManager<class WVolumeSphereComponent, WBlockStorageType::Compact>;

/// A sphere implementation of the WVolumeComponent
class W_GAMEENGINE_DLL WVolumeSphereComponent : public WVolumeComponent
{
  W_DECLARE_COMPONENT_TYPE(WVolumeSphereComponent, WVolumeComponent, WVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WVolumeComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WVolumeSphereComponent

public:
  WVolumeSphereComponent();
  ~WVolumeSphereComponent();

  void SetRadius(float fRadius);
  float GetRadius() const { return m_fRadius; }

  /// Values above 1 make the sphere influence drop off more rapidly, below 1 more slowly.
  void SetFalloff(float fFalloff);
  float GetFalloff() const { return m_fFalloff; }

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg) const;

  float m_fRadius = 5.0f;
  float m_fFalloff = 0.5f;
};

//////////////////////////////////////////////////////////////////////////

using WVolumeBoxComponentManager = WComponentManager<class WVolumeBoxComponent, WBlockStorageType::Compact>;

/// A box implementation of the WVolumeComponent
class W_GAMEENGINE_DLL WVolumeBoxComponent : public WVolumeComponent
{
  W_DECLARE_COMPONENT_TYPE(WVolumeBoxComponent, WVolumeComponent, WVolumeBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WVolumeComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WVolumeBoxComponent

public:
  WVolumeBoxComponent();
  ~WVolumeBoxComponent();

  /// Sets the size of the box.
  void SetExtents(const WVec3& vExtents);
  const WVec3& GetExtents() const { return m_vExtents; }

  /// Values above 1 make the box influence drop off more rapidly, below 1 more slowly.
  ///
  /// Falloff is per cardinal axis.
  void SetFalloff(const WVec3& vFalloff);
  const WVec3& GetFalloff() const { return m_vFalloff; }

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg) const;

  WVec3 m_vExtents = WVec3(10.0f);
  WVec3 m_vFalloff = WVec3(0.5f);
};
