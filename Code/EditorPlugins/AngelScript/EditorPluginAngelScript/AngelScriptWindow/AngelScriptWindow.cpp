#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAngelScript/AngelScriptAsset/AngelScriptAsset.h>
#include <EditorPluginAngelScript/AngelScriptWindow/AngelScriptWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/VisualGraph/View.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

static bool g_bRetrievedScriptInfos = false;
static QSet<QString> g_KeywordsBlue;
static QSet<QString> g_KeywordsPink;
static QSet<QString> g_KeywordsGreen;
static QSet<QString> g_BuiltIn;

//////////////////////////////////////////////////////////////////////////

WQtAngelScriptAssetDocumentWindow::WQtAngelScriptAssetDocumentWindow(WAngelScriptAssetDocument* pDocument)
  : WQtEngineDocumentWindow(pDocument)
{
  m_pAssetDoc = pDocument;

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "AngelScriptAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "AngelScriptAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("AngelScriptAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Central Widget
  {
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);

    m_pSourceLabel = new QTextEdit(this);
    m_pSourceLabel->setFont(font);
    m_pSourceLabel->setUndoRedoEnabled(false);
    m_pSourceLabel->setReadOnly(true);
    m_pSourceLabel->setTabStopDistance(m_pSourceLabel->tabStopDistance() / 4.0f);

    QFontMetricsF fm(m_pSourceLabel->font());
    auto stopWidth = 4 * fm.averageCharWidth();
    m_pSourceLabel->setTabStopDistance(ceil(stopWidth));

    connect(m_pSourceLabel, &QTextEdit::textChanged, this, &WQtAngelScriptAssetDocumentWindow::onTextEditTextChanged);

    m_pHighlighter = new ASHighlighter(m_pSourceLabel->document());

    WQtDocumentPanel* pCentral = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("AngelScriptView");
    pCentral->setWindowTitle("Script");
    pCentral->setWidget(m_pSourceLabel);

    m_pDockManager->setCentralWidget(pCentral);

    UpdateFileContentDisplay();
  }

  // Property Grid
  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("AngelScriptAssetDockWidget");
    pPropertyPanel->setWindowTitle("Angel Script Properties");
    pPropertyPanel->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new WQtAssetStatusIndicator((WAssetDocument*)GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_LastEdit = WTime::MakeZero();
  m_EditTimer.setInterval(100);
  connect(&m_EditTimer, &QTimer::timeout, this, &WQtAngelScriptAssetDocumentWindow::onEditTimer);

  WAssetCurator::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WQtAngelScriptAssetDocumentWindow::AssetEventHandler, this));
  m_pAssetDoc->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtAngelScriptAssetDocumentWindow::DocumentObjectEventHandler, this));

  FinishWindowCreation();

  RetrieveScriptInfos();
}

WQtAngelScriptAssetDocumentWindow::~WQtAngelScriptAssetDocumentWindow()
{
  m_pAssetDoc->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtAngelScriptAssetDocumentWindow::DocumentObjectEventHandler, this));
  WAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtAngelScriptAssetDocumentWindow::AssetEventHandler, this));
}

void WQtAngelScriptAssetDocumentWindow::DocumentObjectEventHandler(const WDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == WDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (!m_bIgnoreCodeChange && (e.m_sProperty == "Source" || e.m_sProperty == "Code"))
    {
      UpdateFileContentDisplay();
    }
  }
}

void WQtAngelScriptAssetDocumentWindow::StoreInlineDocState()
{
  WStringBuilder sContent = m_pSourceLabel->toPlainText().toUtf8().data();
  // sContent.ReplaceAll("\t", "    ");

  if (m_pAssetDoc->GetProperties()->m_CodeMode == WAngelScriptCodeMode::Inline && m_pAssetDoc->GetProperties()->m_sCode != sContent)
  {
    m_bIgnoreCodeChange = true;
    W_SCOPE_EXIT(m_bIgnoreCodeChange = false);

    WObjectCommandAccessor accessor(m_pAssetDoc->GetCommandHistory());
    accessor.StartTransaction("Edit Code");

    const WDocumentObject* pProps = m_pAssetDoc->GetPropertyObject();
    if (pProps)
    {
      accessor.SetValueByName(pProps, "Code", sContent.GetView()).AssertSuccess();
    }

    accessor.FinishTransaction();
  }
}

