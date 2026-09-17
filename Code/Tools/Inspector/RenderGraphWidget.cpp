#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <Inspector/RenderGraphWidget.moc.h>
#include <RendererFoundation/RendererReflection.h>

#include <QCheckBox>
#include <QHideEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPalette>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QToolButton>

namespace
{
  constexpr WUInt32 s_uiSystemId = 'RGPH';
  constexpr WUInt16 s_uiProtocolVersion = 1;

  enum GraphListItemDataRole
  {
    GraphListRole_RenderGraphId = Qt::UserRole,
    GraphListRole_IsGraphItem = Qt::UserRole + 1,
  };

  enum SwapChainListItemDataRole
  {
    SwapChainListRole_SwapChainId = Qt::UserRole,
    SwapChainListRole_Width = Qt::UserRole + 1,
    SwapChainListRole_Height = Qt::UserRole + 2,
  };

  QString MakeGraphLabel(const WRenderGraphExecutionSummary& graph)
  {
    WStringBuilder label;
    if (!graph.m_sUserName.IsEmpty())
      label.SetFormat("{} ({})", graph.m_sUserName, graph.m_sGraphName);
    else
      label = graph.m_sGraphName;

    if (graph.m_uiExecutionOrder >= 0)
      label.AppendFormat("  [{}]", graph.m_uiExecutionOrder);

    return WMakeQString(label);
  }

  QString MakeSwapChainLabel(const WRenderGraphSwapChainSummary& swapChain)
  {
    WStringBuilder label;
    label.SetFormat("0x{}  {}x{}", WArgU(swapChain.m_uiSwapChainId, 8, true, 16), swapChain.m_uiWidth, swapChain.m_uiHeight);
    return WMakeQString(label);
  }

  QString MakePhaseLabel(WRenderGraphPhase::Enum phase)
  {
    switch (phase)
    {
      case WRenderGraphPhase::PreRender:
        return "PreRender";
      case WRenderGraphPhase::Render:
        return "Render";
      case WRenderGraphPhase::PostRender:
        return "PostRender";
      default:
        return "Unknown";
    }
  }

  void AddGraphPhaseSeparator(QListWidget* pList, WRenderGraphPhase::Enum phase)
  {
    QListWidgetItem* pItem = new QListWidgetItem(MakePhaseLabel(phase));
    pItem->setFlags(Qt::NoItemFlags);
    pItem->setData(GraphListRole_IsGraphItem, false);
    pItem->setForeground(pList->palette().color(QPalette::Text));
    pItem->setBackground(pList->palette().color(QPalette::Mid));
    pList->addItem(pItem);
  }

} // namespace

WQtRenderGraphWidget* WQtRenderGraphWidget::s_pWidget = nullptr;

