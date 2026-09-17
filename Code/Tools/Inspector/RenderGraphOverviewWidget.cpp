#include <Inspector/InspectorPCH.h>

#include <Foundation/Math/ColorScheme.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <Inspector/RenderGraphOverviewWidget.moc.h>
#include <RendererFoundation/RendererReflection.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPolygon>

#include <algorithm>

namespace
{
  QColor GetAccessColor(WBitflags<WGALResourceState> access)
  {
    const bool bRead = access.IsAnySet(WGALResourceState::AllReadStates);
    const bool bWrite = access.IsAnySet(WGALResourceState::AllWriteStates);

    if (access.IsSet(WGALResourceState::RenderTarget))
      return WToQtColor(WColorScheme::LightUI(WColorScheme::Orange));

    if (access.IsSet(WGALResourceState::DepthStencilWrite))
      return WToQtColor(WColorScheme::LightUI(WColorScheme::Grape));
    if (access.IsSet(WGALResourceState::DepthStencilRead))
      return WToQtColor(WColorScheme::DarkUI(WColorScheme::Cyan));

    if (bWrite)
      return WToQtColor(WColorScheme::LightUI(WColorScheme::Red));
    if (bRead)
      return WToQtColor(WColorScheme::LightUI(WColorScheme::Lime));

    return WToQtColor(WColorScheme::LightUI(WColorScheme::Gray));
  }

  QString MakePassTooltip(WUInt32 uiPassIndex, const WRenderGraphInspectionInfo::PassInfo& pass)
  {
    WStringBuilder text;
    text.SetFormat("Pass {}\n{}\nQueue: {}\n{}", uiPassIndex, pass.m_sName, WArgEnum(pass.m_QueueType), pass.m_bAlive ? "Alive" : "Culled");
    return WMakeQString(text);
  }

  QString MakeTextureTooltip(WUInt32 uiTextureIndex, const WRenderGraphInspectionInfo::TextureResourceInfo& texture)
  {
    WStringBuilder text;
    text.SetFormat("Texture {} {}\nFormat: {}\nSize: {}x{}", uiTextureIndex,
      texture.m_bImported ? "(Imported)" : "(Transient)", WArgEnum(texture.m_Desc.m_Format), texture.m_Desc.m_uiWidth, texture.m_Desc.m_uiHeight);
    if (texture.m_Desc.m_uiDepth > 1)
      text.AppendFormat("x{}", texture.m_Desc.m_uiDepth);
    if (texture.m_Desc.m_uiArraySize > 1)
      text.AppendFormat("\nSlices: {}", texture.m_Desc.m_uiArraySize);
    if (texture.m_Desc.m_uiMipLevelCount > 1)
      text.AppendFormat("\nMipLevels: {}", texture.m_Desc.m_uiMipLevelCount);
    if (texture.m_Desc.m_SampleCount != WGALMSAASampleCount::None)
      text.AppendFormat("\nMSAA Samples: {}", texture.m_Desc.m_SampleCount.GetValue());

    text.AppendFormat("\nFlags: {}", WArgEnum(texture.m_Desc.m_TextureFlags));
    text.AppendFormat("\nPasses: {} - {}\nResolved: {}", texture.m_uiFirstUsePassIndex, texture.m_uiLastUsePassIndex, texture.m_uiResolvedIndex);

    return WMakeQString(text);
  }

  QString MakeBufferTooltip(WUInt32 uiBufferIndex, const WRenderGraphInspectionInfo::BufferResourceInfo& buffer)
  {
    WStringBuilder text;
    text.SetFormat("Buffer {} {}\nSize: {} bytes\nStruct: {} bytes\nFormat: {}", uiBufferIndex,
      buffer.m_bImported ? "(Imported)" : "(Transient)", buffer.m_Desc.m_uiTotalSize, buffer.m_Desc.m_uiStructSize, WArgEnum(buffer.m_Desc.m_Format));

    text.AppendFormat("\nFlags: {}", WArgEnum(buffer.m_Desc.m_BufferFlags));
    text.AppendFormat("\nPasses: {} - {}\nResolved: {}", buffer.m_uiFirstUsePassIndex, buffer.m_uiLastUsePassIndex, buffer.m_uiResolvedIndex);

    return WMakeQString(text);
  }

