#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/SnapSettingsDlg.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>

WQtSnapSettingsDlg::WQtSnapSettingsDlg(QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);

  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0", 0.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_01", 0.01f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_05", 0.05f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_1", 0.1f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_125", 0.125f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_2", 0.2f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_25", 0.25f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_5", 0.5f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.1", 1.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.2", 2.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.4", 4.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.5", 5.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.8", 8.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.10", 10.0f});

  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.0_Degree", 0.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.1_Degree", 1.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.5_Degree", 5.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.10_Degree", 10.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.15_Degree", 15.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.22_5_Degree", 22.5f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.30_Degree", 30.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.45_Degree", 45.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.90_Degree", 90.0f});

  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.0", 0.0f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.0_125", 0.125f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.0_25", 0.25f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.0_5", 0.5f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.1", 1.0f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.2", 2.0f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.4", 4.0f});

  WUInt32 uiSelectedT = 0;
  WUInt32 uiSelectedR = 0;
  WUInt32 uiSelectedS = 0;

  for (WUInt32 i = 0; i < m_Translation.GetCount(); ++i)
  {
    TranslationSnap->addItem(WMakeQString(WTranslate(m_Translation[i].m_szKey)));

    if (WSnapProvider::GetTranslationSnapValue() == m_Translation[i].m_fValue)
      uiSelectedT = i;
  }

  for (WUInt32 i = 0; i < m_Rotation.GetCount(); ++i)
  {
    RotationSnap->addItem(WMakeQString(WTranslate(m_Rotation[i].m_szKey)));

    if (WSnapProvider::GetRotationSnapValue() == WAngle::MakeFromDegree(m_Rotation[i].m_fValue))
      uiSelectedR = i;
  }

  for (WUInt32 i = 0; i < m_Scale.GetCount(); ++i)
  {
    ScaleSnap->addItem(WMakeQString(WTranslate(m_Scale[i].m_szKey)));

    if (WSnapProvider::GetScaleSnapValue() == m_Scale[i].m_fValue)
      uiSelectedS = i;
  }

  TranslationSnap->setCurrentIndex(uiSelectedT);
  RotationSnap->setCurrentIndex(uiSelectedR);
  ScaleSnap->setCurrentIndex(uiSelectedS);
}

void WQtSnapSettingsDlg::QueryUI()
{
  WSnapProvider::SetTranslationSnapValue(m_Translation[TranslationSnap->currentIndex()].m_fValue);
  WSnapProvider::SetRotationSnapValue(WAngle::MakeFromDegree(m_Rotation[RotationSnap->currentIndex()].m_fValue));
  WSnapProvider::SetScaleSnapValue(m_Scale[ScaleSnap->currentIndex()].m_fValue);
}

void WQtSnapSettingsDlg::on_ButtonBox_clicked(QAbstractButton* button)
{
  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Ok))
  {
    QueryUI();
    accept();
    return;
  }

  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Cancel))
  {
    reject();
    return;
  }
}