WQtRenderGraphWidget::WQtRenderGraphWidget(ads::CDockManager* pDockManager, QWidget* pParent)
  : ads::CDockWidget(pDockManager, "Render Graph", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(HorizontalSplitter);

  QPalette overviewScrollPalette = OverviewScroll->viewport()->palette();
  overviewScrollPalette.setColor(QPalette::Window, palette().color(QPalette::Dark));
  OverviewScroll->viewport()->setAutoFillBackground(true);
  OverviewScroll->viewport()->setPalette(overviewScrollPalette);

  connect(Overview, &WQtRenderGraphOverviewWidget::AccessSelected, this, &WQtRenderGraphWidget::SelectAccess);
  connect(Overview, &WQtRenderGraphOverviewWidget::AccessDeselected, this, &WQtRenderGraphWidget::ClearAccessSelection);
  connect(Preview, &WQtRenderGraphPreviewWidget::RequestChanged, this, &WQtRenderGraphWidget::UpdatePreviewRequest);

  auto requestFromControls = [this]()
  {
    if (!m_bUpdatingControls)
    {
      SetRequestFromControls();
      if (m_bHasLastHistogram)
      {
        Histogram->SetHistogram(m_LastHistogram.GetArrayPtr());
      }
      SendObserverRequest();
    }
  };

  auto setRange = [this](float fMin, float fMax)
  {
    m_bUpdatingControls = true;
    RangeMinSpin->setValue(fMin);
    RangeMaxSpin->setValue(fMax);
    m_bUpdatingControls = false;

    SetRequestFromControls();
    if (m_bHasLastHistogram)
    {
      Histogram->SetHistogram(m_LastHistogram.GetArrayPtr());
    }
    SendObserverRequest();
  };

  connect(GraphList, &QListWidget::currentRowChanged, this, [this](int row)
    {
    if (row < 0)
      return;
    QListWidgetItem* pItem = GraphList->item(row);
    if (pItem == nullptr || !pItem->data(GraphListRole_IsGraphItem).toBool())
      return;

    m_uiSelectedGraphId = pItem->data(GraphListRole_RenderGraphId).toULongLong();
    m_bInfoValid = false;
    m_Info.Clear();
    Overview->Clear();
    m_Request = WRenderGraphObserverRequest();
    m_Request.m_uiRenderGraphId = m_uiSelectedGraphId;
    Preview->SetView(m_Request.m_fZoom, m_Request.m_vPanCenter);
    SendInfoRequest();
    m_bUpdateUi = true; });

  connect(SwapChainList, &QListWidget::currentRowChanged, this, [this](int row)
    {
    if (row < 0)
      return;
    QListWidgetItem* pItem = SwapChainList->item(row);
    m_uiSelectedSwapChainId = pItem->data(SwapChainListRole_SwapChainId).toUInt();
    m_Request.m_uiSwapChainId = m_uiSelectedSwapChainId;
    Preview->SetTargetSize(WVec2U32(pItem->data(SwapChainListRole_Width).toUInt(), pItem->data(SwapChainListRole_Height).toUInt()));
    SendObserverRequest(); });

  connect(MipSpin, qOverload<int>(&QSpinBox::valueChanged), this, requestFromControls);
  connect(SliceSpin, qOverload<int>(&QSpinBox::valueChanged), this, requestFromControls);
  connect(SampleSpin, qOverload<int>(&QSpinBox::valueChanged), this, requestFromControls);
  connect(RangeMinSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, requestFromControls);
  connect(RangeMaxSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, requestFromControls);
  connect(ChannelR, &QCheckBox::toggled, this, requestFromControls);
  connect(ChannelG, &QCheckBox::toggled, this, requestFromControls);
  connect(ChannelB, &QCheckBox::toggled, this, requestFromControls);
  connect(ChannelA, &QCheckBox::toggled, this, requestFromControls);
  connect(ResetToolButton, &QToolButton::clicked, this, [setRange]()
    { setRange(0.0f, 1.0f); });
  connect(AutoToolButton, &QToolButton::clicked, this, [this, setRange]()
    { setRange(m_Response.m_fImageMin, m_Response.m_fImageMax); });
  connect(ResetZoomToolButton, &QToolButton::clicked, this, [this]()
    {
    m_Request.m_fZoom = 1.0f;
    m_Request.m_vPanCenter = WVec2(0.5f);
    Preview->SetView(m_Request.m_fZoom, m_Request.m_vPanCenter);
    SendObserverRequest(); });

  ResetStats();
}

void WQtRenderGraphWidget::ResetStats()
{
  m_Summary.m_AvailableSwapChains.Clear();
  m_Summary.m_RenderGraphs.Clear();
  m_Info.Clear();
  m_Request = WRenderGraphObserverRequest();
  m_Response = WRenderGraphObserverResponse();
  m_sLastPixelValue.Clear();
  m_vLastPixelPosition = WVec2I32(-1, -1);
  m_LastHistogram.Clear();
  m_uiSelectedGraphId = 0;
  m_uiSelectedSwapChainId = 0xFFFFFFFF;
  m_bHasLastPixelValue = false;
  m_bHasLastHistogram = false;
  m_bInfoValid = false;
  m_bUpdateUi = true;
  Overview->Clear();
  Preview->SetTextureSize(WVec2U32(0, 0));
  Histogram->Clear();
  MipSpin->setMaximum(0);
  MipSpin->setEnabled(false);
  SliceSpin->setMaximum(0);
  SliceSpin->setEnabled(false);
  SampleSpin->setMaximum(-1);
  SampleSpin->setEnabled(false);
  SendSummaryRequest();
}