  void AppendAccessTooltipDetails(WStringBuilder& ref_sText, const WRenderGraphInspectionInfo::AccessInfo& access)
  {
    ref_sText.AppendFormat("\nAccess: {}", WArgEnum(access.m_Access));

    if (access.m_bIsTexture)
    {
      const auto& range = access.m_TextureRange;
      ref_sText.AppendFormat("\nRange: mip {}+{}, slice {}+{}", range.m_uiBaseMipLevel, range.m_uiMipLevels,
        range.m_uiBaseArraySlice, range.m_uiArraySlices);
    }
  }
} // namespace

WQtRenderGraphOverviewWidget::WQtRenderGraphOverviewWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setMinimumSize(600, 300);
  setMouseTracking(true);
}

void WQtRenderGraphOverviewWidget::SetInfo(WUInt64 uiSelectedGraphId, const WRenderGraphInspectionInfo& info)
{
  if (m_uiSelectedGraphId != uiSelectedGraphId)
  {
    // While a render graph is selected, we allow the size to only grow to not have the scroll bars flickers (see below).
    // This is reset here when we change to a different graph.
    m_uiSelectedGraphId = uiSelectedGraphId;
    m_MinContentSize = QSize(600, 300);
  }

  m_Info = info;

  // Build caches
  WUInt16 uiMaxResolvedTexture = 0;
  for (const auto& texture : m_Info.m_Textures)
  {
    if (texture.m_uiResolvedIndex != 0xFFFF)
      uiMaxResolvedTexture = WMath::Max(uiMaxResolvedTexture, texture.m_uiResolvedIndex);
  }
  m_uiTextureColumnCount = m_Info.m_Textures.IsEmpty() ? 0 : uiMaxResolvedTexture + 1;

  WUInt16 uiMaxResolvedBuffer = 0;
  for (const auto& buffer : m_Info.m_Buffers)
  {
    if (buffer.m_uiResolvedIndex != 0xFFFF)
      uiMaxResolvedBuffer = WMath::Max(uiMaxResolvedBuffer, buffer.m_uiResolvedIndex);
  }
  m_uiBufferColumnCount = m_Info.m_Buffers.IsEmpty() ? 0 : uiMaxResolvedBuffer + 1;
  m_uiTotalResourceColumnCount = m_uiTextureColumnCount + m_uiBufferColumnCount;

  m_TextureColumnResources.Clear();
  for (WUInt32 i = 0; i < m_Info.m_Textures.GetCount(); ++i)
  {
    const WUInt16 uiResolvedIndex = m_Info.m_Textures[i].m_uiResolvedIndex;
    if (uiResolvedIndex != 0xFFFF)
      m_TextureColumnResources.PushBack({uiResolvedIndex, (WUInt16)i});
  }
  std::sort(begin(m_TextureColumnResources), end(m_TextureColumnResources), [](const TextureColumnResource& lhs, const TextureColumnResource& rhs)
    { return lhs.m_uiResolvedIndex < rhs.m_uiResolvedIndex; });

  m_BufferColumnResources.Clear();
  for (WUInt32 i = 0; i < m_Info.m_Buffers.GetCount(); ++i)
  {
    const WUInt16 uiResolvedIndex = m_Info.m_Buffers[i].m_uiResolvedIndex;
    if (uiResolvedIndex != 0xFFFF)
      m_BufferColumnResources.PushBack({uiResolvedIndex, (WUInt16)i});
  }
  std::sort(begin(m_BufferColumnResources), end(m_BufferColumnResources), [](const BufferColumnResource& lhs, const BufferColumnResource& rhs)
    { return lhs.m_uiResolvedIndex < rhs.m_uiResolvedIndex; });

  // Compute size
  const QSize required(s_iPassLabelWidth + s_iContentPadding + WMath::Max<WInt32>(1, (WInt32)m_uiTotalResourceColumnCount) * s_iCellSize,
    s_iHeaderHeight + s_iContentPadding + WMath::Max<WInt32>(1, (WInt32)m_Info.m_Passes.GetCount()) * s_iCellSize);
  m_MinContentSize.setWidth(WMath::Max(m_MinContentSize.width(), required.width()));
  m_MinContentSize.setHeight(WMath::Max(m_MinContentSize.height(), required.height()));
  setMinimumSize(m_MinContentSize);
  update();
}

