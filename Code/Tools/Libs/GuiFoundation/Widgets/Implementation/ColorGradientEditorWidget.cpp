#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>

WQtColorGradientEditorWidget::WQtColorGradientEditorWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  GradientWidget->setColorGradientData(&m_Gradient);
  GradientWidget->setEditMode(true);
  GradientWidget->FrameExtents();

  GradientWidget->setShowColorCPs(true);
  GradientWidget->setShowAlphaCPs(true);
  GradientWidget->setShowIntensityCPs(true);
  GradientWidget->setShowCoords(true, true);

  on_GradientWidget_selectionChanged(-1, -1, -1);

  connect(
    GradientWidget, &WQtColorGradientWidget::addColorCp, this, [this](double x, const WColorGammaUB& color)
    { Q_EMIT ColorCpAdded(x, color); });
  connect(GradientWidget, &WQtColorGradientWidget::moveColorCpToPos, this, [this](WInt32 iIdx, double x)
    { Q_EMIT ColorCpMoved(iIdx, x); });
  connect(GradientWidget, &WQtColorGradientWidget::deleteColorCp, this, [this](WInt32 iIdx)
    { Q_EMIT ColorCpDeleted(iIdx); });

  connect(GradientWidget, &WQtColorGradientWidget::addAlphaCp, this, [this](double x, WUInt8 uiAlpha)
    { Q_EMIT AlphaCpAdded(x, uiAlpha); });
  connect(GradientWidget, &WQtColorGradientWidget::moveAlphaCpToPos, this, [this](WInt32 iIdx, double x)
    { Q_EMIT AlphaCpMoved(iIdx, x); });
  connect(GradientWidget, &WQtColorGradientWidget::deleteAlphaCp, this, [this](WInt32 iIdx)
    { Q_EMIT AlphaCpDeleted(iIdx); });

  connect(
    GradientWidget, &WQtColorGradientWidget::addIntensityCp, this, [this](double x, float fIntensity)
    { Q_EMIT IntensityCpAdded(x, fIntensity); });
  connect(GradientWidget, &WQtColorGradientWidget::moveIntensityCpToPos, this, [this](WInt32 iIdx, double x)
    { Q_EMIT IntensityCpMoved(iIdx, x); });
  connect(GradientWidget, &WQtColorGradientWidget::deleteIntensityCp, this, [this](WInt32 iIdx)
    { Q_EMIT IntensityCpDeleted(iIdx); });

  connect(GradientWidget, &WQtColorGradientWidget::beginOperation, this, [this]()
    { Q_EMIT BeginOperation(); });
  connect(GradientWidget, &WQtColorGradientWidget::endOperation, this, [this](bool bCommit)
    { Q_EMIT EndOperation(bCommit); });

  connect(GradientWidget, &WQtColorGradientWidget::triggerPickColor, this, [this]()
    { on_ButtonColor_clicked(); });
}


WQtColorGradientEditorWidget::~WQtColorGradientEditorWidget() = default;


void WQtColorGradientEditorWidget::SetColorGradient(const WColorGradient& gradient)
{
  bool clearSelection = false;

  // clear selection if the number of control points has changed
  {
    WUInt32 numRgb = 0xFFFFFFFF, numRgb2 = 0xFFFFFFFF;
    WUInt32 numAlpha = 0xFFFFFFFF, numAlpha2 = 0xFFFFFFFF;
    WUInt32 numIntensity = 0xFFFFFFFF, numIntensity2 = 0xFFFFFFFF;

    gradient.GetNumControlPoints(numRgb, numAlpha, numIntensity);
    m_Gradient.GetNumControlPoints(numRgb2, numAlpha2, numIntensity2);

    if (numRgb != numRgb2 || numAlpha != numAlpha2 || numIntensity != numIntensity2)
      clearSelection = true;
  }

  // const bool wasEmpty = m_Gradient.IsEmpty();

  m_Gradient = gradient;

  {
    WQtScopedUpdatesDisabled ud(this);

    // if (wasEmpty)
    //  GradientWidget->FrameExtents();

    if (clearSelection)
      GradientWidget->ClearSelectedCP();
  }

  UpdateCpUi();

  GradientWidget->update();
}