void WQtRenderGraphWidget::UpdateStats()
{
  UpdateObservationVisibility();

  if (!isVisible())
    return;

  if (!m_bUpdateUi)
    return;
  m_bUpdateUi = false;
  UpdateGraphList();
  UpdateSwapChainList();
  UpdateInfoWidgets();
}

void WQtRenderGraphWidget::SendSummaryRequest()
{
  WTelemetryMessage msg;
  msg.SetMessageID(s_uiSystemId, 'RSUM');
  msg.GetWriter() << s_uiProtocolVersion;
  WTelemetry::SendToServer(msg);
}

void WQtRenderGraphWidget::SendInfoRequest()
{
  if (m_uiSelectedGraphId == 0)
    return;
  WTelemetryMessage msg;
  msg.SetMessageID(s_uiSystemId, 'RINF');
  msg.GetWriter() << s_uiProtocolVersion;
  msg.GetWriter() << m_uiSelectedGraphId;
  WTelemetry::SendToServer(msg);
}

void WQtRenderGraphWidget::SendObserverRequest()
{
  if (m_bObservationPaused)
    return;

  SendObserverRequest(m_Request);
}

void WQtRenderGraphWidget::SendObserverRequest(const WRenderGraphObserverRequest& request)
{
  if (request.m_uiRenderGraphId == 0)
    return;
  WTelemetryMessage msg;
  msg.SetMessageID(s_uiSystemId, 'RREQ');
  msg.GetWriter() << s_uiProtocolVersion;
  msg.GetWriter() << request;
  WTelemetry::SendToServer(msg);
}

void WQtRenderGraphWidget::PauseObservation()
{
  if (m_bObservationPaused)
    return;

  m_bObservationPaused = true;

  WRenderGraphObserverRequest pauseRequest = m_Request;
  pauseRequest.m_sPassName.Clear();
  pauseRequest.m_uiAccessIndex = 0;
  pauseRequest.m_vPixelPosition = WVec2I32(-1, -1);
  pauseRequest.m_bHighlightPixel = false;
  SendObserverRequest(pauseRequest);
}

void WQtRenderGraphWidget::ResumeObservation()
{
  if (!m_bObservationPaused)
    return;

  m_bObservationPaused = false;

  if (!m_Request.m_sPassName.IsEmpty())
  {
    SendObserverRequest();
  }
}

void WQtRenderGraphWidget::UpdateObservationVisibility()
{
  if (isVisible())
  {
    ResumeObservation();
  }
  else
  {
    PauseObservation();
  }
}

void WQtRenderGraphWidget::showEvent(QShowEvent* pEvent)
{
  ads::CDockWidget::showEvent(pEvent);
  UpdateObservationVisibility();
}

void WQtRenderGraphWidget::hideEvent(QHideEvent* pEvent)
{
  ads::CDockWidget::hideEvent(pEvent);
  UpdateObservationVisibility();
}

