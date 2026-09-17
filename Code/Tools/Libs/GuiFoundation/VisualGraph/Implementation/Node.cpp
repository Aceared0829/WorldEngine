#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsPixmapItem>
#include <QPainter>

WQtVisualGraphNode::WQtVisualGraphNode()
{
  auto palette = QApplication::palette();

  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges);

  setBrush(palette.window());
  QPen pen(palette.mid().color(), 3, Qt::SolidLine);
  setPen(pen);

  {
    QFont font = QApplication::font();
    font.setBold(true);

    m_pTitleLabel = new QGraphicsTextItem(this);
    m_pTitleLabel->setFont(font);
  }

  {
    QFont font = QApplication::font();
    font.setPointSizeF(font.pointSizeF() * 0.9f);

    m_pSubtitleLabel = new QGraphicsTextItem(this);
    m_pSubtitleLabel->setFont(font);
    m_pSubtitleLabel->setPos(0, m_pTitleLabel->boundingRect().bottom() - 5);
  }

  {
    m_pIcon = new QGraphicsPixmapItem(this);
  }

  m_HeaderColor = palette.alternateBase().color();
}

WQtVisualGraphNode::~WQtVisualGraphNode()
{
  EnableDropShadow(false);
}

void WQtVisualGraphNode::EnableDropShadow(bool bEnable)
{
  if (bEnable && m_pShadow == nullptr)
  {
    auto palette = QApplication::palette();

    m_pShadow = new QGraphicsDropShadowEffect();
    m_pShadow->setOffset(3, 3);
    m_pShadow->setColor(palette.color(QPalette::Shadow));
    m_pShadow->setBlurRadius(10);
    setGraphicsEffect(m_pShadow);
  }

  if (!bEnable && m_pShadow != nullptr)
  {
    delete m_pShadow;
    m_pShadow = nullptr;
  }
}

void WQtVisualGraphNode::InitNode(const WVisualGraphObjectManager* pManager, const WDocumentObject* pObject)
{
  m_pManager = pManager;
  m_pObject = pObject;
  CreatePins();
  UpdateState();

  UpdateGeometry();

  if (const WColorAttribute* pColorAttr = pObject->GetType()->GetAttributeByType<WColorAttribute>())
  {
    m_HeaderColor = WToQtColor(pColorAttr->GetColor());
  }

  m_DirtyFlags.Add(WQtVisualGraphNodeFlags::UpdateTitle);
}

void WQtVisualGraphNode::UpdateGeometry()
{
  prepareGeometryChange();

  QRectF iconRect = m_pIcon->boundingRect();
  iconRect.moveTo(m_pIcon->pos());
  iconRect.setSize(iconRect.size() * m_pIcon->scale());

  QRectF titleRect;
  {
    QPointF titlePos = m_pTitleLabel->pos();
    titlePos.setX(iconRect.right());
    m_pTitleLabel->setPos(titlePos);

    titleRect = m_pTitleLabel->boundingRect();
    titleRect.moveTo(titlePos);
  }

  m_pIcon->setPos(0, (titleRect.bottom() - iconRect.height()) / 2);

  QRectF subtitleRect;
  if (m_pSubtitleLabel->toPlainText().isEmpty() == false)
  {
    QPointF subtitlePos = m_pSubtitleLabel->pos();
    subtitlePos.setX(iconRect.right());
    m_pSubtitleLabel->setPos(subtitlePos);

    subtitleRect = m_pSubtitleLabel->boundingRect();
    subtitleRect.moveTo(m_pSubtitleLabel->pos());
  }

  int h = WMath::Max(titleRect.bottom(), subtitleRect.bottom()) + 5;

  int y = h;

  // Align inputs
  int maxInputWidth = 10;
  for (WQtVisualGraphPin* pQtPin : m_Inputs)
  {
    auto rectPin = pQtPin->GetPinRect();
    pQtPin->setPos(QPointF(-rectPin.x(), y - rectPin.y()));

    maxInputWidth = WMath::Max(maxInputWidth, (int)rectPin.width());
    y += rectPin.height();
  }

  int maxheight = y;
  y = h;

  // Align outputs
  int maxOutputWidth = 10;
  for (WQtVisualGraphPin* pQtPin : m_Outputs)
  {
    auto rectPin = pQtPin->GetPinRect();
    pQtPin->setPos(QPointF(-rectPin.x(), y - rectPin.y()));

    maxOutputWidth = WMath::Max(maxOutputWidth, (int)rectPin.width());
    y += rectPin.height();
  }

  int w = maxInputWidth + maxOutputWidth + 20;

  const int headerWidth = WMath::Max(titleRect.width(), subtitleRect.width()) + iconRect.width();
  w = WMath::Max(w, headerWidth);

  maxheight = WMath::Max(maxheight, y);

  // Align outputs to the right
  for (WUInt32 i = 0; i < m_Outputs.GetCount(); ++i)
  {
    auto rectPin = m_Outputs[i]->GetPinRect();
    m_Outputs[i]->setX(w - rectPin.width());
  }

  m_HeaderRect = QRectF(-5, -3, w + 10, WMath::Max(titleRect.bottom(), subtitleRect.bottom()) + 5);

  {
    QPainterPath p;
    p.addRoundedRect(-5, -3, w + 10, maxheight + 10, 5, 5);
    setPath(p);
  }
}

