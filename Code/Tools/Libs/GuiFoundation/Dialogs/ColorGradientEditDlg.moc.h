#pragma once

#include <Foundation/Tracks/ColorGradient.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ColorGradientEditDlg.h>

class WObjectAccessorBase;
class WDocumentObject;

class W_GUIFOUNDATION_DLL WQtColorGradientEditDlg : public WQtDialog, public Ui_ColorGradientEditDlg
{
  Q_OBJECT

public:
  WQtColorGradientEditDlg(WObjectAccessorBase* pObjectAccessor, const WDocumentObject* pGradientObject, QWidget* pParent, WStringView sTitle = {});
  ~WQtColorGradientEditDlg();

  static QByteArray GetLastDialogGeometry() { return s_LastDialogGeometry; }

  virtual void reject() override;
  virtual void accept() override;

  void cancel();

private Q_SLOTS:
  // Color CP operations
  void OnColorCpAdded(double fPosX, const WColorGammaUB& color);
  void OnColorCpMoved(WInt32 iIndex, double fNewPosX);
  void OnColorCpDeleted(WInt32 iIndex);
  void OnColorCpChanged(WInt32 iIndex, const WColorGammaUB& color);

  // Alpha CP operations
  void OnAlphaCpAdded(double fPosX, WUInt8 uiAlpha);
  void OnAlphaCpMoved(WInt32 iIndex, double fNewPosX);
  void OnAlphaCpDeleted(WInt32 iIndex);
  void OnAlphaCpChanged(WInt32 iIndex, WUInt8 uiAlpha);

  // Intensity CP operations
  void OnIntensityCpAdded(double fPosX, float fIntensity);
  void OnIntensityCpMoved(WInt32 iIndex, double fNewPosX);
  void OnIntensityCpDeleted(WInt32 iIndex);
  void OnIntensityCpChanged(WInt32 iIndex, float fIntensity);

  // Operation boundaries
  void OnBeginOperation();
  void OnEndOperation(bool bCommit);

  // UI actions
  void OnNormalizeRange();
  void on_actionUndo_triggered();
  void on_actionRedo_triggered();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonUndo_clicked();
  void on_ButtonRedo_clicked();

private:
  static QByteArray s_LastDialogGeometry;

  void RetrieveGradientState();
  void UpdatePreview();
  void UpdateUndoRedoState();

  WColorGradient m_Gradient;
  WUInt32 m_uiActionsUndoBaseline = 0;

  QShortcut* m_pShortcutUndo = nullptr;
  QShortcut* m_pShortcutRedo = nullptr;

  WObjectAccessorBase* m_pObjectAccessor = nullptr;
  const WDocumentObject* m_pGradientObject = nullptr;

protected:
  virtual void closeEvent(QCloseEvent* e) override;
  virtual void showEvent(QShowEvent* e) override;
};
