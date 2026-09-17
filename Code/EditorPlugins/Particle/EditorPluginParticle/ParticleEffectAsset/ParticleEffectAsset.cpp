#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_ColorGradient.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Expression.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Move.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Opacity.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_SizeCurve.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Velocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_CylinderPosition.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomColor.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_SpherePosition.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEffectAssetDocument, 7, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleEffectAssetDocument::WParticleEffectAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WParticleEffectDescriptor>(sDocumentPath, WAssetDocEngineConnection::Simple, true)
{
  WVisualizerManager::GetSingleton()->SetVisualizersActive(this, m_bRenderVisualizers);
}

void WParticleEffectAssetDocument::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WParticleEffectDescriptor>())
  {
    auto& props = *e.m_pPropertyStates;

    bool bShared = e.m_pObject->GetTypeAccessor().GetValue("AlwaysShared").ConvertTo<bool>();

    props["SimulateInLocalSpace"].m_Visibility = bShared ? WPropertyUiState::Disabled : WPropertyUiState::Default;
    props["ApplyOwnerVelocity"].m_Visibility = bShared ? WPropertyUiState::Disabled : WPropertyUiState::Default;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WParticleTypeQuadFactory>())
  {
    auto& props = *e.m_pPropertyStates;

    bool useMaterial = e.m_pObject->GetTypeAccessor().GetValue("UseCustomMaterial").ConvertTo<bool>();
    WInt64 orientation = e.m_pObject->GetTypeAccessor().GetValue("Orientation").ConvertTo<WInt64>();
    WInt64 renderMode = e.m_pObject->GetTypeAccessor().GetValue("RenderMode").ConvertTo<WInt64>();
    WInt64 lightingMode = e.m_pObject->GetTypeAccessor().GetValue("LightingMode").ConvertTo<WInt64>();
    WInt64 textureAtlas = e.m_pObject->GetTypeAccessor().GetValue("TextureAtlas").ConvertTo<WInt64>();

    props["Deviation"].m_Visibility = WPropertyUiState::Invisible;
    props["DistortionTexture"].m_Visibility = WPropertyUiState::Invisible;
    props["DistortionStrength"].m_Visibility = WPropertyUiState::Invisible;
    props["ParticleStretch"].m_Visibility =
      (orientation == WQuadParticleOrientation::FixedAxis_EmitterDir || orientation == WQuadParticleOrientation::FixedAxis_ParticleDir)
        ? WPropertyUiState::Default
        : WPropertyUiState::Invisible;
    props["NumSpritesX"].m_Visibility = (textureAtlas == (int)WParticleTextureAtlasType::None) ? WPropertyUiState::Invisible : WPropertyUiState::Default;
    props["NumSpritesY"].m_Visibility = (textureAtlas == (int)WParticleTextureAtlasType::None) ? WPropertyUiState::Invisible : WPropertyUiState::Default;
    props["NormalCurvature"].m_Visibility = WPropertyUiState::Invisible;
    props["LightDirectionality"].m_Visibility = WPropertyUiState::Invisible;
    props["Texture"].m_Visibility = useMaterial ? WPropertyUiState::Invisible : WPropertyUiState::Default;
    props["CustomMaterial"].m_Visibility = useMaterial ? WPropertyUiState::Default : WPropertyUiState::Invisible;

    if (orientation == WQuadParticleOrientation::Fixed_EmitterDir || orientation == WQuadParticleOrientation::Fixed_WorldUp)
    {
      props["Deviation"].m_Visibility = WPropertyUiState::Default;
    }

    if (lightingMode == WParticleLightingMode::VertexLit)
    {
      props["NormalCurvature"].m_Visibility = WPropertyUiState::Default;
      props["LightDirectionality"].m_Visibility = WPropertyUiState::Default;
    }
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WParticleTypeTrailFactory>())
  {
    auto& props = *e.m_pPropertyStates;

    bool useMaterial = e.m_pObject->GetTypeAccessor().GetValue("UseCustomMaterial").ConvertTo<bool>();
    WInt64 renderMode = e.m_pObject->GetTypeAccessor().GetValue("RenderMode").ConvertTo<WInt64>();
    WInt64 lightingMode = e.m_pObject->GetTypeAccessor().GetValue("LightingMode").ConvertTo<WInt64>();
    WInt64 textureAtlas = e.m_pObject->GetTypeAccessor().GetValue("TextureAtlas").ConvertTo<WInt64>();

    props["DistortionTexture"].m_Visibility = WPropertyUiState::Invisible;
    props["DistortionStrength"].m_Visibility = WPropertyUiState::Invisible;
    props["NumSpritesX"].m_Visibility =
      (textureAtlas == (int)WParticleTextureAtlasType::None) ? WPropertyUiState::Invisible : WPropertyUiState::Default;
    props["NumSpritesY"].m_Visibility =
      (textureAtlas == (int)WParticleTextureAtlasType::None) ? WPropertyUiState::Invisible : WPropertyUiState::Default;
    props["NormalCurvature"].m_Visibility = WPropertyUiState::Invisible;
    props["LightDirectionality"].m_Visibility = WPropertyUiState::Invisible;
    props["Texture"].m_Visibility = useMaterial ? WPropertyUiState::Invisible : WPropertyUiState::Default;
    props["CustomMaterial"].m_Visibility = useMaterial ? WPropertyUiState::Default : WPropertyUiState::Invisible;

    if (lightingMode == WParticleLightingMode::VertexLit)
    {
      props["NormalCurvature"].m_Visibility = WPropertyUiState::Default;
      props["LightDirectionality"].m_Visibility = WPropertyUiState::Default;
    }
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WParticleBehaviorFactory_ColorGradient>())
  {
    auto& props = *e.m_pPropertyStates;

    WInt64 gradientSource = e.m_pObject->GetTypeAccessor().GetValue("GradientSource").ConvertTo<WInt64>();
    WInt64 mode = e.m_pObject->GetTypeAccessor().GetValue("ColorGradientMode").ConvertTo<WInt64>();

    props["Gradient"].m_Visibility = (gradientSource == WGradientSource::CustomGradient) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SharedGradient"].m_Visibility = (gradientSource == WGradientSource::SharedGradient) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["GradientMaxSpeed"].m_Visibility = (mode == WParticleColorGradientMode::Speed) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WParticleBehaviorFactory_Opacity>())
  {
    auto& props = *e.m_pPropertyStates;

    WInt64 curveSource = e.m_pObject->GetTypeAccessor().GetValue("ChangeOpacityWith").ConvertTo<WInt64>();

    props["OpacityCurve"].m_Visibility = (curveSource == WCurveSource::CustomCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SharedOpacityCurve"].m_Visibility = (curveSource == WCurveSource::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WParticleBehaviorFactory_SizeCurve>())
  {
    auto& props = *e.m_pPropertyStates;

    WInt64 curveSource = e.m_pObject->GetTypeAccessor().GetValue("ChangeSizeWith").ConvertTo<WInt64>();

    props["SizeCurve"].m_Visibility = (curveSource == WCurveSource::CustomCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SharedSizeCurve"].m_Visibility = (curveSource == WCurveSource::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WParticleBehaviorFactory_Velocity>())
  {
    auto& props = *e.m_pPropertyStates;

    WInt64 changeSpeedWith = e.m_pObject->GetTypeAccessor().GetValue("ChangeSpeedWith").ConvertTo<WInt64>();

    props["Friction"].m_Visibility = (changeSpeedWith == WVelocityChangeMode::Friction) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SpeedCurve"].m_Visibility = (changeSpeedWith == WVelocityChangeMode::CustomCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SharedSpeedCurve"].m_Visibility = (changeSpeedWith == WVelocityChangeMode::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SpeedCurveOffset"].m_Visibility = (changeSpeedWith == WVelocityChangeMode::CustomCurve || changeSpeedWith == WVelocityChangeMode::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SpeedCurveScale"].m_Visibility = (changeSpeedWith == WVelocityChangeMode::CustomCurve || changeSpeedWith == WVelocityChangeMode::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WParticleBehaviorFactory_Move>())
  {
    auto& props = *e.m_pPropertyStates;

    WInt64 moveX_Mode = e.m_pObject->GetTypeAccessor().GetValue("MoveX_Mode").ConvertTo<WInt64>();
    WInt64 moveY_Mode = e.m_pObject->GetTypeAccessor().GetValue("MoveY_Mode").ConvertTo<WInt64>();
    WInt64 moveZ_Mode = e.m_pObject->GetTypeAccessor().GetValue("MoveZ_Mode").ConvertTo<WInt64>();

    props["MoveX_Speed"].m_Visibility = (moveX_Mode == WMovementMode::Constant) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MoveX_Curve"].m_Visibility = (moveX_Mode == WMovementMode::CustomCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MoveX_SharedCurve"].m_Visibility = (moveX_Mode == WMovementMode::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MoveX_CurveOffset"].m_Visibility = (moveX_Mode == WMovementMode::CustomCurve || moveX_Mode == WMovementMode::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MoveX_CurveScale"].m_Visibility = (moveX_Mode == WMovementMode::CustomCurve || moveX_Mode == WMovementMode::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;

    props["MoveY_Speed"].m_Visibility = (moveY_Mode == WMovementMode::Constant) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MoveY_Curve"].m_Visibility = (moveY_Mode == WMovementMode::CustomCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MoveY_SharedCurve"].m_Visibility = (moveY_Mode == WMovementMode::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MoveY_CurveOffset"].m_Visibility = (moveY_Mode == WMovementMode::CustomCurve || moveY_Mode == WMovementMode::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MoveY_CurveScale"].m_Visibility = (moveY_Mode == WMovementMode::CustomCurve || moveY_Mode == WMovementMode::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;

    props["MoveZ_Speed"].m_Visibility = (moveZ_Mode == WMovementMode::Constant) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MoveZ_Curve"].m_Visibility = (moveZ_Mode == WMovementMode::CustomCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MoveZ_SharedCurve"].m_Visibility = (moveZ_Mode == WMovementMode::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MoveZ_CurveOffset"].m_Visibility = (moveZ_Mode == WMovementMode::CustomCurve || moveZ_Mode == WMovementMode::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["MoveZ_CurveScale"].m_Visibility = (moveZ_Mode == WMovementMode::CustomCurve || moveZ_Mode == WMovementMode::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WParticleInitializerFactory_CylinderPosition>())
  {
    auto& props = *e.m_pPropertyStates;

    bool bSetVelocity = e.m_pObject->GetTypeAccessor().GetValue("SetVelocity").ConvertTo<bool>();

    props["Speed"].m_Visibility = bSetVelocity ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WParticleInitializerFactory_SpherePosition>())
  {
    auto& props = *e.m_pPropertyStates;

    bool bSetVelocity = e.m_pObject->GetTypeAccessor().GetValue("SetVelocity").ConvertTo<bool>();

    props["Speed"].m_Visibility = bSetVelocity ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WParticleInitializerFactory_RandomColor>())
  {
    auto& props = *e.m_pPropertyStates;

    WInt64 gradientSource = e.m_pObject->GetTypeAccessor().GetValue("GradientSource").ConvertTo<WInt64>();

    props["Gradient"].m_Visibility = (gradientSource == WGradientSource::CustomGradient) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SharedGradient"].m_Visibility = (gradientSource == WGradientSource::SharedGradient) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WParticleExpressionInput>())
  {
    auto& props = *e.m_pPropertyStates;

    WInt64 curveSource = e.m_pObject->GetTypeAccessor().GetValue("CurveSource").ConvertTo<WInt64>();

    props["Curve"].m_Visibility = (curveSource == WCurveSource::CustomCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SharedCurve"].m_Visibility = (curveSource == WCurveSource::SharedCurve) ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }
}

void WParticleEffectAssetDocument::WriteResource(WStreamWriter& inout_stream) const
{
  const WParticleEffectDescriptor* pProp = GetProperties();

  pProp->Save(inout_stream);
}


void WParticleEffectAssetDocument::TriggerRestartEffect()
{
  WParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = WParticleEffectAssetEvent::RestartEffect;

  m_Events.Broadcast(e);
}


void WParticleEffectAssetDocument::SetAutoRestart(bool bEnable)
{
  if (m_bAutoRestart == bEnable)
    return;

  m_bAutoRestart = bEnable;

  WParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = WParticleEffectAssetEvent::AutoRestartChanged;

  m_Events.Broadcast(e);
}


void WParticleEffectAssetDocument::SetSimulationPaused(bool bPaused)
{
  if (m_bSimulationPaused == bPaused)
    return;

  m_bSimulationPaused = bPaused;

  WParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = WParticleEffectAssetEvent::SimulationSpeedChanged;

  m_Events.Broadcast(e);
}

void WParticleEffectAssetDocument::SetSimulationSpeed(float fSpeed)
{
  if (m_fSimulationSpeed == fSpeed)
    return;

  m_fSimulationSpeed = fSpeed;

  WParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = WParticleEffectAssetEvent::SimulationSpeedChanged;

  m_Events.Broadcast(e);
}


void WParticleEffectAssetDocument::SetRenderVisualizers(bool b)
{
  if (m_bRenderVisualizers == b)
    return;

  m_bRenderVisualizers = b;

  WVisualizerManager::GetSingleton()->SetVisualizersActive(this, m_bRenderVisualizers);

  WParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = WParticleEffectAssetEvent::RenderVisualizersChanged;

  m_Events.Broadcast(e);
}

WResult WParticleEffectAssetDocument::ComputeObjectTransformation(const WDocumentObject* pObject, WTransform& out_result) const
{
  // currently the preview particle effect is always at the origin
  out_result.SetIdentity();
  return W_SUCCESS;
}

void WParticleEffectAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  auto* desc = GetProperties();

  for (const auto& system : desc->GetParticleSystems())
  {
    for (const auto& type : system->GetTypeFactories())
    {
      if (auto* pType = WDynamicCast<WParticleTypeQuadFactory*>(type))
      {
        // remove unused dependencies
        if (pType->m_bUseCustomMaterial)
        {
          pInfo->m_TransformDependencies.Remove(pType->m_sTexture);
        }
        else
        {
          pInfo->m_TransformDependencies.Remove(pType->m_sCustomMaterial);
        }
      }

      if (auto* pType = WDynamicCast<WParticleTypeTrailFactory*>(type))
      {
        // remove unused dependencies
        if (pType->m_bUseCustomMaterial)
        {
          pInfo->m_TransformDependencies.Remove(pType->m_sTexture);
        }
        else
        {
          pInfo->m_TransformDependencies.Remove(pType->m_sCustomMaterial);
        }
      }
    }
  }

  // shared effects do not support parameters
  if (!desc->m_bAlwaysShared)
  {
    WExposedParameters* pExposedParams = W_DEFAULT_NEW(WExposedParameters);
    for (auto it = desc->m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      WExposedParameter* param = W_DEFAULT_NEW(WExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = it.Key();
      param->m_DefaultValue = it.Value();
    }
    for (auto it = desc->m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      WExposedParameter* param = W_DEFAULT_NEW(WExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = it.Key();
      param->m_DefaultValue = it.Value();
      param->m_Attributes.PushBack(W_DEFAULT_NEW(WExposeColorAlphaAttribute));
    }

    // Info takes ownership of meta data.
    pInfo->m_MetaInfo.PushBack(pExposedParams);
  }
}

WTransformStatus WParticleEffectAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag,
  const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  WriteResource(stream);
  return WStatus(W_SUCCESS);
}

WTransformStatus WParticleEffectAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  WStatus status = WAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}
