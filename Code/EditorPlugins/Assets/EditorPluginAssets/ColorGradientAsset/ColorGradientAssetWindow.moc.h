#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtColorGradientEditorWidget;

class WQtColorGradientAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtColorGradientAssetDocumentWindow(WDocument* pDocument);
  ~WQtColorGradientAssetDocumentWindow();

private Q_SLOTS:
  void onGradientColorCpAdded(double posX, const WColorGammaUB& color);
  void onGradientAlphaCpAdded(double posX, WUInt8 alpha);
  void onGradientIntensityCpAdded(double posX, float intensity);

  void MoveCP(WInt32 idx, double newPosX, const char* szArrayName);
  void onGradientColorCpMoved(WInt32 idx, double newPosX);
  void onGradientAlphaCpMoved(WInt32 idx, double newPosX);
  void onGradientIntensityCpMoved(WInt32 idx, double newPosX);

  void RemoveCP(WInt32 idx, const char* szArrayName);
  void onGradientColorCpDeleted(WInt32 idx);
  void onGradientAlphaCpDeleted(WInt32 idx);
  void onGradientIntensityCpDeleted(WInt32 idx);

  void onGradientColorCpChanged(WInt32 idx, const WColorGammaUB& color);
  void onGradientAlphaCpChanged(WInt32 idx, WUInt8 alpha);
  void onGradientIntensityCpChanged(WInt32 idx, float intensity);

  void onGradientBeginOperation();
  void onGradientEndOperation(bool commit);

  void onGradientNormalizeRange();

private:
  void UpdatePreview();

  void SendLiveResourcePreview();
  void RestoreResource();

  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);

  bool m_bShowFirstTime;
  WQtColorGradientEditorWidget* m_pGradientEditor;
};
