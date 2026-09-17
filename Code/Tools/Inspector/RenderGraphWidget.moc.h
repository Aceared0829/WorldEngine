#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/String.h>
#include <Inspector/RenderGraphHistogramWidget.moc.h>
#include <Inspector/RenderGraphOverviewWidget.moc.h>
#include <Inspector/RenderGraphPreviewWidget.moc.h>
#include <Inspector/ui_RenderGraphWidget.h>
#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>
#include <ads/DockWidget.h>

class QHideEvent;
class QShowEvent;

/// Dock widget that coordinates render graph inspection telemetry and preview controls.
class WQtRenderGraphWidget : public ads::CDockWidget, public Ui_RenderGraphWidget
{
  Q_OBJECT

public:
  WQtRenderGraphWidget(ads::CDockManager* pDockManager, QWidget* pParent = nullptr);

  static WQtRenderGraphWidget* s_pWidget;
  static void ProcessTelemetry(void* pUnused);

  void ResetStats();
  void UpdateStats();

protected:
  void showEvent(QShowEvent* pEvent) override;
  void hideEvent(QHideEvent* pEvent) override;

private:
  void SendSummaryRequest();
  void SendInfoRequest();
  void SendObserverRequest();
  void SendObserverRequest(const WRenderGraphObserverRequest& request);
  void PauseObservation();
  void ResumeObservation();
  void UpdateObservationVisibility();
  void UpdateGraphList();
  void UpdateSwapChainList();
  void UpdateInfoWidgets();
  void UpdateRequestControls();
  void SelectAccess(WUInt16 uiPassIndex, WUInt16 uiAccessIndex);
  void ClearAccessSelection();
  void UpdatePreviewRequest(float fZoom, WVec2 panCenter, WVec2I32 pixel, bool bUpdatePixelPosition, bool bHighlightPixel);
  void SetRequestFromControls();

private:
  WRenderGraphInspectionSummary m_Summary;
  WRenderGraphInspectionInfo m_Info;
  WRenderGraphObserverRequest m_Request;
  WRenderGraphObserverResponse m_Response;
  WString m_sLastPixelValue;
  WVec2I32 m_vLastPixelPosition = WVec2I32(-1, -1);
  WStaticArray<WUInt8, 1024> m_LastHistogram;
  WUInt64 m_uiSelectedGraphId = 0;
  WUInt32 m_uiSelectedSwapChainId = 0xFFFFFFFF;
  bool m_bHasLastPixelValue = false;
  bool m_bHasLastHistogram = false;
  bool m_bInfoValid = false;
  bool m_bUpdateUi = true;
  bool m_bUpdatingControls = false;
  bool m_bObservationPaused = false;
};
