#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>
#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>

class QMouseEvent;
class QPaintEvent;
class QPainter;

/// Visualizes render graph passes, resources, lifetimes, and access points for inspection.
class WQtRenderGraphOverviewWidget : public QWidget
{
  Q_OBJECT

public:
  explicit WQtRenderGraphOverviewWidget(QWidget* pParent = nullptr);

  void SetInfo(WUInt64 uiSelectedGraphId, const WRenderGraphInspectionInfo& info);
  void SetRequest(const WRenderGraphObserverRequest& request);
  void Clear();

Q_SIGNALS:
  void AccessSelected(WUInt16 uiPassIndex, WUInt16 uiAccessIndex);
  void AccessDeselected();

protected:
  void paintEvent(QPaintEvent*) override;
  void mousePressEvent(QMouseEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;

private:
  /// Maps one displayed texture column to the texture resource it represents.
  struct TextureColumnResource
  {
    WUInt16 m_uiResolvedIndex = 0xFFFF;
    WUInt16 m_uiTextureIndex = 0xFFFF;
  };

  /// Maps one displayed buffer column to the buffer resource it represents.
  struct BufferColumnResource
  {
    WUInt16 m_uiResolvedIndex = 0xFFFF;
    WUInt16 m_uiBufferIndex = 0xFFFF;
  };

  /// Describes the render graph overview element currently under the mouse cursor.
  struct HoverInfo
  {
    enum class Area
    {
      None,
      PassHeader,
      TextureHeader,
      BufferHeader,
      ResourceCell,
    };

    Area m_Area = Area::None;
    QPoint m_Position;
    WUInt16 m_uiPassIndex = 0xFFFF;
    WUInt16 m_uiResourceColumn = 0xFFFF;
    bool m_bIsTexture = true;
  };



  HoverInfo GetHoverInfo(const QPoint& pos) const;
  const WRenderGraphInspectionInfo::AccessInfo* FindAccess(const HoverInfo& hover, bool bLastMatch = false) const;
  QString MakeAccessesTooltip(const HoverInfo& hover) const;
  WUInt16 FindTextureLifetimeResource(const HoverInfo& hover) const;
  WUInt16 FindBufferLifetimeResource(const HoverInfo& hover) const;
  QString MakeHoverTooltip(const HoverInfo& hover) const;
  QString MakeTextureColumnTooltip(WUInt32 uiResolvedIndex) const;
  QString MakeBufferColumnTooltip(WUInt32 uiResolvedIndex) const;
  QRect GetCellRect(WUInt32 uiPassIndex, WUInt32 uiResourceColumn) const;
  void DrawResourceNames(QPainter& painter);
  void DrawPassNames(QPainter& painter);
  void DrawGrid(QPainter& painter);
  void DrawResourceLifetimes(QPainter& painter);
  QRect GetLifetimeRect(WUInt32 uiFirstPass, WUInt32 uiLastPass, WUInt32 uiResourceColumn) const;
  void DrawAccesses(QPainter& painter);

  static constexpr WInt32 s_iCellSize = 18;
  static constexpr WInt32 s_iPassLabelWidth = 200;
  static constexpr WInt32 s_iHeaderHeight = 40;
  static constexpr WInt32 s_iContentPadding = 12;
  static constexpr WInt32 s_iLifetimeGap = 2;
  static constexpr WInt32 s_iAccessPadding = 4;

  // Input inspection info
  WUInt64 m_uiSelectedGraphId = 0;
  WRenderGraphInspectionInfo m_Info;
  QSize m_MinContentSize = QSize(600, 300);

  // Caches
  WUInt32 m_uiTextureColumnCount = 0;
  WUInt32 m_uiBufferColumnCount = 0;
  WUInt32 m_uiTotalResourceColumnCount = 0;
  WDynamicArray<TextureColumnResource> m_TextureColumnResources; ///< Find transient textures matching the same m_uiResolvedIndex
  WDynamicArray<BufferColumnResource> m_BufferColumnResources;   ///< Find transient buffers matching the same m_uiResolvedIndex

  // Observed resource
  WString m_sObservedPassName;
  WUInt16 m_uiObservedAccessIndex = 0;
};
