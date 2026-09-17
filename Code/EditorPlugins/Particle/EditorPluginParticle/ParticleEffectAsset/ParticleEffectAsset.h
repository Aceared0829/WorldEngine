#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Foundation/Communication/Event.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>

class WParticleEffectAssetDocument;
struct WPropertyMetaStateEvent;

struct WParticleEffectAssetEvent
{
  enum Type
  {
    RestartEffect,
    AutoRestartChanged,
    SimulationSpeedChanged,
    RenderVisualizersChanged,
  };

  WParticleEffectAssetDocument* m_pDocument;
  Type m_Type;
};

class WParticleEffectAssetDocument : public WSimpleAssetDocument<WParticleEffectDescriptor>
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEffectAssetDocument, WSimpleAssetDocument<WParticleEffectDescriptor>);

public:
  WParticleEffectAssetDocument(WStringView sDocumentPath);

  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

  void WriteResource(WStreamWriter& inout_stream) const;

  void TriggerRestartEffect();

  WEvent<const WParticleEffectAssetEvent&> m_Events;

  void SetAutoRestart(bool bEnable);
  bool GetAutoRestart() const { return m_bAutoRestart; }

  void SetSimulationPaused(bool bPaused);
  bool GetSimulationPaused() const { return m_bSimulationPaused; }

  void SetSimulationSpeed(float fSpeed);
  float GetSimulationSpeed() const { return m_fSimulationSpeed; }

  bool GetRenderVisualizers() const { return m_bRenderVisualizers; }
  void SetRenderVisualizers(bool b);

  // Overridden to enable support for visualizers/manipulators
  virtual WResult ComputeObjectTransformation(const WDocumentObject* pObject, WTransform& out_result) const override;

protected:
  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

private:
  bool m_bSimulationPaused = false;
  bool m_bAutoRestart = true;
  bool m_bRenderVisualizers = false;
  float m_fSimulationSpeed = 1.0f;
};