void WQtRenderGraphWidget::UpdateGraphList()
{
  const QSignalBlocker blocker(GraphList);
  GraphList->clear();
  WInt32 iSelectedRow = -1;

  constexpr WRenderGraphPhase::Enum phases[] = {WRenderGraphPhase::PreRender, WRenderGraphPhase::Render, WRenderGraphPhase::PostRender};
  for (WRenderGraphPhase::Enum phase : phases)
  {
    AddGraphPhaseSeparator(GraphList, phase);

    for (WUInt32 i = 0; i < m_Summary.m_RenderGraphs.GetCount(); ++i)
    {
      const auto& graph = m_Summary.m_RenderGraphs[i];
      if (graph.m_Phase != phase)
        continue;

      const WUInt64 uiIdentity = graph.m_uiRenderGraphId;
      QListWidgetItem* pItem = new QListWidgetItem(MakeGraphLabel(graph));
      pItem->setData(GraphListRole_RenderGraphId, QVariant::fromValue<qulonglong>(uiIdentity));
      pItem->setData(GraphListRole_IsGraphItem, true);
      pItem->setForeground(graph.m_uiExecutionOrder >= 0 ? WToQtColor(WColorScheme::LightUI(WColorScheme::Green)) : GraphList->palette().color(QPalette::Disabled, QPalette::Text));
      GraphList->addItem(pItem);
      if (uiIdentity == m_uiSelectedGraphId)
        iSelectedRow = GraphList->count() - 1;
    }
  }

  if (iSelectedRow >= 0)
    GraphList->setCurrentRow(iSelectedRow);
}

void WQtRenderGraphWidget::UpdateSwapChainList()
{
  const QSignalBlocker blocker(SwapChainList);
  SwapChainList->clear();
  WInt32 iSelectedRow = -1;
  for (WUInt32 i = 0; i < m_Summary.m_AvailableSwapChains.GetCount(); ++i)
  {
    const WRenderGraphSwapChainSummary& swapChain = m_Summary.m_AvailableSwapChains[i];
    const WUInt32 uiIdentity = swapChain.m_uiSwapChainId;
    QListWidgetItem* pItem = new QListWidgetItem(MakeSwapChainLabel(swapChain));
    pItem->setData(SwapChainListRole_SwapChainId, uiIdentity);
    pItem->setData(SwapChainListRole_Width, swapChain.m_uiWidth);
    pItem->setData(SwapChainListRole_Height, swapChain.m_uiHeight);
    SwapChainList->addItem(pItem);
    if (uiIdentity == m_uiSelectedSwapChainId)
      iSelectedRow = (WInt32)i;
  }

  // If the previously selected swap chain is no longer available (or none was selected yet), fall back to
  // the first one so that there is always a target selected as long as at least one swap chain exists.
  bool bSelectionChanged = false;
  if (iSelectedRow < 0 && SwapChainList->count() > 0)
  {
    iSelectedRow = 0;
    m_uiSelectedSwapChainId = SwapChainList->item(0)->data(SwapChainListRole_SwapChainId).toUInt();
    m_Request.m_uiSwapChainId = m_uiSelectedSwapChainId;
    bSelectionChanged = true;
  }

  if (iSelectedRow >= 0)
  {
    SwapChainList->setCurrentRow(iSelectedRow);
    Preview->SetTargetSize(WVec2U32(SwapChainList->item(iSelectedRow)->data(SwapChainListRole_Width).toUInt(), SwapChainList->item(iSelectedRow)->data(SwapChainListRole_Height).toUInt()));
  }

  if (bSelectionChanged)
  {
    SendObserverRequest();
  }
}

void WQtRenderGraphWidget::UpdateInfoWidgets()
{
  if (m_bInfoValid)
  {
    Overview->SetInfo(m_uiSelectedGraphId, m_Info);
    Overview->SetRequest(m_Request);
  }

  if (m_Response.m_PixelValue.IsValid())
  {
    m_sLastPixelValue = m_Response.m_PixelValue.ConvertTo<WString>();
    m_vLastPixelPosition = m_Request.m_vPixelPosition;
    m_bHasLastPixelValue = true;
  }

  if (m_bHasLastPixelValue)
  {
    WStringBuilder text;
    text.SetFormat("Pixel ({}, {}): {}", m_vLastPixelPosition.x, m_vLastPixelPosition.y, m_sLastPixelValue);
    PixelLabel->setText(WMakeQString(text));
  }
  else
  {
    PixelLabel->setText("Pixel: -");
  }

  MinValue->setText(QString::number(m_Response.m_fImageMin, 'g', 9));
  MaxValue->setText(QString::number(m_Response.m_fImageMax, 'g', 9));

  if (m_Response.m_bHistogramValid)
  {
    m_LastHistogram = m_Response.m_Histogram;
    m_bHasLastHistogram = true;
  }

  if (m_bHasLastHistogram)
  {
    Histogram->SetHistogram(m_LastHistogram.GetArrayPtr());
  }
}