void WQtColorGradientEditorWidget::SetScrubberPosition(WUInt64 uiTick)
{
  GradientWidget->SetScrubberPosition(WColorGradient::TickToTime(uiTick));
}

void WQtColorGradientEditorWidget::SetScrubberPosition(WTime time)
{
  GradientWidget->SetScrubberPosition(time.GetSeconds());
}

void WQtColorGradientEditorWidget::FrameGradient()
{
  GradientWidget->FrameExtents();
  GradientWidget->update();
}

void WQtColorGradientEditorWidget::on_ButtonFrame_clicked()
{
  FrameGradient();
}

void WQtColorGradientEditorWidget::on_GradientWidget_selectionChanged(WInt32 colorCP, WInt32 alphaCP, WInt32 intensityCP)
{
  // End any temporary command when selection changes
  if (m_bTemporaryTransaction)
  {
    Q_EMIT EndOperation(true);
    m_bTemporaryTransaction = false;
  }

  m_iSelectedColorCP = colorCP;
  m_iSelectedAlphaCP = alphaCP;
  m_iSelectedIntensityCP = intensityCP;

  SpinPosition->setEnabled((m_iSelectedColorCP != -1) || (m_iSelectedAlphaCP != -1) || (m_iSelectedIntensityCP != -1));

  LabelColor->setVisible(m_iSelectedColorCP != -1);
  ButtonColor->setVisible(m_iSelectedColorCP != -1);

  LabelAlpha->setVisible(m_iSelectedAlphaCP != -1);
  SpinAlpha->setVisible(m_iSelectedAlphaCP != -1);
  SliderAlpha->setVisible(m_iSelectedAlphaCP != -1);

  LabelIntensity->setVisible(m_iSelectedIntensityCP != -1);
  SpinIntensity->setVisible(m_iSelectedIntensityCP != -1);

  UpdateCpUi();
}

void WQtColorGradientEditorWidget::on_SpinPosition_valueChanged(double value)
{
  if (!m_bTemporaryTransaction)
  {
    Q_EMIT BeginOperation();
    m_bTemporaryTransaction = true;
  }

  value = WColorGradient::SnapTimeTo(value);

  if (m_iSelectedColorCP != -1)
  {
    Q_EMIT ColorCpMoved(m_iSelectedColorCP, value);
  }
  else if (m_iSelectedAlphaCP != -1)
  {
    Q_EMIT AlphaCpMoved(m_iSelectedAlphaCP, value);
  }
  else if (m_iSelectedIntensityCP != -1)
  {
    Q_EMIT IntensityCpMoved(m_iSelectedIntensityCP, value);
  }
}

void WQtColorGradientEditorWidget::on_SpinPosition_editingFinished()
{
  if (m_bTemporaryTransaction)
  {
    Q_EMIT EndOperation(true);
  }

  m_bTemporaryTransaction = false;
}


void WQtColorGradientEditorWidget::on_SpinAlpha_valueChanged(int value)
{
  if (m_iSelectedAlphaCP != -1)
  {
    Q_EMIT AlphaCpChanged(m_iSelectedAlphaCP, value);
  }
}

void WQtColorGradientEditorWidget::on_SliderAlpha_valueChanged(int value)
{
  if (m_iSelectedAlphaCP != -1)
  {
    Q_EMIT AlphaCpChanged(m_iSelectedAlphaCP, value);
  }
}


void WQtColorGradientEditorWidget::on_SliderAlpha_sliderPressed()
{
  Q_EMIT BeginOperation();
}


void WQtColorGradientEditorWidget::on_SliderAlpha_sliderReleased()
{
  Q_EMIT EndOperation(true);
}

