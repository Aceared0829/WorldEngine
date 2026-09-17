#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_PreferencesDlg.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class WPreferencesDocument;
class WPreferences;
class WQtDocumentTreeView;

class W_EDITORFRAMEWORK_DLL WQtPreferencesDlg : public WQtDialog, public Ui_WQtPreferencesDlg
{
public:
  Q_OBJECT

public:
  WQtPreferencesDlg(QWidget* pParent);
  ~WQtPreferencesDlg();

  WUuid NativeToObject(WPreferences* pPreferences);
  void ObjectToNative(WUuid objectGuid, const WDocument* pPrefDocument);


private Q_SLOTS:
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked() { reject(); }

private:
  void RegisterAllPreferenceTypes();
  void AllPreferencesToObject();
  void PropertyChangedEventHandler(const WDocumentObjectPropertyEvent& e);
  void ApplyAllChanges();

  WPreferencesDocument* m_pDocument;
  WMap<WUuid, const WDocument*> m_DocumentBinding;
};
