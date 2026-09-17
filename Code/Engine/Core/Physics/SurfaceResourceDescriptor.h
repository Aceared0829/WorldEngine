#pragma once

#include <Core/CoreDLL.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/Variant.h>

using WSurfaceResourceHandle = WTypedResourceHandle<class WSurfaceResource>;
using WPrefabResourceHandle = WTypedResourceHandle<class WPrefabResource>;


/// Defines how prefabs are aligned when spawned during surface interactions.
struct WSurfaceInteractionAlignment
{
  using StorageType = WUInt8;

  enum Enum
  {
    SurfaceNormal,
    IncidentDirection,
    ReflectedDirection,
    ReverseSurfaceNormal,
    ReverseIncidentDirection,
    ReverseReflectedDirection,

    Default = SurfaceNormal
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WSurfaceInteractionAlignment);


/// Describes how a surface responds to a specific type of interaction.
///
/// Configures the prefab to spawn, its alignment, impact thresholds, and custom parameters
/// when objects interact with a surface in a particular way (collision, slide, roll, etc.).
struct W_CORE_DLL WSurfaceInteraction
{
  WString m_sInteractionType;

  WPrefabResourceHandle m_hPrefab;
  WEnum<WSurfaceInteractionAlignment> m_Alignment;
  WAngle m_Deviation;
  float m_fImpulseThreshold = 0.0f;
  float m_fImpulseScale = 1.0f;

  const WRangeView<const char*, WUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const WVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, WVariant& out_value) const; // [ property ] (exposed parameter)

  WArrayMap<WHashedString, WVariant> m_Parameters;
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WSurfaceInteraction);

/// Descriptor containing all configuration data for a surface resource.
///
/// Defines physics properties (restitution, friction), interaction behaviors,
/// base surface inheritance, and navigation ground type information.
struct W_CORE_DLL WSurfaceResourceDescriptor : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WSurfaceResourceDescriptor, WReflectedClass);

public:
  void Load(WStreamReader& inout_stream);
  void Save(WStreamWriter& inout_stream) const;

  void SetCollisionInteraction(const char* szName);
  const char* GetCollisionInteraction() const;

  void SetSlideReactionPrefabFile(const char* szFile);
  const char* GetSlideReactionPrefabFile() const;

  void SetRollReactionPrefabFile(const char* szFile);
  const char* GetRollReactionPrefabFile() const;

  WSurfaceResourceHandle m_hBaseSurface;
  float m_fPhysicsRestitution;
  float m_fPhysicsFrictionStatic;
  float m_fPhysicsFrictionDynamic;
  WHashedString m_sOnCollideInteraction;
  WHashedString m_sSlideInteractionPrefab;
  WHashedString m_sRollInteractionPrefab;
  WInt8 m_iGroundType = -1; ///< What kind of ground this is for navigation purposes. Ground type properties need to be specified elsewhere, this is just a number.

  /// Color to use when visualizing which surface is assigned to which geometry (see the CVar 'Jolt.Visualize.Surfaces').
  /// Has no effect on anything but debug visualizations.
  WColorGammaUB m_DebugColor = WColor::White;

  WHybridArray<WSurfaceInteraction, 16> m_Interactions;
};