void WQtColorGradientEditorWidget::on_SpinIntensity_valueChanged(double value)
{
  if (!m_bTemporaryTransaction)
  {
    Q_EMIT BeginOperation();
    m_bTemporaryTransaction = true;
  }

  if (m_iSelectedIntensityCP != -1)
  {
    Q_EMIT IntensityCpChanged(m_iSelectedIntensityCP, value);
  }
}

void WQtColorGradientEditorWidget::on_SpinIntensity_editingFinished()
{
  if (m_bTemporaryTransaction)
  {
    Q_EMIT EndOperation(true);
  }

  m_bTemporaryTransaction = false;
}


void WQtColorGradientEditorWidget::on_ButtonColor_clicked()
{
  if (m_iSelectedColorCP != -1)
  {
    const auto& cp = m_Gradient.GetColorControlPoint(m_iSelectedColorCP);

    m_PickColorStart = WColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
    m_PickColorCurrent = m_PickColorStart;

    Q_EMIT BeginOperation();

    WQtUiServices::GetSingleton()->ShowColorDialog(m_PickColorStart, false, false, this, SLOT(onCurrentColorChanged(const WColor&)), SLOT(onColorAccepted()), SLOT(onColorReset()));
  }
}

void WQtColorGradientEditorWidget::onCurrentColorChanged(const WColor& col)
{
  if (m_iSelectedColorCP != -1)
  {
    m_PickColorCurrent = col;

    Q_EMIT ColorCpChanged(m_iSelectedColorCP, m_PickColorCurrent);
  }
}

void WQtColorGradientEditorWidget::onColorAccepted()
{
  if (m_iSelectedColorCP != -1)
  {
    Q_EMIT ColorCpChanged(m_iSelectedColorCP, m_PickColorCurrent);

    Q_EMIT EndOperation(true);
  }
}

void WQtColorGradientEditorWidget::onColorReset()
{
  if (m_iSelectedColorCP != -1)
  {
    Q_EMIT ColorCpChanged(m_iSelectedColorCP, m_PickColorStart);

    Q_EMIT EndOperation(false);
  }
}


void WQtColorGradientEditorWidget::on_ButtonNormalize_clicked()
{
  Q_EMIT NormalizeRange();
}

void WQtColorGradientEditorWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  ButtonColor->setPalette(m_Pal);
  QWidget::showEvent(event);
}

void WQtColorGradientEditorWidget::UpdateCpUi()
{
  WQtScopedBlockSignals bs(this);
  WQtScopedUpdatesDisabled ud(this);

  WQtScopedBlockSignals bs2(SpinPosition);
  WQtScopedBlockSignals bs3(SpinAlpha);
  WQtScopedBlockSignals bs4(SliderAlpha);
  WQtScopedBlockSignals bs5(SpinIntensity);

  if (m_iSelectedColorCP != -1)
  {
    const auto& cp = m_Gradient.GetColorControlPoint(m_iSelectedColorCP);

    SpinPosition->setValue(WColorGradient::TickToTime(cp.m_iTick));

    QColor col;
    col.setRgb(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);

    ButtonColor->setAutoFillBackground(true);
    m_Pal.setColor(QPalette::Button, col);
    ButtonColor->setPalette(m_Pal);
  }

  if (m_iSelectedAlphaCP != -1)
  {
    SpinPosition->setValue(WColorGradient::TickToTime(m_Gradient.GetAlphaControlPoint(m_iSelectedAlphaCP).m_iTick));
    SpinAlpha->setValue(m_Gradient.GetAlphaControlPoint(m_iSelectedAlphaCP).m_Alpha);
    SliderAlpha->setValue(m_Gradient.GetAlphaControlPoint(m_iSelectedAlphaCP).m_Alpha);
  }

  if (m_iSelectedIntensityCP != -1)
  {
    SpinPosition->setValue(WColorGradient::TickToTime(m_Gradient.GetIntensityControlPoint(m_iSelectedIntensityCP).m_iTick));
    SpinIntensity->setValue(m_Gradient.GetIntensityControlPoint(m_iSelectedIntensityCP).m_Intensity);
  }
}