void WQtAngelScriptAssetDocumentWindow::onTextEditTextChanged()
{
  if (m_pAssetDoc->GetProperties()->m_CodeMode == WAngelScriptCodeMode::Inline)
  {
    if (!m_EditTimer.isActive())
    {
      m_EditTimer.start();
    }

    m_LastEdit = WTime::Now();
  }
}

void WQtAngelScriptAssetDocumentWindow::onEditTimer()
{
  if (m_LastEdit > m_LastSave)
  {
    if (WTime::Now() - m_LastSave > WTime::Seconds(0.5))
    {
      m_LastSave = WTime::Now();
      StoreInlineDocState();
    }
  }
  else
  {
    m_EditTimer.stop();
  }
}

void WQtAngelScriptAssetDocumentWindow::AssetEventHandler(const WAssetCuratorEvent& e)
{
  if (e.m_AssetGuid == m_pAssetDoc->GetGuid())
  {
    if (e.m_Type == WAssetCuratorEvent::Type::AssetUpdated)
    {
      UpdateFileContentDisplay();
    }
  }
}

void WQtAngelScriptAssetDocumentWindow::UpdateFileContentDisplay()
{
  if (m_pAssetDoc->GetProperties()->m_CodeMode == WAngelScriptCodeMode::Inline)
  {
    const auto& sCode = m_pAssetDoc->GetProperties()->m_sCode;

    m_pSourceLabel->setText(sCode.GetData());
    m_pSourceLabel->setReadOnly(false);
  }
  else
  {

    m_pSourceLabel->setReadOnly(true);

    const auto& sFile = m_pAssetDoc->GetProperties()->m_sScriptFile;

    if (sFile.IsEmpty())
    {
      m_pSourceLabel->setText("No script file specified.");
      return;
    }

    WFileReader file;
    if (file.Open(sFile).Failed())
    {
      m_pSourceLabel->setText(QString("Script file '%1' doesn't exist.").arg(sFile.GetData()));
      return;
    }

    WStringBuilder content;
    content.ReadAll(file);

    m_pSourceLabel->setText(content.GetData());
  }
}

