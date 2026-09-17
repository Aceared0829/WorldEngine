#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Types/TagSet.h>
#include <TerrainPlugin/TerrainPluginDLL.h>
#include <TerrainPlugin/TerrainSystem.h>

struct WMsgTransformChanged;
struct WMsgSplineChanged;

/// Abstract base for all terrain brush components.
///
/// Handles creation and cleanup of TerrainSystem brushes, including automatic spline-path support:
/// if an WSplineComponent exists on the same game object the brush stamps along the spline instead of
/// acting as a single brush. Properties common to 2D and 3D brushes are declared here and inherited by
/// both WTerrainBrush2DComponent and WTerrainBrush3DComponent.
class W_TERRAINPLUGIN_DLL WTerrainBrushBaseComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WTerrainBrushBaseComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WTerrainBrushBaseComponent

public:
  WTerrainBrushBaseComponent();
  ~WTerrainBrushBaseComponent();

  void SetHalfSizeX(float fSize);                                //< [ property ]
  float GetHalfSizeX() const { return m_fHalfSizeX; }            //< [ property ]

  void SetInnerRadius(float fRadius);                            //< [ property ]
  float GetInnerRadius() const { return m_fInnerRadius; }        //< [ property ]

  void SetOuterRadius(float fRadius);                            //< [ property ]
  float GetOuterRadius() const { return m_fOuterRadius; }        //< [ property ]

  void SetFalloff(float fFalloff);                               //< [ property ]
  float GetFalloff() const { return m_fFalloff; }                //< [ property ]

  void SetMaterialIndex(WUInt8 uiIndex);                        //< [ property ]
  WUInt8 GetMaterialIndex() const { return m_uiMaterialIndex; } //< [ property ]

  /// Blend strength for material painting in [0, 1]. 0 = disabled.
  void SetMaterialStrength(float fStrength);                        //< [ property ]
  float GetMaterialStrength() const { return m_fMaterialStrength; } //< [ property ]

  void SetAffectPatches(bool b);                                    //< [ property ]
  bool GetAffectPatches() const { return m_bAffectPatches; }        //< [ property ]

  void SetAffectVolumes(bool b);                                    //< [ property ]
  bool GetAffectVolumes() const { return m_bAffectVolumes; }        //< [ property ]

  void SetNoiseStrength(float fNoise);                              //< [ property ]
  float GetNoiseStrength() const { return m_fNoiseStrength; }       //< [ property ]

  void SetNoiseFrequency(float fNoise);                             //< [ property ]
  float GetNoiseFrequency() const { return m_fNoiseFrequency; }     //< [ property ]

  /// Brush application order. Higher = applied later = wins. Equal priorities use mode-based ordering.
  void SetPriority(WInt8 iPriority);                //< [ property ]
  WInt8 GetPriority() const { return m_iPriority; } //< [ property ]

  const WTagSet& GetTags() const { return m_Tags; } //< [ property ]
  void Reflection_SetTag(const char* szTagName);     //< [ property ]
  void Reflection_RemoveTag(const char* szTagName);  //< [ property ]

protected:
  void OnMsgTransformChanged(WMsgTransformChanged& msg);
  void OnMsgSplineChanged(WMsgSplineChanged& msg);

  void RefreshBrushes();
  void ClearBrushes();

  /// Fills transform, HalfSizeX, and all common properties, then calls FillBrushSpecificProperties.
  void FillBrush(WTerrainData_Brush& brush, const WTransform& transform, float fHalfSizeX);

  /// Subclass fills: ModifyMode, m_vHalfExtents.y, m_fHalfExtentYTop, m_fHalfExtentZ.
  virtual void FillBrushSpecificProperties(WTerrainData_Brush& brush, float fHalfSizeX) = 0;

  WSmallArray<WUInt32, 1> m_BrushIndices;

  float m_fHalfSizeX = 0.0f;
  float m_fInnerRadius = 0.0f;
  float m_fOuterRadius = 5.0f;
  float m_fFalloff = 1.0f;
  float m_fNoiseStrength = 0.0f;
  float m_fNoiseFrequency = 1.0f;
  float m_fMaterialStrength = 0.0f;
  WUInt8 m_uiMaterialIndex = 0;
  WInt8 m_iPriority = 0;
  bool m_bAffectPatches = true;
  bool m_bAffectVolumes = true;
  /// If non-empty, brush only affects terrain objects that have at least one matching tag.
  WTagSet m_Tags;
};
