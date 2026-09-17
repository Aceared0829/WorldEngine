#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

// clang-format off

//////////////////////////////////////////////////////////////////////////

W_BEGIN_STATIC_REFLECTED_ENUM(WParticleTypeRenderMode, 1)
  W_ENUM_CONSTANT(WParticleTypeRenderMode::Opaque),
  W_ENUM_CONSTANT(WParticleTypeRenderMode::Additive),
  W_ENUM_CONSTANT(WParticleTypeRenderMode::Blended),
  W_ENUM_CONSTANT(WParticleTypeRenderMode::BlendedForeground),
  W_ENUM_CONSTANT(WParticleTypeRenderMode::BlendedBackground),
W_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

W_BEGIN_STATIC_REFLECTED_ENUM(WParticleLightingMode, 1)
  W_ENUM_CONSTANT(WParticleLightingMode::Fullbright),
  W_ENUM_CONSTANT(WParticleLightingMode::VertexLit),
W_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

W_BEGIN_STATIC_REFLECTED_ENUM(WEffectInvisibleUpdateRate, 1)
  W_ENUM_CONSTANT(WEffectInvisibleUpdateRate::FullUpdate),
  W_ENUM_CONSTANT(WEffectInvisibleUpdateRate::Max20fps),
  W_ENUM_CONSTANT(WEffectInvisibleUpdateRate::Max10fps),
  W_ENUM_CONSTANT(WEffectInvisibleUpdateRate::Max5fps),
  W_ENUM_CONSTANT(WEffectInvisibleUpdateRate::Pause),
  W_ENUM_CONSTANT(WEffectInvisibleUpdateRate::Discard),
W_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

W_BEGIN_STATIC_REFLECTED_ENUM(WParticleTextureAtlasType, 1)
  W_ENUM_CONSTANT(WParticleTextureAtlasType::None),
  W_ENUM_CONSTANT(WParticleTextureAtlasType::RandomVariations),
  W_ENUM_CONSTANT(WParticleTextureAtlasType::FlipbookAnimation),
  W_ENUM_CONSTANT(WParticleTextureAtlasType::RandomYAnimatedX),
  W_ENUM_CONSTANT(WParticleTextureAtlasType::RandomXAnimatedY),
W_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

W_BEGIN_STATIC_REFLECTED_ENUM(WParticleTextureAtlasOrientation, 1)
  W_ENUM_CONSTANT(WParticleTextureAtlasOrientation::Up),
  W_ENUM_CONSTANT(WParticleTextureAtlasOrientation::Right),
  W_ENUM_CONSTANT(WParticleTextureAtlasOrientation::Down),
  W_ENUM_CONSTANT(WParticleTextureAtlasOrientation::Left),
W_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

W_BEGIN_STATIC_REFLECTED_ENUM(WParticleColorGradientMode, 1)
  W_ENUM_CONSTANT(WParticleColorGradientMode::Age),
  W_ENUM_CONSTANT(WParticleColorGradientMode::Speed),
W_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

W_BEGIN_STATIC_REFLECTED_ENUM(WCurveSource, 1)
  W_ENUM_CONSTANT(WCurveSource::CustomCurve),
  W_ENUM_CONSTANT(WCurveSource::SharedCurve),
W_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

W_BEGIN_STATIC_REFLECTED_ENUM(WGradientSource, 1)
  W_ENUM_CONSTANT(WGradientSource::CustomGradient),
  W_ENUM_CONSTANT(WGradientSource::SharedGradient),
W_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

W_BEGIN_STATIC_REFLECTED_ENUM(WParticleOutOfBoundsMode, 1)
  W_ENUM_CONSTANT(WParticleOutOfBoundsMode::Teleport),
  W_ENUM_CONSTANT(WParticleOutOfBoundsMode::Die),
W_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

// clang-format on

W_STATICLINK_LIBRARY(ParticlePlugin)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Attract);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Bounds);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_BoundsSphere);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_ColorGradient);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Expression);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_FadeOut);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Flies);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Gravity);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Move);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Opacity);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_PullAlong);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Raycast);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_SizeCurve);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Turbulence);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Velocity);
  W_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Wind);
  W_STATICLINK_REFERENCE(ParticlePlugin_Components_ParticleAttractorComponent);
  W_STATICLINK_REFERENCE(ParticlePlugin_Components_ParticleComponent);
  W_STATICLINK_REFERENCE(ParticlePlugin_Components_ParticleFinisherComponent);
  W_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectDescriptor);
  W_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectInstance);
  W_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter);
  W_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Burst);
  W_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Continuous);
  W_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Distance);
  W_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_OnEvent);
  W_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEventReaction);
  W_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEventReaction_Effect);
  W_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEventReaction_Prefab);
  W_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer);
  W_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_Age);
  W_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_ApplyVelocity);
  W_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_LastPosition);
  W_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_Volume);
  W_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer);
  W_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_BoxPosition);
  W_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_CylinderPosition);
  W_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomColor);
  W_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomRotationSpeed);
  W_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomSize);
  W_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_SpherePosition);
  W_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_VelocityCone);
  W_STATICLINK_REFERENCE(ParticlePlugin_Module_ParticleModule);
  W_STATICLINK_REFERENCE(ParticlePlugin_Renderer_ParticleRenderer);
  W_STATICLINK_REFERENCE(ParticlePlugin_Resources_ParticleEffectResource);
  W_STATICLINK_REFERENCE(ParticlePlugin_Startup);
  W_STATICLINK_REFERENCE(ParticlePlugin_Streams_DefaultParticleStreams);
  W_STATICLINK_REFERENCE(ParticlePlugin_Streams_ParticleStream);
  W_STATICLINK_REFERENCE(ParticlePlugin_System_ParticleSystemDescriptor);
  W_STATICLINK_REFERENCE(ParticlePlugin_Type_Effect_ParticleTypeEffect);
  W_STATICLINK_REFERENCE(ParticlePlugin_Type_Light_ParticleTypeLight);
  W_STATICLINK_REFERENCE(ParticlePlugin_Type_Mesh_ParticleTypeMesh);
  W_STATICLINK_REFERENCE(ParticlePlugin_Type_ParticleType);
  W_STATICLINK_REFERENCE(ParticlePlugin_Type_Point_ParticleTypePoint);
  W_STATICLINK_REFERENCE(ParticlePlugin_Type_Point_PointRenderer);
  W_STATICLINK_REFERENCE(ParticlePlugin_Type_Quad_ParticleTypeQuad);
  W_STATICLINK_REFERENCE(ParticlePlugin_Type_Quad_QuadParticleRenderer);
  W_STATICLINK_REFERENCE(ParticlePlugin_Type_Trail_ParticleTypeTrail);
  W_STATICLINK_REFERENCE(ParticlePlugin_Type_Trail_TrailRenderer);
  W_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleWorldModule);
}