void WQtRenderGraphOverviewWidget::SetRequest(const WRenderGraphObserverRequest& request)
{
  m_sObservedPassName = request.m_sPassName;
  m_uiObservedAccessIndex = request.m_uiAccessIndex;
  update();
}

void WQtRenderGraphOverviewWidget::Clear()
{
  m_Info = WRenderGraphInspectionInfo();
  m_uiTextureColumnCount = 0;
  m_uiBufferColumnCount = 0;
  m_uiTotalResourceColumnCount = 0;
  m_TextureColumnResources.Clear();
  m_BufferColumnResources.Clear();
  m_sObservedPassName.Clear();
  m_uiObservedAccessIndex = 0;
  m_MinContentSize = QSize(600, 300);
  setMinimumSize(m_MinContentSize);
  update();
}

void WQtRenderGraphOverviewWidget::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.fillRect(rect(), palette().color(QPalette::Dark));
  painter.setRenderHint(QPainter::Antialiasing, false);

  if (m_Info.m_Passes.IsEmpty() || m_uiTotalResourceColumnCount == 0)
  {
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(8, 18, "Graph has no passes or resources.");
    return;
  }

  {
    painter.setPen(palette().color(QPalette::Text));
    WStringBuilder text;
    text.SetFormat("Passes: {}  Textures: {}  Buffers: {}  Accesses: {}", m_Info.m_Passes.GetCount(), m_Info.m_Textures.GetCount(), m_Info.m_Buffers.GetCount(),
      m_Info.m_Accesses.GetCount());
    painter.drawText(8, 18, WMakeQString(text));
  }

  DrawResourceNames(painter);
  DrawPassNames(painter);
  DrawGrid(painter);
  DrawResourceLifetimes(painter);
  DrawAccesses(painter);
}

void WQtRenderGraphOverviewWidget::mousePressEvent(QMouseEvent* e)
{
  const HoverInfo hover = GetHoverInfo(e->pos());
  const WRenderGraphInspectionInfo::AccessInfo* pAccess = FindAccess(hover);
  if (pAccess != nullptr && pAccess->m_bIsTexture)
  {
    Q_EMIT AccessSelected(pAccess->m_uiPassIndex, pAccess->m_uiAccessIndex);
    return;
  }

  Q_EMIT AccessDeselected();
}

void WQtRenderGraphOverviewWidget::mouseMoveEvent(QMouseEvent* e)
{
  setToolTip(MakeHoverTooltip(GetHoverInfo(e->pos())));
  QWidget::mouseMoveEvent(e);
}

WQtRenderGraphOverviewWidget::HoverInfo WQtRenderGraphOverviewWidget::GetHoverInfo(const QPoint& pos) const
{
  HoverInfo hover;
  hover.m_Position = pos;

  if (pos.x() < 0 || pos.y() < 0)
    return hover;

  if (pos.y() < s_iHeaderHeight)
  {
    if (pos.x() >= s_iPassLabelWidth)
    {
      const WUInt32 uiResourceColumn = (WUInt32)((pos.x() - s_iPassLabelWidth) / s_iCellSize);
      if (uiResourceColumn < m_uiTextureColumnCount)
      {
        hover.m_Area = HoverInfo::Area::TextureHeader;
        hover.m_uiResourceColumn = (WUInt16)uiResourceColumn;
        hover.m_bIsTexture = true;
      }
      else if (uiResourceColumn < m_uiTotalResourceColumnCount)
      {
        hover.m_Area = HoverInfo::Area::BufferHeader;
        hover.m_uiResourceColumn = (WUInt16)(uiResourceColumn - m_uiTextureColumnCount);
        hover.m_bIsTexture = false;
      }
    }

    return hover;
  }

  const WUInt32 uiPassIndex = (WUInt32)((pos.y() - s_iHeaderHeight) / s_iCellSize);
  if (uiPassIndex >= m_Info.m_Passes.GetCount())
    return hover;

  hover.m_uiPassIndex = (WUInt16)uiPassIndex;

  if (pos.x() < s_iPassLabelWidth)
  {
    hover.m_Area = HoverInfo::Area::PassHeader;
    return hover;
  }

  const WUInt32 uiResourceColumn = (WUInt32)((pos.x() - s_iPassLabelWidth) / s_iCellSize);
  if (uiResourceColumn >= m_uiTotalResourceColumnCount)
  {
    hover.m_Area = HoverInfo::Area::None;
    hover.m_uiPassIndex = 0xFFFF;
    return hover;
  }

  hover.m_Area = HoverInfo::Area::ResourceCell;
  hover.m_uiResourceColumn = (WUInt16)uiResourceColumn;
  hover.m_bIsTexture = uiResourceColumn < m_uiTextureColumnCount;
  return hover;
}

