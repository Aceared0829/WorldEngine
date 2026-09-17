#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraphQt.moc.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginVisualScript, Factories)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WVisualScriptNodeRegistry);
    const WRTTI* pBaseType = WVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType();

    WQtVisualGraphScene::GetPinFactory().RegisterCreator(WGetStaticRTTI<WVisualScriptPin>(), [](const WRTTI* pRtti)->WQtVisualGraphPin* { return new WQtVisualScriptPin(); });
    /*WQtVisualGraphScene::GetConnectionFactory().RegisterCreator(WGetStaticRTTI<WVisualScriptConnection>(), [](const WRTTI* pRtti)->WQtVisualGraphConnection* { return new WQtVisualScriptConnection(); });    */
    WQtVisualGraphScene::GetNodeFactory().RegisterCreator(pBaseType, [](const WRTTI* pRtti)->WQtVisualGraphNode* { return new WQtVisualScriptNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const WRTTI* pBaseType = WVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType();

    WQtVisualGraphScene::GetPinFactory().UnregisterCreator(WGetStaticRTTI<WVisualScriptPin>());
    //WQtVisualGraphScene::GetConnectionFactory().UnregisterCreator(WGetStaticRTTI<WVisualScriptConnection>());
    WQtVisualGraphScene::GetNodeFactory().UnregisterCreator(pBaseType);

    WVisualScriptNodeRegistry* pDummy = WVisualScriptNodeRegistry::GetSingleton();
    W_DEFAULT_DELETE(pDummy);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

WQtVisualScriptPin::WQtVisualScriptPin() = default;

void WQtVisualScriptPin::SetPin(const WVisualGraphPin& pin)
{
  m_bTranslatePinName = false;

  WQtVisualGraphPin::SetPin(pin);

  UpdateTooltip();
}

bool WQtVisualScriptPin::UpdatePinColors(const WColorGammaUB* pOverwriteColor)
{
  WColorGammaUB overwriteColor;
  const WVisualScriptPin& vsPin = WStaticCast<const WVisualScriptPin&>(*GetPin());

  WVisualScriptDataType::Enum type = vsPin.GetResolvedScriptDataType();
  if (vsPin.NeedsTypeDeduction())
  {
    overwriteColor = WVisualScriptNodeRegistry::PinDesc::GetColorForScriptDataType(type);
    pOverwriteColor = &overwriteColor;
  }

  bool res = WQtVisualGraphPin::UpdatePinColors(pOverwriteColor);

  if (vsPin.IsRequired() && type != WVisualScriptDataType::GameObject && HasAnyConnections() == false)
  {
    QColor requiredColor = WToQtColor(WColorScheme::LightUI(WColorScheme::Red));

    QPen p = pen();
    p.setColor(requiredColor);
    setPen(p);

    m_pLabel->setDefaultTextColor(requiredColor);

    return true;
  }

  UpdateTooltip();

  return res;
}

void WQtVisualScriptPin::UpdateTooltip()
{
  const WVisualScriptPin& vsPin = WStaticCast<const WVisualScriptPin&>(*GetPin());

  WStringBuilder sTooltip;
  sTooltip = vsPin.GetName();

  if (vsPin.IsDataPin())
  {
    sTooltip.Append(": ", vsPin.GetDataTypeName());

    if (vsPin.IsRequired())
    {
      sTooltip.Append(" (Required)");
    }
  }

  setToolTip(sTooltip.GetData());
}

//////////////////////////////////////////////////////////////////////////

WQtVisualScriptConnection::WQtVisualScriptConnection() = default;

//////////////////////////////////////////////////////////////////////////

WQtVisualScriptNode::WQtVisualScriptNode() = default;

void WQtVisualScriptNode::UpdateState()
{
  const TitleFormat format;

  WStringBuilder sTemplate;
  if (!TryGetTitleTemplateFromAttribute(sTemplate))
  {
    sTemplate = WVisualScriptNodeManager::GetNiceTypeName(GetObject());
  }

  WStringBuilder sTitle;
  WTokenParseUtils::RenderTemplate(sTemplate, [&](WStringView sPlaceholder, WVariant index, bool bOptional, WStringBuilder& ref_sOutput)
    { ResolvePlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput); }, sTitle);

  SetTitleAndSubtitle(sTitle, format);

  auto pManager = static_cast<const WVisualScriptNodeManager*>(GetObject()->GetDocumentObjectManager());

  if (m_pSubtitleLabel->toPlainText().isEmpty())
  {
    auto pNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(GetObject()->GetType());
    if (pNodeDesc != nullptr && pNodeDesc->NeedsTypeDeduction())
    {
      WVisualScriptDataType::Enum deductedType = pManager->GetDeductedType(GetObject());
      m_pSubtitleLabel->setPlainText(deductedType != WVisualScriptDataType::Invalid ? WVisualScriptDataType::GetName(deductedType) : "Unknown");
    }
  }

  auto pScene = static_cast<WQtVisualScriptNodeScene*>(scene());

  if (pManager->IsCoroutine(GetObject()))
  {
    m_pIcon->setPixmap(pScene->GetCoroutineIcon());
    m_pIcon->setScale(0.5);
  }
  else if (pManager->IsLoop(GetObject()))
  {
    m_pIcon->setPixmap(pScene->GetLoopIcon());
    m_pIcon->setScale(0.5);
  }
  else
  {
    m_pIcon->setPixmap(QPixmap());
  }
}

void WQtVisualScriptNode::ResolvePlaceholder(WStringView sPlaceholder, const WVariant& index, bool bOptional, const TitleFormat& format, WStringBuilder& ref_sOutput)
{
  for (const auto& pin : GetInputPins())
  {
    if (pin->GetPin()->GetName() != sPlaceholder || !pin->HasAnyConnections())
      continue;

    // the value of a connected string pin is unknown here, so nothing is shown for it
    if (!bOptional && static_cast<const WVisualScriptPin*>(pin->GetPin())->GetScriptDataType() != WVisualScriptDataType::String)
    {
      ref_sOutput.Append(sPlaceholder);
    }

    return;
  }

  ResolvePropertyPlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput);
}

