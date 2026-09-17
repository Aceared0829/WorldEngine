#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetProfilesDlg.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

#include <GuiFoundation/Dialogs/Dialog.moc.h>

class WAssetProfilesDocument;
class WPlatformProfile;
class WQtDocumentTreeView;
class WDocument;
struct WDocumentObjectPropertyEvent;

class W_EDITORFRAMEWORK_DLL WQtAssetProfilesDlg : public WQtDialog, public Ui_WQtAssetProfilesDlg
{
public:
  Q_OBJECT

public:
  WQtAssetProfilesDlg(QWidget* pParent);
  ~WQtAssetProfilesDlg();

  WUInt32 m_uiActiveConfig = 0;

private Q_SLOTS:
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void OnItemDoubleClicked(QModelIndex idx);
  void on_AddButton_clicked();
  void on_DeleteButton_clicked();
  void on_RenameButton_clicked();
  void on_SwitchToButton_clicked();

private:
  struct Binding
  {
    enum class State
    {
      None,
      Added,
      Deleted
    };

    State m_State = State::None;
    WPlatformProfile* m_pProfile = nullptr;
  };

  bool DetermineNewProfileName(QWidget* parent, WString& result);
  bool CheckProfileNameUniqueness(const char* szName);
  void AllAssetProfilesToObject();
  void PropertyChangedEventHandler(const WDocumentObjectPropertyEvent& e);
  void ApplyAllChanges();
  WUuid NativeToObject(WPlatformProfile* pProfile);
  void ObjectToNative(WUuid objectGuid, WPlatformProfile* pProfile);
  void SelectionEventHandler(const WSelectionManagerEvent& e);

  WAssetProfilesDocument* m_pDocument;
  WMap<WUuid, Binding> m_ProfileBindings;
};
