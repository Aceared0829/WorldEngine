#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <QFrame>

struct WAssetCuratorEvent;
class WAssetDocument;
struct WDocumentEvent;
class QPushButton;

/// A small widget that displays the current transform status of an asset.
///
/// Clicking it allows to re-transform, or see the error log.
class W_EDITORFRAMEWORK_DLL WQtAssetStatusIndicator : public QFrame
{
  Q_OBJECT

public:
  WQtAssetStatusIndicator(WAssetDocument* pDoc, QWidget* pParent = nullptr);
  ~WQtAssetStatusIndicator();

private Q_SLOTS:
  void onClick(bool);
  void onHelp(bool);

private:
  void DocumentEventHandler(const WDocumentEvent& e);
  void AssetEventHandler(const WAssetCuratorEvent& e);

  void UpdateDisplay();

  enum class Action
  {
    None,
    Save,
    Transform,
    ShowErrors,
  };

  WAssetDocument* m_pAsset = nullptr;
  QPushButton* m_pLabel = nullptr;
  QPushButton* m_pHelp = nullptr;
  Action m_Action = Action::None;
};
