#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class W_EDITORFRAMEWORK_DLL WQtCompilerPreferencesWidget : public WQtPropertyTypeWidget
{
  Q_OBJECT;

private Q_SLOTS:

  void on_compiler_preset_changed(int index);

public:
  explicit WQtCompilerPreferencesWidget();
  virtual ~WQtCompilerPreferencesWidget();

  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;

protected:
  QComboBox* m_pCompilerPreset;
};
