#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class WWorld;
class WParticleSystemDescriptor;
class WParticleEventReactionFactory;
class WParticleEventReaction;
class WParticleEmitter;
class WParticleInitializer;
class WParticleBehavior;
class WParticleType;
class WProcessingStreamGroup;
class WProcessingStream;
class WRandom;
struct WParticleEvent;
class WParticleEffectDescriptor;
class WParticleWorldModule;
class WParticleEffectInstance;
class WParticleSystemInstance;
class WRenderViewContext;
class WRenderPipelinePass;
class WParticleFinalizer;
class WParticleFinalizerFactory;

using WParticleEffectResourceHandle = WTypedResourceHandle<class WParticleEffectResource>;

using WParticleEffectId = WGenericId<22, 10>;

/// Handle to a particle effect instance
class W_PARTICLEPLUGIN_DLL WParticleEffectHandle
{
  W_DECLARE_HANDLE_TYPE(WParticleEffectHandle, WParticleEffectId);
};


/// Current state of a particle system
struct W_PARTICLEPLUGIN_DLL WParticleSystemState
{
  enum Enum
  {
    Active,           ///< System is actively emitting and processing particles
    EmittersFinished, ///< Emitters stopped, particles still alive
    OnlyReacting,     ///< Only reacting to events, otherwise finished
    Inactive,         ///< System is inactive
  };
};

class W_PARTICLEPLUGIN_DLL WParticleStreamBinding
{
public:
  void UpdateBindings(const WProcessingStreamGroup* pGroup) const;
  void Clear() { m_Bindings.Clear(); }

private:
  friend class WParticleSystemInstance;

  struct Binding
  {
    WHashedString m_sName;
    WProcessingStream** m_ppStream;
  };

  WSmallArray<Binding, 4> m_Bindings;
};

//////////////////////////////////////////////////////////////////////////

/// Blending mode for particle rendering
struct W_PARTICLEPLUGIN_DLL WParticleTypeRenderMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Additive,          ///< Additive blending
    Blended,           ///< Alpha blending
    Opaque,            ///< Opaque rendering
    Unused,
    BlendedBackground, ///< Alpha blending in background pass
    BlendedForeground, ///< Alpha blending in foreground pass
    Unused2,
    Default = Additive
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WParticleTypeRenderMode);

//////////////////////////////////////////////////////////////////////////

/// Lighting mode for particles
struct W_PARTICLEPLUGIN_DLL WParticleLightingMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Fullbright, ///< No lighting applied
    VertexLit,  ///< Simple vertex lighting
    Default = Fullbright
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WParticleLightingMode);

//////////////////////////////////////////////////////////////////////////

/// Update rate for effects that are not visible
struct W_PARTICLEPLUGIN_DLL WEffectInvisibleUpdateRate
{
  using StorageType = WUInt8;

  enum Enum
  {
    FullUpdate, ///< Continue updating at full rate
    Max20fps,   ///< Update at most 20 times per second
    Max10fps,   ///< Update at most 10 times per second
    Max5fps,    ///< Update at most 5 times per second
    Pause,      ///< Pause simulation completely
    Discard,    ///< Delete the effect

    Default = Max10fps
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WEffectInvisibleUpdateRate);

//////////////////////////////////////////////////////////////////////////

/// How to use texture atlas for particle sprites
struct W_PARTICLEPLUGIN_DLL WParticleTextureAtlasType
{
  using StorageType = WUInt8;

  enum Enum
  {
    None,              ///< No atlas, use full texture

    RandomVariations,  ///< Pick random tile for variation
    FlipbookAnimation, ///< Animate through tiles over lifetime
    RandomYAnimatedX,  ///< Random Y row, animate X over lifetime
    RandomXAnimatedY,  ///< Random X column, animate Y over lifetime

    Default = None
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WParticleTextureAtlasType);

//////////////////////////////////////////////////////////////////////////

/// Which edge of the source texture is treated as "forward" (i.e. unrotated)
struct W_PARTICLEPLUGIN_DLL WParticleTextureAtlasOrientation
{
  using StorageType = WUInt8;

  enum Enum
  {
    Up,    ///< Texture's authored "forward" edge is unrotated (current/default behavior)
    Right, ///< Cell content rotated 90° clockwise
    Down,  ///< Cell content rotated 180°
    Left,  ///< Cell content rotated 270° clockwise

    Default = Up
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WParticleTextureAtlasOrientation);

//////////////////////////////////////////////////////////////////////////

/// How to sample color gradients for particles
struct W_PARTICLEPLUGIN_DLL WParticleColorGradientMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Age,   ///< Sample based on normalized particle lifetime (0-1)
    Speed, ///< Sample based on particle speed

    Default = Age
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WParticleColorGradientMode);

//////////////////////////////////////////////////////////////////////////

/// Source of curve data
struct W_PARTICLEPLUGIN_DLL WCurveSource
{
  using StorageType = WUInt8;

  enum Enum
  {
    CustomCurve, ///< Use embedded curve data
    SharedCurve, ///< Reference shared curve resource

    Default = CustomCurve
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WCurveSource);

//////////////////////////////////////////////////////////////////////////

/// Source of gradient data
struct W_PARTICLEPLUGIN_DLL WGradientSource
{
  using StorageType = WUInt8;

  enum Enum
  {
    CustomGradient, ///< Use embedded gradient data
    SharedGradient, ///< Reference shared gradient resource

    Default = CustomGradient
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WGradientSource);

//////////////////////////////////////////////////////////////////////////

/// Action when particles leave bounds
struct W_PARTICLEPLUGIN_DLL WParticleOutOfBoundsMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Teleport, ///< Wrap particle to opposite side
    Die,      ///< Kill the particle

    Default = Teleport
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WParticleOutOfBoundsMode);

//////////////////////////////////////////////////////////////////////////

struct WParticleEffectFloatParam
{
  W_DECLARE_POD_TYPE();
  WHashedString m_sName;
  float m_Value;
};

struct WParticleEffectColorParam
{
  W_DECLARE_POD_TYPE();
  WHashedString m_sName;
  WColor m_Value;
};

class WParticleEffectParameters final : public WRefCounted
{
public:
  WHybridArray<WParticleEffectFloatParam, 2> m_FloatParams;
  WHybridArray<WParticleEffectColorParam, 2> m_ColorParams;
};