const WRenderGraphInspectionInfo::AccessInfo* WQtRenderGraphOverviewWidget::FindAccess(const HoverInfo& hover, bool bLastMatch) const
{
  if (hover.m_Area != HoverInfo::Area::ResourceCell || hover.m_uiPassIndex == 0xFFFF || hover.m_uiResourceColumn == 0xFFFF)
    return nullptr;

  auto it = std::lower_bound(begin(m_Info.m_Accesses), end(m_Info.m_Accesses), hover.m_uiPassIndex,
    [](const WRenderGraphInspectionInfo::AccessInfo& access, WUInt16 uiPassIndex)
    { return access.m_uiPassIndex < uiPassIndex; });

  const WRenderGraphInspectionInfo::AccessInfo* pMatch = nullptr;
  for (; it != end(m_Info.m_Accesses) && it->m_uiPassIndex == hover.m_uiPassIndex; ++it)
  {
    if (it->m_bIsTexture != hover.m_bIsTexture)
      continue;

    if (it->m_bIsTexture)
    {
      const auto& texture = m_Info.m_Textures[it->m_uiResourceIndex];
      if (texture.m_uiResolvedIndex == hover.m_uiResourceColumn)
      {
        pMatch = &(*it);
        if (!bLastMatch)
          return pMatch;
      }
    }
    else
    {
      const auto& buffer = m_Info.m_Buffers[it->m_uiResourceIndex];
      if (m_uiTextureColumnCount + buffer.m_uiResolvedIndex == hover.m_uiResourceColumn)
      {
        pMatch = &(*it);
        if (!bLastMatch)
          return pMatch;
      }
    }
  }

  return pMatch;
}

QString WQtRenderGraphOverviewWidget::MakeAccessesTooltip(const HoverInfo& hover) const
{
  if (hover.m_Area != HoverInfo::Area::ResourceCell || hover.m_uiPassIndex == 0xFFFF || hover.m_uiResourceColumn == 0xFFFF)
    return QString();

  auto it = std::lower_bound(begin(m_Info.m_Accesses), end(m_Info.m_Accesses), hover.m_uiPassIndex,
    [](const WRenderGraphInspectionInfo::AccessInfo& access, WUInt16 uiPassIndex)
    { return access.m_uiPassIndex < uiPassIndex; });

  WStringBuilder tooltip;
  bool bHasAccess = false;
  // Iterate through all access in this pass
  for (; it != end(m_Info.m_Accesses) && it->m_uiPassIndex == hover.m_uiPassIndex; ++it)
  {
    if (it->m_bIsTexture != hover.m_bIsTexture)
      continue;

    WUInt32 uiResourceColumn = 0;
    if (it->m_bIsTexture)
    {
      const auto& texture = m_Info.m_Textures[it->m_uiResourceIndex];
      uiResourceColumn = texture.m_uiResolvedIndex;
    }
    else
    {
      const auto& buffer = m_Info.m_Buffers[it->m_uiResourceIndex];
      uiResourceColumn = m_uiTextureColumnCount + buffer.m_uiResolvedIndex;
    }

    if (uiResourceColumn != hover.m_uiResourceColumn)
      continue;

    if (!bHasAccess)
    {
      tooltip.SetFormat("Pass: {}", m_Info.m_Passes[it->m_uiPassIndex].m_sName);
      if (it->m_bIsTexture)
        tooltip.AppendFormat("\nResource: Texture {}", it->m_uiResourceIndex);
      else
        tooltip.AppendFormat("\nResource: Buffer {}", it->m_uiResourceIndex);
      bHasAccess = true;
    }
    else
    {
      tooltip.Append("\n");
    }

    AppendAccessTooltipDetails(tooltip, *it);
  }

  return bHasAccess ? WMakeQString(tooltip) : QString();
}

