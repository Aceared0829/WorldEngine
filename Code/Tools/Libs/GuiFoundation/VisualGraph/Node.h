#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <QGraphicsWidget>

// Avoid conflicts with windows.
#ifdef GetObject
#  undef GetObject
#endif

class WQtVisualGraphPin;
class WVisualGraphObjectManager;
class QLabel;
class WDocumentObject;
class QGraphicsTextItem;
class QGraphicsPixmapItem;
class QGraphicsDropShadowEffect;
class WAbstractProperty;

struct WQtVisualGraphNodeFlags
{
  using StorageType = WUInt8;

  enum Enum
  {
    None = 0,
    Moved = W_BIT(0),
    UpdateTitle = W_BIT(1),
    Default = None
  };

  struct Bits
  {
    StorageType Moved : 1;
    StorageType UpdateTitle : 1;
  };
};

/// Qt graphics item representing a single node in a visual graph.
///
/// Displays the node's title, optional subtitle, icon, and manages its pins.
/// Handles rendering, selection, and user interaction for the node.
/// Derive from this class to customize node appearance for specific graph types.
class W_GUIFOUNDATION_DLL WQtVisualGraphNode : public QGraphicsPathItem
{
public:
  WQtVisualGraphNode();
  ~WQtVisualGraphNode();
  virtual int type() const override { return WQtVisualGraphScene::Node; }

  const WDocumentObject* GetObject() const { return m_pObject; }
  virtual void InitNode(const WVisualGraphObjectManager* pManager, const WDocumentObject* pObject);

  virtual void UpdateGeometry();

  void CreatePins();

  WQtVisualGraphPin* GetInputPin(const WVisualGraphPin& pin);
  WQtVisualGraphPin* GetOutputPin(const WVisualGraphPin& pin);

  WBitflags<WQtVisualGraphNodeFlags> GetFlags() const;
  void ResetFlags();

  void EnableDropShadow(bool bEnable);
  virtual void UpdateState();

  const WHybridArray<WQtVisualGraphPin*, 6>& GetInputPins() const { return m_Inputs; }
  const WHybridArray<WQtVisualGraphPin*, 6>& GetOutputPins() const { return m_Outputs; }

  void SetActive(bool bActive);

  virtual void ExtendContextMenu(QMenu& ref_menu) {}

protected:
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  /// Controls how UpdateState() turns values into title text.
  struct TitleFormat
  {
    WUInt32 m_uiMaxStringLength = 23;       ///< Strings longer than this are truncated. 0 disables truncation.
    WUInt32 m_uiMaxTitleLength = 0;         ///< The fully rendered title is truncated to this length. 0 disables truncation.
    bool m_bQuoteStrings = true;             ///< Whether string values are enclosed in quotes.
    bool m_bSplitAtDoubleColon = true;       ///< Whether the part of the title before a '::' becomes the subtitle.
    bool m_bBoolsAsTicks = false;            ///< If set, bools turn into [x]/[ ] instead of 'true'/'false'
    bool m_bIgnoreInvalidProperties = false; ///< If set, any invalid property value is ignored, i.e. resolves to "".
  };

  bool TryGetTitleTemplateFromProperty(WStringView sPropertyName, WStringBuilder& out_sTemplate);
  bool TryGetTitleTemplateFromAttribute(WStringBuilder& out_sTemplate);
  void GetDefaultTitleTemplate(WStringBuilder& out_sTemplate);
  void ResolvePropertyPlaceholder(WStringView sPlaceholder, const WVariant& index, bool bOptional, const TitleFormat& format, WStringBuilder& ref_sOutput);
  void AppendPropertyValue(const WAbstractProperty* pProp, const WVariant& index, bool bOptional, const TitleFormat& format, WStringBuilder& ref_sOutput);
  void SetTitleAndSubtitle(WStringView sTitle, const TitleFormat& format);

  QColor m_HeaderColor;
  QRectF m_HeaderRect;
  QGraphicsTextItem* m_pTitleLabel = nullptr;
  QGraphicsTextItem* m_pSubtitleLabel = nullptr;
  QGraphicsPixmapItem* m_pIcon = nullptr;

private:
  const WVisualGraphObjectManager* m_pManager = nullptr;
  const WDocumentObject* m_pObject = nullptr;
  WBitflags<WQtVisualGraphNodeFlags> m_DirtyFlags;

  bool m_bIsActive = true;

  QGraphicsDropShadowEffect* m_pShadow = nullptr;

  // Pins
  WHybridArray<WQtVisualGraphPin*, 6> m_Inputs;
  WHybridArray<WQtVisualGraphPin*, 6> m_Outputs;
};
