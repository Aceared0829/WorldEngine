#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Tracks/ColorGradient.h>
#include <GuiFoundation/Dialogs/ColorGradientEditDlg.moc.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>
#include <QShortcut>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

QByteArray WQtColorGradientEditDlg::s_LastDialogGeometry;

WQtColorGradientEditDlg::WQtColorGradientEditDlg(WObjectAccessorBase* pObjectAccessor, const WDocumentObject* pGradientObject, QWidget* pParent, WStringView sTitle)
  : WQtDialog(pParent)
{
  m_pObjectAccessor = pObjectAccessor;
  m_pGradientObject = pGradientObject;

  setupUi(this);

  if (!sTitle.IsEmpty())
  {
    WStringBuilder tmp;
    setWindowTitle(sTitle.GetData(tmp));
  }

  WQtColorGradientEditorWidget* pEdit = GradientEditor;

  // Connect color CP signals
  connect(pEdit, &WQtColorGradientEditorWidget::ColorCpAdded, this, &WQtColorGradientEditDlg::OnColorCpAdded);
  connect(pEdit, &WQtColorGradientEditorWidget::ColorCpMoved, this, &WQtColorGradientEditDlg::OnColorCpMoved);
  connect(pEdit, &WQtColorGradientEditorWidget::ColorCpDeleted, this, &WQtColorGradientEditDlg::OnColorCpDeleted);
  connect(pEdit, &WQtColorGradientEditorWidget::ColorCpChanged, this, &WQtColorGradientEditDlg::OnColorCpChanged);

  // Connect alpha CP signals
  connect(pEdit, &WQtColorGradientEditorWidget::AlphaCpAdded, this, &WQtColorGradientEditDlg::OnAlphaCpAdded);
  connect(pEdit, &WQtColorGradientEditorWidget::AlphaCpMoved, this, &WQtColorGradientEditDlg::OnAlphaCpMoved);
  connect(pEdit, &WQtColorGradientEditorWidget::AlphaCpDeleted, this, &WQtColorGradientEditDlg::OnAlphaCpDeleted);
  connect(pEdit, &WQtColorGradientEditorWidget::AlphaCpChanged, this, &WQtColorGradientEditDlg::OnAlphaCpChanged);

  // Connect intensity CP signals
  connect(pEdit, &WQtColorGradientEditorWidget::IntensityCpAdded, this, &WQtColorGradientEditDlg::OnIntensityCpAdded);
  connect(pEdit, &WQtColorGradientEditorWidget::IntensityCpMoved, this, &WQtColorGradientEditDlg::OnIntensityCpMoved);
  connect(pEdit, &WQtColorGradientEditorWidget::IntensityCpDeleted, this, &WQtColorGradientEditDlg::OnIntensityCpDeleted);
  connect(pEdit, &WQtColorGradientEditorWidget::IntensityCpChanged, this, &WQtColorGradientEditDlg::OnIntensityCpChanged);

  // Connect operation boundaries
  connect(pEdit, &WQtColorGradientEditorWidget::BeginOperation, this, &WQtColorGradientEditDlg::OnBeginOperation);
  connect(pEdit, &WQtColorGradientEditorWidget::EndOperation, this, &WQtColorGradientEditDlg::OnEndOperation);

  // Connect utility functions
  connect(pEdit, &WQtColorGradientEditorWidget::NormalizeRange, this, &WQtColorGradientEditDlg::OnNormalizeRange);

  // Setup keyboard shortcuts
  m_pShortcutUndo = new QShortcut(QKeySequence("Ctrl+Z"), this);
  m_pShortcutRedo = new QShortcut(QKeySequence("Ctrl+Y"), this);

  connect(m_pShortcutUndo, &QShortcut::activated, this, &WQtColorGradientEditDlg::on_actionUndo_triggered);
  connect(m_pShortcutRedo, &QShortcut::activated, this, &WQtColorGradientEditDlg::on_actionRedo_triggered);

  RetrieveGradientState();

  m_uiActionsUndoBaseline = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->GetUndoStackSize();

  UpdateUndoRedoState();
}

WQtColorGradientEditDlg::~WQtColorGradientEditDlg()
{
  s_LastDialogGeometry = saveGeometry();
}

