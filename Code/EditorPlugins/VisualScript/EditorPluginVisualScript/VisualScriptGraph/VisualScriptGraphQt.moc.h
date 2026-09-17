#pragma once

#include <GuiFoundation/VisualGraph/Connection.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

/// Qt graphics item for visual script pins.
///
/// Displays pins with color coding based on data type and provides tooltips with detailed type information.
/// Handles both execution pins and data pins with appropriate visual styling.
class WQtVisualScriptPin : public WQtVisualGraphPin
{
public:
  WQtVisualScriptPin();

  virtual void SetPin(const WVisualGraphPin& pin) override;
  virtual bool UpdatePinColors(const WColorGammaUB* pOverwriteColor = nullptr) override;

private:
  void UpdateTooltip();
};

/// Qt graphics item for visual script connections.
///
/// Renders connections between visual script pins, representing both execution flow and data flow.
class WQtVisualScriptConnection : public WQtVisualGraphConnection
{
public:
  WQtVisualScriptConnection();
};

/// Qt graphics item for visual script nodes.
///
/// Visual representation of nodes in a visual script, such as function calls, variables, or control flow nodes.
/// Displays special icons for coroutine and loop nodes.
class WQtVisualScriptNode : public WQtVisualGraphNode
{
public:
  WQtVisualScriptNode();

  virtual void UpdateState() override;

private:
  void ResolvePlaceholder(WStringView sPlaceholder, const WVariant& index, bool bOptional, const TitleFormat& format, WStringBuilder& ref_sOutput);
};

/// Qt scene for visual script graphs.
///
/// Manages the visual scene for visual script editing. Provides icons for coroutine and loop nodes
/// and updates node visuals when their properties change.
class WQtVisualScriptNodeScene : public WQtVisualGraphScene
{
  Q_OBJECT

public:
  WQtVisualScriptNodeScene(QObject* pParent = nullptr);
  ~WQtVisualScriptNodeScene();

  virtual void InitScene(const WVisualGraphObjectManager* pManager);

  const QPixmap& GetCoroutineIcon() const { return m_CoroutineIcon; }
  const QPixmap& GetLoopIcon() const { return m_LoopIcon; }

private:
  void NodeChangedHandler(const WDocumentObject* pObject);

  QPixmap m_CoroutineIcon;
  QPixmap m_LoopIcon;
};
