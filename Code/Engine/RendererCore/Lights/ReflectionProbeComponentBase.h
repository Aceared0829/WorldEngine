#pragma once

#include <Core/World/Component.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

struct WMsgUpdateLocalBounds;
struct WMsgExtractRenderData;
struct WMsgTransformChanged;
class WAbstractObjectNode;

/// Base class for all reflection probes.
class W_RENDERERCORE_DLL WReflectionProbeComponentBase : public WComponent
{
  W_ADD_DYNAMIC_REFLECTION(WReflectionProbeComponentBase, WComponent);

public:
  WReflectionProbeComponentBase();
  ~WReflectionProbeComponentBase();

  void SetReflectionProbeMode(WEnum<WReflectionProbeMode> mode);           // [ property ]
  WEnum<WReflectionProbeMode> GetReflectionProbeMode() const;              // [ property ]

  const WTagSet& GetIncludeTags() const;                                    // [ property ]
  void InsertIncludeTag(const char* szTag);                                  // [ property ]
  void RemoveIncludeTag(const char* szTag);                                  // [ property ]

  const WTagSet& GetExcludeTags() const;                                    // [ property ]
  void InsertExcludeTag(const char* szTag);                                  // [ property ]
  void RemoveExcludeTag(const char* szTag);                                  // [ property ]

  float GetNearPlane() const { return m_Desc.m_fNearPlane; }                 // [ property ]
  void SetNearPlane(float fNearPlane);                                       // [ property ]

  float GetFarPlane() const { return m_Desc.m_fFarPlane; }                   // [ property ]
  void SetFarPlane(float fFarPlane);                                         // [ property ]

  const WVec3& GetCaptureOffset() const { return m_Desc.m_vCaptureOffset; } // [ property ]
  void SetCaptureOffset(const WVec3& vOffset);                              // [ property ]

  void SetShowDebugInfo(bool bShowDebugInfo);                                // [ property ]
  bool GetShowDebugInfo() const;                                             // [ property ]

  void SetShowMipMaps(bool bShowMipMaps);                                    // [ property ]
  bool GetShowMipMaps() const;                                               // [ property ]

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  float ComputePriority(WMsgExtractRenderData& msg, WReflectionProbeRenderData* pRenderData, float fVolume, const WVec3& vScale) const;

protected:
  WReflectionProbeDesc m_Desc;

  WReflectionProbeId m_Id;
  // Set to true if a change was made that requires recomputing the cube map.
  mutable bool m_bStatesDirty = true;
};
