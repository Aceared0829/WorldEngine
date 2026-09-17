#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <QLabel>
#include <QLineEdit>

class WQtFileLineEdit;

class W_EDITORFRAMEWORK_DLL WQtFilePropertyWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtFilePropertyWidget();
  bool IsValidFileReference(WStringView sFile) const;
  void SetReadOnly(bool bReadOnly = true) override;

private Q_SLOTS:
  void on_BrowseFile_clicked();

protected slots:
  void on_TextFinished_triggered();
  void on_TextChanged_triggered(const QString& value);
  void OnOpenExplorer();
  void OnCustomAction();
  void OnOpenFile();
  void OnOpenFileWith();
  void OnCreateFile();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

protected:
  void UpdateRequiredIndicator(bool bValueEmpty);

protected:
  QHBoxLayout* m_pLayout = nullptr;
  WQtFileLineEdit* m_pWidget = nullptr;
  QToolButton* m_pButton = nullptr;
  QLabel* m_pWarningIcon = nullptr;
};

class W_EDITORFRAMEWORK_DLL WQtExternalFilePropertyWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT

public:
  WQtExternalFilePropertyWidget();
  bool IsValidFileReference(WStringView sFile) const;

private Q_SLOTS:
  void on_BrowseFile_clicked();

protected slots:
  void on_TextFinished_triggered();
  void on_TextChanged_triggered(const QString& value);
  void OnOpenExplorer();
  void OnOpenFile();
  void OnOpenFileWith();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

protected:
  QHBoxLayout* m_pLayout = nullptr;
  QLineEdit* m_pWidget = nullptr;
  QToolButton* m_pButton = nullptr;
};