void WQtAngelScriptAssetDocumentWindow::ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = WDynamicCast<const WSimpleDocumentConfigMsgToEditor*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "SyncExposedParams_Clear")
    {
      m_ExposedParams.Clear();
      m_Dependencies.Clear();
    }

    if (pMsg->m_sWhatToDo == "SyncExposedParams_Add")
    {
      auto& p = m_ExposedParams.ExpandAndGetRef();
      p.m_sName = pMsg->m_sPayload;
      p.m_DefaultValue = pMsg->m_PayloadValue;
    }

    if (pMsg->m_sWhatToDo == "SyncDependencies_Add")
    {
      m_Dependencies.PushBack(pMsg->m_sPayload);
    }

    if (pMsg->m_sWhatToDo == "SyncExposedParams_Finish")
    {
      WCommandHistory* history = m_pAssetDoc->GetCommandHistory();

      WObjectCommandAccessor accessor(history);

      auto pProps = m_pAssetDoc->GetPropertyObject();

      const WInt32 uiNum = accessor.GetCountByName(pProps, "Parameters");

      bool bAnyChange = (uiNum != m_ExposedParams.GetCount());

      WStringBuilder sDecl;

      for (WInt32 ui = 0; ui < uiNum; ++ui)
      {
        const WDocumentObject* pArgObj = accessor.GetChildObjectByName(pProps, "Parameters", ui);

        WVariant name, expose, def;
        accessor.GetValueByName(pArgObj, "Name", name).AssertSuccess();
        accessor.GetValueByName(pArgObj, "Expose", expose).AssertSuccess();
        accessor.GetValueByName(pArgObj, "DefaultValue", def).AssertSuccess();

        const WString sName = name.ConvertTo<WString>();
        bool bExpose = expose.ConvertTo<bool>();
        bool bFound = false;

        for (auto& ep : m_ExposedParams)
        {
          if (ep.m_sName == sName)
          {
            bFound = true;
            ep.m_bExpose = bExpose;

            if (ep.m_DefaultValue != def)
            {
              bAnyChange = true;
            }

            break;
          }
        }

        if (!bFound)
        {
          bAnyChange = true;
        }
      }


      if (bAnyChange || m_pAssetDoc->GetProperties()->m_Dependencies != m_Dependencies)
      {
        accessor.StartTransaction("Sync Parameters");

        // clear the entire array
        accessor.ClearByName(pProps, "Parameters").AssertSuccess();

        // and fill it again
        for (WUInt32 clip = 0; clip < m_ExposedParams.GetCount(); ++clip)
        {
          WUuid newItemGuid = WUuid::MakeUuid();
          accessor.AddObjectByName(pProps, "Parameters", -1, WGetStaticRTTI<WAngelScriptParameter>(), newItemGuid).AssertSuccess();

          const WDocumentObject* pNewItem = accessor.GetObject(newItemGuid);

          sDecl.SetFormat("{} {} = {}", WAngelScriptUtils::VariantTypeToString(m_ExposedParams[clip].m_DefaultValue.GetType()), m_ExposedParams[clip].m_sName, m_ExposedParams[clip].m_DefaultValue);

          accessor.SetValueByName(pNewItem, "Name", m_ExposedParams[clip].m_sName).AssertSuccess();
          accessor.SetValueByName(pNewItem, "Declaration", sDecl.GetData()).AssertSuccess();
          accessor.SetValueByName(pNewItem, "Expose", m_ExposedParams[clip].m_bExpose).AssertSuccess();
          accessor.SetValueByName(pNewItem, "DefaultValue", m_ExposedParams[clip].m_DefaultValue).AssertSuccess();
        }

        // clear the entire array
        accessor.ClearByName(pProps, "Dependencies").AssertSuccess();

        const WAbstractProperty* pPropDeps = accessor.FindPropertyByName(pProps, "Dependencies");

        // and fill it again
        for (WUInt32 clip = 0; clip < m_Dependencies.GetCount(); ++clip)
        {
          accessor.InsertValue(pProps, pPropDeps, m_Dependencies[clip], -1).AssertSuccess();
        }

        accessor.FinishTransaction();
      }
    }
  }
}

static void ReadSet(WStringView sBasePath, WStringView sFile, QSet<QString>& inout_set)
{
  const WStringBuilder fullPath(sBasePath, "/", sFile);

  WFileReader file;
  if (file.Open(fullPath).Failed())
    return;

  WStringBuilder tmp;
  tmp.ReadAll(file);

  WDynamicArray<WStringView> lines;
  tmp.Split(false, lines, "\n", "\r");

  for (WStringView line : lines)
  {
    line.Trim();

    if (!line.IsEmpty())
    {
      inout_set.insert(WMakeQString(line));
    }
  }
}

void WQtAngelScriptAssetDocumentWindow::RetrieveScriptInfos()
{
  if (g_bRetrievedScriptInfos)
    return;

  g_bRetrievedScriptInfos = true;

  WStringBuilder sBasePath = WToolsProject::GetSingleton()->GetProjectDataFolder();
  sBasePath.AppendPath("AngelScript");

  WDocumentConfigMsgToEngine msg;
  msg.m_sWhatToDo = "RetrieveScriptInfos";
  msg.m_sValue = sBasePath;
  GetDocument()->SendMessageToEngine(&msg);

  ReadSet(sBasePath, "Types.asgen", g_KeywordsGreen);
  ReadSet(sBasePath, "Namespaces.asgen", g_KeywordsGreen);
  ReadSet(sBasePath, "GlobalFunctions.asgen", g_BuiltIn);
  ReadSet(sBasePath, "Methods.asgen", g_BuiltIn);
  ReadSet(sBasePath, "Enums.asgen", g_BuiltIn);
  ReadSet(sBasePath, "Properties.asgen", g_BuiltIn);
}

