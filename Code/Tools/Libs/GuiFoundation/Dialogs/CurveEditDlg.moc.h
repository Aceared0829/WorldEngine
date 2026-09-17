#pragma once

#include <Foundation/Tracks/CurveEditData.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_CurveEditDlg.h>

class WCurveGroupData;
class WObjectAccessorBase;
class WDocumentObject;

class W_GUIFOUNDATION_DLL WQtCurveEditDlg : public WQtDialog, Ui_CurveEditDlg
{
  Q_OBJECT
public:
  WQtCurveEditDlg(WObjectAccessorBase* pObjectAccessor, const WDocumentObject* pCurveObject, QWidget* pParent, WStringView sTitle = {});
  ~WQtCurveEditDlg();

  static QByteArray GetLastDialogGeometry() { return s_LastDialogGeometry; }

  void SetCurveColor(const WColor& color);
  void SetCurveExtents(double fLower, bool bLowerFixed, double fUpper, bool bUpperFixed);
  void SetCurveRanges(double fLower, double fUpper);

  virtual void reject() override;
  virtual void accept() override;

  void cancel();

Q_SIGNALS:

private Q_SLOTS:
  void OnCpMovedEvent(WUInt32 curveIdx, WUInt32 cpIdx, WInt64 iTickX, double newPosY);
  void OnCpDeletedEvent(WUInt32 curveIdx, WUInt32 cpIdx);
  void OnTangentMovedEvent(WUInt32 curveIdx, WUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void OnInsertCpEvent(WUInt32 uiCurveIdx, WInt64 tickX, double value);
  void OnTangentLinkEvent(WUInt32 curveIdx, WUInt32 cpIdx, bool bLink);
  void OnCpTangentModeEvent(WUInt32 curveIdx, WUInt32 cpIdx, bool rightTangent, int mode); // WCurveTangentMode

  void OnBeginCpChangesEvent(QString name);
  void OnEndCpChangesEvent();

  void OnBeginOperationEvent(QString name);
  void OnEndOperationEvent(bool commit);

  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonUndo_clicked();
  void on_ButtonRedo_clicked();

private:
  void on_actionUndo_triggered();
  void on_actionRedo_triggered();

  static QByteArray s_LastDialogGeometry;

  void RetrieveCurveState();
  void UpdatePreview();
  void UpdateUndoRedoState();

  double m_fLowerRange = -WMath::HighValue<double>();
  double m_fUpperRange = WMath::HighValue<double>();
  double m_fLowerExtents = 0.0;
  double m_fUpperExtents = 1.0;
  bool m_bLowerFixed = false;
  bool m_bUpperFixed = false;
  bool m_bCurveLengthIsFixed = false;
  WCurveGroupData m_Curves;
  WUInt32 m_uiActionsUndoBaseline = 0;

  QShortcut* m_pShortcutUndo = nullptr;
  QShortcut* m_pShortcutRedo = nullptr;

  WObjectAccessorBase* m_pObjectAccessor = nullptr;
  const WDocumentObject* m_pCurveObject = nullptr;

  WInt32 m_iInsertedCurveIdx = -1;
  WUInt32 m_uiInsertedPointIdx = 0;

protected:
  virtual void closeEvent(QCloseEvent* e) override;
  virtual void showEvent(QShowEvent* e) override;
};