void WQtVisualGraphNode::UpdateState()
{
  const TitleFormat format;

  WStringBuilder sTemplate;
  if (!TryGetTitleTemplateFromProperty("CustomTitle", sTemplate) && !TryGetTitleTemplateFromAttribute(sTemplate))
  {
    GetDefaultTitleTemplate(sTemplate);
  }

  WStringBuilder sTitle;
  WTokenParseUtils::RenderTemplate(sTemplate, [&](WStringView sPlaceholder, WVariant index, bool bOptional, WStringBuilder& ref_sOutput)
    { ResolvePropertyPlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput); }, sTitle);

  SetTitleAndSubtitle(sTitle, format);
}

bool WQtVisualGraphNode::TryGetTitleTemplateFromProperty(WStringView sPropertyName, WStringBuilder& out_sTemplate)
{
  const WVariant value = GetObject()->GetTypeAccessor().GetValue(sPropertyName);
  if (!value.IsValid() || !value.CanConvertTo<WString>())
    return false;

  out_sTemplate = value.ConvertTo<WString>();
  return !out_sTemplate.IsEmpty();
}

bool WQtVisualGraphNode::TryGetTitleTemplateFromAttribute(WStringBuilder& out_sTemplate)
{
  auto pTitleAttribute = GetObject()->GetType()->GetAttributeByType<WTitleAttribute>();
  if (pTitleAttribute == nullptr)
    return false;

  out_sTemplate = pTitleAttribute->GetTitle();
  return true;
}

void WQtVisualGraphNode::GetDefaultTitleTemplate(WStringBuilder& out_sTemplate)
{
  auto& typeAccessor = GetObject()->GetTypeAccessor();

  WVariant name = typeAccessor.GetValue("Name");
  if (name.IsA<WString>() && !name.Get<WString>().IsEmpty())
  {
    out_sTemplate = name.Get<WString>();
  }
  else
  {
    out_sTemplate = WTranslate(typeAccessor.GetType()->GetTypeName());
  }
}

void WQtVisualGraphNode::ResolvePropertyPlaceholder(WStringView sPlaceholder, const WVariant& index, bool bOptional, const TitleFormat& format, WStringBuilder& ref_sOutput)
{
  const WAbstractProperty* pProp = GetObject()->GetType()->FindPropertyByName(sPlaceholder);
  if (pProp == nullptr)
    return;

  AppendPropertyValue(pProp, index, bOptional, format, ref_sOutput);
}