ASHighlighter::ASHighlighter(QTextDocument* pParent)
  : QSyntaxHighlighter(pParent)
{
  // default color scheme
  m_Colors[ASEdit::Comment] = QColor("#6A8A35");
  m_Colors[ASEdit::Number] = QColor("#B5CEA8");
  m_Colors[ASEdit::String] = QColor("#CE916A");
  m_Colors[ASEdit::Operator] = QColor("#808000");
  m_Colors[ASEdit::KeywordBlue] = QColor("#569CCA");
  m_Colors[ASEdit::KeywordPink] = QColor("#C586C0");
  m_Colors[ASEdit::KeywordGreen] = QColor("#4EC9B0");
  m_Colors[ASEdit::BuiltIn] = QColor("#DCDCAA");

  g_KeywordsPink << "break";
  g_KeywordsPink << "case";
  g_KeywordsPink << "continue";
  g_KeywordsPink << "default";
  g_KeywordsPink << "do";
  g_KeywordsPink << "for";
  g_KeywordsPink << "return";
  g_KeywordsPink << "switch";
  g_KeywordsPink << "while";
  g_KeywordsPink << "if";
  g_KeywordsPink << "else";
  g_KeywordsPink << "in";
  g_KeywordsPink << "out";
  g_KeywordsPink << "inout";
  g_KeywordsPink << "is";
  g_KeywordsPink << "WAngelScriptClass";

  g_KeywordsBlue << "function";
  g_KeywordsBlue << "funcdef";
  g_KeywordsBlue << "import";
  g_KeywordsBlue << "cast";
  g_KeywordsBlue << "this";
  g_KeywordsBlue << "void";
  g_KeywordsBlue << "true";
  g_KeywordsBlue << "false";
  g_KeywordsBlue << "null";
  g_KeywordsBlue << "class";
  g_KeywordsBlue << "struct";
  g_KeywordsBlue << "const";
  g_KeywordsBlue << "enum";
  g_KeywordsBlue << "private";
  g_KeywordsBlue << "protected";
  g_KeywordsBlue << "auto";
  g_KeywordsBlue << "explicit";
  g_KeywordsBlue << "external";
  g_KeywordsBlue << "final";
  g_KeywordsBlue << "namespace";
  g_KeywordsBlue << "interface";
  g_KeywordsBlue << "mixin";
  g_KeywordsBlue << "abstract";
  g_KeywordsBlue << "not";
  g_KeywordsBlue << "and";
  g_KeywordsBlue << "or";
  g_KeywordsBlue << "xor";
  g_KeywordsBlue << "override";
  g_KeywordsBlue << "property";
  g_KeywordsBlue << "shared";
  g_KeywordsBlue << "super";
  g_KeywordsBlue << "try";
  g_KeywordsBlue << "catch";
  g_KeywordsBlue << "typedef";

  g_KeywordsGreen << "bool";
  g_KeywordsGreen << "float";
  g_KeywordsGreen << "double";
  g_KeywordsGreen << "int";
  g_KeywordsGreen << "int8";
  g_KeywordsGreen << "int16";
  g_KeywordsGreen << "int32";
  g_KeywordsGreen << "int64";
  g_KeywordsGreen << "uint8";
  g_KeywordsGreen << "uint16";
  g_KeywordsGreen << "uint32";
  g_KeywordsGreen << "uint64";
}


