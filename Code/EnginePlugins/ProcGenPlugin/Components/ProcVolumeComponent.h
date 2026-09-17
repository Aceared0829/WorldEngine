#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <ProcGenPlugin/Declarations.h>

struct WMsgTransformChanged;
struct WMsgUpdateLocalBounds;
struct WMsgExtractVolumes;

using WImageDataResourceHandle = WTypedResourceHandle<class WImageDataResource>;

class W_PROCGENPLUGIN_DLL WProcVolumeComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WProcVolumeComponent, WComponent);

public:
  WProcVolumeComponent();
  ~WProcVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetValue(float fValue);
  float GetValue() const { return m_fValue; }

  void SetSortOrder(float fOrder);
  float GetSortOrder() const { return m_fSortOrder; }

  void SetBlendMode(WEnum<WProcGenBlendMode> blendMode);
  WEnum<WProcGenBlendMode> GetBlendMode() const { return m_BlendMode; }

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  void OnTransformChanged(WMsgTransformChanged& ref_msg);

  using AreaInvalidatedEvent = WEvent<const WProcGenInternal::InvalidatedArea&, WMutex>;
  static const AreaInvalidatedEvent& GetAreaInvalidatedEvent() { return s_AreaInvalidatedEvent; }

protected:
  float m_fValue = 1.0f;
  float m_fSortOrder = 0.0f;
  WEnum<WProcGenBlendMode> m_BlendMode;

  void InvalidateArea();
  void InvalidateArea(const WBoundingBox& area);

  static AreaInvalidatedEvent s_AreaInvalidatedEvent;
  static WSpatialData::Category s_SpatialCategory;
};

//////////////////////////////////////////////////////////////////////////

using WProcVolumeSphereComponentManager = WComponentManager<class WProcVolumeSphereComponent, WBlockStorageType::Compact>;

class W_PROCGENPLUGIN_DLL WProcVolumeSphereComponent : public WProcVolumeComponent
{
  W_DECLARE_COMPONENT_TYPE(WProcVolumeSphereComponent, WProcVolumeComponent, WProcVolumeSphereComponentManager);

public:
  WProcVolumeSphereComponent();
  ~WProcVolumeSphereComponent();

  float GetRadius() const { return m_fRadius; }
  void SetRadius(float fRadius);

  float GetFalloff() const { return m_fFalloff; }
  void SetFalloff(float fFalloff);

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg) const;
  void OnExtractVolumes(WMsgExtractVolumes& ref_msg) const;

protected:
  float m_fRadius = 5.0f;
  float m_fFalloff = 0.5f;
};

//////////////////////////////////////////////////////////////////////////

using WProcVolumeBoxComponentManager = WComponentManager<class WProcVolumeBoxComponent, WBlockStorageType::Compact>;

class W_PROCGENPLUGIN_DLL WProcVolumeBoxComponent : public WProcVolumeComponent
{
  W_DECLARE_COMPONENT_TYPE(WProcVolumeBoxComponent, WProcVolumeComponent, WProcVolumeBoxComponentManager);

public:
  WProcVolumeBoxComponent();
  ~WProcVolumeBoxComponent();

  const WVec3& GetExtents() const { return m_vExtents; }
  void SetExtents(const WVec3& vExtents);

  const WVec3& GetPositiveFalloff() const { return m_vPositiveFalloff; }
  void SetPositiveFalloff(const WVec3& vFalloff);
  const WVec3& GetNegativeFalloff() const { return m_vNegativeFalloff; }
  void SetNegativeFalloff(const WVec3& vFalloff);

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg) const;
  void OnExtractVolumes(WMsgExtractVolumes& ref_msg) const;

protected:
  WVec3 m_vExtents = WVec3(10.0f);
  WVec3 m_vPositiveFalloff = WVec3(0.5f);
  WVec3 m_vNegativeFalloff = WVec3(0.5f);
};

//////////////////////////////////////////////////////////////////////////

using WProcVolumeImageComponentManager = WComponentManager<class WProcVolumeImageComponent, WBlockStorageType::Compact>;

class W_PROCGENPLUGIN_DLL WProcVolumeImageComponent : public WProcVolumeBoxComponent
{
  W_DECLARE_COMPONENT_TYPE(WProcVolumeImageComponent, WProcVolumeBoxComponent, WProcVolumeImageComponentManager);

public:
  WProcVolumeImageComponent();
  ~WProcVolumeImageComponent();

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  void OnExtractVolumes(WMsgExtractVolumes& ref_msg) const;

  void SetImage(const WImageDataResourceHandle& hResource);
  WImageDataResourceHandle GetImage() const { return m_hImage; }

protected:
  WImageDataResourceHandle m_hImage; // [ property ]
};