WUInt16 WQtRenderGraphOverviewWidget::FindTextureLifetimeResource(const HoverInfo& hover) const
{
  if (hover.m_Area != HoverInfo::Area::ResourceCell || !hover.m_bIsTexture)
    return 0xFFFF;

  auto it = std::lower_bound(begin(m_TextureColumnResources), end(m_TextureColumnResources), hover.m_uiResourceColumn,
    [](const TextureColumnResource& resource, WUInt16 uiResolvedIndex)
    { return resource.m_uiResolvedIndex < uiResolvedIndex; });

  for (; it != end(m_TextureColumnResources) && it->m_uiResolvedIndex == hover.m_uiResourceColumn; ++it)
  {
    const WUInt16 uiTextureIndex = it->m_uiTextureIndex;
    const auto& texture = m_Info.m_Textures[uiTextureIndex];
    if (texture.m_uiFirstUsePassIndex == 0xFFFF || texture.m_uiResolvedIndex == 0xFFFF)
      continue;

    if (GetLifetimeRect(texture.m_uiFirstUsePassIndex, texture.m_uiLastUsePassIndex, texture.m_uiResolvedIndex).contains(hover.m_Position))
      return uiTextureIndex;
  }

  return 0xFFFF;
}

WUInt16 WQtRenderGraphOverviewWidget::FindBufferLifetimeResource(const HoverInfo& hover) const
{
  if (hover.m_Area != HoverInfo::Area::ResourceCell || hover.m_bIsTexture)
    return 0xFFFF;

  const WUInt32 uiBufferColumn = hover.m_uiResourceColumn - m_uiTextureColumnCount;
  auto it = std::lower_bound(begin(m_BufferColumnResources), end(m_BufferColumnResources), uiBufferColumn,
    [](const BufferColumnResource& resource, WUInt32 uiResolvedIndex)
    { return resource.m_uiResolvedIndex < uiResolvedIndex; });

  for (; it != end(m_BufferColumnResources) && it->m_uiResolvedIndex == uiBufferColumn; ++it)
  {
    const WUInt16 uiBufferIndex = it->m_uiBufferIndex;
    const auto& buffer = m_Info.m_Buffers[uiBufferIndex];
    if (buffer.m_uiFirstUsePassIndex == 0xFFFF || buffer.m_uiResolvedIndex == 0xFFFF)
      continue;

    if (GetLifetimeRect(buffer.m_uiFirstUsePassIndex, buffer.m_uiLastUsePassIndex, m_uiTextureColumnCount + buffer.m_uiResolvedIndex).contains(hover.m_Position))
      return uiBufferIndex;
  }

  return 0xFFFF;
}

QString WQtRenderGraphOverviewWidget::MakeHoverTooltip(const HoverInfo& hover) const
{
  switch (hover.m_Area)
  {
    case HoverInfo::Area::PassHeader:
      return MakePassTooltip(hover.m_uiPassIndex, m_Info.m_Passes[hover.m_uiPassIndex]);

    case HoverInfo::Area::TextureHeader:
      return MakeTextureColumnTooltip(hover.m_uiResourceColumn);

    case HoverInfo::Area::BufferHeader:
      return MakeBufferColumnTooltip(hover.m_uiResourceColumn);

    case HoverInfo::Area::ResourceCell:
    {
      QString accessTooltip = MakeAccessesTooltip(hover);
      if (!accessTooltip.isEmpty())
        return accessTooltip;

      if (hover.m_bIsTexture)
      {
        const WUInt16 uiTextureIndex = FindTextureLifetimeResource(hover);
        if (uiTextureIndex != 0xFFFF)
          return MakeTextureTooltip(uiTextureIndex, m_Info.m_Textures[uiTextureIndex]);
      }
      else
      {
        const WUInt16 uiBufferIndex = FindBufferLifetimeResource(hover);
        if (uiBufferIndex != 0xFFFF)
          return MakeBufferTooltip(uiBufferIndex, m_Info.m_Buffers[uiBufferIndex]);
      }
    }
    break;

    case HoverInfo::Area::None:
      break;
  }

  return QString();
}