void WQtColorGradientEditDlg::RetrieveGradientState()
{
  m_Gradient.Clear();
  WVariant v;

  // Retrieve ColorCPs
  WInt32 iNumColorCPs = 0;
  m_pObjectAccessor->GetCountByName(m_pGradientObject, "ColorCPs", iNumColorCPs).AssertSuccess();

  for (WInt32 i = 0; i < iNumColorCPs; ++i)
  {
    const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "ColorCPs", i);

    m_pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    WInt64 tick = v.ConvertTo<WInt64>();

    m_pObjectAccessor->GetValueByName(pCP, "Red", v).AssertSuccess();
    WUInt8 r = v.ConvertTo<WUInt8>();
    m_pObjectAccessor->GetValueByName(pCP, "Green", v).AssertSuccess();
    WUInt8 g = v.ConvertTo<WUInt8>();
    m_pObjectAccessor->GetValueByName(pCP, "Blue", v).AssertSuccess();
    WUInt8 b = v.ConvertTo<WUInt8>();

    m_Gradient.AddColorControlPoint(WColorGradient::TickToTime(tick), WColorGammaUB(r, g, b));
  }

  // Retrieve AlphaCPs
  WInt32 iNumAlphaCPs = 0;
  m_pObjectAccessor->GetCountByName(m_pGradientObject, "AlphaCPs", iNumAlphaCPs).AssertSuccess();

  for (WInt32 i = 0; i < iNumAlphaCPs; ++i)
  {
    const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "AlphaCPs", i);

    m_pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    WInt64 tick = v.ConvertTo<WInt64>();
    m_pObjectAccessor->GetValueByName(pCP, "Alpha", v).AssertSuccess();
    WUInt8 alpha = v.ConvertTo<WUInt8>();

    m_Gradient.AddAlphaControlPoint(WColorGradient::TickToTime(tick), alpha);
  }

  // Retrieve IntensityCPs
  WInt32 iNumIntensityCPs = 0;
  m_pObjectAccessor->GetCountByName(m_pGradientObject, "IntensityCPs", iNumIntensityCPs).AssertSuccess();

  for (WInt32 i = 0; i < iNumIntensityCPs; ++i)
  {
    const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "IntensityCPs", i);

    m_pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    WInt64 tick = v.ConvertTo<WInt64>();
    m_pObjectAccessor->GetValueByName(pCP, "Intensity", v).AssertSuccess();
    float intensity = v.ConvertTo<float>();

    m_Gradient.AddIntensityControlPoint(WColorGradient::TickToTime(tick), intensity);
  }
}

void WQtColorGradientEditDlg::reject()
{
  // Ignore - use cancel() instead
}

void WQtColorGradientEditDlg::accept()
{
  // Ignore - handled by OK button
}

void WQtColorGradientEditDlg::cancel()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  cmd.Undo(cmd.GetUndoStackSize() - m_uiActionsUndoBaseline).AssertSuccess();

  QDialog::reject();
}

void WQtColorGradientEditDlg::UpdatePreview()
{
  WQtColorGradientEditorWidget* pEdit = GradientEditor;
  pEdit->SetColorGradient(m_Gradient);
}

void WQtColorGradientEditDlg::closeEvent(QCloseEvent*)
{
  cancel();
}

void WQtColorGradientEditDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);
  UpdatePreview();
}

// Color CP handlers
void WQtColorGradientEditDlg::OnColorCpAdded(double fPosX, const WColorGammaUB& color)
{
  fPosX = WColorGradient::SnapTimeTo(fPosX);

  // Update local representation
  m_Gradient.AddColorControlPoint(fPosX, color);

  // Update document object
  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Color Control Point");

  WUuid guid;
  m_pObjectAccessor->AddObjectByName(m_pGradientObject, "ColorCPs", -1, WGetStaticRTTI<WColorGradientColorCP>(), guid).AssertSuccess();

  const WDocumentObject* pCP = m_pObjectAccessor->GetObject(guid);
  WInt64 tick = WColorGradient::TimeToTick(fPosX);

  m_pObjectAccessor->SetValueByName(pCP, "Tick", tick).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Red", color.r).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Green", color.g).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Blue", color.b).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void WQtColorGradientEditDlg::OnColorCpMoved(WInt32 iIndex, double fNewPosX)
{
  // Update local representation
  auto& cp = m_Gradient.ModifyColorControlPoint(iIndex);
  cp.m_iTick = WColorGradient::SnapTimeToTick(fNewPosX);

  // Update document object
  const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "ColorCPs", iIndex);
  m_pObjectAccessor->SetValueByName(pCP, "Tick", cp.m_iTick).AssertSuccess();

  // Update the widget during drag
  UpdatePreview();
}

