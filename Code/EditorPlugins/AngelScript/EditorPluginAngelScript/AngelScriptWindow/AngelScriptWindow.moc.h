#pragma once

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <QTimer>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WAngelScriptAssetDocument;
struct WAssetCuratorEvent;
class WEditorEngineDocumentMsg;
class QTextEdit;

class WQtAngelScriptAssetDocumentWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT

public:
  WQtAngelScriptAssetDocumentWindow(WAngelScriptAssetDocument* pDocument);
  ~WQtAngelScriptAssetDocumentWindow();

private Q_SLOTS:
  void onTextEditTextChanged();
  void onEditTimer();

private:
  void AssetEventHandler(const WAssetCuratorEvent& e);
  void UpdateFileContentDisplay();
  virtual void ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg) override;
  void RetrieveScriptInfos();
  void DocumentObjectEventHandler(const WDocumentObjectPropertyEvent& e);
  void StoreInlineDocState();

  struct ExposedParam
  {
    WString m_sName;
    WVariant m_DefaultValue;
    bool m_bExpose = false;
  };

  WDynamicArray<ExposedParam> m_ExposedParams;
  WDynamicArray<WString> m_Dependencies;

  WAngelScriptAssetDocument* m_pAssetDoc = nullptr;

  bool m_bIgnoreCodeChange = false;
  QTextEdit* m_pSourceLabel = nullptr;
  QSyntaxHighlighter* m_pHighlighter = nullptr;
  QTimer m_EditTimer;
  WTime m_LastEdit;
  WTime m_LastSave;
};


//////////////////////////////////////////////////////////////////////////

#include <QSyntaxHighlighter>

struct ASEdit
{
  enum ColorComponent
  {
    Comment,
    Number,
    String,
    Operator,
    KeywordBlue,
    KeywordPink,
    KeywordGreen,
    BuiltIn,

    Count,
  };
};

class ASBlockData : public QTextBlockUserData
{
public:
  QList<int> bracketPositions;
};

class ASHighlighter : public QSyntaxHighlighter
{
public:
  ASHighlighter(QTextDocument* pParent = 0);

protected:
  void highlightBlock(const QString& text) override;

private:
  QColor m_Colors[ASEdit::Count];
};
