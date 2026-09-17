#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/VisualGraph/Connection.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>

class WQtVisualGraphView;
struct WVisualShaderPinDescriptor;

/// Qt scene for visual shader asset editing.
///
/// Manages the visual scene for editing visual shader assets in the editor.
class WQtVisualShaderScene : public WQtVisualGraphScene
{
  Q_OBJECT

public:
  WQtVisualShaderScene(QObject* pParent = nullptr);
  ~WQtVisualShaderScene();
};

/// Qt graphics item for visual shader pins.
///
/// Displays shader node pins with custom rendering for shader-specific visual feedback.
class WQtVisualShaderPin : public WQtVisualGraphPin
{
public:
  WQtVisualShaderPin();

  virtual void SetPin(const WVisualGraphPin& pin) override;
  virtual void paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;
};

/// Qt graphics item for visual shader nodes.
///
/// Visual representation of shader nodes such as texture samplers, math operations, or output nodes.
class WQtVisualShaderNode : public WQtVisualGraphNode
{
public:
  WQtVisualShaderNode();

  virtual void InitNode(const WVisualGraphObjectManager* pManager, const WDocumentObject* pObject) override;

  virtual void UpdateState() override;

private:
  static bool TryParseSlotPlaceholder(WStringView sPlaceholder, WStringView sPrefix, WUInt32 uiSlotCount, WUInt32& out_uiSlot);
  void ResolvePlaceholder(WStringView sPlaceholder, const WVariant& index, bool bOptional, const TitleFormat& format, WStringBuilder& ref_sOutput);
  void AppendInputPinValue(const WVisualShaderPinDescriptor& pinDesc, WUInt32 uiPin, const TitleFormat& format, WStringBuilder& ref_sOutput);
};