void WQtColorGradientEditDlg::OnColorCpDeleted(WInt32 iIndex)
{
  // Update document object
  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "ColorCPs", iIndex);
  m_pObjectAccessor->RemoveObject(pCP).AssertSuccess();

  history->FinishTransaction();

  // Retrieve updated state
  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void WQtColorGradientEditDlg::OnColorCpChanged(WInt32 iIndex, const WColorGammaUB& color)
{
  // Update local representation
  auto& cp = m_Gradient.ModifyColorControlPoint(iIndex);
  cp.m_GammaRed = color.r;
  cp.m_GammaGreen = color.g;
  cp.m_GammaBlue = color.b;

  // Update document object
  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Color");

  const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "ColorCPs", iIndex);
  m_pObjectAccessor->SetValueByName(pCP, "Red", color.r).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Green", color.g).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Blue", color.b).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

// Alpha CP handlers
void WQtColorGradientEditDlg::OnAlphaCpAdded(double fPosX, WUInt8 uiAlpha)
{
  fPosX = WColorGradient::SnapTimeTo(fPosX);

  m_Gradient.AddAlphaControlPoint(fPosX, uiAlpha);

  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Alpha Control Point");

  WUuid guid;
  m_pObjectAccessor->AddObjectByName(m_pGradientObject, "AlphaCPs", -1, WGetStaticRTTI<WColorGradientAlphaCP>(), guid).AssertSuccess();

  const WDocumentObject* pCP = m_pObjectAccessor->GetObject(guid);
  WInt64 tick = WColorGradient::SnapTimeToTick(fPosX);

  m_pObjectAccessor->SetValueByName(pCP, "Tick", tick).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Alpha", uiAlpha).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void WQtColorGradientEditDlg::OnAlphaCpMoved(WInt32 iIndex, double fNewPosX)
{
  auto& cp = m_Gradient.ModifyAlphaControlPoint(iIndex);
  cp.m_iTick = WColorGradient::SnapTimeToTick(fNewPosX);

  const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "AlphaCPs", iIndex);
  m_pObjectAccessor->SetValueByName(pCP, "Tick", cp.m_iTick).AssertSuccess();

  // Update the widget during drag
  UpdatePreview();
}

void WQtColorGradientEditDlg::OnAlphaCpDeleted(WInt32 iIndex)
{
  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "AlphaCPs", iIndex);
  m_pObjectAccessor->RemoveObject(pCP).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void WQtColorGradientEditDlg::OnAlphaCpChanged(WInt32 iIndex, WUInt8 uiAlpha)
{
  auto& cp = m_Gradient.ModifyAlphaControlPoint(iIndex);
  cp.m_Alpha = uiAlpha;

  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Alpha");

  const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "AlphaCPs", iIndex);
  m_pObjectAccessor->SetValueByName(pCP, "Alpha", uiAlpha).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

// Intensity CP handlers
void WQtColorGradientEditDlg::OnIntensityCpAdded(double fPosX, float fIntensity)
{
  fPosX = WColorGradient::SnapTimeTo(fPosX);

  m_Gradient.AddIntensityControlPoint(fPosX, fIntensity);

  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Intensity Control Point");

  WUuid guid;
  m_pObjectAccessor->AddObjectByName(m_pGradientObject, "IntensityCPs", -1, WGetStaticRTTI<WColorGradientIntensityCP>(), guid).AssertSuccess();

  const WDocumentObject* pCP = m_pObjectAccessor->GetObject(guid);
  WInt64 tick = WColorGradient::SnapTimeToTick(fPosX);

  m_pObjectAccessor->SetValueByName(pCP, "Tick", tick).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Intensity", fIntensity).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void WQtColorGradientEditDlg::OnIntensityCpMoved(WInt32 iIndex, double fNewPosX)
{
  auto& cp = m_Gradient.ModifyIntensityControlPoint(iIndex);
  cp.m_iTick = WColorGradient::SnapTimeToTick(fNewPosX);

  const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "IntensityCPs", iIndex);
  m_pObjectAccessor->SetValueByName(pCP, "Tick", cp.m_iTick).AssertSuccess();

  // Update the widget during drag
  UpdatePreview();
}

void WQtColorGradientEditDlg::OnIntensityCpDeleted(WInt32 iIndex)
{
  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "IntensityCPs", iIndex);
  m_pObjectAccessor->RemoveObject(pCP).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void WQtColorGradientEditDlg::OnIntensityCpChanged(WInt32 iIndex, float fIntensity)
{
  auto& cp = m_Gradient.ModifyIntensityControlPoint(iIndex);
  cp.m_Intensity = fIntensity;

  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Intensity");

  const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "IntensityCPs", iIndex);
  m_pObjectAccessor->SetValueByName(pCP, "Intensity", fIntensity).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

