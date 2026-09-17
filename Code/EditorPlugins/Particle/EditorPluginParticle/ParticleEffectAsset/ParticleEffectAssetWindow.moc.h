#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtOrbitCamViewWidget;
class WParticleEffectAssetDocument;
class QComboBox;
class QToolButton;
class WQtPropertyGridWidget;


class WQtParticleEffectAssetDocumentWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT

public:
  WQtParticleEffectAssetDocumentWindow(WAssetDocument* pDocument);
  ~WQtParticleEffectAssetDocumentWindow();

  WParticleEffectAssetDocument* GetParticleDocument();

private Q_SLOTS:
  void onSystemSelected(int index);
  void onAddSystem(bool);
  void onRemoveSystem(bool);
  void onRenameSystem(bool);

protected:
  virtual void InternalRedraw() override;

private:
  void SendRedrawMsg();
  void RestoreResource();
  void SendLiveResourcePreview();
  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);
  void ParticleEventHandler(const WParticleEffectAssetEvent& e);
  void UpdateSystemList();
  void SelectSystem(const WDocumentObject* pObject);
  WStatus SetupSystem(WStringView sName);

  WParticleEffectAssetDocument* m_pAssetDoc;

  WEngineViewConfig m_ViewConfig;
  WQtOrbitCamViewWidget* m_pViewWidget;

  QComboBox* m_pSystemsCombo = nullptr;
  QToolButton* m_pAddSystem = nullptr;
  QToolButton* m_pRemoveSystem = nullptr;
  QToolButton* m_pRenameSystem = nullptr;
  WQtPropertyGridWidget* m_pPropertyGridSystems = nullptr;
  WQtPropertyGridWidget* m_pPropertyGridEmitter = nullptr;
  WQtPropertyGridWidget* m_pPropertyGridInitializer = nullptr;
  WQtPropertyGridWidget* m_pPropertyGridBehavior = nullptr;
  WQtPropertyGridWidget* m_pPropertyGridType = nullptr;

  WString m_sSelectedSystem;
  WMap<WString, WDocumentObject*> m_ParticleSystems;
  bool m_bDoLiveResourceUpdate = true;
};