void WQtRenderGraphWidget::UpdateRequestControls()
{
  m_bUpdatingControls = true;
  MipSpin->setValue(m_Request.m_uiMipLevel);
  SliceSpin->setValue(m_Request.m_uiArraySlice);
  SampleSpin->setValue(m_Request.m_iSampleIndex);
  RangeMinSpin->setValue(m_Request.m_fRangeMin);
  RangeMaxSpin->setValue(m_Request.m_fRangeMax);
  ChannelR->setChecked((m_Request.m_uiChannelMask & W_BIT(0)) != 0);
  ChannelG->setChecked((m_Request.m_uiChannelMask & W_BIT(1)) != 0);
  ChannelB->setChecked((m_Request.m_uiChannelMask & W_BIT(2)) != 0);
  ChannelA->setChecked((m_Request.m_uiChannelMask & W_BIT(3)) != 0);
  m_bUpdatingControls = false;
}

void WQtRenderGraphWidget::UpdatePreviewRequest(float fZoom, WVec2 panCenter, WVec2I32 pixel, bool bUpdatePixelPosition, bool bHighlightPixel)
{
  m_Request.m_fZoom = fZoom;
  m_Request.m_vPanCenter = panCenter;
  if (bUpdatePixelPosition)
  {
    m_Request.m_vPixelPosition = pixel;
  }
  m_Request.m_bHighlightPixel = bHighlightPixel;
  SendObserverRequest();
}

void WQtRenderGraphWidget::SetRequestFromControls()
{
  m_Request.m_uiMipLevel = static_cast<WUInt8>(WMath::Clamp(MipSpin->value(), 0, 255));
  m_Request.m_uiArraySlice = static_cast<WUInt16>(WMath::Clamp(SliceSpin->value(), 0, 65535));
  m_Request.m_iSampleIndex = static_cast<WInt8>(WMath::Clamp(SampleSpin->value(), -1, 127));
  m_Request.m_fRangeMin = (float)RangeMinSpin->value();
  m_Request.m_fRangeMax = (float)RangeMaxSpin->value();
  m_Request.m_uiChannelMask = 0;
  m_Request.m_uiChannelMask |= ChannelR->isChecked() ? W_BIT(0) : 0;
  m_Request.m_uiChannelMask |= ChannelG->isChecked() ? W_BIT(1) : 0;
  m_Request.m_uiChannelMask |= ChannelB->isChecked() ? W_BIT(2) : 0;
  m_Request.m_uiChannelMask |= ChannelA->isChecked() ? W_BIT(3) : 0;
  m_Request.m_uiSwapChainId = m_uiSelectedSwapChainId;
}