void ASHighlighter::highlightBlock(const QString& text)
{
  // parsing state
  enum
  {
    Start = 0,
    Number = 1,
    Identifier = 2,
    String = 3,
    Comment = 4,
    Regex = 5
  };

  QList<int> bracketPositions;

  int blockState = previousBlockState();
  int bracketLevel = blockState >> 4;
  int state = blockState & 15;
  if (blockState < 0)
  {
    bracketLevel = 0;
    state = Start;
  }

  int start = 0;
  int i = 0;
  while (i <= text.length())
  {
    QChar ch = (i < text.length()) ? text.at(i) : QChar();
    QChar next = (i < text.length() - 1) ? text.at(i + 1) : QChar();

    switch (state)
    {

      case Start:
        start = i;
        if (ch.isSpace())
        {
          ++i;
        }
        else if (ch.isDigit())
        {
          ++i;
          state = Number;
        }
        else if (ch.isLetter() || ch == '_')
        {
          ++i;
          state = Identifier;
        }
        else if (ch == '\'' || ch == '\"')
        {
          ++i;
          state = String;
        }
        else if (ch == '/' && next == '*')
        {
          ++i;
          ++i;
          state = Comment;
        }
        else if (ch == '/' && next == '/')
        {
          i = text.length();
          setFormat(start, text.length(), m_Colors[ASEdit::Comment]);
        }
        else if (ch == '/' && next != '*')
        {
          ++i;
          state = Regex;
        }
        else
        {
          if (!QString("(){}[]").contains(ch))
            setFormat(start, 1, m_Colors[ASEdit::Operator]);
          if (ch == '{' || ch == '}')
          {
            bracketPositions += i;
            if (ch == '{')
              bracketLevel++;
            else
              bracketLevel--;
          }
          ++i;
          state = Start;
        }
        break;

      case Number:
        if (ch.isSpace() || !ch.isDigit())
        {
          setFormat(start, i - start, m_Colors[ASEdit::Number]);
          state = Start;
        }
        else
        {
          ++i;
        }
        break;

      case Identifier:
        if (ch.isSpace() || !(ch.isDigit() || ch.isLetter() || ch == '_'))
        {
          QString token = text.mid(start, i - start).trimmed();
          if (g_KeywordsBlue.contains(token))
            setFormat(start, i - start, m_Colors[ASEdit::KeywordBlue]);
          if (g_KeywordsPink.contains(token))
            setFormat(start, i - start, m_Colors[ASEdit::KeywordPink]);
          if (g_KeywordsGreen.contains(token))
            setFormat(start, i - start, m_Colors[ASEdit::KeywordGreen]);
          else if (g_BuiltIn.contains(token))
            setFormat(start, i - start, m_Colors[ASEdit::BuiltIn]);
          state = Start;
        }
        else
        {
          ++i;
        }
        break;

      case String:
        if (ch == text.at(start))
        {
          QChar prev = (i > 0) ? text.at(i - 1) : QChar();
          if (prev != '\\')
          {
            ++i;
            setFormat(start, i - start, m_Colors[ASEdit::String]);
            state = Start;
          }
          else
          {
            ++i;
          }
        }
        else
        {
          ++i;
        }
        break;

      case Comment:
        if (ch == '*' && next == '/')
        {
          ++i;
          ++i;
          setFormat(start, i - start, m_Colors[ASEdit::Comment]);
          state = Start;
        }
        else
        {
          ++i;
        }
        break;

      case Regex:
        if (ch == '/')
        {
          QChar prev = (i > 0) ? text.at(i - 1) : QChar();
          if (prev != '\\')
          {
            ++i;
            setFormat(start, i - start, m_Colors[ASEdit::String]);
            state = Start;
          }
          else
          {
            ++i;
          }
        }
        else
        {
          ++i;
        }
        break;

      default:
        state = Start;
        break;
    }
  }

  if (state == Comment)
    setFormat(start, text.length(), m_Colors[ASEdit::Comment]);
  else
    state = Start;

  if (!bracketPositions.isEmpty())
  {
    ASBlockData* blockData = reinterpret_cast<ASBlockData*>(currentBlock().userData());
    if (!blockData)
    {
      blockData = new ASBlockData;
      currentBlock().setUserData(blockData);
    }
    blockData->bracketPositions = bracketPositions;
  }

  blockState = (state & 15) | (bracketLevel << 4);
  setCurrentBlockState(blockState);
}
