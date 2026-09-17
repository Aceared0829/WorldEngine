#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/ui_DuplicateDlg.h>

#include <GuiFoundation/Dialogs/Dialog.moc.h>

class WQtDuplicateDlg : public WQtDialog, public Ui_DuplicateDlg
{
  Q_OBJECT

public:
  WQtDuplicateDlg(QWidget* pParent = nullptr);

  static WUInt32 s_uiNumberOfCopies;
  static bool s_bGroupCopies;
  static WVec3 s_vTranslationStep;
  static WVec3 s_vRotationStep;
  static WVec3 s_vRandomTranslation;
  static WVec3 s_vRandomRotation;
  static int s_iRevolveStartAngle;
  static int s_iRevolveAngleStep;
  static float s_fRevolveRadius;
  static int s_iRevolveAxis;

public Q_SLOTS:
  virtual void on_DefaultButtons_clicked(QAbstractButton* pButton);

  virtual void on_toolButtonTransX_clicked();
  virtual void on_toolButtonTransY_clicked();
  virtual void on_toolButtonTransZ_clicked();

  virtual void on_RevolveNone_clicked();
  virtual void on_RevolveX_clicked();
  virtual void on_RevolveY_clicked();
  virtual void on_RevolveZ_clicked();

private:
  WVec3 m_vBoundingBoxSize;
};