void WQtVisualGraphNode::AppendPropertyValue(const WAbstractProperty* pProp, const WVariant& index, bool bOptional, const TitleFormat& format, WStringBuilder& ref_sOutput)
{
  if ((pProp->GetCategory() == WPropertyCategory::Set || pProp->GetCategory() == WPropertyCategory::Array) && !index.IsValid())
  {
    WTempHybridArray<WVariant, 16> values;
    GetObject()->GetTypeAccessor().GetValues(pProp->GetPropertyName(), values);

    if (bOptional && values.IsEmpty())
      return;

    WStringBuilder sSet("{");
    for (const auto& setValue : values)
    {
      if (sSet.GetElementCount() > 1)
      {
        sSet.Append(", ");
      }
      sSet.Append(setValue.ConvertTo<WString>().GetView());
    }
    sSet.Append("}");

    ref_sOutput.Append(sSet.GetView());
    return;
  }

  const WVariant value = GetObject()->GetTypeAccessor().GetValue(pProp->GetPropertyName(), index);

  if (!value.IsValid())
  {
    if (format.m_bIgnoreInvalidProperties || bOptional)
      return;

    ref_sOutput.Append("<Invalid>");
    return;
  }

  if (bOptional)
  {
    if (value == WVariant(0))
      return;

    if ((value.IsA<WString>() || value.IsA<WHashedString>()) && value.ConvertTo<WString>().IsEmpty())
      return;
  }


  WStringBuilder sValue;
  if (pProp->GetSpecificType()->IsDerivedFrom<WEnumBase>() || pProp->GetSpecificType()->IsDerivedFrom<WBitflagsBase>())
  {
    WReflectionUtils::EnumerationToString(pProp->GetSpecificType(), value.ConvertTo<WInt64>(), sValue);
    sValue = WTranslate(sValue);
  }
  else if (value.IsA<bool>())
  {
    if (format.m_bBoolsAsTicks)
      sValue.Set(value.Get<bool>() ? "[x]" : "[ ]");
    else
      sValue.Set(value.Get<bool>() ? "true" : "false");
  }
  else if (value.IsA<WColor>())
  {
    sValue = WConversionUtils::GetColorName(value.Get<WColor>());
  }
  else if (value.IsA<WColorGammaUB>())
  {
    sValue = WConversionUtils::GetColorName(WColor(value.Get<WColorGammaUB>()));
  }
  else if (value.IsA<WVec2>())
  {
    const WVec2 v = value.Get<WVec2>();
    sValue.SetFormat("({}, {})", WArgF(v.x, 2), WArgF(v.y, 2));
  }
  else if (value.IsA<WVec3>())
  {
    const WVec3 v = value.Get<WVec3>();
    sValue.SetFormat("({}, {}, {})", WArgF(v.x, 2), WArgF(v.y, 2), WArgF(v.z, 2));
  }
  else if (value.IsA<WVec4>())
  {
    const WVec4 v = value.Get<WVec4>();
    sValue.SetFormat("({}, {}, {}, {})", WArgF(v.x, 2), WArgF(v.y, 2), WArgF(v.z, 2), WArgF(v.w, 2));
  }
  else if (value.IsA<WString>() || value.IsA<WHashedString>())
  {
    sValue = value.ConvertTo<WString>();

    // asset references are stored as document GUIDs, which are meaningless to the user
    if (WConversionUtils::IsStringUuid(sValue) && WToolsProject::GetSingleton() != nullptr)
    {
      const WStringBuilder sPath = WToolsProject::GetSingleton()->GetPathForDocumentGuid(WConversionUtils::ConvertStringToUuid(sValue));

      if (!sPath.IsEmpty())
      {
        sValue = WPathUtils::GetFileName(sPath);
      }
    }

    sValue.ReplaceAll("\n", " ");
    sValue.ReplaceAll("\t", " ");

    if (format.m_uiMaxStringLength > 0 && sValue.GetCharacterCount() > format.m_uiMaxStringLength)
    {
      sValue.Shrink(0, sValue.GetCharacterCount() - (format.m_uiMaxStringLength - 2));
      sValue.Append("...");
    }

    if (format.m_bQuoteStrings && !sValue.IsEmpty())
    {
      sValue.Prepend("\"");
      sValue.Append("\"");
    }
  }
  else if (value.CanConvertTo<WString>())
  {
    sValue = value.ConvertTo<WString>();
  }
  else
  {
    sValue = "<not-implemented>";
  }

  ref_sOutput.Append(sValue.GetView());
}

void WQtVisualGraphNode::SetTitleAndSubtitle(WStringView sTitle, const TitleFormat& format)
{
  WStringBuilder sCleaned = sTitle;

  if (format.m_uiMaxTitleLength > 0 && sCleaned.GetCharacterCount() > format.m_uiMaxTitleLength)
  {
    sCleaned.Shrink(0, sCleaned.GetCharacterCount() - (format.m_uiMaxTitleLength + 1));
    sCleaned.Append("...");
  }

  if (!format.m_bSplitAtDoubleColon)
  {
    m_pTitleLabel->setPlainText(WMakeQString(sCleaned));
    return;
  }

  if (const char* szSeparator = sCleaned.FindSubString("::"))
  {
    m_pTitleLabel->setPlainText(szSeparator + 2);

    WStringBuilder sSubTitle = WStringView(sCleaned.GetData(), szSeparator);
    sSubTitle.Trim("\"");
    m_pSubtitleLabel->setPlainText(WMakeQString(sSubTitle));
  }
  else
  {
    m_pTitleLabel->setPlainText(WMakeQString(sCleaned));
    m_pSubtitleLabel->setPlainText(QString());
  }
}

void WQtVisualGraphNode::SetActive(bool bActive)
{
  if (m_bIsActive != bActive)
  {
    m_bIsActive = bActive;

    for (auto pInputPin : m_Inputs)
    {
      pInputPin->SetActive(bActive);
    }

    for (auto pOutputPin : m_Outputs)
    {
      pOutputPin->SetActive(bActive);
    }
  }

  update();
}

