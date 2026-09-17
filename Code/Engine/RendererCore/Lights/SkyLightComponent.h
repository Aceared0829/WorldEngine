#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Textures/TextureCubeResource.h>

struct WMsgUpdateLocalBounds;
struct WMsgExtractRenderData;
struct WMsgTransformChanged;

using WSkyLightComponentManager = WSettingsComponentManager<class WSkyLightComponent>;

class W_RENDERERCORE_DLL WSkyLightComponent : public WSettingsComponent
{
  W_DECLARE_COMPONENT_TYPE(WSkyLightComponent, WSettingsComponent, WSkyLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // WSkyLightComponent

public:
  WSkyLightComponent();
  ~WSkyLightComponent();

  void SetReflectionProbeMode(WEnum<WReflectionProbeMode> mode); // [ property ]
  WEnum<WReflectionProbeMode> GetReflectionProbeMode() const;    // [ property ]

  void SetDiffuseIntensity(float fIntensity);                      // [ property ]
  float GetDiffuseIntensity() const;                               // [ property ]

  void SetDiffuseSaturation(float fSaturation);                    // [ property ]
  float GetDiffuseSaturation() const;                              // [ property ]

  void SetSpecularIntensity(float fIntensity);                     // [ property ]
  float GetSpecularIntensity() const;                              // [ property ]

  const WTagSet& GetIncludeTags() const;                          // [ property ]
  void InsertIncludeTag(const char* szTag);                        // [ property ]
  void RemoveIncludeTag(const char* szTag);                        // [ property ]

  const WTagSet& GetExcludeTags() const;                          // [ property ]
  void InsertExcludeTag(const char* szTag);                        // [ property ]
  void RemoveExcludeTag(const char* szTag);                        // [ property ]

  void SetShowDebugInfo(bool bShowDebugInfo);                      // [ property ]
  bool GetShowDebugInfo() const;                                   // [ property ]

  void SetShowMipMaps(bool bShowMipMaps);                          // [ property ]
  bool GetShowMipMaps() const;                                     // [ property ]

  void SetCubeMapFile(WStringView sFile);                         // [ property ]
  WStringView GetCubeMapFile() const;                             // [ property ]

  WTextureCubeResourceHandle GetCubeMap() const
  {
    return m_hCubeMap;
  }

  float GetNearPlane() const { return m_Desc.m_fNearPlane; } // [ property ]
  void SetNearPlane(float fNearPlane);                       // [ property ]

  float GetFarPlane() const { return m_Desc.m_fFarPlane; }   // [ property ]
  void SetFarPlane(float fFarPlane);                         // [ property ]

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;
  void OnTransformChanged(WMsgTransformChanged& msg);

  WReflectionProbeDesc m_Desc;
  WTextureCubeResourceHandle m_hCubeMap;

  WReflectionProbeId m_Id;

  mutable bool m_bStatesDirty = true;
};
