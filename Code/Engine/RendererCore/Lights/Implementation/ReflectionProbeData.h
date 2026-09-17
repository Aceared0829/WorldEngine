#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/TagSet.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

struct WReflectionProbeMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Static,
    Dynamic,

    Default = Static
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WReflectionProbeMode);

/// Describes how a cube map should be generated.
struct W_RENDERERCORE_DLL WReflectionProbeDesc
{
  WUuid m_uniqueID;

  WTagSet m_IncludeTags;
  WTagSet m_ExcludeTags;

  WEnum<WReflectionProbeMode> m_Mode;

  bool m_bShowDebugInfo = false;
  bool m_bShowMipMaps = false;

  float m_fDiffuseIntensity = 0.4f;
  float m_fDiffuseSaturation = 0.3f;
  float m_fSpecularIntensity = 1.0f;
  float m_fNearPlane = 0.0f;
  float m_fFarPlane = 100.0f;
  WVec3 m_vCaptureOffset = WVec3::MakeZero();
};

using WReflectionProbeId = WGenericId<24, 8>;

template <>
struct WHashHelper<WReflectionProbeId>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WReflectionProbeId value) { return WHashHelper<WUInt32>::Hash(value.m_Data); }

  W_ALWAYS_INLINE static bool Equal(WReflectionProbeId a, WReflectionProbeId b) { return a == b; }
};

/// Render data for a reflection probe.
class W_RENDERERCORE_DLL WReflectionProbeRenderData : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WReflectionProbeRenderData, WRenderData);

public:
  WReflectionProbeRenderData()
  {
    m_Id.Invalidate();
    m_vHalfExtents.SetZero();
  }

  WTransform m_GlobalTransform;
  WReflectionProbeId m_Id;
  WUInt32 m_uiIndex = 0;
  WVec3 m_vHalfExtents;
  WVec3 m_vPositiveFalloff;
  WVec3 m_vNegativeFalloff;
  WVec3 m_vInfluenceScale;
  WVec3 m_vInfluenceShift;
};

/// A unique reference to a reflection probe.
struct WReflectionProbeRef
{
  bool operator==(const WReflectionProbeRef& b) const
  {
    return m_Id == b.m_Id && m_uiWorldIndex == b.m_uiWorldIndex;
  }

  WUInt32 m_uiWorldIndex = 0;
  WReflectionProbeId m_Id;
};
static_assert(sizeof(WReflectionProbeRef) == 8);

template <>
struct WHashHelper<WReflectionProbeRef>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WReflectionProbeRef value) { return WHashHelper<WUInt64>::Hash(reinterpret_cast<WUInt64&>(value)); }

  W_ALWAYS_INLINE static bool Equal(WReflectionProbeRef a, WReflectionProbeRef b) { return a.m_Id == b.m_Id && a.m_uiWorldIndex == b.m_uiWorldIndex; }
};

/// Flags that describe a reflection probe.
struct WProbeFlags
{
  using StorageType = WUInt8;

  enum Enum
  {
    SkyLight = W_BIT(0),
    HasCustomCubeMap = W_BIT(1),
    Sphere = W_BIT(2),
    Box = W_BIT(3),
    Dynamic = W_BIT(4),
    Default = 0
  };

  struct Bits
  {
    StorageType SkyLight : 1;
    StorageType HasCustomCubeMap : 1;
    StorageType Sphere : 1;
    StorageType Box : 1;
    StorageType Dynamic : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WProbeFlags);

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WProbeFlags);