void WQtVisualGraphNode::CreatePins()
{
  for (auto pQtPin : m_Inputs)
  {
    delete pQtPin;
  }
  m_Inputs.Clear();

  for (auto pQtPin : m_Outputs)
  {
    delete pQtPin;
  }
  m_Outputs.Clear();

  auto inputs = m_pManager->GetInputPins(m_pObject);
  for (auto& pPinTarget : inputs)
  {
    WQtVisualGraphPin* pQtPin = WQtVisualGraphScene::GetPinFactory().CreateObject(pPinTarget->GetDynamicRTTI());
    if (pQtPin == nullptr)
    {
      pQtPin = new WQtVisualGraphPin();
    }
    pQtPin->setParentItem(this);
    m_Inputs.PushBack(pQtPin);

    pQtPin->SetPin(*pPinTarget);
  }

  auto outputs = m_pManager->GetOutputPins(m_pObject);
  for (auto& pPinSource : outputs)
  {
    WQtVisualGraphPin* pQtPin = WQtVisualGraphScene::GetPinFactory().CreateObject(pPinSource->GetDynamicRTTI());
    if (pQtPin == nullptr)
    {
      pQtPin = new WQtVisualGraphPin();
    }

    pQtPin->setParentItem(this);
    m_Outputs.PushBack(pQtPin);

    pQtPin->SetPin(*pPinSource);
  }
}

WQtVisualGraphPin* WQtVisualGraphNode::GetInputPin(const WVisualGraphPin& pin)
{
  for (WQtVisualGraphPin* pQtPin : m_Inputs)
  {
    if (pQtPin->GetPin() == &pin)
      return pQtPin;
  }
  return nullptr;
}

WQtVisualGraphPin* WQtVisualGraphNode::GetOutputPin(const WVisualGraphPin& pin)
{
  for (WQtVisualGraphPin* pQtPin : m_Outputs)
  {
    if (pQtPin->GetPin() == &pin)
      return pQtPin;
  }
  return nullptr;
}

WBitflags<WQtVisualGraphNodeFlags> WQtVisualGraphNode::GetFlags() const
{
  return m_DirtyFlags;
}

void WQtVisualGraphNode::ResetFlags()
{
  m_DirtyFlags = WQtVisualGraphNodeFlags::UpdateTitle;
}

void WQtVisualGraphNode::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  if (m_DirtyFlags.IsSet(WQtVisualGraphNodeFlags::UpdateTitle))
  {
    UpdateState();
    UpdateGeometry();
    m_DirtyFlags.Remove(WQtVisualGraphNodeFlags::UpdateTitle);
  }

  auto palette = QApplication::palette();

  // Draw background
  painter->setPen(QPen(Qt::NoPen));
  painter->setBrush(brush());
  painter->drawPath(path());

  QColor headerColor = m_HeaderColor;
  if (!m_bIsActive)
    headerColor.setAlpha(50);

  // Draw separator
  {
    QColor separatorColor = pen().color();
    separatorColor.setAlphaF(headerColor.alphaF() * 0.5f);
    QPen p = pen();
    p.setColor(separatorColor);
    painter->setPen(p);
    painter->drawLine(m_HeaderRect.bottomLeft() + QPointF(2, 0), m_HeaderRect.bottomRight() - QPointF(2, 0));
  }

  // Draw header
  QLinearGradient headerGradient(m_HeaderRect.topLeft(), m_HeaderRect.bottomLeft());
  headerGradient.setColorAt(0.0f, headerColor);
  headerGradient.setColorAt(1.0f, headerColor.darker(120));

  painter->setClipPath(path());
  painter->setPen(QPen(Qt::NoPen));
  painter->setBrush(headerGradient);
  painter->drawRect(m_HeaderRect);
  painter->setClipping(false);

  QColor labelColor;

  // Draw outline
  if (isSelected())
  {
    QPen p = pen();
    p.setColor(palette.highlight().color());
    painter->setPen(p);

    labelColor = WToQtColor(WColor::White);
  }
  else
  {
    painter->setPen(pen());

    labelColor = palette.buttonText().color();
  }

  // Label
  if (!m_bIsActive)
    labelColor = labelColor.darker(150);

  const bool bBackgroundIsLight = m_HeaderColor.lightnessF() > 0.6f;
  if (bBackgroundIsLight)
  {
    labelColor.setRed(255 - labelColor.red());
    labelColor.setGreen(255 - labelColor.green());
    labelColor.setBlue(255 - labelColor.blue());
  }

  m_pTitleLabel->setDefaultTextColor(labelColor);
  m_pSubtitleLabel->setDefaultTextColor(labelColor.darker(110));

  painter->setBrush(QBrush(Qt::NoBrush));
  painter->drawPath(path());
}

QVariant WQtVisualGraphNode::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if (!m_pObject)
    return QGraphicsPathItem::itemChange(change, value);

  WCommandHistory* pHistory = m_pManager->GetDocument()->GetCommandHistory();
  switch (change)
  {
    case QGraphicsItem::ItemPositionHasChanged:
    {
      if (!pHistory->IsInUndoRedo() && !pHistory->IsInTransaction())
        m_DirtyFlags.Add(WQtVisualGraphNodeFlags::Moved);
    }
    break;

    default:
      break;
  }
  return QGraphicsPathItem::itemChange(change, value);
}