QString WQtRenderGraphOverviewWidget::MakeTextureColumnTooltip(WUInt32 uiResolvedIndex) const
{
  auto it = std::lower_bound(begin(m_TextureColumnResources), end(m_TextureColumnResources), uiResolvedIndex,
    [](const TextureColumnResource& resource, WUInt32 uiResolvedIndex)
    { return resource.m_uiResolvedIndex < uiResolvedIndex; });

  QString tooltip;
  for (; it != end(m_TextureColumnResources) && it->m_uiResolvedIndex == uiResolvedIndex; ++it)
  {
    if (!tooltip.isEmpty())
      tooltip += "\n\n";
    tooltip += MakeTextureTooltip(it->m_uiTextureIndex, m_Info.m_Textures[it->m_uiTextureIndex]);
  }
  return tooltip;
}

QString WQtRenderGraphOverviewWidget::MakeBufferColumnTooltip(WUInt32 uiResolvedIndex) const
{
  auto it = std::lower_bound(begin(m_BufferColumnResources), end(m_BufferColumnResources), uiResolvedIndex,
    [](const BufferColumnResource& resource, WUInt32 uiResolvedIndex)
    { return resource.m_uiResolvedIndex < uiResolvedIndex; });

  QString tooltip;
  for (; it != end(m_BufferColumnResources) && it->m_uiResolvedIndex == uiResolvedIndex; ++it)
  {
    if (!tooltip.isEmpty())
      tooltip += "\n\n";
    tooltip += MakeBufferTooltip(it->m_uiBufferIndex, m_Info.m_Buffers[it->m_uiBufferIndex]);
  }
  return tooltip;
}

QRect WQtRenderGraphOverviewWidget::GetCellRect(WUInt32 uiPassIndex, WUInt32 uiResourceColumn) const
{
  return QRect(s_iPassLabelWidth + (WInt32)uiResourceColumn * s_iCellSize, s_iHeaderHeight + (WInt32)uiPassIndex * s_iCellSize, s_iCellSize, s_iCellSize);
}

void WQtRenderGraphOverviewWidget::DrawResourceNames(QPainter& painter)
{
  for (WUInt32 column = 0; column < m_uiTextureColumnCount; ++column)
  {
    const WInt32 x = s_iPassLabelWidth + (WInt32)column * s_iCellSize;
    painter.fillRect(QRect(x, s_iHeaderHeight - s_iCellSize, s_iCellSize, s_iCellSize), column % 2 == 0 ? palette().color(QPalette::Mid) : palette().color(QPalette::AlternateBase));
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(QRect(x, s_iHeaderHeight - s_iCellSize, s_iCellSize, s_iCellSize), Qt::AlignCenter, QString::number(column));
  }

  for (WUInt32 column = 0; column < m_uiBufferColumnCount; ++column)
  {
    const WUInt32 resourceColumn = m_uiTextureColumnCount + column;
    const WInt32 x = s_iPassLabelWidth + (WInt32)resourceColumn * s_iCellSize;
    painter.fillRect(QRect(x, s_iHeaderHeight - s_iCellSize, s_iCellSize, s_iCellSize), resourceColumn % 2 == 0 ? palette().color(QPalette::Mid) : palette().color(QPalette::AlternateBase));
    painter.setPen(WToQtColor(WColorScheme::LightUI(WColorScheme::Cyan)));
    painter.drawText(QRect(x, s_iHeaderHeight - s_iCellSize, s_iCellSize, s_iCellSize), Qt::AlignCenter, QString::number(resourceColumn));
  }
}

