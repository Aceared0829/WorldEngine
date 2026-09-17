#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>

/// Qt graphics item for procedural generation nodes.
///
/// Visual representation of procedural generation nodes, such as noise generators, modifiers, or output nodes.
class WQtProcGenNode : public WQtVisualGraphNode
{
public:
  WQtProcGenNode();

  virtual void InitNode(const WVisualGraphObjectManager* pManager, const WDocumentObject* pObject) override;

  virtual void UpdateState() override;

private:
  void ResolvePlaceholder(WStringView sPlaceholder, const WVariant& index, bool bOptional, const TitleFormat& format, WStringBuilder& ref_sOutput);
};

/// Qt graphics item for procedural generation pins.
///
/// Extends the base pin with debugging support. Pins can be marked for debug visualization,
/// allowing users to inspect intermediate results in the procedural generation pipeline.
class WQtProcGenPin : public WQtVisualGraphPin
{
public:
  WQtProcGenPin();
  ~WQtProcGenPin();

  virtual void ExtendContextMenu(QMenu& ref_menu) override;

  virtual void keyPressEvent(QKeyEvent* pEvent) override;
  virtual void paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;
  virtual QRectF boundingRect() const override;

  void SetDebug(bool bDebug);

private:
  bool m_bDebug = false;
};

/// Qt scene for procedural generation graphs.
///
/// Manages the visual scene for procedural generation graph editing, including debug pin tracking
/// for visualizing intermediate generation results.
class WQtProcGenScene : public WQtVisualGraphScene
{
public:
  WQtProcGenScene(QObject* pParent = nullptr);
  ~WQtProcGenScene();

  void SetDebugPin(WQtProcGenPin* pDebugPin);

private:
  virtual WStatus RemoveNode(WQtVisualGraphNode* pNode) override;

  bool m_bUpdatingDebugPin = false;
  WQtProcGenPin* m_pDebugPin = nullptr;
};
