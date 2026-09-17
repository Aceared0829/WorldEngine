#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>


WQtVisualShaderScene::WQtVisualShaderScene(QObject* pParent)
  : WQtVisualGraphScene(pParent)
{
}

WQtVisualShaderScene::~WQtVisualShaderScene() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WQtVisualShaderPin::WQtVisualShaderPin() = default;

void WQtVisualShaderPin::SetPin(const WVisualGraphPin& pin)
{
  WQtVisualGraphPin::SetPin(pin);

  const WVisualShaderPin& shaderPin = WStaticCast<const WVisualShaderPin&>(pin);

  WStringBuilder sTooltip;
  if (!shaderPin.GetTooltip().IsEmpty())
  {
    sTooltip = shaderPin.GetTooltip();
  }
  else
  {
    sTooltip = shaderPin.GetName();
  }

  if (!shaderPin.GetDescriptor()->m_sDefaultValue.IsEmpty())
  {
    if (!sTooltip.IsEmpty())
      sTooltip.Append("\n");

    sTooltip.Append("Default is ", shaderPin.GetDescriptor()->m_sDefaultValue);
  }

  setToolTip(sTooltip.GetData());
}

void WQtVisualShaderPin::paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget)
{
  QPainterPath p = path();

  const WVisualShaderPin* pVsPin = static_cast<const WVisualShaderPin*>(GetPin());

  pPainter->save();
  pPainter->setBrush(brush());
  pPainter->setPen(pen());

  if (pVsPin->GetType() == WVisualGraphPin::Type::Input && GetConnections().IsEmpty())
  {
    if (pVsPin->GetDescriptor()->m_sDefaultValue.IsEmpty())
    {
      // this pin MUST be connected

      QPen pen;
      pen.setColor(qRgb(255, 0, 0));
      pen.setWidth(3);
      pen.setCosmetic(true);
      pen.setStyle(Qt::PenStyle::SolidLine);
      pen.setCapStyle(Qt::PenCapStyle::SquareCap);

      pPainter->setPen(pen);

      pPainter->drawRect(this->path().boundingRect());
      pPainter->restore();
      return;
    }
  }

  pPainter->drawPath(p);
  pPainter->restore();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WQtVisualShaderNode::WQtVisualShaderNode() = default;

void WQtVisualShaderNode::InitNode(const WVisualGraphObjectManager* pManager, const WDocumentObject* pObject)
{
  WQtVisualGraphNode::InitNode(pManager, pObject);

  if (auto pDesc = WVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType()))
  {
    m_HeaderColor = WToQtColor(pDesc->m_Color);
    m_pTitleLabel->setToolTip(WMakeQString(pDesc->m_sDocs));
  }
  else
  {
    m_HeaderColor = qRgb(255, 0, 0);
    WLog::Error("Could not initialize node type, node descriptor is invalid");
  }
}

void WQtVisualShaderNode::UpdateState()
{
  TitleFormat format;
  format.m_uiMaxStringLength = 0;
  format.m_bQuoteStrings = false;
  format.m_bSplitAtDoubleColon = false;

  auto pDesc = WVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(GetObject()->GetType());

  WStringBuilder sTemplate;
  if (pDesc != nullptr && !pDesc->m_sTitle.IsEmpty())
  {
    sTemplate = pDesc->m_sTitle;
  }
  else
  {
    sTemplate = GetObject()->GetType()->GetTypeName();
    if (sTemplate.StartsWith_NoCase("ShaderNode::"))
    {
      sTemplate.Shrink(12, 0);
    }
  }

  WStringBuilder sTitle;
  WTokenParseUtils::RenderTemplate(sTemplate, [&](WStringView sPlaceholder, WVariant index, bool bOptional, WStringBuilder& ref_sOutput)
    { ResolvePlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput); }, sTitle);

  SetTitleAndSubtitle(sTitle, format);
}

void WQtVisualShaderNode::ResolvePlaceholder(WStringView sPlaceholder, const WVariant& index, bool bOptional, const TitleFormat& format, WStringBuilder& ref_sOutput)
{
  auto pDesc = WVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(GetObject()->GetType());

  if (pDesc != nullptr)
  {
    // the titles refer to pins and properties by position, e.g. {$in0} and {$prop0}
    WUInt32 uiSlot = 0;

    if (TryParseSlotPlaceholder(sPlaceholder, "in"_wsv, pDesc->m_InputPins.GetCount(), uiSlot))
    {
      AppendInputPinValue(pDesc->m_InputPins[uiSlot], uiSlot, format, ref_sOutput);
      return;
    }

    if (TryParseSlotPlaceholder(sPlaceholder, "prop"_wsv, pDesc->m_Properties.GetCount(), uiSlot))
    {
      ResolvePropertyPlaceholder(pDesc->m_Properties[uiSlot].m_sName, index, bOptional, format, ref_sOutput);
      return;
    }
  }

  ResolvePropertyPlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput);
}

bool WQtVisualShaderNode::TryParseSlotPlaceholder(WStringView sPlaceholder, WStringView sPrefix, WUInt32 uiSlotCount, WUInt32& out_uiSlot)
{
  if (!sPlaceholder.StartsWith(sPrefix))
    return false;

  sPlaceholder.Shrink(sPrefix.GetElementCount(), 0);

  WInt32 iSlot = 0;
  if (WConversionUtils::StringToInt(sPlaceholder, iSlot).Failed() || iSlot < 0 || static_cast<WUInt32>(iSlot) >= uiSlotCount)
    return false;

  out_uiSlot = static_cast<WUInt32>(iSlot);
  return true;
}

void WQtVisualShaderNode::AppendInputPinValue(const WVisualShaderPinDescriptor& pinDesc, WUInt32 uiPin, const TitleFormat& format, WStringBuilder& ref_sOutput)
{
  const auto& inputPins = GetInputPins();

  if (uiPin < inputPins.GetCount() && inputPins[uiPin]->HasAnyConnections())
  {
    ref_sOutput.Append(pinDesc.m_sName.GetView());
    return;
  }

  if (pinDesc.m_bExposeAsProperty)
  {
    const WAbstractProperty* pProp = GetObject()->GetType()->FindPropertyByName(pinDesc.m_PropertyDesc.m_sName);
    const WVariant value = GetObject()->GetTypeAccessor().GetValue(pinDesc.m_PropertyDesc.m_sName);

    if (pProp != nullptr && value.IsValid() && value.CanConvertTo<WString>())
    {
      AppendPropertyValue(pProp, {}, false, format, ref_sOutput);
    }
    else
    {
      ref_sOutput.Append(pinDesc.m_sDefaultValue.GetView());
    }

    return;
  }

  ref_sOutput.Append(pinDesc.m_sDefaultValue.IsEmpty() ? pinDesc.m_sName.GetView() : pinDesc.m_sDefaultValue.GetView());
}
