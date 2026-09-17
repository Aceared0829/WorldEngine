#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class W_EDITORFRAMEWORK_DLL WQtCodeEditorPreferencesWidget : public WQtPropertyTypeWidget
{
  Q_OBJECT;

private Q_SLOTS:
  void on_code_editor_changed(int index);

public:
  explicit WQtCodeEditorPreferencesWidget();
  virtual ~WQtCodeEditorPreferencesWidget();

  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;

protected:
  QComboBox* m_pCodeEditor;
};
