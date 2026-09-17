#include <Inspector/InspectorPCH.h>

#include <Foundation/Math/ColorScheme.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <Inspector/RenderGraphHistogramWidget.moc.h>

#include <QImage>
#include <QPainter>
#include <QSizePolicy>
#include <QStyle>
#include <QStyleOptionFrame>

WQtRenderGraphHistogramWidget::WQtRenderGraphHistogramWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setFixedSize(260, 96);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  m_Colors[0] = QColor();
  m_Colors[W_BIT(0)] = WToQtColor(WColorScheme::LightUI(WColorScheme::Red));
  m_Colors[W_BIT(1)] = WToQtColor(WColorScheme::LightUI(WColorScheme::Green));
  m_Colors[W_BIT(2)] = WToQtColor(WColorScheme::LightUI(WColorScheme::Blue));
  m_Colors[W_BIT(0) | W_BIT(1)] = WToQtColor(WColorScheme::LightUI(WColorScheme::Yellow));
  m_Colors[W_BIT(0) | W_BIT(2)] = WToQtColor(WColorScheme::LightUI(WColorScheme::Violet));
  m_Colors[W_BIT(1) | W_BIT(2)] = WToQtColor(WColorScheme::LightUI(WColorScheme::Cyan));
  m_Colors[W_BIT(0) | W_BIT(1) | W_BIT(2)] = palette().color(QPalette::Text);
  m_Colors[W_BIT(3)] = palette().color(QPalette::Midlight);
  for (WUInt32 i = 1; i < 8; ++i)
  {
    m_Colors[i + 8] = m_Colors[i].darker();
  }
}

void WQtRenderGraphHistogramWidget::SetHistogram(WArrayPtr<const WUInt8> histogram)
{
  m_Histogram.SetCount(histogram.GetCount());
  for (WUInt32 i = 0; i < histogram.GetCount(); ++i)
  {
    m_Histogram[i] = histogram[i];
  }

  if (!m_bRepaintPending)
  {
    m_bRepaintPending = true;
    update();
  }
}

void WQtRenderGraphHistogramWidget::Clear()
{
  m_Histogram.Clear();
  if (!m_bRepaintPending)
  {
    m_bRepaintPending = true;
    update();
  }
}

void WQtRenderGraphHistogramWidget::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, false);

  QStyleOptionFrame option;
  option.initFrom(this);
  option.rect = rect();
  option.lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, this);
  option.midLineWidth = 0;
  option.state |= QStyle::State_Sunken;

  style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, this);

  const QRect plotRect = rect().adjusted(option.lineWidth, option.lineWidth, -option.lineWidth, -option.lineWidth);
  painter.fillRect(plotRect, palette().color(QPalette::Dark));

  if (m_bRepaintPending)
  {
    m_bRepaintPending = false;
    UpdateHistogram(plotRect);
  }
  // TODO: DPI scaling?
  painter.drawImage(plotRect.adjusted(2, 2, -2, -2), m_HistogramImage);
}

void WQtRenderGraphHistogramWidget::UpdateHistogram(const QRect plotRect)
{
  if (m_HistogramImage.isNull())
  {
    m_HistogramImage = QImage(256, plotRect.height() - 4, QImage::Format_ARGB32_Premultiplied);
  }
  m_HistogramImage.fill(Qt::transparent);
  if (m_Histogram.IsEmpty())
    return;



  const WUInt32 uiImageHeight = (WUInt32)m_HistogramImage.height();
  const WUInt32 uiMaxImageHeight = uiImageHeight - 1u;
  for (WUInt32 bin = 0; bin < 256; ++bin)
  {
    // TODO: Interleave rgba?
    const WUInt32 uiR = m_Histogram[bin];
    const WUInt32 uiG = m_Histogram[256 + bin];
    const WUInt32 uiB = m_Histogram[512 + bin];
    const WUInt32 uiA = m_Histogram[768 + bin];
    const WUInt32 uiRHeight = (uiR * uiMaxImageHeight + 254u) / 255u;
    const WUInt32 uiGHeight = (uiG * uiMaxImageHeight + 254u) / 255u;
    const WUInt32 uiBHeight = (uiB * uiMaxImageHeight + 254u) / 255u;
    const WUInt32 uiAHeight = (uiA * uiMaxImageHeight + 254u) / 255u;
    const WUInt32 uiMaxHeight = WMath::Min(uiMaxImageHeight, WMath::Max(uiRHeight, uiGHeight, uiBHeight, uiAHeight));
    for (WUInt32 y = 0; y <= uiMaxHeight; ++y)
    {
      const bool bR = y <= uiRHeight;
      const bool bG = y <= uiGHeight;
      const bool bB = y <= uiBHeight;
      const bool bA = y <= uiAHeight;
      const WUInt32 uiLookup = W_BIT(0) * bR | W_BIT(1) * bG | W_BIT(2) * bB | W_BIT(3) * bA;
      m_HistogramImage.setPixelColor((WInt32)bin, (WInt32)uiImageHeight - (WInt32)y - 1, m_Colors[uiLookup]);
    }
  }
}