void WQtRenderGraphWidget::SelectAccess(WUInt16 uiPassIndex, WUInt16 uiAccessIndex)
{
  if (uiPassIndex >= m_Info.m_Passes.GetCount())
    return;
  const WRenderGraphInspectionInfo::AccessInfo* pSelectedAccess = nullptr;
  for (const auto& access : m_Info.m_Accesses)
  {
    if (access.m_bIsTexture && access.m_uiPassIndex == uiPassIndex && access.m_uiAccessIndex == uiAccessIndex)
    {
      pSelectedAccess = &access;
      break;
    }
  }
  if (pSelectedAccess == nullptr || pSelectedAccess->m_uiResourceIndex >= m_Info.m_Textures.GetCount())
    return;

  const auto& texture = m_Info.m_Textures[pSelectedAccess->m_uiResourceIndex];
  const WInt32 iMipMax = WMath::Max<WInt32>(0, texture.m_Desc.m_uiMipLevelCount - 1);
  const WInt32 iSliceMax = WMath::Max<WInt32>(0, texture.m_Desc.GetNumberOfSlices() - 1);
  const WInt32 iSampleCount = (WInt32)texture.m_Desc.m_SampleCount;
  const bool bHasMsaaSamples = texture.m_Desc.m_SampleCount != WGALMSAASampleCount::None;
  MipSpin->setMaximum(iMipMax);
  MipSpin->setEnabled(iMipMax > 0);
  SliceSpin->setMaximum(iSliceMax);
  SliceSpin->setEnabled(iSliceMax > 0);
  SampleSpin->setMaximum(bHasMsaaSamples ? iSampleCount - 1 : -1);
  SampleSpin->setEnabled(bHasMsaaSamples);
  Preview->SetTextureSize(WVec2U32(texture.m_Desc.m_uiWidth, texture.m_Desc.m_uiHeight));
  m_Request.m_uiRenderGraphId = m_uiSelectedGraphId;
  m_Request.m_sPassName = m_Info.m_Passes[uiPassIndex].m_sName;
  m_Request.m_uiAccessIndex = uiAccessIndex;
  m_Request.m_uiSwapChainId = m_uiSelectedSwapChainId;
  SetRequestFromControls();
  UpdateRequestControls();
  Overview->SetRequest(m_Request);
  SendObserverRequest();
}

void WQtRenderGraphWidget::ClearAccessSelection()
{
  m_Request.m_uiRenderGraphId = m_uiSelectedGraphId;
  m_Request.m_sPassName.Clear();
  m_Request.m_uiAccessIndex = 0;
  m_Request.m_vPixelPosition = WVec2I32(-1, -1);
  m_Request.m_bHighlightPixel = false;
  m_Request.m_uiSwapChainId = m_uiSelectedSwapChainId;

  m_sLastPixelValue.Clear();
  m_vLastPixelPosition = WVec2I32(-1, -1);
  m_LastHistogram.Clear();
  m_Response = WRenderGraphObserverResponse();
  m_bHasLastPixelValue = false;
  m_bHasLastHistogram = false;

  MipSpin->setMaximum(0);
  MipSpin->setEnabled(false);
  SliceSpin->setMaximum(0);
  SliceSpin->setEnabled(false);
  SampleSpin->setMaximum(-1);
  SampleSpin->setEnabled(false);
  Preview->SetTextureSize(WVec2U32(0, 0));
  Overview->SetRequest(m_Request);
  Histogram->Clear();
  UpdateInfoWidgets();
  SendObserverRequest();
}

void WQtRenderGraphWidget::ProcessTelemetry(void*)
{
  if (s_pWidget == nullptr)
    return;

  WTelemetryMessage msg;
  while (WTelemetry::RetrieveMessage(s_uiSystemId, msg) == W_SUCCESS)
  {
    WUInt16 uiVersion = 0;
    switch (msg.GetMessageID())
    {
      case 'SUMM':
        msg.GetReader() >> uiVersion;
        msg.GetReader() >> s_pWidget->m_Summary;
        s_pWidget->m_bUpdateUi = true;
        break;
      case 'INFO':
      {
        WUInt64 uiGraphIdentity = 0;
        bool bSuccess = false;
        msg.GetReader() >> uiVersion;
        msg.GetReader() >> uiGraphIdentity;
        msg.GetReader() >> bSuccess;
        if (bSuccess)
        {
          msg.GetReader() >> s_pWidget->m_Info;
          s_pWidget->m_bInfoValid = uiGraphIdentity == s_pWidget->m_uiSelectedGraphId;
        }
        else if (uiGraphIdentity == s_pWidget->m_uiSelectedGraphId)
        {
          s_pWidget->m_Info = WRenderGraphInspectionInfo();
          s_pWidget->m_bInfoValid = false;
        }
        s_pWidget->m_bUpdateUi = true;
      }
      break;
      case 'RESP':
        msg.GetReader() >> uiVersion;
        msg.GetReader() >> s_pWidget->m_Response;
        s_pWidget->m_bUpdateUi = true;
        break;
      case ' CLR':
        s_pWidget->ResetStats();
        break;
      default:
        break;
    }
  }
}
