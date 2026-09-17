#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/ui_DeltaTransformDlg.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class WSceneDocument;

class WQtDeltaTransformDlg : public WQtDialog, public Ui_DeltaTransformDlg
{
  Q_OBJECT

public:
  WQtDeltaTransformDlg(QWidget* pParent, WSceneDocument* pSceneDoc);

  enum Mode
  {
    Translate,
    TranslateDeviation,
    RotateX,
    RotateXRandom,
    RotateXDeviation,
    RotateY,
    RotateYRandom,
    RotateYDeviation,
    RotateZ,
    RotateZRandom,
    RotateZDeviation,
    Scale,
    ScaleDeviation,
    UniformScale,
    UniformScaleDeviation,
    NaturalDeviationZ,
  };

  enum Space
  {
    World,
    LocalSelection,
    LocalEach,
  };

private Q_SLOTS:
  void on_ButtonApply_clicked();
  void on_ButtonUndo_clicked();
  void on_ComboMode_currentIndexChanged(int index);
  void on_ComboSpace_currentIndexChanged(int index);
  void on_Value1_valueChanged(double value);
  void on_Value2_valueChanged(double value);
  void on_Value3_valueChanged(double value);
  void on_CheckBoxSnapping_stateChanged(int state);

private:
  void QueryUI();
  void UpdateUI();

  static Mode s_Mode;
  static Space s_Space;
  static WVec3 s_vTranslate;
  static WVec3 s_vTranslateDeviation;
  static WVec3 s_vScale;
  static WVec3 s_vScaleDeviation;
  static float s_fUniformScale;
  static float s_fUniformScaleDeviation;
  static WVec3 s_vRotate;
  static WVec3 s_vRotateRandom;
  static WVec3 s_vRotateDeviation;
  static float s_fNaturalDeviationZ;
  static bool s_bUseCurrentSnapSettings;

  WUInt32 m_uiActionsApplied = 0;
  WSceneDocument* m_pSceneDocument = nullptr;
};