//////////////////////////////////////////////////////////////////////////

WQtVisualScriptNodeScene::WQtVisualScriptNodeScene(QObject* pParent /*= nullptr*/)
  : WQtVisualGraphScene(pParent)
{
  constexpr int iconSize = 32;
  m_CoroutineIcon = QIcon(":/EditorPluginVisualScript/Icons/Coroutine.svg").pixmap(QSize(iconSize, iconSize));
  m_LoopIcon = QIcon(":/EditorPluginVisualScript/Icons/Loop.svg").pixmap(QSize(iconSize, iconSize));
}

WQtVisualScriptNodeScene::~WQtVisualScriptNodeScene()
{
  if (m_pManager != nullptr)
  {
    static_cast<const WVisualScriptNodeManager*>(m_pManager)->m_NodeChangedEvent.RemoveEventHandler(WMakeDelegate(&WQtVisualScriptNodeScene::NodeChangedHandler, this));
  }
}

void WQtVisualScriptNodeScene::InitScene(const WVisualGraphObjectManager* pManager)
{
  WQtVisualGraphScene::InitScene(pManager);

  static_cast<const WVisualScriptNodeManager*>(pManager)->m_NodeChangedEvent.AddEventHandler(WMakeDelegate(&WQtVisualScriptNodeScene::NodeChangedHandler, this));
}

void WQtVisualScriptNodeScene::NodeChangedHandler(const WDocumentObject* pObject)
{
  auto it = m_Nodes.Find(pObject);
  if (it.IsValid() == false)
    return;

  WQtVisualGraphNode* pNode = it.Value();

  pNode->ResetFlags();
  pNode->update();

  auto& inputPins = pNode->GetInputPins();
  for (WQtVisualGraphPin* pPin : inputPins)
  {
    if (static_cast<WQtVisualScriptPin*>(pPin)->UpdatePinColors())
    {
      pPin->update();
    }
  }

  auto& outputPins = pNode->GetOutputPins();
  for (WQtVisualGraphPin* pPin : outputPins)
  {
    if (static_cast<WQtVisualScriptPin*>(pPin)->UpdatePinColors())
    {
      pPin->update();
    }
  }
}