// Operation boundaries
void WQtColorGradientEditDlg::OnBeginOperation()
{
  m_pObjectAccessor->BeginTemporaryCommands("Modify Gradient");
}

void WQtColorGradientEditDlg::OnEndOperation(bool bCommit)
{
  if (bCommit)
  {
    m_pObjectAccessor->FinishTemporaryCommands();
    RetrieveGradientState();
    UpdatePreview();
  }
  else
  {
    m_pObjectAccessor->CancelTemporaryCommands();
    RetrieveGradientState();
    UpdatePreview();
  }

  UpdateUndoRedoState();
}

void WQtColorGradientEditDlg::OnNormalizeRange()
{
  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Normalize Gradient Range");

  // Find min/max tick values across all control points
  double fMinTime = WMath::MaxValue<double>();
  double fMaxTime = -WMath::MaxValue<double>();

  m_Gradient.GetExtents(fMinTime, fMaxTime);

  if (fMinTime >= fMaxTime || (fMinTime == 0.0 && fMaxTime == 1.0))
  {
    history->CancelTransaction();
    return; // Already normalized or invalid
  }

  const double fRange = fMaxTime - fMinTime;

  // Normalize all control points
  WInt32 iNumCPs = 0;

  // Color CPs
  m_pObjectAccessor->GetCountByName(m_pGradientObject, "ColorCPs", iNumCPs).AssertSuccess();
  for (WInt32 i = 0; i < iNumCPs; ++i)
  {
    const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "ColorCPs", i);
    WVariant v;
    m_pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    WInt64 oldTick = v.ConvertTo<WInt64>();
    double oldTime = WColorGradient::TickToTime(oldTick);
    double newTime = (oldTime - fMinTime) / fRange;
    WInt64 newTick = WColorGradient::TimeToTick(newTime);
    m_pObjectAccessor->SetValueByName(pCP, "Tick", newTick).AssertSuccess();
  }

  // Alpha CPs
  m_pObjectAccessor->GetCountByName(m_pGradientObject, "AlphaCPs", iNumCPs).AssertSuccess();
  for (WInt32 i = 0; i < iNumCPs; ++i)
  {
    const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "AlphaCPs", i);
    WVariant v;
    m_pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    WInt64 oldTick = v.ConvertTo<WInt64>();
    double oldTime = WColorGradient::TickToTime(oldTick);
    double newTime = (oldTime - fMinTime) / fRange;
    WInt64 newTick = WColorGradient::TimeToTick(newTime);
    m_pObjectAccessor->SetValueByName(pCP, "Tick", newTick).AssertSuccess();
  }

  // Intensity CPs
  m_pObjectAccessor->GetCountByName(m_pGradientObject, "IntensityCPs", iNumCPs).AssertSuccess();
  for (WInt32 i = 0; i < iNumCPs; ++i)
  {
    const WDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "IntensityCPs", i);
    WVariant v;
    m_pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    WInt64 oldTick = v.ConvertTo<WInt64>();
    double oldTime = WColorGradient::TickToTime(oldTick);
    double newTime = (oldTime - fMinTime) / fRange;
    WInt64 newTick = WColorGradient::TimeToTick(newTime);
    m_pObjectAccessor->SetValueByName(pCP, "Tick", newTick).AssertSuccess();
  }

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void WQtColorGradientEditDlg::UpdateUndoRedoState()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();

  ButtonUndo->setEnabled(cmd.CanUndo());
  ButtonRedo->setEnabled(cmd.CanRedo());
}

void WQtColorGradientEditDlg::on_actionUndo_triggered()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  cmd.Undo().AssertSuccess();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void WQtColorGradientEditDlg::on_actionRedo_triggered()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  cmd.Redo().AssertSuccess();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void WQtColorGradientEditDlg::on_ButtonOk_clicked()
{
  QDialog::accept();
}

void WQtColorGradientEditDlg::on_ButtonCancel_clicked()
{
  cancel();
}

void WQtColorGradientEditDlg::on_ButtonUndo_clicked()
{
  on_actionUndo_triggered();
}

void WQtColorGradientEditDlg::on_ButtonRedo_clicked()
{
  on_actionRedo_triggered();
}
