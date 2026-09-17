#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_PluginSelectionWidget.h>
#include <Foundation/Strings/String.h>
#include <QWidget>

struct WPluginBundleSet;
struct WPluginBundle;

class W_EDITORFRAMEWORK_DLL WQtPluginSelectionWidget : public QWidget, public Ui_PluginSelectionWidget
{
public:
  Q_OBJECT

public:
  WQtPluginSelectionWidget(QWidget* pParent);
  ~WQtPluginSelectionWidget();

  void SetPluginSet(WPluginBundleSet* pPluginSet);
  void SyncStateToSet();
  void SelectTemplate(const char* szTemplate);

private Q_SLOTS:
  void on_PluginsList_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
  void on_PluginsList_itemChanged(QListWidgetItem* item);
  void on_Template_currentIndexChanged(int index);

private:
  struct State
  {
    WString m_sID;
    WPluginBundle* m_pInfo = nullptr;
    bool m_bLoadCopy = false;
    bool m_bSelected = false;
    bool m_bIsDependency = false;
  };

  void UpdateInternalState();
  void ApplyRequired(WArrayPtr<WString> required);

  WHybridArray<State, 8> m_States;
  WPluginBundleSet* m_pPluginSet = nullptr;
};