void WQtRenderGraphOverviewWidget::DrawPassNames(QPainter& painter)
{
  for (WUInt32 passIndex = 0; passIndex < m_Info.m_Passes.GetCount(); ++passIndex)
  {
    const auto& pass = m_Info.m_Passes[passIndex];
    const QRect labelRect(0, s_iHeaderHeight + (WInt32)passIndex * s_iCellSize, s_iPassLabelWidth, s_iCellSize);
    const QColor textColor = !pass.m_bAlive ? palette().color(QPalette::Disabled, QPalette::Text) : pass.m_QueueType == WGALQueueType::Compute  ? WToQtColor(WColorScheme::LightUI(WColorScheme::Green))
                                                                                                  : pass.m_QueueType == WGALQueueType::Transfer ? WToQtColor(WColorScheme::LightUI(WColorScheme::Blue))
                                                                                                                                                 : palette().color(QPalette::Text);
    painter.fillRect(labelRect, passIndex % 2 == 0 ? palette().color(QPalette::Base) : palette().color(QPalette::AlternateBase));
    painter.setPen(textColor);
    painter.drawText(labelRect.adjusted(6, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, WMakeQString(pass.m_sName));
  }
}

void WQtRenderGraphOverviewWidget::DrawGrid(QPainter& painter)
{
  const WInt32 iGridWidth = (WInt32)m_uiTotalResourceColumnCount * s_iCellSize;
  const WInt32 iGridHeight = (WInt32)m_Info.m_Passes.GetCount() * s_iCellSize;
  const QRect gridRect(s_iPassLabelWidth, s_iHeaderHeight, iGridWidth, iGridHeight);

  painter.fillRect(gridRect, palette().color(QPalette::Dark));
  painter.setPen(QPen(palette().color(QPalette::Midlight), 1));
  for (WUInt32 column = 0; column <= m_uiTotalResourceColumnCount; ++column)
  {
    const WInt32 x = s_iPassLabelWidth + (WInt32)column * s_iCellSize;
    painter.drawLine(x, s_iHeaderHeight, x, s_iHeaderHeight + iGridHeight);
  }
  for (WUInt32 passIndex = 0; passIndex <= m_Info.m_Passes.GetCount(); ++passIndex)
  {
    const WInt32 y = s_iHeaderHeight + (WInt32)passIndex * s_iCellSize;
    painter.drawLine(s_iPassLabelWidth, y, s_iPassLabelWidth + iGridWidth, y);
  }
}

void WQtRenderGraphOverviewWidget::DrawResourceLifetimes(QPainter& painter)
{
  painter.setBrush(Qt::NoBrush);
  for (WUInt32 i = 0; i < m_Info.m_Textures.GetCount(); ++i)
  {
    const auto& texture = m_Info.m_Textures[i];
    if (texture.m_uiFirstUsePassIndex == 0xFFFF || texture.m_uiResolvedIndex == 0xFFFF)
      continue;

    const QRect rect = GetLifetimeRect(texture.m_uiFirstUsePassIndex, texture.m_uiLastUsePassIndex, texture.m_uiResolvedIndex);
    painter.setPen(QPen(texture.m_bImported ? WToQtColor(WColorScheme::LightUI(WColorScheme::Orange)) : WToQtColor(WColorScheme::DarkUI(WColorScheme::Yellow)), 1));
    painter.drawRect(rect);
  }

  for (WUInt32 i = 0; i < m_Info.m_Buffers.GetCount(); ++i)
  {
    const auto& buffer = m_Info.m_Buffers[i];
    if (buffer.m_uiFirstUsePassIndex == 0xFFFF || buffer.m_uiResolvedIndex == 0xFFFF)
      continue;

    const QRect rect = GetLifetimeRect(buffer.m_uiFirstUsePassIndex, buffer.m_uiLastUsePassIndex, m_uiTextureColumnCount + buffer.m_uiResolvedIndex);
    painter.setPen(QPen(buffer.m_bImported ? WToQtColor(WColorScheme::LightUI(WColorScheme::Cyan)) : WToQtColor(WColorScheme::DarkUI(WColorScheme::Blue)), 1));
    painter.drawRect(rect);
  }
}

QRect WQtRenderGraphOverviewWidget::GetLifetimeRect(WUInt32 uiFirstPass, WUInt32 uiLastPass, WUInt32 uiResourceColumn) const
{
  const WInt32 x = s_iPassLabelWidth + (WInt32)uiResourceColumn * s_iCellSize + s_iLifetimeGap;
  const WInt32 y = s_iHeaderHeight + (WInt32)uiFirstPass * s_iCellSize + s_iLifetimeGap;
  const WInt32 width = s_iCellSize - 2 * s_iLifetimeGap;
  const WInt32 height = ((WInt32)uiLastPass - (WInt32)uiFirstPass + 1) * s_iCellSize - 2 * s_iLifetimeGap;
  return QRect(x, y, width, height);
}

void WQtRenderGraphOverviewWidget::DrawAccesses(QPainter& painter)
{
  WDynamicArray<WBitflags<WGALResourceState>> accessMasks;
  accessMasks.SetCount(m_uiTotalResourceColumnCount);

  auto it = begin(m_Info.m_Accesses);
  for (WUInt32 passIndex = 0; passIndex < m_Info.m_Passes.GetCount(); ++passIndex)
  {
    WMemoryUtils::ZeroFill(accessMasks.GetData(), accessMasks.GetCount());
    WUInt32 uiObservedResourceColumn = 0xFFFFFFFF;

    // m_Accesses is sorted by pass index first.
    while (it != end(m_Info.m_Accesses) && it->m_uiPassIndex < passIndex)
    {
      ++it;
    }

    // Accumulate all resource accesses on each resource
    for (; it != end(m_Info.m_Accesses) && it->m_uiPassIndex == passIndex; ++it)
    {
      WUInt32 resourceColumn = 0;
      if (it->m_bIsTexture)
      {
        resourceColumn = m_Info.m_Textures[it->m_uiResourceIndex].m_uiResolvedIndex;
      }
      else
      {
        resourceColumn = m_uiTextureColumnCount + m_Info.m_Buffers[it->m_uiResourceIndex].m_uiResolvedIndex;
      }

      accessMasks[resourceColumn] |= it->m_Access;

      if (it->m_bIsTexture &&
          !m_sObservedPassName.IsEmpty() &&
          m_Info.m_Passes[it->m_uiPassIndex].m_sName == m_sObservedPassName &&
          it->m_uiAccessIndex == m_uiObservedAccessIndex)
      {
        uiObservedResourceColumn = resourceColumn;
      }
    }

    // Draw accumulated accesses
    painter.setPen(Qt::NoPen);
    for (WUInt32 resourceColumn = 0; resourceColumn < accessMasks.GetCount(); ++resourceColumn)
    {
      const WBitflags<WGALResourceState> accessMask = accessMasks[resourceColumn];
      if (accessMask.IsNoFlagSet())
        continue;

      const QRect cellRect = GetCellRect(passIndex, resourceColumn);
      const QRect markerRect = cellRect.adjusted(s_iAccessPadding, s_iAccessPadding, -s_iAccessPadding, -s_iAccessPadding);
      const WBitflags<WGALResourceState> readMask = accessMask & WBitflags<WGALResourceState>(WGALResourceState::AllReadStates);
      const WBitflags<WGALResourceState> writeMask = accessMask & WBitflags<WGALResourceState>(WGALResourceState::AllWriteStates);

      if (!readMask.IsNoFlagSet() && !writeMask.IsNoFlagSet())
      {
        const QPoint topLeft = markerRect.topLeft();
        const QPoint topRight(markerRect.right() + 1, markerRect.top());
        const QPoint bottomLeft(markerRect.left(), markerRect.bottom() + 1);
        const QPoint bottomRight(markerRect.right() + 1, markerRect.bottom() + 1);

        QPolygon readTriangle;
        readTriangle << topLeft << topRight << bottomLeft;
        painter.setBrush(GetAccessColor(readMask));
        painter.drawPolygon(readTriangle);

        QPolygon writeTriangle;
        writeTriangle << bottomRight << topRight << bottomLeft;
        painter.setBrush(GetAccessColor(writeMask));
        painter.drawPolygon(writeTriangle);
      }
      else
      {
        painter.fillRect(markerRect, GetAccessColor(accessMask));
      }
    }

    if (uiObservedResourceColumn != 0xFFFFFFFF)
    {
      const QRect cellRect = GetCellRect(passIndex, uiObservedResourceColumn);
      painter.setPen(QPen(palette().color(QPalette::Highlight), 2));
      painter.setBrush(Qt::NoBrush);
      painter.drawRect(cellRect.adjusted(s_iLifetimeGap, s_iLifetimeGap, -s_iLifetimeGap, -s_iLifetimeGap));
    }
  }
}
