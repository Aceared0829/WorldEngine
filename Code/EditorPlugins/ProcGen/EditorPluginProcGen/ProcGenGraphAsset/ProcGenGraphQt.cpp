#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>


#include <QMenu>
#include <QPainter>

namespace
{
  static WColorGammaUB CategoryColor(const char* szCategory)
  {
    WColorScheme::Enum color = WColorScheme::Green;
    if (WStringUtils::IsEqual(szCategory, "Input"))
      color = WColorScheme::Lime;
    else if (WStringUtils::IsEqual(szCategory, "Output"))
      color = WColorScheme::Cyan;
    else if (WStringUtils::IsEqual(szCategory, "Math"))
      color = WColorScheme::Blue;

    return WColorScheme::DarkUI(color);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

WQtProcGenNode::WQtProcGenNode() = default;

void WQtProcGenNode::InitNode(const WVisualGraphObjectManager* pManager, const WDocumentObject* pObject)
{
  WQtVisualGraphNode::InitNode(pManager, pObject);

  const WRTTI* pRtti = pObject->GetType();

  if (const WCategoryAttribute* pAttr = pRtti->GetAttributeByType<WCategoryAttribute>())
  {
    m_HeaderColor = WToQtColor(CategoryColor(pAttr->GetCategory()));
  }
}

void WQtProcGenNode::UpdateState()
{
  TitleFormat format;
  format.m_uiMaxStringLength = 0;
  format.m_bQuoteStrings = false;
  format.m_bSplitAtDoubleColon = false;
  format.m_bBoolsAsTicks = true;

  WStringBuilder sTemplate;
  if (!TryGetTitleTemplateFromAttribute(sTemplate))
  {
    sTemplate = GetObject()->GetType()->GetTypeName();
    if (sTemplate.StartsWith_NoCase("WProcGen"))
    {
      sTemplate.Shrink(9, 0);
    }
    sTemplate.TrimLeft("_");
  }

  WStringBuilder sTitle;
  WTokenParseUtils::RenderTemplate(sTemplate, [&](WStringView sPlaceholder, WVariant index, bool bOptional, WStringBuilder& ref_sOutput)
    { ResolvePlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput); }, sTitle);

  SetTitleAndSubtitle(sTitle, format);

  WVariant active = GetObject()->GetTypeAccessor().GetValue("Active");
  if (active.IsA<bool>())
  {
    SetActive(active.Get<bool>());
  }
}

void WQtProcGenNode::ResolvePlaceholder(WStringView sPlaceholder, const WVariant& index, bool bOptional, const TitleFormat& format, WStringBuilder& ref_sOutput)
{
  for (const auto& pin : GetInputPins())
  {
    if (pin->GetPin()->GetName() != sPlaceholder)
      continue;

    if (pin->HasAnyConnections())
    {
      if (!bOptional)
      {
        ref_sOutput.Append(sPlaceholder);
      }
    }
    else
    {
      // an unconnected pin falls back to the constant stored in the matching 'Input<PinName>' property
      WStringBuilder sInputProperty("Input", sPlaceholder);
      ResolvePropertyPlaceholder(sInputProperty, index, bOptional, format, ref_sOutput);
    }

    return;
  }

  ResolvePropertyPlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput);
}

//////////////////////////////////////////////////////////////////////////

WQtProcGenPin::WQtProcGenPin() = default;
WQtProcGenPin::~WQtProcGenPin() = default;

void WQtProcGenPin::ExtendContextMenu(QMenu& ref_menu)
{
  QAction* pAction = new QAction("Debug", &ref_menu);
  pAction->setCheckable(true);
  pAction->setChecked(m_bDebug);
  pAction->connect(pAction, &QAction::triggered, [this](bool bChecked)
    { SetDebug(bChecked); });

  ref_menu.addAction(pAction);
}

void WQtProcGenPin::keyPressEvent(QKeyEvent* pEvent)
{
  if (WQtUtils::IsEquivalentQtKey(pEvent, Qt::Key_D) || pEvent->key() == Qt::Key_F9)
  {
    SetDebug(!m_bDebug);
  }
}

void WQtProcGenPin::paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget)
{
  WQtVisualGraphPin::paint(pPainter, pOption, pWidget);

  pPainter->save();
  pPainter->setPen(QPen(QColor(220, 0, 0), 3.5f, Qt::DotLine));
  pPainter->setBrush(Qt::NoBrush);

  if (m_bDebug)
  {
    float pad = 3.5f;
    QRectF bounds = path().boundingRect().adjusted(-pad, -pad, pad, pad);
    pPainter->drawEllipse(bounds);
  }

  pPainter->restore();
}

QRectF WQtProcGenPin::boundingRect() const
{
  QRectF bounds = WQtVisualGraphPin::boundingRect();
  return bounds.adjusted(-6, -6, 6, 6);
}

void WQtProcGenPin::SetDebug(bool bDebug)
{
  if (m_bDebug != bDebug)
  {
    m_bDebug = bDebug;

    auto pScene = static_cast<WQtProcGenScene*>(scene());
    pScene->SetDebugPin(bDebug ? this : nullptr);

    update();
  }
}

//////////////////////////////////////////////////////////////////////////

WQtProcGenScene::WQtProcGenScene(QObject* pParent /*= nullptr*/)
  : WQtVisualGraphScene(pParent)
{
}

WQtProcGenScene::~WQtProcGenScene() = default;

void WQtProcGenScene::SetDebugPin(WQtProcGenPin* pDebugPin)
{
  if (m_pDebugPin == pDebugPin || m_bUpdatingDebugPin)
    return;

  if (m_pDebugPin != nullptr)
  {
    // don't recursively call this function, otherwise the resource is written twice
    // once with debug disabled, then with it enabled, and because it is so quick after each other
    // the resource manager may ignore the second update, because the first one is still ongoing
    m_bUpdatingDebugPin = true;
    m_pDebugPin->SetDebug(false);
    m_bUpdatingDebugPin = false;
  }

  m_pDebugPin = pDebugPin;

  if (WQtDocumentWindow* window = qobject_cast<WQtDocumentWindow*>(parent()))
  {
    auto document = static_cast<WProcGenGraphAssetDocument*>(window->GetDocument());
    document->SetDebugPin(pDebugPin != nullptr ? pDebugPin->GetPin() : nullptr);
  }
}

WStatus WQtProcGenScene::RemoveNode(WQtVisualGraphNode* pNode)
{
  auto pins = pNode->GetInputPins();
  pins.PushBackRange(pNode->GetOutputPins());

  for (auto pPin : pins)
  {
    if (pPin == m_pDebugPin)
    {
      m_pDebugPin->SetDebug(false);
    }
  }

  return WQtVisualGraphScene::RemoveNode(pNode);
}
